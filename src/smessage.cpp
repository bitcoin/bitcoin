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

#include "smessage.h"

#include <stdint.h>
#include <time.h>
#include <map>
#include <stdexcept>
#include <sstream>
#include <errno.h>

#include <openssl/crypto.h>
#include <openssl/ec.h>
#include <openssl/ecdh.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>


#include "base58.h"
#include "db.h"
#include "init.h" // pwalletMain
#include "txdb.h"
#include "sync.h"
#include "eckey.h"

#include "lz4/lz4.c"

#include "xxhash/xxhash.h"
#include "xxhash/xxhash.c"


boost::thread_group threadGroupSmsg;

// TODO: For buckets older than current, only need to store no. messages and hash in memory

boost::signals2::signal<void (SecMsgStored& inboxHdr)>  NotifySecMsgInboxChanged;
boost::signals2::signal<void (SecMsgStored& outboxHdr)> NotifySecMsgOutboxChanged;
boost::signals2::signal<void ()> NotifySecMsgWalletUnlocked;

bool fSecMsgEnabled = false;

std::map<int64_t, SecMsgBucket> smsgBuckets;
std::vector<SecMsgAddress>      smsgAddresses;
SecMsgOptions                   smsgOptions;


CCriticalSection cs_smsg;
CCriticalSection cs_smsgDB;
CCriticalSection cs_smsgThreads;

leveldb::DB *smsgDB = NULL;


namespace fs = boost::filesystem;

bool SecMsgCrypter::SetKey(const std::vector<uint8_t>& vchNewKey, uint8_t* chNewIV)
{
    if (vchNewKey.size() < sizeof(chKey))
        return false;

    return SetKey(&vchNewKey[0], chNewIV);
};

bool SecMsgCrypter::SetKey(const uint8_t* chNewKey, uint8_t* chNewIV)
{
    // -- for EVP_aes_256_cbc() key must be 256 bit, iv must be 128 bit.
    memcpy(&chKey[0], chNewKey, sizeof(chKey));
    memcpy(chIV, chNewIV, sizeof(chIV));

    fKeySet = true;
    return true;
};

bool SecMsgCrypter::Encrypt(uint8_t* chPlaintext, uint32_t nPlain, std::vector<uint8_t> &vchCiphertext)
{
    if (!fKeySet)
        return false;

    // -- max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE - 1 bytes
    int nLen = nPlain;

    int nCLen = nLen + AES_BLOCK_SIZE, nFLen = 0;
    vchCiphertext = std::vector<uint8_t> (nCLen);

    EVP_CIPHER_CTX ctx;

    bool fOk = true;

    EVP_CIPHER_CTX_init(&ctx);
    if (fOk) fOk = EVP_EncryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL, &chKey[0], &chIV[0]);
    if (fOk) fOk = EVP_EncryptUpdate(&ctx, &vchCiphertext[0], &nCLen, chPlaintext, nLen);
    if (fOk) fOk = EVP_EncryptFinal_ex(&ctx, (&vchCiphertext[0])+nCLen, &nFLen);
    EVP_CIPHER_CTX_cleanup(&ctx);

    if (!fOk)
        return false;

    vchCiphertext.resize(nCLen + nFLen);

    return true;
};

bool SecMsgCrypter::Decrypt(uint8_t* chCiphertext, uint32_t nCipher, std::vector<uint8_t>& vchPlaintext)
{
    if (!fKeySet)
        return false;

    // plaintext will always be equal to or lesser than length of ciphertext
    int nPLen = nCipher, nFLen = 0;

    vchPlaintext.resize(nCipher);

    EVP_CIPHER_CTX ctx;

    bool fOk = true;

    EVP_CIPHER_CTX_init(&ctx);
    if (fOk) fOk = EVP_DecryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL, &chKey[0], &chIV[0]);
    if (fOk) fOk = EVP_DecryptUpdate(&ctx, &vchPlaintext[0], &nPLen, &chCiphertext[0], nCipher);
    if (fOk) fOk = EVP_DecryptFinal_ex(&ctx, (&vchPlaintext[0])+nPLen, &nFLen);
    EVP_CIPHER_CTX_cleanup(&ctx);

    if (!fOk)
        return false;

    vchPlaintext.resize(nPLen + nFLen);

    return true;
};

void SecMsgBucket::hashBucket()
{
    if (fDebugSmsg)
        LogPrintf("SecMsgBucket::hashBucket()\n");
    
    timeChanged = GetTime();
    
    std::set<SecMsgToken>::iterator it;
    
    void* state = XXH32_init(1);
    
    for (it = setTokens.begin(); it != setTokens.end(); ++it)
    {
        XXH32_update(state, it->sample, 8);
    };
    
    hash = XXH32_digest(state);
    
    if (fDebugSmsg)
        LogPrintf("Hashed %u messages, hash %u\n", setTokens.size(), hash);
};


bool SecMsgDB::Open(const char* pszMode)
{
    if (smsgDB)
    {
        pdb = smsgDB;
        return true;
    };

    bool fCreate = strchr(pszMode, 'c');

    fs::path fullpath = GetDataDir() / "smsgDB";

    if (!fCreate
        && (!fs::exists(fullpath)
            || !fs::is_directory(fullpath)))
    {
        LogPrintf("SecMsgDB::open() - DB does not exist.\n");
        return false;
    };

    leveldb::Options options;
    options.create_if_missing = fCreate;
    leveldb::Status s = leveldb::DB::Open(options, fullpath.string(), &smsgDB);

    if (!s.ok())
    {
        LogPrintf("SecMsgDB::open() - Error opening db: %s.\n", s.ToString().c_str());
        return false;
    };

    pdb = smsgDB;

    return true;
};


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
        };
    };

    virtual void Delete(const leveldb::Slice& key)
    {
        if (key.ToString() == needle)
        {
            foundEntry = true;
            *deleted = true;
        };
    };
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
    };

    return scanner.foundEntry;
}

bool SecMsgDB::TxnBegin()
{
    if (activeBatch)
        return true;
    activeBatch = new leveldb::WriteBatch();
    return true;
};

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
    };

    return true;
};

bool SecMsgDB::TxnAbort()
{
    delete activeBatch;
    activeBatch = NULL;
    return true;
};

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
    };

    if (readFromDb)
    {
        leveldb::Status s = pdb->Get(leveldb::ReadOptions(), ssKey.str(), &strValue);
        if (!s.ok())
        {
            if (s.IsNotFound())
                return false;
            LogPrintf("LevelDB read failure: %s\n", s.ToString().c_str());
            return false;
        };
    };

    try {
        CDataStream ssValue(strValue.data(), strValue.data() + strValue.size(), SER_DISK, CLIENT_VERSION);
        ssValue >> pubkey;
    } catch (std::exception& e) {
        LogPrintf("SecMsgDB::ReadPK() unserialize threw: %s.\n", e.what());
        return false;
    }

    return true;
};

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
    };

    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;
    leveldb::Status s = pdb->Put(writeOptions, ssKey.str(), ssValue.str());
    if (!s.ok())
    {
        LogPrintf("SecMsgDB write failure: %s\n", s.ToString().c_str());
        return false;
    };

    return true;
};

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
        };
    };

    leveldb::Status s = pdb->Get(leveldb::ReadOptions(), ssKey.str(), &unused);
    return s.IsNotFound() == false;
};


bool SecMsgDB::NextSmesg(leveldb::Iterator* it, std::string& prefix, uint8_t* chKey, SecMsgStored& smsgStored)
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
};

bool SecMsgDB::NextSmesgKey(leveldb::Iterator* it, std::string& prefix, uint8_t* chKey)
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
};

bool SecMsgDB::ReadSmesg(uint8_t* chKey, SecMsgStored& smsgStored)
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
    };

    if (readFromDb)
    {
        leveldb::Status s = pdb->Get(leveldb::ReadOptions(), ssKey.str(), &strValue);
        if (!s.ok())
        {
            if (s.IsNotFound())
                return false;
            LogPrintf("LevelDB read failure: %s\n", s.ToString().c_str());
            return false;
        };
    };

    try {
        CDataStream ssValue(strValue.data(), strValue.data() + strValue.size(), SER_DISK, CLIENT_VERSION);
        ssValue >> smsgStored;
    } catch (std::exception& e) {
        LogPrintf("SecMsgDB::ReadSmesg() unserialize threw: %s.\n", e.what());
        return false;
    }

    return true;
};

bool SecMsgDB::WriteSmesg(uint8_t* chKey, SecMsgStored& smsgStored)
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
    };

    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;
    leveldb::Status s = pdb->Put(writeOptions, ssKey.str(), ssValue.str());
    if (!s.ok())
    {
        LogPrintf("SecMsgDB write failed: %s\n", s.ToString().c_str());
        return false;
    };

    return true;
};

bool SecMsgDB::ExistsSmesg(uint8_t* chKey)
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
        };
    };

    leveldb::Status s = pdb->Get(leveldb::ReadOptions(), ssKey.str(), &unused);
    return s.IsNotFound() == false;
    return true;
};

bool SecMsgDB::EraseSmesg(uint8_t* chKey)
{
    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    ssKey.write((const char*)chKey, 18);

    if (activeBatch)
    {
        activeBatch->Delete(ssKey.str());
        return true;
    };

    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;
    leveldb::Status s = pdb->Delete(writeOptions, ssKey.str());

    if (s.ok() || s.IsNotFound())
        return true;
    LogPrintf("SecMsgDB erase failed: %s\n", s.ToString().c_str());
    return false;
};

void ThreadSecureMsg()
{
    // -- bucket management thread
    
    std::vector<std::pair<int64_t, NodeId> > vTimedOutLocks;
    while (fSecMsgEnabled)
    {
        int64_t now = GetTime();

        if (fDebugSmsg)
            LogPrintf("SecureMsgThread %d \n", now);
        
        vTimedOutLocks.resize(0);
        
        int64_t cutoffTime = now - SMSG_RETENTION;
        {
            LOCK(cs_smsg);
            
            for (std::map<int64_t, SecMsgBucket>::iterator it(smsgBuckets.begin()); it != smsgBuckets.end(); it++)
            {
                //if (fDebugSmsg)
                //    LogPrintf("Checking bucket %d", size %u \n", it->first, it->second.setTokens.size());
                if (it->first < cutoffTime)
                {
                    if (fDebugSmsg)
                        LogPrintf("Removing bucket %d \n", it->first);

                    std::string fileName = boost::lexical_cast<std::string>(it->first);

                    fs::path fullPath = GetDataDir() / "smsgStore" / (fileName + "_01.dat");
                    if (fs::exists(fullPath))
                    {
                        try { fs::remove(fullPath);
                        } catch (const fs::filesystem_error& ex)
                        {
                            LogPrintf("Error removing bucket file %s.\n", ex.what());
                        };
                    } else
                    {
                        LogPrintf("Path %s does not exist \n", fullPath.string().c_str());
                    };
                    
                    // -- look for a wl file, it stores incoming messages when wallet is locked
                    fullPath = GetDataDir() / "smsgStore" / (fileName + "_01_wl.dat");
                    if (fs::exists(fullPath))
                    {
                        try { fs::remove(fullPath);
                        } catch (const fs::filesystem_error& ex)
                        {
                            LogPrintf("Error removing wallet locked file %s.\n", ex.what());
                        };
                    };

                    smsgBuckets.erase(it);
                } else
                if (it->second.nLockCount > 0) // -- tick down nLockCount, so will eventually expire if peer never sends data
                {
                    it->second.nLockCount--;

                    if (it->second.nLockCount == 0)     // lock timed out
                    {
                        vTimedOutLocks.push_back(std::make_pair(it->first, it->second.nLockPeerId)); // cs_vNodes 
                        
                        it->second.nLockPeerId = 0;
                    }; // if (it->second.nLockCount == 0)
                    
                }; // ! if (it->first < cutoffTime)
            };
        } // cs_smsg
        
        for (std::vector<std::pair<int64_t, NodeId> >::iterator it(vTimedOutLocks.begin()); it != vTimedOutLocks.end(); it++)
        {
            NodeId nPeerId = it->second;
            int64_t ignoreUntil = GetTime() + SMSG_TIME_IGNORE;

            if (fDebugSmsg)
                LogPrintf("Lock on bucket %d for peer %d timed out.\n", it->first, nPeerId);

            // -- look through the nodes for the peer that locked this bucket
            
            {
                LOCK(cs_vNodes);
                BOOST_FOREACH(CNode* pnode, vNodes)
                {
                    if (pnode->id != nPeerId)
                        continue;
                    LOCK2(pnode->cs_vSend, pnode->smsgData.cs_smsg_net);
                    pnode->smsgData.ignoreUntil = ignoreUntil;
                    
                    // -- alert peer that they are being ignored
                    std::vector<uint8_t> vchData;
                    vchData.resize(8);
                    memcpy(&vchData[0], &ignoreUntil, 8);
                    pnode->PushMessage("smsgIgnore", vchData);

                    if (fDebugSmsg)
                        LogPrintf("This node will ignore peer %d until %d.\n", nPeerId, ignoreUntil);
                    break;
                };
            } // cs_vNodes
        };
        
        MilliSleep(SMSG_THREAD_DELAY * 1000); //  // check every SMSG_THREAD_DELAY seconds
    };
};

