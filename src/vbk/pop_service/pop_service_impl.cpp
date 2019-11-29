#include "pop_service_impl.hpp"

#include <memory>

#include <amount.h>
#include <chain.h>
#include <consensus/validation.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/sigcache.h>
#include <streams.h>
#include <validation.h>

#include <vbk/merkle.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/util.hpp>
#include <vbk/util_service.hpp>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

namespace {

template <typename T>
void handleResult(bool rpcresult, const VeriBlock::GeneralReply& reply, const T& in, T& out)
{
    if (!rpcresult) {
        throw VeriBlock::PopServiceException("alt-integration is shutdown");
    }
    if (!reply.result()) {
        std::string error_message("GRPC returned error : %s", reply.resultmessage().c_str());
        throw VeriBlock::PopServiceException(error_message.c_str());
    }

    // do a copy only if rpc succeeded
    out = in;
}

template <class T>
using DeserializeMemberFunction = Status (VeriBlock::DeserializeService::Stub::*)(::grpc::ClientContext* context, const ::VeriBlock::BytesArrayRequest& request, T* response);

template <class T>
bool grpc_deserialize(const std::shared_ptr<VeriBlock::DeserializeService::Stub>& deserializeService, const std::vector<unsigned char>& bytes, T& reply, DeserializeMemberFunction<T> func)
{
    VeriBlock::BytesArrayRequest request;
    ClientContext context;

    request.set_data(bytes.data(), bytes.size());

    return (*deserializeService.*func)(&context, request, &reply).ok();
}


void BlockToProtoAltChainBlock(const CBlockIndex* blockIndex, VeriBlock::AltChainBlock* protoBlock)
{
    auto* blockIndex1 = new VeriBlock::BlockIndex();
    blockIndex1->set_hash(blockIndex->GetBlockHash().ToString());
    blockIndex1->set_height(blockIndex->nHeight);
    protoBlock->set_allocated_blockindex(blockIndex1);
    protoBlock->set_timestamp(blockIndex->nTime);
}

void BlockToProtoAltChainBlock(const CBlockHeader& block, const int& nHeight, VeriBlock::AltChainBlock* protoBlock)
{
    auto* blockIndex1 = new VeriBlock::BlockIndex();
    blockIndex1->set_hash(block.GetHash().ToString());
    blockIndex1->set_height(nHeight);
    protoBlock->set_allocated_blockindex(blockIndex1);
    protoBlock->set_timestamp(block.nTime);
}
} // namespace


