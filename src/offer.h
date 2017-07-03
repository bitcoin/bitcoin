#ifndef OFFER_H
#define OFFER_H

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

bool ValidatePaymentOptionsMask(const uint32_t paymentOptionsMask);
bool ValidatePaymentOptionsString(const std::string &paymentOptionsString);
bool IsValidPaymentOption(const uint32_t paymentOptionsMask);
uint32_t GetPaymentOptionsMaskFromString(const std::string &paymentOptionsString);
bool IsPaymentOptionInMask(const uint32_t mask, const uint32_t paymentOption);
class COfferAccept {
public:
	std::vector<unsigned char> vchAcceptRand;
	uint64_t nAcceptHeight;
	unsigned int nQty;
	CAmount nPrice;
	uint256 txExtId;
	uint32_t nPaymentOption;
	std::vector<unsigned char> vchBuyerAlias;
	std::vector<unsigned char> vchMessage;
	std::vector<CFeedback> feedback;
	bool bPaymentAck;
	COfferAccept() {
        SetNull();
    }

	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
		READWRITE(vchAcceptRand);
		READWRITE(VARINT(nAcceptHeight));
        READWRITE(VARINT(nQty));
        READWRITE(VARINT(nPaymentOption));
    	READWRITE(nPrice);
		READWRITE(vchBuyerAlias);
		READWRITE(txExtId);
		READWRITE(feedback);
		READWRITE(vchMessage);
		READWRITE(bPaymentAck);
	}

    inline friend bool operator==(const COfferAccept &a, const COfferAccept &b) {
        return (
		a.vchAcceptRand == b.vchAcceptRand
		&& a.nAcceptHeight == b.nAcceptHeight
		&& a.nPaymentOption == b.nPaymentOption
        && a.nQty == b.nQty
        && a.nPrice == b.nPrice
		&& a.vchBuyerAlias == b.vchBuyerAlias
		&& a.txExtId == b.txExtId
		&& a.feedback == b.feedback
		&& a.vchMessage == b.vchMessage
		&& a.bPaymentAck == b.bPaymentAck
        );
    }

    inline COfferAccept operator=(const COfferAccept &b) {
		vchAcceptRand = b.vchAcceptRand;
		nAcceptHeight = b.nAcceptHeight;
        nQty = b.nQty;
        nPaymentOption = b.nPaymentOption;
        nPrice = b.nPrice;
		vchBuyerAlias = b.vchBuyerAlias;
		txExtId = b.txExtId;
		feedback = b.feedback;
		vchMessage = b.vchMessage;
		bPaymentAck = b.bPaymentAck;
        return *this;
    }

    inline friend bool operator!=(const COfferAccept &a, const COfferAccept &b) {
        return !(a == b);
    }

    inline void SetNull() { bPaymentAck = false; vchMessage.clear(); feedback.clear(); vchAcceptRand.clear(); nAcceptHeight = nPaymentOption = nPrice = nQty = 0; txExtId.SetNull(); vchBuyerAlias.clear();}
    inline bool IsNull() const { return (bPaymentAck == false && vchMessage.empty() && feedback.empty() && vchAcceptRand.empty() && nAcceptHeight == 0 && nPrice == 0 && nPaymentOption == 0 && nQty == 0 && txExtId.IsNull() && vchBuyerAlias.empty()); }

};
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
    inline bool GetLinkEntryByHash(const std::vector<unsigned char> &ahash, COfferLinkWhitelistEntry &entry) const {
    	entry.SetNull();
		for(unsigned int i=0;i<entries.size();i++) {
    		if(entries[i].aliasLinkVchRand == ahash) {
    			entry = entries[i];
    			return true;
    		}
    	}
    	return false;
    }
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
	std::vector<unsigned char> vchAlias;
    uint256 txHash;
    uint64_t nHeight;
	std::vector<unsigned char> sCategory;
	std::vector<unsigned char> sTitle;
	std::vector<unsigned char> sDescription;
	CAmount nPrice;
	char nCommission;
	int nQty;
	COfferAccept accept;
	std::vector<unsigned char> vchLinkOffer;
	std::vector<unsigned char> vchLinkAlias;
	std::vector<unsigned char> sCurrencyCode;
	std::vector<unsigned char> vchCert;
	COfferLinkWhitelist linkWhitelist;
	bool bPrivate;
	unsigned int paymentOptions;
	unsigned char safetyLevel;
	unsigned int nSold;
	std::vector<unsigned char> vchGeoLocation;
	bool safeSearch;
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
		accept.SetNull();
		linkWhitelist.SetNull();
		sCategory.clear();
		sTitle.clear();
		sDescription.clear();
		vchLinkOffer.clear();
		vchLinkAlias.clear();
		vchCert.clear();
		vchGeoLocation.clear();
		sCurrencyCode.clear();
	}

 	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
			READWRITE(sCategory);
			READWRITE(sTitle);
			READWRITE(sDescription);
			READWRITE(txHash);
			READWRITE(VARINT(nHeight));
    		READWRITE(nPrice);
    		READWRITE(nQty);
			READWRITE(VARINT(nSold));
    		READWRITE(accept);
			READWRITE(vchLinkOffer);
			READWRITE(linkWhitelist);
			READWRITE(sCurrencyCode);
			READWRITE(nCommission);
			READWRITE(vchAlias);
			READWRITE(vchCert);
			READWRITE(bPrivate);
			READWRITE(VARINT(paymentOptions));
			READWRITE(vchOffer);
			READWRITE(VARINT(safetyLevel));
			READWRITE(safeSearch);
			READWRITE(vchGeoLocation);
			READWRITE(vchLinkAlias);


	}
	inline CAmount GetPrice(const COfferLinkWhitelistEntry& entry=COfferLinkWhitelistEntry()) const{
		COfferLinkWhitelistEntry  myentry;
		CAmount price = nPrice;
		linkWhitelist.GetLinkEntryByHash(entry.aliasLinkVchRand, myentry);

		char nDiscount = myentry.nDiscountPct;
		if(myentry.nDiscountPct > 99)
			nDiscount = 0;
		// nMarkup is a percentage, commission minus discount
		char nMarkup = nCommission - nDiscount;
		if(nMarkup != 0)
		{
			float lMarkup = 1/ (nMarkup/100.0);
			lMarkup = floorf(lMarkup * 100) / 100;
			CAmount priceMarkup = price/lMarkup;
			price += priceMarkup;
		}
		return price;
	}

	inline void SetPrice(CAmount price){
		nPrice = price;
	}
    inline void PutToOfferList(std::vector<COffer> &offerList) {
        for(unsigned int i=0;i<offerList.size();i++) {
            COffer o = offerList[i];
            if(o.txHash == txHash && o.accept.vchAcceptRand == accept.vchAcceptRand) {
                offerList[i] = *this;
                return;
            }
        }
        offerList.push_back(*this);
    }

   inline bool GetOfferFromList(std::vector<COffer> &offerList) {
        if(offerList.size() == 0) return false;
		COffer myOffer = offerList.front();
		if(nHeight <= 0)
		{
			*this = myOffer;
			return true;
		}
		// find the closest offer without going over in height, assuming offerList orders entries by nHeight ascending
        for(std::vector<COffer>::reverse_iterator it = offerList.rbegin(); it != offerList.rend(); ++it) {
            const COffer &o = *it;
			// skip if height is greater than our offer height
			if(o.nHeight > nHeight)
				continue;
            myOffer = o;
			break;
        }
        *this = myOffer;
        return true;
    }
    inline friend bool operator==(const COffer &a, const COffer &b) {
        return (
         a.sCategory==b.sCategory
        && a.sTitle == b.sTitle
        && a.sDescription == b.sDescription
        && a.nPrice == b.nPrice
        && a.nQty == b.nQty
		&& a.nSold == b.nSold
        && a.txHash == b.txHash
        && a.nHeight == b.nHeight
        && a.accept == b.accept
		&& a.vchLinkOffer == b.vchLinkOffer
		&& a.vchLinkAlias == b.vchLinkAlias
		&& a.linkWhitelist == b.linkWhitelist
		&& a.sCurrencyCode == b.sCurrencyCode
		&& a.nCommission == b.nCommission
		&& a.vchAlias == b.vchAlias
		&& a.vchCert == b.vchCert
		&& a.bPrivate == b.bPrivate
		&& a.paymentOptions == b.paymentOptions
		&& a.safetyLevel == b.safetyLevel
		&& a.safeSearch == b.safeSearch
		&& a.vchGeoLocation == b.vchGeoLocation
		&& a.vchOffer == b.vchOffer
        );
    }

    inline COffer operator=(const COffer &b) {
        sCategory = b.sCategory;
        sTitle = b.sTitle;
        sDescription = b.sDescription;
        nPrice = b.nPrice;
        nQty = b.nQty;
		nSold = b.nSold;
        txHash = b.txHash;
        nHeight = b.nHeight;
        accept = b.accept;
		vchLinkOffer = b.vchLinkOffer;
		vchLinkAlias = b.vchLinkAlias;
		linkWhitelist = b.linkWhitelist;
		sCurrencyCode = b.sCurrencyCode;
		nCommission = b.nCommission;
		vchAlias = b.vchAlias;
		vchCert = b.vchCert;
		bPrivate = b.bPrivate;
		paymentOptions = b.paymentOptions;
		safetyLevel = b.safetyLevel;
		safeSearch = b.safeSearch;
		vchGeoLocation = b.vchGeoLocation;
		vchOffer = b.vchOffer;
        return *this;
    }

    inline friend bool operator!=(const COffer &a, const COffer &b) {
        return !(a == b);
    }

    inline void SetNull() { vchOffer.clear(); sCategory.clear(); safetyLevel = nHeight = nPrice = nQty = nSold = paymentOptions = 0; safeSearch = true; txHash.SetNull(); bPrivate = false; accept.SetNull(); sTitle.clear(); sDescription.clear();vchLinkOffer.clear();vchLinkAlias.clear();linkWhitelist.SetNull();sCurrencyCode.clear();nCommission=0;vchAlias.clear();vchCert.clear();vchGeoLocation.clear();}
    inline bool IsNull() const { return (vchOffer.empty() && sCategory.empty() && safetyLevel == 0 && safeSearch && vchAlias.empty() && txHash.IsNull() && nHeight == 0 && nPrice == 0 && paymentOptions == 0 && nQty == 0 && nSold ==0 && linkWhitelist.IsNull() && sTitle.empty() && sDescription.empty() && vchGeoLocation.empty() && nCommission == 0 && bPrivate == false && paymentOptions == 0 && sCurrencyCode.empty() && vchLinkOffer.empty() && vchLinkAlias.empty() && vchCert.empty() ); }

    bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};

