// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/transaction.h"

#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"

#include <streams.h>

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

uint256 CTxOut::GetHash() const
{
    return SerializeHash(*this);
}

std::string CTxOut::ToString() const
{
    return strprintf("CTxOut(nValue=%d.%08d, scriptPubKey=%s)", nValue / COIN, nValue % COIN, HexStr(scriptPubKey).substr(0, 30));
}

CMutableTransaction::CMutableTransaction() : nVersion(CTransaction::CURRENT_VERSION), nLockTime(0) {}
CMutableTransaction::CMutableTransaction(const CTransaction& tx) : nVersion(tx.nVersion), vin(tx.vin), vout(tx.vout), nLockTime(tx.nLockTime) {}

uint256 CMutableTransaction::GetHash() const
{
    CHashWriter ss(0, 0);
    SerializeTransaction(*this, ss, 0, 0, false);
    return ss.GetHash();
}

void CTransaction::UpdateHash()
{
    if (nVersion == 4) {
        if (txData.empty()) {
            CDataStream stream(0, 0);
            SerializeTransaction(*this, stream, 0, 0, false);
            txData = std::vector<char>(stream.begin(), stream.end());
        }
        CHashWriter ss(0, 0);
        ss.write(&txData[0], txData.size());
        hash = ss.GetHash();
    } else {
        hash = SerializeHash(*this);
    }
}

CTransaction::CTransaction() : nVersion(CTransaction::CURRENT_VERSION), vin(), vout(), nLockTime(0) { }

CTransaction::CTransaction(const CMutableTransaction &tx) : nVersion(tx.nVersion), vin(tx.vin), vout(tx.vout), nLockTime(tx.nLockTime) {
    UpdateHash();
}

CTransaction& CTransaction::operator=(const CTransaction &tx) {
    *const_cast<int*>(&nVersion) = tx.nVersion;
    *const_cast<std::vector<CTxIn>*>(&vin) = tx.vin;
    *const_cast<std::vector<CTxOut>*>(&vout) = tx.vout;
    *const_cast<unsigned int*>(&nLockTime) = tx.nLockTime;
    hash = tx.hash;
    txData = tx.txData;
    return *this;
}

uint256 CTransaction::CalculateSignaturesHash() const
{
    CHashWriter ss(0, 0);
    ss << hash;
    for (auto in : vin) {
        CMFToken token(Consensus::TxInScript, std::vector<char>(in.scriptSig.begin(), in.scriptSig.end()));
        ::Serialize<CHashWriter,CMFToken>(ss, token, 0, 0);
    }
    return ss.GetHash();
}

std::vector<char> loadTransaction(const std::vector<CMFToken> &tokens, std::vector<CTxIn> &inputs, std::vector<CTxOut> &outputs, int nVersion)
{
    assert(inputs.empty());
    assert(outputs.empty());
    unsigned int inputCount = 0;
    bool storedOutValue = false, storedOutScript = false;
    int64_t outValue = 0;
    std::vector<char> txData;

    for (unsigned int index = 0; index < tokens.size(); ++index) {
        const auto token = tokens[index];
        switch (token.tag) {
        case Consensus::TxInPrevHash: {
            auto data = boost::get<std::vector<char> >(token.data);
            inputs.push_back(CTxIn(COutPoint(uint256(&data[0]), 0)));
            break;
        }
        case Consensus::TxInPrevIndex: {
            int n = boost::get<int32_t>(token.data);
            inputs[inputs.size()-1].prevout.n = n;
            break;
        }
        case Consensus::TxInScript: {
            if (inputCount == 0) {
                CDataStream stream(0, 4);
                ser_writedata32(stream, nVersion);
                for (unsigned int i = 0; i < index; ++i) {
                    tokens[i].Serialize(stream, 0, 4);
                }
                txData = std::vector<char>(stream.begin(), stream.end());
            }
            auto data = token.unsignedByteArray();
            inputs[inputCount++].scriptSig = CScript(data.begin(), data.end());
            break;
        }
        case Consensus::TxEnd:
            return txData;

            // TxOut* don't have a pre-defined order, just that both are required so they always have to come in pairs.
        case Consensus::TxOutValue:
            if (storedOutScript) { // add it.
                outputs[outputs.size() -1].nValue = token.longData();
                storedOutScript = storedOutValue = false;
            } else { // store it.
                outValue = token.longData();
                storedOutValue = true;
            }
            break;
        case Consensus::TxOutScript: {
            auto data = token.unsignedByteArray();
            outputs.push_back(CTxOut(outValue, CScript(data.begin(), data.end())));
            if (storedOutValue)
                storedOutValue = false;
            else
                storedOutScript = true;
            break;
        }
        case Consensus::LockByBlock:
        case Consensus::LockByTime:
        case Consensus::ScriptVersion:
            // TODO
            break;
        default:
            if (token.tag > 19)
                throw std::runtime_error("Illegal tag in transaction");
        }
    }
    return txData;
}

CAmount CTransaction::GetValueOut() const
{
    CAmount nValueOut = 0;
    for (std::vector<CTxOut>::const_iterator it(vout.begin()); it != vout.end(); ++it)
    {
        nValueOut += it->nValue;
        if (!MoneyRange(it->nValue) || !MoneyRange(nValueOut))
            throw std::runtime_error("CTransaction::GetValueOut(): value out of range");
    }
    return nValueOut;
}

double CTransaction::ComputePriority(double dPriorityInputs, unsigned int nTxSize) const
{
    nTxSize = CalculateModifiedSize(nTxSize);
    if (nTxSize == 0) return 0.0;

    return dPriorityInputs / nTxSize;
}

unsigned int CTransaction::CalculateModifiedSize(unsigned int nTxSize) const
{
    // In order to avoid disincentivizing cleaning up the UTXO set we don't count
    // the constant overhead for each txin and up to 110 bytes of scriptSig (which
    // is enough to cover a compressed pubkey p2sh redemption) for priority.
    // Providing any more cleanup incentive than making additional inputs free would
    // risk encouraging people to create junk outputs to redeem later.
    if (nTxSize == 0)
        nTxSize = ::GetSerializeSize(*this, SER_NETWORK, PROTOCOL_VERSION);
    for (std::vector<CTxIn>::const_iterator it(vin.begin()); it != vin.end(); ++it)
    {
        unsigned int offset = 41U + std::min(110U, (unsigned int)it->scriptSig.size());
        if (nTxSize > offset)
            nTxSize -= offset;
    }
    return nTxSize;
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
    for (unsigned int i = 0; i < vin.size(); i++)
        str += "    " + vin[i].ToString() + "\n";
    for (unsigned int i = 0; i < vout.size(); i++)
        str += "    " + vout[i].ToString() + "\n";
    return str;
}
