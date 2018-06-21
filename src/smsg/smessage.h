// Copyright (c) 2014-2016 The ShadowCoin developers
// Copyright (c) 2017-2018 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PARTICL_SMSG_SMESSAGE_H
#define PARTICL_SMSG_SMESSAGE_H

#include <key_io.h>
#include <net.h>
#include <serialize.h>
#include <ui_interface.h>
#include <lz4/lz4.h>
#include <smsg/keystore.h>
#include <interfaces/handler.h>


class CWallet;

namespace smsg
{

enum SecureMessageCodes {
    SMSG_NO_ERROR = 0,
    SMSG_GENERAL_ERROR,
    SMSG_UNKNOWN_VERSION,
    SMSG_INVALID_ADDRESS,
    SMSG_INVALID_ADDRESS_FROM,
    SMSG_INVALID_ADDRESS_TO,
    SMSG_INVALID_PUBKEY,
    SMSG_PUBKEY_MISMATCH,
    SMSG_PUBKEY_EXISTS,
    SMSG_PUBKEY_NOT_EXISTS,
    SMSG_KEY_EXISTS,
    SMSG_KEY_NOT_EXISTS,
    SMSG_UNKNOWN_KEY,
    SMSG_UNKNOWN_KEY_FROM,
    SMSG_ALLOCATE_FAILED,
    SMSG_MAC_MISMATCH,
    SMSG_WALLET_UNSET,
    SMSG_WALLET_NO_PUBKEY,
    SMSG_WALLET_NO_KEY,
    SMSG_WALLET_LOCKED,
    SMSG_DISABLED,
    SMSG_UNKNOWN_MESSAGE,
    SMSG_PAYLOAD_OVER_SIZE,
    SMSG_TIME_IN_FUTURE,
    SMSG_TIME_EXPIRED,
    SMSG_INVALID_HASH,
    SMSG_CHECKSUM_MISMATCH,
    SMSG_SHUTDOWN_DETECTED,
    SMSG_MESSAGE_TOO_LONG,
    SMSG_COMPRESS_FAILED,
    SMSG_ENCRYPT_FAILED,
    SMSG_FUND_FAILED,
    SMSG_PURGED_MSG,
};

const unsigned int SMSG_HDR_LEN        = 104;               // length of unencrypted header, 4 + 2 + 1 + 8 + 16 + 33 + 32 + 4 + 4
const unsigned int SMSG_PL_HDR_LEN     = 1+20+65+4;         // length of encrypted header in payload

const unsigned int SMSG_BUCKET_LEN     = 60 * 60 * 1;       // seconds
const unsigned int SMSG_RETENTION_OLD  = 60 * 60 * 48;      // seconds

const unsigned int SMSG_SECONDS_IN_DAY = 86400;
const unsigned int SMSG_MAX_PAID_TTL   = SMSG_SECONDS_IN_DAY * 31;        // seconds
const unsigned int SMSG_RETENTION      = SMSG_MAX_PAID_TTL;
const unsigned int SMSG_SEND_DELAY     = 2;                 // seconds, SecureMsgSendData will delay this long between firing
const unsigned int SMSG_THREAD_DELAY   = 30;
const unsigned int SMSG_THREAD_LOG_GAP = 6;

const unsigned int SMSG_TIME_LEEWAY    = 24;
const unsigned int SMSG_TIME_IGNORE    = 90;                // seconds a peer is ignored for if they fail to deliver messages for a smsgWant


const unsigned int SMSG_MAX_MSG_BYTES  = 24000;             // the user input part
const unsigned int SMSG_MAX_AMSG_BYTES = 512;               // the user input part (ANON)
const unsigned int SMSG_MAX_MSG_BYTES_PAID = 512 * 1024;    // the user input part (Paid)

// Max size of payload worst case compression
const unsigned int SMSG_MAX_MSG_WORST = LZ4_COMPRESSBOUND(SMSG_MAX_MSG_BYTES+SMSG_PL_HDR_LEN);
const unsigned int SMSG_MAX_MSG_WORST_PAID = LZ4_COMPRESSBOUND(SMSG_MAX_MSG_BYTES_PAID+SMSG_PL_HDR_LEN);

static const int MIN_SMSG_PROTO_VERSION = 90007;


const CAmount nFundingTxnFeePerK = 200000;
const CAmount nMsgFeePerKPerDay =   50000;

#define SMSG_MASK_UNREAD (1 << 0)

class SecMsgStored;

// Inbox db changed, called with lock cs_smsgDB held.
extern boost::signals2::signal<void (SecMsgStored &inboxHdr)> NotifySecMsgInboxChanged;

// Outbox db changed, called with lock cs_smsgDB held.
extern boost::signals2::signal<void (SecMsgStored &outboxHdr)> NotifySecMsgOutboxChanged;

// Wallet unlocked, called after all messages received while locked have been processed.
extern boost::signals2::signal<void ()> NotifySecMsgWalletUnlocked;


uint32_t SMSGGetSecondsInDay();

inline bool GetFundingTxid(const uint8_t *pPayload, size_t nPayload, uint256 &txid)
{
    if (!pPayload || nPayload < 32)
        return false;
    memcpy(txid.begin(), pPayload+(nPayload-32), 32);
    return true;
};

#pragma pack(push, 1)
class SecureMessage
{
public:
    SecureMessage() {};
    SecureMessage(bool fPaid, size_t nDaysRetention)
    {
        if (fPaid)
        {
            version[0] = 3;
            version[1] = 0;

            nonce[0] = nDaysRetention;
        };
    };
    ~SecureMessage()
    {
        if (pPayload)
            delete[] pPayload;
        pPayload = nullptr;
    };

