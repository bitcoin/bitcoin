#pragma once

#include <consensus/params.h>
#include <mw/node/CoinsView.h>

// Forward Declarations
class CBlock;
class CBlockUndo;
class CBlockIndex;
class CTransaction;
class BlockValidationState;
class TxValidationState;

namespace MWEB {

// MW: TODO - Fix function summaries now that we've rearranged the checks.
class Node
{
public:
    /// <summary>
    /// Context-independent validation of the CBlock's MWEB rules. If MWEB included in block, this verifies:
    /// * Only the final transaction in the block is marked as the HogEx
    /// * All MWEB data has been stripped from canonical transactions (i.e. HasMWEBTx() should be false)
    /// * MWEB header hash matches hash committed to by HogEx transaction
    /// * Peg-In kernels match canonical peg-in outputs (amounts and commitments)
    /// * Peg-Out kernels match HogEx peg-out outputs (amounts and scriptPubKeys)
    /// * MWEB block does not exceed max weight
    /// * Inputs, outputs, and kernels are properly sorted
    /// * No invalid duplicate inputs, outputs, or kernels
    /// * Kernel MMR size and root match the MWEB header
    /// * All signatures and rangeproofs are valid
    /// * Kernel features are valid
    /// * Owner sums balance (i.e. sender keys, receiver keys, and owner offset balance out)
    /// </summary>
    /// <param name="block">The CBlock to validate.</param>
    /// <param name="state">The CValidationState to update if validation fails.</param>
    /// <returns>True if there is no MWEB included, or MWEB context-independent validation checks have succeeded</returns>
    static bool CheckBlock(const CBlock& block, BlockValidationState& state);

    /// <summary>
    /// MWEB validation checks that require knowledge of the block's context (i.e. the previous CBlockIndex must be known).
    /// The following rules are verified:
    /// * MWEB is included when the feature is active, or not included when MWEB has not yet been activated
    /// * MWEB header block height matches the expected height
    /// * The first input in the HogEx points to the HogAddr output of the previous HogEx (except for HogEx in first block after MWEB activated)
    /// * The remaining inputs in the HogEx exactly match the pegin outputs from the block's transactions
    /// * HogEx fee matches the total fee of the extension block
    /// * HogAddr amount differs from previous HogAddr amount by the exact amount expected (previous + pegins - pegouts - fees)
    /// </summary>
    /// <param name="block">The CBlock to validate.</param>
    /// <param name="consensus_params">The consensus parameters defined for the network.</param>
    /// <param name="pindexPrev">The CBlockIndex directly before the CBlock being checked.</param>
    /// <param name="state">The CValidationState to update if validation fails.</param>
    /// <returns>True if all validation checks succeed.</returns>
    static bool ContextualCheckBlock(
        const CBlock& block,
        const Consensus::Params& consensus_params,
        const CBlockIndex* pindexPrev,
        BlockValidationState& state
    );

    /// <summary>
    /// Applies the extension block to the end of the chain in the given view, updating the UTXO set in the process.
    /// The following rules are verified while connecting the block:
    /// * Kernel sums balance, proving no inflation occurred
    /// * All inputs being spent were in the UTXO set
    /// * After applying the UTXO set updates, the TXO PMMR size & root and leafset root match the MWEB header
    /// </summary>
    /// <param name="block">The CBlock to connect.</param>
    /// <param name="consensus_params">The consensus parameters defined for the network.</param>
    /// <param name="pindexPrev">The CBlockIndex directly before the CBlock being connected.</param>
    /// <param name="blockundo">The CBlockUndo which will be updated to include the MWEB undo data upon success.</param>
    /// <param name="mweb_view">The CoinsViewCache the block should be connected to.</param>
    /// <param name="state">The CValidationState to update if validation fails.</param>
    /// <returns>True if all validation checks succeed, and the block is connected.</returns>
    static bool ConnectBlock(
        const CBlock& block,
        const Consensus::Params& consensus_params,
        const CBlockIndex* pindexPrev,
        CBlockUndo& blockundo,
        mw::CoinsViewCache& mweb_view,
        BlockValidationState& state
    );

    /// <summary>
    /// Performs basic context-independent validation checks on individual transactions.
    /// The following rules are verified:
    /// * No pegin witness programs are included in Coinbase and HogEx outputs
    /// * Kernel sums balance, proving no inflation occurred, and the sender(s) knew the input blinding factors
    /// * Owner sums balance, proving the sender(s) knew the input receiver keys
    /// * MWEB transaction does not exceed max weight
    /// * Inputs, outputs, and kernels are properly sorted
    /// * No invalid duplicate inputs, outputs, or kernels
    /// * All signatures and rangeproofs are valid
    /// * Kernel features are valid
    /// 
    /// WARNING: Don't apply this when validating blocks that pre-date MWEB activation.
    /// </summary>
    /// <param name="tx">The CTransaction to validate.</param>
    /// <param name="state">The CValidationState to update if validation fails.</param>
    /// <returns>True if all validation checks succeed.</returns>
    static bool CheckTransaction(const CTransaction& tx, TxValidationState& state);

private:
    static bool ValidateMWEBBlock(const CBlock& block);
};

}