class COfferDB : public CDBWrapper {
public:
	COfferDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "offers", nCacheSize, fMemory, fWipe) {}

	bool WriteOffer(const std::vector<unsigned char>& name, const std::vector<COffer>& vtxPos) {
		return Write(make_pair(std::string("offeri"), name), vtxPos);
	}
	bool WriteOfferTx(const std::vector<unsigned char>& name, const uint256& txid) {
		return Write(make_pair(std::string("offert"), txid), name);
	}
	bool EraseOffer(const std::vector<unsigned char>& name) {
	    return Erase(make_pair(std::string("offeri"), name));
	}

	bool ReadOffer(const std::vector<unsigned char>& name, std::vector<COffer>& vtxPos) {
		return Read(make_pair(std::string("offeri"), name), vtxPos);
	}
	bool ExistsOffer(const std::vector<unsigned char>& name) {
	    return Exists(make_pair(std::string("offeri"), name));
	}
	bool ExistsOfferTx(const uint256& txid) {
	    return Exists(make_pair(std::string("offert"), txid));
	}

    bool ScanOffers(
		const std::vector<unsigned char>& vchOffer,const std::string &strRegExp, bool safeSearch,const std::string& strCategory,
            unsigned int nMax,
            std::vector<COffer>& offerScan);
	bool CleanupDatabase(int &servicesCleaned);

};
void HandleAcceptFeedback(const CFeedback& feedback, COffer& offer, std::vector<COffer> &vtxPos);
void FindFeedback(const std::vector<CFeedback> &feedback, int &numBuyerRatings, int &numSellerRatings,int &numArbiterRatings, int &feedbackBuyerCount, int &feedbackSellerCount, int &feedbackArbiterCount);
void GetFeedback(std::vector<CFeedback> &feedback, float &avgRating, const FeedbackUser type, const std::vector<CFeedback>& feedBack);
bool GetAcceptByHash(std::vector<COffer> &offerList,  COfferAccept &ca,  COffer &offer);
bool GetTxOfOfferAccept(const std::vector<unsigned char> &vchOffer, const std::vector<unsigned char> &vchOfferAccept,
		COffer &theOffer, COfferAccept &theOfferAccept, CTransaction& tx, bool skipExpiresCheck=false);