    void SetNull()
    {
        memset(iv, 0, 16);
        memset(cpkR, 0, 33);
        memset(mac, 0, 32);
    };

    bool IsPaidVersion() const
    {
        return version[0] == 3;
    };

    bool GetFundingTxid(uint256 &txid) const
    {
        if (version[0] != 3)
            return false;
        return smsg::GetFundingTxid(pPayload, nPayload, txid);
    };

    uint8_t *data()
    {
        return &hash[0];
    };

    const uint8_t *data() const
    {
        return &hash[0];
    };

    uint8_t hash[4] = {0, 0, 0, 0};
    uint8_t version[2] = {2, 1};
    uint8_t flags = 0;
    int64_t timestamp = 0;
    uint8_t iv[16];
    uint8_t cpkR[33];
    uint8_t mac[32];
    uint8_t nonce[4] = {0, 0, 0, 0}; // nDaysRetention when paid message
    uint32_t nPayload = 0;
    uint8_t *pPayload = nullptr;
};
#pragma pack(pop)

class MessageData
{
// Decrypted SecureMessage data
public:
    int64_t               timestamp;
    std::string           sToAddress;
    std::string           sFromAddress;
    std::vector<uint8_t>  vchMessage;         // null terminated plaintext
};

class SecMsgToken
{
public:
    SecMsgToken() {};
    SecMsgToken(int64_t ts, const uint8_t *p, int np, long int o, uint8_t ttl_)
    {
        timestamp = ts;

        if (np < 8)
            memset(sample, 0, 8);
        else
            memcpy(sample, p, 8);
        offset = o;
        ttl = ttl_;
    };

    bool operator <(const SecMsgToken &y) const
    {
        if (timestamp == y.timestamp)
            return memcmp(sample, y.sample, 8) < 0;
        return timestamp < y.timestamp;
    };

    std::string ToString() const;

    int64_t timestamp;
    uint8_t sample[8];    // first 8 bytes of payload
    int64_t offset;       // offset
    mutable uint8_t ttl;  // days
};

class SecMsgPurged // Purged token marker
{
public:
    SecMsgPurged() {};
    SecMsgPurged(int64_t ts, int64_t tp)
    {
        timestamp = ts;
        memset(sample, 0, 8);
        timepurged = tp;
    };

