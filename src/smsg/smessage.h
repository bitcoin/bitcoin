// Copyright (c) 2014-2016 The ShadowCoin developers
// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SEC_MESSAGE_H
#define SEC_MESSAGE_H

#include "net.h"
#include "lz4/lz4.h"
#include "base58.h"
#include "serialize.h"
#include "ui_interface.h"


enum SecureMessageCodes {
    SMSG_NO_ERROR           = 0,
    SMSG_GENERAL_ERROR,
    SMSG_UNKNOWN_VERSION,
    SMSG_INVALID_ADDRESS,
    SMSG_INVALID_ADDRESS_FROM,
    SMSG_INVALID_ADDRESS_TO,
    SMSG_INVALID_PUBKEY,
    SMSG_PUBKEY_MISMATCH,
    SMSG_PUBKEY_EXISTS,
    SMSG_PUBKEY_NOT_EXISTS,
    SMSG_UNKNOWN_KEY,
    SMSG_UNKNOWN_KEY_FROM,
    SMSG_ALLOCATE_FAILED,
    SMSG_MAC_MISMATCH,
    SMSG_WALLET_UNSET,
    SMSG_WALLET_NO_PUBKEY,
    SMSG_WALLET_LOCKED,
    SMSG_DISABLED,
    SMSG_UNKNOWN_MESSAGE,
    SMSG_PAYLOAD_OVER_SIZE,
    SMSG_TIME_IN_FUTURE,
    SMSG_INVALID_HASH,
    SMSG_CHECKSUM_MISMATCH,
    SMSG_SHUTDOWN_DETECTED,
    SMSG_MESSAGE_TOO_LONG,
    SMSG_COMPRESS_FAILED,
    SMSG_ENCRYPT_FAILED,
    SMSG_FUND_FAILED,
};

const unsigned int SMSG_HDR_LEN        = 104;               // length of unencrypted header, 4 + 2 + 1 + 8 + 16 + 33 + 32 + 4 + 4
const unsigned int SMSG_PL_HDR_LEN     = 1+20+65+4;         // length of encrypted header in payload

const unsigned int SMSG_BUCKET_LEN     = 60 * 10;           // seconds
const unsigned int SMSG_RETENTION      = 60 * 60 * 48;      // seconds
const unsigned int SMSG_MAX_PAID_TTL   = 86400 * 31;        // seconds
const unsigned int SMSG_SEND_DELAY     = 2;                 // seconds, SecureMsgSendData will delay this long between firing
const unsigned int SMSG_THREAD_DELAY   = 30;
const unsigned int SMSG_THREAD_LOG_GAP = 6;

const unsigned int SMSG_TIME_LEEWAY    = 24;
const unsigned int SMSG_TIME_IGNORE    = 90;                // seconds that a peer is ignored for if they fail to deliver messages for a smsgWant


const unsigned int SMSG_MAX_MSG_BYTES  = 4096;              // the user input part
const unsigned int SMSG_MAX_AMSG_BYTES = 512;               // the user input part (ANON)

// max size of payload worst case compression
const unsigned int SMSG_MAX_MSG_WORST = LZ4_COMPRESSBOUND(SMSG_MAX_MSG_BYTES+SMSG_PL_HDR_LEN);


const CAmount nFundingTxnFeePerK = 100000;
const CAmount nMsgFeePerKPerDay = 10000;

#define SMSG_MASK_UNREAD            (1 << 0)

extern bool fSecMsgEnabled;

class CWallet;
class SecMsgStored;

// Inbox db changed, called with lock cs_smsgDB held.
extern boost::signals2::signal<void (SecMsgStored& inboxHdr)> NotifySecMsgInboxChanged;

// Outbox db changed, called with lock cs_smsgDB held.
extern boost::signals2::signal<void (SecMsgStored& outboxHdr)> NotifySecMsgOutboxChanged;

// Wallet unlocked, called after all messages received while locked have been processed.
extern boost::signals2::signal<void ()> NotifySecMsgWalletUnlocked;

class SecMsgBucket;
class SecMsgAddress;
class SecMsgOptions;

extern std::map<int64_t, SecMsgBucket>  smsgBuckets;
extern std::vector<SecMsgAddress>       smsgAddresses;
extern SecMsgOptions                    smsgOptions;
extern CWallet                          *pwalletSmsg;

extern CCriticalSection cs_smsg;            // all except inbox and outbox

#pragma pack(push, 1)
class SecureMessage
{
public:
    SecureMessage() {};
    SecureMessage(bool fPaid)
    {
        if (fPaid)
        {
            version[0] = 3;
            version[1] = 0;
        };
    };
    ~SecureMessage()
    {
        if (pPayload)
            delete[] pPayload;
        pPayload = nullptr;
    };

    bool GetFundingTxid(uint256 &txid)
    {
        if (version[0] != 3 || !pPayload || nPayload < 32)
            return false;
        memcpy(txid.begin(), pPayload+(nPayload-32), 32);
        return true;
    };

    uint8_t *data()
    {
        return &hash[0];
    }

    const uint8_t *data() const
    {
        return &hash[0];
    }

