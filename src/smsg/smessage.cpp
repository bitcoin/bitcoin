// Copyright (c) 2014-2016 The ShadowCoin developers
// Copyright (c) 2017-2018 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/*
Notes:
    Running with -debug could leave to and from address hashes and public keys in the log.

    Wallet Locked
        A copy of each incoming message is stored in bucket files ending in _wl.dat
        wl (wallet locked) bucket files are deleted if they expire, like normal buckets
        When the wallet is unlocked all the messages in wl files are scanned.

    Address Whitelist
        Owned Addresses are stored in addresses vector
        Saved to smsg.ini
        Modify options using the smsglocalkeys rpc command or edit the smsg.ini file (with client closed)

    TODO:
        For buckets older than current, only need to store no. messages and hash in memory

*/

#include <smsg/smessage.h>

#include <stdint.h>
#include <time.h>
#include <map>
#include <stdexcept>
#include <errno.h>
#include <limits>
#include <compat/byteswap.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/thread/thread.hpp>


#include <secp256k1.h>
#include <secp256k1_ecdh.h>
#include <secp256k1_recovery.h>

#include <crypto/hmac_sha256.h>
#include <crypto/sha512.h>

#include <script/ismine.h>
#include <policy/policy.h>
#include <support/allocators/secure.h>
#include <consensus/validation.h>
#include <validation.h>
#include <validationinterface.h>

#include <sync.h>
#include <random.h>
#include <chain.h>
#include <netmessagemaker.h>
#include <fs.h>

#ifdef ENABLE_WALLET
#include <wallet/coincontrol.h>
#include <wallet/wallet.h>
#endif

#include <utilstrencodings.h>
#include <clientversion.h>

#include <xxhash/xxhash.h>

#include <smsg/crypter.h>
#include <smsg/db.h>

extern void Misbehaving(NodeId pnode, int howmuch, const std::string& message="");
extern bool ReadBlockFromDisk(CBlock& block, const CBlockIndex* pindex, const Consensus::Params& consensusParams);
extern CChain &chainActive;
extern CCriticalSection cs_main;

smsg::CSMSG smsgModule;