    bool operator <(const SecMsgPurged &y) const
    {
        if (timestamp == y.timestamp)
            return memcmp(sample, y.sample, 8) < 0;
        return timestamp < y.timestamp;
    };

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s << timestamp;
        s.write((char*)&sample[0], 33);
        s << timepurged;
    };
    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s >> timestamp;
        s.read((char*)&sample[0], 33);
        s >> timepurged;
    };

    int64_t timestamp;
    uint8_t sample[8];
    int64_t timepurged;
};

class SecMsgBucket
{
public:
    SecMsgBucket()
    {
        timeChanged     = 0;
        hash            = 0;
        nLockCount      = 0;
        nLockPeerId     = 0;
        nLeastTTL       = 0;
        nActive         = 0;
    };

    void hashBucket();
    size_t CountActive();

    int64_t               timeChanged;
    uint32_t              hash;           // token set should get ordered the same on each node
    uint32_t              nLockCount;     // set when smsgWant first sent, unset at end of smsgMsg, ticks down in ThreadSecureMsg()
    uint32_t              nLeastTTL;      // lowest ttl in days of messages in bkt
    uint32_t              nActive;        // Number of untimedout messages in bucket
    NodeId                nLockPeerId;    // id of peer that bucket is locked for
    std::set<SecMsgToken> setTokens;
};

class SecMsgAddress
{
public:
    SecMsgAddress() {};
    SecMsgAddress(CKeyID addr, bool receiveOn, bool receiveAnon)
    {
        address         = addr;
        fReceiveEnabled = receiveOn;
        fReceiveAnon    = receiveAnon;
    };

    CKeyID address;
    bool fReceiveEnabled;
    bool fReceiveAnon;

    size_t GetSerializeSize(int nType, int nVersion) const
    {
        return 22;
    };
    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s << address;
        s << fReceiveEnabled;
        s << fReceiveAnon;
    };
    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s >> address;
        s >> fReceiveEnabled;
        s >> fReceiveAnon;
    };
};

class SecMsgOptions
{
public:
    SecMsgOptions()
    {
        // Default options
        fNewAddressRecv = true;
        fNewAddressAnon = true;
        fScanIncoming   = false;
    };

    bool fNewAddressRecv;
    bool fNewAddressAnon;
    bool fScanIncoming;
};

class SecMsgStored
{
public:
    int64_t              timeReceived;
    uint8_t              status;         // read etc
    uint16_t             folderId;
    CKeyID               addrTo;         // when in owned addr, when sent remote addr
    CKeyID               addrOutbox;     // owned address this copy was encrypted with
    std::vector<uint8_t> vchMessage;     // message header + encryped payload

    size_t GetSerializeSize(int nType, int nVersion) const
    {
        return sizeof(timeReceived) + sizeof(status) + sizeof(folderId) + 20 + 20 +
            GetSizeOfCompactSize(vchMessage.size()) + vchMessage.size() * sizeof(uint8_t);
    };
    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s << timeReceived;
        s << status;
        s << folderId;
        s << addrTo;
        s << addrOutbox;
        s << vchMessage;
    };
    template <typename Stream>
    void Unserialize(Stream &s)
    {
        s >> timeReceived;
        s >> status;
        s >> folderId;
        s >> addrTo;
        s >> addrOutbox;
        s >> vchMessage;
    };
};

void AddOptions();
const char *GetString(size_t errorCode);

extern bool fSecMsgEnabled;
class CSMSG
{
public:
    int BuildBucketSet();
    int BuildPurgedSets();
    int AddWalletAddresses();
    int LoadKeyStore();

    int ReadIni();
    int WriteIni();

    bool Start(std::shared_ptr<CWallet> pwalletIn, bool fDontStart, bool fScanChain);
    bool Shutdown();

    bool Enable(std::shared_ptr<CWallet> pwallet);
    bool Disable();

    int ReceiveData(CNode *pfrom, const std::string &strCommand, CDataStream &vRecv);
    bool SendData(CNode *pto, bool fSendTrickle);

    bool ScanBlock(const CBlock &block);
    bool ScanChainForPublicKeys(CBlockIndex *pindexStart);
    bool ScanBlockChain();
    bool ScanBuckets();

