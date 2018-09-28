// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ASSET_H
#define ASSET_H

#include "rpc/server.h"
#include "dbwrapper.h"
#include "script/script.h"
#include "serialize.h"
#include "primitives/transaction.h"
#include "assetallocation.h"
class CWalletTx;
class CTransaction;
class CReserveKey;
class CCoinsViewCache;
class CBlock;
class CAliasIndex;

bool CheckAssetInputs(const CTransaction &tx, const CCoinsViewCache &inputs, int op, const std::vector<std::vector<unsigned char> > &vvchArgs, const std::vector<unsigned char> &vchAlias, bool fJustCheck, int nHeight, sorted_vector<CAssetAllocationTuple> &revertedAssetAllocations, std::string &errorMessage, bool bSanityCheck=false);
bool DecodeAssetTx(const CTransaction& tx, int& op, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseAssetTx(const CTransaction& tx, int& op, std::vector<std::vector<unsigned char> >& vvch, char& type);
bool DecodeAssetScript(const CScript& script, int& op, std::vector<std::vector<unsigned char> > &vvch);
bool IsAssetOp(int op);
void AssetTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry);
std::string assetFromOp(int op);
bool RemoveAssetScriptPrefix(const CScript& scriptIn, CScript& scriptOut);
// 10m max asset input range circulation
static const CAmount MAX_INPUTRANGE_ASSET = 10000000;
/** Upper bound for mantissa.
* 10^18-1 is the largest arbitrary decimal that will fit in a signed 64-bit integer.
* Larger integers cannot consist of arbitrary combinations of 0-9:
*
*   999999999999999999  10^18-1
*  1000000000000000000  10^18		(would overflow)
*  9223372036854775807  (1<<63)-1   (max int64_t)
*  9999999999999999999  10^19-1     (would overflow)
*/
static const CAmount MAX_ASSET = 1000000000000000000LL - 1LL;
inline bool AssetRange(const CAmount& nValue, bool bUseInputRange) { return (nValue > 0 && nValue <= (bUseInputRange? MAX_INPUTRANGE_ASSET: MAX_ASSET)); }
class CAsset {
public:
	std::vector<unsigned char> vchAsset;
	std::vector<unsigned char> vchSymbol;
	std::vector<unsigned char> vchAliasOrAddress;
	// if allocations are tracked by individual inputs
	std::vector<CRange> listAllocationInputs;
    uint256 txHash;
	unsigned int nHeight;
	std::vector<unsigned char> vchPubData;
	std::vector<unsigned char> sCategory;
	CAmount nBalance;
	CAmount nTotalSupply;
	CAmount nMaxSupply;
	bool bUseInputRanges;
	unsigned char nPrecision;
	float fInterestRate;
	bool bCanAdjustInterestRate;
    CAsset() {
        SetNull();
    }
    CAsset(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
	inline void ClearAsset()
	{
		vchPubData.clear();
		sCategory.clear();
		listAllocationInputs.clear();
		vchAliasOrAddress.clear();
		vchSymbol.clear();

	}
	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {		
		READWRITE(vchPubData);
		READWRITE(txHash);
		READWRITE(VARINT(nHeight));
		READWRITE(vchAsset);
		READWRITE(vchSymbol);
		READWRITE(sCategory);
		READWRITE(vchAliasOrAddress);
		READWRITE(listAllocationInputs);
		READWRITE(nBalance);
		READWRITE(nTotalSupply);
		READWRITE(nMaxSupply);
		READWRITE(bUseInputRanges);
		READWRITE(fInterestRate);
		READWRITE(bCanAdjustInterestRate);
		READWRITE(VARINT(nPrecision));
	}
    inline friend bool operator==(const CAsset &a, const CAsset &b) {
        return (
		a.vchAsset == b.vchAsset
        );
    }

    inline CAsset operator=(const CAsset &b) {
		vchPubData = b.vchPubData;
		txHash = b.txHash;
        nHeight = b.nHeight;
		vchAliasOrAddress = b.vchAliasOrAddress;
		vchAsset = b.vchAsset;
		sCategory = b.sCategory;
		listAllocationInputs = b.listAllocationInputs;
		nBalance = b.nBalance;
		nTotalSupply = b.nTotalSupply;
		nMaxSupply = b.nMaxSupply;
		bUseInputRanges = b.bUseInputRanges;
		fInterestRate = b.fInterestRate;
		bCanAdjustInterestRate = b.bCanAdjustInterestRate;
		nPrecision = b.nPrecision;
		vchSymbol = b.vchSymbol;
        return *this;
    }

    inline friend bool operator!=(const CAsset &a, const CAsset &b) {
        return !(a == b);
    }
	inline void SetNull() { vchSymbol.clear(); nPrecision = 8; bCanAdjustInterestRate = false; fInterestRate = 0;  bUseInputRanges = false; nMaxSupply = 0; nTotalSupply = 0; nBalance = 0; listAllocationInputs.clear(); sCategory.clear(); vchAsset.clear(); nHeight = 0; txHash.SetNull(); vchAliasOrAddress.clear(); vchPubData.clear(); }
    inline bool IsNull() const { return (vchAsset.empty()); }
    bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};


class CAssetDB : public CDBWrapper {
public:
    CAssetDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "assets", nCacheSize, fMemory, fWipe) {}

    bool WriteAsset(const CAsset& asset, const int &op) {
		bool writeState = false;
		writeState = Write(make_pair(std::string("asseti"), asset.vchAsset), asset);
		if(writeState)
			WriteAssetIndex(asset, op);
        return writeState;
    }
	bool EraseAsset(const std::vector<unsigned char>& vchAsset, bool cleanup = false) {
		return Erase(make_pair(std::string("asseti"), vchAsset));
	}
    bool ReadAsset(const std::vector<unsigned char>& vchAsset, CAsset& asset) {
        return Read(make_pair(std::string("asseti"), vchAsset), asset);
    }
	void WriteAssetIndex(const CAsset& asset, const int &op);
	void WriteAssetIndexHistory(const CAsset& asset, const int &op);
	bool ScanAssets(const int count, const int from, const UniValue& oOptions, UniValue& oRes);
};
bool GetAsset(const std::vector<unsigned char> &vchAsset,CAsset& txPos);
bool BuildAssetJson(const CAsset& asset, const bool bGetInputs, UniValue& oName);
bool BuildAssetIndexerJson(const CAsset& asset,UniValue& oName);
bool BuildAssetIndexerHistoryJson(const CAsset& asset, UniValue& oName);
UniValue ValueFromAssetAmount(const CAmount& amount, int precision, bool isInputRange);
CAmount AssetAmountFromValue(UniValue& value, int precision, bool isInputRange);
bool AssetRange(const CAmount& amountIn, int precision, bool isInputRange);
#endif // ASSET_H
