#ifndef OFFER_H
#define OFFER_H

#include "rpcserver.h"
#include "dbwrapper.h"
#include "script/script.h"
#include "serialize.h"
class CWalletTx;
class CTransaction;
class CReserveKey;
class CCoinsViewCache;
class CCoins;
class CBlockIndex;
class CBlock;
bool CheckOfferInputs(const CTransaction &tx, int op, int nOut, const std::vector<std::vector<unsigned char> > &vvchArgs, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, const CBlock *block = NULL);


bool DecodeOfferTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseOfferTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeOfferScript(const CScript& script, int& op, std::vector<std::vector<unsigned char> > &vvch);
bool IsOfferOp(int op);
int IndexOfOfferOutput(const CTransaction& tx);
int GetOfferExpirationDepth();
std::string offerFromOp(int op);
CScript RemoveOfferScriptPrefix(const CScript& scriptIn);

class COfferAccept {
public:
	std::vector<unsigned char> vchAcceptRand;
	uint256 txHash;
	uint64_t nHeight;
	uint64_t nAcceptHeight;
	unsigned int nQty;
	float nPrice;
	uint256 txBTCId;
	std::vector<unsigned char> vchBuyerKey;	
	COfferAccept() {
        SetNull();
    }

	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
		READWRITE(vchAcceptRand);
		READWRITE(txHash);
		READWRITE(VARINT(nHeight));
		READWRITE(VARINT(nAcceptHeight));
        READWRITE(VARINT(nQty));
    	READWRITE(nPrice);
		READWRITE(vchBuyerKey);	
		READWRITE(txBTCId);	
		
	}

    friend bool operator==(const COfferAccept &a, const COfferAccept &b) {
        return (
		a.vchAcceptRand == b.vchAcceptRand
        && a.txHash == b.txHash
        && a.nHeight == b.nHeight
		&& a.nAcceptHeight == b.nAcceptHeight
        && a.nQty == b.nQty
        && a.nPrice == b.nPrice
		&& a.vchBuyerKey == b.vchBuyerKey
		&& a.txBTCId == b.txBTCId
        );
    }

    COfferAccept operator=(const COfferAccept &b) {
		vchAcceptRand = b.vchAcceptRand;
        txHash = b.txHash;
        nHeight = b.nHeight;
		nAcceptHeight = b.nAcceptHeight;
        nQty = b.nQty;
        nPrice = b.nPrice;
		vchBuyerKey = b.vchBuyerKey;
		txBTCId = b.txBTCId;
        return *this;
    }

    friend bool operator!=(const COfferAccept &a, const COfferAccept &b) {
        return !(a == b);
    }

    void SetNull() { vchAcceptRand.clear(); nHeight = nAcceptHeight = nPrice = nQty = 0; txHash.SetNull(); txBTCId.SetNull(); vchBuyerKey.clear();}
    bool IsNull() const { return (vchAcceptRand.empty() && txHash.IsNull() && nHeight == 0 && nAcceptHeight == 0 &&nPrice == 0 && nQty == 0 && txBTCId.IsNull() && vchBuyerKey.empty()); }

};
class COfferLinkWhitelistEntry {
public:
	std::vector<unsigned char> aliasLinkVchRand;
	char nDiscountPct;
	COfferLinkWhitelistEntry() {
		SetNull();
	}

	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(aliasLinkVchRand);
		READWRITE(VARINT(nDiscountPct));
	}

    friend bool operator==(const COfferLinkWhitelistEntry &a, const COfferLinkWhitelistEntry &b) {
        return (
           a.aliasLinkVchRand == b.aliasLinkVchRand
		&& a.nDiscountPct == b.nDiscountPct
        );
    }

    COfferLinkWhitelistEntry operator=(const COfferLinkWhitelistEntry &b) {
    	aliasLinkVchRand = b.aliasLinkVchRand;
		nDiscountPct = b.nDiscountPct;
        return *this;
    }

    friend bool operator!=(const COfferLinkWhitelistEntry &a, const COfferLinkWhitelistEntry &b) {
        return !(a == b);
    }
    
    void SetNull() { aliasLinkVchRand.clear(); nDiscountPct = 0;}
    bool IsNull() const { return (aliasLinkVchRand.empty() && nDiscountPct == 0); }

};
class COfferLinkWhitelist {
public:
	std::vector<COfferLinkWhitelistEntry> entries;
	bool bExclusiveResell;
	COfferLinkWhitelist() {
		SetNull();
	}

	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(entries);
		READWRITE(bExclusiveResell);

	}
    bool GetLinkEntryByHash(const std::vector<unsigned char> &ahash, COfferLinkWhitelistEntry &entry) {
    	for(unsigned int i=0;i<entries.size();i++) {
    		if(entries[i].aliasLinkVchRand == ahash) {
    			entry = entries[i];
    			return true;
    		}
    	}
    	return false;
    }
    bool RemoveWhitelistEntry(const std::vector<unsigned char> &ahash) {
    	for(unsigned int i=0;i<entries.size();i++) {
    		if(entries[i].aliasLinkVchRand == ahash) {
    			return entries.erase(entries.begin()+i) != entries.end();
    		}
    	}
    	return false;
    }	
    void PutWhitelistEntry(const COfferLinkWhitelistEntry &theEntry) {
    	for(unsigned int i=0;i<entries.size();i++) {
    		COfferLinkWhitelistEntry entry = entries[i];
    		if(theEntry.aliasLinkVchRand == entry.aliasLinkVchRand) {
    			entries[i] = theEntry;
    			return;
    		}
    	}
    	entries.push_back(theEntry);
    }
    friend bool operator==(const COfferLinkWhitelist &a, const COfferLinkWhitelist &b) {
        return (
           a.entries == b.entries
		&& a.bExclusiveResell == b.bExclusiveResell

        );
    }

    COfferLinkWhitelist operator=(const COfferLinkWhitelist &b) {
    	entries = b.entries;
		bExclusiveResell = b.bExclusiveResell;
        return *this;
    }

    friend bool operator!=(const COfferLinkWhitelist &a, const COfferLinkWhitelist &b) {
        return !(a == b);
    }
    
    void SetNull() { entries.clear();}
    bool IsNull() const { return (entries.empty());}

};
class COffer {

public:
	std::vector<unsigned char> vchPubKey;
    uint256 txHash;
    uint64_t nHeight;
	std::vector<unsigned char> sCategory;
	std::vector<unsigned char> sTitle;
	std::vector<unsigned char> sDescription;
	float nPrice;
	char nCommission;
	int nQty;
	COfferAccept accept;
	std::vector<unsigned char> vchLinkOffer;
	std::vector<unsigned char> sCurrencyCode;
	std::vector<unsigned char> vchCert;
	std::vector<unsigned char> vchAliasPeg;
	COfferLinkWhitelist linkWhitelist;
	std::vector<std::vector<unsigned char> > offerLinks;
	bool bPrivate;
	bool bOnlyAcceptBTC;
	COffer() { 
        SetNull();
    }