void ThreadSecureMsgPow()
{
    // -- proof of work thread

    int rv;
    std::vector<uint8_t> vchKey;
    SecMsgStored smsgStored;

    std::string sPrefix("qm");
    uint8_t chKey[18];


    while (fSecMsgEnabled)
    {
        // -- sleep at end, then fSecMsgEnabled is tested on wake

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

            uint8_t* pHeader = &smsgStored.vchMessage[0];
            uint8_t* pPayload = &smsgStored.vchMessage[SMSG_HDR_LEN];
            SecureMessage* psmsg = (SecureMessage*) pHeader;

            // -- do proof of work
            rv = SecureMsgSetHash(pHeader, pPayload, psmsg->nPayload);
            if (rv == 2)
                break; // leave message in db, if terminated due to shutdown

            // -- message is removed here, no matter what
            {
                LOCK(cs_smsgDB);
                dbOutbox.EraseSmesg(chKey);
            }
            if (rv != 0)
            {
                LogPrintf("SecMsgPow: Could not get proof of work hash, message removed.\n");
                continue;
            };

            // -- add to message store
            {
                LOCK(cs_smsg);
                if (SecureMsgStore(pHeader, pPayload, psmsg->nPayload, true) != 0)
                {
                    LogPrintf("SecMsgPow: Could not place message in buckets, message removed.\n");
                    continue;
                };
            }

            // -- test if message was sent to self
            if (SecureMsgScanMessage(pHeader, pPayload, psmsg->nPayload, true) != 0)
            {
                // message recipient is not this node (or failed)
            };
        };

        {
            LOCK(cs_smsg);
            delete it;
        }

        // -- shutdown thread waits 5 seconds, this should be less
        MilliSleep(2000); // seconds
    };
};

int SecureMsgBuildBucketSet()
{
    /*
        Build the bucket set by scanning the files in the smsgStore dir.

        smsgBuckets should be empty
    */

    if (fDebugSmsg)
        LogPrintf("SecureMsgBuildBucketSet()\n");

    int64_t  now            = GetTime();
    uint32_t nFiles         = 0;
    uint32_t nMessages      = 0;

    fs::path pathSmsgDir = GetDataDir() / "smsgStore";
    fs::directory_iterator itend;


    if (!fs::exists(pathSmsgDir)
        || !fs::is_directory(pathSmsgDir))
    {
        LogPrintf("Message store directory does not exist.\n");
        return 0; // not an error
    }


    for (fs::directory_iterator itd(pathSmsgDir) ; itd != itend ; ++itd)
    {
        if (!fs::is_regular_file(itd->status()))
            continue;

        std::string fileType = (*itd).path().extension().string();

        if (fileType.compare(".dat") != 0)
            continue;

        std::string fileName = (*itd).path().filename().string();


        if (fDebugSmsg)
            LogPrintf("Processing file: %s.\n", fileName.c_str());

        nFiles++;

        // TODO files must be split if > 2GB
        // time_noFile.dat
        size_t sep = fileName.find_first_of("_");
        if (sep == std::string::npos)
            continue;

        std::string stime = fileName.substr(0, sep);

        int64_t fileTime = boost::lexical_cast<int64_t>(stime);

        if (fileTime < now - SMSG_RETENTION)
        {
            LogPrintf("Dropping file %s, expired.\n", fileName.c_str());
            try {
                fs::remove((*itd).path());
            } catch (const fs::filesystem_error& ex)
            {
                LogPrintf("Error removing bucket file %s, %s.\n", fileName.c_str(), ex.what());
            };
            continue;
        };

        if (boost::algorithm::ends_with(fileName, "_wl.dat"))
        {
            if (fDebugSmsg)
                LogPrintf("Skipping wallet locked file: %s.\n", fileName.c_str());
            continue;
        };

        size_t nTokenSetSize = 0;
        SecureMessage smsg;
        {
            LOCK(cs_smsg);
            
            std::set<SecMsgToken>& tokenSet = smsgBuckets[fileTime].setTokens;
            
            FILE *fp;

            if (!(fp = fopen((*itd).path().string().c_str(), "rb")))
            {
                LogPrintf("Error opening file: %s\n", strerror(errno));
                continue;
            };

            for (;;)
            {
                long int ofs = ftell(fp);
                SecMsgToken token;
                token.offset = ofs;
                errno = 0;
                if (fread(&smsg.hash[0], sizeof(uint8_t), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN)
                {
                    if (errno != 0)
                    {
                        LogPrintf("fread header failed: %s\n", strerror(errno));
                    } else
                    {
                        //LogPrintf("End of file.\n");
                    };
                    break;
                };
                token.timestamp = smsg.timestamp;

                if (smsg.nPayload < 8)
                    continue;

                if (fread(token.sample, sizeof(uint8_t), 8, fp) != 8)
                {
                    LogPrintf("fread data failed: %s\n", strerror(errno));
                    break;
                };

                if (fseek(fp, smsg.nPayload-8, SEEK_CUR) != 0)
                {
                    LogPrintf("fseek, strerror: %s.\n", strerror(errno));
                    break;
                };

                tokenSet.insert(token);
            };

            fclose(fp);
            
            smsgBuckets[fileTime].hashBucket();
            
            nTokenSetSize = tokenSet.size();
        } // LOCK(cs_smsg);
        
        nMessages += nTokenSetSize;
        if (fDebugSmsg)
            LogPrintf("Bucket %d contains %u messages.\n", fileTime, nTokenSetSize);
    };

    LogPrintf("Processed %u files, loaded %u buckets containing %u messages.\n", nFiles, smsgBuckets.size(), nMessages);

    return 0;
};

int SecureMsgAddWalletAddresses()
{
    if (fDebugSmsg)
        LogPrintf("SecureMsgAddWalletAddresses()\n");

    std::string sAnonPrefix("ao ");

    uint32_t nAdded = 0;
    BOOST_FOREACH(const PAIRTYPE(CTxDestination, std::string)& entry, pwalletMain->mapAddressBook)
    {
        if (!IsMine(*pwalletMain, entry.first))
            continue;

        // -- skip addresses for anon outputs
        if (entry.second.compare(0, sAnonPrefix.length(), sAnonPrefix) == 0)
            continue;

        // TODO: skip addresses for stealth transactions

        CBitcoinAddress coinAddress(entry.first);
        if (!coinAddress.IsValid())
            continue;

        std::string address;
        std::string strPublicKey;
        address = coinAddress.ToString();

        bool fExists        = 0;
        for (std::vector<SecMsgAddress>::iterator it = smsgAddresses.begin(); it != smsgAddresses.end(); ++it)
        {
            if (address != it->sAddress)
                continue;
            fExists = 1;
            break;
        };

        if (fExists)
            continue;

        bool recvEnabled    = 1;
        bool recvAnon       = 1;

        smsgAddresses.push_back(SecMsgAddress(address, recvEnabled, recvAnon));
        nAdded++;
    };

    if (fDebugSmsg)
        LogPrintf("Added %u addresses to whitelist.\n", nAdded);

    return 0;
};


int SecureMsgReadIni()
{
    if (!fSecMsgEnabled)
        return false;

    if (fDebugSmsg)
        LogPrintf("SecureMsgReadIni()\n");

    fs::path fullpath = GetDataDir() / "smsg.ini";


    FILE *fp;
    errno = 0;
    if (!(fp = fopen(fullpath.string().c_str(), "r")))
    {
        LogPrintf("Error opening file: %s\n", strerror(errno));
        return 1;
    };

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
        };
    };

    LogPrintf("Loaded %u addresses.\n", smsgAddresses.size());

    fclose(fp);

    return 0;
};

int SecureMsgWriteIni()
{
    if (!fSecMsgEnabled)
        return false;

    if (fDebugSmsg)
        LogPrintf("SecureMsgWriteIni()\n");

    fs::path fullpath = GetDataDir() / "smsg.ini~";

    FILE *fp;
    errno = 0;
    if (!(fp = fopen(fullpath.string().c_str(), "w")))
    {
        LogPrintf("Error opening file: %s\n", strerror(errno));
        return 1;
    };

    if (fwrite("[Options]\n", sizeof(char), 10, fp) != 10)
    {
        LogPrintf("fwrite error: %s\n", strerror(errno));
        fclose(fp);
        return false;
    };

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
    };
    for (std::vector<SecMsgAddress>::iterator it = smsgAddresses.begin(); it != smsgAddresses.end(); ++it)
    {
        errno = 0;
        if (fprintf(fp, "key=%s|%d|%d\n", it->sAddress.c_str(), it->fReceiveEnabled, it->fReceiveAnon) < 0)
        {
            LogPrintf("fprintf error: %s\n", strerror(errno));
            continue;
        };
    };


    fclose(fp);


    try {
        fs::path finalpath = GetDataDir() / "smsg.ini";
        fs::rename(fullpath, finalpath);
    } catch (const fs::filesystem_error& ex)
    {
        LogPrintf("Error renaming file %s, %s.\n", fullpath.string().c_str(), ex.what());
    };
    return 0;
};


/** called from AppInit2() in init.cpp */
bool SecureMsgStart(bool fDontStart, bool fScanChain)
{
    if (fDontStart)
    {
        LogPrintf("Secure messaging not started.\n");
        return false;
    };

    LogPrintf("Secure messaging starting.\n");

    fSecMsgEnabled = true;

    if (SecureMsgReadIni() != 0)
        LogPrintf("Failed to read smsg.ini\n");

    if (smsgAddresses.size() < 1)
    {
        LogPrintf("No address keys loaded.\n");
        if (SecureMsgAddWalletAddresses() != 0)
            LogPrintf("Failed to load addresses from wallet.\n");
    };

    if (fScanChain)
    {
        SecureMsgScanBlockChain();
    };

    if (SecureMsgBuildBucketSet() != 0)
    {
        LogPrintf("SecureMsg could not load bucket sets, secure messaging disabled.\n");
        fSecMsgEnabled = false;
        return false;
    };
    
    threadGroupSmsg.create_thread(boost::bind(&TraceThread<void (*)()>, "smsg", &ThreadSecureMsg));
    threadGroupSmsg.create_thread(boost::bind(&TraceThread<void (*)()>, "smsg-pow", &ThreadSecureMsgPow));
    
    
    /*
    // -- start threads
    if (!NewThread(ThreadSecureMsg, NULL)
        || !NewThread(ThreadSecureMsgPow, NULL))
    {
        LogPrintf("SecureMsg could not start threads, secure messaging disabled.\n");
        fSecMsgEnabled = false;
        return false;
    };
    */
    return true;
};

bool SecureMsgShutdown()
{
    if (!fSecMsgEnabled)
        return false;

    LogPrintf("Stopping secure messaging.\n");


    if (SecureMsgWriteIni() != 0)
        LogPrintf("Failed to save smsg.ini\n");

    fSecMsgEnabled = false;
    
    threadGroupSmsg.interrupt_all();
    threadGroupSmsg.join_all();

    if (smsgDB)
    {
        LOCK(cs_smsgDB);
        delete smsgDB;
        smsgDB = NULL;
    };

    return true;
};

bool SecureMsgEnable()
{
    // -- start secure messaging at runtime
    if (fSecMsgEnabled)
    {
        LogPrintf("SecureMsgEnable: secure messaging is already enabled.\n");
        return false;
    };

    {
        LOCK(cs_smsg);
        fSecMsgEnabled = true;

        smsgAddresses.clear(); // should be empty already
        if (SecureMsgReadIni() != 0)
            LogPrintf("Failed to read smsg.ini\n");

        if (smsgAddresses.size() < 1)
        {
            LogPrintf("No address keys loaded.\n");
            if (SecureMsgAddWalletAddresses() != 0)
                LogPrintf("Failed to load addresses from wallet.\n");
        };

        smsgBuckets.clear(); // should be empty already

        if (SecureMsgBuildBucketSet() != 0)
        {
            LogPrintf("SecureMsgEnable: could not load bucket sets, secure messaging disabled.\n");
            fSecMsgEnabled = false;
            return false;
        };

    } // cs_smsg
    
    // -- start threads
    threadGroupSmsg.create_thread(boost::bind(&TraceThread<void (*)()>, "smsg", &ThreadSecureMsg));
    threadGroupSmsg.create_thread(boost::bind(&TraceThread<void (*)()>, "smsg-pow", &ThreadSecureMsgPow));
    
    /*
    if (!NewThread(ThreadSecureMsg, NULL)
        || !NewThread(ThreadSecureMsgPow, NULL))
    {
        LogPrintf("SecureMsgEnable could not start threads, secure messaging disabled.\n");
        fSecMsgEnabled = false;
        return false;
    };
    */
    // -- ping each peer, don't know which have messaging enabled
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
        {
            pnode->PushMessage("smsgPing");
            pnode->PushMessage("smsgPong"); // Send pong as have missed initial ping sent by peer when it connected
        };
    } // cs_vNodes
    LogPrintf("Secure messaging enabled.\n");
    return true;
};

