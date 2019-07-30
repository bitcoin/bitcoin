// Copyright (c) 2014 The ShadowCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef SEC_MESSAGE_H
#define SEC_MESSAGE_H

#include <leveldb/db.h>
#include <leveldb/write_batch.h>
 
#include <base58.h>
#include <net.h>
#include <db.h>
#include <pubkey.h>
#include <streams.h>
#include <ui_interface.h>
#include <wallet/wallet.h>
#include <smessage/lz4.h>


typedef std::vector<unsigned char, secure_allocator<unsigned char> > secure_buffer;
typedef std::basic_string<char, std::char_traits<char>, secure_allocator<char> > secure_string;


const unsigned int SMSG_HDR_LEN         = 84;                // length of unencrypted header, 4 + 4 + 8 + 33 + 3 + 32
const unsigned int SMSG_PL_HDR_LEN      = 4+33+1+65;         // length of encrypted header in payload

const unsigned int SMSG_BUCKET_LEN      = 60 * 10;           // in seconds
const unsigned int SMSG_RETENTION       = 60 * 60 * 24 * 90;      // in seconds
const unsigned int SMSG_SEND_DELAY      = 2;                 // in seconds, SecureMsgSendData will delay this long between firing
const unsigned int SMSG_THREAD_DELAY    = 5;

const unsigned int SMSG_TIME_LEEWAY     = 60;
const unsigned int SMSG_TIME_IGNORE     = 90;                // seconds that a peer is ignored for if they fail to deliver messages for a smsgWant


const unsigned int SMSG_MAX_MSG_BYTES   = 4096;              // the user input part

// max size of payload worst case compression
const unsigned int SMSG_MAX_MSG_WORST = LZ4_COMPRESSBOUND(SMSG_MAX_MSG_BYTES+SMSG_PL_HDR_LEN);



#define SMSG_MASK_UNREAD            (1 << 0)



extern bool fSecMsgEnabled;

class SecMsgStored;

// Inbox db changed, called with lock cs_smsgDB held.
extern boost::signals2::signal<void (SecMsgStored& inboxHdr)> NotifySecMsgInboxChanged;

// Outbox db changed, called with lock cs_smsgDB held.
extern boost::signals2::signal<void (SecMsgStored& outboxHdr)> NotifySecMsgOutboxChanged;

// Wallet Unlocked, called after all messages received while locked have been processed.
extern boost::signals2::signal<void ()> NotifySecMsgWalletUnlocked;


class SecMsgBucket;
class SecMsgAddress;
class SecMsgOptions;

extern std::map<int64_t, SecMsgBucket>  smsgBuckets;
extern std::vector<SecMsgAddress>       smsgAddresses;
extern SecMsgOptions                    smsgOptions;

extern CCriticalSection cs_smsg;            // all except inbox and outbox
extern CCriticalSection cs_smsgDB;


struct PayloadHeader {
    unsigned char  cpkS[33]; // public key for reply
    unsigned char  lenPlain[4];
    unsigned char  sigKeyVersion[1]; // version of public key hash derived form signature
    unsigned char  signature[65]; // provides authentication, signature[0] == 0 -> anon message
};


struct SecureMessageHeader {
    uint32_t        nVersion;
    uint32_t        nPayload;
    int64_t         timestamp;
    unsigned char   cpkR[33];
    unsigned char   reserved[3];
    unsigned char   mac[32];

    unsigned char   hash[32];

    SecureMessageHeader() {}
    SecureMessageHeader(const unsigned char *header) {
        memcpy(this, header, SMSG_HDR_LEN);
    }
    
    unsigned char *begin() const {
        return (unsigned char*) &nVersion;
    }
    unsigned char *end() const {
        return (unsigned char*) (hash + sizeof(hash));
    }


    unsigned int GetSerializeSize(int, int=0) const
    {
        return (char*) hash - (char*) this;
    }

    template<typename Stream>
    void Serialize(Stream& s, int, int=0) const
    {
        s.write((char*) this, (char*) hash - (char*) this);
    }

    template<typename Stream>
    void Unserialize(Stream& s, int, int=0)
    {
        s.read((char*) this, (char*) hash - (char*) this);
    }
};


class SecureMessage : public SecureMessageHeader {
public:
    std::vector<unsigned char> vchPayload;

    SecureMessageHeader *Header() {
        return (SecureMessageHeader *) this;
    }

    SecureMessage() { }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(*this->Header());
        READWRITE(this->vchPayload);
    }
};


class MessageData
{
// -- Decrypted SecureMessage data
public:
    int64_t        timestamp;
    secure_string  sToAddress;
    secure_string  sFromAddress;
    secure_string  sMessage;
};


class SecMsgToken
{
public:
    SecMsgToken(int64_t ts, const unsigned char* p, int np, long int o)
    {
        timestamp = ts;

        if (np < 8) // payload will always be > 8, just make sure
            memset(sample, 0, 8);
        else
            memcpy(sample, p, 8);
        offset = o;
    };

    SecMsgToken() {};

    ~SecMsgToken() {};

    bool operator <(const SecMsgToken& y) const
    {
        // pack and memcmp from timesent?
        if (timestamp == y.timestamp)
            return memcmp(sample, y.sample, 8) < 0;
        return timestamp < y.timestamp;
    }

    int64_t                     timestamp;
    unsigned char               sample[8];    // a message hash
    int64_t                     offset;       // offset

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
    ~SecMsgBucket() {};

    void hashBucket();