    COffer(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
	// clear everything but the necessary information for an offer to prepare it to go into a txn
	void ClearOffer()
	{
		accept.SetNull();
		linkWhitelist.SetNull();
		offerLinks.clear();
		sCategory.clear();
		sTitle.clear();
		sDescription.clear();
		vchLinkOffer.clear();
		vchLinkOffer.clear();
		vchCert.clear();
		vchAliasPeg.clear();
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
    	READWRITE(accept);
		READWRITE(vchLinkOffer);
		READWRITE(linkWhitelist);
		READWRITE(sCurrencyCode);
		READWRITE(nCommission);
		READWRITE(offerLinks);
		READWRITE(vchPubKey);
		READWRITE(vchCert);
		READWRITE(bPrivate);
		READWRITE(vchAliasPeg);
		READWRITE(bOnlyAcceptBTC);
		
		
	}
	float GetPrice(const COfferLinkWhitelistEntry& entry=COfferLinkWhitelistEntry()){
		float price = nPrice;
		if(price==0 && !accept.IsNull())
			return accept.nPrice;
		float fDiscount = entry.nDiscountPct;
		if(entry.nDiscountPct < -99 || entry.nDiscountPct > 99)
			fDiscount = 0;
		// fMarkup is a percentage, commission minus discount
		float fMarkup = nCommission - fDiscount;
		
		// add commission , subtract discount
		fMarkup = price*(fMarkup / 100);
		price = price + fMarkup;
		return price;
	}

	void SetPrice(float price){
		nPrice = price;
	}
    void PutToOfferList(std::vector<COffer> &offerList) {
        for(unsigned int i=0;i<offerList.size();i++) {
            COffer o = offerList[i];
            if(o.txHash == txHash && o.accept.vchAcceptRand == accept.vchAcceptRand) {
                offerList[i] = *this;
                return;
            }
        }
        offerList.push_back(*this);
    }

    bool GetOfferFromList(std::vector<COffer> &offerList) {
        if(offerList.size() == 0) return false;
		COffer myOffer = offerList.front();
		// find the closest offer without going over in height, assuming offerList orders entries by nHeight ascending
        for(std::vector<COffer>::reverse_iterator it = offerList.rbegin(); it != offerList.rend(); ++it) {
            const COffer &o = *it;
			// skip if this is an offeraccept or height is greater than our offer height
			if(!o.accept.IsNull() || o.nHeight > nHeight)
				continue;
            myOffer = o;
			break;
        }
        *this = myOffer;
        return true;
    }

    friend bool operator==(const COffer &a, const COffer &b) {
        return (
         a.sCategory==b.sCategory
        && a.sTitle == b.sTitle 
        && a.sDescription == b.sDescription 
        && a.nPrice == b.nPrice 
        && a.nQty == b.nQty 
        && a.txHash == b.txHash
        && a.nHeight == b.nHeight
        && a.accept == b.accept
		&& a.vchLinkOffer == b.vchLinkOffer
		&& a.linkWhitelist == b.linkWhitelist
		&& a.sCurrencyCode == b.sCurrencyCode
		&& a.nCommission == b.nCommission
		&& a.vchPubKey == b.vchPubKey
		&& a.vchCert == b.vchCert
		&& a.bPrivate == b.bPrivate
		&& a.bOnlyAcceptBTC == b.bOnlyAcceptBTC
		&& a.vchAliasPeg == b.vchAliasPeg
		
        );
    }

    COffer operator=(const COffer &b) {
        sCategory = b.sCategory;
        sTitle = b.sTitle;
        sDescription = b.sDescription;
        nPrice = b.nPrice;
        nQty = b.nQty;
        txHash = b.txHash;
        nHeight = b.nHeight;
        accept = b.accept;
		vchLinkOffer = b.vchLinkOffer;
		linkWhitelist = b.linkWhitelist;
		sCurrencyCode = b.sCurrencyCode;
		offerLinks = b.offerLinks;
		nCommission = b.nCommission;
		vchPubKey = b.vchPubKey;
		vchCert = b.vchCert;
		bPrivate = b.bPrivate;
		bOnlyAcceptBTC = b.bOnlyAcceptBTC;
		vchAliasPeg = b.vchAliasPeg;
        return *this;
    }

    friend bool operator!=(const COffer &a, const COffer &b) {
        return !(a == b);
    }
    
    void SetNull() { nHeight = nPrice = nQty = 0; txHash.SetNull(); bPrivate = false; bOnlyAcceptBTC = false; accept.SetNull(); vchAliasPeg.clear(); sTitle.clear(); sDescription.clear();vchLinkOffer.clear();linkWhitelist.SetNull();sCurrencyCode.clear();offerLinks.clear();nCommission=0;vchPubKey.clear();vchCert.clear();}
    bool IsNull() const { return (vchPubKey.empty() && txHash.IsNull() && nHeight == 0 && nPrice == 0 && nQty == 0 &&  linkWhitelist.IsNull() && vchAliasPeg.empty() && offerLinks.empty() && nCommission == 0 && bPrivate == false && bOnlyAcceptBTC == false); }

    bool UnserializeFromTx(const CTransaction &tx);
	const std::vector<unsigned char> Serialize();
};

class COfferDB : public CDBWrapper {
public:
	COfferDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "offers", nCacheSize, fMemory, fWipe) {}

	bool WriteOffer(const std::vector<unsigned char>& name, std::vector<COffer>& vtxPos) {
		return Write(make_pair(std::string("offeri"), name), vtxPos);
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


    bool ScanOffers(
            const std::vector<unsigned char>& vchName,
            unsigned int nMax,
            std::vector<std::pair<std::vector<unsigned char>, COffer> >& offerScan);

    bool ReconstructOfferIndex(CBlockIndex *pindexRescan);
};
bool GetAcceptByHash(std::vector<COffer> &offerList,  COfferAccept &ca);
bool GetTxOfOfferAccept(const std::vector<unsigned char> &vchOffer, const std::vector<unsigned char> &vchOfferAccept,
		COffer &theOffer, COfferAccept &theOfferAccept, CTransaction& tx);
bool GetTxOfOffer(const std::vector<unsigned char> &vchOffer, COffer& txPos, CTransaction& tx);
#endif // OFFER_H
