// Copyright (c) 2015-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ESCROW_H
#define ESCROW_H

#include "rpc/server.h"
#include "dbwrapper.h"
#include "feedback.h"
#include "sync.h"
class CWalletTx;
class CTransaction;
class CReserveKey;
class CCoinsViewCache;
class CBlock;
class COffer;
bool CheckEscrowInputs(const CTransaction &tx, int op,const std::vector<std::vector<unsigned char> > &vvchArgs, const std::vector<std::vector<unsigned char> > &vvchAliasArgs, bool fJustCheck, int nHeight, std::string &errorMessage, bool bSanityCheck=false);
bool DecodeEscrowTx(const CTransaction& tx, int& op, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseEscrowTx(const CTransaction& tx, int& op, std::vector<std::vector<unsigned char> >& vvch, char &type);
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
enum EscrowStatus {
	Unknown = 0,
	InEscrow = 1,
	EscrowReleased = 2,
	EscrowReleaseComplete = 3,
	EscrowRefunded = 4,
	EscrowRefundComplete = 5
};
class CEscrow {
public:
	std::vector<unsigned char> vchEscrow;
	std::vector<unsigned char> vchWitness;
	std::vector<unsigned char> vchSellerAlias;
	std::vector<unsigned char> vchLinkSellerAlias;
	std::vector<unsigned char> vchArbiterAlias;
	std::vector<unsigned char> vchOffer;
	std::vector<CScriptBase> scriptSigs;
	std::vector<unsigned char> vchBuyerAlias;
	CFeedback feedback;
    uint256 txHash;
	uint256 extTxId;
	uint256 redeemTxId;
	unsigned int nHeight;
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
		vchLinkSellerAlias.clear();
		vchOffer.clear();
		scriptSigs.clear();
		vchWitness.clear();
		vchRedeemScript.clear();
		vchSellerAlias.clear();
		vchArbiterAlias.clear();
		vchBuyerAlias.clear();
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
    inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(vchSellerAlias);
		READWRITE(vchLinkSellerAlias);
		READWRITE(vchArbiterAlias);
		READWRITE(vchWitness);
        READWRITE(vchOffer);
		READWRITE(scriptSigs);
		READWRITE(txHash);
		READWRITE(extTxId);
		READWRITE(redeemTxId);
		READWRITE(VARINT(nHeight));
		READWRITE(VARINT(nQty));
		READWRITE(VARINT(op));
		READWRITE(VARINT(nPaymentOption));
		READWRITE(VARINT(role));
        READWRITE(vchBuyerAlias);
		READWRITE(vchEscrow);
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
		return (a.vchEscrow == b.vchEscrow
        );
    }

    inline CEscrow operator=(const CEscrow &b) {
        vchBuyerAlias = b.vchBuyerAlias;
		vchSellerAlias = b.vchSellerAlias;
		vchLinkSellerAlias = b.vchLinkSellerAlias;
		vchArbiterAlias = b.vchArbiterAlias;
        vchOffer = b.vchOffer;
		scriptSigs = b.scriptSigs;
		extTxId = b.extTxId;
		txHash = b.txHash;
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
	inline void SetNull() { role = 0; nAmountOrBidPerUnit = 0; vchWitness.clear();  fBidPerUnit = 0;  nDeposit = nArbiterFee = nNetworkFee = nCommission = nShipping = nWitnessFee = 0; extTxId.SetNull(); op = 0; bPaymentAck = bBuyNow = false; redeemTxId.SetNull(); feedback.SetNull(); vchLinkSellerAlias.clear(); vchEscrow.clear(); nHeight = nPaymentOption = 0; txHash.SetNull(); nQty = 0; vchBuyerAlias.clear(); vchArbiterAlias.clear(); vchSellerAlias.clear(); vchOffer.clear(); scriptSigs.clear(); vchRedeemScript.clear(); }
	inline bool IsNull() const { return (vchEscrow.empty()); }
    bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};


class CEscrowDB : public CDBWrapper {
public:
    CEscrowDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "escrow", nCacheSize, fMemory, fWipe) {}

    bool WriteEscrow( const std::vector<std::vector<unsigned char> > &vvchArgs, const COffer &offer, const CEscrow& escrow) {
		bool writeState = false;
		writeState = Write(make_pair(std::string("escrowi"), escrow.vchEscrow), escrow);
		if(writeState)
			WriteEscrowIndex(offer, escrow, vvchArgs);
        return writeState;
    }
	void WriteEscrowBid(const COffer& offer, const CEscrow& escrow, const std::string& status) {
		WriteEscrowBidIndex(offer, escrow, status);
	}
    bool EraseEscrow(const std::vector<unsigned char>& vchEscrow, bool cleanup = false) {
		return Erase(make_pair(std::string("escrowi"), vchEscrow));
    }
    bool ReadEscrow(const std::vector<unsigned char>& vchEscrow, CEscrow& escrow) {
        return Read(make_pair(std::string("escrowi"), vchEscrow), escrow);
    }
	bool ReadEscrowLastTXID(const std::vector<unsigned char>& escrow, uint256& txid) {
		return Read(make_pair(std::string("escrowlt"), escrow), txid);
	}
	bool CleanupDatabase(int &servicesCleaned);
	void WriteEscrowIndex(const COffer& offer, const CEscrow& escrow, const std::vector<std::vector<unsigned char> > &vvchArgs);
	void WriteEscrowFeedbackIndex(const COffer& offer, const CEscrow& escrow);
	void WriteEscrowBidIndex(const COffer& offer, const CEscrow& escrow, const std::string& status);
	void RefundEscrowBidIndex(const std::vector<unsigned char>& vchEscrow, const std::string& status);
	bool ScanEscrows(const int count, const int from, const UniValue& oOptions, UniValue& oRes);
};

bool GetEscrow(const std::vector<unsigned char> &vchEscrow, CEscrow& txPos);
bool BuildEscrowJson(const CEscrow &escrow, UniValue& oEscrow);
bool BuildEscrowIndexerJson(const COffer& offer, const CEscrow &escrow, UniValue& oEscrow);
void BuildEscrowBidJson(const COffer& offer, const CEscrow& escrow, const std::string& status, UniValue& oBid);
void BuildFeedbackJson(const COffer& offer, const CEscrow& escrow, UniValue& oFeedback);
int64_t GetEscrowArbiterFee(const int64_t &escrowValue, const float &fEscrowFee);
int64_t GetEscrowWitnessFee(const int64_t &escrowValue, const float &fWitnessFee);
int64_t GetEscrowDepositFee(const int64_t &escrowValue, const float &fDepositPercentage);
uint64_t GetEscrowExpiration(const CEscrow& escrow);
#endif // ESCROW_H
