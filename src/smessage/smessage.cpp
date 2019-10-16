// Copyright (c) 2014 The ShadowCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/*
Notes:
    Running with -debug could leave to and from address hashes and public keys in the log.


    parameters:
        -nosmsg             Disable secure messaging (fNoSmsg)
        -debugsmsg          Show extra debug messages (fDebugSmsg)
        -smsgscanchain      Scan the block chain for public key addresses on startup


    Wallet Locked
        A copy of each incoming message is stored in bucket files ending in _wl.dat
        wl (wallet locked) bucket files are deleted if they expire, like normal buckets
        When the wallet is unlocked all the messages in wl files are scanned.


    Address Whitelist
        Owned Addresses are stored in smsgAddresses vector
        Saved to smsg.ini
        Modify options using the smsglocalkeys rpc command or edit the smsg.ini file (with client closed)


*/

#include <smessage/smessage.h>

#include <stdint.h>
#include <time.h>
#include <map>
#include <stdexcept>
#include <sstream>
#include <errno.h>

#include <secp256k1.h>
#include "crypto/sha512.h"
#include "crypto/hmac_sha256.h"

#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>


#include "base58.h"
#include "init.h"
#include "key_io.h"
#include <logging.h>
#include <net_processing.h>
#include <netmessagemaker.h>

#include "rpc/protocol.h"
#include "dbwrapper.h"
#include "smessage/lz4.h"
#include "smessage/xxhash.h"
#include "validation.h"
//#include "xxhash/xxhash.c"
#include <secp256k1_recovery.h>
#ifdef ENABLE_WALLET
#include "wallet/crypter.h"
#include "wallet/db.h"
#include <wallet/wallet.h>
#endif

static secp256k1_context *ctx = NULL;

/*
//! anonymous namespace
namespace
{
// Global secp256k1_context object used for verification. 
secp256k1_context* secp256k1_context_verify = nullptr;
} // namespace
*/
static bool DeriveKey(secure_buffer &vchP, const CKey &keyR, const CPubKey &cpkDestK)
{
/*
    secp256k1_pubkey pubkey;
    unsigned char out[32];
    //vchP.assign(cpkDestK.begin(), cpkDestK.end());
    secure_buffer r(keyR.begin(), keyR.end());
    memcpy(out, keyR.begin(), 32);
    if (!secp256k1_ec_pubkey_parse(secp256k1_context_verify, &pubkey, cpkDestK.data(), vchP.size())) {
        return false;
    }
    return secp256k1_ec_pubkey_tweak_mul(secp256k1_context_verify, &pubkey, out);   
    //,&vchP[0]
    
    //return secp256k1_ec_pubkey_tweak_mul(secp256k1_context_verify, &pubkey, &r[0]);
*/

    vchP.assign(cpkDestK.begin(), cpkDestK.end());
    secure_buffer r(keyR.begin(), keyR.end());
    return secp256k1_ec_pubkey_tweak_mul(ctx, (secp256k1_pubkey*)&vchP[0], &r[0]);
}


// TODO: For buckets older than current, only need to store no. messages and hash in memory

boost::signals2::signal<void (SecMsgStored& inboxHdr)>  NotifySecMsgInboxChanged;
boost::signals2::signal<void (SecMsgStored& outboxHdr)> NotifySecMsgOutboxChanged;
boost::signals2::signal<void ()> NotifySecMsgWalletUnlocked;

bool fSecMsgEnabled = false;
boost::thread secureMsgThread;

std::map<int64_t, SecMsgBucket> smsgBuckets;
std::vector<SecMsgAddress>      smsgAddresses;
SecMsgOptions                   smsgOptions;

uint32_t nPeerIdCounter = 1;

CCriticalSection cs_smsg;
CCriticalSection cs_smsgDB;
CCriticalSection cs_smsgThreads;

leveldb::DB *smsgDB = NULL;


namespace fs = boost::filesystem;


void SecMsgBucket::hashBucket()
{
    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecMsgBucket::hashBucket()\n");

    timeChanged = GetTime();

    std::set<SecMsgToken>::iterator it;

    void* state = XXH32_init(1);

    for (it = setTokens.begin(); it != setTokens.end(); ++it)
    {
        XXH32_update(state, it->sample, 8);
    }

    hash = XXH32_digest(state);

    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("Hashed %d messages, hash %u\n", (int) setTokens.size(), hash);
}


bool SecMsgDB::Open(const char* pszMode)
{
    if (smsgDB)
    {
        pdb = smsgDB;
        return true;
    }

    bool fCreate = strchr(pszMode, 'c');

    fs::path fullpath = GetDataDir() / "smsgDB";

    if (!fCreate
        && (!fs::exists(fullpath)
            || !fs::is_directory(fullpath)))
    {
        LogPrintf("SecMsgDB::open() - DB does not exist.\n");
        return false;
    }

    leveldb::Options options;
    options.create_if_missing = fCreate;
    leveldb::Status s = leveldb::DB::Open(options, fullpath.string(), &smsgDB);

    if (!s.ok())
    {
        LogPrintf("SecMsgDB::open() - Error opening db: %s.\n", s.ToString().c_str());
        return false;
    }

    pdb = smsgDB;

    return true;
}


class SecMsgBatchScanner : public leveldb::WriteBatch::Handler
{
public:
    std::string needle;
    bool* deleted;
    std::string* foundValue;
    bool foundEntry;

    SecMsgBatchScanner() : foundEntry(false) {}

    virtual void Put(const leveldb::Slice& key, const leveldb::Slice& value)
    {
        if (key.ToString() == needle)
        {
            foundEntry = true;
            *deleted = false;
            *foundValue = value.ToString();
        }
    }

    virtual void Delete(const leveldb::Slice& key)
    {
        if (key.ToString() == needle)
        {
            foundEntry = true;
            *deleted = true;
        }
    }
};

// When performing a read, if we have an active batch we need to check it first
// before reading from the database, as the rest of the code assumes that once
// a database transaction begins reads are consistent with it. It would be good
// to change that assumption in future and avoid the performance hit, though in
// practice it does not appear to be large.
bool SecMsgDB::ScanBatch(const CDataStream& key, std::string* value, bool* deleted) const
{
    if (!activeBatch)
        return false;

    *deleted = false;
    SecMsgBatchScanner scanner;
    scanner.needle = key.str();
    scanner.deleted = deleted;
    scanner.foundValue = value;
    leveldb::Status s = activeBatch->Iterate(&scanner);
    if (!s.ok())
    {
        LogPrintf("SecMsgDB ScanBatch error: %s\n", s.ToString().c_str());
        return false;
    }

    return scanner.foundEntry;
}

bool SecMsgDB::TxnBegin()
{
    if (activeBatch)
        return true;
    activeBatch = new leveldb::WriteBatch();
    return true;
}

bool SecMsgDB::TxnCommit()
{
    if (!activeBatch)
        return false;

    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;
    leveldb::Status status = pdb->Write(writeOptions, activeBatch);
    delete activeBatch;
    activeBatch = NULL;

    if (!status.ok())
    {
        LogPrintf("SecMsgDB batch commit failure: %s\n", status.ToString().c_str());
        return false;
    }

    return true;
}

bool SecMsgDB::TxnAbort()
{
    delete activeBatch;
    activeBatch = NULL;
    return true;
}

bool SecMsgDB::ReadPK(CKeyID& addr, CPubKey& pubkey)
{
    if (!pdb)
        return false;

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    ssKey.reserve(sizeof(addr) + 2);
    ssKey << 'p';
    ssKey << 'k';
    ssKey << addr;
    std::string strValue;

    bool readFromDb = true;
    if (activeBatch)
    {
        // -- check activeBatch first
        bool deleted = false;
        readFromDb = ScanBatch(ssKey, &strValue, &deleted) == false;
        if (deleted)
            return false;
    }

    if (readFromDb)
    {
        leveldb::Status s = pdb->Get(leveldb::ReadOptions(), ssKey.str(), &strValue);
        if (!s.ok())
        {
            if (s.IsNotFound())
                return false;
            LogPrintf("LevelDB read failure: %s\n", s.ToString().c_str());
            return false;
        }
    }

    try {
        CDataStream ssValue(strValue.data(), strValue.data() + strValue.size(), SER_DISK, CLIENT_VERSION);
        ssValue >> pubkey;
    } catch (std::exception& e) {
        LogPrintf("SecMsgDB::ReadPK() unserialize threw: %s.\n", e.what());
        return false;
    }

    return true;
}

bool SecMsgDB::WritePK(CKeyID& addr, CPubKey& pubkey)
{
    if (!pdb)
        return false;

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    ssKey.reserve(sizeof(addr) + 2);
    ssKey << 'p';
    ssKey << 'k';
    ssKey << addr;
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);
    ssValue.reserve(sizeof(pubkey));
    ssValue << pubkey;

    if (activeBatch)
    {
        activeBatch->Put(ssKey.str(), ssValue.str());
        return true;
    }

    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;
    leveldb::Status s = pdb->Put(writeOptions, ssKey.str(), ssValue.str());
    if (!s.ok())
    {
        LogPrintf("SecMsgDB write failure: %s\n", s.ToString().c_str());
        return false;
    }

    return true;
}

bool SecMsgDB::ExistsPK(CKeyID& addr)
{
    if (!pdb)
        return false;

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    ssKey.reserve(sizeof(addr)+2);
    ssKey << 'p';
    ssKey << 'k';
    ssKey << addr;
    std::string unused;

    if (activeBatch)
    {
        bool deleted;
        if (ScanBatch(ssKey, &unused, &deleted) && !deleted)
        {
            return true;
        }
    }

    leveldb::Status s = pdb->Get(leveldb::ReadOptions(), ssKey.str(), &unused);
    return s.IsNotFound() == false;
}


bool SecMsgDB::NextSmesg(leveldb::Iterator* it, std::string& prefix, unsigned char* chKey, SecMsgStored& smsgStored)
{
    if (!pdb)
        return false;

    if (!it->Valid()) // first run
        it->Seek(prefix);
    else
        it->Next();

    if (!(it->Valid()
        && it->key().size() == 18
        && memcmp(it->key().data(), prefix.data(), 2) == 0))
        return false;

    memcpy(chKey, it->key().data(), 18);

    try {
        CDataStream ssValue(it->value().data(), it->value().data() + it->value().size(), SER_DISK, CLIENT_VERSION);
        ssValue >> smsgStored;
    } catch (std::exception& e) {
        LogPrintf("SecMsgDB::NextSmesg() unserialize threw: %s.\n", e.what());
        return false;
    }

    return true;
}

bool SecMsgDB::NextSmesgKey(leveldb::Iterator* it, std::string& prefix, unsigned char* chKey)
{
    if (!pdb)
        return false;

    if (!it->Valid()) // first run
        it->Seek(prefix);
    else
        it->Next();

    if (!(it->Valid()
        && it->key().size() == 18
        && memcmp(it->key().data(), prefix.data(), 2) == 0))
        return false;

    memcpy(chKey, it->key().data(), 18);

    return true;
}

bool SecMsgDB::ReadSmesg(unsigned char* chKey, SecMsgStored& smsgStored)
{
    if (!pdb)
        return false;

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    ssKey.write((const char*)chKey, 18);
    std::string strValue;

    bool readFromDb = true;
    if (activeBatch)
    {
        // -- check activeBatch first
        bool deleted = false;
        readFromDb = ScanBatch(ssKey, &strValue, &deleted) == false;
        if (deleted)
            return false;
    }

    if (readFromDb)
    {
        leveldb::Status s = pdb->Get(leveldb::ReadOptions(), ssKey.str(), &strValue);
        if (!s.ok())
        {
            if (s.IsNotFound())
                return false;
            LogPrintf("LevelDB read failure: %s\n", s.ToString().c_str());
            return false;
        }
    }

    try {
        CDataStream ssValue(strValue.data(), strValue.data() + strValue.size(), SER_DISK, CLIENT_VERSION);
        ssValue >> smsgStored;
    } catch (std::exception& e) {
        LogPrintf("SecMsgDB::ReadSmesg() unserialize threw: %s.\n", e.what());
        return false;
    }

    return true;
}

bool SecMsgDB::WriteSmesg(unsigned char* chKey, SecMsgStored& smsgStored)
{
    if (!pdb)
        return false;

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    ssKey.write((const char*)chKey, 18);
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);
    ssValue << smsgStored;

    if (activeBatch)
    {
        activeBatch->Put(ssKey.str(), ssValue.str());
        return true;
    }

    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;
    leveldb::Status s = pdb->Put(writeOptions, ssKey.str(), ssValue.str());
    if (!s.ok())
    {
        LogPrintf("SecMsgDB write failed: %s\n", s.ToString().c_str());
        return false;
    }

    return true;
}

bool SecMsgDB::ExistsSmesg(unsigned char* chKey)
{
    if (!pdb)
        return false;

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    ssKey.write((const char*)chKey, 18);
    std::string unused;

    if (activeBatch)
    {
        bool deleted;
        if (ScanBatch(ssKey, &unused, &deleted) && !deleted)
        {
            return true;
        }
    }

    leveldb::Status s = pdb->Get(leveldb::ReadOptions(), ssKey.str(), &unused);
    return s.IsNotFound() == false;
    return true;
}

bool SecMsgDB::EraseSmesg(unsigned char* chKey)
{
    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    ssKey.write((const char*)chKey, 18);

    if (activeBatch)
    {
        activeBatch->Delete(ssKey.str());
        return true;
    }

    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;
    leveldb::Status s = pdb->Delete(writeOptions, ssKey.str());

    if (s.ok() || s.IsNotFound())
        return true;
    LogPrintf("SecMsgDB erase failed: %s\n", s.ToString().c_str());
    return false;
}