namespace smsg
{
bool fSecMsgEnabled = false;

boost::thread_group threadGroupSmsg;

boost::signals2::signal<void (SecMsgStored &inboxHdr)> NotifySecMsgInboxChanged;
boost::signals2::signal<void (SecMsgStored &outboxHdr)> NotifySecMsgOutboxChanged;
boost::signals2::signal<void ()> NotifySecMsgWalletUnlocked;


secp256k1_context *secp256k1_context_smsg = nullptr;

uint32_t SMSGGetSecondsInDay()
{
    static bool fIsRegTest = Params().NetworkIDString() == "regtest";
    return fIsRegTest ? 600 : SMSG_SECONDS_IN_DAY;
}

std::string SecMsgToken::ToString() const
{
    return strprintf("%d-%08x", timestamp, *((uint64_t*)sample));
}

void SecMsgBucket::hashBucket()
{
    void *state = XXH32_init(1);

    int64_t now = GetAdjustedTime();

    nActive = 0;
    nLeastTTL = 0;
    for (auto it = setTokens.begin(); it != setTokens.end(); ++it)
    {
        if (it->timestamp + it->ttl * SMSGGetSecondsInDay() < now)
            continue;

        XXH32_update(state, it->sample, 8);
        if (it->ttl > 0 && (nLeastTTL == 0 || it->ttl < nLeastTTL))
            nLeastTTL = it->ttl;
        nActive++;
    };

    uint32_t hash_new = XXH32_digest(state);

    if (hash != hash_new)
    {
        LogPrint(BCLog::SMSG, "Bucket hash updated from %u to %u.\n", hash, hash_new);

        hash = hash_new;
        timeChanged = GetAdjustedTime();
    };

    LogPrint(BCLog::SMSG, "Hashed %u messages, hash %u\n", nActive, hash_new);
    return;
};

size_t SecMsgBucket::CountActive()
{
    size_t nMessages = 0;

    int64_t now = GetAdjustedTime();
    for (auto it = setTokens.begin(); it != setTokens.end(); ++it)
    {
        if (it->timestamp + it->ttl * SMSGGetSecondsInDay() < now)
            continue;
        nMessages++;
    };

    return nMessages;
};

void ThreadSecureMsg()
{
    // Bucket management thread

    uint32_t nLoop = 0;
    std::vector<std::pair<int64_t, NodeId> > vTimedOutLocks;
    while (fSecMsgEnabled)
    {
        nLoop++;
        int64_t now = GetAdjustedTime();

        if (LogAcceptCategory(BCLog::SMSG) && nLoop % SMSG_THREAD_LOG_GAP == 0) // log every SMSG_THREAD_LOG_GAP instance, is useful source of timestamps
            LogPrintf("SecureMsgThread %d \n", now);

        vTimedOutLocks.resize(0);
        int64_t cutoffTime = now - SMSG_RETENTION;
        {
            LOCK(smsgModule.cs_smsg);
            for (std::map<int64_t, SecMsgBucket>::iterator it(smsgModule.buckets.begin()); it != smsgModule.buckets.end(); )
            {
                bool fErase = it->first < cutoffTime;

                if (!fErase
                    && it->first + it->second.nLeastTTL * SMSGGetSecondsInDay() < now)
                {
                    it->second.hashBucket();

                    // TODO: periodically prune files
                    if (it->second.nActive < 1)
                        fErase = true;
                };

                if (fErase)
                {
                    LogPrint(BCLog::SMSG, "Removing bucket %d \n", it->first);

                    std::string fileName = std::to_string(it->first);

                    fs::path fullPath = GetDataDir() / "smsgstore" / (fileName + "_01.dat");
                    if (fs::exists(fullPath))
                    {
                        try { fs::remove(fullPath);
                        } catch (const fs::filesystem_error &ex)
                        {
                            LogPrintf("Error removing bucket file %s.\n", ex.what());
                        };
                    } else
                    {
                        LogPrintf("Path %s does not exist \n", fullPath.string());
                    };

                    // Look for a wl file, it stores incoming messages when wallet is locked
                    fullPath = GetDataDir() / "smsgstore" / (fileName + "_01_wl.dat");
                    if (fs::exists(fullPath))
                    {
                        try { fs::remove(fullPath);
                        } catch (const fs::filesystem_error &ex)
                        {
                            LogPrintf("Error removing wallet locked file %s.\n", ex.what());
                        };
                    };

                    smsgModule.buckets.erase(it++);
                } else
                {
                    if (it->second.nLockCount > 0) // Tick down nLockCount, to eventually expire if peer never sends data
                    {
                        it->second.nLockCount--;

                        if (it->second.nLockCount == 0) // lock timed out
                        {
                            vTimedOutLocks.push_back(std::make_pair(it->first, it->second.nLockPeerId)); // g_connman->cs_vNodes

                            it->second.nLockPeerId = 0;
                        };
                    };

                    ++it;
                };
            };

            if (smsgModule.nLastProcessedPurged + SMSGGetSecondsInDay() < now)
            {
                smsgModule.BuildPurgedSets();
            };
        } // cs_smsg

        for (std::vector<std::pair<int64_t, NodeId> >::iterator it(vTimedOutLocks.begin()); it != vTimedOutLocks.end(); it++)
        {
            NodeId nPeerId = it->second;
            uint32_t fExists = 0;

            LogPrint(BCLog::SMSG, "Lock on bucket %d for peer %d timed out.\n", it->first, nPeerId);

            // Look through the nodes for the peer that locked this bucket

            {
                LOCK(g_connman->cs_vNodes);
                for (auto *pnode : g_connman->vNodes)
                {
                    if (pnode->GetId() != nPeerId)
                        continue;

                    fExists = 1; //found in g_connman->vNodes

                    LOCK(pnode->smsgData.cs_smsg_net);
                    int64_t ignoreUntil = GetTime() + SMSG_TIME_IGNORE;
                    pnode->smsgData.ignoreUntil = ignoreUntil;

                    // Alert peer that they are being ignored
                    std::vector<uint8_t> vchData;
                    vchData.resize(8);
                    memcpy(&vchData[0], &ignoreUntil, 8);
                    g_connman->PushMessage(pnode,
                        CNetMsgMaker(INIT_PROTO_VERSION).Make("smsgIgnore", vchData));

                    LogPrint(BCLog::SMSG, "This node will ignore peer %d until %d.\n", nPeerId, ignoreUntil);
                    break;
                };
            } // g_connman->cs_vNodes

            LogPrint(BCLog::SMSG, "smsg-thread: ignoring - looked peer %d, status on search %u\n", nPeerId, fExists);
        };

        MilliSleep(SMSG_THREAD_DELAY * 1000); //  // check every SMSG_THREAD_DELAY seconds
    };
    return;
};

void ThreadSecureMsgPow()
{
    // Proof of work thread

    int rv;
    std::vector<uint8_t> vchKey;
    SecMsgStored smsgStored;

    std::string sPrefix("qm");
    uint8_t chKey[30];

    while (fSecMsgEnabled)
    {
        // Sleep at end, then fSecMsgEnabled is tested on wake

        SecMsgDB dbOutbox;
        leveldb::Iterator *it;
        {
            LOCK(cs_smsgDB);
            if (!dbOutbox.Open("cr+"))
                continue;

            // fifo (smallest key first)
            it = dbOutbox.pdb->NewIterator(leveldb::ReadOptions());
        }
        // Break up lock, SecureMsgSetHash will take long

        for (;;)
        {
            if (!fSecMsgEnabled)
                break;
            {
                LOCK(cs_smsgDB);
                if (!dbOutbox.NextSmesg(it, sPrefix, chKey, smsgStored))
                    break;
            }

            uint8_t *pHeader = &smsgStored.vchMessage[0];
            uint8_t *pPayload = &smsgStored.vchMessage[SMSG_HDR_LEN];
            SecureMessage *psmsg = (SecureMessage*) pHeader;

            const int64_t FUND_TXN_TIMEOUT = 3600 * 48;
            int64_t now = GetTime();

            if (psmsg->version[0] == 3)
            {
                uint256 txid;
                uint160 msgId;
                if (0 != smsgModule.HashMsg(*psmsg, pPayload, psmsg->nPayload-32, msgId)
                    || !GetFundingTxid(pPayload, psmsg->nPayload, txid))
                {
                    LogPrintf("%s: Get msgID or Txn Hash failed.\n", __func__);
                    LOCK(cs_smsgDB);
                    dbOutbox.EraseSmesg(chKey);
                    continue;
                };


                CTransactionRef txOut;
                uint256 hashBlock;
                {
                    LOCK(cs_main);
                    if (!GetTransaction(txid, txOut, Params().GetConsensus(), hashBlock))
                    {
                        // drop through
                    }
                }

                int blockDepth = -1;
                if (!hashBlock.IsNull())
                {
                    BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
                    if (mi != mapBlockIndex.end())
                    {
                        CBlockIndex *pindex = mi->second;
                        if (pindex && chainActive.Contains(pindex))
                            blockDepth = chainActive.Height() - pindex->nHeight + 1;
                    };
                };

                if (blockDepth > 0)
                {

                    LogPrintf("Found txn %s at depth %d\n", txid.ToString(), blockDepth);
                } else
                {
                    // Failure
                    if (psmsg->timestamp > now + FUND_TXN_TIMEOUT)
                    {
                        LogPrintf("%s: Funding txn timeout, dropping message %s\n", __func__, msgId.ToString());
                        LOCK(cs_smsgDB);
                        dbOutbox.EraseSmesg(chKey);
                    }
                    continue;
                }
            } else
            {
                // Do proof of work
                rv = smsgModule.SetHash(pHeader, pPayload, psmsg->nPayload);
                if (rv == SMSG_SHUTDOWN_DETECTED)
                    break; // leave message in db, if terminated due to shutdown
                if (rv != 0)
                {
                    LogPrintf("SecMsgPow: Could not get proof of work hash, message removed.\n");
                    LOCK(cs_smsgDB);
                    dbOutbox.EraseSmesg(chKey);
                    continue;
                };
            };


            // Remove message from queue
            {
                LOCK(cs_smsgDB);
                dbOutbox.EraseSmesg(chKey);
            }

            // Add to message store
            {
                LOCK(smsgModule.cs_smsg);
                if (smsgModule.Store(pHeader, pPayload, psmsg->nPayload, true) != 0)
                {
                    LogPrintf("SecMsgPow: Could not place message in buckets, message removed.\n");
                    continue;
                };
            }

            // Test if message was sent to self
            if (smsgModule.ScanMessage(pHeader, pPayload, psmsg->nPayload, true) != 0)
            {
                // Message recipient is not this node (or failed)
            };
        };

        delete it;

        // Shutdown thread waits 5 seconds, this should be less
        MilliSleep(2000);
    };
    return;
};

void AddOptions()
{
    gArgs.AddArg("-smsg", _("Enable secure messaging. (default: true)"), false, OptionsCategory::SMSG);
    gArgs.AddArg("-smsgscanchain", _("Scan the block chain for public key addresses on startup. (default: false)"), false, OptionsCategory::SMSG);
    gArgs.AddArg("-smsgscanincoming", _("Scan incoming blocks for public key addresses. (default: false)"), false, OptionsCategory::SMSG);
    gArgs.AddArg("-smsgnotify=<cmd>", _("Execute command when a message is received. (%s in cmd is replaced by receiving address)"), false, OptionsCategory::SMSG);
    gArgs.AddArg("-smsgsaddnewkeys", _("Scan for incoming messages on new wallet keys. (default: false)"), false, OptionsCategory::SMSG);

    return;
};

const char *GetString(size_t errorCode)
{
    switch(errorCode)
    {
        case SMSG_UNKNOWN_VERSION:                      return "Unknown version";
        case SMSG_INVALID_ADDRESS:                      return "Invalid address";
        case SMSG_INVALID_ADDRESS_FROM:                 return "Invalid address from";
        case SMSG_INVALID_ADDRESS_TO:                   return "Invalid address to";
        case SMSG_INVALID_PUBKEY:                       return "Invalid public key";
        case SMSG_PUBKEY_MISMATCH:                      return "Public key does not match address";
        case SMSG_PUBKEY_EXISTS:                        return "Public key exists in database";
        case SMSG_PUBKEY_NOT_EXISTS:                    return "Public key not in database";
        case SMSG_KEY_EXISTS:                           return "Key exists in database";
        case SMSG_KEY_NOT_EXISTS:                       return "Key not in database";
        case SMSG_UNKNOWN_KEY:                          return "Unknown key";
        case SMSG_UNKNOWN_KEY_FROM:                     return "Unknown private key for from address";
        case SMSG_ALLOCATE_FAILED:                      return "Allocate failed";
        case SMSG_MAC_MISMATCH:                         return "MAC mismatch";
        case SMSG_WALLET_UNSET:                         return "Wallet unset";
        case SMSG_WALLET_NO_PUBKEY:                     return "Pubkey not found in wallet";
        case SMSG_WALLET_NO_KEY:                        return "Key not found in wallet";
        case SMSG_WALLET_LOCKED:                        return "Wallet is locked";
        case SMSG_DISABLED:                             return "SMSG is disabled";
        case SMSG_UNKNOWN_MESSAGE:                      return "Unknown Message";
        case SMSG_PAYLOAD_OVER_SIZE:                    return "Payload too large";
        case SMSG_TIME_IN_FUTURE:                       return "Timestamp is in the future";
        case SMSG_TIME_EXPIRED:                         return "Time to live expired";
        case SMSG_INVALID_HASH:                         return "Invalid hash";
        case SMSG_CHECKSUM_MISMATCH:                    return "Checksum mismatch";
        case SMSG_SHUTDOWN_DETECTED:                    return "Shutdown detected";
        case SMSG_MESSAGE_TOO_LONG:                     return "Message is too long";
        case SMSG_COMPRESS_FAILED:                      return "Compression failed";
        case SMSG_ENCRYPT_FAILED:                       return "Encryption failed";
        case SMSG_FUND_FAILED:                          return "Fund message failed";
        default:
            return "Unknown error";
    };
    return "No Error";
};

static void NotifyUnload(CSMSG *ps)
{
    LogPrintf("SMSG NotifyUnload\n");
    ps->Disable();
};

int CSMSG::BuildBucketSet()
{
    /*
        Build the bucket set by scanning the files in the smsgstore dir.
        buckets should be empty
    */

    LogPrint(BCLog::SMSG, "%s\n", __func__);

    int64_t  now            = GetAdjustedTime();
    uint32_t nFiles         = 0;
    uint32_t nMessages      = 0;

    fs::path pathSmsgDir = GetDataDir() / "smsgstore";
    fs::directory_iterator itend;

    if (!fs::exists(pathSmsgDir)
        || !fs::is_directory(pathSmsgDir))
    {
        LogPrintf("Message store directory does not exist.\n");
        return SMSG_NO_ERROR; // not an error
    };

    for (fs::directory_iterator itd(pathSmsgDir); itd != itend; ++itd)
    {
        if (!fs::is_regular_file(itd->status()))
            continue;

        std::string fileType = itd->path().extension().string();

        if (fileType.compare(".dat") != 0)
            continue;

        nFiles++;
        std::string fileName = itd->path().filename().string();

        LogPrint(BCLog::SMSG, "Processing file: %s.\n", fileName);

        // TODO files must be split if > 2GB
        // time_noFile.dat
        size_t sep = fileName.find_first_of("_");
        if (sep == std::string::npos)
            continue;

        std::string stime = fileName.substr(0, sep);
        int64_t fileTime;
        if (!ParseInt64(stime, &fileTime))
        {
            LogPrintf("%s: ParseInt64 failed %s.\n", __func__, stime);
            continue;
        };

        if (fileTime < now - SMSG_RETENTION)
        {
            LogPrintf("Dropping file %s, expired.\n", fileName);
            try {
                fs::remove(itd->path());
            } catch (const fs::filesystem_error &ex) {
                LogPrintf("Error removing bucket file %s, %s.\n", fileName, ex.what());
            };
            continue;
        };

        if (part::endsWith(fileName, "_wl.dat"))
        {
            LogPrint(BCLog::SMSG, "Skipping wallet locked file: %s.\n", fileName);
            continue;
        };

        size_t nTokenSetSize = 0;
        SecureMessage smsg;
        {
            LOCK(cs_smsg);

            SecMsgBucket &bucket = buckets[fileTime];
            std::set<SecMsgToken> &tokenSet = bucket.setTokens;

            FILE *fp;
            if (!(fp = fopen(itd->path().string().c_str(), "rb")))
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
                if (fread(smsg.data(), sizeof(uint8_t), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN)
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

                uint32_t nDaysToLive = smsg.version[0] == 0 && smsg.version[1] == 0 ? 0  // Purged message header
                    : smsg.version[0] < 3 ? 2 : smsg.nonce[0];

                token.ttl = nDaysToLive;
                if (nDaysToLive > 0 && (bucket.nLeastTTL == 0 || nDaysToLive < bucket.nLeastTTL))
                    bucket.nLeastTTL = nDaysToLive;

                if (smsg.nPayload < 8)
                    continue;

                if (fread(token.sample, sizeof(uint8_t), 8, fp) != 8)
                {
                    LogPrintf("fread failed: %s\n", strerror(errno));
                    break;
                };

                if (fseek(fp, smsg.nPayload-8, SEEK_CUR) != 0)
                {
                    LogPrintf("fseek failed: %s.\n", strerror(errno));
                    break;
                };

                tokenSet.insert(token);
            };

            fclose(fp);

            buckets[fileTime].hashBucket();

            nTokenSetSize = tokenSet.size();
        } // cs_smsg

        nMessages += nTokenSetSize;
        LogPrint(BCLog::SMSG, "Bucket %d contains %u messages.\n", fileTime, nTokenSetSize);
    };

    LogPrintf("Processed %u files, loaded %u buckets containing %u messages.\n", nFiles, buckets.size(), nMessages);
    return SMSG_NO_ERROR;
};

int CSMSG::BuildPurgedSets()
{
    LogPrint(BCLog::SMSG, "%s\n", __func__);
    LOCK2(cs_smsg, cs_smsgDB);

    setPurged.clear();
    setPurgedTimestamps.clear();

    SecMsgDB db;
    if (!db.Open("cr+"))
        return SMSG_GENERAL_ERROR;

    int64_t now = GetTime();
    size_t nPurged = 0;
    std::string sPrefix("pm");
    uint8_t chKey[30];
    SecMsgPurged purged;
    leveldb::Iterator *it = db.pdb->NewIterator(leveldb::ReadOptions());
    while (db.NextPurged(it, sPrefix, chKey, purged))
    {
        if (purged.timepurged + 31 * SMSGGetSecondsInDay() < now)
        {
            db.ErasePurged(chKey);
            continue;
        };
        setPurged.insert(purged);
        setPurgedTimestamps.insert(purged.timestamp);
        nPurged++;
    };
    delete it;

    LogPrint(BCLog::SMSG, "Loaded %u purged tokens from database.\n", nPurged);

    nLastProcessedPurged = now;

    return SMSG_NO_ERROR;
};

/*
SecureMsgAddWalletAddresses
    Enumerates the AddressBook, filters out anon outputs and checks the "real addresses"
    Adds these to the vector addresses to be used for decryption

    Returns 0 on success
*/

int CSMSG::AddWalletAddresses()
{
    LogPrint(BCLog::SMSG, "%s\n", __func__);

#ifdef ENABLE_WALLET
    if (!pwallet)
        return errorN(SMSG_WALLET_UNSET, "No wallet.");

    if (!gArgs.GetBoolArg("-smsgsaddnewkeys", false))
    {
        LogPrint(BCLog::SMSG, "%s smsgsaddnewkeys option is disabled.\n", __func__);
        return SMSG_GENERAL_ERROR;
    };

    uint32_t nAdded = 0;

    for (const auto &entry : pwallet->mapAddressBook) // PAIRTYPE(CTxDestination, CAddressBookData)
    {
        if (!IsMine(*pwallet, entry.first))
            continue;

        // TODO: skip addresses for stealth transactions
        CKeyID keyID;
        CBitcoinAddress coinAddress(entry.first);
        if (!coinAddress.IsValid()
            || !coinAddress.GetKeyID(keyID))
            continue;

        bool fExists = 0;
        for (std::vector<SecMsgAddress>::iterator it = addresses.begin(); it != addresses.end(); ++it)
        {
            if (keyID != it->address)
                continue;
            fExists = 1;
            break;
        };

        if (fExists)
            continue;

        bool recvEnabled    = 1;
        bool recvAnon       = 1;

        addresses.push_back(SecMsgAddress(keyID, recvEnabled, recvAnon));
        nAdded++;
    };

    LogPrint(BCLog::SMSG, "Added %u addresses to whitelist.\n", nAdded);
#endif
    return SMSG_NO_ERROR;
};

int CSMSG::LoadKeyStore()
{
    LOCK(cs_smsgDB);

    SecMsgDB db;
    if (!db.Open("cr+"))
        return SMSG_GENERAL_ERROR;

    size_t nKeys = 0;
    std::string sPrefix("sk");
    CKeyID idk;
    SecMsgKey key;
    leveldb::Iterator *it = db.pdb->NewIterator(leveldb::ReadOptions());
    while (db.NextPrivKey(it, sPrefix, idk, key))
    {
        if (!(key.nFlags & SMK_RECEIVE_ON))
            continue;
        keyStore.AddKey(idk, key);
        nKeys++;
    };
    delete it;

    LogPrint(BCLog::SMSG, "Loaded %u keys from database.\n", nKeys);
    return SMSG_NO_ERROR;
};

int CSMSG::ReadIni()
{
    if (!fSecMsgEnabled)
        return SMSG_DISABLED;

    LogPrint(BCLog::SMSG, "%s\n", __func__);

    fs::path fullpath = GetDataDir() / "smsg.ini";

    FILE *fp;
    errno = 0;
    if (!(fp = fopen(fullpath.string().c_str(), "r")))
        return errorN(SMSG_GENERAL_ERROR, "%s: Error opening file: %s", __func__, strerror(errno));

    char cLine[512];
    char *pName, *pValue;

    char cAddress[64];
    int addrRecv, addrRecvAnon;

    while (fgets(cLine, 512, fp))
    {
        cLine[strcspn(cLine, "\n")] = '\0';
        cLine[strcspn(cLine, "\r")] = '\0';
        cLine[511] = '\0'; // for safety

        // Check that line contains a name value pair and is not a comment, or section header
        if (cLine[0] == '#' || cLine[0] == '[' || strcspn(cLine, "=") < 1)
            continue;

        if (!(pName = strtok(cLine, "="))
            || !(pValue = strtok(nullptr, "=")))
            continue;

        if (strcmp(pName, "newAddressRecv") == 0)
        {
            options.fNewAddressRecv = (strcmp(pValue, "true") == 0) ? true : false;
        } else
        if (strcmp(pName, "newAddressAnon") == 0)
        {
            options.fNewAddressAnon = (strcmp(pValue, "true") == 0) ? true : false;
        } else
        if (strcmp(pName, "scanIncoming") == 0)
        {
            options.fScanIncoming = (strcmp(pValue, "true") == 0) ? true : false;
        } else
        if (strcmp(pName, "key") == 0)
        {
            int rv = sscanf(pValue, "%64[^|]|%d|%d", cAddress, &addrRecv, &addrRecvAnon);
            if (rv == 3)
            {
                CKeyID k;
                CBitcoinAddress(cAddress).GetKeyID(k);

                if (k.IsNull())
                    LogPrintf("Could not parse key line %s, rv %d.\n", pValue, rv);
                else
                    addresses.push_back(SecMsgAddress(k, addrRecv, addrRecvAnon));
            } else
            {
                LogPrintf("Could not parse key line %s, rv %d.\n", pValue, rv);
            };
        } else
        {
            LogPrintf("Unknown setting name: '%s'.\n", pName);
        };
    };

    fclose(fp);
    LogPrintf("Loaded %u addresses.\n", addresses.size());
    return SMSG_NO_ERROR;
};

int CSMSG::WriteIni()
{
    if (!fSecMsgEnabled)
        return SMSG_DISABLED;

    LogPrint(BCLog::SMSG, "%s\n", __func__);

    fs::path fullpath = GetDataDir() / "smsg.ini~";

    FILE *fp;
    errno = 0;
    if (!(fp = fopen(fullpath.string().c_str(), "w")))
        return errorN(SMSG_GENERAL_ERROR, "%s: Error opening file: %s", __func__, strerror(errno));

    if (fwrite("[Options]\n", sizeof(char), 10, fp) != 10)
    {
        LogPrintf("fwrite error: %s\n", strerror(errno));
        fclose(fp);
        return SMSG_GENERAL_ERROR;
    };

    if (fprintf(fp, "newAddressRecv=%s\n", options.fNewAddressRecv ? "true" : "false") < 0
        || fprintf(fp, "newAddressAnon=%s\n", options.fNewAddressAnon ? "true" : "false") < 0
        || fprintf(fp, "scanIncoming=%s\n", options.fScanIncoming ? "true" : "false") < 0)
    {
        LogPrintf("fprintf error: %s\n", strerror(errno));
        fclose(fp);
        return SMSG_GENERAL_ERROR;
    };

    if (fwrite("\n[Keys]\n", sizeof(char), 8, fp) != 8)
    {
        LogPrintf("fwrite error: %s\n", strerror(errno));
        fclose(fp);
        return SMSG_GENERAL_ERROR;
    };

    for (std::vector<SecMsgAddress>::iterator it = addresses.begin(); it != addresses.end(); ++it)
    {
        errno = 0;

        CBitcoinAddress cAddress(it->address);

        if (!cAddress.IsValid())
        {
            LogPrintf("%s: Error saving address - invalid.\n", __func__);
            continue;
        };

        if (fprintf(fp, "key=%s|%d|%d\n", cAddress.ToString().c_str(), it->fReceiveEnabled, it->fReceiveAnon) < 0)
        {
            LogPrintf("fprintf error: %s\n", strerror(errno));
            continue;
        };
    };

    fclose(fp);

    try {
        fs::path finalpath = GetDataDir() / "smsg.ini";
        fs::rename(fullpath, finalpath);
    } catch (const fs::filesystem_error &ex) {
        LogPrintf("Error renaming file %s, %s.\n", fullpath.string(), ex.what());
    };
    return SMSG_NO_ERROR;
};

bool CSMSG::Start(std::shared_ptr<CWallet> pwalletIn, bool fDontStart, bool fScanChain)
{
    if (fDontStart)
    {
        LogPrintf("Secure messaging not started.\n");
        return false;
    };

    LogPrintf("Secure messaging starting.\n");

    if (pwallet)
        return error("%s: pwallet is already set.", __func__);
    pwallet = pwalletIn;
    m_handler_unload = interfaces::MakeHandler(pwallet->NotifyUnload.connect(boost::bind(&NotifyUnload, this)));

    fSecMsgEnabled = true;
    g_connman->SetLocalServices(ServiceFlags(g_connman->GetLocalServices() | NODE_SMSG));

    if (ReadIni() != 0)
        LogPrintf("Failed to read smsg.ini\n");

    if (addresses.size() < 1)
    {
        LogPrintf("No address keys loaded.\n");
        if (AddWalletAddresses() != 0)
            LogPrintf("Failed to load addresses from wallet.\n");
        else
            LogPrintf("Loaded addresses from wallet.\n");
    } else
    {
        LogPrintf("Loaded addresses from SMSG.ini\n");
    };

    if (LoadKeyStore() != 0)
        return error("%s: LoadKeyStore failed.", __func__);

    if (secp256k1_context_smsg)
        return error("%s: secp256k1_context_smsg already exists.", __func__);

    if (!(secp256k1_context_smsg = secp256k1_context_create(SECP256K1_CONTEXT_SIGN)))
        return error("%s: secp256k1_context_create failed.", __func__);

    {
        // Pass in a random blinding seed to the secp256k1 context.
        std::vector<uint8_t, secure_allocator<uint8_t>> vseed(32);
        GetRandBytes(vseed.data(), 32);
        bool ret = secp256k1_context_randomize(secp256k1_context_smsg, vseed.data());
        assert(ret);
    }

    if (fScanChain)
    {
        ScanBlockChain();
    };

    if (BuildBucketSet() != 0)
    {
        fSecMsgEnabled = false;
        return error("%s: Could not load bucket sets, secure messaging disabled.", __func__);
    };

    if (BuildPurgedSets() != 0)
    {
        fSecMsgEnabled = false;
        return error("%s: Could not load purged sets, secure messaging disabled.", __func__);
    };

    threadGroupSmsg.create_thread(boost::bind(&TraceThread<void (*)()>, "smsg", &ThreadSecureMsg));
    threadGroupSmsg.create_thread(boost::bind(&TraceThread<void (*)()>, "smsg-pow", &ThreadSecureMsgPow));

    return true;
};

bool CSMSG::Shutdown()
{
    if (!fSecMsgEnabled)
        return false;

    LogPrintf("Stopping secure messaging.\n");

    if (WriteIni() != 0)
        LogPrintf("Failed to save smsg.ini\n");

    fSecMsgEnabled = false;
    g_connman->SetLocalServices(ServiceFlags(g_connman->GetLocalServices() & ~NODE_SMSG));

    threadGroupSmsg.interrupt_all();
    threadGroupSmsg.join_all();

    if (smsgDB)
    {
        LOCK(cs_smsgDB);
        delete smsgDB;
        smsgDB = nullptr;
    };

    keyStore.Clear();

    if (secp256k1_context_smsg)
        secp256k1_context_destroy(secp256k1_context_smsg);
    secp256k1_context_smsg = nullptr;

    m_handler_unload->disconnect();
    pwallet.reset();
    return true;
};

bool CSMSG::Enable(std::shared_ptr<CWallet> pwallet)
{
    // Start secure messaging at runtime
    if (fSecMsgEnabled)
    {
        LogPrintf("SecureMsgEnable: secure messaging is already enabled.\n");
        return false;
    };

    {
        LOCK(cs_smsg);

        addresses.clear(); // should be empty already
        buckets.clear(); // should be empty already

        if (!Start(pwallet, false, false))
            return error("%s: SecureMsgStart failed.\n", __func__);
    } // cs_smsg

    // Ping each peer advertising smsg
    {
        LOCK(g_connman->cs_vNodes);
        for(auto *pnode : g_connman->vNodes)
        {
            if (!(pnode->GetLocalServices() & NODE_SMSG))
                continue;
            g_connman->PushMessage(pnode,
                CNetMsgMaker(INIT_PROTO_VERSION).Make("smsgPing"));
            g_connman->PushMessage(pnode,
                CNetMsgMaker(INIT_PROTO_VERSION).Make("smsgPong")); // Send pong as have missed initial ping sent by peer when it connected
        };
    } // g_connman->cs_vNodes

    LogPrintf("Secure messaging enabled.\n");
    return true;
};

bool CSMSG::Disable()
{
    // Stop secure messaging at runtime
    if (!fSecMsgEnabled)
        return error("%s: Secure messaging is already disabled.", __func__);

    {
        LOCK(cs_smsg);

        if (!Shutdown())
            return error("%s: SecureMsgShutdown failed.\n", __func__);

        // Clear buckets
        std::map<int64_t, SecMsgBucket>::iterator it;
        it = buckets.begin();
        for (it = buckets.begin(); it != buckets.end(); ++it)
            it->second.setTokens.clear();
        buckets.clear();
        addresses.clear();
    } // cs_smsg

    // Tell each smsg enabled peer that this node is disabling
    {
        LOCK(g_connman->cs_vNodes);
        for (auto *pnode : g_connman->vNodes)
        {
            if (!pnode->smsgData.fEnabled)
                continue;
            LOCK(pnode->smsgData.cs_smsg_net);
            g_connman->PushMessage(pnode,
                CNetMsgMaker(INIT_PROTO_VERSION).Make("smsgDisabled"));
            pnode->smsgData.fEnabled = false;
        };
    } // g_connman->cs_vNodes

    LogPrintf("Secure messaging disabled.\n");
    return true;
};


int CSMSG::ReceiveData(CNode *pfrom, const std::string &strCommand, CDataStream &vRecv)
{
    /*
        Called from ProcessMessage
        Runs in ThreadMessageHandler2
    */

    /*
        returns SecureMessageCodes

        TODO:
        Explain better and make use of better terminology such as
        Node A <-> Node B <-> Node C

        Commands
        + smsgInv =
            (1) received inventory of other node.
                (1.1) sanity checks
            (2) loop through buckets
                (2.1) sanity checks
                (2.2) check if bucket is locked to node C, if so continue but don't match. TODO: handle this properly, add critical section, lock on write. On read: nothing changes = no lock
                    (2.2.3) If our bucket is not locked to another node then add hash to buffer to be requested..
            (3) send smsgShow with list of hashes to request.

        + smsgShow =
            (1) received a list of requested bucket hashes which the other party does not have.
            (2) respond with smsgHave - contains all the message hashes within the requested buckets.
        + smsgHave =
            (1) A list of all the message hashes which a node has in response to smsgShow.
        + smsgWant =
            (1) A list of the message hashes that a node does not have and wants to retrieve from the node which sent smsgHave
        + smsgMsg =
            (1) In response to
        + smsgPing = ping request
        + smsgPong = pong response
        + smsgMatch =
            Obsolete, it used tell a node up to which time their messages were synced in response to smsg, but this is overhead because we know exactly when we sent them
    */

    if (LogAcceptCategory(BCLog::SMSG))
        LogPrintf("%s: %s %s.\n", __func__, pfrom->GetAddrName(), strCommand);

    if (IsInitialBlockDownload()) // Wait until chain synced
    {
        if (strCommand == "smsgPing")
            pfrom->smsgData.lastSeen = -1; // Mark node as requiring a response once chain is synced
        return SMSG_NO_ERROR;
    };

    if (!fSecMsgEnabled)
    {
        if (strCommand == "smsgPing") // ignore smsgPing
            return SMSG_NO_ERROR;
        return SMSG_UNKNOWN_MESSAGE;
    };

    if (pfrom->nVersion < MIN_SMSG_PROTO_VERSION)
        return SMSG_NO_ERROR;

    if (strCommand == "smsgInv")
    {
        std::vector<uint8_t> vchData;
        vRecv >> vchData;

        if (vchData.size() < 4)
        {
            Misbehaving(pfrom->GetId(), 1);
            return SMSG_GENERAL_ERROR; // Not enough data received to be a valid smsgInv
        };

        int64_t now = GetAdjustedTime();

        {
            LOCK(pfrom->smsgData.cs_smsg_net);

            if (now < pfrom->smsgData.ignoreUntil)
            {
                LogPrint(BCLog::SMSG, "Node is ignoring peer %d until %d.\n", pfrom->GetId(), pfrom->smsgData.ignoreUntil);
                return SMSG_GENERAL_ERROR;
            };
        }

        uint32_t nBuckets = buckets.size();
        uint32_t nLocked = 0;           // no. of locked buckets on this node
        uint32_t nInvBuckets;           // no. of bucket headers sent by peer in smsgInv
        memcpy(&nInvBuckets, &vchData[0], 4);
        LogPrint(BCLog::SMSG, "Remote node sent %d bucket headers, this has %d.\n", nInvBuckets, nBuckets);


        // Check no of buckets:
        if (nInvBuckets > (SMSG_RETENTION / SMSG_BUCKET_LEN) + 1) // +1 for some leeway
        {
            LogPrintf("Peer sent more bucket headers than possible %u, %u.\n", nInvBuckets, (SMSG_RETENTION / SMSG_BUCKET_LEN));
            Misbehaving(pfrom->GetId(), 1);
            return SMSG_GENERAL_ERROR;
        };

        if (vchData.size() < 4 + nInvBuckets * 16)
        {
            LogPrintf("Remote node did not send enough data.\n");
            Misbehaving(pfrom->GetId(), 1);
            return SMSG_GENERAL_ERROR;
        };

        std::vector<uint8_t> vchDataOut;
        vchDataOut.reserve(4 + 8 * nInvBuckets); // Reserve max possible size
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
                LogPrint(BCLog::SMSG, "Not interested in peer bucket %d, has expired.\n", time);

                if (time < now - SMSG_RETENTION - SMSG_TIME_LEEWAY)
                    Misbehaving(pfrom->GetId(), 1);
                continue;
            };
            if (time > now + SMSG_TIME_LEEWAY)
            {
                LogPrint(BCLog::SMSG, "Not interested in peer bucket %d, in the future.\n", time);
                Misbehaving(pfrom->GetId(), 1);
                continue;
            };

            if (ncontent < 1)
            {
                LogPrint(BCLog::SMSG, "Peer sent empty bucket, ignore %d %u %u.\n", time, ncontent, hash);
                continue;
            };

            if (LogAcceptCategory(BCLog::SMSG))
            {
                LogPrintf("Peer bucket %d %u %u.\n", time, ncontent, hash);
                LogPrintf("This bucket %d %u %u.\n", time, buckets[time].setTokens.size(), buckets[time].hash);
            };
            {
                LOCK(cs_smsg);
                if (buckets[time].nLockCount > 0)
                {
                    LogPrint(BCLog::SMSG, "Bucket is locked %u, waiting for peer %u to send data.\n", buckets[time].nLockCount, buckets[time].nLockPeerId);
                    nLocked++;
                    continue;
                };

                // If this node has more than the peer node, peer node will pull from this
                //  if then peer node has more this node will pull fom peer
                if (buckets[time].setTokens.size() < ncontent
                    || (buckets[time].setTokens.size() == ncontent
                        && buckets[time].hash != hash)) // if same amount in buckets check hash
                {
                    LogPrint(BCLog::SMSG, "Requesting contents of bucket %d.\n", time);

                    uint32_t sz = vchDataOut.size();
                    vchDataOut.resize(sz + 8);
                    memcpy(&vchDataOut[sz], &time, 8);

                    nShowBuckets++;
                };
            } // cs_smsg
        };

