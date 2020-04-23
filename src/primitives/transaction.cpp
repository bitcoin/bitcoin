// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>

#include <hash.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <assert.h>
// SYSCOIN
#include <compressor.h>
#include <services/asset.h>
#include <consensus/validation.h>
std::string COutPoint::ToString() const
{
    return strprintf("COutPoint(%s, %u)", hash.ToString().substr(0,10), n);
}
// SYSCOIN
std::string COutPoint::ToStringShort() const
{
    return strprintf("%s-%u", hash.ToString().substr(0,64), n);
}
CTxIn::CTxIn(COutPoint prevoutIn, CScript scriptSigIn, uint32_t nSequenceIn)
{
    prevout = prevoutIn;
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

CTxIn::CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn, uint32_t nSequenceIn)
{
    prevout = COutPoint(hashPrevTx, nOut);
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

std::string CTxIn::ToString() const
{
    std::string str;
    str += "CTxIn(";
    str += prevout.ToString();
    if (prevout.IsNull())
        str += strprintf(", coinbase %s", HexStr(scriptSig));
    else
        str += strprintf(", scriptSig=%s", HexStr(scriptSig).substr(0, 24));
    if (nSequence != SEQUENCE_FINAL)
        str += strprintf(", nSequence=%u", nSequence);
    str += ")";
    return str;
}

CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn)
{
    nValue = nValueIn;
    scriptPubKey = scriptPubKeyIn;
}
std::string CTxOut::ToString() const
{
    return strprintf("CTxOut(nValue=%d.%08d, scriptPubKey=%s", nValue / COIN, nValue % COIN, HexStr(scriptPubKey).substr(0, 30));
}
// SYSCOIN
std::string CTxOutCoin::ToString() const
{
    if(assetInfo.IsNull())
        return strprintf("CTxOutCoin(nValue=%d.%08d, scriptPubKey=%s)", nValue / COIN, nValue % COIN, HexStr(scriptPubKey).substr(0, 30));
    else
        return strprintf("CTxOutCoin(nValue=%d.%08d, scriptPubKey=%s, nAsset=%d, nAssetValue=%d.%08d)", nValue / COIN, nValue % COIN, HexStr(scriptPubKey).substr(0, 30), assetInfo.nAsset, assetInfo.nValue / COIN, assetInfo.nValue % COIN);
}
CMutableTransaction::CMutableTransaction() : nVersion(CTransaction::CURRENT_VERSION), nLockTime(0) {}
CMutableTransaction::CMutableTransaction(const CTransaction& tx) : vin(tx.vin), vout(tx.vout), nVersion(tx.nVersion), nLockTime(tx.nLockTime) {}