static boost::atomic_uint nThreadCount(0);
void ThreadSecureMsg(void* parg)
{
    nThreadCount.fetch_add(1, boost::memory_order_relaxed);
    // -- bucket management thread
    RenameThread("talkcoin-smsg"); // Make this thread recognisable
const CNetMsgMaker msgMaker(PROTOCOL_VERSION);


    std::vector<unsigned char> vchKey;
    SecMsgStored smsgStored;

    std::string sPrefix("qm");
    unsigned char chKey[18];


    while (fSecMsgEnabled) {
        int64_t now = GetTime();

        int64_t cutoffTime = now - SMSG_RETENTION;
        {
            LOCK(cs_smsg);

            for (std::map<int64_t, SecMsgBucket>::iterator it(smsgBuckets.begin()); it != smsgBuckets.end(); it++) {
                //if (LogInstance().WillLogCategory(BCLog::SMSG))
                //    LogPrintf("Checking bucket %"PRId64", size %"PRIszu" \n", it->first, it->second.setTokens.size());
                if (it->first < cutoffTime) {
                    if (LogInstance().WillLogCategory(BCLog::SMSG))
                        LogPrintf("Removing bucket %d\n", (int) it->first);

                    std::string fileName = boost::lexical_cast<std::string>(it->first);

                    fs::path fullPath = GetDataDir() / "smsgStore" / (fileName + "_01.dat");
                    if (fs::exists(fullPath)) {
                        try {
                            fs::remove(fullPath);
                        }
                        catch (const fs::filesystem_error& ex) {
                            LogPrintf("Error removing bucket file %s.\n", ex.what());
                        }
                    }
                    else {
                        LogPrintf("Path %s does not exist\n", fullPath.string().c_str());
                    }

                    // -- look for a wl file, it stores incoming messages when wallet is locked
                    fullPath = GetDataDir() / "smsgStore" / (fileName + "_01_wl.dat");
                    if (fs::exists(fullPath)) {
                        try {
                            fs::remove(fullPath);
                        }
                        catch (const fs::filesystem_error& ex) {
                            LogPrintf("Error removing wallet locked file %s.\n", ex.what());
                        }
                    }

                    smsgBuckets.erase(it);
                }
                else if (it->second.nLockCount > 0) { // -- tick down nLockCount, so will eventually expire if peer never sends data
                    it->second.nLockCount--;

                    if (it->second.nLockCount == 0) {    // lock timed out
                        uint32_t nPeerId     = it->second.nLockPeerId;
                        int64_t  ignoreUntil = GetTime() + SMSG_TIME_IGNORE;

                        if (LogInstance().WillLogCategory(BCLog::SMSG))
                            LogPrintf("Lock on bucket %d for peer %u timed out.\n", (int) it->first, nPeerId);

                        // -- look through the nodes for the peer that locked this bucket
                        g_connman->ForEachNode([nPeerId, ignoreUntil,it, msgMaker](CNode* pnode) {
                            if (pnode->smsgData.nPeerId == nPeerId){
                            
								pnode->smsgData.ignoreUntil = ignoreUntil;

								// -- alert peer that they are being ignored
								std::vector<unsigned char> vchData;
								vchData.resize(8);
								memcpy(&vchData[0], &ignoreUntil, 8);
								g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::SMSGIGNORE, vchData));

								if (LogInstance().WillLogCategory(BCLog::SMSG))
									LogPrintf("Lock on bucket %d for peer %u timed out.\n", (int) it->first, nPeerId);
							}

                        });
                        it->second.nLockPeerId = 0;
                    } // if (it->second.nLockCount == 0)
                } // ! if (it->first < cutoffTime)
            }
        } // LOCK(cs_smsg);


        SecMsgDB dbOutbox;
        leveldb::Iterator* it;
        {
            LOCK(cs_smsgDB);

            if (!dbOutbox.Open("cr+"))
                continue;

            // -- fifo (smallest key first)
            it = dbOutbox.pdb->NewIterator(leveldb::ReadOptions());
        }
        // -- break up lock, SecureMsgSetHash will take long

        for (;;)
        {
            {
                LOCK(cs_smsgDB);
                if (!dbOutbox.NextSmesg(it, sPrefix, chKey, smsgStored))
                    break;
            }

            SecureMessageHeader smsg(&smsgStored.vchMessage[0]);
            const unsigned char* pPayload = &smsgStored.vchMessage[SMSG_HDR_LEN];

            // -- message is removed here, no matter what
            {
                LOCK(cs_smsgDB);
                dbOutbox.EraseSmesg(chKey);
            }

            // -- add to message store
            {
                LOCK(cs_smsg);
                if (SecureMsgStore(smsg, pPayload, true) != 0) {
                    LogPrintf("SecMsgPow: Could not place message in buckets, message removed.\n");
                    continue;
                }
            }

            // -- test if message was sent to self
            if (SecureMsgScanMessage(smsg, pPayload, true) != 0) {
                // message recipient is not this node (or failed)
            }
        }

        {
            LOCK(cs_smsg);
            delete it;
        }


        try {
            boost::this_thread::sleep_for(boost::chrono::seconds(SMSG_THREAD_DELAY)); // check every SMSG_THREAD_DELAY seconds
        }
        catch(boost::thread_interrupted &e) {
            break;
        }
    }

    LogPrintf("ThreadSecureMsg exited.\n");
    nThreadCount.fetch_sub(1, boost::memory_order_relaxed);
}


int SecureMsgBuildBucketSet()
{
    /*
        Build the bucket set by scanning the files in the smsgStore dir.

        smsgBuckets should be empty
    */

    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecureMsgBuildBucketSet()\n");

    int64_t  now            = GetTime();
    uint32_t nFiles         = 0;
    uint32_t nMessages      = 0;

    fs::path pathSmsgDir = GetDataDir() / "smsgStore";
    fs::directory_iterator itend;


    if (!fs::exists(pathSmsgDir)
        || !fs::is_directory(pathSmsgDir))
    {
        if (!fs::create_directory(pathSmsgDir)) {
            LogPrintf("Message store directory does not exist and could not be created.\n");
            return 1;
        }
    }

    for (fs::directory_iterator itd(pathSmsgDir) ; itd != itend ; ++itd) {
        if (!fs::is_regular_file(itd->status()))
            continue;

        std::string fileType = (*itd).path().extension().string();

        if (fileType.compare(".dat") != 0)
            continue;

        std::string fileName = (*itd).path().filename().string();


        if (LogInstance().WillLogCategory(BCLog::SMSG))
            LogPrintf("Processing file: %s.\n", fileName.c_str());

        nFiles++;

        // TODO files must be split if > 2GB
        // time_noFile.dat
        size_t sep = fileName.find_first_of("_");
        if (sep == std::string::npos)
            continue;

        std::string stime = fileName.substr(0, sep);

        int64_t fileTime = boost::lexical_cast<int64_t>(stime);

        if (fileTime < now - SMSG_RETENTION) {
            LogPrintf("Dropping file %s, expired.\n", fileName.c_str());
            try {
                fs::remove((*itd).path());
            }
            catch (const fs::filesystem_error& ex) {
                LogPrintf("Error removing bucket file %s, %s.\n", fileName.c_str(), ex.what());
            }
            continue;
        }

        if (boost::algorithm::ends_with(fileName, "_wl.dat")) {
            if (LogInstance().WillLogCategory(BCLog::SMSG))
                LogPrintf("Skipping wallet locked file: %s.\n", fileName.c_str());
            continue;
        }

        SecureMessageHeader smsg;
        std::set<SecMsgToken>& tokenSet = smsgBuckets[fileTime].setTokens;

        {
            LOCK(cs_smsg);
            FILE *fp;

            if (!(fp = fopen((*itd).path().string().c_str(), "rb"))) {
                LogPrintf("Error opening file: %s (%d)\n", strerror(errno), __LINE__);
                continue;
            }

            for (;;) {
                long int ofs = ftell(fp);
                SecMsgToken token;
                token.offset = ofs;
                if (fread(&smsg, sizeof(unsigned char), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN) {
                    if (!feof(fp)) {
                        LogPrintf("fread header failed\n");
                    }
                    break;
                }
                token.timestamp = smsg.timestamp;
                if (smsg.nPayload < 8)
                    continue;

                if (fread(token.sample, sizeof(unsigned char), 8, fp) != 8) {
                    LogPrintf("fread data failed: %s\n", strerror(errno));
                    break;
                }

                if (fseek(fp, smsg.nPayload-8, SEEK_CUR) != 0) {
                    LogPrintf("fseek, strerror: %s.\n", strerror(errno));
                    break;
                }

                tokenSet.insert(token);
            }

            fclose(fp);
        }
        smsgBuckets[fileTime].hashBucket();

        nMessages += tokenSet.size();

        if (LogInstance().WillLogCategory(BCLog::SMSG))
            LogPrintf("Bucket %d contains %d messages.\n", (int) fileTime, (int) tokenSet.size());
    }

    LogPrintf("Processed %u files, loaded %d buckets containing %u messages.\n", nFiles, (int) smsgBuckets.size(), nMessages);

    return 0;
}

int SecureMsgAddWalletAddresses()
{
	std::shared_ptr<CWallet> pwallet = GetWallet("default");
	if (!pwallet)
	   return 0;

    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecureMsgAddWalletAddresses()\n");

    std::string sAnonPrefix("ao");

    uint32_t nAdded = 0;
    LOCK(pwallet->cs_wallet);
    for (const auto& entry : pwallet->mapAddressBook)
    {
        if (IsMine(*pwallet, entry.first) != ISMINE_SPENDABLE)
            continue;

        // -- skip addresses for anon outputs
        if (entry.second.purpose.compare(0, sAnonPrefix.length(), sAnonPrefix) == 0)
            continue;

        // TODO: skip addresses for stealth transactions

		CTxDestination dest = entry.first;

		if (!IsValidDestination(dest))
			continue;

        std::string address = EncodeDestination(dest);

        bool fExists        = 0;
        for (std::vector<SecMsgAddress>::iterator it = smsgAddresses.begin(); it != smsgAddresses.end(); ++it)
        {
            if (address != it->sAddress)
                continue;
            fExists = 1;
            break;
        }

        if (fExists)
            continue;

        bool recvEnabled    = 1;
        bool recvAnon       = 1;

        smsgAddresses.push_back(SecMsgAddress(address, recvEnabled, recvAnon));
        nAdded++;
    }

    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("Added %u addresses to whitelist.\n", nAdded);

    return 0;
}


int SecureMsgReadIni()
{
    if (!fSecMsgEnabled)
        return false;

    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecureMsgReadIni()\n");

    fs::path fullpath = GetDataDir() / "talkcoin.conf";

    FILE *fp;
    errno = 0;
    if (!(fp = fopen(fullpath.string().c_str(), "r")))
    {
        LogPrintf("Error opening file: %s (%d)\n", strerror(errno), __LINE__);
        return 1;
    }

    char cLine[512];
    char *pName, *pValue;

    char cAddress[64];
    int addrRecv, addrRecvAnon;

    while (fgets(cLine, 512, fp))
    {
        cLine[strcspn(cLine, "\n")] = '\0';
        cLine[strcspn(cLine, "\r")] = '\0';
        cLine[511] = '\0'; // for safety

        // -- check that line contains a name value pair and is not a comment, or section header
        if (cLine[0] == '#' || cLine[0] == '[' || strcspn(cLine, "=") < 1)
            continue;

        if (!(pName = strtok(cLine, "="))
            || !(pValue = strtok(NULL, "=")))
            continue;

        if (strcmp(pName, "newAddressRecv") == 0)
        {
            smsgOptions.fNewAddressRecv = (strcmp(pValue, "true") == 0) ? true : false;
        } else
        if (strcmp(pName, "newAddressAnon") == 0)
        {
            smsgOptions.fNewAddressAnon = (strcmp(pValue, "true") == 0) ? true : false;
        } else
        if (strcmp(pName, "key") == 0)
        {
            int rv = sscanf(pValue, "%64[^|]|%d|%d", cAddress, &addrRecv, &addrRecvAnon);
            if (rv == 3)
            {
                smsgAddresses.push_back(SecMsgAddress(std::string(cAddress), addrRecv, addrRecvAnon));
            } else
            {
                LogPrintf("Could not parse key line %s, rv %d.\n", pValue, rv);
            }
        } else
        {
            LogPrintf("Unknown setting name: '%s'.", pName);
        }
    }

    LogPrintf("Loaded %d addresses.\n", (int) smsgAddresses.size());

    fclose(fp);

    return 0;
}

int SecureMsgWriteIni()
{
    if (!fSecMsgEnabled)
        return false;

    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecureMsgWriteIni()\n");

    fs::path fullpath = GetDataDir() / "smsg.ini~";

    FILE *fp;
    errno = 0;
    if (!(fp = fopen(fullpath.string().c_str(), "w")))
    {
        LogPrintf("Error opening file: %s (%d)\n", strerror(errno), __LINE__);
        return 1;
    }

    if (fwrite("[Options]\n", sizeof(char), 10, fp) != 10)
    {
        LogPrintf("fwrite error: %s\n", strerror(errno));
        fclose(fp);
        return false;
    }

    if (fprintf(fp, "newAddressRecv=%s\n", smsgOptions.fNewAddressRecv ? "true" : "false") < 0
        || fprintf(fp, "newAddressAnon=%s\n", smsgOptions.fNewAddressAnon ? "true" : "false") < 0)
    {
        LogPrintf("fprintf error: %s\n", strerror(errno));
        fclose(fp);
        return false;
    }

    if (fwrite("\n[Keys]\n", sizeof(char), 8, fp) != 8)
    {
        LogPrintf("fwrite error: %s\n", strerror(errno));
        fclose(fp);
        return false;
    }
    for (std::vector<SecMsgAddress>::iterator it = smsgAddresses.begin(); it != smsgAddresses.end(); ++it)
    {
        errno = 0;
        if (fprintf(fp, "key=%s|%d|%d\n", it->sAddress.c_str(), it->fReceiveEnabled, it->fReceiveAnon) < 0)
        {
            LogPrintf("fprintf error: %s\n", strerror(errno));
            continue;
        }
    }


    fclose(fp);


    try {
        fs::path finalpath = GetDataDir() / "smsg.ini";
        fs::rename(fullpath, finalpath);
    } catch (const fs::filesystem_error& ex)
    {
        LogPrintf("Error renaming file %s, %s.\n", fullpath.string().c_str(), ex.what());
    }
    return 0;
}


/** called from AppInit2() in init.cpp */
bool SecureMsgStart(bool fDontStart, bool fScanChain) {
    if (fDontStart) {
        LogPrintf("Secure messaging not started.\n");
        return false;
    }

    LogPrintf("Secure messaging starting.\n");
    SecureMsgEnable();

    if (fScanChain) {
        SecureMsgScanBlockChain();
    }

    return true;
}

/** called from AppInit2() in init.cpp */
bool SecureMsgStop() {
    LogPrintf("Secure messaging stopping.\n");
    SecureMsgDisable();
    return true;
}

bool SecureMsgEnable() {
    // -- start secure messaging at runtime
    if (fSecMsgEnabled) {
        LogPrintf("SecureMsgEnable: secure messaging is already enabled.\n");
        return false;
    }
    const CNetMsgMaker msgMaker(PROTOCOL_VERSION);

    {
        LOCK(cs_smsg);
        fSecMsgEnabled = true;

        smsgAddresses.clear(); // should be empty already
        if (SecureMsgReadIni() != 0)
            LogPrintf("Failed to read smsg.ini\n");

        if (smsgAddresses.size() < 1) {
            LogPrintf("No address keys loaded.\n");
            if (SecureMsgAddWalletAddresses() != 0)
                LogPrintf("Failed to load addresses from wallet.\n");
        }

        smsgBuckets.clear(); // should be empty already

        if (SecureMsgBuildBucketSet() != 0) {
            LogPrintf("SecureMsgEnable: could not load bucket sets, secure messaging disabled.\n");
            fSecMsgEnabled = false;
            return false;
        }
    } // LOCK(cs_smsg);

    // -- start threads
    secureMsgThread = boost::thread(&ThreadSecureMsg, (void*) NULL);
    if (secureMsgThread.get_id() == boost::this_thread::get_id()) {
        LogPrintf("SecureMsgEnable could not start threads, secure messaging disabled.\n");
        fSecMsgEnabled = false;
        return false;
    }

    // -- ping each peer, don't know which have messaging enabled
    {
		g_connman->ForEachNode([msgMaker](CNode* pnode) {
			g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::SMSGPING));
			g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::SMSGPONG));
		});
    }

    LogPrintf("Secure messaging enabled.\n");
    return true;
}