        // TODO: should include hash?
        memcpy(&vchDataOut[0], &nShowBuckets, 4);
        if (vchDataOut.size() > 4)
        {
            g_connman->PushMessage(pfrom,
                CNetMsgMaker(INIT_PROTO_VERSION).Make("smsgShow", vchDataOut));
        } else
        if (nLocked < 1) // Don't report buckets as matched if any are locked
        {
            // Peer has no buckets we want, don't send them again until something changes
            //  peer will still request buckets from this node if needed (< ncontent)
            vchDataOut.resize(8);
            memcpy(&vchDataOut[0], &now, 8);
            g_connman->PushMessage(pfrom,
                CNetMsgMaker(INIT_PROTO_VERSION).Make("smsgMatch", vchDataOut));
            LogPrint(BCLog::SMSG, "Sending smsgMatch, no locked buckets, time = %d.\n", now);
        } else
        if (nLocked >= 1)
        {
            LogPrint(BCLog::SMSG, "%u buckets were locked, time = %d.\n", nLocked, now);
        };
    } else
    if (strCommand == "smsgShow")
    {
        std::vector<uint8_t> vchData;
        vRecv >> vchData;

        if (vchData.size() < 4)
            return SMSG_GENERAL_ERROR;

        uint32_t nBuckets;
        memcpy(&nBuckets, &vchData[0], 4);

        if (vchData.size() < 4 + nBuckets * 8)
            return SMSG_GENERAL_ERROR;

        LogPrint(BCLog::SMSG, "smsgShow: peer wants to see content of %u buckets.\n", nBuckets);

        std::map<int64_t, SecMsgBucket>::iterator itb;
        std::set<SecMsgToken>::iterator it;

        std::vector<uint8_t> vchDataOut;
        int64_t time;
        uint8_t *pIn = &vchData[4];
        for (uint32_t i = 0; i < nBuckets; ++i, pIn += 8)
        {
            memcpy(&time, pIn, 8);

            {
                LOCK(cs_smsg);
                itb = buckets.find(time);
                if (itb == buckets.end())
                {
                    LogPrint(BCLog::SMSG, "Don't have bucket %d.\n", time);
                    continue;
                };

                std::set<SecMsgToken> &tokenSet = itb->second.setTokens;

                try { vchDataOut.resize(8 + 16 * tokenSet.size());
                } catch (std::exception &e) {
                    LogPrintf("vchDataOut.resize %u threw: %s.\n", 8 + 16 * tokenSet.size(), e.what());
                    continue;
                };
                memcpy(&vchDataOut[0], &time, 8);

                int64_t now = GetAdjustedTime();
                size_t nMessages = 0;
                uint8_t *p = &vchDataOut[8];
                for (it = tokenSet.begin(); it != tokenSet.end(); ++it)
                {
                    if (it->timestamp + it->ttl * SMSGGetSecondsInDay() < now)
                        continue;
                    memcpy(p, &it->timestamp, 8);
                    memcpy(p+8, &it->sample, 8);

                    p += 16;
                    nMessages++;
                };
                if (nMessages != tokenSet.size())
                {
                    try { vchDataOut.resize(8 + 16 * nMessages);
                    } catch (std::exception &e) {
                        LogPrintf("vchDataOut.resize %u threw: %s.\n", 8 + 16 * nMessages, e.what());
                        continue;
                    };
                };
            }
            g_connman->PushMessage(pfrom,
                CNetMsgMaker(INIT_PROTO_VERSION).Make("smsgHave", vchDataOut));
        };
    } else
    if (strCommand == "smsgHave")
    {
        // Peer has these messages in bucket
        std::vector<uint8_t> vchData;
        vRecv >> vchData;

        if (vchData.size() < 8)
            return SMSG_GENERAL_ERROR;

        int n = (vchData.size() - 8) / 16;

        int64_t time;
        memcpy(&time, &vchData[0], 8);

        // Check time valid:
        int64_t now = GetAdjustedTime();
        if (time < now - SMSG_RETENTION)
        {
            LogPrint(BCLog::SMSG, "Not interested in peer bucket %d, has expired.\n", time);
            return SMSG_GENERAL_ERROR;
        };
        if (time > now + SMSG_TIME_LEEWAY)
        {
            LogPrint(BCLog::SMSG, "Not interested in peer bucket %d, in the future.\n", time);
            Misbehaving(pfrom->GetId(), 1);
            return SMSG_GENERAL_ERROR;
        };

        std::vector<uint8_t> vchDataOut;

        {
            LOCK(cs_smsg);
            if (buckets[time].nLockCount > 0)
            {
                LogPrint(BCLog::SMSG, "Bucket %d lock count %u, waiting for message data from peer %u.\n", time, buckets[time].nLockCount, buckets[time].nLockPeerId);
                return SMSG_GENERAL_ERROR;
            };

            LogPrint(BCLog::SMSG, "Sifting through bucket %d.\n", time);

            vchDataOut.resize(8);
            memcpy(&vchDataOut[0], &vchData[0], 8);

            std::set<SecMsgToken> &tokenSet = buckets[time].setTokens;
            std::set<SecMsgToken>::iterator it;
            SecMsgToken token;
            SecMsgPurged purgedToken;
            uint8_t *p = &vchData[8];

            for (int i = 0; i < n; ++i, p += 16)
            {
                memcpy(&token.timestamp, p, 8);
                memcpy(&token.sample, p+8, 8);

                if (setPurgedTimestamps.find(token.timestamp) != setPurgedTimestamps.end())
                {
                    memcpy(&purgedToken.timestamp, p, 8);
                    memcpy(&purgedToken.sample, p+8, 8);
                    if (setPurged.find(purgedToken) != setPurged.end())
                    {
                        continue;
                    };
                };

                it = tokenSet.find(token);
                if (it == tokenSet.end())
                {
                    int nd = vchDataOut.size();
                    try {
                        vchDataOut.resize(nd + 16);
                    } catch (std::exception &e) {
                        LogPrintf("vchDataOut.resize %d threw: %s.\n", nd + 16, e.what());
                        continue;
                    };

                    memcpy(&vchDataOut[nd], p, 16);
                };
            };
        } // cs_smsg

        if (vchDataOut.size() > 8)
        {
            if (LogAcceptCategory(BCLog::SMSG))
            {
                LogPrintf("Asking peer for %u messages.\n", (vchDataOut.size() - 8) / 16);
                LogPrintf("Locking bucket %u for peer %d.\n", time, pfrom->GetId());
            };
            {
                LOCK(cs_smsg);
                buckets[time].nLockCount   = 3; // lock this bucket for at most 3 * SMSG_THREAD_DELAY seconds, unset when peer sends smsgMsg
                buckets[time].nLockPeerId  = pfrom->GetId();
            }
            g_connman->PushMessage(pfrom,
                CNetMsgMaker(INIT_PROTO_VERSION).Make("smsgWant", vchDataOut));
        };
    } else
    if (strCommand == "smsgWant")
    {
        std::vector<uint8_t> vchData;
        vRecv >> vchData;

        if (vchData.size() < 8)
            return SMSG_GENERAL_ERROR;

        std::vector<uint8_t> vchOne, vchBunch;

        vchBunch.resize(4 + 8); // nMessages + bucketTime

        int n = (vchData.size() - 8) / 16;

        int64_t time;
        uint32_t nBunch = 0;
        memcpy(&time, &vchData[0], 8);

        {
            LOCK(cs_smsg);
            auto itb = buckets.find(time);
            if (itb == buckets.end())
            {
                LogPrint(BCLog::SMSG, "Don't have bucket %d.\n", time);
                return SMSG_GENERAL_ERROR;
            };

            std::set<SecMsgToken> &tokenSet = itb->second.setTokens;
            std::set<SecMsgToken>::iterator it;
            SecMsgToken token;
            uint8_t *p = &vchData[8];
            for (int i = 0; i < n; ++i)
            {
                memcpy(&token.timestamp, p, 8);
                memcpy(&token.sample, p+8, 8);

                it = tokenSet.find(token);
                if (it == tokenSet.end())
                {
                    LogPrint(BCLog::SMSG, "Don't have wanted message %d.\n", token.timestamp);
                } else
                {
                    //LogPrintf("Have message at %d.\n", it->offset); // DEBUG
                    token.offset = it->offset;

                    // Place in vchOne so if SecureMsgRetrieve fails it won't corrupt vchBunch
                    if (Retrieve(token, vchOne) == SMSG_NO_ERROR)
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
                        LogPrint(BCLog::SMSG, "Break bunch %u, %u.\n", nBunch, vchBunch.size());
                        break; // end here, peer will send more want messages if needed.
                    };
                };
                p += 16;
            };
        } // cs_smsg

        if (nBunch > 0)
        {
            LogPrint(BCLog::SMSG, "Sending block of %u messages for bucket %d.\n", nBunch, time);

            memcpy(&vchBunch[0], &nBunch, 4);
            memcpy(&vchBunch[4], &time, 8);
            g_connman->PushMessage(pfrom,
                CNetMsgMaker(INIT_PROTO_VERSION).Make("smsgMsg", vchBunch));
        };
    } else
    if (strCommand == "smsgMsg")
    {
        std::vector<uint8_t> vchData;
        vRecv >> vchData;

        LogPrint(BCLog::SMSG, "smsgMsg vchData.size() %u.\n", vchData.size());

        Receive(pfrom, vchData);
    } else
    if (strCommand == "smsgMatch")
    {
        /*
        Basically all this code has to go..
        For now we can use it to punish nodes running the older version, not that it's really need because the overhead is small.
        TODO: remove this code.
        */
        std::vector<uint8_t> vchData;
        vRecv >> vchData;

        if (vchData.size() < 8)
        {
            LogPrintf("smsgMatch, not enough data %u.\n", vchData.size());
            Misbehaving(pfrom->GetId(), 1);
            return SMSG_GENERAL_ERROR;
        };

        int64_t time;
        memcpy(&time, &vchData[0], 8);

        int64_t now = GetAdjustedTime();
        if (time > now + SMSG_TIME_LEEWAY)
        {
            LogPrintf("Warning: Peer buckets matched in the future: %d.\nEither this node or the peer node has the incorrect time set.\n", time);
            LogPrint(BCLog::SMSG, "Peer match time set to now.\n");
            time = now;
        };
        /*
        {
            LOCK(pfrom->smsgData.cs_smsg_net);
            pfrom->smsgData.lastMatched = time;
        }*/
        LogPrint(BCLog::SMSG, "Peer buckets matched in smsgWant at %d.\n", time);
    } else
    if (strCommand == "smsgPing")
    {
        // smsgPing is the initial message, send reply
        g_connman->PushMessage(pfrom,
            CNetMsgMaker(INIT_PROTO_VERSION).Make("smsgPong"));
    } else
    if (strCommand == "smsgPong")
    {
        LogPrint(BCLog::SMSG, "Peer replied, secure messaging enabled.\n");

        {
            LOCK(pfrom->smsgData.cs_smsg_net);
            pfrom->smsgData.fEnabled = true;
        }
    } else
    if (strCommand == "smsgDisabled")
    {
        LogPrint(BCLog::SMSG, "Peer %d has disabled secure messaging.\n", pfrom->GetId());

        {
            LOCK(pfrom->smsgData.cs_smsg_net);
            pfrom->smsgData.fEnabled = false;
        }
    } else
    if (strCommand == "smsgIgnore")
    {
        // Peer is reporting that it will ignore this node until time.
        //  Ignore peer too
        std::vector<uint8_t> vchData;
        vRecv >> vchData;

        if (vchData.size() < 8)
        {
            LogPrintf("smsgIgnore, not enough data %u.\n", vchData.size());
            Misbehaving(pfrom->GetId(), 1);
            return SMSG_GENERAL_ERROR;
        };

        int64_t time;
        memcpy(&time, &vchData[0], 8);

        {
            LOCK(pfrom->smsgData.cs_smsg_net);
            pfrom->smsgData.ignoreUntil = time;
        }

        LogPrint(BCLog::SMSG, "Peer %d is ignoring this node until %d, ignore peer too.\n", pfrom->GetId(), time);
    } else
    {
        return SMSG_UNKNOWN_MESSAGE;
    };

    return SMSG_NO_ERROR;
};

