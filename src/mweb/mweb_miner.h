#pragma once

#include <mw/node/BlockBuilder.h>
#include <txmempool.h>

// Forward Declarations
struct CBlockTemplate;

namespace MWEB {

class Miner
{
public:
    void NewBlock(const uint64_t nHeight);
    bool AddMWEBTransaction(CTxMemPool::txiter iter);
    void AddHogExTransaction(const CBlockIndex* pIndexPrev, CBlock* pblock, CBlockTemplate* pblocktemplate, CAmount& nFees);

private:
    bool ValidatePegIns(const CTransactionRef& pTx, const std::vector<PegInCoin>& pegins) const;

    // MWEB Attributes
    mw::BlockBuilder::Ptr mweb_builder;
    CAmount mweb_amount_change;
    CAmount hogex_fees;
    int64_t hogex_sigops;
    std::vector<CTxIn> hogex_inputs;
    std::vector<CTxOut> hogex_outputs;
};

} // namespace MWEB