bool SecureMsgDisable()
{
    // -- stop secure messaging at runtime
    if (!fSecMsgEnabled)
    {
        LogPrintf("SecureMsgDisable: secure messaging is already disabled.\n");
        return false;
    };
    
    {
        LOCK(cs_smsg);
        fSecMsgEnabled = false;
        
        threadGroupSmsg.interrupt_all();
        threadGroupSmsg.join_all();
        
        // -- clear smsgBuckets
        std::map<int64_t, SecMsgBucket>::iterator it;
        it = smsgBuckets.begin();
        for (it = smsgBuckets.begin(); it != smsgBuckets.end(); ++it)
        {
            it->second.setTokens.clear();
        };
        smsgBuckets.clear();
        smsgAddresses.clear();
    } // cs_smsg
    
    // -- tell each smsg enabled peer that this node is disabling
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
        {
            if (!pnode->smsgData.fEnabled)
                continue;
            LOCK2(pnode->cs_vSend, pnode->smsgData.cs_smsg_net);
            pnode->PushMessage("smsgDisabled");
            pnode->smsgData.fEnabled = false;
        };
    } // cs_vNodes


    if (SecureMsgWriteIni() != 0)
        LogPrintf("Failed to save smsg.ini\n");

    // -- allow time for threads to stop
    MilliSleep(3000); // seconds
    // TODO be certain that threads have stopped

    if (smsgDB)
    {
        LOCK(cs_smsgDB);
        delete smsgDB;
        smsgDB = NULL;
    };


    LogPrintf("Secure messaging disabled.\n");
    return true;
};


bool SecureMsgReceiveData(CNode* pfrom, std::string strCommand, CDataStream& vRecv)
{
    /*
        Called from ProcessMessage
        Runs in ThreadMessageHandler2
    */

    if (fDebugSmsg)
        LogPrintf("SecureMsgReceiveData() %s %s.\n", pfrom->addrName.c_str(), strCommand.c_str());
    
    
    
    if (strCommand == "smsgInv")
    {
        std::vector<uint8_t> vchData;
        vRecv >> vchData;

        if (vchData.size() < 4)
        {
            pfrom->Misbehaving(1);
            return false; // not enough data received to be a valid smsgInv
        };

        int64_t now = GetTime();
        
        {
            LOCK(pfrom->smsgData.cs_smsg_net);
                
            if (now < pfrom->smsgData.ignoreUntil)
            {
                if (fDebugSmsg)
                    LogPrintf("Node is ignoring peer %d until %d.\n", pfrom->id, pfrom->smsgData.ignoreUntil);
                return false;
            };
        }
        
        uint32_t nBuckets       = smsgBuckets.size();
        uint32_t nLocked        = 0;    // no. of locked buckets on this node
        uint32_t nInvBuckets;           // no. of bucket headers sent by peer in smsgInv
        memcpy(&nInvBuckets, &vchData[0], 4);
        if (fDebugSmsg)
            LogPrintf("Remote node sent %d bucket headers, this has %d.\n", nInvBuckets, nBuckets);


        // -- Check no of buckets:
        if (nInvBuckets > (SMSG_RETENTION / SMSG_BUCKET_LEN) + 1) // +1 for some leeway
        {
            LogPrintf("Peer sent more bucket headers than possible %u, %u.\n", nInvBuckets, (SMSG_RETENTION / SMSG_BUCKET_LEN));
            pfrom->Misbehaving(1);
            return false;
        };

        if (vchData.size() < 4 + nInvBuckets*16)
        {
            LogPrintf("Remote node did not send enough data.\n");
            pfrom->Misbehaving(1);
            return false;
        };

        std::vector<uint8_t> vchDataOut;
        vchDataOut.reserve(4 + 8 * nInvBuckets); // reserve max possible size
        vchDataOut.resize(4);
        uint32_t nShowBuckets = 0;


        uint8_t *p = &vchData[4];
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
                if (fDebugSmsg)
                    LogPrintf("Not interested in peer bucket %d, has expired.\n", time);

                if (time < now - SMSG_RETENTION - SMSG_TIME_LEEWAY)
                    pfrom->Misbehaving(1);
                continue;
            };
            if (time > now + SMSG_TIME_LEEWAY)
            {
                if (fDebugSmsg)
                    LogPrintf("Not interested in peer bucket %d, in the future.\n", time);
                pfrom->Misbehaving(1);
                continue;
            };

            if (ncontent < 1)
            {
                if (fDebugSmsg)
                    LogPrintf("Peer sent empty bucket, ignore %d %u %u.\n", time, ncontent, hash);
                continue;
            };

            if (fDebugSmsg)
            {
                LogPrintf("peer bucket %d %u %u.\n", time, ncontent, hash);
                LogPrintf("this bucket %d %u %u.\n", time, smsgBuckets[time].setTokens.size(), smsgBuckets[time].hash);
            };
            {
            LOCK(cs_smsg);
                if (smsgBuckets[time].nLockCount > 0)
                {
                    if (fDebugSmsg)
                        LogPrintf("Bucket is locked %u, waiting for peer %u to send data.\n", smsgBuckets[time].nLockCount, smsgBuckets[time].nLockPeerId);
                    nLocked++;
                    continue;
                };

                // -- if this node has more than the peer node, peer node will pull from this
                //    if then peer node has more this node will pull fom peer
                if (smsgBuckets[time].setTokens.size() < ncontent
                    || (smsgBuckets[time].setTokens.size() == ncontent
                        && smsgBuckets[time].hash != hash)) // if same amount in buckets check hash
                {
                    if (fDebugSmsg)
                        LogPrintf("Requesting contents of bucket %d.\n", time);

                    uint32_t sz = vchDataOut.size();
                    vchDataOut.resize(sz + 8);
                    memcpy(&vchDataOut[sz], &time, 8);

                    nShowBuckets++;
                };
            } // LOCK(cs_smsg);
        };

        // TODO: should include hash?
        memcpy(&vchDataOut[0], &nShowBuckets, 4);
        if (vchDataOut.size() > 4)
        {
            pfrom->PushMessage("smsgShow", vchDataOut);
        } else
        if (nLocked < 1) // Don't report buckets as matched if any are locked
        {
            // -- peer has no buckets we want, don't send them again until something changes
            //    peer will still request buckets from this node if needed (< ncontent)
            vchDataOut.resize(8);
            memcpy(&vchDataOut[0], &now, 8);
            pfrom->PushMessage("smsgMatch", vchDataOut);
            if (fDebugSmsg)
                LogPrintf("Sending smsgMatch, %d.\n", now);
        };

    } else
    if (strCommand == "smsgShow")
    {
        std::vector<uint8_t> vchData;
        vRecv >> vchData;

        if (vchData.size() < 4)
            return false;

        uint32_t nBuckets;
        memcpy(&nBuckets, &vchData[0], 4);

        if (vchData.size() < 4 + nBuckets * 8)
            return false;

        if (fDebugSmsg)
            LogPrintf("smsgShow: peer wants to see content of %u buckets.\n", nBuckets);
        
        std::map<int64_t, SecMsgBucket>::iterator itb;
        std::set<SecMsgToken>::iterator it;

        std::vector<uint8_t> vchDataOut;
        int64_t time;
        uint8_t* pIn = &vchData[4];
        for (uint32_t i = 0; i < nBuckets; ++i, pIn += 8)
        {
            memcpy(&time, pIn, 8);
            
            {
                LOCK(cs_smsg);
                itb = smsgBuckets.find(time);
                if (itb == smsgBuckets.end())
                {
                    if (fDebugSmsg)
                        LogPrintf("Don't have bucket %d.\n", time);
                    continue;
                };

                std::set<SecMsgToken>& tokenSet = (*itb).second.setTokens;

                try { vchDataOut.resize(8 + 16 * tokenSet.size()); } catch (std::exception& e)
                {
                    LogPrintf("vchDataOut.resize %u threw: %s.\n", 8 + 16 * tokenSet.size(), e.what());
                    continue;
                };
                memcpy(&vchDataOut[0], &time, 8);

                uint8_t* p = &vchDataOut[8];
                for (it = tokenSet.begin(); it != tokenSet.end(); ++it)
                {
                    memcpy(p, &it->timestamp, 8);
                    memcpy(p+8, &it->sample, 8);

                    p += 16;
                };
            }
            pfrom->PushMessage("smsgHave", vchDataOut);
        };


    } else
    if (strCommand == "smsgHave")
    {
        // -- peer has these messages in bucket
        std::vector<uint8_t> vchData;
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
            if (fDebugSmsg)
                LogPrintf("Not interested in peer bucket %d, has expired.\n", time);
            return false;
        };
        if (time > now + SMSG_TIME_LEEWAY)
        {
            if (fDebugSmsg)
                LogPrintf("Not interested in peer bucket %d, in the future.\n", time);
            pfrom->Misbehaving(1);
            return false;
        };
        
        std::vector<uint8_t> vchDataOut;
        
        {
            LOCK(cs_smsg);
            if (smsgBuckets[time].nLockCount > 0)
            {
                if (fDebugSmsg)
                    LogPrintf("Bucket %d lock count %u, waiting for message data from peer %u.\n", time, smsgBuckets[time].nLockCount, smsgBuckets[time].nLockPeerId);
                return false;
            };

            if (fDebugSmsg)
                LogPrintf("Sifting through bucket %d.\n", time);
            
            vchDataOut.resize(8);
            memcpy(&vchDataOut[0], &vchData[0], 8);

            std::set<SecMsgToken>& tokenSet = smsgBuckets[time].setTokens;
            std::set<SecMsgToken>::iterator it;
            SecMsgToken token;
            uint8_t* p = &vchData[8];

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
                    };

                    memcpy(&vchDataOut[nd], p, 16);
                };

                p += 16;
            };
        }
        
        if (vchDataOut.size() > 8)
        {
            if (fDebugSmsg)
            {
                LogPrintf("Asking peer for %u messages.\n", (vchDataOut.size() - 8) / 16);
                LogPrintf("Locking bucket %u for peer %d.\n", time, pfrom->id);
            };
            {
                LOCK(cs_smsg);
                smsgBuckets[time].nLockCount   = 3; // lock this bucket for at most 3 * SMSG_THREAD_DELAY seconds, unset when peer sends smsgMsg
                smsgBuckets[time].nLockPeerId  = pfrom->id;
            }
            pfrom->PushMessage("smsgWant", vchDataOut);
        };
    } else
    if (strCommand == "smsgWant")
    {
        std::vector<uint8_t> vchData;
        vRecv >> vchData;

        if (vchData.size() < 8)
            return false;

        std::vector<uint8_t> vchOne;
        std::vector<uint8_t> vchBunch;

        vchBunch.resize(4+8); // nmessages + bucketTime

        int n = (vchData.size() - 8) / 16;

        int64_t time;
        uint32_t nBunch = 0;
        memcpy(&time, &vchData[0], 8);
        
        
        std::map<int64_t, SecMsgBucket>::iterator itb;
        
        {
            LOCK(cs_smsg);
            itb = smsgBuckets.find(time);
            if (itb == smsgBuckets.end())
            {
                if (fDebugSmsg)
                    LogPrintf("Don't have bucket %d.\n", time);
                return false;
            };

            std::set<SecMsgToken>& tokenSet = itb->second.setTokens;
            std::set<SecMsgToken>::iterator it;
            SecMsgToken token;
            uint8_t* p = &vchData[8];
            for (int i = 0; i < n; ++i)
            {
                memcpy(&token.timestamp, p, 8);
                memcpy(&token.sample, p+8, 8);

                it = tokenSet.find(token);
                if (it == tokenSet.end())
                {
                    if (fDebugSmsg)
                        LogPrintf("Don't have wanted message %d.\n", token.timestamp);
                } else
                {
                    //LogPrintf("Have message at %d.\n", it->offset); // DEBUG
                    token.offset = it->offset;
                    //LogPrintf("winb before SecureMsgRetrieve %d.\n", token.timestamp);

                    // -- place in vchOne so if SecureMsgRetrieve fails it won't corrupt vchBunch
                    if (SecureMsgRetrieve(token, vchOne) == 0)
                    {
                        nBunch++;
                        vchBunch.insert(vchBunch.end(), vchOne.begin(), vchOne.end()); // append
                    } else
                    {
                        LogPrintf("SecureMsgRetrieve failed %d.\n", token.timestamp);
                    };

                    if (nBunch >= 500
                        || vchBunch.size() >= 96000)
                    {
                        if (fDebugSmsg)
                            LogPrintf("Break bunch %u, %u.\n", nBunch, vchBunch.size());
                        break; // end here, peer will send more want messages if needed.
                    };
                };
                p += 16;
            };
        } // LOCK(cs_smsg);
        
        if (nBunch > 0)
        {
            if (fDebugSmsg)
                LogPrintf("Sending block of %u messages for bucket %d.\n", nBunch, time);

            memcpy(&vchBunch[0], &nBunch, 4);
            memcpy(&vchBunch[4], &time, 8);
            pfrom->PushMessage("smsgMsg", vchBunch);
        };
    } else
    if (strCommand == "smsgMsg")
    {
        std::vector<uint8_t> vchData;
        vRecv >> vchData;

        if (fDebugSmsg)
            LogPrintf("smsgMsg vchData.size() %u.\n", vchData.size());

        SecureMsgReceive(pfrom, vchData);
    } else
    if (strCommand == "smsgMatch")
    {
        std::vector<uint8_t> vchData;
        vRecv >> vchData;


        if (vchData.size() < 8)
        {
            LogPrintf("smsgMatch, not enough data %u.\n", vchData.size());
            pfrom->Misbehaving(1);
            return false;
        };

        int64_t time;
        memcpy(&time, &vchData[0], 8);

        int64_t now = GetTime();
        if (time > now + SMSG_TIME_LEEWAY)
        {
            LogPrintf("Warning: Peer buckets matched in the future: %d.\nEither this node or the peer node has the incorrect time set.\n", time);
            if (fDebugSmsg)
                LogPrintf("Peer match time set to now.\n");
            time = now;
        };
        
        {
            LOCK(pfrom->smsgData.cs_smsg_net);
            pfrom->smsgData.lastMatched = time;
        }
        if (fDebugSmsg)
            LogPrintf("Peer buckets matched at %d.\n", time);

    } else
    if (strCommand == "smsgPing")
    {
        // -- smsgPing is the initial message, send reply
        pfrom->PushMessage("smsgPong");
    } else
    if (strCommand == "smsgPong")
    {
        if (fDebugSmsg)
             LogPrintf("Peer replied, secure messaging enabled.\n");
        
        {
            LOCK(pfrom->smsgData.cs_smsg_net);
            pfrom->smsgData.fEnabled = true;
        }
        
    } else
    if (strCommand == "smsgDisabled")
    {
        // -- peer has disabled secure messaging.
        
        {
            LOCK(pfrom->smsgData.cs_smsg_net);
            pfrom->smsgData.fEnabled = false;
        }
        
        if (fDebugSmsg)
            LogPrintf("Peer %d has disabled secure messaging.\n", pfrom->id);

    } else
    if (strCommand == "smsgIgnore")
    {
        // -- peer is reporting that it will ignore this node until time.
        //    Ignore peer too
        std::vector<uint8_t> vchData;
        vRecv >> vchData;

        if (vchData.size() < 8)
        {
            LogPrintf("smsgIgnore, not enough data %u.\n", vchData.size());
            pfrom->Misbehaving(1);
            return false;
        };

        int64_t time;
        memcpy(&time, &vchData[0], 8);
        
        {
            LOCK(pfrom->smsgData.cs_smsg_net);
            pfrom->smsgData.ignoreUntil = time;
        }
        

        if (fDebugSmsg)
            LogPrintf("Peer %d is ignoring this node until %d, ignore peer too.\n", pfrom->id, time);
    } else
    {
        // Unknown message
    };

    return true;
};