bool CSMSG::SendData(CNode *pto, bool fSendTrickle)
{
    /*
        Called from ProcessMessage
        Runs in ThreadMessageHandler2
    */

    if (IsInitialBlockDownload()) // Wait until chain synced
    {
        return true;
    };

    LOCK(pto->smsgData.cs_smsg_net);

    //LogPrintf("SecureMsgSendData() %s.\n", pto->GetAddrName());
    int64_t now = GetTime();

    if (pto->smsgData.lastSeen <= 0)
    {
        // First contact
        LogPrint(BCLog::SMSG, "%s: New node %s, peer id %u.\n", __func__, pto->GetAddrName(), pto->GetId());
        // Send smsgPing once, do nothing until receive 1st smsgPong (then set fEnabled)
        g_connman->PushMessage(pto,
            CNetMsgMaker(INIT_PROTO_VERSION).Make("smsgPing"));

        // Send smsgPong message if received smsgPing from peer while syncing chain
        if (pto->smsgData.lastSeen < 0)
        {
            g_connman->PushMessage(pto,
                CNetMsgMaker(INIT_PROTO_VERSION).Make("smsgPong"));
        };

        pto->smsgData.lastSeen = GetTime();
        return true;
    } else
    if (!pto->smsgData.fEnabled
        || now - pto->smsgData.lastSeen < SMSG_SEND_DELAY
        || now < pto->smsgData.ignoreUntil)
    {
        return true;
    };

    {
        LOCK(cs_smsg);
        std::map<int64_t, SecMsgBucket>::iterator it;

        uint32_t nBuckets = buckets.size();
        if (nBuckets > 0) // no need to send keep alive pkts, coin messages already do that
        {
            std::vector<uint8_t> vchData;
            // should reserve?
            vchData.reserve(4 + nBuckets*16); // timestamp + size + hash

            uint32_t nBucketsShown = 0;
            vchData.resize(4);

            /*
            Get time before loop and after looping through messages set nLastMatched to time before loop.
            This prevents scenario where:
                Loop()
                    message = locked and  thus skipped
                   message become free and nTimeChanged is updated
                End loop

                nLastMatched = GetTime()
                => bucket that became free in loop is now skipped :/

            Scenario 2:
                Same as one but time is updated before

                    bucket nTimeChanged is updated but not unlocked yet
                    now = GetTime()
                    Loop of buckets skips message

                But this is nanoseconds, very unlikely.

             */

            for (it = buckets.begin(); it != buckets.end(); ++it)
            {
                SecMsgBucket &bkt = it->second;

                uint32_t nMessages = bkt.setTokens.size();

                if (bkt.timeChanged < pto->smsgData.lastMatched     // peer was last sent all buckets at time of lastMatched. It should have this bucket
                    || nMessages < 1)                               // this bucket is empty
                    continue;

                uint32_t hash = bkt.hash;

                if (LogAcceptCategory(BCLog::SMSG))
                    LogPrintf("Preparing bucket with hash %d for transfer to node %u. timeChanged=%d > lastMatched=%d\n", hash, pto->GetId(), bkt.timeChanged, pto->smsgData.lastMatched);

                size_t sz = vchData.size();
                try { vchData.resize(vchData.size() + 16); } catch (std::exception& e)
                {
                    LogPrintf("vchData.resize %u threw: %s.\n", vchData.size() + 16, e.what());
                    continue;
                };

                uint8_t *p = &vchData[sz];
                memcpy(p, &it->first, 8);
                memcpy(p+8, &nMessages, 4);
                memcpy(p+12, &hash, 4);

                nBucketsShown++;
                //if (fDebug)
                //    LogPrintf("Sending bucket %d, size %d \n", it->first, it->second.size());
            };

            if (vchData.size() > 4)
            {
                memcpy(&vchData[0], &nBucketsShown, 4);
                LogPrint(BCLog::SMSG, "Sending %d bucket headers.\n", nBucketsShown);

                g_connman->PushMessage(pto,
                    CNetMsgMaker(INIT_PROTO_VERSION).Make("smsgInv", vchData));
            };
        };
    } // cs_smsg

    pto->smsgData.lastSeen = now;
    pto->smsgData.lastMatched = now; //bug fix smsg 3

    return true;
};