bool SecureMsgDisable() {
    // -- stop secure messaging at runtime
    if (!fSecMsgEnabled) {
        LogPrintf("SecureMsgDisable: secure messaging is already disabled.\n");
        return false;
    }

const CNetMsgMaker msgMaker(PROTOCOL_VERSION);


    {
        LOCK(cs_smsg);
        fSecMsgEnabled = false;

        // -- clear smsgBuckets
        std::map<int64_t, SecMsgBucket>::iterator it;
        it = smsgBuckets.begin();
        for (it = smsgBuckets.begin(); it != smsgBuckets.end(); ++it) {
            it->second.setTokens.clear();
        }
        smsgBuckets.clear();

        // -- tell each smsg enabled peer that this node is disabling
        {
			g_connman->ForEachNode([msgMaker](CNode* pnode) {
				if (pnode->smsgData.fEnabled){
					g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::SMSGDISABLED));
					pnode->smsgData.fEnabled = false;
				}
			});
        }

        if (SecureMsgWriteIni() != 0)
            LogPrintf("Failed to save smsg.ini\n");

        smsgAddresses.clear();

    } // LOCK(cs_smsg);

    secureMsgThread.interrupt();
    if (secureMsgThread.joinable())
        secureMsgThread.join();

    if (smsgDB) {
        LOCK(cs_smsgDB);
        delete smsgDB;
        smsgDB = NULL;
    }


    LogPrintf("Secure messaging disabled.\n");
    return true;
}


bool SecureMsgReceiveData(CNode* pfrom, std::string strCommand, CDataStream& vRecv)
{
    /*
        Called from ProcessMessage
        Runs in ThreadMessageHandler2
    */
    const CNetMsgMaker msgMaker(PROTOCOL_VERSION);

    LOCK(pfrom->cs_addrName);
    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecureMsgReceiveData() %s %s.\n", pfrom->addrName.c_str(), strCommand.c_str());

    {
    // break up?
    LOCK(cs_smsg);

    if (strCommand == NetMsgType::SMSGINV)
    {
        std::vector<unsigned char> vchData;
        vRecv >> vchData;

        if (vchData.size() < 4)
        {
			LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 1);

            return false; // not enough data received to be a valid smsgInv
        }

        int64_t now = GetTime();

        if (now < pfrom->smsgData.ignoreUntil)
        {
            if (LogInstance().WillLogCategory(BCLog::SMSG))
                LogPrintf("Node is ignoring peer %u until %d.\n", pfrom->smsgData.nPeerId, (int) pfrom->smsgData.ignoreUntil);
            return false;
        }

        uint32_t nBuckets       = smsgBuckets.size();
        uint32_t nLocked        = 0;    // no. of locked buckets on this node
        uint32_t nInvBuckets;           // no. of bucket headers sent by peer in smsgInv
        memcpy(&nInvBuckets, &vchData[0], 4);
        if (LogInstance().WillLogCategory(BCLog::SMSG))
            LogPrintf("Remote node sent %d bucket headers, this has %d.\n", nInvBuckets, nBuckets);


        // -- Check no of buckets:
        if (nInvBuckets > (SMSG_RETENTION / SMSG_BUCKET_LEN) + 1) // +1 for some leeway
        {
            LogPrintf("Peer sent more bucket headers than possible %u, %u.\n", nInvBuckets, (SMSG_RETENTION / SMSG_BUCKET_LEN));
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 1);
            return false;
        }

        if (vchData.size() < 4 + nInvBuckets*16)
        {
            LogPrintf("Remote node did not send enough data.\n");
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 1);
            return false;
        }

        std::vector<unsigned char> vchDataOut;
        vchDataOut.reserve(4 + 8 * nInvBuckets); // reserve max possible size
        vchDataOut.resize(4);
        uint32_t nShowBuckets = 0;


        unsigned char *p = &vchData[4];
        for (uint32_t i = 0; i < nInvBuckets; ++i)
        {
            int64_t time;
            uint32_t ncontent, hash;
            memcpy(&time, p, 8);
            memcpy(&ncontent, p+8, 4);
            memcpy(&hash, p+12, 4);

            p += 16;

            // Check time valid:
            if (time < now - SMSG_RETENTION)
            {
                if (LogInstance().WillLogCategory(BCLog::SMSG))
                    LogPrintf("Not interested in peer bucket %d, has expired.\n", (int) time);

                if (time < now - SMSG_RETENTION - SMSG_TIME_LEEWAY){
					LOCK(cs_main);
                    Misbehaving(pfrom->GetId(), 1);
                }
                continue;
            }
            if (time > now + SMSG_TIME_LEEWAY)
            {
                if (LogInstance().WillLogCategory(BCLog::SMSG))
                    LogPrintf("Not interested in peer bucket %d, in the future.\n", (int) time);
                LOCK(cs_main);
                Misbehaving(pfrom->GetId(), 1);
                continue;
            }

            if (ncontent < 1)
            {
                if (LogInstance().WillLogCategory(BCLog::SMSG))
                    LogPrintf("Peer sent empty bucket, ignore %d %u %u.\n", (int32_t) time, ncontent, hash);
                continue;
            }

            if (LogInstance().WillLogCategory(BCLog::SMSG))
            {
                LogPrintf("peer bucket %d %u %u.\n", (int32_t) time, ncontent, hash);
                //std::cout << "this bucket " << time << ", " << smsgBuckets[time].setTokens.size() << ", " << smsgBuckets[time].hash << std::endl;
            }

            if (smsgBuckets[time].nLockCount > 0)
            {
                if (LogInstance().WillLogCategory(BCLog::SMSG))
                    //std::cout << "Bucket is locked " << smsgBuckets[time].nLockCount << " waiting for peer " << smsgBuckets[time].nLockPeerId << " to send data." << std::endl;
                    LogPrintf("Bucket is locked  %d, waiting for peer %d  to send data.\n", smsgBuckets[time].nLockCount, smsgBuckets[time].nLockPeerId);
                nLocked++;
                continue;
            }

            // -- if this node has more than the peer node, peer node will pull from this
            //    if then peer node has more this node will pull fom peer
            if (smsgBuckets[time].setTokens.size() < ncontent
                || (smsgBuckets[time].setTokens.size() == ncontent
                    && smsgBuckets[time].hash != hash)) // if same amount in buckets check hash
            {
                if (LogInstance().WillLogCategory(BCLog::SMSG))
                    LogPrintf("Requesting contents of bucket %d.\n", (int32_t) time);

                uint32_t sz = vchDataOut.size();
                vchDataOut.resize(sz + 8);
                memcpy(&vchDataOut[sz], &time, 8);

                nShowBuckets++;
            }
        }

        // TODO: should include hash?
        memcpy(&vchDataOut[0], &nShowBuckets, 4);
        if (vchDataOut.size() > 4)
        {
			g_connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::SMSGSHOW, vchDataOut));
        } else
        if (nLocked < 1) // Don't report buckets as matched if any are locked
        {
            // -- peer has no buckets we want, don't send them again until something changes
            //    peer will still request buckets from this node if needed (< ncontent)
            vchDataOut.resize(8);
            memcpy(&vchDataOut[0], &now, 8);
            g_connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::SMSGMATCH, vchDataOut));
            if (LogInstance().WillLogCategory(BCLog::SMSG))
                LogPrintf("Sending smsgMatch, %d.\n", (int32_t) now);
        }

    } else
    if (strCommand == NetMsgType::SMSGSHOW)
    {
        std::vector<unsigned char> vchData;
        vRecv >> vchData;

        if (vchData.size() < 4)
            return false;

        uint32_t nBuckets;
        memcpy(&nBuckets, &vchData[0], 4);

        if (vchData.size() < 4 + nBuckets * 8)
            return false;

        if (LogInstance().WillLogCategory(BCLog::SMSG))
            LogPrintf("smsgShow: peer wants to see content of %u buckets.\n", nBuckets);

        std::map<int64_t, SecMsgBucket>::iterator itb;
        std::set<SecMsgToken>::iterator it;

        std::vector<unsigned char> vchDataOut;
        int64_t time;
        unsigned char* pIn = &vchData[4];
        for (uint32_t i = 0; i < nBuckets; ++i, pIn += 8)
        {
            memcpy(&time, pIn, 8);

            itb = smsgBuckets.find(time);
            if (itb == smsgBuckets.end())
            {
                if (LogInstance().WillLogCategory(BCLog::SMSG))
                    LogPrintf("Don't have bucket %d.\n", (int32_t) time);
                continue;
            }

            std::set<SecMsgToken>& tokenSet = (*itb).second.setTokens;

            try { vchDataOut.resize(8 + 16 * tokenSet.size()); } catch (std::exception& e)
            {
                std::cout << "vchDataOut.resize " << (8 + 16 * tokenSet.size()) << " threw " << e.what() << std::endl;
                continue;
            }
            memcpy(&vchDataOut[0], &time, 8);

            unsigned char* p = &vchDataOut[8];
            for (it = tokenSet.begin(); it != tokenSet.end(); ++it)
            {
                memcpy(p, &it->timestamp, 8);
                memcpy(p+8, &it->sample, 8);

                p += 16;
            }
            g_connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::SMSGHAVE, vchDataOut));
        }


    } else
    if (strCommand == NetMsgType::SMSGHAVE)
    {
        // -- peer has these messages in bucket
        std::vector<unsigned char> vchData;
        vRecv >> vchData;

        if (vchData.size() < 8)
            return false;

        int n = (vchData.size() - 8) / 16;

        int64_t time;
        memcpy(&time, &vchData[0], 8);

        // -- Check time valid:
        int64_t now = GetTime();
        if (time < now - SMSG_RETENTION)
        {
            if (LogInstance().WillLogCategory(BCLog::SMSG))
                LogPrintf("Not interested in peer bucket %d, has expired.\n", (int32_t) time);
            return false;
        }
        if (time > now + SMSG_TIME_LEEWAY)
        {
            if (LogInstance().WillLogCategory(BCLog::SMSG))
                LogPrintf("Not interested in peer bucket %d, in the future.\n", (int32_t) time);
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 1);
            return false;
        }

        if (smsgBuckets[time].nLockCount > 0)
        {
            if (LogInstance().WillLogCategory(BCLog::SMSG))
                LogPrintf("Bucket %d lock count %u, waiting for message data from peer %u.\n", (int32_t) time, smsgBuckets[time].nLockCount, smsgBuckets[time].nLockPeerId);
            return false;
        }

        if (LogInstance().WillLogCategory(BCLog::SMSG))
            LogPrintf("Sifting through bucket %d.\n", (int32_t) time);

        std::vector<unsigned char> vchDataOut;
        vchDataOut.resize(8);
        memcpy(&vchDataOut[0], &vchData[0], 8);

        std::set<SecMsgToken>& tokenSet = smsgBuckets[time].setTokens;
        std::set<SecMsgToken>::iterator it;
        SecMsgToken token;
        unsigned char* p = &vchData[8];

        for (int i = 0; i < n; ++i)
        {
            memcpy(&token.timestamp, p, 8);
            memcpy(&token.sample, p+8, 8);

            it = tokenSet.find(token);
            if (it == tokenSet.end())
            {
                int nd = vchDataOut.size();
                try {
                    vchDataOut.resize(nd + 16);
                } catch (std::exception& e) {
                    LogPrintf("vchDataOut.resize %d threw: %s.\n", nd + 16, e.what());
                    continue;
                }

                memcpy(&vchDataOut[nd], p, 16);
            }

            p += 16;
        }

        if (vchDataOut.size() > 8)
        {
            if (LogInstance().WillLogCategory(BCLog::SMSG))
            {
                LogPrintf("Asking peer for  %d messages.\n", (int) (vchDataOut.size() - 8) / 16);
                LogPrintf("Locking bucket %d for peer %u.\n", (int) time, pfrom->smsgData.nPeerId);
            }
            smsgBuckets[time].nLockCount   = 3; // lock this bucket for at most 3 * SMSG_THREAD_DELAY seconds, unset when peer sends smsgMsg
            smsgBuckets[time].nLockPeerId  = pfrom->smsgData.nPeerId;
            g_connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::SMSGWANT, vchDataOut));
        }
    } else
    if (strCommand == NetMsgType::SMSGWANT)
    {
        std::vector<unsigned char> vchData;
        vRecv >> vchData;

        if (vchData.size() < 8)
            return false;

        std::vector<unsigned char> vchOne;
        std::vector<unsigned char> vchBunch;

        vchBunch.resize(4+8); // nmessages + bucketTime

        int n = (vchData.size() - 8) / 16;

        int64_t time;
        uint32_t nBunch = 0;
        memcpy(&time, &vchData[0], 8);

        std::map<int64_t, SecMsgBucket>::iterator itb;
        itb = smsgBuckets.find(time);
        if (itb == smsgBuckets.end())
        {
            if (LogInstance().WillLogCategory(BCLog::SMSG))
                LogPrintf("Don't have bucket %d.\n", (int32_t) time);
            return false;
        }

        std::set<SecMsgToken>& tokenSet = itb->second.setTokens;
        std::set<SecMsgToken>::iterator it;
        SecMsgToken token;
        unsigned char* p = &vchData[8];
        for (int i = 0; i < n; ++i)
        {
            memcpy(&token.timestamp, p, 8);
            memcpy(&token.sample, p+8, 8);

            it = tokenSet.find(token);
            if (it == tokenSet.end())
            {
                if (LogInstance().WillLogCategory(BCLog::SMSG))
                    LogPrintf("Don't have wanted message %d.\n", (int32_t) token.timestamp);
            } else
            {
                //LogPrintf("Have message at %"PRId64".\n", it->offset); // DEBUG
                token.offset = it->offset;
                //LogPrintf("winb before SecureMsgRetrieve %"PRId64".\n", token.timestamp);

                // -- place in vchOne so if SecureMsgRetrieve fails it won't corrupt vchBunch
                SecureMessage smsg;
                if (SecureMsgRetrieve(token, smsg) == 0)
                {
                    nBunch++;
                    const size_t size = vchBunch.size();
                    vchBunch.resize(size + SMSG_HDR_LEN + smsg.nPayload);
                    memcpy(&vchBunch[size], &smsg, SMSG_HDR_LEN);
                    memcpy(&vchBunch[size+SMSG_HDR_LEN], &smsg.vchPayload[0], smsg.nPayload);
                } else
                {
                    LogPrintf("SecureMsgRetrieve failed %d.\n", (int32_t) token.timestamp);
                }

                if (nBunch >= 500
                    || vchBunch.size() >= 96000)
                {
                    if (LogInstance().WillLogCategory(BCLog::SMSG))
                        LogPrintf("Break bunch %u, %d.\n", nBunch, (int) vchBunch.size());
                    break; // end here, peer will send more want messages if needed.
                }
            }
            p += 16;
        }

        if (nBunch > 0)
        {
            if (LogInstance().WillLogCategory(BCLog::SMSG))
                LogPrintf("Sending block of %u messages for bucket %d.\n", nBunch, (int32_t) time);

            memcpy(&vchBunch[0], &nBunch, 4);
            memcpy(&vchBunch[4], &time, 8);
            g_connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::SMSGMSG, vchBunch));
        }
    } else
    if (strCommand == NetMsgType::SMSGMSG)
    {
        std::vector<unsigned char> vchData;
        vRecv >> vchData;

        if (LogInstance().WillLogCategory(BCLog::SMSG))
            LogPrintf("smsgMsg vchData.size() %d.\n", (int) vchData.size());

        SecureMsgReceive(pfrom, vchData);
    } else
    if (strCommand == NetMsgType::SMSGMATCH)
    {
        std::vector<unsigned char> vchData;
        vRecv >> vchData;


        if (vchData.size() < 8)
        {
            LogPrintf("smsgMatch, not enough data %d.\n", (int) vchData.size());
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 1);
            return false;
        }

        int64_t time;
        memcpy(&time, &vchData[0], 8);

        int64_t now = GetTime();
        if (time > now + SMSG_TIME_LEEWAY)
        {
            LogPrintf("Warning: Peer buckets matched in the future: %d.\nEither this node or the peer node has the incorrect time set.\n", (int32_t) time);
            if (LogInstance().WillLogCategory(BCLog::SMSG))
                LogPrintf("Peer match time set to now.\n");
            time = now;
        }

        pfrom->smsgData.lastMatched = time;

        if (LogInstance().WillLogCategory(BCLog::SMSG))
            LogPrintf("Peer buckets matched at %d.\n", (int32_t) time);

    } else
    if (strCommand == NetMsgType::SMSGPING)
    {
        // -- smsgPing is the initial message, send reply
        g_connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::SMSGPONG));
    } else
    if (strCommand == NetMsgType::SMSGPONG)
    {
        if (LogInstance().WillLogCategory(BCLog::SMSG))
             LogPrintf("Peer replied, secure messaging enabled.\n");

        pfrom->smsgData.fEnabled = true;
    } else
    if (strCommand == NetMsgType::SMSGDISABLED)
    {
        // -- peer has disabled secure messaging.

        pfrom->smsgData.fEnabled = false;

        if (LogInstance().WillLogCategory(BCLog::SMSG))
            LogPrintf("Peer %u has disabled secure messaging.\n", pfrom->smsgData.nPeerId);

    } else
    if (strCommand == NetMsgType::SMSGIGNORE)
    {
        // -- peer is reporting that it will ignore this node until time.
        //    Ignore peer too
        std::vector<unsigned char> vchData;
        vRecv >> vchData;

        if (vchData.size() < 8)
        {
            LogPrintf("smsgIgnore, not enough data %d.\n", (int) vchData.size());
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 1);
            return false;
        }

        int64_t time;
        memcpy(&time, &vchData[0], 8);

        pfrom->smsgData.ignoreUntil = time;

        if (LogInstance().WillLogCategory(BCLog::SMSG))
            LogPrintf("Peer %u is ignoring this node until %d, ignore peer too.\n", pfrom->smsgData.nPeerId, (int) time);
    } else
    {
        // Unknown message
    }

    } //  LOCK(cs_smsg);

    return true;
}

