#ifndef ESCROW_H
#define ESCROW_H

#include "rpc/server.h"
#include "dbwrapper.h"
#include "feedback.h"
class CWalletTx;
class CTransaction;
class CReserveKey;
class CCoinsViewCache;
class CCoins;
class CBlock;
bool CheckEscrowInputs(const CTransaction &tx, int op, int nOut, const std::vector<std::vector<unsigned char> > &vvchArgs, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, std::string &errorMessage,  bool dontaddtodb=false);
bool DecodeEscrowTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseEscrowTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeEscrowScript(const CScript& script, int& op, std::vector<std::vector<unsigned char> > &vvch);
bool IsEscrowOp(int op);
int IndexOfEscrowOutput(const CTransaction& tx);
void EscrowTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry);
std::string escrowFromOp(int op);
CScript RemoveEscrowScriptPrefix(const CScript& scriptIn);
class CEscrow {
public:
	std::vector<unsigned char> vchEscrow;
	std::vector<unsigned char> vchSellerAlias;
	std::vector<unsigned char> vchLinkSellerAlias;
	std::vector<unsigned char> vchArbiterAlias;
	std::vector<unsigned char> vchRedeemScript;
	std::vector<unsigned char> vchOffer;
	std::vector<unsigned char> vchPaymentMessage;
	std::vector<unsigned char> rawTx;
	std::vector<unsigned char> vchBuyerAlias;
	std::vector<unsigned char> vchLinkAlias;
	std::vector<CFeedback> feedback;
    uint256 txHash;
	uint256 extTxId;
	uint256 redeemTxId;
    uint64_t nHeight;
	uint64_t nAcceptHeight;
	uint32_t nPaymentOption;
	unsigned int nQty;
	unsigned int op;
	bool bPaymentAck;
	void ClearEscrow()
	{
		feedback.clear();
		vchLinkSellerAlias.clear();
		vchLinkAlias.clear();
		vchRedeemScript.clear();
		vchLinkAlias.clear();
		vchOffer.clear();
		rawTx.clear();
		vchPaymentMessage.clear();
	}
    CEscrow() {
        SetNull();
    }
    CEscrow(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
		READWRITE(vchSellerAlias);
		READWRITE(vchLinkSellerAlias);
		READWRITE(vchArbiterAlias);
		READWRITE(vchRedeemScript);
        READWRITE(vchOffer);
		READWRITE(vchPaymentMessage);
		READWRITE(rawTx);
		READWRITE(txHash);
		READWRITE(extTxId);
		READWRITE(redeemTxId);
		READWRITE(VARINT(nHeight));
		READWRITE(VARINT(nAcceptHeight));
		READWRITE(VARINT(nQty));
		READWRITE(VARINT(op));
		READWRITE(VARINT(nPaymentOption));
        READWRITE(vchBuyerAlias);
		READWRITE(vchEscrow);
		READWRITE(vchLinkAlias);
		READWRITE(feedback);
		READWRITE(bPaymentAck);
	}

    friend bool operator==(const CEscrow &a, const CEscrow &b) {
        return (
        a.vchBuyerAlias == b.vchBuyerAlias
		&& a.vchSellerAlias == b.vchSellerAlias
		&& a.vchLinkSellerAlias == b.vchLinkSellerAlias
		&& a.vchArbiterAlias == b.vchArbiterAlias
		&& a.vchRedeemScript == b.vchRedeemScript
        && a.vchOffer == b.vchOffer
		&& a.vchPaymentMessage == b.vchPaymentMessage
		&& a.rawTx == b.rawTx
		&& a.extTxId == b.extTxId
		&& a.txHash == b.txHash
		&& a.nHeight == b.nHeight
		&& a.nAcceptHeight == b.nAcceptHeight
		&& a.nQty == b.nQty
		&& a.nPaymentOption == b.nPaymentOption
		&& a.vchLinkAlias == b.vchLinkAlias
		&& a.vchEscrow == b.vchEscrow
		&& a.op == b.op
		&& a.feedback == b.feedback
		&& a.redeemTxId == b.redeemTxId
		&& a.bPaymentAck == b.bPaymentAck
        );
    }

