#include <vbk/test/integration/grpc_integration_service.hpp>

#include <vbk/pop_service/pop_service_exception.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/config.hpp>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

namespace VeriBlock {

GrpcIntegrationService::GrpcIntegrationService()
{
    auto& config = VeriBlock::getService<VeriBlock::Config>();
    std::string ip = config.service_ip;
    std::string port = config.service_port;

    auto creds = grpc::InsecureChannelCredentials();
    std::shared_ptr<Channel> channel = grpc::CreateChannel(ip + ":" + port, creds);

    validationService = VeriBlock::ValidationService::NewStub(channel);
    serializeService = VeriBlock::SerializeService::NewStub(channel);
    integrationService = VeriBlock::IntegrationService::NewStub(channel);
}

bool GrpcIntegrationService::verifyVeriBlockBlock(const VeriBlock::VeriBlockBlock& block)
{
    GeneralReply reply;
    ClientContext context;

    Status status = validationService->VerifyVeriBlockBlock(&context, block, &reply);

    if (!status.ok()) {
        throw PopServiceException(status);
    }

    return reply.result();
}

bool GrpcIntegrationService::verifyBitcoinBlock(const VeriBlock::BitcoinBlock& block)
{
    GeneralReply reply;
    ClientContext context;

    Status status = validationService->VerifyBitcoinBlock(&context, block, &reply);

    if (!status.ok()) {
        throw PopServiceException(status);
    }

    return reply.result();
}

bool GrpcIntegrationService::verifyAltPublication(const VeriBlock::AltPublication& pub)
{
    GeneralReply reply;
    ClientContext context;

    Status status = validationService->VerifyAltPublication(&context, pub, &reply);

    if (!status.ok()) {
        throw PopServiceException(status);
    }

    return reply.result();
}

bool GrpcIntegrationService::checkATVAgainstView(const VeriBlock::AltPublication& pub)
{
    GeneralReply reply;
    ClientContext context;

    Status status = integrationService->CheckATVAgainstView(&context, pub, &reply);

    if (!status.ok()) {
        throw PopServiceException(status);
    }

    return reply.result();
}

std::vector<uint8_t> GrpcIntegrationService::serializeAddress(const VeriBlock::Address& address)
{
    BytesArrayReply reply;
    ClientContext context;
		
    Status status = serializeService->SerializeAddress(&context, address, &reply);

    if (!status.ok()) {
        throw PopServiceException(status);
    }

    return std::vector<unsigned char>(reply.data().begin(), reply.data().end());
}

std::vector<uint8_t> GrpcIntegrationService::serializePublicationData(const VeriBlock::PublicationData& publicationData)
{
    BytesArrayReply reply;
    ClientContext context;
		
    Status status = serializeService->SerializePublicationData(&context, publicationData, &reply);

    if (!status.ok()) {
        throw PopServiceException(status);
    }

    return std::vector<unsigned char>(reply.data().begin(), reply.data().end());
}

std::vector<uint8_t> GrpcIntegrationService::serializeAltPublication(const VeriBlock::AltPublication& publication)
{
    BytesArrayReply reply;
    ClientContext context;
		
    Status status = serializeService->SerializeAltPublication(&context, publication, &reply);

    if (!status.ok()) {
        throw PopServiceException(status);
    }

    return std::vector<unsigned char>(reply.data().begin(), reply.data().end());

}
}