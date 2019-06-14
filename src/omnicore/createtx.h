#ifndef OMNICORE_CREATETX_H
#define OMNICORE_CREATETX_H

class CCoinsViewCache;
class CMutableTransaction;
class COutPoint;
class CPubKey;
class CScript;
class CTxOut;
class uint256;

#include "script/standard.h"

#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

/**
 * A structure representing previous transaction outputs.
 */
struct PrevTxsEntry
{
    /** Creates a new previous output entry. */
    PrevTxsEntry(const uint256& txid, uint32_t nOut, int64_t nValue, const CScript& scriptPubKey);

    COutPoint outPoint;
    CTxOut txOut;
};

// TODO:
// CMutableTransaction tx = OmniTxBuilder().addInput(input).addOpReturn(data).build();
// ... currently doesn't work, because addInput() returns a reference to a TxBuilder,
// but not to an OmniTxBuilder.

/**
 * Builder to create transactions.
 *
 * The TxBuilder class can be used to build transactions by adding outputs to a
 * newly created, or an already existing, transaction. The builder can also be
 * used to add change outputs.
 *
 * Example usage:
 *
 * @code
 *   // build a transaction with two outputs and change:
 *   CMutableTransaction tx = TxBuilder()
 *           .addInput(outPoint)
 *           .addOutput(scriptPubKeyA, nValueA)
 *           .addOutput(scriptPubKeyB, nValueB)
 *           .addChange(destinationForChange, viewContainingInputs, nFee)
 *           .build();
 * @endcode
 */
class TxBuilder
{
public:
    /**
     * Creates a new transaction builder.
     */
    TxBuilder();

    /**
     * Creates a new transaction builder to extend a transaction.
     *
     * @param transactionIn The transaction used to build upon
     */
    TxBuilder(const CMutableTransaction& transactionIn);

    /**
     * Adds an outpoint as input to the transaction.
     *
     * @param outPoint The transaction outpoint to add
     */
    TxBuilder& addInput(const COutPoint& outPoint);

    /**
     * Adds a transaction input to the transaction.
     *
     * @param txid The hash of the input transaction
     * @param nOut The index of the transaction output used as input
     */
    TxBuilder& addInput(const uint256& txid, uint32_t nOut);

    /**
     * Adds an output to the transaction.
     *
     * @param scriptPubKey The output script
     * @param value        The output value
     */
    TxBuilder& addOutput(const CScript& scriptPubKey, int64_t value);

    /**
     * Adds a collection of outputs to the transaction.
     *
     * @param txOutputs The outputs to add
     */
    TxBuilder& addOutputs(const std::vector<std::pair<CScript, int64_t> >& txOutputs);

    /**
     * Adds an output for change.
     *
     * Optionally a position can be provided, where the change output should be
     * inserted, starting with 0. If the number of outputs is smaller than the
     * position, then the change output is added to the end.
     *
     * If the change amount would be considered as dust, then no change output
     * is added.
     *
     * @param destination The destination of the change
     * @param view        The coins view, which contains the inputs being spent
     * @param txFee       The desired transaction fees
     * @param position    The position of the change output (default: last position)
     */
    TxBuilder& addChange(const CTxDestination& destination, const CCoinsViewCache& view, int64_t txFee, uint32_t position = 999999);

    /**
     * Returns the created transaction.
     *
     * @return The created transaction
     */
    CMutableTransaction build();

protected:
    CMutableTransaction transaction;
};

/**
 * Builder to create Omni transactions.
 *
 * The OmniTxBuilder class is an extension of the TxBuilder, with additional
 * methods to build Omni transactions. Payloads can be embedded with class B
 * (bare-multisig) or class C (op-return) encoding.
 *
 * The output values are based on the dust threshold, but may optionally be
 * higher for reference outputs.
 *
 * Example usage:
 *
 * @code
 *   // add a bare-multisig encoded payload to an existing transaction:
 *   CMutableTransaction modifiedTx = OmniTxBuilder(basisTx)
 *           .addMultisig(payload, obfuscationSeed, redeemingPubKey)
 *           .build();
 * @endcode
 */
class OmniTxBuilder: public TxBuilder
{
public:
    /**
     * Creates a new Omni transaction builder.
     */
    OmniTxBuilder();

    /**
     * Creates a new Omni transaction builder to extend a transaction.
     *
     * @param transactionIn The transaction used to build upon
     */
    OmniTxBuilder(const CMutableTransaction& transactionIn);

    /**
     * Adds a collection of previous outputs as inputs to the transaction.
     *
     * @param outPoint The transaction outpoint to add
     */
    OmniTxBuilder& addInputs(const std::vector<PrevTxsEntry>& prevTxs);

    /**
     * Adds an output for the reference address.
     *
     * The output value is set to at least the dust threshold.
     *
     * @param destination The reference address
     * @param value       The optional reference amount
     */
    OmniTxBuilder& addReference(const std::string& destination, int64_t value = 0);

    /**
     * Embeds a payload with class C (op-return) encoding.
     *
     * If the data encoding fails, then the transaction is not modified.
     *
     * @param data The payload to embed
     */
    OmniTxBuilder& addOpReturn(const std::vector<unsigned char>& data);

    /**
     * Embeds a payload with class B (bare-multisig) encoding.
     *
     * If the data encoding fails, then the transaction is not modified.
     *
     * @param data   The payload to embed
     * @param seed   The address of the sender, used as seed for obfuscation
     * @param pubKey A public key that may be used to redeem the multisig dust
     */
    OmniTxBuilder& addMultisig(const std::vector<unsigned char>& data, const std::string& seed, const CPubKey& pubKey);

    /**
     * Adds an output for change.
     *
     * Optionally a position can be provided, where the change output should be
     * inserted, starting with 0. If the number of outputs is smaller than the
     * position, then the change output is added to the end.
     *
     * If no position is specified, then the change output is added to the first
     * position. This default behavior differs from the TxBuilder class.
     *
     * If the change amount would be considered as dust, then no change output is added.
     *
     * @param destination The destination of the change
     * @param view        The coins view, which contains the inputs being spent
     * @param txFee       The desired transaction fees
     * @param position    The position of the change output (default: first position)
     */
    OmniTxBuilder& addChange(const std::string& destination, const CCoinsViewCache& view, int64_t txFee, uint32_t position = 0);
};

/**
 * Adds previous transaction outputs to coins view.
 *
 * @param prevTxs The previous transaction outputs
 * @param view    The coins view to modify
 */
void InputsToView(const std::vector<PrevTxsEntry>& prevTxs, CCoinsViewCache& view);


#endif // OMNICORE_CREATETX_H