static int InsertAddress(CKeyID &hashKey, CPubKey &pubKey, SecMsgDB &addrpkdb)
{
    /*
    Insert key hash and public key to addressdb.

    (+) Called when receiving a message, it will automatically add the public key of the sender to our database so we can reply.

    Should have LOCK(cs_smsg) where db is opened

    returns SecureMessageCodes
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
        return SMSG_PUBKEY_EXISTS;
    };

    if (!addrpkdb.WritePK(hashKey, pubKey))
        return errorN(SMSG_GENERAL_ERROR, "%s: Write pair failed.", __func__);

    return SMSG_NO_ERROR;
};

static int InsertAddress(CKeyID &hashKey, CPubKey &pubKey)
{
    LOCK(cs_smsgDB);
    SecMsgDB addrpkdb;

    if (!addrpkdb.Open("cr+"))
        return SMSG_GENERAL_ERROR;

    return InsertAddress(hashKey, pubKey, addrpkdb);
};

static bool ScanBlock(CSMSG &smsg, const CBlock &block, SecMsgDB &addrpkdb,
    uint32_t &nTransactions, uint32_t &nElements, uint32_t &nPubkeys, uint32_t &nDuplicates)
{
    AssertLockHeld(cs_smsgDB);

    std::string reason;

    // Only scan inputs of standard txns and coinstakes
    for (const auto &tx : block.vtx)
    {
        // Harvest public keys from coinstake txns

        if (!tx->IsParticlVersion()) // skip legacy txns
            continue;

        for (const auto &txin : tx->vin)
        {
            if (txin.IsAnonInput())
                continue;

            if (txin.scriptWitness.stack.size() != 2)
                continue;

            if (txin.scriptWitness.stack[1].size() != 33)
                continue;

            CPubKey pubKey(txin.scriptWitness.stack[1]);

            if (!pubKey.IsValid()
                || !pubKey.IsCompressed())
            {
                LogPrintf("Public key is invalid %s.\n", HexStr(pubKey));
                continue;
            };

            CKeyID addrKey = pubKey.GetID();
            switch (InsertAddress(addrKey, pubKey, addrpkdb))
            {
                case SMSG_NO_ERROR: nPubkeys++; break;          // added key
                case SMSG_PUBKEY_EXISTS: nDuplicates++; break;  // duplicate key
            };

            if (tx->IsCoinStake()) // coinstake inputs are always from the same address/pubkey
                break;
        };

        nTransactions++;

        if (nTransactions % 10000 == 0) // for ScanChainForPublicKeys
        {
            LogPrintf("Scanning transaction no. %u.\n", nTransactions);
        };
    };
    return true;
};


bool CSMSG::ScanBlock(const CBlock &block)
{
    // - scan block for public key addresses

    if (!options.fScanIncoming)
        return true;

    LogPrint(BCLog::SMSG, "%s.\n", __func__);

    uint32_t nTransactions  = 0;
    uint32_t nElements      = 0;
    uint32_t nPubkeys       = 0;
    uint32_t nDuplicates    = 0;

    {
        LOCK(cs_smsgDB);

        SecMsgDB addrpkdb;
        if (!addrpkdb.Open("cw")
            || !addrpkdb.TxnBegin())
            return false;

        smsg::ScanBlock(*this, block, addrpkdb,
            nTransactions, nElements, nPubkeys, nDuplicates);

        addrpkdb.TxnCommit();
    } // cs_smsgDB

    LogPrint(BCLog::SMSG, "Found %u transactions, %u elements, %u new public keys, %u duplicates.\n", nTransactions, nElements, nPubkeys, nDuplicates);

    return true;
};

bool CSMSG::ScanChainForPublicKeys(CBlockIndex *pindexStart)
{
    LogPrintf("Scanning block chain for public keys.\n");
    int64_t nStart = GetTimeMillis();

    LogPrint(BCLog::SMSG, "From height %u.\n", pindexStart->nHeight);

    // Public keys are in txin.scriptSig
    //  matching addresses are in scriptPubKey of txin's referenced output

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

        CBlockIndex *pindex = pindexStart;
        while (pindex)
        {
            nBlocks++;
            CBlock block;
            if (!ReadBlockFromDisk(block, pindex, Params().GetConsensus()))
                LogPrintf("%s: ReadBlockFromDisk failed.\n", __func__);
            else
                smsg::ScanBlock(*this, block, addrpkdb,
                    nTransactions, nInputs, nPubkeys, nDuplicates);

            pindex = chainActive.Next(pindex);
        };

        addrpkdb.TxnCommit();
    } // cs_smsgDB

    LogPrintf("Scanned %u blocks, %u transactions, %u inputs\n", nBlocks, nTransactions, nInputs);
    LogPrintf("Found %u public keys, %u duplicates.\n", nPubkeys, nDuplicates);
    LogPrintf("Took %d ms\n", GetTimeMillis() - nStart);

    return true;
};

bool CSMSG::ScanBlockChain()
{
    TRY_LOCK(cs_main, lockMain);
    if (lockMain)
    {
        CBlockIndex *pindexScan = chainActive.Genesis();
        if (pindexScan == nullptr)
            return error("%s: pindexGenesisBlock not set.", __func__);

        try { // In try to catch errors opening db,
            if (!ScanChainForPublicKeys(pindexScan))
                return false;
        } catch (std::exception &e)
        {
            return error("%s: threw: %s.", __func__, e.what());
        };
    } else
    {
        return error("%s: Could not lock main.", __func__);
    };

    return true;
};

bool CSMSG::ScanBuckets()
{
    LogPrint(BCLog::SMSG, "%s\n", __func__);

    if (!fSecMsgEnabled)
        return error("%s: SMSG is disabled.\n", __func__);

#ifdef ENABLE_WALLET
    if (pwallet && pwallet->IsLocked()
        && addresses.size() > 0)
        return error("%s: Wallet is locked.\n", __func__);
#endif

    int64_t  mStart         = GetTimeMillis();
    int64_t  now            = GetTime();
    uint32_t nFiles         = 0;
    uint32_t nMessages      = 0;
    uint32_t nFoundMessages = 0;

    fs::path pathSmsgDir = GetDataDir() / "smsgstore";
    fs::directory_iterator itend;

    if (!fs::exists(pathSmsgDir)
        || !fs::is_directory(pathSmsgDir))
    {
        LogPrintf("Message store directory does not exist.\n");
        return true; // not an error
    };

    SecureMessage smsg;
    std::vector<uint8_t> vchData;

    for (fs::directory_iterator itd(pathSmsgDir); itd != itend; ++itd)
    {
        if (!fs::is_regular_file(itd->status()))
            continue;

        std::string fileType = itd->path().extension().string();

        if (fileType.compare(".dat") != 0)
            continue;

        std::string fileName = itd->path().filename().string();

        LogPrint(BCLog::SMSG, "Processing file: %s.\n", fileName);
        nFiles++;

        // TODO files must be split if > 2GB
        // time_noFile.dat
        size_t sep = fileName.find_first_of("_");
        if (sep == std::string::npos)
            continue;

        std::string stime = fileName.substr(0, sep);
        int64_t fileTime;
        if (!ParseInt64(stime, &fileTime))
        {
            LogPrintf("%s: ParseInt64 failed %s.\n", __func__, stime);
            continue;
        };

        if (fileTime < now - SMSG_RETENTION)
        {
            LogPrintf("Dropping file %s, expired.\n", fileName);
            try {
                fs::remove(itd->path());
            } catch (const fs::filesystem_error &ex)
            {
                LogPrintf("Error removing bucket file %s, %s.\n", fileName, ex.what());
            };
            continue;
        };

        if (part::endsWith(fileName, "_wl.dat"))
        {
            // ScanBuckets must be run with unlocked wallet (if any receiving keys are wallet keys), remove any redundant _wl files
            LogPrint(BCLog::SMSG, "Removing wallet locked file: %s.\n", fileName);
            try { fs::remove(itd->path());
            } catch (const fs::filesystem_error &ex)
            {
                LogPrintf("Error removing wallet locked file %s.\n", ex.what());
            };
            continue;
        };

        {
            LOCK(cs_smsg);
            FILE *fp;
            errno = 0;
            if (!(fp = fopen(itd->path().string().c_str(), "rb")))
            {
                LogPrintf("Error opening file: %s\n", strerror(errno));
                continue;
            };

            for (;;)
            {
                errno = 0;
                if (fread(smsg.data(), sizeof(uint8_t), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN)
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

                try { vchData.resize(smsg.nPayload); } catch (std::exception &e)
                {
                    LogPrintf("SecureMsgWalletUnlocked(): Could not resize vchData, %u, %s\n", smsg.nPayload, e.what());
                    fclose(fp);
                    return false;
                };

                if (fread(&vchData[0], sizeof(uint8_t), smsg.nPayload, fp) != smsg.nPayload)
                {
                    LogPrintf("fread data failed: %s\n", strerror(errno));
                    break;
                };

                // Don't report to gui,
                int rv = ScanMessage(smsg.data(), &vchData[0], smsg.nPayload, false);

                if (rv == SMSG_NO_ERROR)
                {
                    nFoundMessages++;
                } else
                {
                    // SecureMsgScanMessage failed
                };

                nMessages++;
            };

            fclose(fp);
        } // cs_smsg
    };

    LogPrintf("Processed %u files, scanned %u messages, received %u messages.\n", nFiles, nMessages, nFoundMessages);
    LogPrintf("Took %d ms\n", GetTimeMillis() - mStart);

    return true;
}

int CSMSG::ManageLocalKey(CKeyID &keyId, ChangeType mode)
{
    // TODO: default recv and recvAnon
    {
        LOCK(cs_smsg);

        std::vector<SecMsgAddress>::iterator itFound = addresses.end();
        for (std::vector<SecMsgAddress>::iterator it = addresses.begin(); it != addresses.end(); ++it)
        {
            if (keyId != it->address)
                continue;
            itFound = it;
            break;
        };

        switch(mode)
        {
            case CT_REPLACE:
            case CT_NEW:
                if (itFound == addresses.end())
                {
                    addresses.push_back(SecMsgAddress(keyId, options.fNewAddressRecv, options.fNewAddressAnon));
                } else
                {
                    LogPrint(BCLog::SMSG, "%s: Already have address: %s.\n", __func__, CBitcoinAddress(keyId).ToString());
                    return SMSG_KEY_EXISTS;
                }
                break;
            case CT_DELETED:
                if (itFound != addresses.end())
                {
                    addresses.erase(itFound);
                } else
                {
                    return SMSG_KEY_NOT_EXISTS;
                }
                break;
            default:
                break;
        };
    } // cs_smsg

    return SMSG_NO_ERROR;
};

int CSMSG::WalletUnlocked()
{
#ifdef ENABLE_WALLET
    /*
    When the wallet is unlocked, scan messages received while wallet was locked.
    */
    if (!fSecMsgEnabled || !pwallet)
        return SMSG_WALLET_UNSET;

    LogPrintf("SecureMsgWalletUnlocked()\n");

    if (pwallet->IsLocked())
        return errorN(SMSG_WALLET_LOCKED, "%s: Wallet is locked.", __func__);

    int64_t  now            = GetTime();
    uint32_t nFiles         = 0;
    uint32_t nMessages      = 0;
    uint32_t nFoundMessages = 0;

    fs::path pathSmsgDir = GetDataDir() / "smsgstore";
    fs::directory_iterator itend;

    if (!fs::exists(pathSmsgDir)
        || !fs::is_directory(pathSmsgDir))
    {
        LogPrintf("Message store directory does not exist.\n");
        return SMSG_NO_ERROR; // not an error
    };

    SecureMessage smsg;
    std::vector<uint8_t> vchData;

    for (fs::directory_iterator itd(pathSmsgDir); itd != itend; ++itd)
    {
        if (!fs::is_regular_file(itd->status()))
            continue;

        std::string fileName = itd->path().filename().string();

        if (!part::endsWith(fileName, "_wl.dat"))
            continue;

        LogPrint(BCLog::SMSG, "Processing file: %s.\n", fileName);

        nFiles++;

        // TODO files must be split if > 2GB
        // time_noFile_wl.dat
        size_t sep = fileName.find_first_of("_");
        if (sep == std::string::npos)
            continue;

        std::string stime = fileName.substr(0, sep);
        int64_t fileTime;
        if (!ParseInt64(stime, &fileTime))
        {
            LogPrintf("%s: ParseInt64 failed %s.\n", __func__, stime);
            continue;
        };

        if (fileTime < now - SMSG_RETENTION)
        {
            LogPrintf("Dropping wallet locked file %s, expired.\n", fileName);
            try {
                fs::remove(itd->path());
            } catch (const fs::filesystem_error &ex)
            {
                return errorN(SMSG_GENERAL_ERROR, "%s: Could not remove file %s - %s.", __func__, fileName, ex.what());
            };
            continue;
        };

        {
            LOCK(cs_smsg);
            FILE *fp;
            errno = 0;
            if (!(fp = fopen(itd->path().string().c_str(), "rb")))
            {
                LogPrintf("Error opening file: %s\n", strerror(errno));
                continue;
            };

            for (;;)
            {
                errno = 0;
                if (fread(smsg.data(), sizeof(uint8_t), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN)
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

                try { vchData.resize(smsg.nPayload); } catch (std::exception &e)
                {
                    LogPrintf("%s: Could not resize vchData, %u, %s\n", __func__, smsg.nPayload, e.what());
                    fclose(fp);
                    return SMSG_GENERAL_ERROR;
                };

                if (fread(&vchData[0], sizeof(uint8_t), smsg.nPayload, fp) != smsg.nPayload)
                {
                    LogPrintf("fread data failed: %s\n", strerror(errno));
                    break;
                };

                // Don't report to gui,
                int rv = ScanMessage(smsg.data(), &vchData[0], smsg.nPayload, false);

                if (rv == 0)
                {
                    nFoundMessages++;
                } else
                if (rv != 0)
                {
                    // SecureMsgScanMessage failed
                };

                nMessages++;
            };

            fclose(fp);

            // Remove wl file when scanned
            try {
                fs::remove(itd->path());
            } catch (const fs::filesystem_error &ex)
            {
                return errorN(SMSG_GENERAL_ERROR, "%s: Could not remove file %s - %s.", __func__, fileName, ex.what());
            };
        } // cs_smsg
    };

    LogPrintf("Processed %u files, scanned %u messages, received %u messages.\n", nFiles, nMessages, nFoundMessages);

    // Notify gui
    NotifySecMsgWalletUnlocked();
#endif
    return SMSG_NO_ERROR;
};

int CSMSG::WalletKeyChanged(CKeyID &keyId, const std::string &sLabel, ChangeType mode)
{
    /*
    SecureMsgWalletKeyChanged():
    When a key changes in the wallet, this function should be called to update the addresses vector.

    mode:
        CT_NEW : a new key was added
        CT_DELETED : delete an existing key from vector.
    */

    if (!fSecMsgEnabled)
        return SMSG_DISABLED;

    LogPrintf("%s\n", __func__);

    if (!gArgs.GetBoolArg("-smsgsaddnewkeys", false))
    {
        LogPrint(BCLog::SMSG, "%s smsgsaddnewkeys option is disabled.\n", __func__);
        return SMSG_GENERAL_ERROR;
    };

    return ManageLocalKey(keyId, mode);
};

int CSMSG::ScanMessage(const uint8_t *pHeader, const uint8_t *pPayload, uint32_t nPayload, bool reportToGui)
{
    LogPrint(BCLog::SMSG, "%s\n", __func__);
    /*
    Check if message belongs to this node.
    If so add to inbox db.

    if !reportToGui don't fire NotifySecMsgInboxChanged
     - loads messages received when wallet locked in bulk.

    returns SecureMessageCodes
    */

    bool fOwnMessage = false;
    MessageData msg; // placeholder
    CKeyID addressTo;
    for (auto &p : smsgModule.keyStore.mapKeys)
    {
        auto &address = p.first;
        auto &key = p.second;

        if (!(key.nFlags & SMK_RECEIVE_ON))
            continue;

        if (!(key.nFlags & SMK_RECEIVE_ANON))
        {
            // Have to do full decrypt to see address from
            if (Decrypt(false, key.key, address, pHeader, pPayload, nPayload, msg) == 0)
            {
                if (LogAcceptCategory(BCLog::SMSG))
                    LogPrintf("Decrypted message with %s.\n", CBitcoinAddress(addressTo).ToString());
                if (msg.sFromAddress.compare("anon") != 0)
                    fOwnMessage = true;
                addressTo = address;
                break;
            };
        } else
        {
            if (Decrypt(true, key.key, address, pHeader, pPayload, nPayload, msg) == 0)
            {
                if (LogAcceptCategory(BCLog::SMSG))
                    LogPrintf("Decrypted message with %s.\n", CBitcoinAddress(addressTo).ToString());
                fOwnMessage = true;
                addressTo = address;
                break;
            };
        };
    };

    if (!fOwnMessage)
    {
#ifdef ENABLE_WALLET
        if (!pwallet)
        {
            LogPrint(BCLog::SMSG, "%s: Wallet is not set.\n", __func__);
            return SMSG_NO_ERROR;
        };

        if (pwallet->IsLocked())
        {
            LogPrint(BCLog::SMSG, "%s: Wallet is locked, storing message to scan later.\n", __func__);

            if (addresses.size() > 0) // Only save unscanned if there are addresses
            {
                int rv;
                if ((rv = StoreUnscanned(pHeader, pPayload, nPayload)) != 0)
                    return SMSG_GENERAL_ERROR;
            };

            return SMSG_WALLET_LOCKED;
        };

        for (std::vector<SecMsgAddress>::iterator it = addresses.begin(); it != addresses.end(); ++it)
        {
            if (!it->fReceiveEnabled)
                continue;

            addressTo = it->address;

            CKey keyDest;
            if (!pwallet->GetKey(addressTo, keyDest))
                continue;

            if (!it->fReceiveAnon)
            {
                // Have to do full decrypt to see address from
                if (Decrypt(false, keyDest, addressTo, pHeader, pPayload, nPayload, msg) == 0)
                {
                    if (LogAcceptCategory(BCLog::SMSG))
                        LogPrintf("Decrypted message with %s.\n", CBitcoinAddress(addressTo).ToString());
                    if (msg.sFromAddress.compare("anon") != 0)
                        fOwnMessage = true;
                    break;
                };
            } else
            {
                if (Decrypt(true, keyDest, addressTo, pHeader, pPayload, nPayload, msg) == 0)
                {
                    if (LogAcceptCategory(BCLog::SMSG))
                        LogPrintf("Decrypted message with %s.\n", CBitcoinAddress(addressTo).ToString());
                    fOwnMessage = true;
                    break;
                };
            };
        };
#endif
    };

    if (fOwnMessage)
    {
        // Save to inbox
        SecureMessage *psmsg = (SecureMessage*) pHeader;

        uint160 hash;
        HashMsg(*psmsg, pPayload, nPayload-(psmsg->IsPaidVersion() ? 32 : 0), hash);

        std::string sPrefix("im");
        uint8_t chKey[30];
        int64_t timestamp_be = bswap_64(psmsg->timestamp);
        memcpy(&chKey[0], sPrefix.data(), 2);
        memcpy(&chKey[2], &timestamp_be, 8);
        memcpy(&chKey[10], hash.begin(), 20);

        SecMsgStored smsgInbox;
        smsgInbox.timeReceived  = GetTime();
        smsgInbox.status        = (SMSG_MASK_UNREAD) & 0xFF;
        smsgInbox.addrTo        = addressTo;

        try { smsgInbox.vchMessage.resize(SMSG_HDR_LEN + nPayload); } catch (std::exception &e)
        {
            return errorN(SMSG_ALLOCATE_FAILED, "%s: Could not resize vchData, %u, %s.", __func__, SMSG_HDR_LEN + nPayload, e.what());
        };
        memcpy(&smsgInbox.vchMessage[0], pHeader, SMSG_HDR_LEN);
        memcpy(&smsgInbox.vchMessage[SMSG_HDR_LEN], pPayload, nPayload);

        bool fExisted = false;
        {
            LOCK(cs_smsgDB);
            SecMsgDB dbInbox;

            if (dbInbox.Open("cw"))
            {
                if (dbInbox.ExistsSmesg(chKey))
                {
                    fExisted = true;
                    LogPrint(BCLog::SMSG, "Message already exists in inbox db.\n");
                } else
                {
                    dbInbox.WriteSmesg(chKey, smsgInbox);
                    if (reportToGui)
                        NotifySecMsgInboxChanged(smsgInbox);
                    LogPrintf("SecureMsg saved to inbox, received with %s.\n", CBitcoinAddress(addressTo).ToString());
                };
            };
        } // cs_smsgDB

        if (!fExisted)
        {
            // notify an external script when a message comes in
            std::string strCmd = gArgs.GetArg("-smsgnotify", "");

            //TODO: Format message
            if (!strCmd.empty())
            {
                boost::replace_all(strCmd, "%s", CBitcoinAddress(addressTo).ToString());
                std::thread t(runCommand, strCmd);
                t.detach(); // thread runs free
            };

            GetMainSignals().NewSecureMessage(psmsg, hash);
        };
    };

    return SMSG_NO_ERROR;
};

int CSMSG::GetLocalKey(const CKeyID &ckid, CPubKey &cpkOut)
{
    LogPrint(BCLog::SMSG, "%s\n", __func__);

    if (keyStore.GetPubKey(ckid, cpkOut))
        return SMSG_NO_ERROR;

#ifdef ENABLE_WALLET
    if (!pwallet)
        return errorN(SMSG_WALLET_UNSET, "%s: Wallet disabled.", __func__);

    if (!pwallet->GetPubKey(ckid, cpkOut))
        return SMSG_WALLET_NO_PUBKEY;

    if (!cpkOut.IsValid()
        || !cpkOut.IsCompressed())
    {
        return errorN(SMSG_INVALID_PUBKEY, "%s: Public key is invalid %s.", __func__, HexStr(cpkOut));
    };
    return SMSG_NO_ERROR;
#endif

    return SMSG_WALLET_NO_PUBKEY;
};

int CSMSG::GetLocalPublicKey(const std::string &strAddress, std::string &strPublicKey)
{
    // returns SecureMessageCodes

    CBitcoinAddress address;
    CKeyID keyID;
    if (!address.SetString(strAddress) || !address.GetKeyID(keyID))
        return SMSG_INVALID_ADDRESS;

    int rv;
    CPubKey pubKey;
    if ((rv = GetLocalKey(keyID, pubKey)) != 0)
        return rv;

    strPublicKey = EncodeBase58(pubKey.begin(), pubKey.end());
    return SMSG_NO_ERROR;
};

int CSMSG::GetStoredKey(const CKeyID &ckid, CPubKey &cpkOut)
{
    /* returns SecureMessageCodes
    */
    LogPrint(BCLog::SMSG, "%s\n", __func__);

    {
        LOCK(cs_smsgDB);
        SecMsgDB addrpkdb;

        if (!addrpkdb.Open("r"))
            return SMSG_GENERAL_ERROR;

        if (!addrpkdb.ReadPK(ckid, cpkOut))
        {
            //LogPrintf("addrpkdb.Read failed: %s.\n", coinAddress.ToString());
            return SMSG_PUBKEY_NOT_EXISTS;
        };
    } // cs_smsgDB

    return SMSG_NO_ERROR;
};

int CSMSG::AddAddress(std::string &address, std::string &publicKey)
{
    /*
    Add address and matching public key to the database
    address and publicKey are in base58

    returns SecureMessageCodes
    */

    CBitcoinAddress coinAddress(address);
    if (!coinAddress.IsValid())
        return errorN(SMSG_INVALID_ADDRESS, "%s - Address is not valid: %s.", __func__, address);

    CKeyID idk;
    if (!coinAddress.GetKeyID(idk))
        return errorN(SMSG_INVALID_ADDRESS, "%s - coinAddress.GetKeyID failed: %s.", __func__, address);

    std::vector<uint8_t> vchTest;

    if (IsHex(publicKey))
    {
       vchTest = ParseHex(publicKey);
    } else
    {
        DecodeBase58(publicKey, vchTest);
    };

    CPubKey pubKey(vchTest);
    if (!pubKey.IsValid())
        return errorN(SMSG_INVALID_PUBKEY, "%s - Invalid PubKey.", __func__);

    // Check that public key matches address hash
    CKeyID keyIDT = pubKey.GetID();
    if (idk != keyIDT)
        return errorN(SMSG_PUBKEY_MISMATCH, "%s - Public key does not hash to address %s.", __func__, address);

    return InsertAddress(idk, pubKey);
};

int CSMSG::AddLocalAddress(const std::string &sAddress)
{
#ifdef ENABLE_WALLET
    LogPrintf("%s: %s\n", __func__, sAddress);

    if (!pwallet)
        return errorN(SMSG_WALLET_UNSET, "%s: Wallet disabled.", __func__);

    CBitcoinAddress addr(sAddress);
    if (!addr.IsValid(CChainParams::PUBKEY_ADDRESS))
        return errorN(SMSG_INVALID_ADDRESS, "%s - Address is not valid: %s.", __func__, sAddress);

    CKeyID idk;
    if (!addr.GetKeyID(idk))
        return errorN(SMSG_INVALID_ADDRESS, "%s - GetKeyID failed: %s.", __func__, sAddress);

    if (!pwallet->HaveKey(idk))
        return errorN(SMSG_WALLET_NO_KEY, "%s: Key to %s not found in wallet.", __func__, sAddress);

    return ManageLocalKey(idk, CT_NEW);
#else
    return SMSG_WALLET_UNSET;
#endif
};

int CSMSG::ImportPrivkey(const CBitcoinSecret &vchSecret, const std::string &sLabel)
{
    SecMsgKey key;
    key.key = vchSecret.GetKey();
    key.sLabel = sLabel;
    CKeyID idk = key.key.GetPubKey().GetID();
    key.nFlags |= SMK_RECEIVE_ON;
    key.nFlags |= SMK_RECEIVE_ANON;

    LOCK(cs_smsgDB);

    SecMsgDB db;
    if (!db.Open("cr+"))
        return SMSG_GENERAL_ERROR;

    if (!db.WriteKey(idk, key))
        return errorN(SMSG_GENERAL_ERROR, "%s - WriteKey failed.", __func__);

    keyStore.AddKey(idk, key);

    return SMSG_NO_ERROR;
};

bool CSMSG::SetWalletAddressOption(const CKeyID &idk, std::string sOption, bool fValue)
{
    std::vector<smsg::SecMsgAddress>::iterator it;
    for (it = addresses.begin(); it != addresses.end(); ++it)
    {
        if (idk != it->address)
            continue;
        break;
    };

    if (it == addresses.end())
        return false;

    if (sOption == "anon")
        it->fReceiveAnon = fValue;
    else
    if (sOption == "receive")
        it->fReceiveEnabled = fValue;
    else
        return error("%s: Unknown option %s.\n", __func__, sOption);

    return true;
};

bool CSMSG::SetSmsgAddressOption(const CKeyID &idk, std::string sOption, bool fValue)
{
    LOCK(cs_smsgDB);

    SecMsgDB db;
    if (!db.Open("cr+"))
        return error("%s: Failed to open db.\n", __func__);

    SecMsgKey key;
    if (!db.ReadKey(idk, key))
        return false;

    if (sOption == "anon")
    {
        if (fValue)
            key.nFlags |= SMK_RECEIVE_ANON;
        else
            key.nFlags &= ~SMK_RECEIVE_ANON;
    } else
    if (sOption == "receive")
    {
        if (fValue)
            key.nFlags |= SMK_RECEIVE_ON;
        else
            key.nFlags &= ~SMK_RECEIVE_ON;
    } else
    {
        return error("%s: Unknown option %s.\n", __func__, sOption);
    };

    if (!db.WriteKey(idk, key))
        return false;

    if (key.nFlags & SMK_RECEIVE_ON)
        keyStore.AddKey(idk, key);
    else
        keyStore.EraseKey(idk);

    return true;
};

int CSMSG::ReadSmsgKey(const CKeyID &idk, CKey &key)
{
    LOCK(cs_smsgDB);

    SecMsgDB db;
    if (!db.Open("cr+"))
        return SMSG_GENERAL_ERROR;

    SecMsgKey smk;
    if (!db.ReadKey(idk, smk))
        return SMSG_KEY_NOT_EXISTS;

    key = smk.key;

    return SMSG_NO_ERROR;
};

int CSMSG::Retrieve(const SecMsgToken &token, std::vector<uint8_t> &vchData)
{
    LogPrint(BCLog::SMSG, "%s: %d.\n", __func__, token.timestamp);
    AssertLockHeld(cs_smsg);

    fs::path pathSmsgDir = GetDataDir() / "smsgstore";

    int64_t bucket = token.timestamp - (token.timestamp % SMSG_BUCKET_LEN);
    std::string fileName = std::to_string(bucket) + "_01.dat";
    fs::path fullpath = pathSmsgDir / fileName;

    FILE *fp;
    errno = 0;
    if (!(fp = fopen(fullpath.string().c_str(), "rb")))
        return errorN(SMSG_GENERAL_ERROR, "%s - Can't open file: %s\nPath %s.", __func__, strerror(errno), fullpath.string());

    errno = 0;
    if (fseek(fp, token.offset, SEEK_SET) != 0)
    {
        fclose(fp);
        return errorN(SMSG_GENERAL_ERROR, "%s - fseek, strerror: %s.", __func__, strerror(errno));
    };

    SecureMessage smsg;
    errno = 0;
    if (fread(smsg.data(), sizeof(uint8_t), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN)
    {
        fclose(fp);
        return errorN(SMSG_GENERAL_ERROR, "%s - read header failed, strerror: %s.", __func__, strerror(errno));
    };

    try {vchData.resize(SMSG_HDR_LEN + smsg.nPayload);} catch (std::exception &e)
    {
        fclose(fp);
        return errorN(SMSG_ALLOCATE_FAILED, "%s - Could not resize vchData, %u, %s.", __func__, SMSG_HDR_LEN + smsg.nPayload, e.what());
    };

    memcpy(vchData.data(), smsg.data(), SMSG_HDR_LEN);
    errno = 0;
    if (fread(&vchData[SMSG_HDR_LEN], sizeof(uint8_t), smsg.nPayload, fp) != smsg.nPayload)
    {
        fclose(fp);
        return errorN(SMSG_GENERAL_ERROR, "%s - fread data failed: %s. Wanted %u bytes.", __func__, strerror(errno), smsg.nPayload);
    };

    fclose(fp);
    return SMSG_NO_ERROR;
};

int CSMSG::Remove(const SecMsgToken &token)
{
    LogPrint(BCLog::SMSG, "%s: %d.\n", __func__, token.timestamp);
    AssertLockHeld(cs_smsg);

    fs::path pathSmsgDir = GetDataDir() / "smsgstore";

    int64_t bucket = token.timestamp - (token.timestamp % SMSG_BUCKET_LEN);
    std::string fileName = std::to_string(bucket) + "_01.dat";
    fs::path fullpath = pathSmsgDir / fileName;

    FILE *fp;
    errno = 0;
    if (!(fp = fopen(fullpath.string().c_str(), "rb+")))
        return errorN(SMSG_GENERAL_ERROR, "%s - Can't open file: %s\nPath %s.", __func__, strerror(errno), fullpath.string());

    errno = 0;
    if (fseek(fp, token.offset, SEEK_SET) != 0)
    {
        fclose(fp);
        return errorN(SMSG_GENERAL_ERROR, "%s - fseek, strerror: %s.", __func__, strerror(errno));
    };

    SecureMessage smsg;
    errno = 0;
    if (fread(smsg.data(), sizeof(uint8_t), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN)
    {
        fclose(fp);
        return errorN(SMSG_GENERAL_ERROR, "%s - read header failed, strerror: %s.", __func__, strerror(errno));
    };

    uint8_t z = 0;
    if (0 != fseek(fp, token.offset + 4, SEEK_SET)
        || 2 != fwrite(&z, 1, 2, fp))
    {
        fclose(fp);
        return errorN(SMSG_GENERAL_ERROR, "%s - zero version strerror: %s.", __func__, strerror(errno));
    };

    if (fseek(fp, token.offset + SMSG_HDR_LEN + 8, SEEK_SET) != 0)
    {
        fclose(fp);
        return errorN(SMSG_GENERAL_ERROR, "%s - fseek, strerror: %s.", __func__, strerror(errno));
    };

    size_t zlen = smsg.nPayload - 8;
    if (smsg.nPayload <= 8 ||  zlen != fwrite(&z, 1, zlen, fp))
    {
        fclose(fp);
        return errorN(SMSG_GENERAL_ERROR, "%s - fwrite, zlen %d, strerror: %s.", __func__, zlen, strerror(errno));
    };

    fclose(fp);
    return SMSG_NO_ERROR;
};

int CSMSG::Receive(CNode *pfrom, std::vector<uint8_t> &vchData)
{
    LogPrint(BCLog::SMSG, "%s\n", __func__);

    if (vchData.size() < 12) // nBunch4 + timestamp8
        return errorN(SMSG_GENERAL_ERROR, "%s - Not enough data.", __func__);

    uint32_t nBunch;
    int64_t bktTime;

    memcpy(&nBunch, &vchData[0], 4);
    memcpy(&bktTime, &vchData[4], 8);

    // Check bktTime ()
    //  Bucket may not exist yet - will be created when messages are added
    int64_t now = GetAdjustedTime();
    if (bktTime > now + SMSG_TIME_LEEWAY)
    {
        LogPrint(BCLog::SMSG, "bktTime > now.\n");
        // misbehave?
        return SMSG_GENERAL_ERROR;
    } else
    if (bktTime < now - SMSG_RETENTION)
    {
        LogPrint(BCLog::SMSG, "bktTime < now - SMSG_RETENTION.\n");
        // misbehave?
        return SMSG_GENERAL_ERROR;
    };

    std::map<int64_t, SecMsgBucket>::iterator itb;

    if (nBunch == 0 || nBunch > 500)
    {
        LogPrintf("Error: Invalid no. messages received in bunch %u, for bucket %d.\n", nBunch, bktTime);
        Misbehaving(pfrom->GetId(), 1);

        {
            LOCK(cs_smsg);
            // Release lock on bucket if it exists
            itb = buckets.find(bktTime);
            if (itb != buckets.end())
                itb->second.nLockCount = 0;
        } // cs_smsg
        return SMSG_GENERAL_ERROR;
    };

    uint32_t n = 12;

    for (uint32_t i = 0; i < nBunch; ++i)
    {
        if (vchData.size() - n < SMSG_HDR_LEN)
        {
            LogPrintf("Error: not enough data sent, n = %u.\n", n);
            break;
        };

        SecureMessage *psmsg = (SecureMessage*) &vchData[n];

        int rv;
        if ((rv = Validate(&vchData[n], &vchData[n + SMSG_HDR_LEN], psmsg->nPayload)) != 0)
        {
            // Message dropped
            if (rv == SMSG_INVALID_HASH) // Invalid proof of work
            {
                Misbehaving(pfrom->GetId(), 10);
            } else
            if (rv == SMSG_FUND_FAILED) // Bad funding tx
            {
                Misbehaving(pfrom->GetId(), 10);
            } else
            {
                Misbehaving(pfrom->GetId(), 1);
            };
            continue;
        };

        {
            LOCK(cs_smsg);
            // Store message, but don't hash bucket
            if (Store(&vchData[n], &vchData[n + SMSG_HDR_LEN], psmsg->nPayload, false) != 0)
            {
                // Message dropped
                break;
            };

            if (ScanMessage(&vchData[n], &vchData[n + SMSG_HDR_LEN], psmsg->nPayload, true) != 0)
            {
                // message recipient is not this node (or failed)
            };
        } // cs_smsg

        n += SMSG_HDR_LEN + psmsg->nPayload;
    };

    {
        LOCK(cs_smsg);
        // If messages have been added, bucket must exist now
        itb = buckets.find(bktTime);
        if (itb == buckets.end())
        {
            LogPrint(BCLog::SMSG, "Don't have bucket %d.\n", bktTime);
            return SMSG_GENERAL_ERROR;
        };

        itb->second.nLockCount  = 0; // This node has received data from peer, release lock
        itb->second.nLockPeerId = 0;
        itb->second.hashBucket();
    } // cs_smsg

    return SMSG_NO_ERROR;
};

int CSMSG::CheckPurged(const SecureMessage *psmsg, const uint8_t *pPayload)
{
    if (setPurgedTimestamps.find(psmsg->timestamp) != setPurgedTimestamps.end())
        return SMSG_NO_ERROR;

    std::vector<uint8_t> vMsgId = GetMsgID(psmsg, pPayload);

    uint8_t chKey[30];
    chKey[0] = 'p';
    chKey[1] = 'm';
    memcpy(chKey+2, vMsgId.data(), 28);

    LOCK2(cs_smsg, cs_smsgDB);

    SecMsgDB db;
    if (!db.Open("cr+"))
        return SMSG_GENERAL_ERROR;

    SecMsgPurged purged;
    if (db.ReadPurged(chKey, purged))
    {
        LogPrint(BCLog::SMSG, "%s Found purged %s\n", __func__, HexStr(vMsgId));

        // Add sample to purged token
        memcpy(purged.sample, pPayload, 8);
        setPurged.insert(purged);

        return SMSG_PURGED_MSG;
    };

    return SMSG_NO_ERROR;
};

int CSMSG::StoreUnscanned(const uint8_t *pHeader, const uint8_t *pPayload, uint32_t nPayload)
{
    /*
    When the wallet is locked a copy of each received message is stored to be scanned later if wallet is unlocked
    */

    LogPrint(BCLog::SMSG, "%s\n", __func__);

    if (!pHeader
        || !pPayload)
    {
        return errorN(SMSG_GENERAL_ERROR, "%s - Null pointer to header or payload.", __func__);
    };

    SecureMessage *psmsg = (SecureMessage*) pHeader;

    if (SMSG_NO_ERROR != CheckPurged(psmsg, pPayload))
        return errorN(SMSG_PURGED_MSG, "%s: Purged message.", __func__);

    fs::path pathSmsgDir;
    try {
        pathSmsgDir = GetDataDir() / "smsgstore";
        fs::create_directory(pathSmsgDir);
    } catch (const fs::filesystem_error &ex)
    {
        return errorN(SMSG_GENERAL_ERROR, "%s - Failed to create directory %s - %s.", __func__, pathSmsgDir.string(), ex.what());
    };

    int64_t now = GetAdjustedTime();
    if (psmsg->timestamp > now + SMSG_TIME_LEEWAY)
        return errorN(SMSG_GENERAL_ERROR, "%s: Message > now.", __func__);
    if (psmsg->timestamp < now - SMSG_RETENTION)
        return errorN(SMSG_GENERAL_ERROR, "%s: Message < SMSG_RETENTION.", __func__);

    int64_t bucket = psmsg->timestamp - (psmsg->timestamp % SMSG_BUCKET_LEN);

    std::string fileName = std::to_string(bucket) + "_01_wl.dat";
    fs::path fullpath = pathSmsgDir / fileName;

    FILE *fp;
    errno = 0;
    if (!(fp = fopen(fullpath.string().c_str(), "ab")))
    {
        return errorN(SMSG_GENERAL_ERROR, "%s - Can't open file, strerror: %s.", __func__, strerror(errno));
    };

    if (fwrite(pHeader, sizeof(uint8_t), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN
        || fwrite(pPayload, sizeof(uint8_t), nPayload, fp) != nPayload)
    {
        fclose(fp);
        return errorN(SMSG_GENERAL_ERROR, "%s - fwrite failed, strerror: %s.", __func__, strerror(errno));
    };

    fclose(fp);
    return SMSG_NO_ERROR;
};


int CSMSG::Store(const uint8_t *pHeader, const uint8_t *pPayload, uint32_t nPayload, bool fHashBucket)
{
    LogPrint(BCLog::SMSG, "%s\n", __func__);
    AssertLockHeld(cs_smsg);

    if (!pHeader || !pPayload)
        return errorN(SMSG_GENERAL_ERROR, "Null pointer to header or payload.");

    SecureMessage *psmsg = (SecureMessage*) pHeader;

    if (SMSG_NO_ERROR != CheckPurged(psmsg, pPayload))
        return errorN(SMSG_PURGED_MSG, "%s: Purged message.", __func__);

    long int ofs;
    fs::path pathSmsgDir;
    try {
        pathSmsgDir = GetDataDir() / "smsgstore";
        fs::create_directory(pathSmsgDir);
    } catch (const fs::filesystem_error &ex)
    {
        return errorN(SMSG_GENERAL_ERROR, "Failed to create directory %s - %s.", pathSmsgDir.string(), ex.what());
    };

    int64_t now = GetAdjustedTime();
    if (psmsg->timestamp > now + SMSG_TIME_LEEWAY)
        return errorN(SMSG_GENERAL_ERROR, "%s: Message > now.", __func__);
    if (psmsg->timestamp < now - SMSG_RETENTION)
        return errorN(SMSG_GENERAL_ERROR, "%s: Message < SMSG_RETENTION.", __func__);

    int64_t bucketTime = psmsg->timestamp - (psmsg->timestamp % SMSG_BUCKET_LEN);

    uint32_t nDaysToLive = psmsg->version[0] < 3 ? 2 : psmsg->nonce[0];
    SecMsgToken token(psmsg->timestamp, pPayload, nPayload, 0, nDaysToLive);

    SecMsgBucket &bucket = buckets[bucketTime];
    std::set<SecMsgToken> &tokenSet = bucket.setTokens;
    std::set<SecMsgToken>::iterator it;
    it = tokenSet.find(token);
    if (it != tokenSet.end())
    {
        LogPrint(BCLog::SMSG, "Already have message.\n");

        if (LogAcceptCategory(BCLog::SMSG))
        {
            LogPrintf("bucketTime: %d\n", bucketTime);
            LogPrintf("Message token: %s, nPayload %u\n", token.ToString(), nPayload);
        };
        return SMSG_GENERAL_ERROR;
    };

    std::string fileName = std::to_string(bucketTime) + "_01.dat";
    fs::path fullpath = pathSmsgDir / fileName;

    FILE *fp;
    errno = 0;
    if (!(fp = fopen(fullpath.string().c_str(), "ab")))
        return errorN(SMSG_GENERAL_ERROR, "fopen failed: %s.", strerror(errno));

    // On windows ftell will always return 0 after fopen(ab), call fseek to set.
    errno = 0;
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        return errorN(SMSG_GENERAL_ERROR, "fseek failed: %s.", strerror(errno));
    };

    ofs = ftell(fp);

    if (fwrite(pHeader,  sizeof(uint8_t), SMSG_HDR_LEN, fp) != (size_t)SMSG_HDR_LEN
     || fwrite(pPayload, sizeof(uint8_t),     nPayload, fp) != nPayload)
    {
        fclose(fp);
        return errorN(SMSG_GENERAL_ERROR, "fwrite failed: %s.", strerror(errno));
    };

    fclose(fp);

    token.offset = ofs;

    tokenSet.insert(token);

    if (nDaysToLive > 0 && (bucket.nLeastTTL == 0 || nDaysToLive < bucket.nLeastTTL))
        bucket.nLeastTTL = nDaysToLive;

    if (fHashBucket)
        bucket.hashBucket();

    LogPrint(BCLog::SMSG, "SecureMsg added to bucket %d.\n", bucketTime);

    return SMSG_NO_ERROR;
};

int CSMSG::Store(const SecureMessage &smsg, bool fHashBucket)
{
    return Store(smsg.data(), smsg.pPayload, smsg.nPayload, fHashBucket);
};

int CSMSG::Purge(std::vector<uint8_t> &vMsgId, std::string &sError)
{
    LogPrint(BCLog::SMSG, "%s %s\n", __func__, HexStr(vMsgId));

    LOCK(cs_smsg);
    LOCK(cs_smsgDB);
    SecMsgDB db;
    if (!db.Open("cw"))
        return SMSG_GENERAL_ERROR;
    int64_t now = GetTime();
    int64_t msgtime;
    memcpy(&msgtime, vMsgId.data(), 8);
    msgtime = bswap_64(msgtime);
    SecMsgPurged purged(msgtime, now);

    uint8_t chKey[30];
    chKey[0] = 'i';
    chKey[1] = 'm';
    memcpy(chKey+2, vMsgId.data(), 28);
    db.EraseSmesg(chKey);

    // Find in buckets
    int64_t bucketTime = msgtime - (msgtime % SMSG_BUCKET_LEN);

    SecMsgBucket &bucket = buckets[bucketTime];
    std::set<SecMsgToken> &tokenSet = bucket.setTokens;

    std::vector<uint8_t> vchOne;
    for (auto it = tokenSet.begin(); it != tokenSet.end(); ++it)
    {
        if (it->timestamp != msgtime)
            continue;

        if (Retrieve(*it, vchOne) != SMSG_NO_ERROR)
        {
            LogPrintf("%s: Retrieve failed, msgid: %s\n", __func__, HexStr(vMsgId));
            continue;
        };

        const SecureMessage *psmsg = (SecureMessage*) vchOne.data();
        if (GetMsgID(psmsg, vchOne.data() + SMSG_HDR_LEN) != vMsgId)
            continue;

        if (Remove(*it) != SMSG_NO_ERROR)
        {
            LogPrintf("%s: Remove failed, msgid: %s\n", __func__, HexStr(vMsgId));
            break;
        };
        memcpy(purged.sample, vchOne.data() + SMSG_HDR_LEN, 8);
        it->ttl = 0;
        LogPrint(BCLog::SMSG, "Purged message %s in bucket %d\n", it->ToString(), bucketTime);
        memcpy(purged.sample, it->sample, 8);

        break;
    };

    chKey[0] = 'p';
    db.WritePurged(chKey, purged);

    setPurged.insert(purged);
    setPurgedTimestamps.insert(purged.timestamp); // So network sync can prefilter on timestamp before checking for purged msgid in db


    return SMSG_NO_ERROR;
};

int CSMSG::Validate(const uint8_t *pHeader, const uint8_t *pPayload, uint32_t nPayload)
{
    // return SecureMessageCodes
    SecureMessage *psmsg = (SecureMessage*) pHeader;

    if (psmsg->IsPaidVersion())
    {
        if (nPayload > SMSG_MAX_MSG_BYTES_PAID)
            return SMSG_PAYLOAD_OVER_SIZE;
    } else
    if (nPayload > SMSG_MAX_MSG_WORST)
    {
        return SMSG_PAYLOAD_OVER_SIZE;
    };

    int64_t now = GetAdjustedTime();
    if (psmsg->timestamp > now + SMSG_TIME_LEEWAY)
    {
        LogPrint(BCLog::SMSG, "Time in future %d.\n", psmsg->timestamp);
        return SMSG_TIME_IN_FUTURE;
    };

    if (psmsg->version[0] == 3)
    {
        if (Params().GetConsensus().nPaidSmsgTime > now)
        {
            LogPrintf("%s: Paid SMSG not yet active on mainnet.\n", __func__);
            return SMSG_GENERAL_ERROR;
        };

        size_t nDaysRetention = psmsg->nonce[0];
        int64_t ttl = SMSGGetSecondsInDay() * nDaysRetention;
        if (ttl < SMSGGetSecondsInDay() || ttl > SMSG_MAX_PAID_TTL)
        {
            LogPrint(BCLog::SMSG, "TTL out of range %d.\n", ttl);
            return SMSG_GENERAL_ERROR;
        };
        if (now > psmsg->timestamp + ttl)
        {
            LogPrint(BCLog::SMSG, "Time expired %d, ttl %d.\n", psmsg->timestamp, ttl);
            return SMSG_TIME_EXPIRED;
        };

        size_t nMsgBytes = SMSG_HDR_LEN + psmsg->nPayload;
        int64_t nExpectFee = ((nMsgFeePerKPerDay * nMsgBytes) / 1000) * nDaysRetention;

        uint256 txid;
        uint160 msgId;
        if (0 != HashMsg(*psmsg, pPayload, psmsg->nPayload-32, msgId)
            || !GetFundingTxid(pPayload, psmsg->nPayload, txid))
        {
            LogPrintf("%s: Get msgID or Txn Hash failed.\n", __func__);
            return SMSG_GENERAL_ERROR;
        };

        CTransactionRef txOut;
        uint256 hashBlock;
        {
            LOCK(cs_main);
            if (!GetTransaction(txid, txOut, Params().GetConsensus(), hashBlock) || hashBlock.IsNull())
                return errorN(SMSG_GENERAL_ERROR, "%s: Transaction %s not found for message %s.\n", __func__, txid.ToString(), msgId.ToString());

            if (txOut->IsCoinStake())
                return errorN(SMSG_GENERAL_ERROR, "%s: Transaction %s for message %s, is coinstake.\n", __func__, txid.ToString(), msgId.ToString());

            int blockDepth = -1;
            BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
            if (mi != mapBlockIndex.end())
            {
                CBlockIndex *pindex = mi->second;
                if (pindex && chainActive.Contains(pindex))
                    blockDepth = chainActive.Height() - pindex->nHeight + 1;
            };

            if (blockDepth < 1)
                return errorN(SMSG_GENERAL_ERROR, "%s: Transaction %s for message %s, low depth %d.\n", __func__, txid.ToString(), msgId.ToString(), blockDepth);

            bool fFound = false;
            // Find all msg pairs
            // Message funding is enforced in tx_verify.cpp
            for (const auto &v : txOut->vpout)
            {
                if (!v->IsType(OUTPUT_DATA))
                    continue;
                CTxOutData *txd = (CTxOutData*) v.get();
                if (txd->vData.size() < 25 || txd->vData[0] != DO_FUND_MSG)
                    continue;

                size_t n = (txd->vData.size()-1) / 24;

                for (size_t k = 0; k < n; ++k)
                {
                    uint160 *pMsgIdTx = (uint160*)&txd->vData[1+k*24];
                    uint32_t *nAmount = (uint32_t*)&txd->vData[1+k*24+20];

                    if (*pMsgIdTx == msgId)
                    {
                        if (*nAmount < nExpectFee)
                        {
                            LogPrintf("%s: Transaction %s underfunded message %s, expected %d paid %d.\n", __func__, txid.ToString(), msgId.ToString(), nExpectFee, *nAmount);
                            return SMSG_FUND_FAILED;
                        };
                        fFound = true;
                    };
                };
            };

            if (!fFound)
                return errorN(SMSG_FUND_FAILED, "%s: Transaction %s does not fund message %s.\n", __func__, txid.ToString(), msgId.ToString());

        } // cs_main

        return SMSG_NO_ERROR; // smsg is valid and funded
    };

    if (psmsg->version[0] != 2)
        return SMSG_UNKNOWN_VERSION;

    uint8_t civ[32];
    uint8_t sha256Hash[32];
    int rv = SMSG_INVALID_HASH; // invalid

    uint32_t nonce;
    memcpy(&nonce, &psmsg->nonce[0], 4);

    LogPrint(BCLog::SMSG, "%s: nonce %u.\n", __func__, nonce);

    for (int i = 0; i < 32; i+=4)
        memcpy(civ+i, &nonce, 4);

    CHMAC_SHA256 ctx(&civ[0], 32);
    ctx.Write((uint8_t*) pHeader+4, SMSG_HDR_LEN-4);
    ctx.Write((uint8_t*) pPayload, nPayload);
    ctx.Finalize(sha256Hash);

    if (sha256Hash[31] == 0
        && sha256Hash[30] == 0
        && (~(sha256Hash[29]) & ((1<<0) | (1<<1) | (1<<2))))
    {
        LogPrint(BCLog::SMSG, "Hash Valid.\n");
        rv = SMSG_NO_ERROR; // smsg is valid
    };

    if (part::memcmp_nta(psmsg->hash, sha256Hash, 4) != 0)
    {
        LogPrint(BCLog::SMSG, "Checksum mismatch.\n");
        rv = SMSG_CHECKSUM_MISMATCH; // checksum mismatch
    };

    return rv;
};

int CSMSG::SetHash(uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload)
{
    /*  proof of work and checksum

        May run in a thread, if shutdown detected, return.

        returns SecureMessageCodes
    */

    SecureMessage *psmsg = (SecureMessage*) pHeader;

    int64_t nStart = GetTimeMillis();
    uint8_t civ[32];
    uint8_t sha256Hash[32];

    bool found = false;

    uint32_t nonce = 0;
    memcpy(&nonce, &psmsg->nonce[0], 4);

    //CBigNum bnTarget(2);
    //bnTarget = bnTarget.pow(256 - 40);

    // Break for HMAC_CTX_cleanup
    for (;;)
    {
        if (!fSecMsgEnabled)
           break;

        //psmsg->timestamp = GetTime();
        //memcpy(&psmsg->timestamp, &now, 8);
        memcpy(&psmsg->nonce[0], &nonce, 4);

        for (int i = 0; i < 32; i+=4)
            memcpy(civ+i, &nonce, 4);

        CHMAC_SHA256 ctx(&civ[0], 32);
        ctx.Write((uint8_t*) pHeader+4, SMSG_HDR_LEN-4);
        ctx.Write((uint8_t*) pPayload, nPayload);
        ctx.Finalize(sha256Hash);

        if (sha256Hash[31] == 0
            && sha256Hash[30] == 0
            && (~(sha256Hash[29]) & ((1<<0) | (1<<1) | (1<<2))))
        //    && sha256Hash[29] == 0)
        {
            found = true;
            //if (fDebugSmsg)
            //    LogPrintf("Match %u\n", nonce);
            break;
        };

        if (nonce >= 4294967295U) // UINT32_MAX
        {
            LogPrint(BCLog::SMSG, "No match %u\n", nonce);
            break;
        };
        nonce++;
    };

    if (!fSecMsgEnabled)
    {
        LogPrint(BCLog::SMSG, "%s: Stopped, shutdown detected.\n", __func__);
        return SMSG_SHUTDOWN_DETECTED;
    };

    if (!found)
    {
        LogPrint(BCLog::SMSG, "%s: Failed, took %d ms, nonce %u\n", __func__, GetTimeMillis() - nStart, nonce);
        return SMSG_GENERAL_ERROR;
    };

    memcpy(psmsg->hash, sha256Hash, 4);

    LogPrint(BCLog::SMSG, "%s: Took %d ms, nonce %u\n", __func__, GetTimeMillis() - nStart, nonce);

    return SMSG_NO_ERROR;
};

int CSMSG::Encrypt(SecureMessage &smsg, const CKeyID &addressFrom, const CKeyID &addressTo, const std::string &message)
{
#ifdef ENABLE_WALLET
    /* Create a secure message

    Using similar method to bitmessage.
    If bitmessage is secure this should be too.
    https://bitmessage.org/wiki/Encryption

    Some differences:
    bitmessage seems to use curve sect283r1
    *coin addresses use secp256k1

    returns SecureMessageCodes

    */

    bool fSendAnonymous = addressFrom.IsNull();

    if (LogAcceptCategory(BCLog::SMSG))
    {
        LogPrint(BCLog::SMSG, "SecureMsgEncrypt(%s, %s, ...)\n",
            fSendAnonymous ? "anon" : CBitcoinAddress(addressFrom).ToString(),
            CBitcoinAddress(addressTo).ToString());
    };

    if (smsg.timestamp == 0)
        smsg.timestamp = GetTime();

    CBitcoinAddress coinAddrFrom;
    CKeyID ckidFrom;
    CKey keyFrom;

    if (!fSendAnonymous)
    {
        if (!coinAddrFrom.Set(addressFrom)
            || !coinAddrFrom.GetKeyID(ckidFrom))
            return errorN(SMSG_INVALID_ADDRESS_FROM, "%s: addressFrom is not valid: %s.", __func__, coinAddrFrom.ToString());
    };

    CBitcoinAddress coinAddrDest;
    CKeyID ckidDest = addressTo;

    // Public key K is the destination address
    CPubKey cpkDestK;
    if (GetStoredKey(ckidDest, cpkDestK) != 0
        && GetLocalKey(ckidDest, cpkDestK) != 0) // maybe it's a local key (outbox?)
    {
        return errorN(SMSG_PUBKEY_NOT_EXISTS, "%s: Could not get public key for destination address.", __func__);
    };

    // Generate 16 random bytes as IV.
    GetStrongRandBytes(&smsg.iv[0], 16);

    // Generate a new random EC key pair with private key called r and public key called R.
    CKey keyR;
    keyR.MakeNewKey(true); // make compressed key

    //uint256 P = keyR.ECDH(cpkDestK);
    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_parse(secp256k1_context_smsg, &pubkey, cpkDestK.begin(), cpkDestK.size()))
        return errorN(SMSG_INVALID_ADDRESS_TO, "%s: secp256k1_ec_pubkey_parse failed: %s.", __func__, HexStr(cpkDestK));

    uint256 P;
    if (!secp256k1_ecdh(secp256k1_context_smsg, P.begin(), &pubkey, keyR.begin()))
        return errorN(SMSG_GENERAL_ERROR, "%s: secp256k1_ecdh failed.", __func__);

    CPubKey cpkR = keyR.GetPubKey();
    if (!cpkR.IsValid())
        return errorN(SMSG_GENERAL_ERROR, "%s: Could not get public key for key R.", __func__);

    memcpy(smsg.cpkR, cpkR.begin(), 33);

    // Use public key P and calculate the SHA512 hash H.
    //   The first 32 bytes of H are called key_e and the last 32 bytes are called key_m.
    std::vector<uint8_t> vchHashed;
    vchHashed.resize(64); // 512
    memset(vchHashed.data(), 0, 64);
    CSHA512().Write(P.begin(), 32).Finalize(&vchHashed[0]);
    std::vector<uint8_t> key_e(&vchHashed[0], &vchHashed[0]+32);
    std::vector<uint8_t> key_m(&vchHashed[32], &vchHashed[32]+32);

    std::vector<uint8_t> vchPayload;
    std::vector<uint8_t> vchCompressed;
    uint8_t *pMsgData;
    uint32_t lenMsgData;

    uint32_t lenMsg = message.size();
    if (lenMsg > 128)
    {
        // Only compress if over 128 bytes
        int worstCase = LZ4_compressBound(message.size());
        try { vchCompressed.resize(worstCase); } catch (std::exception &e)
        {
            return errorN(SMSG_ALLOCATE_FAILED, "%s: vchCompressed.resize %u threw: %s.", __func__, worstCase, e.what());
        };

        int lenComp = LZ4_compress((char*)message.c_str(), (char*)vchCompressed.data(), lenMsg);
        if (lenComp < 1)
        {
            return errorN(SMSG_COMPRESS_FAILED, "%s: Could not compress message data.", __func__);
        };

        pMsgData = vchCompressed.data();
        lenMsgData = lenComp;
    } else
    {
        // No compression
        pMsgData = (uint8_t*)message.c_str();
        lenMsgData = lenMsg;
    };

    if (fSendAnonymous)
    {
        try { vchPayload.resize(9 + lenMsgData); } catch (std::exception &e) {
            return errorN(SMSG_ALLOCATE_FAILED, "%s: vchPayload.resize %u threw: %s.", __func__, 9 + lenMsgData, e.what());
        };

        memcpy(&vchPayload[9], pMsgData, lenMsgData);

        vchPayload[0] = 250; // id as anonymous message
        // Next 4 bytes are unused - there to ensure encrypted payload always > 8 bytes
        memcpy(&vchPayload[5], &lenMsg, 4); // length of uncompressed plain text
    } else
    {
        try { vchPayload.resize(SMSG_PL_HDR_LEN + lenMsgData); } catch (std::exception &e) {
            return errorN(SMSG_ALLOCATE_FAILED, "%s: vchPayload.resize %u threw: %s.", __func__, SMSG_PL_HDR_LEN + lenMsgData, e.what());
        };

        memcpy(&vchPayload[SMSG_PL_HDR_LEN], pMsgData, lenMsgData);
        // Compact signature proves ownership of from address and allows the public key to be recovered, recipient can always reply.
        if (!pwallet->GetKey(ckidFrom, keyFrom))
        {
            return errorN(SMSG_UNKNOWN_KEY_FROM, "%s: Could not get private key for addressFrom.", __func__);
        };

        // Sign the plaintext
        std::vector<uint8_t> vchSignature;
        vchSignature.resize(65);
        keyFrom.SignCompact(Hash(message.begin(), message.end()), vchSignature);

        // Save some bytes by sending address raw
        vchPayload[0] = (static_cast<CBitcoinAddress*>(&coinAddrFrom))->getVersion(); // vchPayload[0] = coinAddrDest.nVersion;
        memcpy(&vchPayload[1], ckidFrom.begin(), 20); // memcpy(&vchPayload[1], ckidDest.pn, 20);

        memcpy(&vchPayload[1+20], &vchSignature[0], vchSignature.size());
        memcpy(&vchPayload[1+20+65], &lenMsg, 4); // length of uncompressed plain text
    };

    SecMsgCrypter crypter;
    crypter.SetKey(key_e, smsg.iv);
    std::vector<uint8_t> vchCiphertext;

    if (!crypter.Encrypt(vchPayload.data(), vchPayload.size(), vchCiphertext))
        return errorN(SMSG_ENCRYPT_FAILED, "%s: Encrypt failed.", __func__);

    bool fPaid = smsg.version[0] == 3;
    try { smsg.pPayload = new uint8_t[vchCiphertext.size() + (fPaid ? 32 : 0)]; } catch (std::exception &e)
    {
        return errorN(SMSG_ALLOCATE_FAILED, "%s: Could not allocate pPayload, exception: %s.", __func__, e.what());
    };

    memcpy(smsg.pPayload, vchCiphertext.data(), vchCiphertext.size());
    smsg.nPayload = vchCiphertext.size() + (fPaid ? 32 : 0);

    // Calculate a 32 byte MAC with HMACSHA256, using key_m as salt
    //  Message authentication code, (hash of timestamp + iv + destination + payload)
    CHMAC_SHA256 ctx(&key_m[0], 32);
    ctx.Write((uint8_t*) &smsg.timestamp, sizeof(smsg.timestamp));
    ctx.Write((uint8_t*) smsg.iv, sizeof(smsg.iv));
    ctx.Write((uint8_t*) vchCiphertext.data(), vchCiphertext.size());
    ctx.Finalize(smsg.mac);

#endif
    return SMSG_NO_ERROR;
};

int CSMSG::Send(CKeyID &addressFrom, CKeyID &addressTo, std::string &message,
    SecureMessage &smsg, std::string &sError, bool fPaid,
    size_t nDaysRetention, bool fTestFee, CAmount *nFee, bool fFromFile)
{
#ifdef ENABLE_WALLET
    /* Encrypt secure message, and place it on the network
        Make a copy of the message to sender's first address and place in send queue db
        proof of work thread will pick up messages from  send queue db
    */

    bool fSendAnonymous = (addressFrom.IsNull());

    if (LogAcceptCategory(BCLog::SMSG))
    {
        LogPrintf("SecureMsgSend(%s, %s, ...)\n",
            fSendAnonymous ? "anon" : CBitcoinAddress(addressFrom).ToString(), CBitcoinAddress(addressTo).ToString());
    };

    if (!pwallet)
        return errorN(SMSG_WALLET_LOCKED, sError, __func__, "Wallet is not enabled");
    if (pwallet->IsLocked())
        return errorN(SMSG_WALLET_LOCKED, sError, __func__, "Wallet is locked, wallet must be unlocked to send messages");

    std::string sFromFile;
    if (fFromFile)
    {
        FILE *fp;
        errno = 0;
        if (!(fp = fopen(message.c_str(), "rb")))
            return errorN(SMSG_GENERAL_ERROR, sError, __func__, "fopen failed: %s", strerror(errno));

        if (fseek(fp, 0, SEEK_END) != 0)
        {
            fclose(fp);
            return errorN(SMSG_GENERAL_ERROR, sError, __func__, "fseek failed: %s", strerror(errno));
        };

        int64_t ofs = ftell(fp);
        if (ofs > SMSG_MAX_MSG_BYTES_PAID)
        {
            fclose(fp);
            return errorN(SMSG_MESSAGE_TOO_LONG, sError, __func__, "Message is too long, %d > %d", ofs, SMSG_MAX_MSG_BYTES_PAID);
        };
        rewind(fp);

        sFromFile.resize(ofs);

        int64_t nRead = fread(&sFromFile[0], 1, ofs, fp);
        fclose(fp);
        if (ofs != nRead)
            return errorN(SMSG_GENERAL_ERROR, sError, __func__, "fread failed: %s", strerror(errno));
    };

    std::string &sData = fFromFile ? sFromFile : message;

    if (fPaid)
    {
        if (sData.size() > SMSG_MAX_MSG_BYTES_PAID)
        {
            sError = strprintf("Message is too long, %d > %d", sData.size(), SMSG_MAX_MSG_BYTES_PAID);
            return errorN(SMSG_MESSAGE_TOO_LONG, "%s: %s.", __func__, sError);
        };
    } else
    if (sData.size() > (fSendAnonymous ? SMSG_MAX_AMSG_BYTES : SMSG_MAX_MSG_BYTES))
    {
        sError = strprintf("Message is too long, %d > %d", sData.size(), fSendAnonymous ? SMSG_MAX_AMSG_BYTES : SMSG_MAX_MSG_BYTES);
        return errorN(SMSG_MESSAGE_TOO_LONG, "%s: %s.", __func__, sError);
    };

    int rv;
    smsg = SecureMessage(fPaid, nDaysRetention);
    if ((rv = Encrypt(smsg, addressFrom, addressTo, sData)) != 0)
    {
        sError = GetString(rv);
        return errorN(rv, "%s: %s.", __func__, sError);
    };

    if (fPaid)
    {
        if (0 != FundMsg(smsg, sError, fTestFee, nFee))
            return errorN(SMSG_FUND_FAILED, "%s: SecureMsgFund failed %s.", __func__, sError);

        if (fTestFee)
            return SMSG_NO_ERROR;
    } else
    {
        // HACK: Premine so hash unpaid outbox hashes match, remove with unpaid messages
        if (SMSG_NO_ERROR != SetHash((uint8_t*)&smsg, smsg.pPayload, smsg.nPayload))
            return errorN(SMSG_FUND_FAILED, "%s: SetHash failed %s.", __func__, sError);
    };


    // Place message in send queue, proof of work will happen in a thread.
    uint160 msgId;
    HashMsg(smsg, smsg.pPayload, smsg.nPayload-(fPaid ? 32 : 0), msgId);

    std::string sPrefix("qm");
    uint8_t chKey[30];
    int64_t timestamp_be = bswap_64(smsg.timestamp);
    memcpy(&chKey[0], sPrefix.data(), 2);
    memcpy(&chKey[2], &timestamp_be, 8);
    memcpy(&chKey[10], msgId.begin(), 20);

    SecMsgStored smsgSQ;
    smsgSQ.timeReceived  = GetTime();
    smsgSQ.addrTo        = addressTo;

    try { smsgSQ.vchMessage.resize(SMSG_HDR_LEN + smsg.nPayload); } catch (std::exception &e)
    {
        LogPrintf("smsgSQ.vchMessage.resize %u threw: %s.\n", SMSG_HDR_LEN + smsg.nPayload, e.what());
        sError = "Could not allocate memory.";
        return SMSG_ALLOCATE_FAILED;
    };

    memcpy(&smsgSQ.vchMessage[0], smsg.data(), SMSG_HDR_LEN);
    memcpy(&smsgSQ.vchMessage[SMSG_HDR_LEN], smsg.pPayload, smsg.nPayload);

    {
        LOCK(cs_smsgDB);
        SecMsgDB dbSendQueue;
        if (dbSendQueue.Open("cw"))
        {
            dbSendQueue.WriteSmesg(chKey, smsgSQ);
            //NotifySecMsgSendQueueChanged(smsgOutbox);
        };
    } // cs_smsgDB

    //  For outbox create a copy encrypted for owned address
    //   if the wallet is encrypted private key needed to decrypt will be unavailable

    LogPrint(BCLog::SMSG, "Encrypting message for outbox.\n");

    CKeyID addressOutbox;

    for (const auto &entry : pwallet->mapAddressBook) // PAIRTYPE(CTxDestination, CAddressBookData)
    {
        // Get first owned address
        if (!IsMine(*pwallet, entry.first))
            continue;

        const CBitcoinAddress &address = entry.first;

        if (!address.IsValid())
            continue;
        address.GetKeyID(addressOutbox);
        break;
    };

    if (addressOutbox.IsNull())
    {
        LogPrintf("%s: Warning, could not find an address to encrypt outbox message with.\n", __func__);
    } else
    {
        if (LogAcceptCategory(BCLog::SMSG))
            LogPrintf("Encrypting a copy for outbox, using address %s\n", CBitcoinAddress(addressOutbox).ToString());

        SecureMessage smsgForOutbox(fPaid, nDaysRetention);
        smsgForOutbox.timestamp = smsg.timestamp;
        if ((rv = Encrypt(smsgForOutbox, addressFrom, addressOutbox, sData)) != 0)
        {
            LogPrintf("%s: Encrypt for outbox failed, %d.\n", __func__, rv);
        } else
        {
            if (fPaid)
            {
                uint256 txfundId;
                if (!smsg.GetFundingTxid(txfundId))
                    return errorN(SMSG_GENERAL_ERROR, "%s: GetFundingTxid failed.\n");
                // SecureMsgEncrypt will alloc an extra 32 bytes when smsg version describes paid msg
                memcpy(smsgForOutbox.pPayload+smsgForOutbox.nPayload-32, txfundId.begin(), 32);
            };

            // Save sent message to db
            std::string sPrefix("sm");
            uint8_t chKey[30];
            int64_t timestamp_be = bswap_64(smsgForOutbox.timestamp);
            memcpy(&chKey[0], sPrefix.data(), 2);
            memcpy(&chKey[2], &timestamp_be, 8);
            memcpy(&chKey[10], msgId.begin(), 20);

            SecMsgStored smsgOutbox;

            smsgOutbox.timeReceived  = GetTime();
            smsgOutbox.addrTo        = addressTo;
            smsgOutbox.addrOutbox    = addressOutbox;

            try {
                smsgOutbox.vchMessage.resize(SMSG_HDR_LEN + smsgForOutbox.nPayload);
            } catch (std::exception &e) {
                LogPrintf("smsgOutbox.vchMessage.resize %u threw: %s.\n", SMSG_HDR_LEN + smsgForOutbox.nPayload, e.what());
                sError = "Could not allocate memory.";
                return SMSG_ALLOCATE_FAILED;
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
            } // cs_smsgDB
        };
    };

    if (LogAcceptCategory(BCLog::SMSG))
        LogPrintf("Secure message queued for sending to %s.\n", CBitcoinAddress(addressTo).ToString());

#endif
    return SMSG_NO_ERROR;
};

int CSMSG::HashMsg(const SecureMessage &smsg, const uint8_t *pPayload, uint32_t nPayload, uint160 &hash)
{
    if (smsg.nPayload < nPayload)
        return errorN(SMSG_GENERAL_ERROR, "%s: Data length mismatch.\n", __func__);

    CRIPEMD160()
        .Write(smsg.data(), SMSG_HDR_LEN)
        .Write(pPayload, nPayload)
        .Finalize(hash.begin());

    return SMSG_NO_ERROR;
};

int CSMSG::FundMsg(SecureMessage &smsg, std::string &sError, bool fTestFee, CAmount *nFee)
{
    // smsg.pPayload must have smsg.nPayload + 32 bytes allocated
#ifdef ENABLE_WALLET
    if (!pwallet)
        return SMSG_WALLET_UNSET;

    if (smsg.version[0] != 3)
        return errorN(SMSG_UNKNOWN_VERSION, sError, __func__, "Bad message version.");

    size_t nDaysRetention = smsg.nonce[0];
    if (nDaysRetention < 1 || nDaysRetention > 31)
        return errorN(SMSG_GENERAL_ERROR, sError, __func__, "Bad message ttl.");

    uint256 txfundId;
    uint160 msgId;
    CMutableTransaction txFund;

    if (0 != HashMsg(smsg, smsg.pPayload, smsg.nPayload-32, msgId))
        return errorN(SMSG_GENERAL_ERROR, sError, __func__, "Message hash failed.");

    txFund.nVersion = PARTICL_TXN_VERSION;

    size_t nMsgBytes = SMSG_HDR_LEN + smsg.nPayload;

    CCoinControl coinControl;
    coinControl.m_feerate = CFeeRate(nFundingTxnFeePerK);
    coinControl.fOverrideFeeRate = true;
    coinControl.m_extrafee = ((nMsgFeePerKPerDay * nMsgBytes) / 1000) * nDaysRetention;

    assert(coinControl.m_extrafee <= std::numeric_limits<uint32_t>::max());
    uint32_t msgFee = coinControl.m_extrafee;

    std::shared_ptr<CTxOutData> out0 = MAKE_OUTPUT<CTxOutData>();
    out0->vData.resize(25); // 4 byte fee, max 42.94967295
    out0->vData[0] = DO_FUND_MSG;
    memcpy(&out0->vData[1], msgId.begin(), 20);
    memcpy(&out0->vData[21], &msgFee, 4);
    txFund.vpout.push_back(out0);

    int nChangePosInOut;
    CAmount nFeeRet;
    const std::set<int> setSubtractFeeFromOutputs;

    {
        LOCK2(cs_main, pwallet->cs_wallet);
        if (!pwallet->FundTransaction(txFund, nFeeRet, nChangePosInOut, sError, false, setSubtractFeeFromOutputs, coinControl))
            return errorN(SMSG_GENERAL_ERROR, "%s: FundTransaction failed.\n", __func__);

        if (nFee)
            *nFee = pwallet->GetDebit(txFund, ISMINE_ALL) - pwallet->GetCredit(txFund, ISMINE_ALL);

        if (fTestFee)
            return SMSG_NO_ERROR;

        if (!pwallet->SignTransaction(txFund))
            return errorN(SMSG_GENERAL_ERROR, sError, __func__, "SignTransaction failed.");

        txfundId = txFund.GetHash();

        if (!pwallet->GetBroadcastTransactions())
            return errorN(SMSG_GENERAL_ERROR, sError, __func__, "Broadcast transactions disabled.");

        CWalletTx wtx(pwallet.get(), MakeTransactionRef(txFund));

        CAmount maxTxFee = 1 * COIN;

        CValidationState state;
        if (!wtx.AcceptToMemoryPool(maxTxFee, state))
            return errorN(SMSG_GENERAL_ERROR, sError, __func__, "Transaction cannot be broadcast immediately: %s.", state.GetRejectReason());

        pwallet->AddToWallet(wtx);
        wtx.RelayWalletTransaction(g_connman.get());
    }
    memcpy(smsg.pPayload+(smsg.nPayload-32), txfundId.begin(), 32);
#else
    return SMSG_WALLET_UNSET;
#endif
    return SMSG_NO_ERROR;
};

std::vector<uint8_t> CSMSG::GetMsgID(const SecureMessage *psmsg, const uint8_t *pPayload)
{
    std::vector<uint8_t> rv(28);
    int64_t timestamp_be = bswap_64(psmsg->timestamp);
    memcpy(rv.data(), &timestamp_be, 8);

    HashMsg(*psmsg, pPayload, psmsg->nPayload-(psmsg->IsPaidVersion() ? 32 : 0), *((uint160*)&rv[8]));

    return rv;
};

std::vector<uint8_t> CSMSG::GetMsgID(const SecureMessage &smsg)
{
    std::vector<uint8_t> rv(28);
    int64_t timestamp_be = bswap_64(smsg.timestamp);
    memcpy(rv.data(), &timestamp_be, 8);

    HashMsg(smsg, smsg.pPayload, smsg.nPayload-(smsg.IsPaidVersion() ? 32 : 0), *((uint160*)&rv[8]));

    return rv;
};

int CSMSG::Decrypt(bool fTestOnly, const CKey &keyDest, const CKeyID &address, const uint8_t *pHeader, const uint8_t *pPayload, uint32_t nPayload, MessageData &msg)
{
    /* Decrypt secure message

        address is the owned address to decrypt with.

        validate first in SecureMsgValidate

        returns SecureMessageErrors
    */

    if (LogAcceptCategory(BCLog::SMSG))
        LogPrintf("%s: using %s, testonly %d.\n", __func__, CBitcoinAddress(address).ToString(), fTestOnly);

    if (!pHeader
        || !pPayload)
    {
        return errorN(SMSG_GENERAL_ERROR, "%s: null pointer to header or payload.", __func__);
    };

    SecureMessage *psmsg = (SecureMessage*) pHeader;
    if (psmsg->version[0] == 3)
    {
        nPayload -= 32; // Exclude funding txid
    } else
    if (psmsg->version[0] != 2)
    {
        return errorN(SMSG_UNKNOWN_VERSION, "%s: Unknown version number.", __func__);
    };

    // Do an EC point multiply with private key k and public key R. This gives you public key P.
    //CPubKey R(psmsg->cpkR, psmsg->cpkR+33);
    //uint256 P = keyDest.ECDH(R);
    secp256k1_pubkey R;
    if (!secp256k1_ec_pubkey_parse(secp256k1_context_smsg, &R, psmsg->cpkR, 33))
        return errorN(SMSG_GENERAL_ERROR, "%s: secp256k1_ec_pubkey_parse failed: %s.", __func__, HexStr(psmsg->cpkR, psmsg->cpkR+33));

    uint256 P;
    if (!secp256k1_ecdh(secp256k1_context_smsg, P.begin(), &R, keyDest.begin()))
        return errorN(SMSG_GENERAL_ERROR, "%s: secp256k1_ecdh failed.", __func__);

    // Use public key P to calculate the SHA512 hash H.
    //  The first 32 bytes of H are called key_e and the last 32 bytes are called key_m.
    std::vector<uint8_t> vchHashedDec;
    vchHashedDec.resize(64);    // 512 bits
    memset(vchHashedDec.data(), 0, 64);
    CSHA512().Write(P.begin(), 32).Finalize(&vchHashedDec[0]);
    std::vector<uint8_t> key_e(&vchHashedDec[0], &vchHashedDec[0]+32);
    std::vector<uint8_t> key_m(&vchHashedDec[32], &vchHashedDec[32]+32);

    // Message authentication code, (hash of timestamp + iv + destination + payload)
    uint8_t MAC[32];

    CHMAC_SHA256 ctx(key_m.data(), 32);
    ctx.Write((uint8_t*) &psmsg->timestamp, sizeof(psmsg->timestamp));
    ctx.Write((uint8_t*) psmsg->iv, sizeof(psmsg->iv));
    ctx.Write((uint8_t*) pPayload, nPayload);
    ctx.Finalize(MAC);

    if (part::memcmp_nta(MAC, psmsg->mac, 32) != 0)
    {
        LogPrint(BCLog::SMSG, "MAC does not match.\n"); // expected if message is not to address on node
        return SMSG_MAC_MISMATCH;
    };

    if (fTestOnly)
        return SMSG_NO_ERROR;

    SecMsgCrypter crypter;
    crypter.SetKey(key_e, psmsg->iv);
    std::vector<uint8_t> vchPayload;
    if (!crypter.Decrypt(pPayload, nPayload, vchPayload))
        return errorN(SMSG_GENERAL_ERROR, "%s: Decrypt failed.", __func__);

    msg.timestamp = psmsg->timestamp;
    uint32_t lenData;
    uint32_t lenPlain;

    uint8_t *pMsgData;
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
    } catch (std::exception &e) {
        return errorN(SMSG_ALLOCATE_FAILED, "%s: msg.vchMessage.resize %u threw: %s.", __func__, lenPlain + 1, e.what());
    };

    if (lenPlain > 128)
    {
        // Decompress
        if (LZ4_decompress_safe((char*) pMsgData, (char*) &msg.vchMessage[0], lenData, lenPlain) != (int) lenPlain)
            return errorN(SMSG_GENERAL_ERROR, "%s: Could not decompress message data.", __func__);
    } else
    {
        // Plaintext
        memcpy(&msg.vchMessage[0], pMsgData, lenPlain);
    };

    msg.vchMessage[lenPlain] = '\0';

    if (fFromAnonymous)
    {
        // Anonymous sender
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
            return errorN(SMSG_INVALID_ADDRESS, "%s: From Address is invalid.", __func__);

        std::vector<uint8_t> vchSig;
        vchSig.resize(65);

        memcpy(&vchSig[0], &vchPayload[1+20], 65);

        CPubKey cpkFromSig;
        cpkFromSig.RecoverCompact(Hash(msg.vchMessage.begin(), msg.vchMessage.end()-1), vchSig);
        if (!cpkFromSig.IsValid())
            return errorN(SMSG_GENERAL_ERROR, "%s: Signature validation failed.", __func__);

        // Get address for the compressed public key
        CBitcoinAddress coinAddrFromSig;
        coinAddrFromSig.Set(cpkFromSig.GetID());

        if (!(coinAddrFrom == coinAddrFromSig))
            return errorN(SMSG_GENERAL_ERROR, "%s: Signature validation failed.", __func__);

        int rv = SMSG_GENERAL_ERROR;
        try {
            rv = InsertAddress(ckidFrom, cpkFromSig);
        } catch (std::exception &e) {
            LogPrintf("%s, exception: %s.\n", __func__, e.what());
            //return 1;
        };

        if (rv != SMSG_NO_ERROR)
        {
            if (rv == SMSG_PUBKEY_EXISTS)
            {
                LogPrint(BCLog::SMSG, "%s: Sender public key not added to db, %s.\n", __func__, GetString(rv));
            } else
            {
                LogPrintf("%s: Sender public key not added to db, %s.\n", __func__, GetString(rv));
            };
        };

        msg.sFromAddress = coinAddrFrom.ToString();
    };

    if (LogAcceptCategory(BCLog::SMSG))
        LogPrintf("Decrypted message for %s.\n", CBitcoinAddress(address).ToString());

    return SMSG_NO_ERROR;
};

int CSMSG::Decrypt(bool fTestOnly, const CKey &keyDest, const CKeyID &address, const SecureMessage &smsg, MessageData &msg)
{
    return CSMSG::Decrypt(fTestOnly, keyDest, address, smsg.data(), smsg.pPayload, smsg.nPayload, msg);
};

int CSMSG::Decrypt(bool fTestOnly, const CKeyID &address, const uint8_t *pHeader, const uint8_t *pPayload, uint32_t nPayload, MessageData &msg)
{
    // Fetch private key k, used to decrypt
    CKey keyDest;
    ReadSmsgKey(address, keyDest);

#ifdef ENABLE_WALLET
    if (!keyDest.IsValid())
    {
        if (pwallet->IsLocked())
            return SMSG_WALLET_LOCKED;
        pwallet->GetKey(address, keyDest);
    };
#endif
    if (!keyDest.IsValid())
        return errorN(SMSG_UNKNOWN_KEY, "%s: Could not get private key for addressDest.", __func__);

    return CSMSG::Decrypt(fTestOnly, keyDest, address, pHeader, pPayload, nPayload, msg);
};

int CSMSG::Decrypt(bool fTestOnly, const CKeyID &address, const SecureMessage &smsg, MessageData &msg)
{
    return CSMSG::Decrypt(fTestOnly, address, smsg.data(), smsg.pPayload, smsg.nPayload, msg);
};

} // namespace smsg
