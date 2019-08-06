#include <omnicore/createtx.h>

#include <omnicore/encoding.h>
#include <omnicore/script.h>

#include <base58.h>
#include <coins.h>
#include <key_io.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>
#include <uint256.h>

#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

/** Creates a new previous output entry. */
PrevTxsEntry::PrevTxsEntry(const uint256& txid, uint32_t nOut, int64_t nValue, const CScript& scriptPubKey)
  : outPoint(txid, nOut), txOut(nValue, scriptPubKey)
{
}

/** Creates a new transaction builder. */
TxBuilder::TxBuilder()
{
}

/** Creates a new transaction builder to extend a transaction. */
TxBuilder::TxBuilder(const CMutableTransaction& transactionIn)
  : transaction(transactionIn)
{
}

/** Adds an outpoint as input to the transaction. */
TxBuilder& TxBuilder::addInput(const COutPoint& outPoint)
{
    CTxIn txIn(outPoint);
    transaction.vin.push_back(txIn);

    return *this;
}

/** Adds a transaction input to the transaction. */
TxBuilder& TxBuilder::addInput(const uint256& txid, uint32_t nOut)
{
    COutPoint outPoint(txid, nOut);

    return addInput(outPoint);
}

/** Adds an output to the transaction. */
TxBuilder& TxBuilder::addOutput(const CScript& scriptPubKey, int64_t value)
{
    CTxOut txOutput(value, scriptPubKey);
    transaction.vout.push_back(txOutput);

    return *this;
}

/** Adds a collection of outputs to the transaction. */
TxBuilder& TxBuilder::addOutputs(const std::vector<std::pair<CScript, int64_t> >& txOutputs)
{
    for (std::vector<std::pair<CScript, int64_t> >::const_iterator it = txOutputs.begin();
            it != txOutputs.end(); ++it) {
        addOutput(it->first, it->second);
    }

    return *this;
}

/** Adds an output for change. */
TxBuilder& TxBuilder::addChange(const CTxDestination& destination, const CCoinsViewCache& view, int64_t txFee, uint32_t position)
{
    CTransaction tx(transaction);

    if (!view.HaveInputs(tx)) {
        return *this;
    }

    CScript scriptPubKey = GetScriptForDestination(destination);

    int64_t txChange = view.GetValueIn(tx) - tx.GetValueOut() - txFee;
    int64_t minValue = OmniGetDustThreshold(scriptPubKey);

    if (txChange < minValue) {
        return *this;
    }

    std::vector<CTxOut>::iterator it = transaction.vout.end();
    if (position < transaction.vout.size()) {
        it = transaction.vout.begin() + position;
    }

    CTxOut txOutput(txChange, scriptPubKey);
    transaction.vout.insert(it, txOutput);

    return *this;
}

/** Returns the created transaction. */
CMutableTransaction TxBuilder::build()
{
    return transaction;
}

/** Creates a new Omni transaction builder. */
OmniTxBuilder::OmniTxBuilder()
  : TxBuilder()
{
}

/** Creates a new Omni transaction builder to extend a transaction. */
OmniTxBuilder::OmniTxBuilder(const CMutableTransaction& transactionIn)
  : TxBuilder(transactionIn)
{
}

/** Adds a collection of previous outputs as inputs to the transaction. */
OmniTxBuilder& OmniTxBuilder::addInputs(const std::vector<PrevTxsEntry>& prevTxs)
{
    for (std::vector<PrevTxsEntry>::const_iterator it = prevTxs.begin();
            it != prevTxs.end(); ++it) {
        addInput(it->outPoint);
    }

    return *this;
}

/** Adds an output for the reference address. */
OmniTxBuilder& OmniTxBuilder::addReference(const std::string& destination, int64_t value)
{
    CTxDestination dest = DecodeDestination(destination);
    CScript scriptPubKey = GetScriptForDestination(dest);

    int64_t minValue = OmniGetDustThreshold(scriptPubKey);
    value = std::max(minValue, value);

    return (OmniTxBuilder&) TxBuilder::addOutput(scriptPubKey, value);
}

/** Embeds a payload with class C (op-return) encoding. */
OmniTxBuilder& OmniTxBuilder::addOpReturn(const std::vector<unsigned char>& data)
{
    std::vector<std::pair<CScript, int64_t> > outputs;

    if (!OmniCore_Encode_ClassC(data, outputs)) {
        return *this;
    }

    return (OmniTxBuilder&) TxBuilder::addOutputs(outputs);
}

/** Embeds a payload with class B (bare-multisig) encoding. */
OmniTxBuilder& OmniTxBuilder::addMultisig(const std::vector<unsigned char>& data, const std::string& seed, const CPubKey& pubKey)
{
    std::vector<std::pair<CScript, int64_t> > outputs;

    if (!OmniCore_Encode_ClassB(seed, pubKey, data, outputs)) {
        return *this;
    }

    return (OmniTxBuilder&) TxBuilder::addOutputs(outputs);
}

/** Adds an output for change. */
OmniTxBuilder& OmniTxBuilder::addChange(const std::string& destination, const CCoinsViewCache& view, int64_t txFee, uint32_t position)
{
    CTxDestination dest = DecodeDestination(destination);

    return (OmniTxBuilder&) TxBuilder::addChange(dest, view, txFee, position);
}

/** Adds previous transaction outputs to coins view. */
void InputsToView(const std::vector<PrevTxsEntry>& prevTxs, CCoinsViewCache& view)
{
    for (std::vector<PrevTxsEntry>::const_iterator it = prevTxs.begin(); it != prevTxs.end(); ++it) {
        Coin newcoin;
        newcoin.out.scriptPubKey = it->txOut.scriptPubKey;
        newcoin.out.nValue = it->txOut.nValue;
        view.AddCoin(it->outPoint, std::move(newcoin), true);
    }
}

