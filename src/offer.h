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
bool CheckOfferInputs(const CTransaction &tx, int op, int nOut, const std::vector<std::vector<unsigned char> > &vvchArgs, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, std::string &errorMessage, bool dontaddtodb=false);


bool DecodeOfferTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseOfferTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeOfferScript(const CScript& script, int& op, std::vector<std::vector<unsigned char> > &vvch);
bool IsOfferOp(int op);
int IndexOfOfferOutput(const CTransaction& tx);
std::string offerFromOp(int op);
void OfferTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry);
bool RemoveOfferScriptPrefix(const CScript& scriptIn, CScript& scriptOut);
#define PAYMENTOPTION_SYS 0x01
#define PAYMENTOPTION_BTC 0x02
#define PAYMENTOPTION_ZEC 0x04

bool ValidatePaymentOptionsMask(const uint64_t &paymentOptionsMask);
bool ValidatePaymentOptionsString(const std::string &paymentOptionsString);
bool IsValidPaymentOption(const uint64_t &paymentOptionsMask);
uint64_t GetPaymentOptionsMaskFromString(const std::string &paymentOptionsString);
bool IsPaymentOptionInMask(const uint64_t &mask, const uint64_t &paymentOption);
std::string GetPaymentOptionsString(const uint64_t &paymentOptions);
CChainParams::AddressType PaymentOptionToAddressType(const uint64_t &paymentOptions);

class COfferLinkWhitelistEntry {
public:
	std::vector<unsigned char> aliasLinkVchRand;
	unsigned char nDiscountPct;
	COfferLinkWhitelistEntry() {
		SetNull();
	}

	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(aliasLinkVchRand);
		READWRITE(VARINT(nDiscountPct));
	}

    inline friend bool operator==(const COfferLinkWhitelistEntry &a, const COfferLinkWhitelistEntry &b) {
        return (
           a.aliasLinkVchRand == b.aliasLinkVchRand
		&& a.nDiscountPct == b.nDiscountPct
        );
    }

    inline COfferLinkWhitelistEntry operator=(const COfferLinkWhitelistEntry &b) {
    	aliasLinkVchRand = b.aliasLinkVchRand;
		nDiscountPct = b.nDiscountPct;
        return *this;
    }

    inline friend bool operator!=(const COfferLinkWhitelistEntry &a, const COfferLinkWhitelistEntry &b) {
        return !(a == b);
    }

    inline void SetNull() { aliasLinkVchRand.clear(); nDiscountPct = 0;}
    inline bool IsNull() const { return (aliasLinkVchRand.empty() && nDiscountPct == 0); }

};
class COfferLinkWhitelist {
public:
	std::vector<COfferLinkWhitelistEntry> entries;
	COfferLinkWhitelist() {
		SetNull();
	}

	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(entries);
	}
	bool GetLinkEntryByHash(const std::vector<unsigned char> &ahash, COfferLinkWhitelistEntry &entry) const;
    	
    inline bool RemoveWhitelistEntry(const std::vector<unsigned char> &ahash) {
    	for(unsigned int i=0;i<entries.size();i++) {
    		if(entries[i].aliasLinkVchRand == ahash) {
    			return entries.erase(entries.begin()+i) != entries.end();
    		}
    	}
    	return false;
    }
    inline void PutWhitelistEntry(const COfferLinkWhitelistEntry &theEntry) {
    	for(unsigned int i=0;i<entries.size();i++) {
    		COfferLinkWhitelistEntry entry = entries[i];
    		if(theEntry.aliasLinkVchRand == entry.aliasLinkVchRand) {
    			entries[i] = theEntry;
    			return;
    		}
    	}
    	entries.push_back(theEntry);
    }
    inline friend bool operator==(const COfferLinkWhitelist &a, const COfferLinkWhitelist &b) {
        return (
           a.entries == b.entries
        );
    }

    inline COfferLinkWhitelist operator=(const COfferLinkWhitelist &b) {
    	entries = b.entries;
        return *this;
    }

    inline friend bool operator!=(const COfferLinkWhitelist &a, const COfferLinkWhitelist &b) {
        return !(a == b);
    }

    inline void SetNull() { entries.clear();}
    inline bool IsNull() const { return (entries.empty());}

};

class COffer {

public:

	std::vector<unsigned char> vchOffer;
	CNameTXIDTuple aliasTuple;
    uint256 txHash;
    uint64_t nHeight;
	std::string sPrice;
	float fUnits;
	char nCommission;
	int nQty;
	CNameTXIDTuple linkOfferTuple;
	CNameTXIDTuple linkAliasTuple;
	std::vector<unsigned char> sCurrencyCode;
	CNameTXIDTuple certTuple;
	COfferLinkWhitelist linkWhitelist;
	bool bPrivate;
	bool bCoinOffer;
	uint64_t paymentOptions;
	unsigned char paymentPrecision;
	unsigned int nSold;
	std::vector<unsigned char> sCategory;
	std::vector<unsigned char> sTitle;
	std::vector<unsigned char> sDescription;
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
		linkWhitelist.SetNull();
		sCategory.clear();
		sTitle.clear();
		sDescription.clear();
		linkOfferTuple.first.clear();
		linkAliasTuple.first.clear();
		certTuple.first.clear();
		sCurrencyCode.clear();
		sPrice.clear();
	}

 	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
			READWRITE(sCategory);
			READWRITE(sTitle);
			READWRITE(sDescription);
			READWRITE(txHash);
			READWRITE(VARINT(nHeight));
			READWRITE(sPrice);
    		READWRITE(nQty);
			READWRITE(VARINT(nSold));
			READWRITE(VARINT(paymentPrecision));
			READWRITE(fUnits);
			READWRITE(linkOfferTuple);
			READWRITE(linkWhitelist);
			READWRITE(sCurrencyCode);
			READWRITE(nCommission);
			READWRITE(aliasTuple);
			READWRITE(certTuple);
			READWRITE(bPrivate);
			READWRITE(bCoinOffer);
			READWRITE(VARINT(paymentOptions));
			READWRITE(vchOffer);
			READWRITE(linkAliasTuple);
	}
	double GetPrice(const COfferLinkWhitelistEntry& entry = COfferLinkWhitelistEntry(), bool display = false) const;

    inline friend bool operator==(const COffer &a, const COffer &b) {
		return (
			a.sCategory == b.sCategory
			&& a.sTitle == b.sTitle
			&& a.sDescription == b.sDescription
			&& a.sPrice == b.sPrice
			&& a.nQty == b.nQty
			&& a.fUnits == b.fUnits
			&& a.nSold == b.nSold
			&& a.txHash == b.txHash
			&& a.nHeight == b.nHeight
			&& a.linkOfferTuple == b.linkOfferTuple
			&& a.linkAliasTuple == b.linkAliasTuple
			&& a.linkWhitelist == b.linkWhitelist
			&& a.sCurrencyCode == b.sCurrencyCode
			&& a.nCommission == b.nCommission
			&& a.aliasTuple == b.aliasTuple
			&& a.certTuple == b.certTuple
			&& a.bPrivate == b.bPrivate
			&& a.bCoinOffer == b.bCoinOffer
			&& a.paymentOptions == b.paymentOptions
			&& a.vchOffer == b.vchOffer
			&& a.paymentPrecision == b.paymentPrecision
        );
    }

    inline COffer operator=(const COffer &b) {
        sCategory = b.sCategory;
        sTitle = b.sTitle;
        sDescription = b.sDescription;
		sPrice = b.sPrice;
		nQty = b.nQty;
		nSold = b.nSold;
        fUnits = b.fUnits;
        txHash = b.txHash;
        nHeight = b.nHeight;
		linkOfferTuple = b.linkOfferTuple;
		linkAliasTuple = b.linkAliasTuple;
		linkWhitelist = b.linkWhitelist;
		sCurrencyCode = b.sCurrencyCode;
		nCommission = b.nCommission;
		aliasTuple = b.aliasTuple;
		certTuple = b.certTuple;
		bPrivate = b.bPrivate;
		bCoinOffer = b.bCoinOffer;
		paymentOptions = b.paymentOptions;
		vchOffer = b.vchOffer;
		paymentPrecision = b.paymentPrecision;
        return *this;
    }

    inline friend bool operator!=(const COffer &a, const COffer &b) {
        return !(a == b);
    }

	inline void SetNull() { paymentPrecision = 0; sCategory.clear(); sTitle.clear(); vchOffer.clear(); sDescription.clear(); bCoinOffer = false; sPrice.clear(); fUnits = 1; nHeight = nQty = nSold = paymentOptions = 0; txHash.SetNull(); bPrivate = false; linkOfferTuple.first.clear(); aliasTuple.first.clear(); linkWhitelist.SetNull(); sCurrencyCode.clear(); nCommission = 0; aliasTuple.first.clear(); certTuple.first.clear(); }
    inline bool IsNull() const { return (paymentPrecision == 0 && sCategory.empty() && sTitle.empty() && sPrice.empty() && sDescription.empty() && vchOffer.empty() && !bCoinOffer && fUnits == 1 && aliasTuple.first.empty() && txHash.IsNull() && nHeight == 0 && paymentOptions == 0 && nQty == 0 && nSold ==0 && linkWhitelist.IsNull() && nCommission == 0 && bPrivate == false && paymentOptions == 0 && sCurrencyCode.empty() && linkOfferTuple.first.empty() && linkAliasTuple.first.empty() && certTuple.first.empty() ); }

    bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};

class COfferDB : public CDBWrapper {
public:
	COfferDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "offers", nCacheSize, fMemory, fWipe) {}

	bool WriteOffer(const COffer& offer) {
		bool writeState = WriteOfferLastTXID(offer.vchOffer, offer.txHash) && Write(make_pair(std::string("offeri"), CNameTXIDTuple(offer.vchOffer, offer.txHash)), offer);
		WriteOfferIndex(offer);
		return writeState;
	}

	bool EraseOffer(const CNameTXIDTuple& offerTuple) {
		bool eraseState = Erase(make_pair(std::string("offeri"), offerTuple));
		EraseOfferLastTXID(offerTuple.first);
		EraseOfferIndex(offerTuple.first);
	    return eraseState;
	}

	bool ReadOffer(const CNameTXIDTuple& offerTuple, COffer& offer) {
		return Read(make_pair(std::string("offeri"), offerTuple), offer);
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
	void WriteOfferIndex(const COffer& offer);
	void EraseOfferIndex(const std::vector<unsigned char>& vchOffer);

};
bool GetOffer(const CNameTXIDTuple& offerTuple, COffer& txPos);
bool GetOffer(const std::vector<unsigned char> &vchOffer, COffer& txPos);
bool BuildOfferJson(const COffer& theOffer, UniValue& oOffer);
uint64_t GetOfferExpiration(const COffer& offer);
#endif // OFFER_H