bool SecureMsgSendData(CNode* pto, bool fSendTrickle)
{
    /*
        Called from ProcessMessage
        Runs in ThreadMessageHandler2
    */
    
    LOCK(pto->smsgData.cs_smsg_net);

    //LogPrintf("SecureMsgSendData() %s.\n", pto->addrName.c_str());
    
    int64_t now = GetTime();

    if (pto->smsgData.lastSeen == 0)
    {
        // -- first contact
        if (fDebugSmsg)
            LogPrintf("SecureMsgSendData() new node %s, peer id %u.\n", pto->addrName.c_str(), pto->id);
        // -- Send smsgPing once, do nothing until receive 1st smsgPong (then set fEnabled)
        pto->PushMessage("smsgPing");
        pto->smsgData.lastSeen = GetTime();
        return true;
    } else
    if (!pto->smsgData.fEnabled
        || now - pto->smsgData.lastSeen < SMSG_SEND_DELAY
        || now < pto->smsgData.ignoreUntil)
    {
        return true;
    };

    // -- When nWakeCounter == 0, resend bucket inventory.
    if (pto->smsgData.nWakeCounter < 1)
    {
        pto->smsgData.lastMatched = 0;
        pto->smsgData.nWakeCounter = 10 + GetRandInt(300);  // set to a random time between [10, 300] * SMSG_SEND_DELAY seconds

        if (fDebugSmsg)
            LogPrintf("SecureMsgSendData(): nWakeCounter expired, sending bucket inventory to %s.\n"
            "Now %d next wake counter %u\n", pto->addrName.c_str(), now, pto->smsgData.nWakeCounter);
    };
    pto->smsgData.nWakeCounter--;

    {
        LOCK(cs_smsg);
        std::map<int64_t, SecMsgBucket>::iterator it;

        uint32_t nBuckets = smsgBuckets.size();
        if (nBuckets > 0) // no need to send keep alive pkts, coin messages already do that
        {
            std::vector<uint8_t> vchData;
            // should reserve?
            vchData.reserve(4 + nBuckets*16); // timestamp + size + hash

            uint32_t nBucketsShown = 0;
            vchData.resize(4);

            uint8_t* p = &vchData[4];
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
                    LogPrintf("vchData.resize %u threw: %s.\n", vchData.size() + 16, e.what());
                    continue;
                };
                memcpy(p, &it->first, 8);
                memcpy(p+8, &nMessages, 4);
                memcpy(p+12, &hash, 4);

                p += 16;
                nBucketsShown++;
                //if (fDebug)
                //    LogPrintf("Sending bucket %d, size %d \n", it->first, it->second.size());
            };

            if (vchData.size() > 4)
            {
                memcpy(&vchData[0], &nBucketsShown, 4);
                if (fDebugSmsg)
                    LogPrintf("Sending %d bucket headers.\n", nBucketsShown);

                pto->PushMessage("smsgInv", vchData);
            };
        };
    }

    pto->smsgData.lastSeen = GetTime();

    return true;
};


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
        };
        return 4;
    };

    if (!addrpkdb.WritePK(hashKey, pubKey))
    {
        LogPrintf("Write pair failed.\n");
        return 1;
    };

    return 0;
};

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
};


static bool ScanBlock(CBlock& block, CTxDB& txdb, SecMsgDB& addrpkdb,
    uint32_t& nTransactions, uint32_t& nElements, uint32_t& nPubkeys, uint32_t& nDuplicates)
{
    AssertLockHeld(cs_smsgDB);
    
    valtype vch;
    opcodetype opcode;
    
    // -- only scan inputs of standard txns and coinstakes
    
    BOOST_FOREACH(CTransaction& tx, block.vtx)
    {
        // - harvest public keys from coinstake txns
        if (tx.IsCoinStake())
        {
            const CTxOut& txout = tx.vout[1];
            CScript::const_iterator pc = txout.scriptPubKey.begin();
            while (pc < txout.scriptPubKey.end())
            {
                if (!txout.scriptPubKey.GetOp(pc, opcode, vch))
                    break;
                
                if (vch.size() == 33) // pubkey
                {
                    CPubKey pubKey(vch);
                    
                    if (!pubKey.IsValid()
                        || !pubKey.IsCompressed())
                    {
                        LogPrintf("Public key is invalid %s.\n", HexStr(pubKey).c_str());
                        continue;
                    };
                    
                    CKeyID addrKey = pubKey.GetID();
                    switch (SecureMsgInsertAddress(addrKey, pubKey, addrpkdb))
                    {
                        case 0: nPubkeys++; break;      // added key
                        case 4: nDuplicates++; break;   // duplicate key
                    }
                    break;
                };
            };
            nElements++;
        } else
        if (tx.IsStandard())
        {
            for (uint32_t i = 0; i < tx.vin.size(); i++)
            {
                if (tx.nVersion == ANON_TXN_VERSION
                    && tx.vin[i].IsAnonInput())
                    continue; // skip anon inputs

                CScript *script = &tx.vin[i].scriptSig;
                CScript::const_iterator pc = script->begin();
                CScript::const_iterator pend = script->end();

                uint256 prevoutHash;
                CKey key;

                while (pc < pend)
                {
                    if (!script->GetOp(pc, opcode, vch))
                        break;
                    // -- opcode is the length of the following data, compressed public key is always 33
                    if (opcode == 33)
                    {
                        CPubKey pubKey(vch);
                        
                        if (!pubKey.IsValid()
                            || !pubKey.IsCompressed())
                        {
                            LogPrintf("Public key is invalid %s.\n", HexStr(pubKey).c_str());
                            continue;
                        };
                        
                        CKeyID addrKey = pubKey.GetID();
                        switch (SecureMsgInsertAddress(addrKey, pubKey, addrpkdb))
                        {
                            case 0: nPubkeys++; break;      // added key
                            case 4: nDuplicates++; break;   // duplicate key
                        }
                        break;
                    };

                    //LogPrintf("opcode %d, %s, value %s.\n", opcode, GetOpName(opcode), ValueString(vch).c_str());
                };
                nElements++;
            };
        };
        nTransactions++;

        if (nTransactions % 10000 == 0) // for ScanChainForPublicKeys
        {
            LogPrintf("Scanning transaction no. %u.\n", nTransactions);
        };
    };
    return true;
};


bool SecureMsgScanBlock(CBlock& block)
{
    /*
    scan block for public key addresses
    called from ProcessMessage() in main where strCommand == "block"
    */

    if (fDebugSmsg)
        LogPrintf("SecureMsgScanBlock().\n");

    uint32_t nTransactions  = 0;
    uint32_t nElements      = 0;
    uint32_t nPubkeys       = 0;
    uint32_t nDuplicates    = 0;

    {
        LOCK(cs_smsgDB);
        CTxDB txdb("r");

        SecMsgDB addrpkdb;
        if (!addrpkdb.Open("cw")
            || !addrpkdb.TxnBegin())
            return false;

        ScanBlock(block, txdb, addrpkdb,
            nTransactions, nElements, nPubkeys, nDuplicates);

        addrpkdb.TxnCommit();
    }

    if (fDebugSmsg)
        LogPrintf("Found %u transactions, %u elements, %u new public keys, %u duplicates.\n", nTransactions, nElements, nPubkeys, nDuplicates);

    return true;
};

bool ScanChainForPublicKeys(CBlockIndex* pindexStart)
{
    LogPrintf("Scanning block chain for public keys.\n");
    int64_t nStart = GetTimeMillis();

    if (fDebugSmsg)
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

        CTxDB txdb("r");

        SecMsgDB addrpkdb;
        if (!addrpkdb.Open("cw")
            || !addrpkdb.TxnBegin())
            return false;

        CBlockIndex* pindex = pindexStart;
        while (pindex)
        {
            nBlocks++;
            CBlock block;
            block.ReadFromDisk(pindex, true);

            ScanBlock(block, txdb, addrpkdb,
                nTransactions, nInputs, nPubkeys, nDuplicates);

            pindex = pindex->pnext;
        };

        addrpkdb.TxnCommit();
    };

    LogPrintf("Scanned %u blocks, %u transactions, %u inputs\n", nBlocks, nTransactions, nInputs);
    LogPrintf("Found %u public keys, %u duplicates.\n", nPubkeys, nDuplicates);
    LogPrintf("Took %d ms\n", GetTimeMillis() - nStart);

    return true;
};

bool SecureMsgScanBlockChain()
{
    TRY_LOCK(cs_main, lockMain);
    if (lockMain)
    {
        CBlockIndex *pindexScan = pindexGenesisBlock;
        if (pindexScan == NULL)
        {
            LogPrintf("Error: pindexGenesisBlock not set.\n");
            return false;
        };


        try { // -- in try to catch errors opening db,
            if (!ScanChainForPublicKeys(pindexScan))
                return false;
        } catch (std::exception& e)
        {
            LogPrintf("ScanChainForPublicKeys() threw: %s.\n", e.what());
            return false;
        };
    } else
    {
        LogPrintf("ScanChainForPublicKeys() Could not lock main.\n");
        return false;
    };

    return true;
};

