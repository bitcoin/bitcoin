#ifndef ALIAS_H
#define ALIAS_H

#include "rpcserver.h"
#include "dbwrapper.h"
#include "script/script.h"
#include "serialize.h"
#include "consensus/params.h"
class CWalletTx;
class CTransaction;
class CTxOut;
class COutPoint;
class CReserveKey;
class CCoinsViewCache;
class CCoins;
class CBlock;
struct CRecipient;
class CSyscoinAddress;
static const unsigned int MAX_NAME_LENGTH = 255;
static const unsigned int MAX_VALUE_LENGTH = 1023;
static const unsigned int MAX_ID_LENGTH = 20;
static const unsigned int MAX_ENCRYPTED_VALUE_LENGTH = 1108;

class CAliasIndex {
public:
    uint256 txHash;
    int64_t nHeight;
    std::vector<unsigned char> vchPublicValue;
	std::vector<unsigned char> vchPrivateValue;
	std::vector<unsigned char> vchPubKey;
    CAliasIndex() { 
        SetNull();
    }
    CAliasIndex(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
	void ClearAlias()
	{
		vchPublicValue.clear();
		vchPrivateValue.clear();
	}
	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {        
		READWRITE(txHash);
        READWRITE(VARINT(nHeight));
    	READWRITE(vchPublicValue);
		READWRITE(vchPrivateValue);
		READWRITE(vchPubKey);
	}

    friend bool operator==(const CAliasIndex &a, const CAliasIndex &b) {
		return (a.nHeight == b.nHeight && a.txHash == b.txHash && a.vchPublicValue == b.vchPrivateValue && a.vchPubKey == b.vchPubKey);
    }

    friend bool operator!=(const CAliasIndex &a, const CAliasIndex &b) {
        return !(a == b);
    }
    CAliasIndex operator=(const CAliasIndex &b) {
        txHash = b.txHash;
        nHeight = b.nHeight;
        vchPublicValue = b.vchPublicValue;
        vchPrivateValue = b.vchPrivateValue;
        vchPubKey = b.vchPubKey;
        return *this;
    }   
    void SetNull() { txHash.SetNull(); nHeight = 0; vchPublicValue.clear(); vchPrivateValue.clear(); vchPubKey.clear(); }
    bool IsNull() const { return (nHeight == 0 && txHash.IsNull() && vchPublicValue.empty() && vchPrivateValue.empty() && vchPubKey.empty()); }
	bool UnserializeFromTx(const CTransaction &tx);
	const std::vector<unsigned char> Serialize();
};

class CAliasDB : public CDBWrapper {
public:
    CAliasDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "aliases", nCacheSize, fMemory, fWipe) {
    }

	bool WriteAlias(const std::vector<unsigned char>& name, const std::vector<unsigned char>& address, std::vector<CAliasIndex>& vtxPos) {
		return Write(make_pair(std::string("namei"), name), vtxPos) && Write(make_pair(std::string("namea"), address), name);
	}

	bool EraseAlias(const std::vector<unsigned char>& name) {
	    return Erase(make_pair(std::string("namei"), name));
	}
	bool ReadAlias(const std::vector<unsigned char>& name, std::vector<CAliasIndex>& vtxPos) {
		return Read(make_pair(std::string("namei"), name), vtxPos);
	}
	bool ReadAddress(const std::vector<unsigned char>& address, std::vector<unsigned char>& name) {
		return Read(make_pair(std::string("namea"), address), name);
	}
	bool ExistsAlias(const std::vector<unsigned char>& name) {
	    return Exists(make_pair(std::string("namei"), name));
	}
	bool ExistsAddress(const std::vector<unsigned char>& address) {
	    return Exists(make_pair(std::string("namea"), address));
	}
    bool ScanNames(
            const std::vector<unsigned char>& vchName,
            unsigned int nMax,
            std::vector<std::pair<std::vector<unsigned char>, CAliasIndex> >& nameScan);

    bool ReconstructAliasIndex(CBlockIndex *pindexRescan);
};

class COfferDB;
class CCertDB;
class CEscrowDB;
class CMessageDB;
extern CAliasDB *paliasdb;
extern COfferDB *pofferdb;
extern CCertDB *pcertdb;
extern CEscrowDB *pescrowdb;
extern CMessageDB *pmessagedb;



std::string stringFromVch(const std::vector<unsigned char> &vch);
std::vector<unsigned char> vchFromValue(const UniValue& value);
std::vector<unsigned char> vchFromString(const std::string &str);
std::string stringFromValue(const UniValue& value);
bool IsCompressedOrUncompressedPubKey(const std::vector<unsigned char> &vchPubKey);
int GetSyscoinTxVersion();
const int SYSCOIN_TX_VERSION = 0x7400;
bool CheckAliasInputs(const CTransaction &tx, int op, int nOut, const vector<vector<unsigned char> > &vvchArgs, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, const CBlock *block = NULL);
void CreateRecipient(const CScript& scriptPubKey, CRecipient& recipient);
void CreateFeeRecipient(const CScript& scriptPubKey, const std::vector<unsigned char>& data, CRecipient& recipient);
bool IsSyscoinTxMine(const CTransaction& tx,const std::string &type);
bool IsAliasOp(int op);


bool GetTxOfAlias(const std::vector<unsigned char> &vchName, CAliasIndex& alias, CTransaction& tx);
int IndexOfAliasOutput(const CTransaction& tx);
bool GetAliasOfTx(const CTransaction& tx, std::vector<unsigned char>& name);
bool DecodeAliasTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseAliasTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseSyscoinTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAliasScript(const CScript& script, int& op,
		std::vector<std::vector<unsigned char> > &vvch);
void GetAddressFromAlias(const std::string& strAlias, std::string& strAddress);
void GetAliasFromAddress(const std::string& strAddress, std::string& strAlias);
CAmount convertCurrencyCodeToSyscoin(const std::vector<unsigned char> &vchCurrencyCode, const float &nPrice, const unsigned int &nHeight, int &precision);
bool ExistsInMempool(const std::vector<unsigned char> &vchToFind, opcodetype type);
unsigned int QtyOfPendingAcceptsInMempool(const std::vector<unsigned char>& vchToFind);
std::string getCurrencyToSYSFromAlias(const std::vector<unsigned char> &vchCurrency, CAmount &nFee, const unsigned int &nHeightToFind, std::vector<std::string>& rateList, int &precision);
std::string aliasFromOp(int op);
bool IsAliasOp(int op);
int GetAliasExpirationDepth();
CScript RemoveAliasScriptPrefix(const CScript& scriptIn);
int GetSyscoinDataOutput(const CTransaction& tx);
bool IsSyscoinDataOutput(const CTxOut& out);
bool GetSyscoinData(const CTransaction &tx, std::vector<unsigned char> &vchData);
bool GetSyscoinTransaction(int nHeight, const uint256 &hash, CTransaction &txOut, const Consensus::Params& consensusParams);
bool IsSyscoinScript(const CScript& scriptPubKey, int &op, std::vector<std::vector<unsigned char> > &vvchArgs);
void RemoveSyscoinScript(const CScript& scriptPubKeyIn, CScript& scriptPubKeyOut);
bool GetPreviousInput(const COutPoint * outpoint, int &op, std::vector<std::vector<unsigned char> > &vvchArgs);
#endif // ALIAS_H