    CEscrow operator=(const CEscrow &b) {
        vchBuyerAlias = b.vchBuyerAlias;
		vchSellerAlias = b.vchSellerAlias;
		vchLinkSellerAlias = b.vchLinkSellerAlias;
		vchArbiterAlias = b.vchArbiterAlias;
		vchRedeemScript = b.vchRedeemScript;
        vchOffer = b.vchOffer;
		vchPaymentMessage = b.vchPaymentMessage;
		rawTx = b.rawTx;
		extTxId = b.extTxId;
		txHash = b.txHash;
		vchLinkAlias = b.vchLinkAlias;
		nHeight = b.nHeight;
		nAcceptHeight = b.nAcceptHeight;
		nQty = b.nQty;
		nPaymentOption = b.nPaymentOption;
		vchEscrow = b.vchEscrow;
		op = b.op;
		feedback = b.feedback;
		redeemTxId = b.redeemTxId;
		bPaymentAck = b.bPaymentAck;
        return *this;
    }

    friend bool operator!=(const CEscrow &a, const CEscrow &b) {
        return !(a == b);
    }
    void SetNull() { extTxId.SetNull(); op = 0; bPaymentAck = false; redeemTxId.SetNull(); vchLinkAlias.clear(); feedback.clear(); vchLinkSellerAlias.clear(); vchEscrow.clear(); nHeight = nPaymentOption = nAcceptHeight = 0; txHash.SetNull(); nQty = 0; vchBuyerAlias.clear(); vchArbiterAlias.clear(); vchSellerAlias.clear(); vchRedeemScript.clear(); vchOffer.clear(); rawTx.clear(); vchPaymentMessage.clear();}
    bool IsNull() const { return (extTxId.IsNull() && bPaymentAck == false && redeemTxId.IsNull() && vchLinkSellerAlias.empty() && vchLinkAlias.empty() && feedback.empty() && op == 0 && vchEscrow.empty() && txHash.IsNull() && nHeight == 0 && nPaymentOption == 0 && nAcceptHeight == 0 && nQty == 0 && vchBuyerAlias.empty() && vchArbiterAlias.empty() && vchSellerAlias.empty() && vchRedeemScript.empty() && vchOffer.empty() && rawTx.empty() && vchPaymentMessage.empty()); }
    bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};


class CEscrowDB : public CDBWrapper {
public:
    CEscrowDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "escrow", nCacheSize, fMemory, fWipe) {}

    bool WriteEscrow(const std::vector<unsigned char>& name, const std::vector<CEscrow>& vtxPos) {
        return Write(make_pair(std::string("escrowi"), name), vtxPos);
    }
    bool WriteEscrowTx(const std::vector<unsigned char>& name, const uint256& txid) {
        return Write(make_pair(std::string("escrowt"), txid), name);
    }
    bool EraseEscrow(const std::vector<unsigned char>& name) {
        return Erase(make_pair(std::string("escrowi"), name));
    }

    bool ReadEscrow(const std::vector<unsigned char>& name, std::vector<CEscrow>& vtxPos) {
        return Read(make_pair(std::string("escrowi"), name), vtxPos);
    }

    bool ExistsEscrow(const std::vector<unsigned char>& name) {
        return Exists(make_pair(std::string("escrowi"), name));
    }
    bool ExistsEscrowTx(const uint256& txid) {
        return Exists(make_pair(std::string("escrowt"), txid));
    }
    bool ScanEscrows(
		const std::vector<unsigned char>& vchEscrow, const std::string& strRegExp, const std::vector<std::string>& aliasArray,
            unsigned int nMax,
            std::vector<std::pair<CEscrow, CEscrow> >& escrowScan);
	bool CleanupDatabase(int &servicesCleaned);
};

bool GetTxOfEscrow(const std::vector<unsigned char> &vchEscrow, CEscrow& txPos, CTransaction& tx);
bool GetTxAndVtxOfEscrow(const std::vector<unsigned char> &vchEscrow, CEscrow& txPos, CTransaction& tx, std::vector<CEscrow> &vtxPos);
bool GetVtxOfEscrow(const std::vector<unsigned char> &vchEscrow, CEscrow& txPos, std::vector<CEscrow> &vtxPos);
void HandleEscrowFeedback(const CEscrow& serializedEscrow, CEscrow& dbEscrow, std::vector<CEscrow> &vtxPos);
bool BuildEscrowJson(const CEscrow &escrow, const CEscrow &firstEscrow, UniValue& oEscrow, const std::string &strPrivKey="");
uint64_t GetEscrowExpiration(const CEscrow& escrow);
#endif // ESCROW_H
