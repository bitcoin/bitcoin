// Copyright (c) 2015-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
bool CheckEscrowInputs(const CTransaction &tx, int op, int nOut, const std::vector<std::vector<unsigned char> > &vvchArgs, bool fJustCheck, int nHeight, std::string &errorMessage,  bool dontaddtodb=false);
bool DecodeEscrowTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseEscrowTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch, char &type);
bool DecodeEscrowScript(const CScript& script, int& op, std::vector<std::vector<unsigned char> > &vvch);
bool IsEscrowOp(int op);
void EscrowTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry);
std::string escrowFromOp(int op);
bool RemoveEscrowScriptPrefix(const CScript& scriptIn, CScript& scriptOut);
enum EscrowRoles {
	BUYER = 1,
	SELLER = 2,
	ARBITER = 3,
};
class CEscrow {
public:
	std::vector<unsigned char> vchEscrow;
	std::vector<unsigned char> vchWitness;
	CNameTXIDTuple sellerAliasTuple;
	CNameTXIDTuple linkSellerAliasTuple;
	CNameTXIDTuple arbiterAliasTuple;
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
	bool bBuyNow;
	CAmount nWitnessFee;
	CAmount nCommission;
	CAmount nArbiterFee;
	CAmount nNetworkFee;
	CAmount nDeposit;
	CAmount nShipping;
	CAmount nAmountOrBidPerUnit;
	float fBidPerUnit;
	std::vector<unsigned char> vchRedeemScript;
	unsigned char role;
	inline void ClearEscrow()
	{
		feedback.SetNull();
		linkSellerAliasTuple.first.clear();
		linkAliasTuple.first.clear();
		offerTuple.first.clear();
		scriptSigs.clear();
		vchWitness.clear();
		vchRedeemScript.clear();
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
		READWRITE(vchWitness);
        READWRITE(offerTuple);
		READWRITE(scriptSigs);
		READWRITE(txHash);
		READWRITE(extTxId);
		READWRITE(redeemTxId);
		READWRITE(VARINT(nHeight));
		READWRITE(VARINT(nQty));
		READWRITE(VARINT(op));
		READWRITE(VARINT(nPaymentOption));
		READWRITE(VARINT(role));
        READWRITE(buyerAliasTuple);
		READWRITE(vchEscrow);
		READWRITE(linkAliasTuple);
		READWRITE(feedback);
		READWRITE(bPaymentAck);
		READWRITE(bBuyNow);
		READWRITE(nArbiterFee);
		READWRITE(nNetworkFee);
		READWRITE(nCommission);
		READWRITE(nWitnessFee);
		READWRITE(fBidPerUnit);
		READWRITE(nDeposit);
		READWRITE(vchRedeemScript);
		READWRITE(nShipping);
		READWRITE(nAmountOrBidPerUnit);
	}

    inline friend bool operator==(const CEscrow &a, const CEscrow &b) {
		return (
			a.buyerAliasTuple == b.buyerAliasTuple
			&& a.sellerAliasTuple == b.sellerAliasTuple
			&& a.linkSellerAliasTuple == b.linkSellerAliasTuple
			&& a.arbiterAliasTuple == b.arbiterAliasTuple
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
			&& a.bBuyNow == b.bBuyNow
			&& a.fBidPerUnit == b.fBidPerUnit
			&& a.nDeposit == b.nDeposit
			&& a.nArbiterFee == b.nArbiterFee
			&& a.nNetworkFee == b.nNetworkFee
			&& a.nCommission == b.nCommission
			&& a.nWitnessFee == b.nWitnessFee
			&& a.vchWitness == b.vchWitness
			&& a.vchRedeemScript == b.vchRedeemScript
			&& a.nShipping == b.nShipping
			&& a.nAmountOrBidPerUnit == b.nAmountOrBidPerUnit
			&& a.role == b.role
        );
    }

