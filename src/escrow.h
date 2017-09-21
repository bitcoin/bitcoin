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
bool RemoveEscrowScriptPrefix(const CScript& scriptIn, CScript& scriptOut);
class CEscrow {
public:
	std::vector<unsigned char> vchEscrow;
	CNameTXIDTuple sellerAliasTuple;
	CNameTXIDTuple linkSellerAliasTuple;
	CNameTXIDTuple arbiterAliasTuple;
	std::vector<unsigned char> vchRedeemScript;
	CNameTXIDTuple offerTuple;
	std::vector<CScriptBase> scriptSigs;
	CNameTXIDTuple buyerAliasTuple;
	CNameTXIDTuple linkAliasTuple;
	CFeedback feedback;
    uint256 txHash;
	uint256 extTxId;
	uint256 redeemTxId;
    uint64_t nHeight;
	uint64_t nPaymentOption;
	unsigned int nQty;
	unsigned int op;
	bool bPaymentAck;
	std::string sTotal;
	std::string sCurrencyCode;
	bool bCoinOffer;
	CAmount nShipping;
	CAmount nCommission;
	CAmount nTotal;
	CAmount nArbiterFee;
	CAmount nNetworkFee;
	void ClearEscrow()
	{
		feedback.SetNull();
		linkSellerAliasTuple.first.clear();
		linkAliasTuple.first.clear();
		vchRedeemScript.clear();
		offerTuple.first.clear();
		scriptSigs.clear();
		sCurrencyCode.clear();
		sTotal.clear();
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
		READWRITE(sellerAliasTuple);
		READWRITE(linkSellerAliasTuple);
		READWRITE(arbiterAliasTuple);
		READWRITE(vchRedeemScript);
        READWRITE(offerTuple);
		READWRITE(scriptSigs);
		READWRITE(txHash);
		READWRITE(extTxId);
		READWRITE(redeemTxId);
		READWRITE(VARINT(nHeight));
		READWRITE(VARINT(nQty));
		READWRITE(VARINT(op));
		READWRITE(VARINT(nPaymentOption));
        READWRITE(buyerAliasTuple);
		READWRITE(vchEscrow);
		READWRITE(linkAliasTuple);
		READWRITE(feedback);
		READWRITE(bPaymentAck);
		READWRITE(sTotal);
		READWRITE(nTotal);
		READWRITE(nArbiterFee);
		READWRITE(nNetworkFee);
		READWRITE(sCurrencyCode);
		READWRITE(bCoinOffer);
		READWRITE(nCommission);
		READWRITE(nShipping);
	}

    friend bool operator==(const CEscrow &a, const CEscrow &b) {
		return (
			a.buyerAliasTuple == b.buyerAliasTuple
			&& a.sellerAliasTuple == b.sellerAliasTuple
			&& a.linkSellerAliasTuple == b.linkSellerAliasTuple
			&& a.arbiterAliasTuple == b.arbiterAliasTuple
			&& a.vchRedeemScript == b.vchRedeemScript
			&& a.offerTuple == b.offerTuple
			&& a.scriptSigs == b.scriptSigs
			&& a.extTxId == b.extTxId
			&& a.txHash == b.txHash
			&& a.nHeight == b.nHeight
			&& a.nQty == b.nQty
			&& a.nPaymentOption == b.nPaymentOption
			&& a.linkAliasTuple == b.linkAliasTuple
			&& a.vchEscrow == b.vchEscrow
			&& a.op == b.op
			&& a.feedback == b.feedback
			&& a.redeemTxId == b.redeemTxId
			&& a.bPaymentAck == b.bPaymentAck
			&& a.sTotal == b.sTotal
			&& a.nTotal == b.nTotal
			&& a.nArbiterFee == b.nArbiterFee
			&& a.nNetworkFee == b.nNetworkFee
			&& a.sCurrencyCode == b.sCurrencyCode
			&& a.bCoinOffer == b.bCoinOffer
			&& a.nCommission == b.nCommission
			&& a.nShipping == b.nShipping
        );
    }

    CEscrow operator=(const CEscrow &b) {
        buyerAliasTuple = b.buyerAliasTuple;
		sellerAliasTuple = b.sellerAliasTuple;
		linkSellerAliasTuple = b.linkSellerAliasTuple;
		arbiterAliasTuple = b.arbiterAliasTuple;
		vchRedeemScript = b.vchRedeemScript;
        offerTuple = b.offerTuple;
		scriptSigs = b.scriptSigs;
		extTxId = b.extTxId;
		txHash = b.txHash;
		linkAliasTuple = b.linkAliasTuple;
		nHeight = b.nHeight;
		nQty = b.nQty;
		nPaymentOption = b.nPaymentOption;
		vchEscrow = b.vchEscrow;
		op = b.op;
		feedback = b.feedback;
		redeemTxId = b.redeemTxId;
		bPaymentAck = b.bPaymentAck;
		sTotal = b.sTotal;
		nTotal = b.nTotal;
		nArbiterFee = b.nArbiterFee;
		nNetworkFee = b.nNetworkFee;
		sCurrencyCode = b.sCurrencyCode;
		bCoinOffer = b.bCoinOffer;
		nCommission = b.nCommission;
		nShipping = b.nShipping;
        return *this;
    }