namespace VeriBlock {


PopServiceImpl::PopServiceImpl()
{
    auto& config = VeriBlock::getService<VeriBlock::Config>();
    std::string ip = config.service_ip;
    std::string port = config.service_port;

    printf("Connecting to alt-service at %s:%s...", ip.c_str(), port.c_str());
    auto creds = grpc::InsecureChannelCredentials();
    std::shared_ptr<Channel> channel = grpc::CreateChannel(ip + ":" + port, creds);

    integrationService = VeriBlock::IntegrationService::NewStub(channel);
    rewardsService = VeriBlock::RewardsService::NewStub(channel);
    deserializeService = VeriBlock::DeserializeService::NewStub(channel);
    forkresolutionService = VeriBlock::ForkresolutionService::NewStub(channel);
}

bool PopServiceImpl::addPayloads(const CBlock& block, const int& nHeight, const std::vector<VeriBlockPublication>& VTBs, const VeriBlock::AltPublication& ATV)
{
    AddPayloadsRequest request;
    GeneralReply reply;
    ClientContext context;

    auto* blockInfo = new BlockIndex();
    blockInfo->set_height(nHeight);
    blockInfo->set_hash(block.GetHash().ToString());

    request.set_allocated_blockindex(blockInfo);

    for (const auto& it : VTBs) {
        VeriBlockPublication* publication = request.add_veriblockpublications();
        *publication = it;
    }
    AltPublication* publication = request.add_altpublications();
    *publication = ATV;

    Status status = integrationService->AddPayloads(&context, request, &reply);
    if (status.ok()) {
        return reply.result();
    } else {
        throw VeriBlock::PopServiceException("grpc is shutdown");
    }


    return false;
}

void PopServiceImpl::removePayloads(const CBlock& block, const int& nHeight)
{
    GeneralReply reply;
    ClientContext context;
    RemovePayloadsRequest request;

    auto* blockInfo = new BlockIndex();
    blockInfo->set_height(nHeight);
    blockInfo->set_hash(block.GetHash().ToString());

    request.set_allocated_blockindex(blockInfo);

    Status status = integrationService->RemovePayloads(&context, request, &reply);
    if (status.ok()) {
        if (!reply.result()) {
            std::string error_message("GRPC returned error : %s", reply.resultmessage().c_str());
            throw VeriBlock::PopServiceException(error_message.c_str());
        }
    } else {
        throw VeriBlock::PopServiceException("grpc is shutdown");
    }
}

void PopServiceImpl::savePopTxToDatabase(const CBlock& block, const int& nHeight)
{
    for (const auto& tx : block.vtx) {
        if (!isPopTx(*tx)) {
            continue;
        }

        // Parse Pop data from TX
        auto* ATV = new AltPublication();
        std::vector<VeriBlockPublication> VTBs;

        this->getPublicationsData(tx, *ATV, VTBs);

        // Fill the proto objects with pop data
        auto* popData = new PoPTransactionData();
        popData->set_hash(tx->GetHash().ToString());
        popData->set_allocated_altpublication(ATV);

        VeriBlock::PublicationData publicationData = ATV->transaction().publicationdata();

        CDataStream stream(std::vector<unsigned char>(publicationData.header().begin(), publicationData.header().end()), SER_NETWORK, PROTOCOL_VERSION);
        CBlockHeader endorsedBlock;
        stream >> endorsedBlock;

        for (const auto& VTB : VTBs) {
            VeriBlockPublication* pub = popData->add_veriblockpublications();
            pub->CopyFrom(VTB);
        }

        auto* b1 = new AltChainBlock();
        BlockToProtoAltChainBlock(block, nHeight, b1);

        auto* b2 = new AltChainBlock();
        BlockToProtoAltChainBlock(block, nHeight, b2);

        SavePoPTransactionDataRequest request;
        request.set_allocated_poptx(popData);
        request.set_allocated_containingblock(b1);
        request.set_allocated_endorsedblock(b2);
        GeneralReply reply;
        ClientContext context;

        Status status = integrationService->SavePoPTransactionData(&context, request, &reply);

        if (status.ok()) {
            if (!reply.result()) {
                std::string error_message("GRPC returned error : %s", reply.resultmessage().c_str());
                throw VeriBlock::PopServiceException(error_message.c_str());
            }
        } else {
            throw VeriBlock::PopServiceException("grpc is shutdown");
        }
    }
}


std::vector<BlockBytes> PopServiceImpl::getLastKnownVBKBlocks(size_t blocks)
{
    GetLastKnownBlocksRequest request;
    request.set_maxblockcount(blocks);
    GetLastKnownVBKBlocksReply reply;
    ClientContext context;

    Status status = integrationService->GetLastKnownVBKBlocks(&context, request, &reply);

    std::vector<BlockBytes> result;

    if (status.ok()) {
        result.resize(reply.blocks_size());
        for (size_t i = 0, size = reply.blocks_size(); i < size ; i++) {
            auto& block = reply.blocks(i);
            result[i] = std::vector<uint8_t>{block.begin(), block.end()};
        }
    } else {
        throw PopServiceException(status.error_message().c_str());
    }

    return result;
}

std::vector<BlockBytes> PopServiceImpl::getLastKnownBTCBlocks(size_t blocks)
{
    GetLastKnownBlocksRequest request;
    request.set_maxblockcount(blocks);
    GetLastKnownBTCBlocksReply reply;
    ClientContext context;

    Status status = integrationService->GetLastKnownBTCBlocks(&context, request, &reply);

    std::vector<BlockBytes> result;

    if (status.ok()) {
        result.resize(reply.blocks_size());
        for (size_t i = 0, size = reply.blocks_size(); i < size ; i++) {
            auto& block = reply.blocks(i);
            result[i] = std::vector<uint8_t>{block.begin(), block.end()};
        }
    } else {
        throw PopServiceException(status.error_message().c_str());
    }

    return result;
}

bool PopServiceImpl::checkVTBinternally(const std::vector<uint8_t>& bytes)
{
    VeriBlockPublication publication;
    this->parseVeriBlockPublication(bytes, publication);

    GeneralReply reply;
    ClientContext context;

    Status status = integrationService->CheckVTBInternally(&context, publication, &reply);

    if (status.ok()) {
        return reply.result();
    } else {
        throw PopServiceException(status.error_message().c_str());
    }
}

bool PopServiceImpl::checkATVinternally(const std::vector<uint8_t>& bytes)
{
    AltPublication publication;
    this->parseAltPublication(bytes, publication);

    GeneralReply reply;
    ClientContext context;

    Status status = integrationService->CheckATVInternally(&context, publication, &reply);

    if (status.ok()) {
        return reply.result();
    } else {
        throw PopServiceException("grpc is shutdown");
    }
}

// Forkresolution
int PopServiceImpl::compareTwoBranches(const CBlockIndex* commonKeystone, const CBlockIndex* leftForkTip, const CBlockIndex* rightForkTip)
{
    TwoBranchesRequest request;
    CompareReply reply;
    ClientContext context;

    const CBlockIndex* workingLeft = leftForkTip;
    const CBlockIndex* workingRight = rightForkTip;

    while (true) {
        AltChainBlock* b = request.add_leftfork();
        ::BlockToProtoAltChainBlock(workingLeft, b);

        if (workingLeft == commonKeystone)
            break;
        workingLeft = workingLeft->pprev;
    }

    while (true) {
        AltChainBlock* b = request.add_rightfork();
        ::BlockToProtoAltChainBlock(workingRight, b);

        if (workingRight == commonKeystone)
            break;
        workingRight = workingRight->pprev;
    }

    Status status = forkresolutionService->CompareTwoBranches(&context, request, &reply);

    if (status.ok()) {
        if (reply.result().result()) {
            return reply.comparingsresult();
        } else {
            std::string error_message("GRPC returned error : %s", reply.result().resultmessage().c_str());
            throw PopServiceException(error_message.c_str());
        }
    } else {
        throw PopServiceException("grpc is shutdown");
    }
    return 0;
}

// Pop rewards
void PopServiceImpl::rewardsCalculateOutputs(const int& blockHeight, const CBlockIndex* endorsedBlock, const CBlockIndex* contaningBlocksTip, const std::string& difficulty, std::map<CScript, int64_t>& outputs)
{
    RewardsCalculateOutputsRequest request;
    RewardsCalculateOutputsReply reply;
    ClientContext context;

    const CBlockIndex* workingBlock = contaningBlocksTip;

    while (workingBlock != endorsedBlock) {
        AltChainBlock* b = request.add_endorsmentblocks();
        ::BlockToProtoAltChainBlock(workingBlock, b);

        workingBlock = workingBlock->pprev;
    }

    auto* b = new AltChainBlock();
    ::BlockToProtoAltChainBlock(endorsedBlock, b);

    request.set_allocated_endorsedblock(b);
    request.set_blockaltheight(blockHeight);
    request.set_difficulty(difficulty);

    Status status = rewardsService->RewardsCalculateOutputs(&context, request, &reply);

    if (status.ok()) {
        if (reply.result().result()) {
            for (int i = 0; reply.outputs_size(); ++i) {
                CScript script;
                const VeriBlock::RewardOutput& output = reply.outputs(i);
                std::vector<unsigned char> payout_bytes(output.payoutinfo().begin(), output.payoutinfo().end());
                script << payout_bytes;

                try {
                    int64_t amount = std::stoll(output.reward());
                    if (MoneyRange(amount)) {
                        outputs[script] = amount;
                    } else {
                        outputs[script] = MAX_MONEY;
                    }
                } catch (const std::invalid_argument&) {
                    std::string error_message("GRPC returned error : %s", reply.result().resultmessage().c_str());
                    throw VeriBlock::PopServiceException(error_message.c_str());
                } catch (const std::out_of_range&) {
                    outputs[script] = MAX_MONEY;
                }
            }

        } else {
            std::string error_message("GRPC returned error : %s", reply.result().resultmessage().c_str());
            throw PopServiceException(error_message.c_str());
        }
    } else {
        throw PopServiceException("grpc is shutdown");
    }
}

std::string PopServiceImpl::rewardsCalculatePopDifficulty(const CBlockIndex* start_interval, const CBlockIndex* end_interval)
{
    RewardsCalculatePopDifficultyRequest request;
    RewardsCalculateScoreReply reply;
    ClientContext context;

    const CBlockIndex* workingBlock = end_interval;

    while (workingBlock != start_interval->pprev) // including the start_interval block
    {
        AltChainBlock* b = request.add_blocks();
        ::BlockToProtoAltChainBlock(workingBlock, b);

        workingBlock = workingBlock->pprev;
    }

    Status status = rewardsService->RewardsCalculatePopDifficulty(&context, request, &reply);

    if (status.ok()) {
        if (reply.result().result()) {
            return reply.score();
        } else {
            std::string error_message("GRPC returned error : %s", reply.result().resultmessage().c_str());
            throw PopServiceException(error_message.c_str());
        }
    } else {
        throw PopServiceException("grpc is shutdown");
    }

    return "0";
}

// Deserialization
void PopServiceImpl::parseAltPublication(const std::vector<unsigned char>& bytes, VeriBlock::AltPublication& publication)
{
    AltPublicationReply reply;
    bool result = ::grpc_deserialize(deserializeService, bytes, reply, &DeserializeService::Stub::ParseAltPublication);
    ::handleResult(result, reply.result(), reply.publication(), publication);
}

void PopServiceImpl::parseVeriBlockPublication(const std::vector<unsigned char>& bytes, VeriBlock::VeriBlockPublication& publication)
{
    VeriBlockPublicationReply reply;
    bool result = ::grpc_deserialize(deserializeService, bytes, reply, &DeserializeService::Stub::ParseVeriBlockPublication);
    ::handleResult(result, reply.result(), reply.publication(), publication);
}

void PopServiceImpl::getPublicationsData(const CTransactionRef& tx, AltPublication& ATV, std::vector<VeriBlockPublication>& VTBs)
{
    Publications publicationsBytes;
    std::vector<std::vector<uint8_t>> stack;
    getService<UtilService>().EvalScript(tx->vin[0].scriptSig, stack, nullptr, &publicationsBytes, false);

    this->parseAltPublication(publicationsBytes.atv, ATV);
    for (const auto& vtbBytes : publicationsBytes.vtbs) {
        VeriBlockPublication vtb;
        this->parseVeriBlockPublication(vtbBytes, vtb);
        VTBs.push_back(vtb);
    }
}

bool PopServiceImpl::determineATVPlausibilityWithBTCRules(AltchainId altChainIdentifier, const CBlockHeader& popEndorsementHeader, const Consensus::Params& params)
{
    // Some additional checks could be done here to ensure that the context info container
    // is more apparently initially valid (such as checking both included block hashes
    // against the minimum PoW difficulty on the network).
    // However, the ATV will fail to validate upon attempted inclusion into a block anyway
    // if the context info container contains a bad block height, or nonexistent previous keystones

    if (altChainIdentifier.unwrap() != getService<Config>().index.unwrap()) {
        return false;
    }


    return CheckProofOfWork(popEndorsementHeader.GetHash(), popEndorsementHeader.nBits, params);
}

bool blockPopValidationImpl(PopServiceImpl& pop, const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, CValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    bool isValid = true;
    const auto& config = getService<Config>();

    //    LOCK2(mempool.cs, cs_main); // TODO(veriblock): check if this is correct place for a lock

    LOCK(mempool.cs);
    AssertLockHeld(mempool.cs);
    AssertLockHeld(cs_main);
    for (const auto& tx : block.vtx) {
        if (!isPopTx(*tx)) {
            continue;
        }

        AltPublication ATV;
        std::vector<VeriBlockPublication> VTBs;
        pop.getPublicationsData(tx, ATV, VTBs);

        PublicationData popEndorsement = ATV.transaction().publicationdata();

        CDataStream stream(std::vector<unsigned char>(popEndorsement.header().begin(), popEndorsement.header().end()), SER_NETWORK, PROTOCOL_VERSION);
        CBlockHeader popEndorsementHeader;

        try {
            stream >> popEndorsementHeader;
        } catch (const std::exception&) {
            isValid = false;
            mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
            continue;
        }

        if (!pop.determineATVPlausibilityWithBTCRules(AltchainId(popEndorsement.identifier()), popEndorsementHeader, params)) {
            isValid = false;
            mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
            continue;
        }

        const CBlockIndex* popEndorsementIdnex = LookupBlockIndex(popEndorsementHeader.GetHash());
        if (popEndorsementIdnex == nullptr || pindexPrev.GetAncestor(popEndorsementIdnex->nHeight) == nullptr) {
            isValid = false;
            mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
            continue;
        }

        CBlock popEndorsementBlock;
        if (!ReadBlockFromDisk(popEndorsementBlock, popEndorsementIdnex, params)) {
            isValid = false;
            mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
            continue;
        }

        if (!VeriBlock::VerifyTopLevelMerkleRoot(popEndorsementBlock, state, popEndorsementIdnex->pprev)) {
            isValid = false;
            mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
            continue;
        }

        if (pindexPrev.nHeight + 1 - popEndorsementIdnex->nHeight > config.POP_REWARD_SETTLEMENT_INTERVAL) {
            isValid = false;
            mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
            continue;
        }

        if (!pop.addPayloads(block, pindexPrev.nHeight + 1, VTBs, ATV)) {
            isValid = false;
            mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
            continue;
        }
    }

    if (!isValid) {
        pop.removePayloads(block, pindexPrev.nHeight + 1);
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, strprintf("blockPopValidation(): pop check is failed"), "bad pop data");
    }

    return true;
}

bool PopServiceImpl::blockPopValidation(const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, CValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    return blockPopValidationImpl(*this, block, pindexPrev, params, state);
}
} // namespace VeriBlock
