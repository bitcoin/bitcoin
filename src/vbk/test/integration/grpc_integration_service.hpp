#ifndef VERIBLOCK_INTEGRATION_TEST_VALIDATION_SERVICE_H
#define VERIBLOCK_INTEGRATION_TEST_VALIDATION_SERVICE_H

#include <memory>

#include <grpc++/grpc++.h>

#include <vbk/pop_service/integration.grpc.pb.h>

namespace VeriBlock {

class GrpcIntegrationService
{
private:
    std::shared_ptr<VeriBlock::ValidationService::Stub> validationService;
    std::shared_ptr<VeriBlock::SerializeService::Stub> serializeService;
    std::shared_ptr<VeriBlock::IntegrationService::Stub> integrationService;

public:
    GrpcIntegrationService();

    ~GrpcIntegrationService() = default;

    bool verifyVeriBlockBlock(const VeriBlock::VeriBlockBlock& block);
    bool verifyBitcoinBlock(const VeriBlock::BitcoinBlock& block);
    bool verifyAltPublication(const VeriBlock::AltPublication& pub);
    bool checkATVAgainstView(const VeriBlock::AltPublication& pub);

    std::vector<uint8_t> serializeAltPublication(const VeriBlock::AltPublication& publication);
    std::vector<uint8_t> serializeAddress(const VeriBlock::Address& address);
    std::vector<uint8_t> serializePublicationData(const VeriBlock::PublicationData& publicationData);
};
} // namespace VeriBlock
#endif