    friend bool operator!=(const CEscrow &a, const CEscrow &b) {
        return !(a == b);
    }
	void SetNull() { sTotal.clear(); sCurrencyCode.clear();  bCoinOffer = false;  nTotal = nArbiterFee = nNetworkFee = nCommission = nShipping = 0; extTxId.SetNull(); op = 0; bPaymentAck = false; redeemTxId.SetNull(); linkAliasTuple.first.clear(); feedback.SetNull(); linkSellerAliasTuple.first.clear(); vchEscrow.clear(); nHeight = nPaymentOption = 0; txHash.SetNull(); nQty = 0; buyerAliasTuple.first.clear(); arbiterAliasTuple.first.clear(); sellerAliasTuple.first.clear(); vchRedeemScript.clear(); offerTuple.first.clear(); scriptSigs.clear(); }
    bool IsNull() const { return (sTotal.empty() && sCurrencyCode.empty() && !bCoinOffer && nTotal == 0 && nArbiterFee == 0 && nCommission == 0 && nNetworkFee == 0 && nShipping == 0 && extTxId.IsNull() && bPaymentAck == false && redeemTxId.IsNull() && linkSellerAliasTuple.first.empty() && linkAliasTuple.first.empty() && feedback.IsNull() && op == 0 && vchEscrow.empty() && txHash.IsNull() && nHeight == 0 && nPaymentOption == 0 && nQty == 0 && buyerAliasTuple.first.empty() && arbiterAliasTuple.first.empty() && sellerAliasTuple.first.empty() && vchRedeemScript.empty() && offerTuple.first.empty() && scriptSigs.empty()); }
    bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};


class CEscrowDB : public CDBWrapper {
public:
    CEscrowDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "escrow", nCacheSize, fMemory, fWipe) {}

    bool WriteEscrow( const std::vector<std::vector<unsigned char> > &vvchArgs, const CEscrow& escrow) {
		bool writeState = WriteEscrowLastTXID(escrow.vchEscrow, escrow.txHash) && Write(make_pair(std::string("escrowi"), CNameTXIDTuple(escrow.vchEscrow, escrow.txHash)), escrow);
		WriteEscrowIndex(escrow, vvchArgs);
        return writeState;
    }
    bool EraseEscrow(const CNameTXIDTuple& escrowTuple) {
		bool eraseState = Erase(make_pair(std::string("escrowi"), escrowTuple));
		EraseEscrowLastTXID(escrowTuple.first);
		EraseEscrowIndex(escrowTuple.first);
		EraseEscrowFeedbackIndex(escrowTuple.first);
        return eraseState;
    }
    bool ReadEscrow(const CNameTXIDTuple& escrowTuple, CEscrow& escrow) {
        return Read(make_pair(std::string("escrowi"), escrowTuple), escrow);
    }
	bool WriteEscrowLastTXID(const std::vector<unsigned char>& escrow, const uint256& txid) {
		return Write(make_pair(std::string("escrowlt"), escrow), txid);
	}
	bool ReadEscrowLastTXID(const std::vector<unsigned char>& escrow, uint256& txid) {
		return Read(make_pair(std::string("escrowlt"), escrow), txid);
	}
	bool EraseEscrowLastTXID(const std::vector<unsigned char>& escrow) {
		return Erase(make_pair(std::string("escrowlt"), escrow));
	}
	bool CleanupDatabase(int &servicesCleaned);
	void WriteEscrowIndex(const CEscrow& escrow, const std::vector<std::vector<unsigned char> > &vvchArgs);
	void EraseEscrowIndex(const std::vector<unsigned char>& vchEscrow);
	void WriteEscrowFeedbackIndex(const CEscrow& escrow);
	void EraseEscrowFeedbackIndex(const std::vector<unsigned char>& vchEscrow);
};

bool GetEscrow(const CNameTXIDTuple &escrowTuple, CEscrow& txPos);
bool GetEscrow(const std::vector<unsigned char> &vchEscrow, CEscrow& txPos);
bool BuildEscrowJson(const CEscrow &escrow, const std::vector<std::vector<unsigned char> > &vvch, UniValue& oEscrow);
void BuildFeedbackJson(const CEscrow& escrow, UniValue& oFeedback);
uint64_t GetEscrowExpiration(const CEscrow& escrow);
#endif // ESCROW_H