bool SecureMsgScanBuckets()
{
    if (fDebugSmsg)
        LogPrintf("SecureMsgScanBuckets()\n");

    if (!fSecMsgEnabled
        || pwalletMain->IsLocked())
        return false;

    int64_t  mStart         = GetTimeMillis();
    int64_t  now            = GetTime();
    uint32_t nFiles         = 0;
    uint32_t nMessages      = 0;
    uint32_t nFoundMessages = 0;

    fs::path pathSmsgDir = GetDataDir() / "smsgStore";
    fs::directory_iterator itend;

    if (!fs::exists(pathSmsgDir)
        || !fs::is_directory(pathSmsgDir))
    {
        LogPrintf("Message store directory does not exist.\n");
        return 0; // not an error
    };

    SecureMessage smsg;
    std::vector<uint8_t> vchData;

    for (fs::directory_iterator itd(pathSmsgDir) ; itd != itend ; ++itd)
    {
        if (!fs::is_regular_file(itd->status()))
            continue;

        std::string fileType = (*itd).path().extension().string();

        if (fileType.compare(".dat") != 0)
            continue;

        std::string fileName = (*itd).path().filename().string();


        if (fDebugSmsg)
            LogPrintf("Processing file: %s.\n", fileName.c_str());

        nFiles++;

        // TODO files must be split if > 2GB
        // time_noFile.dat
        size_t sep = fileName.find_first_of("_");
        if (sep == std::string::npos)
            continue;

        std::string stime = fileName.substr(0, sep);

        int64_t fileTime = boost::lexical_cast<int64_t>(stime);

        if (fileTime < now - SMSG_RETENTION)
        {
            LogPrintf("Dropping file %s, expired.\n", fileName.c_str());
            try {
                fs::remove((*itd).path());
            } catch (const fs::filesystem_error& ex)
            {
                LogPrintf("Error removing bucket file %s, %s.\n", fileName.c_str(), ex.what());
            };
            continue;
        };

        if (boost::algorithm::ends_with(fileName, "_wl.dat"))
        {
            if (fDebugSmsg)
                LogPrintf("Skipping wallet locked file: %s.\n", fileName.c_str());
            continue;
        };

        {
            LOCK(cs_smsg);
            FILE *fp;
            errno = 0;
            if (!(fp = fopen((*itd).path().string().c_str(), "rb")))
            {
                LogPrintf("Error opening file: %s\n", strerror(errno));
                continue;
            };

            for (;;)
            {
                errno = 0;
                if (fread(&smsg.hash[0], sizeof(uint8_t), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN)
                {
                    if (errno != 0)
                    {
                        LogPrintf("fread header failed: %s\n", strerror(errno));
                    } else
                    {
                        //LogPrintf("End of file.\n");
                    };
                    break;
                };

                try { vchData.resize(smsg.nPayload); } catch (std::exception& e)
                {
                    LogPrintf("SecureMsgWalletUnlocked(): Could not resize vchData, %u, %s\n", smsg.nPayload, e.what());
                    fclose(fp);
                    return 1;
                };

                if (fread(&vchData[0], sizeof(uint8_t), smsg.nPayload, fp) != smsg.nPayload)
                {
                    LogPrintf("fread data failed: %s\n", strerror(errno));
                    break;
                };

                // -- don't report to gui,
                int rv = SecureMsgScanMessage(&smsg.hash[0], &vchData[0], smsg.nPayload, false);

                if (rv == 0)
                {
                    nFoundMessages++;
                } else
                if (rv != 0)
                {
                    // SecureMsgScanMessage failed
                };

                nMessages ++;
            };

            fclose(fp);

            // -- remove wl file when scanned
            try {
                fs::remove((*itd).path());
            } catch (const boost::filesystem::filesystem_error& ex)
            {
                LogPrintf("Error removing wl file %s - %s\n", fileName.c_str(), ex.what());
                return 1;
            };
        };
    };

    LogPrintf("Processed %u files, scanned %u messages, received %u messages.\n", nFiles, nMessages, nFoundMessages);
    LogPrintf("Took %d ms\n", GetTimeMillis() - mStart);

    return true;
}


int SecureMsgWalletUnlocked()
{
    /*
    When the wallet is unlocked, scan messages received while wallet was locked.
    */
    if (!fSecMsgEnabled)
        return 0;

    LogPrintf("SecureMsgWalletUnlocked()\n");

    if (pwalletMain->IsLocked())
    {
        LogPrintf("Error: Wallet is locked.\n");
        return 1;
    };

    int64_t  now            = GetTime();
    uint32_t nFiles         = 0;
    uint32_t nMessages      = 0;
    uint32_t nFoundMessages = 0;

    fs::path pathSmsgDir = GetDataDir() / "smsgStore";
    fs::directory_iterator itend;

    if (!fs::exists(pathSmsgDir)
        || !fs::is_directory(pathSmsgDir))
    {
        LogPrintf("Message store directory does not exist.\n");
        return 0; // not an error
    };

    SecureMessage smsg;
    std::vector<uint8_t> vchData;

    for (fs::directory_iterator itd(pathSmsgDir) ; itd != itend ; ++itd)
    {
        if (!fs::is_regular_file(itd->status()))
            continue;

        std::string fileName = (*itd).path().filename().string();

        if (!boost::algorithm::ends_with(fileName, "_wl.dat"))
            continue;

        if (fDebugSmsg)
            LogPrintf("Processing file: %s.\n", fileName.c_str());

        nFiles++;

        // TODO files must be split if > 2GB
        // time_noFile_wl.dat
        size_t sep = fileName.find_first_of("_");
        if (sep == std::string::npos)
            continue;

        std::string stime = fileName.substr(0, sep);

        int64_t fileTime = boost::lexical_cast<int64_t>(stime);

        if (fileTime < now - SMSG_RETENTION)
        {
            LogPrintf("Dropping wallet locked file %s, expired.\n", fileName.c_str());
            try {
                fs::remove((*itd).path());
            } catch (const boost::filesystem::filesystem_error& ex)
            {
                LogPrintf("Error removing wl file %s - %s\n", fileName.c_str(), ex.what());
                return 1;
            };
            continue;
        };

        {
            LOCK(cs_smsg);
            FILE *fp;
            errno = 0;
            if (!(fp = fopen((*itd).path().string().c_str(), "rb")))
            {
                LogPrintf("Error opening file: %s\n", strerror(errno));
                continue;
            };

            for (;;)
            {
                errno = 0;
                if (fread(&smsg.hash[0], sizeof(uint8_t), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN)
                {
                    if (errno != 0)
                    {
                        LogPrintf("fread header failed: %s\n", strerror(errno));
                    } else
                    {
                        //LogPrintf("End of file.\n");
                    };
                    break;
                };

                try { vchData.resize(smsg.nPayload); } catch (std::exception& e)
                {
                    LogPrintf("SecureMsgWalletUnlocked(): Could not resize vchData, %u, %s\n", smsg.nPayload, e.what());
                    fclose(fp);
                    return 1;
                };

                if (fread(&vchData[0], sizeof(uint8_t), smsg.nPayload, fp) != smsg.nPayload)
                {
                    LogPrintf("fread data failed: %s\n", strerror(errno));
                    break;
                };

                // -- don't report to gui,
                int rv = SecureMsgScanMessage(&smsg.hash[0], &vchData[0], smsg.nPayload, false);

                if (rv == 0)
                {
                    nFoundMessages++;
                } else
                if (rv != 0)
                {
                    // SecureMsgScanMessage failed
                };

                nMessages ++;
            };

            fclose(fp);

            // -- remove wl file when scanned
            try {
                fs::remove((*itd).path());
            } catch (const boost::filesystem::filesystem_error& ex)
            {
                LogPrintf("Error removing wl file %s - %s\n", fileName.c_str(), ex.what());
                return 1;
            };
        };
    };

    LogPrintf("Processed %u files, scanned %u messages, received %u messages.\n", nFiles, nMessages, nFoundMessages);
    
    // -- notify gui
    NotifySecMsgWalletUnlocked();
    return 0;
};

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
                };
                break;
            default:
                break;
        }

    } // cs_smsg


    return 0;
};

int SecureMsgScanMessage(uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload, bool reportToGui)
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

    if (fDebugSmsg)
        LogPrintf("SecureMsgScanMessage()\n");

    if (pwalletMain->IsLocked())
    {
        if (fDebugSmsg)
            LogPrintf("ScanMessage: Wallet is locked, storing message to scan later.\n");

        int rv;
        if ((rv = SecureMsgStoreUnscanned(pHeader, pPayload, nPayload)) != 0)
            return 1;

        return 3;
    };

    std::string addressTo;
    MessageData msg; // placeholder
    bool fOwnMessage = false;

    for (std::vector<SecMsgAddress>::iterator it = smsgAddresses.begin(); it != smsgAddresses.end(); ++it)
    {
        if (!it->fReceiveEnabled)
            continue;

        CBitcoinAddress coinAddress(it->sAddress);
        addressTo = coinAddress.ToString();

        if (!it->fReceiveAnon)
        {
            // -- have to do full decrypt to see address from
            if (SecureMsgDecrypt(false, addressTo, pHeader, pPayload, nPayload, msg) == 0)
            {
                if (fDebugSmsg)
                    LogPrintf("Decrypted message with %s.\n", addressTo.c_str());

                if (msg.sFromAddress.compare("anon") != 0)
                    fOwnMessage = true;
                break;
            };
        } else
        {

            if (SecureMsgDecrypt(true, addressTo, pHeader, pPayload, nPayload, msg) == 0)
            {
                if (fDebugSmsg)
                    LogPrintf("Decrypted message with %s.\n", addressTo.c_str());

                fOwnMessage = true;
                break;
            };
        }
    };

    if (fOwnMessage)
    {
        // -- save to inbox
        SecureMessage* psmsg = (SecureMessage*) pHeader;
        std::string sPrefix("im");
        uint8_t chKey[18];
        memcpy(&chKey[0],  sPrefix.data(),    2);
        memcpy(&chKey[2],  &psmsg->timestamp, 8);
        memcpy(&chKey[10], pPayload,          8);

        SecMsgStored smsgInbox;
        smsgInbox.timeReceived  = GetTime();
        smsgInbox.status        = (SMSG_MASK_UNREAD) & 0xFF;
        smsgInbox.sAddrTo       = addressTo;

        // -- data may not be contiguous
        try {
            smsgInbox.vchMessage.resize(SMSG_HDR_LEN + nPayload);
        } catch (std::exception& e) {
            LogPrintf("SecureMsgScanMessage(): Could not resize vchData, %u, %s\n", SMSG_HDR_LEN + nPayload, e.what());
            return 1;
        };
        memcpy(&smsgInbox.vchMessage[0], pHeader, SMSG_HDR_LEN);
        memcpy(&smsgInbox.vchMessage[SMSG_HDR_LEN], pPayload, nPayload);

        {
            LOCK(cs_smsgDB);
            SecMsgDB dbInbox;

            if (dbInbox.Open("cw"))
            {
                if (dbInbox.ExistsSmesg(chKey))
                {
                    if (fDebugSmsg)
                        LogPrintf("Message already exists in inbox db.\n");
                } else
                {
                    dbInbox.WriteSmesg(chKey, smsgInbox);

                    if (reportToGui)
                        NotifySecMsgInboxChanged(smsgInbox);
                    LogPrintf("SecureMsg saved to inbox, received with %s.\n", addressTo.c_str());
                };
            };
        }
    };

    return 0;
};

int SecureMsgGetLocalKey(CKeyID& ckid, CPubKey& cpkOut)
{
    if (fDebugSmsg)
        LogPrintf("SecureMsgGetLocalKey()\n");

    CKey key;
    if (!pwalletMain->GetKey(ckid, key))
        return 4;

    cpkOut = key.GetPubKey(true);

    if (!cpkOut.IsValid()
        || !cpkOut.IsCompressed())
    {
        LogPrintf("Public key is invalid %s.\n", HexStr(cpkOut).c_str());
        return 1;
    };
    
    return 0;
};

int SecureMsgGetLocalPublicKey(std::string& strAddress, std::string& strPublicKey)
{
    /* returns
        0 success,
        1 error
        2 invalid address
        3 address does not refer to a key
        4 address not in wallet
    */

    CBitcoinAddress address;
    if (!address.SetString(strAddress))
        return 2; // Invalid coin address

    CKeyID keyID;
    if (!address.GetKeyID(keyID))
        return 3;

    int rv;
    CPubKey pubKey;
    if ((rv = SecureMsgGetLocalKey(keyID, pubKey)) != 0)
        return rv;

    strPublicKey = EncodeBase58(pubKey.begin(), pubKey.end());

    return 0;
};

int SecureMsgGetStoredKey(CKeyID& ckid, CPubKey& cpkOut)
{
    /* returns
        0 success,
        1 error
        2 public key not in database
    */
    if (fDebugSmsg)
        LogPrintf("SecureMsgGetStoredKey().\n");

    {
        LOCK(cs_smsgDB);
        SecMsgDB addrpkdb;

        if (!addrpkdb.Open("r"))
            return 1;

        if (!addrpkdb.ReadPK(ckid, cpkOut))
        {
            //LogPrintf("addrpkdb.Read failed: %s.\n", coinAddress.ToString().c_str());
            return 2;
        };
    }

    return 0;
};

int SecureMsgAddAddress(std::string& address, std::string& publicKey)
{
    static const char *fn = "SecureMsgAddAddress()";
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

    CBitcoinAddress coinAddress(address);

    if (!coinAddress.IsValid())
    {
        LogPrintf("%s - Address is not valid: %s.\n", fn, address.c_str());
        return 5;
    };

    CKeyID hashKey;

    if (!coinAddress.GetKeyID(hashKey))
    {
        LogPrintf("%s - coinAddress.GetKeyID failed: %s.\n", fn, coinAddress.ToString().c_str());
        return 5;
    };

    std::vector<uint8_t> vchTest;
    DecodeBase58(publicKey, vchTest);
    CPubKey pubKey(vchTest);

    // -- check that public key matches address hash
    CPubKey pubKeyT(pubKey);
    if (!pubKeyT.IsValid())
    {
        LogPrintf("%s - Invalid PubKey.\n", fn);
        return 2;
    };
    
    CKeyID keyIDT = pubKeyT.GetID();
    CBitcoinAddress addressT(keyIDT);

    if (addressT.ToString().compare(address) != 0)
    {
        LogPrintf("%s - Public key does not hash to address, addressT %s.\n", fn, addressT.ToString().c_str());
        return 3;
    };

    return SecureMsgInsertAddress(hashKey, pubKey);
};

