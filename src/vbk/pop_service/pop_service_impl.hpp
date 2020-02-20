#ifndef BITCOIN_SRC_VBK_POP_SERVICE_POP_SERVICE_IMPL_HPP
#define BITCOIN_SRC_VBK_POP_SERVICE_POP_SERVICE_IMPL_HPP

#include <vbk/entity/pop.hpp>
#include <vbk/pop_service.hpp>
#include <vbk/pop_service/pop_service_exception.hpp>

#include <memory>
#include <vector>

#include <chainparams.h>

#include <grpc++/grpc++.h>
#include <sync.h>

#include "integration.grpc.pb.h"

namespace VeriBlock {

class PopServiceImpl : public PopService
{
private:
    std::mutex mutex;
    std::shared_ptr<VeriBlock::GrpcPopService::Stub> grpcPopService;

public:
    PopServiceImpl(bool altautoconfig = false);

    ~PopServiceImpl() override = default;

    void clearTemporaryPayloads() override;

    void savePopTxToDatabase(const CBlock& block, const int& nHeight) override;

    std::vector<BlockBytes> getLastKnownVBKBlocks(size_t blocks) override;
    std::vector<BlockBytes> getLastKnownBTCBlocks(size_t blocks) override;

    bool checkVTBinternally(const std::vector<uint8_t>& bytes) override;
    bool checkATVinternally(const std::vector<uint8_t>& bytes) override;

    int compareTwoBranches(const CBlockIndex* commonKeystone, const CBlockIndex* leftForkTip, const CBlockIndex* rightForkTip) override;

    void rewardsCalculateOutputs(const int& blockHeight, const CBlockIndex& endorsedBlock, const CBlockIndex& contaningBlocksTip, const CBlockIndex* difficulty_start_interval, const CBlockIndex* difficulty_end_interval, std::map<CScript, int64_t>& outputs) override;

    bool blockPopValidation(const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, BlockValidationState& state) override;

    void updateContext(const std::vector<std::vector<uint8_t>>& veriBlockBlocks, const std::vector<std::vector<uint8_t>>& bitcoinBlocks) override;

    bool parsePopTx(const CTransactionRef& tx, ScriptError* serror, Publications* publications, Context* ctx, PopTxType* type) override;

    bool determineATVPlausibilityWithBTCRules(AltchainId altChainIdentifier, const CBlockHeader& popEndorsementHeader, const Consensus::Params& params, TxValidationState& state) override;

    void addPayloads(std::string blockHash, const int& nHeight, const Publications& publications) override;
    void addPayloads(const CBlockIndex & blockIndex, const CBlock & block) override;

    void removePayloads(const CBlockIndex & blockIndex) override;
    void removePayloads(std::string blockHash, const int& blockHeight) override;

    void setConfig() override;

public:
    virtual void getPublicationsData(const Publications& tx, PublicationData& publicationData);
};

bool blockPopValidationImpl(PopServiceImpl& pop, const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, BlockValidationState& state);

bool txPopValidation(PopServiceImpl& pop, const CTransactionRef& tx, const CBlockIndex& pindexPrev, const Consensus::Params& params, TxValidationState& state, uint32_t heightIndex);

} // namespace VeriBlock
#endif //BITCOIN_SRC_VBK_POP_SERVICE_POP_SERVICE_IMPL_HPP
