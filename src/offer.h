// Copyright (c) 2015-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef OFFER_H
#define OFFER_H

#include <math.h>
#include "rpc/server.h"
#include "dbwrapper.h"
#include "chainparams.h"
#include "script/script.h"
#include "serialize.h"

class CWalletTx;
class CTransaction;
class CReserveKey;
class CCoinsViewCache;
class CCoins;
class CBlockIndex;
class CBlock;
class CAliasIndex;
class COfferLinkWhitelistEntry;

bool CheckOfferInputs(const CTransaction &tx, int op, int nOut, const std::vector<std::vector<unsigned char> > &vvchArgs, const std::vector<std::vector<unsigned char> > &vvchAliasArgs, bool fJustCheck, int nHeight, std::string &errorMessage, bool dontaddtodb=false);


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
	std::vector<unsigned char> vchAlias;
    uint256 txHash;
    uint64_t nHeight;
	float fPrice;
	float fUnits;
	char nCommission;
	int nQty;
	std::vector<unsigned char> vchLinkOffer;
	std::vector<unsigned char> sCurrencyCode;
	std::vector<unsigned char> vchCert;
	bool bPrivate;
	uint32_t offerType;
	uint64_t paymentOptions;
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
		vchLinkOffer.clear();
		vchCert.clear();
		sCurrencyCode.clear();
		auctionOffer.SetNull();
		vchAlias.clear();
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
			READWRITE(fUnits);
			READWRITE(vchLinkOffer);
			READWRITE(sCurrencyCode);
			READWRITE(nCommission);
			READWRITE(vchAlias);
			READWRITE(vchCert);
			READWRITE(bPrivate);
			READWRITE(VARINT(offerType));
			READWRITE(VARINT(paymentOptions));
			READWRITE(vchOffer);
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
        fUnits = b.fUnits;
        txHash = b.txHash;
        nHeight = b.nHeight;
		vchLinkOffer = b.vchLinkOffer;
		sCurrencyCode = b.sCurrencyCode;
		nCommission = b.nCommission;
		vchAlias = b.vchAlias;
		vchCert = b.vchCert;
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

	inline void SetNull() { auctionOffer.SetNull();  sCategory.clear(); sTitle.clear(); vchOffer.clear(); sDescription.clear(); offerType = 0; fPrice = 0; fUnits = 1; nHeight = nQty = paymentOptions = 0; txHash.SetNull(); bPrivate = false; vchLinkOffer.clear(); vchAlias.clear(); sCurrencyCode.clear(); nCommission = 0; vchAlias.clear(); vchCert.clear(); }
    inline bool IsNull() const { return (vchOffer.empty()); }

    bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};

class COfferDB : public CDBWrapper {
public:
	COfferDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "offers", nCacheSize, fMemory, fWipe) {}

	bool WriteOffer(const COffer& offer, const int &op, const bool& fJustCheck) {
		bool writeState = Write(make_pair(std::string("offeri"), offer.vchOffer), offer);
		if (!fJustCheck)
			writeState = writeState && Write(make_pair(std::string("offerp"), offer.vchOffer), offer);
		else if (fJustCheck)
			writeState = writeState && Write(make_pair(std::string("offerl"), offer.vchOffer), fJustCheck);
		WriteOfferIndex(offer, op);
		return writeState;
	}

	bool EraseOffer(const std::vector<unsigned char>& vchOffer, bool cleanup = false) {
		bool eraseState = Erase(make_pair(std::string("offeri"), vchOffer));
		Erase(make_pair(std::string("offerp"), vchOffer));
		EraseISLock(vchOffer);
		EraseOfferIndex(vchOffer, cleanup);
	    return eraseState;
	}

	bool ReadOffer(const  std::vector<unsigned char>& vchOffer, COffer& offer) {
		return Read(make_pair(std::string("offeri"), vchOffer), offer);
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
	bool WriteExtTXID(const uint256& txid) {
		return Write(make_pair(std::string("offert"), txid), txid);
	}
	bool ExistsExtTXID(const uint256& txid) {
		return Exists(make_pair(std::string("offert"), txid));
	}
	bool EraseExtTXID(const uint256& txid) {
		return Erase(make_pair(std::string("offert"), txid));
	}
	bool CleanupDatabase(int &servicesCleaned);
	void WriteOfferIndex(const COffer& offer, const int &op);
	void EraseOfferIndex(const std::vector<unsigned char>& vchOffer, bool cleanup);
	void WriteOfferIndexHistory(const COffer& offer, const int &op);
	void EraseOfferIndexHistory(const std::vector<unsigned char>& vchOffer, bool cleanup);
	void EraseOfferIndexHistory(const std::string& id);

};
bool GetOffer(const std::vector<unsigned char> &vchOffer, COffer& txPos);
bool BuildOfferJson(const COffer& theOffer, UniValue& oOffer);
bool BuildOfferIndexerJson(const COffer& theOffer, UniValue& oOffer);
bool BuildOfferIndexerHistoryJson(const COffer& theOffer, UniValue& oOffer);
uint64_t GetOfferExpiration(const COffer& offer);
#endif // OFFER_H
