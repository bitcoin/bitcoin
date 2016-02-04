// Copyright (c) 2014 The ShadowCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef SEC_MESSAGE_H
#define SEC_MESSAGE_H

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#include "net.h"
#include "db.h"
#include "wallet.h"
#include "lz4/lz4.h"


const unsigned int SMSG_HDR_LEN         = 104;               // length of unencrypted header, 4 + 2 + 1 + 8 + 16 + 33 + 32 + 4 +4
const unsigned int SMSG_PL_HDR_LEN      = 1+20+65+4;         // length of encrypted header in payload

const unsigned int SMSG_BUCKET_LEN      = 60 * 10;           // in seconds
const unsigned int SMSG_RETENTION       = 60 * 60 * 48;      // in seconds
const unsigned int SMSG_SEND_DELAY      = 2;                 // in seconds, SecureMsgSendData will delay this long between firing
const unsigned int SMSG_THREAD_DELAY    = 20;

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


#pragma pack(push, 1)
class SecureMessage
{
public:
    SecureMessage()
    {
        nPayload = 0;
        pPayload = NULL;
    };

    ~SecureMessage()
    {
        if (pPayload)
            delete[] pPayload;
        pPayload = NULL;
    };

    uint8_t   hash[4];
    uint8_t   version[2];
    uint8_t   flags;
    int64_t   timestamp;
    uint8_t   iv[16];
    uint8_t   cpkR[33];
    uint8_t   mac[32];
    uint8_t   nonse[4];
    uint32_t  nPayload;
    uint8_t*  pPayload;

};
#pragma pack(pop)


class MessageData
{
// -- Decrypted SecureMessage data
public:
    int64_t               timestamp;
    std::string           sToAddress;
    std::string           sFromAddress;
    std::vector<uint8_t>  vchMessage;         // null terminated plaintext
};


class SecMsgToken
{
public:
    SecMsgToken(int64_t ts, uint8_t* p, int np, long int o)
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

    int64_t               timestamp;    // doesn't need to be full 64 bytes?
    uint8_t               sample[8];    // first 8 bytes of payload - a hash
    int64_t               offset;       // offset

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
    NodeId                      nLockPeerId;    // id of peer that bucket is locked for
    std::set<SecMsgToken>       setTokens;

};


// -- get at the data
class CBitcoinAddress_B : public CBitcoinAddress
{
public:
    uint8_t getVersion()
    {
        // TODO: fix
        if (vchVersion.size() > 0)
            return vchVersion[0];
        return 0;
    }
};

class CKeyID_B : public CKeyID
{
public:
    uint32_t* GetPPN()
    {
        return pn;
    }
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

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->sAddress);
        READWRITE(this->fReceiveEnabled);
        READWRITE(this->fReceiveAnon);
    );
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


class SecMsgCrypter
{
private:
    uint8_t chKey[32];
    uint8_t chIV[16];
    bool fKeySet;
public:

    SecMsgCrypter()
    {
        // Try to keep the key data out of swap (and be a bit over-careful to keep the IV that we don't even use out of swap)
        // Note that this does nothing about suspend-to-disk (which will put all our key data on disk)
        // Note as well that at no point in this program is any attempt made to prevent stealing of keys by reading the memory of the running process.
        LockedPageManager::instance.LockRange(&chKey[0], sizeof chKey);
        LockedPageManager::instance.LockRange(&chIV[0], sizeof chIV);
        fKeySet = false;
    }

    ~SecMsgCrypter()
    {
        // clean key
        memset(&chKey, 0, sizeof chKey);
        memset(&chIV, 0, sizeof chIV);
        fKeySet = false;

        LockedPageManager::instance.UnlockRange(&chKey[0], sizeof chKey);
        LockedPageManager::instance.UnlockRange(&chIV[0], sizeof chIV);
    }

