#ifndef MESSAGE_H
#define MESSAGE_H

#include "rpcserver.h"
#include "dbwrapper.h"
#include "script/script.h"
#include "serialize.h"
class CWalletTx;
class CTransaction;
class CReserveKey;
class CCoinsViewCache;
class CCoins;
class CBlock;

bool CheckMessageInputs( const CTransaction &tx, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, CBlock const* block = NULL);
bool DecodeMessageTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseMessageTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeMessageScript(const CScript& script, int& op, std::vector<std::vector<unsigned char> > &vvch);
bool IsMessageOp(int op);
int IndexOfMessageOutput(const CTransaction& tx);
int GetMessageExpirationDepth();
bool ExtractMessageAddress(const CScript& script, std::string& address);
CScript RemoveMessageScriptPrefix(const CScript& scriptIn);

std::string messageFromOp(int op);


class CMessage {
public:
	std::vector<unsigned char> vchPubKeyTo;
	std::vector<unsigned char> vchPubKeyFrom;
	std::vector<unsigned char> vchSubject;
	std::vector<unsigned char> vchMessageTo;
	std::vector<unsigned char> vchMessageFrom;
    uint256 txHash;
    uint64_t nHeight;
    CMessage() {
        SetNull();
    }
    CMessage(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vchPubKeyTo);
		READWRITE(vchPubKeyFrom);
		READWRITE(vchSubject);
		READWRITE(vchMessageTo);
		READWRITE(vchMessageFrom);
		READWRITE(txHash);
		READWRITE(nHeight);
	}

    friend bool operator==(const CMessage &a, const CMessage &b) {
        return (
        a.vchPubKeyTo == b.vchPubKeyTo
		&& a.vchPubKeyFrom == b.vchPubKeyFrom
		&& a.vchSubject == b.vchSubject
		&& a.vchMessageTo == b.vchMessageTo
		&& a.vchMessageFrom == b.vchMessageFrom
		&& a.txHash == b.txHash
		&& a.nHeight == b.nHeight
        );
    }

    CMessage operator=(const CMessage &b) {
        vchPubKeyTo = b.vchPubKeyTo;
		vchPubKeyFrom = b.vchPubKeyFrom;
		vchSubject = b.vchSubject;
		vchMessageTo = b.vchMessageTo;
		vchMessageFrom = b.vchMessageFrom;
		txHash = b.txHash;
		nHeight = b.nHeight;
        return *this;
    }

    friend bool operator!=(const CMessage &a, const CMessage &b) {
        return !(a == b);
    }

    void SetNull() { txHash.SetNull(); nHeight = 0; vchPubKeyTo.clear(); vchPubKeyFrom.clear(); vchSubject.clear(); vchMessageTo.clear();vchMessageFrom.clear();}
    bool IsNull() const { return (txHash.IsNull() && nHeight == 0 && vchPubKeyTo.empty() && vchPubKeyFrom.empty()); }
    bool UnserializeFromTx(const CTransaction &tx);
	const std::vector<unsigned char> Serialize();
};


class CMessageDB : public CDBWrapper {
public:
    CMessageDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "message", nCacheSize, fMemory, fWipe) {}

    bool WriteMessage(const std::vector<unsigned char>& name, std::vector<CMessage>& vtxPos) {
        return Write(make_pair(std::string("messagei"), name), vtxPos);
    }

    bool EraseMessage(const std::vector<unsigned char>& name) {
        return Erase(make_pair(std::string("messagei"), name));
    }

    bool ReadMessage(const std::vector<unsigned char>& name, std::vector<CMessage>& vtxPos) {
        return Read(make_pair(std::string("messagei"), name), vtxPos);
    }

    bool ExistsMessage(const std::vector<unsigned char>& name) {
        return Exists(make_pair(std::string("messagei"), name));
    }

    bool ScanMessages(
            const std::vector<unsigned char>& vchName,
            unsigned int nMax,
            std::vector<std::pair<std::vector<unsigned char>, CMessage> >& MessageScan);

    bool ReconstructMessageIndex(CBlockIndex *pindexRescan);
};

bool GetTxOfMessage(const std::vector<unsigned char> &vchMessage, CTransaction& tx);

#endif // MESSAGE_H