int SecureMsgRetrieve(SecMsgToken &token, std::vector<uint8_t>& vchData)
{
    if (fDebugSmsg)
        LogPrintf("SecureMsgRetrieve() %d.\n", token.timestamp);

    // -- has cs_smsg lock from SecureMsgReceiveData

    fs::path pathSmsgDir = GetDataDir() / "smsgStore";

    //LogPrintf("token.offset %d.\n", token.offset); // DEBUG
    int64_t bucket = token.timestamp - (token.timestamp % SMSG_BUCKET_LEN);
    std::string fileName = boost::lexical_cast<std::string>(bucket) + "_01.dat";
    fs::path fullpath = pathSmsgDir / fileName;

    //LogPrintf("bucket %d.\n", bucket);
    //LogPrintf("bucket d %d.\n", bucket);
    //LogPrintf("fileName %s.\n", fileName.c_str());

    FILE *fp;
    errno = 0;
    if (!(fp = fopen(fullpath.string().c_str(), "rb")))
    {
        LogPrintf("Error opening file: %s\nPath %s\n", strerror(errno), fullpath.string().c_str());
        return 1;
    };

    errno = 0;
    if (fseek(fp, token.offset, SEEK_SET) != 0)
    {
        LogPrintf("fseek, strerror: %s.\n", strerror(errno));
        fclose(fp);
        return 1;
    };

    SecureMessage smsg;
    errno = 0;
    if (fread(&smsg.hash[0], sizeof(uint8_t), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN)
    {
        LogPrintf("fread header failed: %s\n", strerror(errno));
        fclose(fp);
        return 1;
    };

    try {
        vchData.resize(SMSG_HDR_LEN + smsg.nPayload);
    } catch (std::exception& e) {
        LogPrintf("SecureMsgRetrieve(): Could not resize vchData, %u, %s\n", SMSG_HDR_LEN + smsg.nPayload, e.what());
        return 1;
    };

    memcpy(&vchData[0], &smsg.hash[0], SMSG_HDR_LEN);
    errno = 0;
    if (fread(&vchData[SMSG_HDR_LEN], sizeof(uint8_t), smsg.nPayload, fp) != smsg.nPayload)
    {
        LogPrintf("fread data failed: %s. Wanted %u bytes.\n", strerror(errno), smsg.nPayload);
        fclose(fp);
        return 1;
    };


    fclose(fp);

    return 0;
};

int SecureMsgReceive(CNode* pfrom, std::vector<uint8_t>& vchData)
{
    if (fDebugSmsg)
        LogPrintf("SecureMsgReceive().\n");

    if (vchData.size() < 12) // nBunch4 + timestamp8
    {
        LogPrintf("Error: not enough data.\n");
        return 1;
    };

    uint32_t nBunch;
    int64_t bktTime;

    memcpy(&nBunch, &vchData[0], 4);
    memcpy(&bktTime, &vchData[4], 8);


    // -- check bktTime ()
    //    bucket may not exist yet - will be created when messages are added
    int64_t now = GetTime();
    if (bktTime > now + SMSG_TIME_LEEWAY)
    {
        if (fDebugSmsg)
            LogPrintf("bktTime > now.\n");
        // misbehave?
        return 1;
    } else
    if (bktTime < now - SMSG_RETENTION)
    {
        if (fDebugSmsg)
            LogPrintf("bktTime < now - SMSG_RETENTION.\n");
        // misbehave?
        return 1;
    };

    std::map<int64_t, SecMsgBucket>::iterator itb;

    if (nBunch == 0 || nBunch > 500)
    {
        LogPrintf("Error: Invalid no. messages received in bunch %u, for bucket %d.\n", nBunch, bktTime);
        pfrom->Misbehaving(1);
        
        {
            LOCK(cs_smsg);
            // -- release lock on bucket if it exists
            itb = smsgBuckets.find(bktTime);
            if (itb != smsgBuckets.end())
                itb->second.nLockCount = 0;
        } // LOCK(cs_smsg);
        return 1;
    };

    uint32_t n = 12;

    for (uint32_t i = 0; i < nBunch; ++i)
    {
        if (vchData.size() - n < SMSG_HDR_LEN)
        {
            LogPrintf("Error: not enough data sent, n = %u.\n", n);
            break;
        };

        SecureMessage* psmsg = (SecureMessage*) &vchData[n];

        int rv;
        if ((rv = SecureMsgValidate(&vchData[n], &vchData[n + SMSG_HDR_LEN], psmsg->nPayload)) != 0)
        {
            // message dropped
            if (rv == 2) // invalid proof of work
            {
                pfrom->Misbehaving(10);
            } else
            {
                pfrom->Misbehaving(1);
            };
            continue;
        };
        
        {
            LOCK(cs_smsg);
            // -- store message, but don't hash bucket
            if (SecureMsgStore(&vchData[n], &vchData[n + SMSG_HDR_LEN], psmsg->nPayload, false) != 0)
            {
                // message dropped
                break; // continue?
            };

            if (SecureMsgScanMessage(&vchData[n], &vchData[n + SMSG_HDR_LEN], psmsg->nPayload, true) != 0)
            {
                // message recipient is not this node (or failed)
            };
        } // LOCK(cs_smsg);
        
        n += SMSG_HDR_LEN + psmsg->nPayload;
    };
    
    {
        LOCK(cs_smsg);
        // -- if messages have been added, bucket must exist now
        itb = smsgBuckets.find(bktTime);
        if (itb == smsgBuckets.end())
        {
            if (fDebugSmsg)
                LogPrintf("Don't have bucket %d.\n", bktTime);
            return 1;
        };

        itb->second.nLockCount  = 0; // this node has received data from peer, release lock
        itb->second.nLockPeerId = 0;
        itb->second.hashBucket();
    } // LOCK(cs_smsg);
    return 0;
};

int SecureMsgStoreUnscanned(uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload)
{
    /*
    When the wallet is locked a copy of each received message is stored to be scanned later if wallet is unlocked
    */

    if (fDebugSmsg)
        LogPrintf("SecureMsgStoreUnscanned()\n");

    if (!pHeader
        || !pPayload)
    {
        LogPrintf("Error: null pointer to header or payload.\n");
        return 1;
    };

    SecureMessage* psmsg = (SecureMessage*) pHeader;

    fs::path pathSmsgDir;
    try {
        pathSmsgDir = GetDataDir() / "smsgStore";
        fs::create_directory(pathSmsgDir);
    } catch (const boost::filesystem::filesystem_error& ex)
    {
        LogPrintf("Error: Failed to create directory %s - %s\n", pathSmsgDir.string().c_str(), ex.what());
        return 1;
    };

    int64_t now = GetTime();
    if (psmsg->timestamp > now + SMSG_TIME_LEEWAY)
    {
        LogPrintf("Message > now.\n");
        return 1;
    } else
    if (psmsg->timestamp < now - SMSG_RETENTION)
    {
        LogPrintf("Message < SMSG_RETENTION.\n");
        return 1;
    };

    int64_t bucket = psmsg->timestamp - (psmsg->timestamp % SMSG_BUCKET_LEN);

    std::string fileName = boost::lexical_cast<std::string>(bucket) + "_01_wl.dat";
    fs::path fullpath = pathSmsgDir / fileName;

    FILE *fp;
    errno = 0;
    if (!(fp = fopen(fullpath.string().c_str(), "ab")))
    {
        LogPrintf("Error opening file: %s\n", strerror(errno));
        return 1;
    };

    if (fwrite(pHeader, sizeof(uint8_t), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN
        || fwrite(pPayload, sizeof(uint8_t), nPayload, fp) != nPayload)
    {
        LogPrintf("fwrite failed: %s\n", strerror(errno));
        fclose(fp);
        return 1;
    };

    fclose(fp);

    return 0;
};


int SecureMsgStore(uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload, bool fUpdateBucket)
{
    if (fDebugSmsg)
        LogPrintf("SecureMsgStore()\n");
    
    AssertLockHeld(cs_smsg);
    
    if (!pHeader
        || !pPayload)
    {
        LogPrintf("Error: null pointer to header or payload.\n");
        return 1;
    };

    SecureMessage* psmsg = (SecureMessage*) pHeader;


    long int ofs;
    fs::path pathSmsgDir;
    try {
        pathSmsgDir = GetDataDir() / "smsgStore";
        fs::create_directory(pathSmsgDir);
    } catch (const boost::filesystem::filesystem_error& ex)
    {
        LogPrintf("Error: Failed to create directory %s - %s\n", pathSmsgDir.string().c_str(), ex.what());
        return 1;
    };

    int64_t now = GetTime();
    if (psmsg->timestamp > now + SMSG_TIME_LEEWAY)
    {
        LogPrintf("Message > now.\n");
        return 1;
    } else
    if (psmsg->timestamp < now - SMSG_RETENTION)
    {
        LogPrintf("Message < SMSG_RETENTION.\n");
        return 1;
    };

    int64_t bucket = psmsg->timestamp - (psmsg->timestamp % SMSG_BUCKET_LEN);

    SecMsgToken token(psmsg->timestamp, pPayload, nPayload, 0);

    std::set<SecMsgToken>& tokenSet = smsgBuckets[bucket].setTokens;
    std::set<SecMsgToken>::iterator it;
    it = tokenSet.find(token);
    if (it != tokenSet.end())
    {
        LogPrintf("Already have message.\n");
        if (fDebugSmsg)
        {
            LogPrintf("nPayload: %u\n", nPayload);
            LogPrintf("bucket: %d\n", bucket);

            LogPrintf("message ts: %d", token.timestamp);
            std::vector<uint8_t> vchShow;
            vchShow.resize(8);
            memcpy(&vchShow[0], token.sample, 8);
            LogPrintf(" sample %s\n", ValueString(vchShow).c_str());
            /*
            LogPrintf("\nmessages in bucket:\n");
            for (it = tokenSet.begin(); it != tokenSet.end(); ++it)
            {
                LogPrintf("message ts: %d", (*it).timestamp);
                vchShow.resize(8);
                memcpy(&vchShow[0], (*it).sample, 8);
                LogPrintf(" sample %s\n", ValueString(vchShow).c_str());
            };
            */
        };
        return 1;
    };

    std::string fileName = boost::lexical_cast<std::string>(bucket) + "_01.dat";
    fs::path fullpath = pathSmsgDir / fileName;

    FILE *fp;
    errno = 0;
    if (!(fp = fopen(fullpath.string().c_str(), "ab")))
    {
        LogPrintf("Error opening file: %s\n", strerror(errno));
        return 1;
    };

    // -- on windows ftell will always return 0 after fopen(ab), call fseek to set.
    errno = 0;
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        LogPrintf("Error fseek failed: %s\n", strerror(errno));
        return 1;
    };


    ofs = ftell(fp);

    if (fwrite(pHeader, sizeof(uint8_t), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN
        || fwrite(pPayload, sizeof(uint8_t), nPayload, fp) != nPayload)
    {
        LogPrintf("fwrite failed: %s\n", strerror(errno));
        fclose(fp);
        return 1;
    };

    fclose(fp);

    token.offset = ofs;

    //LogPrintf("token.offset: %d\n", token.offset); // DEBUG
    tokenSet.insert(token);

    if (fUpdateBucket)
        smsgBuckets[bucket].hashBucket();

    if (fDebugSmsg)
        LogPrintf("SecureMsg added to bucket %d.\n", bucket);
    return 0;
};

int SecureMsgStore(SecureMessage& smsg, bool fUpdateBucket)
{
    return SecureMsgStore(&smsg.hash[0], smsg.pPayload, smsg.nPayload, fUpdateBucket);
};

int SecureMsgValidate(uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload)
{
    /*
    returns
        0 success
        1 error
        2 invalid hash
        3 checksum mismatch
        4 invalid version
        5 payload is too large
    */
    SecureMessage* psmsg = (SecureMessage*) pHeader;

    if (psmsg->version[0] != 1)
        return 4;

    if (nPayload > SMSG_MAX_MSG_WORST)
        return 5;

    uint8_t civ[32];
    uint8_t sha256Hash[32];
    int rv = 2; // invalid

    uint32_t nonse;
    memcpy(&nonse, &psmsg->nonse[0], 4);

    if (fDebugSmsg)
        LogPrintf("SecureMsgValidate() nonse %u.\n", nonse);

    for (int i = 0; i < 32; i+=4)
        memcpy(civ+i, &nonse, 4);

    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);

    uint32_t nBytes;
    if (!HMAC_Init_ex(&ctx, &civ[0], 32, EVP_sha256(), NULL)
        || !HMAC_Update(&ctx, (uint8_t*) pHeader+4, SMSG_HDR_LEN-4)
        || !HMAC_Update(&ctx, (uint8_t*) pPayload, nPayload)
        || !HMAC_Update(&ctx, pPayload, nPayload)
        || !HMAC_Final(&ctx, sha256Hash, &nBytes)
        || nBytes != 32)
    {
        if (fDebugSmsg)
            LogPrintf("HMAC error.\n");
        rv = 1; // error
    } else
    {
        if (sha256Hash[31] == 0
            && sha256Hash[30] == 0
            && (~(sha256Hash[29]) & ((1<<0) || (1<<1) || (1<<2)) ))
        {
            if (fDebugSmsg)
                LogPrintf("Hash Valid.\n");
            rv = 0; // smsg is valid
        };

        if (memcmp(psmsg->hash, sha256Hash, 4) != 0)
        {
             if (fDebugSmsg)
                LogPrintf("Checksum mismatch.\n");
            rv = 3; // checksum mismatch
        }
    }
    HMAC_CTX_cleanup(&ctx);

    return rv;
};

int SecureMsgSetHash(uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload)
{
    /*  proof of work and checksum

        May run in a thread, if shutdown detected, return.

        returns:
            0 success
            1 error
            2 stopped due to node shutdown

    */

    SecureMessage* psmsg = (SecureMessage*) pHeader;

    int64_t nStart = GetTimeMillis();
    uint8_t civ[32];
    uint8_t sha256Hash[32];

    bool found = false;
    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);

    uint32_t nonse = 0;

    //CBigNum bnTarget(2);
    //bnTarget = bnTarget.pow(256 - 40);

    // -- break for HMAC_CTX_cleanup
    for (;;)
    {
        if (!fSecMsgEnabled)
           break;

        //psmsg->timestamp = GetTime();
        //memcpy(&psmsg->timestamp, &now, 8);
        memcpy(&psmsg->nonse[0], &nonse, 4);

        for (int i = 0; i < 32; i+=4)
            memcpy(civ+i, &nonse, 4);

        uint32_t nBytes;
        if (!HMAC_Init_ex(&ctx, &civ[0], 32, EVP_sha256(), NULL)
            || !HMAC_Update(&ctx, (uint8_t*) pHeader+4, SMSG_HDR_LEN-4)
            || !HMAC_Update(&ctx, (uint8_t*) pPayload, nPayload)
            || !HMAC_Update(&ctx, pPayload, nPayload)
            || !HMAC_Final(&ctx, sha256Hash, &nBytes)
            //|| !HMAC_Final(&ctx, &vchHash[0], &nBytes)
            || nBytes != 32)
            break;

        /*
        if (CBigNum(vchHash) <= bnTarget)
        {
            found = true;
            if (fDebugSmsg)
                LogPrintf("Match %u\n", nonse);
            break;
        };
        */

        if (sha256Hash[31] == 0
            && sha256Hash[30] == 0
            && (~(sha256Hash[29]) & ((1<<0) || (1<<1) || (1<<2)) ))
        //    && sha256Hash[29] == 0)
        {
            found = true;
            //if (fDebugSmsg)
            //    LogPrintf("Match %u\n", nonse);
            break;
        }

        //if (nonse >= UINT32_MAX)
        if (nonse >= 4294967295U)
        {
            if (fDebugSmsg)
                LogPrintf("No match %u\n", nonse);
            break;
            //return 1;
        }
        nonse++;
    };

    HMAC_CTX_cleanup(&ctx);

    if (!fSecMsgEnabled)
    {
        if (fDebugSmsg)
            LogPrintf("SecureMsgSetHash() stopped, shutdown detected.\n");
        return 2;
    };

    if (!found)
    {
        if (fDebugSmsg)
            LogPrintf("SecureMsgSetHash() failed, took %d ms, nonse %u\n", GetTimeMillis() - nStart, nonse);
        return 1;
    };

    memcpy(psmsg->hash, sha256Hash, 4);
    //memcpy(psmsg->hash, &vchHash[0], 4);

    if (fDebugSmsg)
        LogPrintf("SecureMsgSetHash() took %d ms, nonse %u\n", GetTimeMillis() - nStart, nonse);

    return 0;
};

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

    if (fDebugSmsg)
        LogPrintf("SecureMsgEncrypt(%s, %s, ...)\n", addressFrom.c_str(), addressTo.c_str());


    if (message.size() > SMSG_MAX_MSG_BYTES)
    {
        LogPrintf("Message is too long, %u.\n", message.size());
        return 2;
    };

    smsg.version[0] = 1;
    smsg.version[1] = 1;
    smsg.timestamp = GetTime();


    bool fSendAnonymous;
    CBitcoinAddress coinAddrFrom;
    CKeyID ckidFrom;
    CKey keyFrom;

    if (addressFrom.compare("anon") == 0)
    {
        fSendAnonymous = true;

    } else
    {
        fSendAnonymous = false;

        if (!coinAddrFrom.SetString(addressFrom))
        {
            LogPrintf("addressFrom is not valid.\n");
            return 3;
        };

        if (!coinAddrFrom.GetKeyID(ckidFrom))
        {
            LogPrintf("coinAddrFrom.GetKeyID failed: %s.\n", coinAddrFrom.ToString().c_str());
            return 3;
        };
    };


    CBitcoinAddress coinAddrDest;
    CKeyID ckidDest;

    if (!coinAddrDest.SetString(addressTo))
    {
        LogPrintf("addressTo is not valid.\n");
        return 4;
    };

    if (!coinAddrDest.GetKeyID(ckidDest))
    {
        LogPrintf("coinAddrDest.GetKeyID failed: %s.\n", coinAddrDest.ToString().c_str());
        return 4;
    };

    // -- public key K is the destination address
    CPubKey cpkDestK;
    if (SecureMsgGetStoredKey(ckidDest, cpkDestK) != 0
        && SecureMsgGetLocalKey(ckidDest, cpkDestK) != 0) // maybe it's a local key (outbox?)
    {
        LogPrintf("Could not get public key for destination address.\n");
        return 5;
    };


    // -- Generate 16 random bytes as IV.
    RandAddSeedPerfmon();
    RAND_bytes(&smsg.iv[0], 16);


    // -- Generate a new random EC key pair with private key called r and public key called R.
    CKey keyR;
    keyR.MakeNewKey(true); // make compressed key
    
    CECKey ecKeyR;
    ecKeyR.SetSecretBytes(keyR.begin());
    
    // -- Do an EC point multiply with public key K and private key r. This gives you public key P.
    CECKey ecKeyK;
    if (!ecKeyK.SetPubKey(cpkDestK))
    {
        LogPrintf("Could not set pubkey for K: %s.\n", HexStr(cpkDestK).c_str());
        return 4; // address to is invalid
    };

    std::vector<uint8_t> vchP;
    vchP.resize(32);
    EC_KEY* pkeyr = ecKeyR.GetECKey();
    EC_KEY* pkeyK = ecKeyK.GetECKey();

    // always seems to be 32, worth checking?
    //int field_size = EC_GROUP_get_degree(EC_KEY_get0_group(pkeyr));
    //int secret_len = (field_size+7)/8;
    //LogPrintf("secret_len %d.\n", secret_len);

    // -- ECDH_compute_key returns the same P if fed compressed or uncompressed public keys
    ECDH_set_method(pkeyr, ECDH_OpenSSL());
    int lenP = ECDH_compute_key(&vchP[0], 32, EC_KEY_get0_public_key(pkeyK), pkeyr, NULL);

    if (lenP != 32)
    {
        LogPrintf("ECDH_compute_key failed, lenP: %d.\n", lenP);
        return 6;
    };

    CPubKey cpkR = keyR.GetPubKey();
    if (!cpkR.IsValid()
        || !cpkR.IsCompressed())
    {
        LogPrintf("Could not get public key for key R.\n");
        return 1;
    };

    memcpy(smsg.cpkR, cpkR.begin(), 33);
    

    // -- Use public key P and calculate the SHA512 hash H.
    //    The first 32 bytes of H are called key_e and the last 32 bytes are called key_m.
    std::vector<uint8_t> vchHashed;
    vchHashed.resize(64); // 512
    SHA512(&vchP[0], vchP.size(), (uint8_t*)&vchHashed[0]);
    std::vector<uint8_t> key_e(&vchHashed[0], &vchHashed[0]+32);
    std::vector<uint8_t> key_m(&vchHashed[32], &vchHashed[32]+32);


    std::vector<uint8_t> vchPayload;
    std::vector<uint8_t> vchCompressed;
    uint8_t* pMsgData;
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
        };

        int lenComp = LZ4_compress((char*)message.c_str(), (char*)&vchCompressed[0], lenMsg);
        if (lenComp < 1)
        {
            LogPrintf("Could not compress message data.\n");
            return 9;
        };

        pMsgData = &vchCompressed[0];
        lenMsgData = lenComp;

    } else
    {
        // -- no compression
        pMsgData = (uint8_t*)message.c_str();
        lenMsgData = lenMsg;
    };

    if (fSendAnonymous)
    {
        try {
            vchPayload.resize(9 + lenMsgData);
        } catch (std::exception& e) {
            LogPrintf("vchPayload.resize %u threw: %s.\n", 9 + lenMsgData, e.what());
            return 8;
        };

        memcpy(&vchPayload[9], pMsgData, lenMsgData);

        vchPayload[0] = 250; // id as anonymous message
        // -- next 4 bytes are unused - there to ensure encrypted payload always > 8 bytes
        memcpy(&vchPayload[5], &lenMsg, 4); // length of uncompressed plain text
    } else
    {
        try {
            vchPayload.resize(SMSG_PL_HDR_LEN + lenMsgData);
        } catch (std::exception& e) {
            LogPrintf("vchPayload.resize %u threw: %s.\n", SMSG_PL_HDR_LEN + lenMsgData, e.what());
            return 8;
        };
        
        memcpy(&vchPayload[SMSG_PL_HDR_LEN], pMsgData, lenMsgData);
        // -- compact signature proves ownership of from address and allows the public key to be recovered, recipient can always reply.
        if (!pwalletMain->GetKey(ckidFrom, keyFrom))
        {
            LogPrintf("Could not get private key for addressFrom.\n");
            return 7;
        };

        // -- sign the plaintext
        std::vector<uint8_t> vchSignature;
        vchSignature.resize(65);
        keyFrom.SignCompact(Hash(message.begin(), message.end()), vchSignature);

        // -- Save some bytes by sending address raw
        vchPayload[0] = (static_cast<CBitcoinAddress_B*>(&coinAddrFrom))->getVersion(); // vchPayload[0] = coinAddrDest.nVersion;
        memcpy(&vchPayload[1], (static_cast<CKeyID_B*>(&ckidFrom))->GetPPN(), 20); // memcpy(&vchPayload[1], ckidDest.pn, 20);

        memcpy(&vchPayload[1+20], &vchSignature[0], vchSignature.size());
        memcpy(&vchPayload[1+20+65], &lenMsg, 4); // length of uncompressed plain text
    };


    SecMsgCrypter crypter;
    crypter.SetKey(key_e, smsg.iv);
    std::vector<uint8_t> vchCiphertext;

    if (!crypter.Encrypt(&vchPayload[0], vchPayload.size(), vchCiphertext))
    {
        LogPrintf("crypter.Encrypt failed.\n");
        return 11;
    };

    try { smsg.pPayload = new uint8_t[vchCiphertext.size()]; } catch (std::exception& e)
    {
        LogPrintf("Could not allocate pPayload, exception: %s.\n", e.what());
        return 8;
    };

    memcpy(smsg.pPayload, &vchCiphertext[0], vchCiphertext.size());
    smsg.nPayload = vchCiphertext.size();


    // -- Calculate a 32 byte MAC with HMACSHA256, using key_m as salt
    //    Message authentication code, (hash of timestamp + destination + payload)
    bool fHmacOk = true;
    uint32_t nBytes = 32;
    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);

    if (!HMAC_Init_ex(&ctx, &key_m[0], 32, EVP_sha256(), NULL)
        || !HMAC_Update(&ctx, (uint8_t*) &smsg.timestamp, sizeof(smsg.timestamp))
        || !HMAC_Update(&ctx, &vchCiphertext[0], vchCiphertext.size())
        || !HMAC_Final(&ctx, smsg.mac, &nBytes)
        || nBytes != 32)
        fHmacOk = false;

    HMAC_CTX_cleanup(&ctx);

    if (!fHmacOk)
    {
        LogPrintf("Could not generate MAC.\n");
        return 10;
    };


    return 0;
};

