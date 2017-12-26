// Copyright (c) 2015-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef OFFER_H
#define OFFER_H

#include <math.h>
#include "rpc/server.h"
#include "dbwrapper.h"
#include "feedback.h"
#include "chainparams.h"

class CWalletTx;
class CTransaction;
class CReserveKey;
class CCoinsViewCache;
class CCoins;
class CBlockIndex;
class CBlock;
class CAliasIndex;
class COfferLinkWhitelistEntry;

bool CheckOfferInputs(const CTransaction &tx, int op, int nOut, const std::vector<std::vector<unsigned char> > &vvchArgs, bool fJustCheck, int nHeight, std::string &errorMessage, bool bInstantSend = false,bool dontaddtodb=false);


bool DecodeOfferTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseOfferTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch, char &type);
bool DecodeOfferScript(const CScript& script, int& op, std::vector<std::vector<unsigned char> > &vvch);
bool IsOfferOp(int op);
std::string offerFromOp(int op);
void OfferTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry);
bool RemoveOfferScriptPrefix(const CScript& scriptIn, CScript& scriptOut);
#define PAYMENTOPTION_SYS 0x01
#define PAYMENTOPTION_BTC 0x02
#define PAYMENTOPTION_ZEC 0x04

#define OFFERTYPE_BUYNOW 0x01
#define OFFERTYPE_COIN 0x02
#define OFFERTYPE_AUCTION 0x04

bool ValidatePaymentOptionsMask(const uint64_t &paymentOptionsMask);
bool ValidatePaymentOptionsString(const std::string &paymentOptionsString);
bool IsValidPaymentOption(const uint64_t &paymentOptionsMask);
uint64_t GetPaymentOptionsMaskFromString(const std::string &paymentOptionsString);
bool IsPaymentOptionInMask(const uint64_t &mask, const uint64_t &paymentOption);
bool ValidateOfferTypeMask(const uint32_t &offerTypeMask);
bool ValidateOfferTypeString(const std::string &offerTypeString);
bool IsValidOfferType(const uint32_t &offerTypeMask);
uint32_t GetOfferTypeMaskFromString(const std::string &offerTypeString);
bool IsOfferTypeInMask(const uint32_t &mask, const uint32_t &offerType);
std::string GetPaymentOptionsString(const uint64_t &paymentOptions);
std::string GetOfferTypeString(const uint32_t &offerType);
CChainParams::AddressType PaymentOptionToAddressType(const uint64_t &paymentOptions);

class CAuctionOffer {
public:
	uint64_t nExpireTime;
	float fReservePrice;
	bool bRequireWitness;
	float fDepositPercentage;
	CAuctionOffer() {
		SetNull();
	}

	ADD_SERIALIZE_METHODS;
	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
		READWRITE(VARINT(nExpireTime));
		READWRITE(fReservePrice);
		READWRITE(bRequireWitness);
		READWRITE(fDepositPercentage);
	}

	inline friend bool operator==(const CAuctionOffer &a, const CAuctionOffer &b) {
		return (
			a.nExpireTime == b.nExpireTime &&
			a.fReservePrice == b.fReservePrice &&
			a.bRequireWitness == b.bRequireWitness &&
			a.fDepositPercentage == b.fDepositPercentage
			);
	}

	inline CAuctionOffer operator=(const CAuctionOffer &b) {
		nExpireTime = b.nExpireTime;
		fReservePrice = b.fReservePrice;
		bRequireWitness = b.bRequireWitness;
		fDepositPercentage = b.fDepositPercentage;
		return *this;
	}

	inline friend bool operator!=(const CAuctionOffer &a, const CAuctionOffer &b) {
		return !(a == b);
	}

	inline void SetNull() { fDepositPercentage = 0; nExpireTime = 0; fReservePrice = 0;  bRequireWitness = false; }
	inline bool IsNull() const { return (fDepositPercentage == 0 && nExpireTime == 0 && fReservePrice == 0 && !bRequireWitness); }

};
class COffer {

public:

	std::vector<unsigned char> vchOffer;
	CNameTXIDTuple aliasTuple;
    uint256 txHash;
    uint64_t nHeight;
	float fPrice;
	float fUnits;
	char nCommission;
	int nQty;
	CNameTXIDTuple linkOfferTuple;
	CNameTXIDTuple linkAliasTuple;
	std::vector<unsigned char> sCurrencyCode;
	CNameTXIDTuple certTuple;
	bool bPrivate;
	uint32_t offerType;
	uint64_t paymentOptions;
	unsigned int nSold;
	std::vector<unsigned char> sCategory;
	std::vector<unsigned char> sTitle;
	std::vector<unsigned char> sDescription;
	CAuctionOffer auctionOffer;
	COffer() {
        SetNull();
    }

    COffer(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
	// clear everything but the necessary information for an offer to prepare it to go into a txn
	inline void ClearOffer()
	{
		sCategory.clear();
		sTitle.clear();
		sDescription.clear();
		linkOfferTuple.first.clear();
		linkAliasTuple.first.clear();
		certTuple.first.clear();
		sCurrencyCode.clear();
		auctionOffer.SetNull();
	}

 	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
			READWRITE(sCategory);
			READWRITE(sTitle);
			READWRITE(sDescription);
			READWRITE(txHash);
			READWRITE(VARINT(nHeight));
			READWRITE(fPrice);
    		READWRITE(nQty);
			READWRITE(VARINT(nSold));
			READWRITE(fUnits);
			READWRITE(linkOfferTuple);
			READWRITE(sCurrencyCode);
			READWRITE(nCommission);
			READWRITE(aliasTuple);
			READWRITE(certTuple);
			READWRITE(bPrivate);
			READWRITE(VARINT(offerType));
			READWRITE(VARINT(paymentOptions));
			READWRITE(vchOffer);
			READWRITE(linkAliasTuple);
			READWRITE(auctionOffer);
	}
	float GetPrice(const int commission=0) const;
    inline friend bool operator==(const COffer &a, const COffer &b) {
		return (a.vchOffer == b.vchOffer
        );
    }