bool SecureMsgSendData(CNode* pto, bool fSendTrickle)
{
    /*
        Called from ProcessMessage
        Runs in ThreadMessageHandler2
    */

    //LogPrintf("SecureMsgSendData() %s.\n", pto->addrName.c_str());
const CNetMsgMaker msgMaker(PROTOCOL_VERSION);


    int64_t now = GetTime();

    if (pto->smsgData.lastSeen == 0)
    {
        // -- first contact
        pto->smsgData.nPeerId = nPeerIdCounter++;
        
        if (LogInstance().WillLogCategory(BCLog::SMSG)){
            LOCK(pto->cs_addrName);
            LogPrintf("SecureMsgSendData() new node %s, peer id %u.\n", pto->addrName.c_str(), pto->smsgData.nPeerId);
		}
        // -- Send smsgPing once, do nothing until receive 1st smsgPong (then set fEnabled)

        g_connman->PushMessage(pto, msgMaker.Make(NetMsgType::SMSGPING));
        pto->smsgData.lastSeen = GetTime();
        return true;
    } else
    if (!pto->smsgData.fEnabled
        || now - pto->smsgData.lastSeen < SMSG_SEND_DELAY
        || now < pto->smsgData.ignoreUntil)
    {
        return true;
    }

    // -- When nWakeCounter == 0, resend bucket inventory.
    if (pto->smsgData.nWakeCounter < 1)
    {
        pto->smsgData.lastMatched = 0;
        pto->smsgData.nWakeCounter = 10 + GetRandInt(300);  // set to a random time between [10, 300] * SMSG_SEND_DELAY seconds

        if (LogInstance().WillLogCategory(BCLog::SMSG)){
			LOCK(pto->cs_addrName);
            LogPrintf("SecureMsgSendData(): nWakeCounter expired, sending bucket inventory to %s.\n"
            "Now %d next wake counter %u\n", pto->addrName.c_str(), (int32_t) now, pto->smsgData.nWakeCounter);
		}
    }
    pto->smsgData.nWakeCounter--;

    {
        LOCK(cs_smsg);
        std::map<int64_t, SecMsgBucket>::iterator it;

        uint32_t nBuckets = smsgBuckets.size();
        if (nBuckets > 0) // no need to send keep alive pkts, coin messages already do that
        {
            std::vector<unsigned char> vchData;
            // should reserve?
            vchData.reserve(4 + nBuckets*16); // timestamp + size + hash

            uint32_t nBucketsShown = 0;
            vchData.resize(4);

            unsigned char* p = &vchData[4];
            for (it = smsgBuckets.begin(); it != smsgBuckets.end(); ++it)
            {
                SecMsgBucket &bkt = it->second;

                uint32_t nMessages = bkt.setTokens.size();

                if (bkt.timeChanged < pto->smsgData.lastMatched     // peer has this bucket
                    || nMessages < 1)                               // this bucket is empty
                    continue;


                uint32_t hash = bkt.hash;

                try { vchData.resize(vchData.size() + 16); } catch (std::exception& e)
                {
                    LogPrintf("vchData.resize %d threw: %s.\n", (int) vchData.size() + 16, e.what());
                    continue;
                }
                memcpy(p, &it->first, 8);
                memcpy(p+8, &nMessages, 4);
                memcpy(p+12, &hash, 4);

                p += 16;
                nBucketsShown++;
                //if (fDebug)
                //    LogPrintf("Sending bucket %"PRId64", size %d \n", it->first, it->second.size());
            }

            if (vchData.size() > 4)
            {
                memcpy(&vchData[0], &nBucketsShown, 4);
                if (LogInstance().WillLogCategory(BCLog::SMSG))
                    LogPrintf("Sending %d bucket headers.\n", nBucketsShown);

                g_connman->PushMessage(pto, msgMaker.Make(NetMsgType::SMSGINV, vchData));
            }
        }
    }

    pto->smsgData.lastSeen = GetTime();

    return true;
}


static int SecureMsgInsertAddress(CKeyID& hashKey, CPubKey& pubKey, SecMsgDB& addrpkdb)
{
    /* insert key hash and public key to addressdb

        should have LOCK(cs_smsg) where db is opened

        returns
            0 success
            1 error
            4 address is already in db
    */


    if (addrpkdb.ExistsPK(hashKey))
    {
        //LogPrintf("DB already contains public key for address.\n");
        CPubKey cpkCheck;
        if (!addrpkdb.ReadPK(hashKey, cpkCheck))
        {
            LogPrintf("addrpkdb.Read failed.\n");
        } else
        {
            if (cpkCheck != pubKey)
                LogPrintf("DB already contains existing public key that does not match .\n");
        }
        return 4;
    }

    if (!addrpkdb.WritePK(hashKey, pubKey))
    {
        LogPrintf("Write pair failed.\n");
        return 1;
    }
    CTxDestination dest = PKHash(hashKey);

	LogPrintf("Add public key of %s to database\n", EncodeDestination(dest).c_str());
    return 0;
}

int SecureMsgInsertAddress(CKeyID& hashKey, CPubKey& pubKey)
{
    int rv;
    {
        LOCK(cs_smsgDB);
        SecMsgDB addrpkdb;

        if (!addrpkdb.Open("cr+"))
            return 1;

        rv = SecureMsgInsertAddress(hashKey, pubKey, addrpkdb);
    }
    return rv;
}