int SecureMsgSend(std::string& addressFrom, std::string& addressTo, std::string& message, std::string& sError)
{
    /* Encrypt secure message, and place it on the network
        Make a copy of the message to sender's first address and place in send queue db
        proof of work thread will pick up messages from  send queue db

    */

    if (fDebugSmsg)
        LogPrintf("SecureMsgSend(%s, %s, ...)\n", addressFrom.c_str(), addressTo.c_str());

    if (pwalletMain->IsLocked())
    {
        sError = "Wallet is locked, wallet must be unlocked to send and recieve messages.";
        LogPrintf("Wallet is locked, wallet must be unlocked to send and recieve messages.\n");
        return 1;
    };

    if (message.size() > SMSG_MAX_MSG_BYTES)
    {
        std::ostringstream oss;
        oss << message.size() << " > " << SMSG_MAX_MSG_BYTES;
        sError = "Message is too long, " + oss.str();
        LogPrintf("Message is too long, %u.\n", message.size());
        return 1;
    };


    int rv;
    SecureMessage smsg;

    if ((rv = SecureMsgEncrypt(smsg, addressFrom, addressTo, message)) != 0)
    {
        LogPrintf("SecureMsgSend(), encrypt for recipient failed.\n");

        switch(rv)
        {
            case 2:  sError = "Message is too long.";                       break;
            case 3:  sError = "Invalid addressFrom.";                       break;
            case 4:  sError = "Invalid addressTo.";                         break;
            case 5:  sError = "Could not get public key for addressTo.";    break;
            case 6:  sError = "ECDH_compute_key failed.";                   break;
            case 7:  sError = "Could not get private key for addressFrom."; break;
            case 8:  sError = "Could not allocate memory.";                 break;
            case 9:  sError = "Could not compress message data.";           break;
            case 10: sError = "Could not generate MAC.";                    break;
            case 11: sError = "Encrypt failed.";                            break;
            default: sError = "Unspecified Error.";                         break;
        };

        return rv;
    };


    // -- Place message in send queue, proof of work will happen in a thread.
    std::string sPrefix("qm");
    uint8_t chKey[18];
    memcpy(&chKey[0],  sPrefix.data(),  2);
    memcpy(&chKey[2],  &smsg.timestamp, 8);
    memcpy(&chKey[10], &smsg.pPayload,  8);

    SecMsgStored smsgSQ;

    smsgSQ.timeReceived  = GetTime();
    smsgSQ.sAddrTo       = addressTo;

    try {
        smsgSQ.vchMessage.resize(SMSG_HDR_LEN + smsg.nPayload);
    } catch (std::exception& e) {
        LogPrintf("smsgSQ.vchMessage.resize %u threw: %s.\n", SMSG_HDR_LEN + smsg.nPayload, e.what());
        sError = "Could not allocate memory.";
        return 8;
    };

    memcpy(&smsgSQ.vchMessage[0], &smsg.hash[0], SMSG_HDR_LEN);
    memcpy(&smsgSQ.vchMessage[SMSG_HDR_LEN], smsg.pPayload, smsg.nPayload);

    {
        LOCK(cs_smsgDB);
        SecMsgDB dbSendQueue;
        if (dbSendQueue.Open("cw"))
        {
            dbSendQueue.WriteSmesg(chKey, smsgSQ);
            //NotifySecMsgSendQueueChanged(smsgOutbox);
        };
    }

    // TODO: only update outbox when proof of work thread is done.

    //  -- for outbox create a copy encrypted for owned address
    //     if the wallet is encrypted private key needed to decrypt will be unavailable

    if (fDebugSmsg)
        LogPrintf("Encrypting message for outbox.\n");

    std::string addressOutbox = "None";
    CBitcoinAddress coinAddrOutbox;

    BOOST_FOREACH(const PAIRTYPE(CTxDestination, std::string)& entry, pwalletMain->mapAddressBook)
    {
        // -- get first owned address
        if (!IsMine(*pwalletMain, entry.first))
            continue;

        const CBitcoinAddress& address = entry.first;

        addressOutbox = address.ToString();
        if (!coinAddrOutbox.SetString(addressOutbox)) // test valid
            continue;
        break;
    };

    if (addressOutbox == "None")
    {
        LogPrintf("Warning: SecureMsgSend() could not find an address to encrypt outbox message with.\n");
    } else
    {
        if (fDebugSmsg)
            LogPrintf("Encrypting a copy for outbox, using address %s\n", addressOutbox.c_str());

        SecureMessage smsgForOutbox;
        if ((rv = SecureMsgEncrypt(smsgForOutbox, addressFrom, addressOutbox, message)) != 0)
        {
            LogPrintf("SecureMsgSend(), encrypt for outbox failed, %d.\n", rv);
        } else
        {
            // -- save sent message to db
            std::string sPrefix("sm");
            uint8_t chKey[18];
            memcpy(&chKey[0],  sPrefix.data(),           2);
            memcpy(&chKey[2],  &smsgForOutbox.timestamp, 8);
            memcpy(&chKey[10], &smsgForOutbox.pPayload,  8);   // sample

            SecMsgStored smsgOutbox;

            smsgOutbox.timeReceived  = GetTime();
            smsgOutbox.sAddrTo       = addressTo;
            smsgOutbox.sAddrOutbox   = addressOutbox;

            try {
                smsgOutbox.vchMessage.resize(SMSG_HDR_LEN + smsgForOutbox.nPayload);
            } catch (std::exception& e) {
                LogPrintf("smsgOutbox.vchMessage.resize %u threw: %s.\n", SMSG_HDR_LEN + smsgForOutbox.nPayload, e.what());
                sError = "Could not allocate memory.";
                return 8;
            };
            memcpy(&smsgOutbox.vchMessage[0], &smsgForOutbox.hash[0], SMSG_HDR_LEN);
            memcpy(&smsgOutbox.vchMessage[SMSG_HDR_LEN], smsgForOutbox.pPayload, smsgForOutbox.nPayload);


            {
                LOCK(cs_smsgDB);
                SecMsgDB dbSent;

                if (dbSent.Open("cw"))
                {
                    dbSent.WriteSmesg(chKey, smsgOutbox);
                    NotifySecMsgOutboxChanged(smsgOutbox);
                };
            }
        };
    };

    if (fDebugSmsg)
        LogPrintf("Secure message queued for sending to %s.\n", addressTo.c_str());

    return 0;
};


