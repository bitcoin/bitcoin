#ifndef MESSAGE_H
#define MESSAGE_H

#include "rpc/server.h"
#include "dbwrapper.h"
#include "script/script.h"
#include "serialize.h"
class CWalletTx;
class CTransaction;
class CReserveKey;
class CCoinsViewCache;
class CCoins;
class CBlock;

bool CheckMessageInputs( const CTransaction &tx, int op, int nOut, const std::vector<std::vector<unsigned char> > &vvchArgs, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, std::string &errorMessage, bool dontaddtodb=false);
bool DecodeMessageTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseMessageTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeMessageScript(const CScript& script, int& op, std::vector<std::vector<unsigned char> > &vvch);
bool IsMessageOp(int op);
int IndexOfMessageOutput(const CTransaction& tx);
bool ExtractMessageAddress(const CScript& script, std::string& address);
bool RemoveMessageScriptPrefix(const CScript& scriptIn, CScript& scriptOut);
void MessageTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry);
std::string messageFromOp(int op);

class CMessage {
public:
	std::vector<unsigned char> vchMessage;
	std::vector<unsigned char> vchAliasTo;
	std::vector<unsigned char> vchAliasFrom;
	std::vector<unsigned char> vchPubData;
	std::vector<unsigned char> vchData;
	// message private data encrypted to this public key
	std::vector<unsigned char> vchEncryptionPublicKey;
	// secret key encrypted to vchAliasFrom(sender) private key
	std::vector<unsigned char> vchEncryptionPrivateKeyFrom;
	// secret key encrypted to vchAliasTo(recipient) private key
	std::vector<unsigned char> vchEncryptionPrivateKeyTo;

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
		READWRITE(vchEncryptionPrivateKeyTo);
		READWRITE(vchEncryptionPrivateKeyFrom);
		READWRITE(txHash);
		READWRITE(VARINT(nHeight));
		READWRITE(vchMessage);
        READWRITE(vchAliasTo);
		READWRITE(vchAliasFrom);
		READWRITE(vchData);
		READWRITE(vchPubData);
	}

    friend bool operator==(const CMessage &a, const CMessage &b) {
        return (
        a.vchAliasTo == b.vchAliasTo
		&& a.vchAliasFrom == b.vchAliasFrom
		&& a.vchEncryptionPrivateKeyTo == b.vchEncryptionPrivateKeyTo
		&& a.vchEncryptionPrivateKeyFrom == b.vchEncryptionPrivateKeyFrom
		&& a.txHash == b.txHash
		&& a.nHeight == b.nHeight
		&& a.vchMessage == b.vchMessage
		&& a.vchData == b.vchData
		&& a.vchPubData == b.vchPubData
        );
    }

    CMessage operator=(const CMessage &b) {
        vchAliasTo = b.vchAliasTo;
		vchAliasFrom = b.vchAliasFrom;
		vchEncryptionPrivateKeyTo = b.vchEncryptionPrivateKeyTo;
		vchEncryptionPrivateKeyFrom = b.vchEncryptionPrivateKeyFrom;
		txHash = b.txHash;
		nHeight = b.nHeight;
		vchMessage = b.vchMessage;
		vchData = b.vchData;
		vchPubData = b.vchPubData;
        return *this;
    }

    friend bool operator!=(const CMessage &a, const CMessage &b) {
        return !(a == b);
    }

    void SetNull() {vchPubData.clear(); vchData.clear(); vchMessage.clear(); txHash.SetNull(); nHeight = 0; vchAliasTo.clear(); vchAliasFrom.clear(); vchEncryptionPrivateKeyFrom.clear();vchEncryptionPrivateKeyTo.clear();}
    bool IsNull() const { return (vchPubData.empty() && vchData.empty() && vchMessage.empty() && txHash.IsNull() && nHeight == 0 && vchAliasTo.empty() && vchAliasFrom.empty()); }
    bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};


class CMessageDB : public CDBWrapper {
public:
    CMessageDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "message", nCacheSize, fMemory, fWipe) {}

    bool WriteMessage(const std::vector<unsigned char>& name, const std::vector<CMessage>& vtxPos) {
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
	bool GetDBMessages(std::vector<CMessage>& message, const uint64_t& nExpireFilter, const std::vector<std::string>& aliasArray);
	bool CleanupDatabase(int &servicesCleaned);

};

bool GetTxOfMessage(const std::vector<unsigned char> &vchMessage, CTransaction& tx);
bool BuildMessageJson(const CMessage& message, UniValue& oName, const std::string &strWalletless="");
uint64_t GetMessageExpiration(const CMessage& message);
bool BuildMessageStatsJson(const std::vector<CMessage> &messages, UniValue& oMessageStats);
#endif // MESSAGE_H
