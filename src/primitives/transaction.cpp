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
CTxOut::CTxOut(const CAmount& nValueIn,  const CScript &scriptPubKeyIn)
{
    nValue = nValueIn;
    scriptPubKey = scriptPubKeyIn;
    vchNEVMData.clear();
}
std::string CTxOut::ToString() const
{
    return strprintf("CTxOut(nValue=%d.%08d, scriptPubKey=%s)", nValue / COIN, nValue % COIN, HexStr(scriptPubKey).substr(0, 30));
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
CTransaction::CTransaction(const CMutableTransaction& tx) : vin(tx.vin), vout(tx.vout), nVersion(tx.nVersion), nLockTime(tx.nLockTime), hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {}
CTransaction::CTransaction(CMutableTransaction&& tx) : vin(std::move(tx.vin)), vout(std::move(tx.vout)), nVersion(tx.nVersion), nLockTime(tx.nLockTime), hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {}

CAmount CTransaction::GetValueOut() const
{
    CAmount nValueOut = 0;
    for (const auto& tx_out : vout) {
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

bool CTransaction::IsNEVMData() const
{
    return IsSyscoinNEVMDataTx(nVersion);
}
bool CMutableTransaction::IsNEVMData() const
{
    return IsSyscoinNEVMDataTx(nVersion);
}
bool CTransaction::IsMintTx() const
{
    return IsSyscoinMintTx(nVersion);
}
bool CTransaction::IsMnTx() const
{
    return IsMasternodeTx(nVersion);
}
bool CMutableTransaction::IsMnTx() const
{
    return IsMasternodeTx(nVersion);
}
bool CMutableTransaction::IsMintTx() const
{
    return IsSyscoinMintTx(nVersion);
}
bool IsSyscoinMintTx(const int &nVersion) {
    return nVersion == SYSCOIN_TX_VERSION_MINT;
}

bool IsSyscoinNEVMDataTx(const int &nVersion) {
    return nVersion == SYSCOIN_TX_VERSION_NEVM_DATA_SHA3;
}
bool IsMasternodeTx(const int &nVersion) {
    return nVersion == SYSCOIN_TX_VERSION_MN_COINBASE ||
     nVersion == SYSCOIN_TX_VERSION_MN_QUORUM_COMMITMENT ||
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
    if(!vchData.empty() && vchData[0] == 0xaa &&
        vchData[1] == 0x21 &&
        vchData[2] == 0xa9 &&
        vchData[3] == 0xed &&
        vchData.size() >= 36) {
        if (!scriptPubKey.GetOp(pc, opcode, vchData))
		    return false;
    }
    // shouldn't be another opcode after this
	return !scriptPubKey.GetOp(pc, opcode);
}

CMintSyscoin::CMintSyscoin(const CTransaction &tx) {
    SetNull();
    UnserializeFromTx(tx);
}
CMintSyscoin::CMintSyscoin(const CMutableTransaction &mtx) {
    SetNull();
    UnserializeFromTx(mtx);
}

int CMintSyscoin::UnserializeFromData(const std::vector<unsigned char> &vchData) {
    try {
        CDataStream dsMS(vchData, SER_NETWORK, PROTOCOL_VERSION);
        Unserialize(dsMS);
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
        if(vchVersionHash.size() != 32) {
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
        vchNEVMData = &tx.vout[nOut].vchNEVMData;
        if(vchNEVMData->size() > MAX_NEVM_DATA_BLOB) {
            // avoid from deleting in SetNull because vchNEVMData memory isn't owned by CNEVMData
            vchNEVMData = nullptr;
            SetNull();
            return false;
        }
    }
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
    CDataStream dsNEVM(SER_NETWORK, PROTOCOL_VERSION);
    Ser(dsNEVM);
    const auto &bytesVec = MakeUCharSpan(dsNEVM);
	vchData = std::vector<unsigned char>(bytesVec.begin(), bytesVec.end());
}