static bool ScanBlock(CBlock& block, SecMsgDB& addrpkdb, uint32_t& nTransactions, uint32_t& nInputs, uint32_t& nPubkeys, uint32_t& nDuplicates){
	std::shared_ptr<CWallet> pwallet = GetWallet("default");
	if (!pwallet)
	   return 0;

    // -- should have LOCK(cs_smsg) where db is opened
    for(const CTransactionRef& tx: block.vtx){
        if (tx->IsCoinBase())
            continue; // leave out coinbase


        /*
        Look at the inputs of every tx.
        If the inputs are standard, get the pubkey from scriptsig and
        look for the corresponding output (the input(output of other tx) to the input of this tx)
        get the address from scriptPubKey
        add to db if address is unique.

        Would make more sense to do this the other way around, get address first for early out.

        */

        for (size_t i = 0; i < tx->vin.size(); i++){
/*
            TODO ???
            if (tx.nVersion == ANON_TXN_VERSION
                && tx.vin[i].IsAnonInput())
                continue; // skip anon inputs
*/
            const CScript &script = tx->vin[i].scriptSig;

            opcodetype opcode;
            std::vector<unsigned char> vch;

            uint256 prevoutHash, blockHash;

            // -- matching address is in scriptPubKey of previous tx output
            for (CScript::const_iterator pc = script.begin(); script.GetOp(pc, opcode, vch); )
            {
                // -- opcode is the length of the following data, compressed public key is always 33
                if (opcode == 33)
                {
                    CPubKey pubKey(vch);

                    if (!pubKey.IsValid())
                    {
                        LogPrintf("Public key is invalid %s.\n", HexStr(vch).c_str());
                        continue;
                    }

                    prevoutHash = tx->vin[i].prevout.hash;
                    CTransactionRef txOfPrevOutput;
                    CBlockIndex* blockindex = nullptr;
                    LOCK(cs_main);
                    blockindex = LookupBlockIndex(block.GetHash());
                    //const Consensus::Params& consensusParams = Params().GetConsensus();

                    if (!GetTransaction(prevoutHash, txOfPrevOutput, Params().GetConsensus(), blockHash, blockindex))
                    {
                        LogPrintf("Could not get transaction %s (output %d) referenced by input #%d of transaction %s in block %s\n", prevoutHash.ToString().c_str(), tx->vin[i].prevout.n, (int) i, tx->GetHash().ToString().c_str(), block.GetHash().ToString().c_str());
                        continue;
                    }

                    unsigned int nOut = tx->vin[i].prevout.n;
                    if (nOut >= txOfPrevOutput->vout.size())
                    {
                        LogPrintf("Output %u, not in transaction: %s\n", nOut, prevoutHash.ToString().c_str());
                        continue;
                    }

                    const CTxOut &txOut = txOfPrevOutput->vout[nOut];

                    CTxDestination addressRet;
                    txnouttype ttype;
                    if (!ExtractDestination(txOut.scriptPubKey, addressRet, &ttype))
                    {
                        LogPrintf("ExtractDestination failed: %s\n", prevoutHash.ToString().c_str());
                        continue;
                    }

                    CKeyID hashKey = GetKeyForDestination(*pwallet, addressRet);
                    if (hashKey.IsNull())
                    {
                        LogPrintf("coinAddress.GetKeyID failed: %s\n", EncodeDestination(addressRet).c_str());
                        continue;
                    }
                    if(PKHash(pubKey) != boost::get<PKHash>(addressRet))
                        continue;

                    int rv = SecureMsgInsertAddress(hashKey, pubKey, addrpkdb);
                    nPubkeys += (rv == 0);
                    nDuplicates += (rv == 4);
                }

                //LogPrintf("opcode %d, %s, value %s.\n", opcode, GetOpName(opcode), HexStr(vch).c_str());
            }
            nInputs++;
        }
        nTransactions++;

        if (nTransactions % 10000 == 0) // for ScanChainForPublicKeys
        {
            LogPrintf("Scanning transaction no. %u.\n", nTransactions);
        }
    }
    return true;
}


bool SecureMsgScanBlock(CBlock& block)
{
    /*
    scan block for public key addresses
    called from ProcessMessage() in main where strCommand == "block"
    */

    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecureMsgScanBlock().\n");

    uint32_t nTransactions  = 0;
    uint32_t nInputs        = 0;
    uint32_t nPubkeys       = 0;
    uint32_t nDuplicates    = 0;

    {
        LOCK(cs_smsgDB);

        SecMsgDB addrpkdb;
        if (!addrpkdb.Open("cw") || !addrpkdb.TxnBegin())
            return false;

        ScanBlock(block, addrpkdb, nTransactions, nInputs, nPubkeys, nDuplicates);

        addrpkdb.TxnCommit();
    }

    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("Found %u transactions, %u inputs, %u new public keys, %u duplicates.\n", nTransactions, nInputs, nPubkeys, nDuplicates);

    return true;
}

bool ScanChainForPublicKeys(CBlockIndex* pindexStart, size_t n)
{
    LogPrintf("Scanning block chain for public keys.\n");
    int64_t nStart = GetTimeMillis();

    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("From height %u.\n", pindexStart->nHeight);

    // -- public keys are in txin.scriptSig
    //    matching addresses are in scriptPubKey of txin's referenced output

    uint32_t nBlocks        = 0;
    uint32_t nTransactions  = 0;
    uint32_t nInputs        = 0;
    uint32_t nPubkeys       = 0;
    uint32_t nDuplicates    = 0;

    {
        LOCK(cs_smsgDB);

        SecMsgDB addrpkdb;
        if (!addrpkdb.Open("cw")
            || !addrpkdb.TxnBegin())
            return false;

        CBlockIndex* pindex = pindexStart;
        for (size_t i = 0; pindex != NULL && i < n; i++, pindex = pindex->pprev)
        {
            nBlocks++;
            CBlock block;
            ReadBlockFromDisk(block, pindex, Params().GetConsensus());

            ScanBlock(block, addrpkdb, nTransactions, nInputs, nPubkeys, nDuplicates);
        }

        addrpkdb.TxnCommit();
    }

    LogPrintf("Scanned %u blocks, %u transactions, %u inputs\n", nBlocks, nTransactions, nInputs);
    LogPrintf("Found %u public keys, %u duplicates.\n", nPubkeys, nDuplicates);
    LogPrintf("Took %d ms\n", (int32_t)(GetTimeMillis() - nStart));

    return true;
}

bool SecureMsgScanBlockChain()
{
    TRY_LOCK(cs_main, lockMain);
    if (lockMain)
    {
        CBlockIndex *indexScan = ::ChainActive().Tip();

        try { // -- in try to catch errors opening db,
            if (!ScanChainForPublicKeys(indexScan, indexScan->nHeight))
                return false;
        } catch (std::exception& e)
        {
            LogPrintf("ScanChainForPublicKeys() threw: %s.\n", e.what());
            return false;
        }
    } else
    {
        LogPrintf("ScanChainForPublicKeys() Could not lock main.\n");
        return false;
    }

    return true;
}