    inline COffer operator=(const COffer &b) {
        sCategory = b.sCategory;
        sTitle = b.sTitle;
        sDescription = b.sDescription;
		fPrice = b.fPrice;
		nQty = b.nQty;
		nSold = b.nSold;
        fUnits = b.fUnits;
        txHash = b.txHash;
        nHeight = b.nHeight;
		linkOfferTuple = b.linkOfferTuple;
		linkAliasTuple = b.linkAliasTuple;
		sCurrencyCode = b.sCurrencyCode;
		nCommission = b.nCommission;
		aliasTuple = b.aliasTuple;
		certTuple = b.certTuple;
		bPrivate = b.bPrivate;
		offerType = b.offerType;
		paymentOptions = b.paymentOptions;
		vchOffer = b.vchOffer;
		auctionOffer = b.auctionOffer;
        return *this;
    }

    inline friend bool operator!=(const COffer &a, const COffer &b) {
        return !(a == b);
    }

	inline void SetNull() { auctionOffer.SetNull();  sCategory.clear(); sTitle.clear(); vchOffer.clear(); sDescription.clear(); offerType = 0; fPrice = 0; fUnits = 1; nHeight = nQty = nSold = paymentOptions = 0; txHash.SetNull(); bPrivate = false; linkOfferTuple.first.clear(); aliasTuple.first.clear(); sCurrencyCode.clear(); nCommission = 0; aliasTuple.first.clear(); certTuple.first.clear(); }
    inline bool IsNull() const { return (vchOffer.empty()); }

    bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};

class COfferDB : public CDBWrapper {
public:
	COfferDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "offers", nCacheSize, fMemory, fWipe) {}

	bool WriteOffer(const COffer& offer, const COffer& prevOffer, const int &op, const bool& bInstantSend) {
		bool writeState = WriteOfferLastTXID(offer.vchOffer, offer.txHash) && Write(make_pair(std::string("offeri"), CNameTXIDTuple(offer.vchOffer, offer.txHash)), offer);
		if (!bInstantSend && !prevOffer.IsNull())
			writeState = writeState && Write(make_pair(std::string("offerp"), offer.vchOffer), prevOffer);
		else if (bInstantSend)
			writeState = writeState && Write(make_pair(std::string("offerl"), offer.vchOffer), bInstantSend);
		WriteOfferIndex(offer, op);
		return writeState;
	}

	bool EraseOffer(const CNameTXIDTuple& offerTuple, bool cleanup = false) {
		bool eraseState = Erase(make_pair(std::string("offeri"), offerTuple));
		Erase(make_pair(std::string("offerp"), offerTuple.first));
		EraseISLock(offerTuple.first);
		EraseOfferLastTXID(offerTuple.first);
		EraseOfferIndex(offerTuple.first, cleanup);
	    return eraseState;
	}

	bool ReadOffer(const CNameTXIDTuple& offerTuple, COffer& offer) {
		return Read(make_pair(std::string("offeri"), offerTuple), offer);
	}
	bool ReadLastOffer(const std::vector<unsigned char>& vchGuid, COffer& offer) {
		return Read(make_pair(std::string("offerp"), vchGuid), offer);
	}
	bool ReadISLock(const std::vector<unsigned char>& vchGuid, bool& lock) {
		return Read(make_pair(std::string("offerl"), vchGuid), lock);
	}
	bool EraseISLock(const std::vector<unsigned char>& vchGuid) {
		return Erase(make_pair(std::string("offerl"), vchGuid));
	}
	bool WriteOfferLastTXID(const std::vector<unsigned char>& offer, const uint256& txid) {
		return Write(make_pair(std::string("offerlt"), offer), txid);
	}
	bool ReadOfferLastTXID(const std::vector<unsigned char>& offer, uint256& txid) {
		return Read(make_pair(std::string("offerlt"), offer), txid);
	}
	bool EraseOfferLastTXID(const std::vector<unsigned char>& offer) {
		return Erase(make_pair(std::string("offerlt"), offer));
	}
	bool WriteExtTXID(const uint256& txid) {
		return Write(make_pair(std::string("offert"), txid), txid);
	}
	bool ExistsExtTXID(const uint256& txid) {
		return Exists(make_pair(std::string("offert"), txid));
	}

	bool CleanupDatabase(int &servicesCleaned);
	void WriteOfferIndex(const COffer& offer, const int &op);
	void EraseOfferIndex(const std::vector<unsigned char>& vchOffer, bool cleanup);
	void WriteOfferIndexHistory(const COffer& offer, const int &op);
	void EraseOfferIndexHistory(const std::vector<unsigned char>& vchOffer, bool cleanup);

};
bool GetOffer(const CNameTXIDTuple& offerTuple, COffer& txPos);
bool GetOffer(const std::vector<unsigned char> &vchOffer, COffer& txPos);
bool BuildOfferJson(const COffer& theOffer, UniValue& oOffer);
bool BuildOfferIndexerJson(const COffer& theOffer, UniValue& oOffer);
bool BuildOfferIndexerHistoryJson(const COffer& theOffer, UniValue& oOffer);
uint64_t GetOfferExpiration(const COffer& offer);
#endif // OFFER_H
