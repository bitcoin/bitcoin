#ifndef ESCROW_H
#define ESCROW_H

#include "rpcserver.h"
#include "dbwrapper.h"
#include "script/script.h"
#include "serialize.h"
class CWalletTx;
class CTransaction;
class CReserveKey;
class CValidationState;
class CCoinsViewCache;
class CCoins;


bool CheckEscrowInputs(const CTransaction &tx, CValidationState &state, const CCoinsViewCache &inputs, bool fBlock, bool fMiner, bool fJustCheck, int nHeight);
bool IsEscrowMine(const CTransaction& tx);
bool DecodeEscrowTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch, int nHeight);
bool DecodeEscrowScript(const CScript& script, int& op, std::vector<std::vector<unsigned char> > &vvch);
bool IsEscrowOp(int op);
int IndexOfEscrowOutput(const CTransaction& tx);
bool GetValueOfEscrowTxHash(const uint256 &txHash, std::vector<unsigned char>& vchValue, uint256& hash, int& nHeight);
int GetEscrowExpirationDepth();
int64_t GetEscrowNetworkFee(opcodetype seed, unsigned int nHeight);
int64_t GetEscrowNetFee(const CTransaction& tx);
bool InsertEscrowFee(CBlockIndex *pindex, uint256 hash, uint64_t nValue);

bool GetNameOfEscrowTx(const CTransaction& tx, std::vector<unsigned char>& escrow);
std::string escrowFromOp(int op);
CScript RemoveEscrowScriptPrefix(const CScript& scriptIn);
bool DecodeEscrowScript(const CScript& script, int& op,
        std::vector<std::vector<unsigned char> > &vvch,
        CScript::const_iterator& pc);

class CEscrow {
public:
    std::vector<unsigned char> vchRand;
	std::vector<unsigned char> vchBuyerKey;
	std::vector<unsigned char> vchSellerKey;
	std::vector<unsigned char> vchArbiterKey;
	std::string arbiter;
	std::string seller;
	std::vector<unsigned char> vchRedeemScript;
	std::vector<unsigned char> vchOffer;
	std::vector<unsigned char> vchPaymentMessage;
	std::vector<unsigned char> rawTx;
	std::vector<unsigned char> vchOfferAcceptLink;
	
    uint256 txHash;
	uint256 escrowInputTxHash;
    uint64_t nHeight;
	unsigned int nQty;
	int64_t nPricePerUnit;
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
        READWRITE(vchRand);
        READWRITE(vchBuyerKey);
		READWRITE(arbiter);
		READWRITE(vchSellerKey);
		READWRITE(seller);
		READWRITE(vchArbiterKey);
		READWRITE(vchRedeemScript);
        READWRITE(vchOffer);
		READWRITE(vchPaymentMessage);
		READWRITE(rawTx);
		READWRITE(vchOfferAcceptLink);
		READWRITE(txHash);
		READWRITE(escrowInputTxHash);
		READWRITE(VARINT(nHeight));
		READWRITE(VARINT(nQty));
		READWRITE(VARINT(nPricePerUnit));
		
		
	}

    friend bool operator==(const CEscrow &a, const CEscrow &b) {
        return (
        a.vchRand == b.vchRand
        && a.vchBuyerKey == b.vchBuyerKey
		&& a.vchSellerKey == b.vchSellerKey
        && a.arbiter == b.arbiter
		&& a.seller == b.seller
		&& a.vchArbiterKey == b.vchArbiterKey
		&& a.vchRedeemScript == b.vchRedeemScript
        && a.vchOffer == b.vchOffer
		&& a.vchPaymentMessage == b.vchPaymentMessage
		&& a.rawTx == b.rawTx
		&& a.vchOfferAcceptLink == b.vchOfferAcceptLink
		&& a.txHash == b.txHash
		&& a.escrowInputTxHash == b.escrowInputTxHash
		&& a.nHeight == b.nHeight
		&& a.nQty == b.nQty
		&& a.nPricePerUnit == b.nPricePerUnit
        );
    }

    CEscrow operator=(const CEscrow &b) {
        vchRand = b.vchRand;
        vchBuyerKey = b.vchBuyerKey;
		vchSellerKey = b.vchSellerKey;
        arbiter = b.arbiter;
		seller = b.seller;
		vchArbiterKey = b.vchArbiterKey;
		vchRedeemScript = b.vchRedeemScript;
        vchOffer = b.vchOffer;
		vchPaymentMessage = b.vchPaymentMessage;
		rawTx = b.rawTx;
		vchOfferAcceptLink = b.vchOfferAcceptLink;
		txHash = b.txHash;
		escrowInputTxHash = b.escrowInputTxHash;
		nHeight = b.nHeight;
		nQty = b.nQty;
		nPricePerUnit = b.nPricePerUnit;
        return *this;
    }

    friend bool operator!=(const CEscrow &a, const CEscrow &b) {
        return !(a == b);
    }

    void SetNull() { nHeight = 0; txHash.SetNull(); arbiter = ""; seller = ""; escrowInputTxHash.SetNull(); nQty = 0; nPricePerUnit = 0; vchRand.clear(); vchBuyerKey.clear(); vchArbiterKey.clear(); vchSellerKey.clear(); vchRedeemScript.clear(); vchOffer.clear(); rawTx.clear(); vchOfferAcceptLink.clear(); vchPaymentMessage.clear();}
    bool IsNull() const { return (txHash.IsNull() && escrowInputTxHash.IsNull() && nHeight == 0 && nQty == 0 && nPricePerUnit == 0 && vchRand.size() == 0); }
    bool UnserializeFromTx(const CTransaction &tx);
    std::string SerializeToString();
};


class CEscrowDB : public CDBWrapper {
public:
    CEscrowDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "escrow", nCacheSize, fMemory, fWipe) {}

    bool WriteEscrow(const std::vector<unsigned char>& name, std::vector<CEscrow>& vtxPos) {
        return Write(make_pair(std::string("escrowi"), name), vtxPos);
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

    bool ScanEscrows(
            const std::vector<unsigned char>& vchName,
            unsigned int nMax,
            std::vector<std::pair<std::vector<unsigned char>, CEscrow> >& escrowScan);

    bool ReconstructEscrowIndex(CBlockIndex *pindexRescan);
};

bool GetTxOfEscrow(CEscrowDB& dbEscrow, const std::vector<unsigned char> &vchEscrow, CEscrow& txPos, CTransaction& tx);

#endif // ESCROW_H