int SecureMsgDecrypt(bool fTestOnly, std::string& address, uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload, MessageData& msg)
{
    /* Decrypt secure message

        address is the owned address to decrypt with.

        validate first in SecureMsgValidate

        returns
            1       Error
            2       Unknown version number
            3       Decrypt address is not valid.
            8       Could not allocate memory
    */

    if (fDebugSmsg)
        LogPrintf("SecureMsgDecrypt(), using %s, testonly %d.\n", address.c_str(), fTestOnly);

    if (!pHeader
        || !pPayload)
    {
        LogPrintf("Error: null pointer to header or payload.\n");
        return 1;
    };

    SecureMessage* psmsg = (SecureMessage*) pHeader;


    if (psmsg->version[0] != 1)
    {
        LogPrintf("Unknown version number.\n");
        return 2;
    };



    // -- Fetch private key k, used to decrypt
    CBitcoinAddress coinAddrDest;
    CKeyID ckidDest;
    CKey keyDest;
    if (!coinAddrDest.SetString(address))
    {
        LogPrintf("Address is not valid.\n");
        return 3;
    };
    if (!coinAddrDest.GetKeyID(ckidDest))
    {
        LogPrintf("coinAddrDest.GetKeyID failed: %s.\n", coinAddrDest.ToString().c_str());
        return 3;
    };
    if (!pwalletMain->GetKey(ckidDest, keyDest))
    {
        LogPrintf("Could not get private key for addressDest.\n");
        return 3;
    };



    CPubKey cpkR(psmsg->cpkR, psmsg->cpkR+33);
    if (!cpkR.IsValid())
    {
        LogPrintf("Could not get pubkey for key R.\n");
        return 1;
    };
    
    CECKey ecKeyR;
    if (!ecKeyR.SetPubKey(cpkR))
    {
        LogPrintf("Could not set pubkey for key R: %s.\n", HexStr(cpkR).c_str());
        return 1;
    };
    
    CECKey ecKeyDest;
    ecKeyDest.SetSecretBytes(keyDest.begin());
    
    // -- Do an EC point multiply with private key k and public key R. This gives you public key P.
    std::vector<uint8_t> vchP;
    vchP.resize(32);
    EC_KEY* pkeyk = ecKeyDest.GetECKey();
    EC_KEY* pkeyR = ecKeyR.GetECKey();

    ECDH_set_method(pkeyk, ECDH_OpenSSL());
    int lenPdec = ECDH_compute_key(&vchP[0], 32, EC_KEY_get0_public_key(pkeyR), pkeyk, NULL);

    if (lenPdec != 32)
    {
        LogPrintf("ECDH_compute_key failed, lenPdec: %d.\n", lenPdec);
        return 1;
    };


    // -- Use public key P to calculate the SHA512 hash H.
    //    The first 32 bytes of H are called key_e and the last 32 bytes are called key_m.
    std::vector<uint8_t> vchHashedDec;
    vchHashedDec.resize(64);    // 512 bits
    SHA512(&vchP[0], vchP.size(), (uint8_t*)&vchHashedDec[0]);
    std::vector<uint8_t> key_e(&vchHashedDec[0], &vchHashedDec[0]+32);
    std::vector<uint8_t> key_m(&vchHashedDec[32], &vchHashedDec[32]+32);


    // -- Message authentication code, (hash of timestamp + destination + payload)
    uint8_t MAC[32];
    bool fHmacOk = true;
    uint32_t nBytes = 32;
    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);

    if (!HMAC_Init_ex(&ctx, &key_m[0], 32, EVP_sha256(), NULL)
        || !HMAC_Update(&ctx, (uint8_t*) &psmsg->timestamp, sizeof(psmsg->timestamp))
        || !HMAC_Update(&ctx, pPayload, nPayload)
        || !HMAC_Final(&ctx, MAC, &nBytes)
        || nBytes != 32)
        fHmacOk = false;

    HMAC_CTX_cleanup(&ctx);

    if (!fHmacOk)
    {
        LogPrintf("Could not generate MAC.\n");
        return 1;
    };

    if (memcmp(MAC, psmsg->mac, 32) != 0)
    {
        if (fDebugSmsg)
            LogPrintf("MAC does not match.\n"); // expected if message is not to address on node

        return 1;
    };

    if (fTestOnly)
        return 0;

    SecMsgCrypter crypter;
    crypter.SetKey(key_e, psmsg->iv);
    std::vector<uint8_t> vchPayload;
    if (!crypter.Decrypt(pPayload, nPayload, vchPayload))
    {
        LogPrintf("Decrypt failed.\n");
        return 1;
    };

    msg.timestamp = psmsg->timestamp;
    uint32_t lenData;
    uint32_t lenPlain;

    uint8_t* pMsgData;
    bool fFromAnonymous;
    if ((uint32_t)vchPayload[0] == 250)
    {
        fFromAnonymous = true;
        lenData = vchPayload.size() - (9);
        memcpy(&lenPlain, &vchPayload[5], 4);
        pMsgData = &vchPayload[9];
    } else
    {
        fFromAnonymous = false;
        lenData = vchPayload.size() - (SMSG_PL_HDR_LEN);
        memcpy(&lenPlain, &vchPayload[1+20+65], 4);
        pMsgData = &vchPayload[SMSG_PL_HDR_LEN];
    };

    try {
        msg.vchMessage.resize(lenPlain + 1);
    } catch (std::exception& e) {
        LogPrintf("msg.vchMessage.resize %u threw: %s.\n", lenPlain + 1, e.what());
        return 8;
    };


    if (lenPlain > 128)
    {
        // -- decompress
        if (LZ4_decompress_safe((char*) pMsgData, (char*) &msg.vchMessage[0], lenData, lenPlain) != (int) lenPlain)
        {
            LogPrintf("Could not decompress message data.\n");
            return 1;
        };
    } else
    {
        // -- plaintext
        memcpy(&msg.vchMessage[0], pMsgData, lenPlain);
    };

    msg.vchMessage[lenPlain] = '\0';

    if (fFromAnonymous)
    {
        // -- Anonymous sender
        msg.sFromAddress = "anon";
    } else
    {
        std::vector<uint8_t> vchUint160;
        vchUint160.resize(20);

        memcpy(&vchUint160[0], &vchPayload[1], 20);

        uint160 ui160(vchUint160);
        CKeyID ckidFrom(ui160);

        CBitcoinAddress coinAddrFrom;
        coinAddrFrom.Set(ckidFrom);
        if (!coinAddrFrom.IsValid())
        {
            LogPrintf("From Addess is invalid.\n");
            return 1;
        };

        std::vector<uint8_t> vchSig;
        vchSig.resize(65);

        memcpy(&vchSig[0], &vchPayload[1+20], 65);

        CPubKey cpkFromSig;
        cpkFromSig.RecoverCompact(Hash(msg.vchMessage.begin(), msg.vchMessage.end()-1), vchSig);
        if (!cpkFromSig.IsValid())
        {
            LogPrintf("Signature validation failed.\n");
            return 1;
        };

        // -- get address for the compressed public key
        CBitcoinAddress coinAddrFromSig;
        coinAddrFromSig.Set(cpkFromSig.GetID());

        if (!(coinAddrFrom == coinAddrFromSig))
        {
            LogPrintf("Signature validation failed.\n");
            return 1;
        };

        int rv = 5;
        try {
            rv = SecureMsgInsertAddress(ckidFrom, cpkFromSig);
        } catch (std::exception& e) {
            LogPrintf("SecureMsgInsertAddress(), exception: %s.\n", e.what());
            //return 1;
        };

        switch(rv)
        {
            case 0:
                LogPrintf("Sender public key added to db.\n");
                break;
            case 4:
                LogPrintf("Sender public key already in db.\n");
                break;
            default:
                LogPrintf("Error adding sender public key to db.\n");
                break;
        };

        msg.sFromAddress = coinAddrFrom.ToString();
    };

    if (fDebugSmsg)
        LogPrintf("Decrypted message for %s.\n", address.c_str());

    return 0;
};

int SecureMsgDecrypt(bool fTestOnly, std::string& address, SecureMessage& smsg, MessageData& msg)
{
    return SecureMsgDecrypt(fTestOnly, address, &smsg.hash[0], smsg.pPayload, smsg.nPayload, msg);
};

