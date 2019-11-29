#ifndef BITCOIN_SRC_VBK_POP_SERVICE_POP_SERVICE_IMPL_HPP
#define BITCOIN_SRC_VBK_POP_SERVICE_POP_SERVICE_IMPL_HPP

#include "vbk/pop_service.hpp"
#include "vbk/pop_service/pop_service_exception.hpp"

#include <memory>
#include <vector>

#include <chainparams.h>

#include <grpc++/grpc++.h>

#include "integration.grpc.pb.h"

namespace VeriBlock {

class PopServiceImpl : public PopService
{
private:
    std::shared_ptr<VeriBlock::IntegrationService::Stub> integrationService;
    std::shared_ptr<VeriBlock::RewardsService::Stub> rewardsService;
    std::shared_ptr<VeriBlock::DeserializeService::Stub> deserializeService;
    std::shared_ptr<VeriBlock::ForkresolutionService::Stub> forkresolutionService;

public:
    PopServiceImpl();

    ~PopServiceImpl() override = default;

    void savePopTxToDatabase(const CBlock& block, const int& nHeight) override;

    std::vector<BlockBytes> getLastKnownVBKBlocks(size_t blocks) override;
    std::vector<BlockBytes> getLastKnownBTCBlocks(size_t blocks) override;

    bool checkVTBinternally(const std::vector<uint8_t>& bytes) override;
    bool checkATVinternally(const std::vector<uint8_t>& bytes) override;

    int compareTwoBranches(const CBlockIndex* commonKeystone, const CBlockIndex* leftForkTip, const CBlockIndex* rightForkTip) override;

    void rewardsCalculateOutputs(const int& blockHeight, const CBlockIndex* endorsedBlock, const CBlockIndex* contaningBlocksTip, const std::string& difficulty, std::map<CScript, int64_t>& outputs) override;
    std::string rewardsCalculatePopDifficulty(const CBlockIndex* start_interval, const CBlockIndex* end_interval) override;

    bool blockPopValidation(const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, CValidationState& state) override;

public:
    virtual void parseAltPublication(const std::vector<uint8_t>&, VeriBlock::AltPublication&);
    virtual void parseVeriBlockPublication(const std::vector<uint8_t>&, VeriBlock::VeriBlockPublication&);
    virtual void getPublicationsData(const CTransactionRef& tx, AltPublication& ATV, std::vector<VeriBlockPublication>& VTBs);

    virtual bool determineATVPlausibilityWithBTCRules(AltchainId altChainIdentifier, const CBlockHeader& popEndorsementHeader, const Consensus::Params& params);

    virtual bool addPayloads(const CBlock&, const int&, const std::vector<VeriBlockPublication>&, const VeriBlock::AltPublication&);
    virtual void removePayloads(const CBlock&, const int&);
};

bool blockPopValidationImpl(PopServiceImpl& pop, const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, CValidationState& state);

} // namespace VeriBlock
#endif //BITCOIN_SRC_VBK_POP_SERVICE_POP_SERVICE_IMPL_HPP