    int ManageLocalKey(CKeyID &keyId, ChangeType mode);
    int WalletUnlocked();
    int WalletKeyChanged(CKeyID &keyId, const std::string &sLabel, ChangeType mode);

    int ScanMessage(const uint8_t *pHeader, const uint8_t *pPayload, uint32_t nPayload, bool reportToGui);

    int GetStoredKey(const CKeyID &ckid, CPubKey &cpkOut);
    int GetLocalKey(const CKeyID &ckid, CPubKey &cpkOut);
    int GetLocalPublicKey(const std::string &strAddress, std::string &strPublicKey);

    int AddAddress(std::string &address, std::string &publicKey);
    int AddLocalAddress(const std::string &sAddress);
    int ImportPrivkey(const CBitcoinSecret &vchSecret, const std::string &sLabel);

    bool SetWalletAddressOption(const CKeyID &idk, std::string sOption, bool fValue);
    bool SetSmsgAddressOption(const CKeyID &idk, std::string sOption, bool fValue);

    int ReadSmsgKey(const CKeyID &idk, CKey &key);

    int Retrieve(const SecMsgToken &token, std::vector<uint8_t> &vchData);
    int Remove(const SecMsgToken &token);

    int Receive(CNode *pfrom, std::vector<uint8_t> &vchData);

    int CheckPurged(const SecureMessage *psmsg, const uint8_t *pPayload);

    int StoreUnscanned(const uint8_t *pHeader, const uint8_t *pPayload, uint32_t nPayload);
    int Store(const uint8_t *pHeader, const uint8_t *pPayload, uint32_t nPayload, bool fHashBucket);
    int Store(const SecureMessage &smsg, bool fHashBucket);

    int Purge(std::vector<uint8_t> &vMsgId, std::string &sError);

    int Send(CKeyID &addressFrom, CKeyID &addressTo, std::string &message,
        SecureMessage &smsg, std::string &sError, bool fPaid=false,
        size_t nDaysRetention=0, bool fTestFee=false, CAmount *nFee=NULL, bool fFromFile=false);


    int HashMsg(const SecureMessage &smsg, const uint8_t *pPayload, uint32_t nPayload, uint160 &hash);
    int FundMsg(SecureMessage &smsg, std::string &sError, bool fTestFee, CAmount *nFee);

    std::vector<uint8_t> GetMsgID(const SecureMessage *psmsg, const uint8_t *pPayload);
    std::vector<uint8_t> GetMsgID(const SecureMessage &smsg);

    int Validate(const uint8_t *pHeader, const uint8_t *pPayload, uint32_t nPayload);
    int SetHash (uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload);

    int Encrypt(SecureMessage &smsg, const CKeyID &addressFrom, const CKeyID &addressTo, const std::string &message);

    int Decrypt(bool fTestOnly, const CKey &keyDest, const CKeyID &address, const uint8_t *pHeader, const uint8_t *pPayload, uint32_t nPayload, MessageData &msg);
    int Decrypt(bool fTestOnly, const CKey &keyDest, const CKeyID &address, const SecureMessage &smsg, MessageData &msg);

    int Decrypt(bool fTestOnly, const CKeyID &address, const uint8_t *pHeader, const uint8_t *pPayload, uint32_t nPayload, MessageData &msg);
    int Decrypt(bool fTestOnly, const CKeyID &address, const SecureMessage &smsg, MessageData &msg);

    CCriticalSection cs_smsg; // all except inbox and outbox

    SecMsgKeyStore keyStore;
    std::map<int64_t, SecMsgBucket> buckets;
    std::vector<SecMsgAddress> addresses;
    std::set<SecMsgPurged> setPurged;
    std::set<int64_t> setPurgedTimestamps;
    SecMsgOptions options;
    std::shared_ptr<CWallet> pwallet;
    std::unique_ptr<interfaces::Handler> m_handler_unload;

    int64_t nLastProcessedPurged = 0;
};

} // namespace smsg

extern smsg::CSMSG smsgModule;

#endif // PARTICL_SMSG_SMESSAGE_H