    int64_t                     timeChanged;
    uint32_t                    hash;           // token set should get ordered the same on each node
    uint32_t                    nLockCount;     // set when smsgWant first sent, unset at end of smsgMsg, ticks down in ThreadSecureMsg()
    uint32_t                    nLockPeerId;    // id of peer that bucket is locked for
    std::set<SecMsgToken>       setTokens;

};


class SecMsgAddress
{
public:
    SecMsgAddress() {};
    SecMsgAddress(std::string sAddr, bool receiveOn, bool receiveAnon)
    {
        sAddress            = sAddr;
        fReceiveEnabled     = receiveOn;
        fReceiveAnon        = receiveAnon;
    };

    std::string     sAddress;
    bool            fReceiveEnabled;
    bool            fReceiveAnon;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(this->sAddress);
        READWRITE(this->fReceiveEnabled);
        READWRITE(this->fReceiveAnon);
    }
};

class SecMsgOptions
{
public:
    SecMsgOptions()
    {
        // -- default options
        fNewAddressRecv = true;
        fNewAddressAnon = true;
    }

    bool fNewAddressRecv;
    bool fNewAddressAnon;
};


class SecMsgStored
{
public:
    int64_t                         timeReceived;
    char                            status;         // read etc
    uint16_t                        folderId;
    std::string                     sAddrTo;        // when in owned addr, when sent remote addr
    std::string                     sAddrOutbox;    // owned address this copy was encrypted with
    std::vector<unsigned char>      vchMessage;     // message header + encryped payload

	ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(this->timeReceived);
        READWRITE(this->status);
        READWRITE(this->folderId);
        READWRITE(this->sAddrTo);
        READWRITE(this->sAddrOutbox);
        READWRITE(this->vchMessage);
    }
};

class SecMsgDB
{
public:
    SecMsgDB()
    {
        activeBatch = NULL;
    };

    ~SecMsgDB()
    {
        // -- deletes only data scoped to this TxDB object.

        if (activeBatch)
            delete activeBatch;
    };

    bool Open(const char* pszMode="r+");

    bool ScanBatch(const CDataStream& key, std::string* value, bool* deleted) const;

    bool TxnBegin();
    bool TxnCommit();
    bool TxnAbort();

    bool ReadPK(CKeyID& addr, CPubKey& pubkey);
    bool WritePK(CKeyID& addr, CPubKey& pubkey);
    bool ExistsPK(CKeyID& addr);

    bool NextSmesg(leveldb::Iterator* it, std::string& prefix, unsigned char* vchKey, SecMsgStored& smsgStored);
    bool NextSmesgKey(leveldb::Iterator* it, std::string& prefix, unsigned char* vchKey);
    bool ReadSmesg(unsigned char* chKey, SecMsgStored& smsgStored);
    bool WriteSmesg(unsigned char* chKey, SecMsgStored& smsgStored);
    bool ExistsSmesg(unsigned char* chKey);
    bool EraseSmesg(unsigned char* chKey);

    leveldb::DB *pdb;       // points to the global instance
    leveldb::WriteBatch *activeBatch;

};


int SecureMsgBuildBucketSet();
int SecureMsgAddWalletAddresses();

int SecureMsgReadIni();
int SecureMsgWriteIni();

bool SecureMsgStart(bool fDontStart, bool fScanChain);
bool SecureMsgStop();

bool SecureMsgEnable();
bool SecureMsgDisable();

bool SecureMsgReceiveData(CNode* pfrom, std::string strCommand, CDataStream& vRecv);
bool SecureMsgSendData(CNode* pto, bool fSendTrickle);


bool SecureMsgScanBlock(CBlock& block);
bool ScanChainForPublicKeys(CBlockIndex* pindexStart, size_t n);
bool SecureMsgScanBlockChain();
int SecureMsgScanBuckets(std::string &, bool = false);


static inline int SecureMsgWalletUnlocked() {
    std::string error;
    return SecureMsgScanBuckets(error, true);
}
int SecureMsgWalletKeyChanged(std::string sAddress, std::string sLabel, ChangeType mode);

int SecureMsgScanMessage(const SecureMessageHeader &smsg, const unsigned char *pPayload, bool reportToGui);

int SecureMsgGetStoredKey(CKeyID& ckid, CPubKey& cpkOut);
int SecureMsgGetLocalKey(CKeyID& ckid, CPubKey& cpkOut);
int SecureMsgGetLocalPublicKey(std::string& strAddress, std::string& strPublicKey);

int SecureMsgAddAddress(std::string& address, std::string& publicKey);

int SecureMsgRetrieve(SecMsgToken &token, SecureMessage &smsg);

int SecureMsgReceive(CNode* pfrom, std::vector<unsigned char>& vchData);

int SecureMsgStoreUnscanned(const SecureMessageHeader &smsg, const unsigned char *pPayload);
int SecureMsgStore(const SecureMessageHeader &smsg, const unsigned char *pPayload, bool fUpdateBucket);
int SecureMsgStore(const SecureMessage &smsg, bool fUpdateBucket);


int SecureMsgSend(std::string& addressFrom, std::string& addressTo, std::string& message, std::string& sError);

int SecureMsgValidate(const SecureMessageHeader &smsg, size_t nPayload);

int SecureMsgEncrypt(SecureMessage& smsg, std::string& addressFrom, std::string& addressTo, std::string& message);

int SecureMsgDecrypt(bool fTestOnly, const std::string& address, const SecureMessageHeader& smsg, const unsigned char *pPayload, MessageData& msg);
int SecureMsgDecrypt(const SecMsgStored& smsgStored, MessageData &msg, std::string &errorMsg);


#endif // SEC_MESSAGE_H