    uint8_t  hash[4];
    uint8_t  version[2] = {2, 1};
    uint8_t  flags;
    int64_t  timestamp;
    uint8_t  iv[16];
    uint8_t  cpkR[33];
    uint8_t  mac[32];
    uint8_t  nonce[4]; // nDaysRetention when paid message
    uint32_t nPayload = 0;
    uint8_t* pPayload = nullptr;
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
    SecMsgToken(int64_t ts, uint8_t *p, int np, long int o)
    {
        timestamp = ts;

        if (np < 8) // payload will always be > 8, just make sure
            memset(sample, 0, 8);
        else
            memcpy(sample, p, 8);
        offset = o;
    };

    SecMsgToken() {};

    bool operator <(const SecMsgToken &y) const
    {
        // pack and memcmp from timesent?
        if (timestamp == y.timestamp)
            return memcmp(sample, y.sample, 8) < 0;
        return timestamp < y.timestamp;
    }

    int64_t timestamp;    // TODO doesn't need to be full 64 bytes?
    uint8_t sample[8];    // first 8 bytes of payload - a hash
    int64_t offset;       // offset
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
    };

    void hashBucket();

    int64_t               timeChanged;
    uint32_t              hash;           // token set should get ordered the same on each node
    uint32_t              nLockCount;     // set when smsgWant first sent, unset at end of smsgMsg, ticks down in ThreadSecureMsg()
    NodeId                nLockPeerId;    // id of peer that bucket is locked for
    std::set<SecMsgToken> setTokens;
};

class CBitcoinAddress_B : public CBitcoinAddress
{
public:
    uint8_t getVersion()
    {
        // TODO: fix
        if (vchVersion.size() > 0)
            return vchVersion[0];
        return 0;
    };
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

    CKeyID      address;
    bool        fReceiveEnabled;
    bool        fReceiveAnon;

    unsigned int GetSerializeSize(int nType, int nVersion) const
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
        fScanIncoming   = true;
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

    unsigned int GetSerializeSize(int nType, int nVersion) const
    {
        return 64 + 1 + 2 + 20 + 20 +
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
    void Unserialize(Stream& s)
    {
        s >> timeReceived;
        s >> status;
        s >> folderId;
        s >> addrTo;
        s >> addrOutbox;
        s >> vchMessage;
    };
};

std::string SecureMsgGetHelpString(bool showDebug);
const char *SecureMessageGetString(size_t errorCode);

int SecureMsgBuildBucketSet();
int SecureMsgAddWalletAddresses();

int SecureMsgReadIni();
int SecureMsgWriteIni();

bool SecureMsgStart(CWallet *pwallet, bool fDontStart, bool fScanChain);
bool SecureMsgShutdown();

bool SecureMsgEnable(CWallet *pwallet);
bool SecureMsgDisable();

int SecureMsgReceiveData(CNode *pfrom, const std::string &strCommand, CDataStream &vRecv);
bool SecureMsgSendData(CNode *pto, bool fSendTrickle);

bool SecureMsgScanBlock(const CBlock &block);
bool ScanChainForPublicKeys(CBlockIndex *pindexStart);
bool SecureMsgScanBlockChain();
bool SecureMsgScanBuckets();

int SecureMsgWalletUnlocked();
int SecureMsgWalletKeyChanged(CKeyID &keyId, const std::string &sLabel, ChangeType mode);

int SecureMsgScanMessage(uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload, bool reportToGui);

int SecureMsgGetStoredKey(CKeyID &ckid, CPubKey &cpkOut);
int SecureMsgGetLocalKey(CKeyID &ckid, CPubKey &cpkOut);
int SecureMsgGetLocalPublicKey(std::string &strAddress, std::string &strPublicKey);

//int SecureMsgAddAddress(CKeyID &address, CPubKey &publicKey); // TODO: necessary?
int SecureMsgAddAddress(std::string &address, std::string &publicKey);

int SecureMsgRetrieve(SecMsgToken &token, std::vector<uint8_t> &vchData);

int SecureMsgReceive(CNode *pfrom, std::vector<uint8_t> &vchData);

int SecureMsgStoreUnscanned(uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload);
int SecureMsgStore(uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload, bool fUpdateBucket);
int SecureMsgStore(SecureMessage& smsg, bool fUpdateBucket);

int SecureMsgSend(CKeyID &addressFrom, CKeyID &addressTo, std::string &message, std::string &sError, bool fPaid=false, size_t nDaysRetention=0, bool fTestFee=false, CAmount *nFee=NULL);

int SecureMsgHash(const SecureMessage &smsg, uint160 &hash);
int SecureMsgFund(SecureMessage &smsg, std::string &sError, bool fTestFee, CAmount *nFee);

int SecureMsgValidate(uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload);
int SecureMsgSetHash (uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload);

int SecureMsgEncrypt(SecureMessage &smsg, const CKeyID &addressFrom, const CKeyID &addressTo, const std::string &message);

int SecureMsgDecrypt(bool fTestOnly, CKeyID &address, uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload, MessageData &msg);
int SecureMsgDecrypt(bool fTestOnly, CKeyID &address, SecureMessage &smsg, MessageData &msg);

#endif // SEC_MESSAGE_H