uint256 CMutableTransaction::GetHash() const
{
    return SerializeHash(*this, SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
}

uint256 CTransaction::ComputeHash() const
{
    return SerializeHash(*this, SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
}

uint256 CTransaction::ComputeWitnessHash() const
{
    if (!HasWitness()) {
        return hash;
    }
    return SerializeHash(*this, SER_GETHASH, 0);
}

/* For backward compatibility, the hash is initialized to 0. TODO: remove the need for this default constructor entirely. */
// SYSCOIN
CTransaction::CTransaction() : vin(), vout(), nVersion(CTransaction::CURRENT_VERSION), nLockTime(0), voutAssets(), hash{}, m_witness_hash{} {}
CTransaction::CTransaction(const CMutableTransaction& tx) : vin(tx.vin), vout(tx.vout), nVersion(tx.nVersion), nLockTime(tx.nLockTime), voutAssets(tx.voutAssets), hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {}
CTransaction::CTransaction(CMutableTransaction&& tx) : vin(std::move(tx.vin)), vout(std::move(tx.vout)), nVersion(tx.nVersion), nLockTime(tx.nLockTime), voutAssets(tx.voutAssets), hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {}

CAmount CTransaction::GetValueOut() const
{
    CAmount nValueOut = 0;
    bool bFirstOutput = true;
    for (const auto& tx_out : vout) {
        // SYSCOIN
        if(bFirstOutput && (nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN || nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN_LEGACY)){
            bFirstOutput = false;
            continue;
        }
        if (!MoneyRange(tx_out.nValue) || !MoneyRange(nValueOut + tx_out.nValue))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
        nValueOut += tx_out.nValue;
    }
    assert(MoneyRange(nValueOut));
    return nValueOut;
}

unsigned int CTransaction::GetTotalSize() const
{
    return ::GetSerializeSize(*this, PROTOCOL_VERSION);
}

std::string CTransaction::ToString() const
{
    std::string str;
    str += strprintf("CTransaction(hash=%s, ver=%d, vin.size=%u, vout.size=%u, nLockTime=%u)\n",
        GetHash().ToString().substr(0,10),
        nVersion,
        vin.size(),
        vout.size(),
        nLockTime);
    for (const auto& tx_in : vin)
        str += "    " + tx_in.ToString() + "\n";
    for (const auto& tx_in : vin)
        str += "    " + tx_in.scriptWitness.ToString() + "\n";
    for (const auto& tx_out : vout)
        str += "    " + tx_out.ToString() + "\n";
    return str;
}

// SYSCOIN
bool CTransaction::HasAssets() const
{
    return IsSyscoinTx(nVersion);
}

bool CMutableTransaction::HasAssets() const
{
    return IsSyscoinTx(nVersion);
}

void CMutableTransaction::LoadAssets()
{
    if(HasAssets()) {
        CAssetAllocation allocation(*MakeTransactionRef(*this));
        if(allocation.IsNull()) {
            throw std::ios_base::failure("Unknown asset data");
        }
        voutAssets = std::move(allocation.voutAssets);
        
        const size_t &nVoutSize = vout.size();
        for(const auto &it: voutAssets) {
            const int32_t &nAsset = it.first;
            if(it.second.empty()) {
                throw std::ios_base::failure("asset empty outputs");
            }
            for(const auto& voutAsset: it.second) {
                const uint32_t& nOut = voutAsset.n;
                if(nOut > nVoutSize) {
                    throw std::ios_base::failure("asset vout out of range");
                }
                // store in vout
                CAssetCoinInfo& coinInfo = vout[nOut].assetInfo;
                coinInfo.nAsset = nAsset;
                coinInfo.nValue = voutAsset.nValue;
            }
        }       
    }
}
bool CTransaction::GetAssetValueOut(const bool &isAssetTx, std::unordered_map<int32_t, CAmount> &mapAssetOut, TxValidationState& state) const
{
    std::unordered_set<uint32_t> setUsedIndex;
    for(const auto &it: voutAssets) {
        if(it.second.empty()) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-empty");
        }
        CAmount nTotal = 0;
        const int32_t &nAsset = it.first;
        if(nAsset < 0) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-negative-guid");
        }
        
        // never allow more than 1 zero value output per asset
        // this bool should be sufficient because later on uniqueness per asset is enforced with bad-txns-asset-not-unique check
        bool bFoundZeroValOutput = false;
        const size_t &nVoutSize = vout.size();
        for(const auto& voutAsset: it.second) {
            const uint32_t& nOut = voutAsset.n;
            if(nOut > nVoutSize) {
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-outofrange");
            }
            const CAmount& nAmount = voutAsset.nValue;
            // make sure the vout assetinfo matches the asset commitment in OP_RETURN
            if(vout[nOut].assetInfo.nAsset != nAsset || vout[nOut].assetInfo.nValue != nAmount) {
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-out-assetinfo-mismatch");
            }
            nTotal += nAmount;
            if(nAmount == 0) {
                // 0 amount output not possible for anything except asset tx (new/update/send)
                if(!isAssetTx) {
                    return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-zero-out-non-asset");
                }
                // only 1 is allowed for change
                if(bFoundZeroValOutput) {
                    return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-multiple-zero-out-found");
                } 
                else {
                    bFoundZeroValOutput = true;
                }
            }
            else if(!AssetRange(nAmount) || !AssetRange(nTotal)) {
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-out-outofrange");
            }
            auto itSet = setUsedIndex.emplace(nOut);
            if(!itSet.second) {
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-out-not-unique");
            }
        }
        // zero value output is required (even though it is sending 0, receiving 0) for asset tx
        if(isAssetTx && !bFoundZeroValOutput) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-no-zero-out-found");
        }
        if(nTotal > 0 && !AssetRange(nTotal)) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-total-outofrange");
        }
        auto itRes = mapAssetOut.emplace(nAsset, nTotal);
        if(!itRes.second) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-not-unique");
        }
    }
    return true;
}
RecursiveMutex cs_setethstatus;

bool IsSyscoinMintTx(const int &nVersion) {
    return nVersion == SYSCOIN_TX_VERSION_ALLOCATION_MINT;
}

bool IsAssetTx(const int &nVersion) {
    return nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE || nVersion == SYSCOIN_TX_VERSION_ASSET_UPDATE || nVersion == SYSCOIN_TX_VERSION_ASSET_SEND;
}

bool IsAssetAllocationTx(const int &nVersion) {
    return nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM || nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN || nVersion == SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION ||
        nVersion == SYSCOIN_TX_VERSION_ALLOCATION_SEND;
}

bool IsZdagTx(const int &nVersion) {
    return nVersion == SYSCOIN_TX_VERSION_ALLOCATION_SEND;
}

bool IsSyscoinTx(const int &nVersion) {
    return IsAssetTx(nVersion) || IsAssetAllocationTx(nVersion) || IsSyscoinMintTx(nVersion);
}

bool IsSyscoinWithNoInputTx(const int &nVersion) {
    return nVersion == SYSCOIN_TX_VERSION_ASSET_SEND || nVersion == SYSCOIN_TX_VERSION_ALLOCATION_MINT || nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE || nVersion == SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION;
}