    inline CEscrow operator=(const CEscrow &b) {
        buyerAliasTuple = b.buyerAliasTuple;
		sellerAliasTuple = b.sellerAliasTuple;
		linkSellerAliasTuple = b.linkSellerAliasTuple;
		arbiterAliasTuple = b.arbiterAliasTuple;
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
		bBuyNow = b.bBuyNow;
		fBidPerUnit = b.fBidPerUnit;
		nArbiterFee = b.nArbiterFee;
		nNetworkFee = b.nNetworkFee;
		nCommission = b.nCommission;
		nShipping = b.nShipping;
		nWitnessFee = b.nWitnessFee;
		vchWitness = b.vchWitness;
		vchRedeemScript = b.vchRedeemScript;
		nAmountOrBidPerUnit = b.nAmountOrBidPerUnit;
		role = b.role;
        return *this;
    }

    inline friend bool operator!=(const CEscrow &a, const CEscrow &b) {
        return !(a == b);
    }
	inline void SetNull() { role = 0; nAmountOrBidPerUnit = 0; vchWitness.clear();  fBidPerUnit = 0;  nDeposit = nArbiterFee = nNetworkFee = nCommission = nShipping = nWitnessFee = 0; extTxId.SetNull(); op = 0; bPaymentAck = bBuyNow = false; redeemTxId.SetNull(); linkAliasTuple.first.clear(); feedback.SetNull(); linkSellerAliasTuple.first.clear(); vchEscrow.clear(); nHeight = nPaymentOption = 0; txHash.SetNull(); nQty = 0; buyerAliasTuple.first.clear(); arbiterAliasTuple.first.clear(); sellerAliasTuple.first.clear(); offerTuple.first.clear(); scriptSigs.clear(); vchRedeemScript.clear(); }
	inline bool IsNull() const { return (vchEscrow.empty()); }
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
	void WriteEscrowBid(const CEscrow& escrow) {
		WriteEscrowBidIndex(escrow, "valid");
	}
	void RefundEscrowBid(const std::vector<unsigned char> &vchEscrow) {
		RefundEscrowBidIndex(vchEscrow, "refunded");
	}
    bool EraseEscrow(const CNameTXIDTuple& escrowTuple, bool cleanup = false) {
		bool eraseState = Erase(make_pair(std::string("escrowi"), escrowTuple));
		EraseEscrowLastTXID(escrowTuple.first);
		EraseEscrowIndex(escrowTuple.first, cleanup);
		EraseEscrowFeedbackIndex(escrowTuple.first, cleanup);
		EraseEscrowBidIndex(escrowTuple.first, cleanup);
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
	void EraseEscrowIndex(const std::vector<unsigned char>& vchEscrow, bool cleanup);
	void WriteEscrowFeedbackIndex(const CEscrow& escrow);
	void EraseEscrowFeedbackIndex(const std::vector<unsigned char>& vchEscrow, bool cleanup);
	void WriteEscrowBidIndex(const CEscrow& escrow, const std::string& status);
	void RefundEscrowBidIndex(const std::vector<unsigned char>& vchEscrow, const std::string& status);
	void EraseEscrowBidIndex(const std::vector<unsigned char>& vchEscrow, bool cleanup);
};

bool GetEscrow(const CNameTXIDTuple &escrowTuple, CEscrow& txPos);
bool GetEscrow(const std::vector<unsigned char> &vchEscrow, CEscrow& txPos);
bool BuildEscrowJson(const CEscrow &escrow, UniValue& oEscrow);
bool BuildEscrowIndexerJson(const CEscrow &escrow, UniValue& oEscrow);
void BuildEscrowBidJson(const CEscrow& escrow, const std::string& status, UniValue& oBid);
void BuildFeedbackJson(const CEscrow& escrow, UniValue& oFeedback);
int64_t GetEscrowArbiterFee(const int64_t &escrowValue, const float &fEscrowFee);
int64_t GetEscrowWitnessFee(const int64_t &escrowValue, const float &fWitnessFee);
int64_t GetEscrowDepositFee(const int64_t &escrowValue, const float &fDepositPercentage);
uint64_t GetEscrowExpiration(const CEscrow& escrow);
#endif // ESCROW_H
