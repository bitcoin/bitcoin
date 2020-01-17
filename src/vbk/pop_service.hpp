#ifndef BITCOIN_SRC_VBK_POP_SERVICE_HPP
#define BITCOIN_SRC_VBK_POP_SERVICE_HPP

#include <map>
#include <vector>

#include <consensus/validation.h>
#include <script/interpreter.h>
#include <vbk/entity/context_info_container.hpp>
#include <vbk/entity/pop.hpp>

class CBlock;
class CTransaction;
class COutPoint;
class CBlockIndex;
class uint256;
class CScript;

namespace Consensus {
struct Params;
}

// GRPC calls of the service

namespace VeriBlock {

using BlockBytes = std::vector<uint8_t>;

struct PopService {
    virtual ~PopService() = default;

    virtual void savePopTxToDatabase(const CBlock& block, const int& nHeight) = 0;

    virtual std::vector<BlockBytes> getLastKnownVBKBlocks(size_t blocks) = 0;
    virtual std::vector<BlockBytes> getLastKnownBTCBlocks(size_t blocks) = 0;

    virtual bool checkVTBinternally(const std::vector<uint8_t>& bytes) = 0;
    virtual bool checkATVinternally(const std::vector<uint8_t>& bytes) = 0;

    virtual int compareTwoBranches(const CBlockIndex* commonKeystone, const CBlockIndex* leftForkTip, const CBlockIndex* rightForkTip) = 0;

    virtual void rewardsCalculateOutputs(const int& blockHeight, const CBlockIndex& endorsedBlock, const CBlockIndex& contaningBlocksTip, const CBlockIndex* difficulty_start_interval, const CBlockIndex* difficulty_end_interval, std::map<CScript, int64_t>& outputs) = 0;

    virtual bool blockPopValidation(const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, BlockValidationState& state) = 0;

    virtual void updateContext(const std::vector<std::vector<uint8_t>>& veriBlockBlocks, const std::vector<std::vector<uint8_t>>& bitcoinBlocks) = 0;

    virtual bool parsePopTx(const CTransactionRef& tx, Publications* publications, Context* ctx, PopTxType* type) = 0;

    virtual bool determineATVPlausibilityWithBTCRules(AltchainId altChainIdentifier, const CBlockHeader& popEndorsementHeader, const Consensus::Params& params) = 0;

    virtual void addPayloads(const CBlock& block, const int& nHeight, const Publications& publications) = 0;
    virtual void removePayloads(const CBlock&, const int&) = 0;
    virtual void setConfig() = 0;
};
} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_POP_SERVICE_HPP