bool GetOfferAccept(const std::vector<unsigned char> &vchOffer, const std::vector<unsigned char> &vchOfferAccept,
		COffer &theOffer, COfferAccept &theOfferAccept, bool skipExpiresCheck=false);
bool GetTxOfOffer(const std::vector<unsigned char> &vchOffer, COffer& txPos, CTransaction& tx, bool skipExpiresCheck=false);
bool GetTxAndVtxOfOffer(const std::vector<unsigned char> &vchOffer,
				  COffer& txPos, CTransaction& tx, std::vector<COffer> &vtxPos, bool skipExpiresCheck=false);
bool GetVtxOfOffer(const std::vector<unsigned char> &vchOffer,
				  COffer& txPos, std::vector<COffer> &vtxPos, bool skipExpiresCheck=false);
std::string GetPaymentOptionsString(const uint32_t paymentOptions);
CChainParams::AddressType PaymentOptionToAddressType(const uint32_t paymentOptions);
bool BuildOfferAcceptJson(const COffer& theOffer, const CAliasIndex &alias, const CTransaction &aliastx, UniValue& oOfferAccept);
bool BuildOfferJson(const COffer& theOffer, const CAliasIndex &alias, UniValue& oOffer);
uint64_t GetOfferExpiration(const COffer& offer);
#endif // OFFER_H
