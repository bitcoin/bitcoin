// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>

#include <consensus/amount.h>
#include <hash.h>
#include <script/script.h>
#include <serialize.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <version.h>

#include <cassert>
#include <stdexcept>
// SYSCOIN
#include <streams.h>
#include <pubkey.h>
#include <logging.h>
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#endif
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

bool IsValidNEVMVersionHash(const std::vector<uint8_t>& vchVersionHash)
{
    if (vchVersionHash.size() == NEVM_DATA_LEGACY_VERSIONHASH_SIZE) {
        return true;
    }
    if (vchVersionHash.size() != NEVM_DATA_VERSIONED_HASH_SIZE) {
        return false;
    }
    return vchVersionHash[0] == NEVM_DATA_BLAKE2S_VERSION_BYTE;
}

bool DecodeNEVMVersionHash(const std::vector<uint8_t>& version_hash, uint8_t& hash_type, std::vector<uint8_t>& hash_digest)
{
    if (!IsValidNEVMVersionHash(version_hash)) {
        return false;
    }
    if (version_hash.size() == NEVM_DATA_LEGACY_VERSIONHASH_SIZE) {
        hash_type = NEVM_DATA_LEGACY_VERSION_BYTE;
        hash_digest = version_hash;
        return true;
    }
    hash_type = version_hash[0];
    hash_digest.assign(version_hash.begin() + 1, version_hash.end());
    return true;
}

std::vector<uint8_t> EncodeNEVMVersionHash(const std::vector<uint8_t>& hash_digest, uint8_t hash_type)
{
    if (hash_digest.size() != NEVM_DATA_LEGACY_VERSIONHASH_SIZE) {
        return {};
    }
    if (hash_type == NEVM_DATA_LEGACY_VERSION_BYTE) {
        return hash_digest;
    }
    if (hash_type != NEVM_DATA_BLAKE2S_VERSION_BYTE) {
        return {};
    }
    std::vector<uint8_t> encoded_hash;
    encoded_hash.reserve(NEVM_DATA_VERSIONED_HASH_SIZE);
    encoded_hash.push_back(hash_type);
    encoded_hash.insert(encoded_hash.end(), hash_digest.begin(), hash_digest.end());
    return encoded_hash;
}

