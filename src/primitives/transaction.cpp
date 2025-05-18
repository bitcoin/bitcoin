// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>

#include <consensus/amount.h>
#include <crypto/hex_base.h>
#include <hash.h>
#include <script/script.h>
#include <serialize.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/transaction_identifier.h>

#include <algorithm>
#include <cassert>
#include <stdexcept>

std::string COutPoint::ToString() const
{
    return strprintf("COutPoint(%s, %u)", hash.ToString().substr(0,10), n);
}

CTxIn::CTxIn(COutPoint prevoutIn, CScript scriptSigIn, uint32_t nSequenceIn)
{
    prevout = prevoutIn;
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

CTxIn::CTxIn(Txid hashPrevTx, uint32_t nOut, CScript scriptSigIn, uint32_t nSequenceIn)
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
    return strprintf("CTxOut(nValue=%d.%08d, scriptPubKey=%s)", nValue / COIN, nValue % COIN, HexStr(scriptPubKey).substr(0, 30));
}

CMutableTransaction::CMutableTransaction() : version{CTransaction::CURRENT_VERSION}, nLockTime{0} {}
CMutableTransaction::CMutableTransaction(const CTransaction& tx) : vin(tx.vin), vout(tx.vout), version{tx.version}, nLockTime{tx.nLockTime} {}

Txid CMutableTransaction::GetHash() const
{
    return Txid::FromUint256((HashWriter{} << TX_NO_WITNESS(*this)).GetHash());
}

bool CTransaction::ComputeHasWitness() const
{
    return std::any_of(vin.begin(), vin.end(), [](const auto& input) {
        return !input.scriptWitness.IsNull();
    });
}

Txid CTransaction::ComputeHash() const
{
    return Txid::FromUint256((HashWriter{} << TX_NO_WITNESS(*this)).GetHash());
}

Wtxid CTransaction::ComputeWitnessHash() const
{
    if (!HasWitness()) {
        return Wtxid::FromUint256(hash.ToUint256());
    }

    return Wtxid::FromUint256((HashWriter{} << TX_WITH_WITNESS(*this)).GetHash());
}

CTransaction::CTransaction(const CMutableTransaction& tx) : vin(tx.vin), vout(tx.vout), version{tx.version}, nLockTime{tx.nLockTime}, m_has_witness{ComputeHasWitness()}, hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {}
CTransaction::CTransaction(CMutableTransaction&& tx) : vin(std::move(tx.vin)), vout(std::move(tx.vout)), version{tx.version}, nLockTime{tx.nLockTime}, m_has_witness{ComputeHasWitness()}, hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {}

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
    return ::GetSerializeSize(TX_WITH_WITNESS(*this));
}

std::string CTransaction::ToString() const
{
    std::string str;
    str += strprintf("CTransaction(hash=%s, ver=%u, vin.size=%u, vout.size=%u, nLockTime=%u)\n",
        GetHash().ToString().substr(0,10),
        version,
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