int SecureMsgScanBuckets(std::string &error, bool fDecrypt)
{
	std::shared_ptr<CWallet> pwallet = GetWallet("default");
	if (!pwallet)
	   return 0;
    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecureMsgScanBuckets()\n");

    if (!fSecMsgEnabled) {
        error = "Secure messaging is disabled.";
        return fDecrypt ? 0 : RPC_METHOD_NOT_FOUND;
    }
    if (pwallet->IsLocked()) {
        error = "Wallet is locked. Secure messaging needs an unlocked wallet.";
        return RPC_WALLET_UNLOCK_NEEDED;
    }

    int64_t  mStart         = GetTimeMillis();
    int64_t  now            = GetTime();
    uint32_t nFiles         = 0;
    uint32_t nMessages      = 0;
    uint32_t nFoundMessages = 0;

    fs::path pathSmsgDir = GetDataDir() / "smsgStore";
    fs::directory_iterator itend;

    if (!fs::exists(pathSmsgDir) || !fs::is_directory(pathSmsgDir)) {
        if (!fs::create_directory(pathSmsgDir)) {
            error = "Message store directory does not exist and could not be created.";
            return 1;
        }
    }

    SecureMessage smsg;

    for (fs::directory_iterator itd(pathSmsgDir) ; itd != itend ; ++itd) {
        if (!fs::is_regular_file(itd->status()))
            continue;

        std::string fileName = (*itd).path().filename().string();

        if (!boost::algorithm::ends_with(fileName, fDecrypt ? "_wl.dat" : ".dat"))
            continue;

        if (LogInstance().WillLogCategory(BCLog::SMSG))
            LogPrintf("Processing file: %s.\n", fileName.c_str());

        nFiles++;

        // TODO files must be split if > 2GB
        // time_noFile.dat
        size_t sep = fileName.find_first_of("_");
        if (sep == std::string::npos)
            continue;

        std::string stime = fileName.substr(0, sep);

        int64_t fileTime = boost::lexical_cast<int64_t>(stime);

        if (fileTime < now - SMSG_RETENTION) {
            LogPrintf("Dropping file %s, expired.\n", fileName.c_str());
            try {
                fs::remove((*itd).path());
            }
            catch (const fs::filesystem_error& ex) {
                LogPrintf("Error removing bucket file %s, %s.\n", fileName.c_str(), ex.what());
            }
            continue;
        }

        if (!fDecrypt && boost::algorithm::ends_with(fileName, "_wl.dat")) {
            if (LogInstance().WillLogCategory(BCLog::SMSG))
                LogPrintf("Skipping wallet locked file: %s.\n", fileName.c_str());
            continue;
        }

        {
            LOCK(cs_smsg);
            FILE *fp;
            errno = 0;
            if (!(fp = fopen((*itd).path().string().c_str(), "rb"))) {
                LogPrintf("Error opening file: %s (%d)\n", strerror(errno), __LINE__);
                continue;
            }

            for (;;) {
                errno = 0;
                if (fread(smsg.Header(), sizeof(unsigned char), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN) {
                    if (errno != 0) {
                        LogPrintf("fread header failed: %s\n", strerror(errno));
                    }
                    else {
                        //LogPrintf("End of file.\n");
                    }
                    break;
                }

                try {
                    smsg.vchPayload.resize(smsg.nPayload);
                }
                catch (std::exception& e) {
                    LogPrintf("SecureMsgWalletUnlocked(): Could not resize vchPayload, %u, %s\n", smsg.nPayload, e.what());
                    fclose(fp);
                    return 1;
                }

                if (fread(&smsg.vchPayload[0], 1, smsg.nPayload, fp) != smsg.nPayload) {
                    LogPrintf("fread data failed: %s\n", strerror(errno));
                    break;
                }

                // -- don't report to gui,
                int rv = SecureMsgScanMessage(*smsg.Header(), &smsg.vchPayload[0], false);

                if (rv == 0) {
                    nFoundMessages++;
                }
                else if (rv != 0) {
                    // SecureMsgScanMessage failed
                }

                nMessages ++;
            }

            fclose(fp);

            // -- remove wl file when scanned
            try {
                fs::remove((*itd).path());
            } catch (const boost::filesystem::filesystem_error& ex) {
                LogPrintf("Error removing wl file %s - %s\n", fileName.c_str(), ex.what());
                return 1;
            }
        }
    }

    LogPrintf("Processed %u files, scanned %u messages, received %u messages.\n", nFiles, nMessages, nFoundMessages);
    LogPrintf("Took %d ms\n", (int)(GetTimeMillis() - mStart));

	// -- notify gui
    if (fDecrypt)
        NotifySecMsgWalletUnlocked();
    return 0;
}


int SecureMsgWalletKeyChanged(std::string sAddress, std::string sLabel, ChangeType mode)
{
    if (!fSecMsgEnabled)
        return 0;

    LogPrintf("SecureMsgWalletKeyChanged()\n");

    // TODO: default recv and recvAnon

    {
        LOCK(cs_smsg);

        switch(mode)
        {
            case CT_NEW:
                smsgAddresses.push_back(SecMsgAddress(sAddress, smsgOptions.fNewAddressRecv, smsgOptions.fNewAddressAnon));
                break;
            case CT_DELETED:
                for (std::vector<SecMsgAddress>::iterator it = smsgAddresses.begin(); it != smsgAddresses.end(); ++it)
                {
                    if (sAddress != it->sAddress)
                        continue;
                    smsgAddresses.erase(it);
                    break;
                }
                break;
            default:
                break;
        }

    } // LOCK(cs_smsg);


    return 0;
}

int SecureMsgScanMessage(const SecureMessageHeader &smsg, const unsigned char *pPayload, bool reportToGui)
{
    /*
    Check if message belongs to this node.
    If so add to inbox db.

    if !reportToGui don't fire NotifySecMsgInboxChanged
     - loads messages received when wallet locked in bulk.

    returns
        0 success,
        1 error
        2 no match
        3 wallet is locked - message stored for scanning later.
    */

	std::shared_ptr<CWallet> pwallet = GetWallet("default");
	if (!pwallet)
	   return 0;

    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecureMsgScanMessage()\n");

    if (pwallet->IsLocked())
    {
        if (LogInstance().WillLogCategory(BCLog::SMSG))
            LogPrintf("ScanMessage: Wallet is locked, storing message to scan later.\n");

        int rv;
        if ((rv = SecureMsgStoreUnscanned(smsg, pPayload)) != 0)
            return 1;

        return 3;
    }

    // -- Calculate hash of payload and enable verification of the HMAC
    SecureMessageHeader header((const unsigned char*) smsg.begin());
    memcpy(header.hash, Hash(&pPayload[0], &pPayload[smsg.nPayload]).begin(), 32);

    std::string addressTo;
    MessageData msg; // placeholder
    bool fOwnMessage = false;

    for (std::vector<SecMsgAddress>::iterator it = smsgAddresses.begin(); it != smsgAddresses.end(); ++it)
    {
        if (!it->fReceiveEnabled)
            continue;

        CTxDestination dest = DecodeDestination(it->sAddress);

        if (!it->fReceiveAnon)
        {
            // -- have to do full decrypt to see address from
            if (SecureMsgDecrypt(false, EncodeDestination(dest), header, pPayload, msg) == 0)
            {
                if (LogInstance().WillLogCategory(BCLog::SMSG))
                    LogPrintf("%d: Decrypted message with %s.\n", __LINE__, addressTo.c_str());

                if (msg.sFromAddress.compare("anon") != 0)
                    fOwnMessage = true;
                break;
            }
        } else
        {

            if (SecureMsgDecrypt(true, EncodeDestination(dest), header, pPayload, msg) == 0)
            {
                if (LogInstance().WillLogCategory(BCLog::SMSG))
                    LogPrintf("%d: Decrypted message with %s.\n", __LINE__, addressTo.c_str());

                fOwnMessage = true;
                break;
            }
        }
    }

    if (fOwnMessage)
    {
        // -- save to inbox
        std::string sPrefix("im");
        unsigned char chKey[18];
        memcpy(&chKey[0],  sPrefix.data(),  2);
        memcpy(&chKey[2],  &smsg.timestamp, 8);
        memcpy(&chKey[10], pPayload,        8);

        SecMsgStored smsgInbox;
        smsgInbox.timeReceived  = GetTime();
        smsgInbox.status        = (SMSG_MASK_UNREAD) & 0xFF;
        smsgInbox.sAddrTo       = addressTo;

        // -- data may not be contiguous
        try {
            smsgInbox.vchMessage.resize(SMSG_HDR_LEN + smsg.nPayload);
        } catch (std::exception& e) {
            LogPrintf("SecureMsgScanMessage(): Could not resize vchData, %u, %s\n", SMSG_HDR_LEN + smsg.nPayload, e.what());
            return 1;
        }
        memcpy(&smsgInbox.vchMessage[0], smsg.begin(), SMSG_HDR_LEN);
        memcpy(&smsgInbox.vchMessage[SMSG_HDR_LEN], pPayload, smsg.nPayload);

        {
            LOCK(cs_smsgDB);
            SecMsgDB dbInbox;

            if (dbInbox.Open("cw"))
            {
                if (dbInbox.ExistsSmesg(chKey))
                {
                    if (LogInstance().WillLogCategory(BCLog::SMSG))
                        LogPrintf("Message already exists in inbox db.\n");
                } else
                {
                    dbInbox.WriteSmesg(chKey, smsgInbox);

                    if (reportToGui)
                        NotifySecMsgInboxChanged(smsgInbox);
                    LogPrintf("SecureMsg saved to inbox, received with %s.\n", addressTo.c_str());
                }
            }
        }
    }

    return 0;
}

int SecureMsgGetLocalKey(CKeyID& ckid, CPubKey& cpkOut)
{
	std::shared_ptr<CWallet> pwallet = GetWallet("default");
	if (!pwallet)
	   return 0;
    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecureMsgGetLocalKey()\n");

    CKey key;
    if (!pwallet->GetKey(ckid, key))
        return 4;

    cpkOut = key.GetPubKey();
    if (!cpkOut.IsValid()
        || !cpkOut.IsCompressed())
    {
        LogPrintf("Public key is invalid %s.\n", HexStr(std::vector<unsigned char>(cpkOut.begin(), cpkOut.end())).c_str());
        return 1;
    }

    return 0;
}

int SecureMsgGetLocalPublicKey(std::string& strAddress, std::string& strPublicKey)
{
		std::shared_ptr<CWallet> pwallet = GetWallet("default");
	if (!pwallet)
	   return 0;
    /* returns
        0 success,
        1 error
        2 invalid address
        3 address does not refer to a key
        4 address not in wallet
    */

	CTxDestination dest = DecodeDestination(strAddress);

	if (!IsValidDestination(dest))
		return 2;

	CScript scriptPubKey = GetScriptForDestination(dest);
	std::string sPublicKey;
	CKeyID keyID = GetKeyForDestination(*pwallet, dest);

	if (keyID.IsNull())
		return 3;

    int rv;
    CPubKey pubKey;
    if ((rv = SecureMsgGetLocalKey(keyID, pubKey)) != 0)
        return rv;

    strPublicKey = EncodeBase58(pubKey.begin(), pubKey.end());

    return 0;
}

int SecureMsgGetStoredKey(CKeyID& ckid, CPubKey& cpkOut)
{
    /* returns
        0 success,
        1 error
        2 public key not in database
    */
    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecureMsgGetStoredKey().\n");

    {
        LOCK(cs_smsgDB);
        SecMsgDB addrpkdb;

        if (!addrpkdb.Open("r")) {
            LogPrintf("addrpkdb. Open() failed.\n");
            return 1;
        }

        if (!addrpkdb.ReadPK(ckid, cpkOut))
        {
            LogPrintf("addrpkdb.Read failed: %s.\n", ckid.ToString().c_str());
            return 2;
        }
    }

    if (LogInstance().WillLogCategory(BCLog::SMSG)) {
        LogPrintf("SecureMsgGetStoredKey(): found public key of %s\n", ckid.ToString().c_str());
    }
    return 0;
}

int SecureMsgAddAddress(std::string& address, std::string& publicKey)
{
    /*
        Add address and matching public key to the database
        address and publicKey are in base58

        returns
            0 success
            1 error
            2 publicKey is invalid
            3 publicKey != address
            4 address is already in db
            5 address is invalid
    */

	std::shared_ptr<CWallet> pwallet = GetWallet("default");
	if (!pwallet)
	   return 0;

	CTxDestination dest = DecodeDestination(address);

	if (!IsValidDestination(dest))
    {
        LogPrintf("Address is not valid: %s.\n", EncodeDestination(dest).c_str());
        return 5;
    }

	CKeyID hashKey = GetKeyForDestination(*pwallet, dest);

    if (hashKey.IsNull())
    {
        LogPrintf("coinAddress.GetKeyID failed: %s.\n", EncodeDestination(dest).c_str());
        return 5;
    }

    std::vector<unsigned char> vchTest;
    DecodeBase58(publicKey, vchTest);
    CPubKey pubKey(vchTest);

    // -- check that public key matches address hash
    if(PKHash(pubKey) == boost::get<PKHash>(dest))
    {
        LogPrintf("Public key does not hash to address, addressT %s.\n", pubKey.GetID().ToString().c_str());
        return 3;
    }

    return SecureMsgInsertAddress(hashKey, pubKey);
}

int SecureMsgRetrieve(SecMsgToken &token, SecureMessage &smsg)
{
    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecureMsgRetrieve() %d.\n", (int32_t) token.timestamp);

    // -- has cs_smsg lock from SecureMsgReceiveData

    fs::path pathSmsgDir = GetDataDir() / "smsgStore";

    int64_t bucket = token.timestamp - (token.timestamp % SMSG_BUCKET_LEN);
    std::string fileName = boost::lexical_cast<std::string>(bucket) + "_01.dat";
    fs::path fullpath = pathSmsgDir / fileName;

    //LogPrintf("bucket %"PRId64".\n", bucket);
    //LogPrintf("bucket lld %lld.\n", bucket);
    //LogPrintf("fileName %s.\n", fileName.c_str());

    FILE *fp;
    if (!(fp = fopen(fullpath.string().c_str(), "rb"))) {
        LogPrintf("Error opening file: %s\nPath %s\n", strerror(errno), fullpath.string().c_str());
        return 1;
    }

    if (fseek(fp, token.offset, SEEK_SET) != 0) {
        LogPrintf("fseek, strerror: %s.\n", strerror(errno));
        fclose(fp);
        return 1;
    }

    if (fread(smsg.Header(), sizeof(unsigned char), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN) {
        LogPrintf("fread header failed: %s\n", strerror(errno));
        fclose(fp);
        return 1;
    }

    try {
        smsg.vchPayload.resize(smsg.nPayload);
    }
    catch (std::exception& e) {
        LogPrintf("SecureMsgRetrieve(): Could not resize vchPayload, %u, %s\n", smsg.nPayload, e.what());
        return 1;
    }

    if (fread(&smsg.vchPayload[0], sizeof(unsigned char), smsg.nPayload, fp) != smsg.nPayload) {
        LogPrintf("fread data failed: %s. Wanted %u bytes.\n", strerror(errno), smsg.nPayload);
        fclose(fp);
        return 1;
    }


    fclose(fp);

    return 0;
}

int SecureMsgReceive(CNode* pfrom, std::vector<unsigned char>& vchData)
{
    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecureMsgReceive().\n");

    if (vchData.size() < 12) { // nBunch4 + timestamp8
        LogPrintf("Error: not enough data.\n");
        return 1;
    }

    uint32_t nBunch;
    int64_t bktTime;

    memcpy(&nBunch, &vchData[0], 4);
    memcpy(&bktTime, &vchData[4], 8);


    // -- check bktTime ()
    //    bucket may not exist yet - will be created when messages are added
    int64_t now = GetTime();
    if (bktTime > now + SMSG_TIME_LEEWAY) {
        if (LogInstance().WillLogCategory(BCLog::SMSG))
            LogPrintf("bktTime > now.\n");
        // misbehave?
        return 1;
    }
    else if (bktTime < now - SMSG_RETENTION) {
        if (LogInstance().WillLogCategory(BCLog::SMSG))
            LogPrintf("bktTime < now - SMSG_RETENTION.\n");
        // misbehave?
        return 1;
    }

    std::map<int64_t, SecMsgBucket>::iterator itb;

    if (nBunch == 0 || nBunch > 500) {
        LogPrintf("Error: Invalid no. messages received in bunch %u, for bucket %d.\n", nBunch, (int32_t) bktTime);
        LOCK(cs_main);
        Misbehaving(pfrom->GetId(), 1);

        // -- release lock on bucket if it exists
        itb = smsgBuckets.find(bktTime);
        if (itb != smsgBuckets.end())
            itb->second.nLockCount = 0;
        return 1;
    }

    uint32_t n = 12;

    for (uint32_t i = 0; i < nBunch; ++i) {
        if (vchData.size() - n < SMSG_HDR_LEN) {
            LogPrintf("Error: not enough data sent, n = %u.\n", n);
            break;
        }

        SecureMessageHeader header(&vchData[n]);

        int rv = SecureMsgValidate(header, vchData.size() - (n + SMSG_HDR_LEN));
        std::string reason;
        if (rv != 0) {
			LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 0);
            if( rv==1 )
                reason ="error";
            if( rv==4 )
                reason ="invalid version";
            if( rv==5 )
                reason ="payload too large";

             printf("SMSG Misbehave reason == %s .\n", reason.c_str());
            continue;
        }

        // -- store message, but don't hash bucket
        if (SecureMsgStore(header, &vchData[n + SMSG_HDR_LEN], false) != 0) {
            // message dropped
            break; // continue?
        }

        if (SecureMsgScanMessage(header, &vchData[n + SMSG_HDR_LEN], true) != 0) {
            // message recipient is not this node (or failed)
        }

        n += SMSG_HDR_LEN + header.nPayload;
    }

    // -- if messages have been added, bucket must exist now
    itb = smsgBuckets.find(bktTime);
    if (itb == smsgBuckets.end()) {
        if (LogInstance().WillLogCategory(BCLog::SMSG))
            LogPrintf("Don't have bucket %d.\n", (int32_t) bktTime);
        return 1;
    }

    itb->second.nLockCount  = 0; // this node has received data from peer, release lock
    itb->second.nLockPeerId = 0;
    itb->second.hashBucket();

    return 0;
}

int SecureMsgStoreUnscanned(const SecureMessageHeader &header, const unsigned char *pPayload)
{
    /*
    When the wallet is locked a copy of each received message is stored to be scanned later if wallet is unlocked
    */

    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecureMsgStoreUnscanned()\n");

    if (!pPayload) {
        LogPrintf("Error: null pointer to payload.\n");
        return 1;
    }

    fs::path pathSmsgDir;
    try {
        pathSmsgDir = GetDataDir() / "smsgStore";
        fs::create_directory(pathSmsgDir);
    }
    catch (const boost::filesystem::filesystem_error& ex) {
        LogPrintf("Error: Failed to create directory %s - %s\n", pathSmsgDir.string().c_str(), ex.what());
        return 1;
    }

    int64_t now = GetTime();
    if (header.timestamp > now + SMSG_TIME_LEEWAY) {
        LogPrintf("Message > now.\n");
        return 1;
    }
    else if (header.timestamp < now - SMSG_RETENTION) {
        LogPrintf("Message < SMSG_RETENTION.\n");
        return 1;
    }

    int64_t bucket = header.timestamp - (header.timestamp % SMSG_BUCKET_LEN);

    std::string fileName = boost::lexical_cast<std::string>(bucket) + "_01_wl.dat";
    fs::path fullpath = pathSmsgDir / fileName;

    FILE *fp;
    if (!(fp = fopen(fullpath.string().c_str(), "ab"))) {
        LogPrintf("Error opening file: %s (%d)\n", strerror(errno), __LINE__);
        return 1;
    }

    if (fwrite(&header, sizeof(unsigned char), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN
        || fwrite(pPayload, sizeof(unsigned char), header.nPayload, fp) != header.nPayload)
    {
        LogPrintf("fwrite failed: %s\n", strerror(errno));
        fclose(fp);
        return 1;
    }

    fclose(fp);

    return 0;
}


int SecureMsgStore(const SecureMessageHeader &smsg, const unsigned char *pPayload, bool fUpdateBucket)
{
    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecureMsgStore()\n");

    if (!pPayload) {
        LogPrintf("Error: null pointer to payload.\n");
        return 1;
    }


    long int ofs;
    fs::path pathSmsgDir;
    try {
        pathSmsgDir = GetDataDir() / "smsgStore";
        fs::create_directory(pathSmsgDir);
    }
    catch (const boost::filesystem::filesystem_error& ex) {
        LogPrintf("Error: Failed to create directory %s - %s\n", pathSmsgDir.string().c_str(), ex.what());
        return 1;
    }

    int64_t now = GetTime();
    if (smsg.timestamp > now + SMSG_TIME_LEEWAY) {
        LogPrintf("Message > now.\n");
        return 1;
    }
    else if (smsg.timestamp < now - SMSG_RETENTION) {
        LogPrintf("Message < SMSG_RETENTION.\n");
        return 1;
    }

    int64_t bucket = smsg.timestamp - (smsg.timestamp % SMSG_BUCKET_LEN);

    {
        // -- must lock cs_smsg before calling
        //LOCK(cs_smsg);

        SecMsgToken token(smsg.timestamp, pPayload, smsg.nPayload, 0);

        std::set<SecMsgToken>& tokenSet = smsgBuckets[bucket].setTokens;
        std::set<SecMsgToken>::iterator it;
        it = tokenSet.find(token);
        if (it != tokenSet.end())
        {
            LogPrintf("Already have message.\n");
            if (LogInstance().WillLogCategory(BCLog::SMSG))
            {
                LogPrintf("nPayload: %u\n", smsg.nPayload);
                LogPrintf("bucket: %d\n", (int) bucket);

                LogPrintf("message ts: %d\n", (int) token.timestamp);
                std::vector<unsigned char> vchShow;
                vchShow.resize(8);
                memcpy(&vchShow[0], token.sample, 8);
                LogPrintf(" sample %s\n", HexStr(vchShow).c_str());
                /*
                LogPrintf("\nmessages in bucket:\n");
                for (it = tokenSet.begin(); it != tokenSet.end(); ++it)
                {
                    LogPrintf("message ts: %"PRId64, (*it).timestamp);
                    vchShow.resize(8);
                    memcpy(&vchShow[0], (*it).sample, 8);
                    LogPrintf(" sample %s\n", HexStr(vchShow).c_str());
                }
                */
            }
            return 1;
        }

        std::string fileName = boost::lexical_cast<std::string>(bucket) + "_01.dat";
        fs::path fullpath = pathSmsgDir / fileName;

        FILE *fp;
        if (!(fp = fopen(fullpath.string().c_str(), "ab")))
        {
            LogPrintf("Error opening file: %s (%d)\n", strerror(errno), __LINE__);
            return 1;
        }

        // -- on windows ftell will always return 0 after fopen(ab), call fseek to set.
        if (fseek(fp, 0, SEEK_END) != 0)
        {
            LogPrintf("Error fseek failed: %s\n", strerror(errno));
            return 1;
        }


        ofs = ftell(fp);

        if (fwrite(&smsg, sizeof(unsigned char), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN
            || fwrite(pPayload, sizeof(unsigned char), smsg.nPayload, fp) != smsg.nPayload)
        {
            LogPrintf("fwrite failed: %s\n", strerror(errno));
            fclose(fp);
            return 1;
        }

        fclose(fp);

        token.offset = ofs;

        //LogPrintf("token.offset: %"PRId64"\n", token.offset); // DEBUG
        tokenSet.insert(token);

        if (fUpdateBucket)
            smsgBuckets[bucket].hashBucket();
    }

    //if (LogInstance().WillLogCategory(BCLog::SMSG))
    LogPrintf("SecureMsg added to bucket %d.\n", (int) bucket);
    return 0;
}

int SecureMsgStore(const SecureMessage& smsg, bool fUpdateBucket)
{
    return SecureMsgStore(smsg, &smsg.vchPayload[0], fUpdateBucket);
}

int SecureMsgValidate(const SecureMessageHeader &smsg_header, size_t nPayload)
{
    /*
    returns
        0 success
        1 error
        4 invalid version
        5 payload is too large
    */

    if (smsg_header.nPayload != nPayload){
        printf("Message payload does not match got  %d, expected %d .\n", smsg_header.nPayload, (int) nPayload);
        return 1;
    }

    if (smsg_header.nVersion != 1)
        return 4;

    if (smsg_header.nPayload > SMSG_MAX_MSG_WORST){
       LogPrintf("Message payload larger than SMSG_MAX_MSG_WORST got  %d, expected %d .\n", smsg_header.nPayload, SMSG_MAX_MSG_WORST);
        return 5;
    }

    return 0;
}


int SecureMsgEncrypt(SecureMessage& smsg, std::string& addressFrom, std::string& addressTo, std::string& message)
{
    /* Create a secure message

        Using similar method to bitmessage.
        If bitmessage is secure this should be too.
        https://bitmessage.org/wiki/Encryption

        Some differences:
        bitmessage seems to use curve sect283r1
        *coin addresses use secp256k1

        returns
            2       message is too long.
            3       addressFrom is invalid.
            4       addressTo is invalid.
            5       Could not get public key for addressTo.
            6       ECDH_compute_key failed
            7       Could not get private key for addressFrom.
            8       Could not allocate memory.
            9       Could not compress message data.
            10      Could not generate MAC.
            11      Encrypt failed.
    */

	std::shared_ptr<CWallet> pwallet = GetWallet("default");
	if (!pwallet)
	   return 0;

    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecureMsgEncrypt(%s, %s, ...)\n", addressFrom.c_str(), addressTo.c_str());


    if (message.size() > SMSG_MAX_MSG_BYTES) {
        LogPrintf("Message is too long, %d.\n", (int) message.size());
        return 2;
    }

    smsg.nVersion = 1;
    smsg.timestamp = GetTime();


    bool fSendAnonymous;
    CKey keyFrom;

    CTxDestination orig = DecodeDestination(addressFrom);
    CKeyID ckidFrom = GetKeyForDestination(*pwallet, orig);

    if (addressFrom.compare("anon") == 0) {
        fSendAnonymous = true;
    }
    else {
        fSendAnonymous = false;
        
        if (!IsValidDestination(orig)) {
            LogPrintf("addressFrom is not valid.\n");
            return 3;
        }        

        if (ckidFrom.IsNull()) {
            LogPrintf("coinAddrFrom.GetKeyID failed: %s.\n", EncodeDestination(orig).c_str());
            return 3;
        }
    }

    CTxDestination dest = DecodeDestination(addressTo);
    CKeyID ckidDest = GetKeyForDestination(*pwallet, dest);

    if (!IsValidDestination(orig)) {
        LogPrintf("addressTo is not valid.\n");
        return 4;
    }

    if (ckidDest.IsNull()) {
        LogPrintf("%d: coinAddrDest.GetKeyID failed: %s.\n", __LINE__, EncodeDestination(dest).c_str());
        return 4;
    }

    // -- public key K is the destination address
    CPubKey cpkDestK;
    if (SecureMsgGetStoredKey(ckidDest, cpkDestK) != 0
        && SecureMsgGetLocalKey(ckidDest, cpkDestK) != 0) // maybe it's a local key (outbox?)
    {
        LogPrintf("Could not get public key for destination address.\n");
        return 5;
    }

    // -- create save hash instances
    secure_buffer sha_mem(sizeof(CSHA256) + sizeof(CSHA512) + sizeof(CHMAC_SHA256), 0);
    CSHA256 &sha256 = *new (&sha_mem[0]) CSHA256();
    CSHA512 &sha512 = *new (&sha256 + 1) CSHA512();

    // -- make a key pair to which the receiver can respond
    CKey keyS;
    keyS.MakeNewKey(true); // make compressed key
    CPubKey pubKeyS = keyS.GetPubKey();

    // -- hash the timestamp and plaintext
    uint256 msgHash;
    sha256.Write((const unsigned char*) &smsg.timestamp, sizeof(smsg.timestamp)).
           Write((const unsigned char*) &*message.begin(), message.size()).
           Finalize(msgHash.begin());

    // -- hash some entropy
    secure_buffer vchKey(64+4, 0);
    GetRandBytes(&vchKey[0], 16);
    sha256.Write(&vchKey[0], 16);
    sha256.Write(pubKeyS.begin(), 33);
    sha256.Finalize(&vchKey[0]); // use this as key for hmac

    // -- derive a new key pair, which is used for the encryption key computation
    CKey keyR;
    while (!keyR.IsValid()) {
        CHMAC_SHA256 &hmac = *new (&sha512 + 1) CHMAC_SHA256(&vchKey[0], 32);
        hmac.Write(&vchKey[64], 4); // prepend a counter
        hmac.Write(keyS.begin(), 32);
        hmac.Finalize(&vchKey[32]);
        keyR.Set(&vchKey[32], &vchKey[64], true);
        (*(uint32_t*) &vchKey[64])++;
    }

    // -- Compute a shared secret vchP derived from a new private key keyR and the public key of addressTo
    secure_buffer vchP;
    memcpy(smsg.cpkR, &keyR.GetPubKey()[0], sizeof(smsg.cpkR)); // copy public key
    if (!DeriveKey(vchP, keyR, cpkDestK))
    {
        LogPrintf("ECDH key computation failed\n");
        return 6;
    }

    // -- Use vchP and calculate the SHA512 hash H.
    //    The first 32 bytes of H are called key_e (encryption key) and the last 32 bytes are called key_m (key for HMAC).
    secure_buffer vchHashed(64); // 512 bit
    sha512.Write(&vchP[0], vchP.size());
    sha512.Finalize(&vchHashed[0]);


    secure_buffer vchPayload;
    secure_buffer vchCompressed;
    unsigned char* pMsgData;
    uint32_t lenMsgData;

    uint32_t lenMsg = message.size();
    if (lenMsg > 128)
    {
        // -- only compress if over 128 bytes
        int worstCase = LZ4_compressBound(message.size());
        try {
            vchCompressed.resize(worstCase);
        } catch (std::exception& e) {
            LogPrintf("vchCompressed.resize %u threw: %s.\n", worstCase, e.what());
            return 8;
        }

        int lenComp = LZ4_compress((char*)message.c_str(), (char*)&vchCompressed[0], lenMsg);
        if (lenComp < 1)
        {
            LogPrintf("Could not compress message data.\n");
            return 9;
        }

        pMsgData = &vchCompressed[0];
        lenMsgData = lenComp;

    } else
    {
        // -- no compression
        pMsgData = (unsigned char*)message.c_str();
        lenMsgData = lenMsg;
    }

    const unsigned int size = SMSG_PL_HDR_LEN + lenMsgData;
    try {
        vchPayload.resize(size);
    }
    catch (std::exception& e) {
        LogPrintf("vchPayload.resize %u threw: %s.\n", size, e.what());
        return 8;
    }

	// -- create the payload header
    PayloadHeader &header = *(PayloadHeader*) &vchPayload[0];
    memcpy(header.lenPlain, &lenMsg, 4);
    memcpy(&vchPayload[SMSG_PL_HDR_LEN], pMsgData, lenMsgData);
    memcpy(header.cpkS, pubKeyS.begin(), sizeof(header.cpkS));

    if (fSendAnonymous) {
        memset(header.sigKeyVersion, 0, 66);
    }
    else {
        // -- compact signature proves ownership of from address and allows the public key to be recovered, recipient can always reply.
        if (!pwallet->GetKey(ckidFrom, keyFrom))
        {
            LogPrintf("Could not get private key for addressFrom.\n");
            return 7;
        }
        LogPrintf("private key of %s obtained\n", EncodeDestination(PKHash(ckidFrom)).c_str());

        // -- sign the the new public key of keyR with the private key of addressFrom
        std::vector<unsigned char> vchSignature(65);
        keyFrom.SignCompact(msgHash, vchSignature);
        std::vector<unsigned char> sourcedata = Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS);

        header.sigKeyVersion[0] = sourcedata[0];
        memcpy(header.signature, &vchSignature[0], vchSignature.size());

        memset(&vchSignature[0], 0, vchSignature.size());
    }

    // -- use first 16 bytes of hash of cpkR as IV, fill up with zero
    //    less critical because the key shall be unique and the payload, too
    std::vector<unsigned char> vchIV(WALLET_CRYPTO_KEY_SIZE, 0);
    memcpy(&vchIV[0], Hash(smsg.cpkR, smsg.cpkR + sizeof(smsg.cpkR)).begin(), 16);

    // -- Init crypter
    CCrypter crypter;
    crypter.SetKey(secure_buffer(vchHashed.begin(), vchHashed.begin()+32), vchIV);

    // -- encrypt the plaintext
    if (!crypter.Encrypt(vchPayload, smsg.vchPayload)) {
        LogPrintf("crypter.Encrypt failed.\n");
        return 11;
    }
    smsg.nPayload = smsg.vchPayload.size();

    // -- calculate hash of encrypted payload
    memcpy(smsg.hash, Hash(&smsg.vchPayload[0], &smsg.vchPayload[smsg.nPayload]).begin(), 32);

    // -- Calculate a 32 byte MAC with HMACSHA256, using key_m as salt
    //    Message authentication code of the header
    CHMAC_SHA256 hmac(&vchHashed[32], 32);
    memcpy(smsg.mac, smsg.hash, 32);
    hmac.Write(smsg.begin(), SMSG_HDR_LEN);
    hmac.Finalize(smsg.mac);

    //TODO save the private key keyS
    return 0;
}

int SecureMsgSend(std::string& addressFrom, std::string& addressTo, std::string& message, std::string& sError)
{
    /* Encrypt secure message, and place it on the network
        Make a copy of the message to sender's first address and place in send queue db
        proof of work thread will pick up messages from  send queue db

    */
	std::shared_ptr<CWallet> pwallet = GetWallet("default");
	if (!pwallet)
	   return 0;
	   
    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecureMsgSend(%s, %s, ...)\n", addressFrom.c_str(), addressTo.c_str());

    if (pwallet->IsLocked()) {
        sError = "Wallet is locked, wallet must be unlocked to send and receive messages.";
        return RPC_WALLET_UNLOCK_NEEDED;
    }

    SecureMessage smsg;
    int rv = SecureMsgEncrypt(smsg, addressFrom, addressTo, message);

    if (rv != 0)
    {
        LogPrintf("SecureMsgSend(), encrypt for recipient failed.\n");
        std::ostringstream oss;

        switch(rv)
        {
            case 2:
                oss << "Message size of " << message.size() << " bytes exceeds the maximum message length of " << SMSG_MAX_MSG_BYTES << " bytes.";
                sError = oss.str();
                return RPC_INVALID_PARAMS;
            case 3:
                sError = "Invalid addressFrom.";
                return RPC_INVALID_ADDRESS_OR_KEY;
            case 4:
                sError = "Invalid addressTo.";
                return RPC_INVALID_ADDRESS_OR_KEY;
            case 5:  sError = "Could not get public key for addressTo.";    break;
            case 6:  sError = "ECDH key computation failed.";               break;
            case 7:  sError = "Could not get private key for addressFrom."; break;
            case 8:
                 sError = "Could not allocate memory.";
                 return RPC_OUT_OF_MEMORY;
            case 9:  sError = "Could not compress message data.";           break;
            case 11: sError = "Encrypt failed.";                            break;
            default: sError = "Unspecified Error.";                         break;
        }

        return rv;
    }


    // -- Place message in send queue, proof of work will happen in a thread.
    std::string sPrefix("qm");
    unsigned char chKey[18];
    memcpy(&chKey[0],  sPrefix.data(),      2);
    memcpy(&chKey[2],  &smsg.timestamp,     8);
    memcpy(&chKey[10], &smsg.vchPayload[0], 8);

    SecMsgStored smsgSQ;

    smsgSQ.timeReceived  = GetTime();
    smsgSQ.sAddrTo       = addressTo;

    try {
        smsgSQ.vchMessage.resize(SMSG_HDR_LEN + smsg.nPayload);
    } catch (std::exception& e) {
        LogPrintf("smsgSQ.vchMessage.resize %u threw: %s.\n", SMSG_HDR_LEN + smsg.nPayload, e.what());
        sError = "Could not allocate memory.";
        return RPC_OUT_OF_MEMORY;
    }

    memcpy(&smsgSQ.vchMessage[0], smsg.begin(), SMSG_HDR_LEN);
    memcpy(&smsgSQ.vchMessage[SMSG_HDR_LEN], &smsg.vchPayload[0], smsg.nPayload);

    {
        LOCK(cs_smsgDB);
        SecMsgDB dbSendQueue;
        if (dbSendQueue.Open("cw"))
        {
            dbSendQueue.WriteSmesg(chKey, smsgSQ);
            //NotifySecMsgSendQueueChanged(smsgOutbox);
        }
    }

    //  -- for outbox create a copy encrypted for owned address
    //     if the wallet is encrypted private key needed to decrypt will be unavailable

    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("Encrypting message for outbox.\n");

    std::string addressOutbox = "None";
    CTxDestination destr;
    LOCK(pwallet->cs_wallet);
    for (const auto& entry : pwallet->mapAddressBook)
    {
        // -- get first owned address
        if (IsMine(*pwallet, entry.first) != ISMINE_SPENDABLE)
            continue;

        destr = entry.first;
        addressOutbox = EncodeDestination(destr);
        break;
    }

    if (addressOutbox == "None")
    {
        LogPrintf("Warning: SecureMsgSend() could not find an address to encrypt outbox message with.\n");
    } else
    {
        if (LogInstance().WillLogCategory(BCLog::SMSG))
            LogPrintf("Encrypting a copy for outbox, using address %s\n", addressOutbox.c_str());

        SecureMessage smsgForOutbox;
        if ((rv = SecureMsgEncrypt(smsgForOutbox, addressFrom, addressOutbox, message)) != 0)
        {
            LogPrintf("SecureMsgSend(), encrypt for outbox failed, %d.\n", rv);
        } else
        {
            // -- save sent message to db
            std::string sPrefix("sm");
            unsigned char chKey[18];
            memcpy(&chKey[0],  sPrefix.data(),               2);
            memcpy(&chKey[2],  &smsgForOutbox.timestamp,     8);
            memcpy(&chKey[10], &smsgForOutbox.vchPayload[0], 8);   // sample

            SecMsgStored smsgOutbox;

            smsgOutbox.timeReceived  = GetTime();
            smsgOutbox.sAddrTo       = addressTo;
            smsgOutbox.sAddrOutbox   = addressOutbox;

            try {
                smsgOutbox.vchMessage.resize(SMSG_HDR_LEN + smsgForOutbox.nPayload);
            } catch (std::exception& e) {
                LogPrintf("smsgOutbox.vchMessage.resize %u threw: %s.\n", SMSG_HDR_LEN + smsgForOutbox.nPayload, e.what());
                sError = "Could not allocate memory.";
                return RPC_OUT_OF_MEMORY;
            }
            memcpy(&smsgOutbox.vchMessage[0], smsgForOutbox.begin(), SMSG_HDR_LEN);
            memcpy(&smsgOutbox.vchMessage[SMSG_HDR_LEN], &smsgForOutbox.vchPayload[0], smsgForOutbox.nPayload);


            {
                LOCK(cs_smsgDB);
                SecMsgDB dbSent;

                if (dbSent.Open("cw"))
                {
                    dbSent.WriteSmesg(chKey, smsgOutbox);
                    NotifySecMsgOutboxChanged(smsgOutbox);
                }
            }
        }
    }

    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("Secure message queued for sending to %s.\n", addressTo.c_str());

    return 0;
}


int SecureMsgDecrypt(bool fTestOnly, const std::string& address, const SecureMessageHeader &smsg, const unsigned char *pPayload, MessageData& msg) {

    /* Decrypt secure message

        address is the owned address to decrypt with.

        validate first in SecureMsgValidate

        returns
            1       Error
            2       Unknown version number
            3       Decrypt address is not valid.
            8       Could not allocate memory
    */

	std::shared_ptr<CWallet> pwallet = GetWallet("default");
	if (!pwallet)
	   return 0;

    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("SecureMsgDecrypt(), using %s, testonly %d.\n", address.c_str(), fTestOnly);

    if (smsg.nVersion != 1) {
        LogPrintf("Unknown version number.\n");
        return 2;
    }

    // -- Fetch private key k, used to decrypt
	CTxDestination dest = DecodeDestination(address);

	if (!IsValidDestination(dest))
    {
        LogPrintf("Address is not valid.\n");
        return RPC_INVALID_ADDRESS_OR_KEY;
    }

	CScript scriptPubKey = GetScriptForDestination(dest);
	std::string sPublicKey;
	CKeyID ckidDest = GetKeyForDestination(*pwallet, dest);

	if (ckidDest.IsNull())
    {
        LogPrintf("%d: ckidDest is null failed: %s, %s.\n", __LINE__, EncodeDestination(PKHash(ckidDest)).c_str(), address.c_str());
        return RPC_INVALID_ADDRESS_OR_KEY;
    }
		
    CKey keyDest;

    if (!pwallet->GetKey(ckidDest, keyDest)) {
        LogPrintf("Could not get private key for addressDest.\n");
        return 3;
    }

    CPubKey keyR(smsg.cpkR, smsg.cpkR+33);
    if (!keyR.IsValid()) {
        LogPrintf("Could not get compressed public key for key R.\n");
        return 1;
    }


    // -- Do an EC point multiply with private key k and public key R. This gives you public EC key P.
    secure_buffer vchP;
    if (!DeriveKey(vchP, keyDest, keyR)) {
        LogPrintf("ECDH key derivation failed\n");
        return 1;
    }


    // -- Use public key P to calculate the SHA512 hash H.
    //    The first 32 bytes of H are called key_e and the last 32 bytes are called key_m.
    secure_buffer vchHashedDec(64); // 512 bits
    secure_buffer sha512_mem(sizeof(CSHA512), 0);
    CSHA512 &sha512 = *new (&sha512_mem[0]) CSHA512();
    sha512.Write(&vchP[0], vchP.size());
    sha512.Finalize(&vchHashedDec[0]);
    sha512.~CSHA512();

    // -- Message authentication code of header
    SecureMessageHeader smsg_(smsg.begin());
    memcpy(smsg_.mac, smsg.hash, 32);
    unsigned char mac[32];
    CHMAC_SHA256 hmac(&vchHashedDec[32], 32);
    hmac.Write((const unsigned char*) smsg_.begin(), SMSG_HDR_LEN);
    hmac.Finalize(mac);

    if (memcmp(mac, smsg.mac, 32) != 0) {
        if (LogInstance().WillLogCategory(BCLog::SMSG))
            LogPrintf("MAC does not match for address %s.\n", EncodeDestination(dest).c_str()); // expected if message is not to address on node
        return 1;
    }

    if (fTestOnly)
        return 0;

    std::vector<unsigned char> vchIV(WALLET_CRYPTO_KEY_SIZE, 0);
    memcpy(&vchIV[0], Hash(smsg.cpkR, smsg.cpkR + sizeof(smsg.cpkR)).begin(), 16);

    CCrypter crypter;
    crypter.SetKey(secure_buffer(vchHashedDec.begin(), vchHashedDec.begin()+32), vchIV);
    //secure_buffer vchPayload;
    CKeyingMaterial vchPayload;
	std::vector<unsigned char> vchm;
	memcpy(&vchm[0], &pPayload[0], smsg.nPayload);

    if (!crypter.Decrypt(vchm, vchPayload)) {
        LogPrintf("Decrypt failed.\n");
        return 1;
    }

    PayloadHeader &header = *(PayloadHeader*) &vchPayload[0];
    bool fFromAnonymous = (header.sigKeyVersion[0] == 0);
    uint32_t lenData = vchPayload.size() - SMSG_PL_HDR_LEN;
    uint32_t lenPlain;
    memcpy(&lenPlain, header.lenPlain, 4);
    unsigned char* pMsgData = &vchPayload[SMSG_PL_HDR_LEN];

    try {
        msg.sMessage.reserve(lenPlain + 1);
        msg.sMessage.resize(lenPlain);
    }
    catch (std::exception& e) {
        LogPrintf("msg.vchMessage.resize %u threw: %s.\n", lenPlain, e.what());
        return 8;
    }
    char *message = &msg.sMessage[0];

    if (lenPlain > 128) {
        // -- decompress
        if (LZ4_decompress_safe((char*) pMsgData, message, lenData, lenPlain) != (int) lenPlain) {
            LogPrintf("Could not decompress message data.\n");
            return 1;
        }
    }
    else {
        // -- plaintext
        memcpy(message, pMsgData, lenPlain);
    }

    if (fFromAnonymous) {
        // -- Anonymous sender
        msg.sFromAddress = "anon";
        LogPrintf("from anon\n");
    }
    else {
        uint256 msgHash;
        CSHA256().Write((const unsigned char*) &smsg.timestamp, 8).
                  Write((const unsigned char*) message, lenPlain).
                  Finalize(msgHash.begin());

        CPubKey cpkFromSig;
        std::vector<unsigned char> vchSig(65, 0);
        memcpy(&vchSig[0], header.signature, vchSig.size());
        bool valid = cpkFromSig.RecoverCompact(msgHash, vchSig);
        if (!valid) {
            LogPrintf("Signature validation failed.\n");
            return 1;
        }
        if (!cpkFromSig.IsCompressed())
            LogPrintf("key is not compressed\n");

		// TODO check whether the public key is trusted
        CKeyID ckidFrom(cpkFromSig.GetID());
        int rv = 5;
        try {
            rv = SecureMsgInsertAddress(ckidFrom, cpkFromSig);
        }
        catch (std::exception& e) {
            LogPrintf("SecureMsgInsertAddress(), exception: %s.\n", e.what());
            //return 1;
        }

        switch(rv) {
            case 0:
                LogPrintf("Sender public key added to db.\n");
                break;
            case 4:
                LogPrintf("Sender public key already in db.\n");
                break;
            default:
                LogPrintf("Error adding sender public key to db.\n");
                break;
        }

        msg.sFromAddress = secure_string(EncodeDestination(PKHash(ckidFrom)).c_str());
    }

    msg.sToAddress = secure_string(address.c_str());
    msg.timestamp = smsg.timestamp;
    // TODO insert new public key and link it with cpkFromSig

    if (LogInstance().WillLogCategory(BCLog::SMSG))
        LogPrintf("Decrypted message for %s: %s.\n", address.c_str(), msg.sMessage.c_str());

    return 0;
}

int SecureMsgDecrypt(const SecMsgStored& smsgStored, MessageData &msg, std::string &errorMsg)
{
    SecureMessageHeader smsg(&smsgStored.vchMessage[0]);
    const unsigned char* pPayload = &smsgStored.vchMessage[SMSG_HDR_LEN];
    memcpy(smsg.hash, Hash(&pPayload[0], &pPayload[smsg.nPayload]).begin(), 32);
    int error = SecureMsgDecrypt(false, smsgStored.sAddrTo, smsg, pPayload, msg);
    errorMsg = "";
    return error;
}