bool fTestNet = false;
std::string COutPoint::ToString() const
{
    return strprintf("COutPoint(%s, %u)", hash.ToString().substr(0,10), n);
}
// SYSCOIN
std::string COutPoint::ToStringShort() const
{
    return strprintf("%s-%u", hash.ToString().substr(0,64), n);
}
// SYSCOIN
CTxIn::CTxIn(const COutPoint &prevoutIn, const CScript &scriptSigIn, uint32_t nSequenceIn)
{
    prevout = prevoutIn;
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

CTxIn::CTxIn(const uint256 &hashPrevTx, uint32_t nOut, const CScript &scriptSigIn, uint32_t nSequenceIn)
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
// SYSCOIN
CTxOut::CTxOut(const CAmount& nValueIn,  const CScript &scriptPubKeyIn)
{
    nValue = nValueIn;
    scriptPubKey = scriptPubKeyIn;
    vchNEVMData.clear();
    assetInfo.SetNull();
}
std::string CTxOut::ToString() const
{
    if(assetInfo.IsNull())
        return strprintf("CTxOut(nValue=%d.%08d, scriptPubKey=%s)", nValue / COIN, nValue % COIN, HexStr(scriptPubKey).substr(0, 30));
    else
        return strprintf("CTxOut(nValue=%d.%08d, scriptPubKey=%s, nAsset=%llu, nAssetValue=%d.%08d)", nValue / COIN, nValue % COIN, HexStr(scriptPubKey).substr(0, 30), assetInfo.nAsset, assetInfo.nValue / COIN, assetInfo.nValue % COIN);
}
CMutableTransaction::CMutableTransaction() : nVersion(CTransaction::CURRENT_VERSION), nLockTime(0) {}
CMutableTransaction::CMutableTransaction(const CTransaction& tx) : vin(tx.vin), vout(tx.vout), nVersion(tx.nVersion), nLockTime(tx.nLockTime) {}

uint256 CMutableTransaction::GetHash() const
{
    return (CHashWriter{SERIALIZE_TRANSACTION_NO_WITNESS} << *this).GetHash();
}

uint256 CTransaction::ComputeHash() const
{
    return (CHashWriter{SERIALIZE_TRANSACTION_NO_WITNESS} << *this).GetHash();
}

uint256 CTransaction::ComputeWitnessHash() const
{
    if (!HasWitness()) {
        return hash;
    }
    return (CHashWriter{0} << *this).GetHash();
}
CTransaction::CTransaction(const CMutableTransaction& tx) : vin(tx.vin), vout(tx.vout), nVersion(tx.nVersion), nLockTime(tx.nLockTime), hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {}
CTransaction::CTransaction(CMutableTransaction&& tx) : vin(std::move(tx.vin)), vout(std::move(tx.vout)), nVersion(tx.nVersion), nLockTime(tx.nLockTime), hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {}

CAmount CTransaction::GetValueOut() const
{
    CAmount nValueOut = 0;
    bool bFirstOutput = true;
    for (const auto& tx_out : vout) {
        if (!MoneyRange(tx_out.nValue) || !MoneyRange(nValueOut + tx_out.nValue))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
        // SYSCOIN
        if(bFirstOutput && (nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN || nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN_LEGACY || nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN_LEGACY1)){
            bFirstOutput = false;
            continue;
        }
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
/*
 * These check for scripts for which a special case with a shorter encoding is defined.
 * They are implemented separately from the CScript test, as these test for exact byte
 * sequence correspondences, and are more strict. For example, IsToPubKey also verifies
 * whether the public key is valid (as invalid ones cannot be represented in compressed
 * form).
 */

static bool IsToKeyID(const CScript& script, CKeyID &hash)
{
    if (script.size() == 25 && script[0] == OP_DUP && script[1] == OP_HASH160
                            && script[2] == 20 && script[23] == OP_EQUALVERIFY
                            && script[24] == OP_CHECKSIG) {
        memcpy(&hash, &script[3], 20);
        return true;
    }
    return false;
}

static bool IsToScriptID(const CScript& script, CScriptID &hash)
{
    if (script.size() == 23 && script[0] == OP_HASH160 && script[1] == 20
                            && script[22] == OP_EQUAL) {
        memcpy(&hash, &script[2], 20);
        return true;
    }
    return false;
}

static bool IsToPubKey(const CScript& script, CPubKey &pubkey)
{
    if (script.size() == 35 && script[0] == 33 && script[34] == OP_CHECKSIG
                            && (script[1] == 0x02 || script[1] == 0x03)) {
        pubkey.Set(&script[1], &script[34]);
        return true;
    }
    if (script.size() == 67 && script[0] == 65 && script[66] == OP_CHECKSIG
                            && script[1] == 0x04) {
        pubkey.Set(&script[1], &script[66]);
        return pubkey.IsFullyValid(); // if not fully valid, a case that would not be compressible
    }
    return false;
}

bool CompressScript(const CScript& script, CompressedScript &out)
{
    CKeyID keyID;
    if (IsToKeyID(script, keyID)) {
        out.resize(21);
        out[0] = 0x00;
        memcpy(&out[1], &keyID, 20);
        return true;
    }
    CScriptID scriptID;
    if (IsToScriptID(script, scriptID)) {
        out.resize(21);
        out[0] = 0x01;
        memcpy(&out[1], &scriptID, 20);
        return true;
    }
    CPubKey pubkey;
    if (IsToPubKey(script, pubkey)) {
        out.resize(33);
        memcpy(&out[1], &pubkey[1], 32);
        if (pubkey[0] == 0x02 || pubkey[0] == 0x03) {
            out[0] = pubkey[0];
            return true;
        } else if (pubkey[0] == 0x04) {
            out[0] = 0x04 | (pubkey[64] & 0x01);
            return true;
        }
    }
    return false;
}

unsigned int GetSpecialScriptSize(unsigned int nSize)
{
    if (nSize == 0 || nSize == 1)
        return 20;
    if (nSize == 2 || nSize == 3 || nSize == 4 || nSize == 5)
        return 32;
    return 0;
}

bool DecompressScript(CScript& script, unsigned int nSize, const CompressedScript& in)
{
    switch(nSize) {
    case 0x00:
        script.resize(25);
        script[0] = OP_DUP;
        script[1] = OP_HASH160;
        script[2] = 20;
        memcpy(&script[3], in.data(), 20);
        script[23] = OP_EQUALVERIFY;
        script[24] = OP_CHECKSIG;
        return true;
    case 0x01:
        script.resize(23);
        script[0] = OP_HASH160;
        script[1] = 20;
        memcpy(&script[2], in.data(), 20);
        script[22] = OP_EQUAL;
        return true;
    case 0x02:
    case 0x03:
        script.resize(35);
        script[0] = 33;
        script[1] = nSize;
        memcpy(&script[2], in.data(), 32);
        script[34] = OP_CHECKSIG;
        return true;
    case 0x04:
    case 0x05:
        unsigned char vch[33] = {};
        vch[0] = nSize - 2;
        memcpy(&vch[1], in.data(), 32);
        CPubKey pubkey{vch};
        if (!pubkey.Decompress())
            return false;
        assert(pubkey.size() == 65);
        script.resize(67);
        script[0] = 65;
        memcpy(&script[1], pubkey.begin(), 65);
        script[66] = OP_CHECKSIG;
        return true;
    }
    return false;
}

// Amount compression:
// * If the amount is 0, output 0
// * first, divide the amount (in base units) by the largest power of 10 possible; call the exponent e (e is max 9)
// * if e<9, the last digit of the resulting number cannot be 0; store it as d, and drop it (divide by 10)
//   * call the result n
//   * output 1 + 10*(9*n + d - 1) + e
// * if e==9, we only know the resulting number is not zero, so output 1 + 10*(n - 1) + 9
// (this is decodable, as d is in [1-9] and e is in [0-9])

uint64_t CompressAmount(uint64_t n)
{
    if (n == 0)
        return 0;
    int e = 0;
    while (((n % 10) == 0) && e < 9) {
        n /= 10;
        e++;
    }
    if (e < 9) {
        int d = (n % 10);
        assert(d >= 1 && d <= 9);
        n /= 10;
        return 1 + (n*9 + d - 1)*10 + e;
    } else {
        return 1 + (n - 1)*10 + 9;
    }
}

uint64_t DecompressAmount(uint64_t x)
{
    // x = 0  OR  x = 1+10*(9*n + d - 1) + e  OR  x = 1+10*(n - 1) + 9
    if (x == 0)
        return 0;
    x--;
    // x = 10*(9*n + d - 1) + e
    int e = x % 10;
    x /= 10;
    uint64_t n = 0;
    if (e < 9) {
        // x = 9*n + d - 1
        int d = (x % 9) + 1;
        x /= 9;
        // x = n
        n = x*10 + d;
    } else {
        n = x+1;
    }
    while (e) {
        n *= 10;
        e--;
    }
    return n;
}

bool CTransaction::HasAssets() const
{
    return IsSyscoinTx(nVersion);
}
bool CTransaction::IsNEVMData() const
{
    return IsSyscoinNEVMDataTx(nVersion);
}
bool CMutableTransaction::HasAssets() const
{
    return IsSyscoinTx(nVersion);
}
bool CMutableTransaction::IsNEVMData() const
{
    return IsSyscoinNEVMDataTx(nVersion);
}
bool CTransaction::IsMnTx() const
{
    return IsMasternodeTx(nVersion);
}
bool CMutableTransaction::IsMnTx() const
{
    return IsMasternodeTx(nVersion);
}
void CMutableTransaction::LoadAssets()
{
    if(HasAssets()) {
        CAssetAllocation allocation(*this);
        if(allocation.IsNull()) {
            throw std::ios_base::failure("Unknown asset data");
        }
        if(allocation.voutAssets.empty()) {
            throw std::ios_base::failure("asset empty map");
        }
        const size_t nVoutSize = vout.size();
        for(const auto &it: allocation.voutAssets) {
            const uint64_t &nAsset = it.key;
            if(it.values.empty()) {
                throw std::ios_base::failure("asset empty outputs");
            }
            for(const auto& voutAsset: it.values) {
                const uint32_t& nOut = voutAsset.n;
                if(nOut >= nVoutSize) {
                    throw std::ios_base::failure("asset vout out of range");
                }
                if(!MoneyRange(voutAsset.nValue)) {
                    throw std::ios_base::failure("asset vout value out of range");
                }
                // store in vout
                CAssetCoinInfo& coinInfo = vout[nOut].assetInfo;
                coinInfo.nAsset = nAsset;
                coinInfo.nValue = voutAsset.nValue;
            }
        }       
    }
}

bool CTransaction::GetAssetValueOut(CAssetsMap &mapAssetOut, std::string &err) const
{
    for(const auto &out: vout) {
        if(out.assetInfo.IsNull()) {
            continue;
        }
        const uint64_t &nAsset = out.assetInfo.nAsset;
        const CAmount& nAmount = out.assetInfo.nValue;
        auto itRes = mapAssetOut.try_emplace(nAsset, nAmount);
        if(!itRes.second) {
            itRes.first->second += nAmount;
            if (!MoneyRange(nAmount) || !MoneyRange(itRes.first->second)) {
                err = "bad-txns-asset-out-outofrange";
                return false;
            }
        }
    }
    return true;
}


bool IsSyscoinMintTx(const int &nVersion) {
    return nVersion == SYSCOIN_TX_VERSION_ALLOCATION_MINT;
}

bool IsAssetAllocationTx(const int &nVersion) {
    return nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_NEVM || nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN || nVersion == SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION ||
        nVersion == SYSCOIN_TX_VERSION_ALLOCATION_SEND;
}

bool IsSyscoinNEVMDataTx(const int &nVersion) {
    return nVersion == SYSCOIN_TX_VERSION_NEVM_DATA_SHA3;
}

bool IsZdagTx(const int &nVersion) {
    return nVersion == SYSCOIN_TX_VERSION_ALLOCATION_SEND;
}

bool IsSyscoinTx(const int &nVersion) {
    return IsAssetAllocationTx(nVersion) || IsSyscoinMintTx(nVersion);
}
bool IsMasternodeTx(const int &nVersion) {
    return 
     nVersion == SYSCOIN_TX_VERSION_MN_REGISTER ||
     nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_SERVICE ||
     nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_REGISTRAR ||
     nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_REVOKE;
}

int GetSyscoinDataOutput(const CTransaction& tx) {
	for (unsigned int i = 0; i<tx.vout.size(); i++) {
		if (tx.vout[i].scriptPubKey.IsUnspendable())
			return i;
	}
	return -1;
}
int GetSyscoinDataOutput(const CMutableTransaction& mtx) {
	for (unsigned int i = 0; i<mtx.vout.size(); i++) {
		if (mtx.vout[i].scriptPubKey.IsUnspendable())
			return i;
	}
	return -1;
}
bool GetSyscoinData(const CTransaction &tx, std::vector<unsigned char> &vchData, int& nOut)
{
	nOut = GetSyscoinDataOutput(tx);
	if (nOut == -1) {
		return false;
    }

	const CScript &scriptPubKey = tx.vout[nOut].scriptPubKey;
	return GetSyscoinData(scriptPubKey, vchData);
}
bool GetSyscoinData(const CMutableTransaction &mtx, std::vector<unsigned char> &vchData, int& nOut)
{
	nOut = GetSyscoinDataOutput(mtx);
	if (nOut == -1) {
		return false;
    }

	const CScript &scriptPubKey = mtx.vout[nOut].scriptPubKey;
	return GetSyscoinData(scriptPubKey, vchData);
}

bool GetSyscoinData(const CScript &scriptPubKey, std::vector<unsigned char> &vchData)
{
	CScript::const_iterator pc = scriptPubKey.begin();
	opcodetype opcode;
	if (!scriptPubKey.GetOp(pc, opcode))
		return false;
	if (opcode != OP_RETURN) {
		return false;
    }
	if (!scriptPubKey.GetOp(pc, opcode, vchData))
		return false;
    // if witness script we get the next element which should be our MN data
    if(vchData.size() >= 36 && vchData[0] == 0xaa &&
        vchData[1] == 0x21 &&
        vchData[2] == 0xa9 &&
        vchData[3] == 0xed) {
        if (!scriptPubKey.GetOp(pc, opcode, vchData))
		    return false;
    }
    // shouldn't be another opcode after this
	return !scriptPubKey.GetOp(pc, opcode);
}

CAssetAllocation::CAssetAllocation(const CTransaction &tx) {
    SetNull();
    UnserializeFromTx(tx);
}
CAssetAllocation::CAssetAllocation(const CMutableTransaction &mtx) {
    SetNull();
    UnserializeFromTx(mtx);
}
CMintSyscoin::CMintSyscoin(const CTransaction &tx) {
    SetNull();
    UnserializeFromTx(tx);
}
CMintSyscoin::CMintSyscoin(const CMutableTransaction &mtx) {
    SetNull();
    UnserializeFromTx(mtx);
}
CBurnSyscoin::CBurnSyscoin(const CTransaction &tx) {
    SetNull();
    UnserializeFromTx(tx);
}
CBurnSyscoin::CBurnSyscoin(const CMutableTransaction &mtx) {
    SetNull();
    UnserializeFromTx(mtx);
}
int CAssetAllocation::UnserializeFromData(const std::vector<unsigned char> &vchData) {
    try {
        CDataStream dsAsset(vchData, SER_NETWORK, PROTOCOL_VERSION);
        Unserialize(dsAsset);
        return dsAsset.size();
    } catch (std::exception &e) {
		SetNull();
    }
	return -1;
}
bool CAssetAllocation::UnserializeFromTx(const CTransaction &tx) {
	std::vector<unsigned char> vchData;
	int nOut;
    if (!GetSyscoinData(tx, vchData, nOut))
    {
        SetNull();
        return false;
    }
    const int bytesLeft = UnserializeFromData(vchData);
    const bool allocationMemoThreshold = IsAssetAllocationTx(tx.nVersion) && tx.nVersion != SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_NEVM && tx.nVersion != SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN;
	if(bytesLeft == -1 || (bytesLeft > MAX_MEMO && allocationMemoThreshold))
	{	
		SetNull();
		return false;
	}
    
    return true;
}
bool CAssetAllocation::UnserializeFromTx(const CMutableTransaction &mtx) {
	std::vector<unsigned char> vchData;
	int nOut;
    if (!GetSyscoinData(mtx, vchData, nOut))
    {
        SetNull();
        return false;
    }
    const int bytesLeft = UnserializeFromData(vchData);
    const bool allocationMemoThreshold =  IsAssetAllocationTx(mtx.nVersion) && mtx.nVersion != SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_NEVM && mtx.nVersion != SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN;
	if(bytesLeft == -1 || (bytesLeft > MAX_MEMO && allocationMemoThreshold))
	{	
		SetNull();
		return false;
	}
    return true;
}
int CMintSyscoin::UnserializeFromData(const std::vector<unsigned char> &vchData) {
    try {
        CDataStream dsMS(vchData, SER_NETWORK, PROTOCOL_VERSION);
        Unserialize(dsMS);
        if (posTx >= vchTxParentNodes.size() || posReceipt >= vchReceiptParentNodes.size()) {
            SetNull();
            return -1;
        }
        return dsMS.size();
    } catch (std::exception &e) {
        SetNull();
    }

    return -1;
}

bool CMintSyscoin::UnserializeFromTx(const CTransaction &tx) {
    std::vector<unsigned char> vchData;
    int nOut;
    if (!GetSyscoinData(tx, vchData, nOut))
    {
        SetNull();
        return false;
    }
	if(UnserializeFromData(vchData) != 0)
	{
		SetNull();
		return false;
	}
    return true;
}
bool CMintSyscoin::UnserializeFromTx(const CMutableTransaction &mtx) {
    std::vector<unsigned char> vchData;
    int nOut;
    if (!GetSyscoinData(mtx, vchData, nOut))
    {
        SetNull();
        return false;
    }
	if(UnserializeFromData(vchData) != 0)
	{
		SetNull();
		return false;
	}
    return true;
}

int CBurnSyscoin::UnserializeFromData(const std::vector<unsigned char> &vchData) {
    try {
        CDataStream dsMS(vchData, SER_NETWORK, PROTOCOL_VERSION);
        Unserialize(dsMS);
        return dsMS.size();
    } catch (std::exception &e) {
        SetNull();
    }
    return -1;
}

bool CBurnSyscoin::UnserializeFromTx(const CTransaction &tx) {
    std::vector<unsigned char> vchData;
    int nOut;
    if (!GetSyscoinData(tx, vchData, nOut))
    {
        SetNull();
        return false;
    }
	if(UnserializeFromData(vchData) != 0)
	{	
		SetNull();
		return false;
	} 
    return true;
}
bool CBurnSyscoin::UnserializeFromTx(const CMutableTransaction &mtx) {
    std::vector<unsigned char> vchData;
    int nOut;
    if (!GetSyscoinData(mtx, vchData, nOut))
    {
        SetNull();
        return false;
    }
	if(UnserializeFromData(vchData) != 0)
	{	
		SetNull();
		return false;
	} 
    return true;
}
void CAssetAllocation::SerializeData( std::vector<unsigned char> &vchData) {
    CDataStream dsAsset(SER_NETWORK, PROTOCOL_VERSION);
    Serialize(dsAsset);
    const auto &bytesVec = MakeUCharSpan(dsAsset);
	vchData = std::vector<unsigned char>(bytesVec.begin(), bytesVec.end());

}
void CMintSyscoin::SerializeData( std::vector<unsigned char> &vchData) {
    CDataStream dsMint(SER_NETWORK, PROTOCOL_VERSION);
    Serialize(dsMint);
    const auto &bytesVec = MakeUCharSpan(dsMint);
    vchData = std::vector<unsigned char>(bytesVec.begin(), bytesVec.end());
}

void CBurnSyscoin::SerializeData( std::vector<unsigned char> &vchData) {
    CDataStream dsBurn(SER_NETWORK, PROTOCOL_VERSION);
    Serialize(dsBurn);
    const auto &bytesVec = MakeUCharSpan(dsBurn);
    vchData = std::vector<unsigned char>(bytesVec.begin(), bytesVec.end());
}
CNEVMData::CNEVMData(const CTransaction &tx, const int nVersion) {
    SetNull();
    UnserializeFromTx(tx, nVersion);
}
CNEVMData::CNEVMData(const CTransaction &tx):CNEVMData(tx, PROTOCOL_VERSION) {
}
int CNEVMData::UnserializeFromData(const std::vector<unsigned char> &vchPayload, const int nVersion) {
    try {
		CDataStream dsNEVMData(vchPayload, SER_NETWORK, nVersion);
		Unser(dsNEVMData);
        if(vchVersionHash.size() != NEVM_DATA_LEGACY_VERSIONHASH_SIZE) {
            SetNull();
            return -1;
        }
        if (nVersionHashType != NEVM_DATA_LEGACY_VERSION_BYTE &&
            nVersionHashType != NEVM_DATA_BLAKE2S_VERSION_BYTE) {
            SetNull();
            return -1;
        }
        if(vchNEVMData && vchNEVMData->size() > MAX_NEVM_DATA_BLOB) {
            SetNull();
            return -1;
        }
        return dsNEVMData.size();
    } catch (std::exception &e) {
		SetNull();
    }
	return -1;
}
CNEVMData::CNEVMData(const CScript &script) {
    SetNull();
    UnserializeFromScript(script);
}
bool CNEVMData::UnserializeFromTx(const CTransaction &tx, const int nVersion) {
	std::vector<unsigned char> vchData;
	int nOut;
	if (!GetSyscoinData(tx, vchData, nOut))
	{
		SetNull();
		return false;
	}
	if(UnserializeFromData(vchData, nVersion) != 0)
	{
		SetNull();
		return false;
	}
    if(!tx.vout[nOut].vchNEVMData.empty()) {
        vchNEVMData = std::make_shared<const std::vector<uint8_t>>(tx.vout[nOut].vchNEVMData);
        if(vchNEVMData->size() > MAX_NEVM_DATA_BLOB) {
            SetNull();
            return false;
        }
    }
    txid = tx.GetHash();
    return true;
}
bool CNEVMData::UnserializeFromScript(const CScript &scriptPubKey) {
	std::vector<unsigned char> vchData;
	if (!GetSyscoinData(scriptPubKey, vchData))
	{
		SetNull();
		return false;
	}
	if(UnserializeFromData(vchData, PROTOCOL_VERSION) != 0)
	{
		SetNull();
		return false;
	}
    return true;
}
void CNEVMData::SerializeData(std::vector<unsigned char> &vchData) {
    CDataStream dsNEVMAsset(SER_NETWORK, PROTOCOL_VERSION);
    Ser(dsNEVMAsset);
    const auto &bytesVec = MakeUCharSpan(dsNEVMAsset);
	vchData = std::vector<unsigned char>(bytesVec.begin(), bytesVec.end());
}