    bool SetKey(const std::vector<uint8_t>& vchNewKey, uint8_t* chNewIV);
    bool SetKey(const uint8_t* chNewKey, uint8_t* chNewIV);
    bool Encrypt(uint8_t* chPlaintext, uint32_t nPlain, std::vector<uint8_t> &vchCiphertext);
    bool Decrypt(uint8_t* chCiphertext, uint32_t nCipher, std::vector<uint8_t>& vchPlaintext);
};


class SecMsgStored
{
public:
    int64_t                   timeReceived;
    char                      status;         // read etc
    uint16_t                  folderId;
    std::string               sAddrTo;        // when in owned addr, when sent remote addr
    std::string               sAddrOutbox;    // owned address this copy was encrypted with
    std::vector<uint8_t>      vchMessage;     // message header + encryped payload

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->timeReceived);
        READWRITE(this->status);
        READWRITE(this->folderId);
        READWRITE(this->sAddrTo);
        READWRITE(this->sAddrOutbox);
        READWRITE(this->vchMessage);
    );
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

    bool NextSmesg(leveldb::Iterator* it, std::string& prefix, uint8_t* vchKey, SecMsgStored& smsgStored);
    bool NextSmesgKey(leveldb::Iterator* it, std::string& prefix, uint8_t* vchKey);
    bool ReadSmesg(uint8_t* chKey, SecMsgStored& smsgStored);
    bool WriteSmesg(uint8_t* chKey, SecMsgStored& smsgStored);
    bool ExistsSmesg(uint8_t* chKey);
    bool EraseSmesg(uint8_t* chKey);

    leveldb::DB *pdb;       // points to the global instance
    leveldb::WriteBatch *activeBatch;

};


int SecureMsgBuildBucketSet();
int SecureMsgAddWalletAddresses();

int SecureMsgReadIni();
int SecureMsgWriteIni();

bool SecureMsgStart(bool fDontStart, bool fScanChain);
bool SecureMsgShutdown();

bool SecureMsgEnable();
bool SecureMsgDisable();

bool SecureMsgReceiveData(CNode* pfrom, std::string strCommand, CDataStream& vRecv);
bool SecureMsgSendData(CNode* pto, bool fSendTrickle);


bool SecureMsgScanBlock(CBlock& block);
bool ScanChainForPublicKeys(CBlockIndex* pindexStart);
bool SecureMsgScanBlockChain();
bool SecureMsgScanBuckets();


int SecureMsgWalletUnlocked();
int SecureMsgWalletKeyChanged(std::string sAddress, std::string sLabel, ChangeType mode);

int SecureMsgScanMessage(uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload, bool reportToGui);

int SecureMsgGetStoredKey(CKeyID& ckid, CPubKey& cpkOut);
int SecureMsgGetLocalKey(CKeyID& ckid, CPubKey& cpkOut);
int SecureMsgGetLocalPublicKey(std::string& strAddress, std::string& strPublicKey);

int SecureMsgAddAddress(std::string& address, std::string& publicKey);

int SecureMsgRetrieve(SecMsgToken &token, std::vector<uint8_t>& vchData);

int SecureMsgReceive(CNode* pfrom, std::vector<uint8_t>& vchData);

int SecureMsgStoreUnscanned(uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload);
int SecureMsgStore(uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload, bool fUpdateBucket);
int SecureMsgStore(SecureMessage& smsg, bool fUpdateBucket);



int SecureMsgSend(std::string& addressFrom, std::string& addressTo, std::string& message, std::string& sError);

int SecureMsgValidate(uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload);
int SecureMsgSetHash(uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload);

int SecureMsgEncrypt(SecureMessage& smsg, std::string& addressFrom, std::string& addressTo, std::string& message);

int SecureMsgDecrypt(bool fTestOnly, std::string& address, uint8_t *pHeader, uint8_t *pPayload, uint32_t nPayload, MessageData& msg);
int SecureMsgDecrypt(bool fTestOnly, std::string& address, SecureMessage& smsg, MessageData& msg);



#endif // SEC_MESSAGE_H