unsigned int GetSyscoinDataOutput(const CTransaction& tx) {
	for (unsigned int i = 0; i<tx.vout.size(); i++) {
		if (tx.vout[i].scriptPubKey.IsUnspendable())
			return i;
	}
	return -1;
}

bool GetSyscoinData(const CTransaction &tx, std::vector<unsigned char> &vchData, int& nOut)
{
	nOut = GetSyscoinDataOutput(tx);
	if (nOut == -1)
		return false;

	const CScript &scriptPubKey = tx.vout[nOut].scriptPubKey;
	return GetSyscoinData(scriptPubKey, vchData);
}

bool GetSyscoinData(const CScript &scriptPubKey, std::vector<unsigned char> &vchData)
{
	CScript::const_iterator pc = scriptPubKey.begin();
	opcodetype opcode;
	if (!scriptPubKey.GetOp(pc, opcode))
		return false;
	if (opcode != OP_RETURN)
		return false;
	if (!scriptPubKey.GetOp(pc, opcode, vchData))
		return false;
    const unsigned int & nSize = scriptPubKey.size();
    // allow up to 80 bytes of data after our stack on standard asset transactions
    unsigned int nDifferenceAllowed = 83;
    // if data is more than 1 byte we used 2 bytes to store the varint (good enough for 64kb which is within limit of opreturn data on sys tx's)
    if(nSize >= 0xff){
        nDifferenceAllowed++;
    }
    if(nSize > (vchData.size() + nDifferenceAllowed)){
        return false;
    }
	return true;
}

bool CAssetAllocation::UnserializeFromData(const std::vector<unsigned char> &vchData) {
    try {
        CDataStream dsAsset(vchData, SER_NETWORK, PROTOCOL_VERSION);
        Unserialize(dsAsset);
    } catch (std::exception &e) {
		SetNull();
        return false;
    }
	return true;
}
bool CAssetAllocation::UnserializeFromTx(const CTransaction &tx) {
	std::vector<unsigned char> vchData;
	int nOut;
    if (!GetSyscoinData(tx, vchData, nOut))
    {
        SetNull();
        return false;
    }
    if(!UnserializeFromData(vchData))
    {	
        SetNull();
        return false;
    }
    
    return true;
}
void CAssetAllocation::SerializeData( std::vector<unsigned char> &vchData) {
    CDataStream dsAsset(SER_NETWORK, PROTOCOL_VERSION);
    Serialize(dsAsset);
	vchData = std::vector<unsigned char>(dsAsset.begin(), dsAsset.end());

}

bool CMintSyscoin::UnserializeFromData(const std::vector<unsigned char> &vchData) {
    try {
        CDataStream dsMS(vchData, SER_NETWORK, PROTOCOL_VERSION);
        Unserialize(dsMS);
    } catch (std::exception &e) {
        SetNull();
        return false;
    }
    return true;
}

bool CMintSyscoin::UnserializeFromTx(const CTransaction &tx) {
    std::vector<unsigned char> vchData;
    int nOut;
    if (!GetSyscoinData(tx, vchData, nOut))
    {
        SetNull();
        return false;
    }
    if(!UnserializeFromData(vchData))
    {   
        SetNull();
        return false;
    }  
    return true;
}

void CMintSyscoin::SerializeData( std::vector<unsigned char> &vchData) {
    CDataStream dsMint(SER_NETWORK, PROTOCOL_VERSION);
    Serialize(dsMint);
    vchData = std::vector<unsigned char>(dsMint.begin(), dsMint.end());
}

bool CBurnSyscoin::UnserializeFromData(const std::vector<unsigned char> &vchData) {
    try {
        CDataStream dsMS(vchData, SER_NETWORK, PROTOCOL_VERSION);
        Unserialize(dsMS);
    } catch (std::exception &e) {
        SetNull();
        return false;
    }
    return true;
}

bool CBurnSyscoin::UnserializeFromTx(const CTransaction &tx) {
    if(tx.nVersion != SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM)
        return false;
    std::vector<unsigned char> vchData;
    int nOut;
    if (!GetSyscoinData(tx, vchData, nOut))
    {
        SetNull();
        return false;
    }
    if(!UnserializeFromData(vchData))
    {   
        SetNull();
        return false;
    }
    return true;
}

void CBurnSyscoin::SerializeData( std::vector<unsigned char> &vchData) {
    CDataStream dsBurn(SER_NETWORK, PROTOCOL_VERSION);
    Serialize(dsBurn);
    vchData = std::vector<unsigned char>(dsBurn.begin(), dsBurn.end());
}

template<typename Stream, typename AssetOutType>
void SerializeAssetOut(AssetOutType& assetOut, Stream& s) {
    s << VARINT(assetOut.n);
    ::Serialize(s, Using<AmountCompression>(assetOut.nValue));	
}

template<typename Stream, typename AssetOutType>
void UnserializeAssetOut(AssetOutType& assetOut, Stream& s) {
    s >> VARINT(assetOut.n);
    ::Unserialize(s, Using<AmountCompression>(assetOut.nValue));	
}