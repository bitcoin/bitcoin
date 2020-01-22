#include "pop_service_impl.hpp"

#include <chrono>
#include <memory>
#include <thread>

#include <amount.h>
#include <chain.h>
#include <consensus/validation.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/sigcache.h>
#include <shutdown.h>
#include <streams.h>
#include <util/strencodings.h>
#include <validation.h>

#include <vbk/merkle.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/util.hpp>
#include <vbk/util_service.hpp>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

namespace {

void BlockToProtoAltChainBlock(const CBlockIndex& blockIndex, VeriBlock::AltChainBlock& protoBlock)
{
    auto* blockIndex1 = new VeriBlock::BlockIndex();
    blockIndex1->set_hash(blockIndex.GetBlockHash().ToString());
    blockIndex1->set_height(blockIndex.nHeight);
    protoBlock.set_allocated_blockindex(blockIndex1);
    protoBlock.set_timestamp(blockIndex.nTime);
}

void BlockToProtoAltChainBlock(const CBlockHeader& block, const int& nHeight, VeriBlock::AltChainBlock& protoBlock)
{
    auto* blockIndex1 = new VeriBlock::BlockIndex();
    blockIndex1->set_hash(block.GetHash().ToString());
    blockIndex1->set_height(nHeight);
    protoBlock.set_allocated_blockindex(blockIndex1);
    protoBlock.set_timestamp(block.nTime);
}
} // namespace


namespace VeriBlock {

PopServiceImpl::PopServiceImpl(bool altautoconfig)
{
    auto& config = VeriBlock::getService<VeriBlock::Config>();
    std::string ip = config.service_ip;
    std::string port = config.service_port;

    LogPrintf("Connecting to alt-service at %s:%s... \n", ip.c_str(), port.c_str());
    auto creds = grpc::InsecureChannelCredentials();
    std::shared_ptr<Channel> channel = grpc::CreateChannel(ip + ":" + port, creds);

    grpcPopService = VeriBlock::GrpcPopService::NewStub(channel);

    std::chrono::system_clock::time_point deadline =
        std::chrono::system_clock::now() + std::chrono::seconds(5);
    gpr_timespec time;
    grpc::Timepoint2Timespec(deadline, &time);

    if (!channel->WaitForConnected(time)) {
        LogPrintf("Alt-service is not working, please run the alt-service before you start the daemon \n");
        LogPrintf("-------------------------------------------------------------------------------------------------\n");
        StartShutdown();
    } else if (altautoconfig) {
        setConfig();
    }
}

void PopServiceImpl::addPayloads(const CBlock& block, const int& nHeight, const Publications& publications)
{
    AddPayloadsDataRequest request;
    EmptyReply reply;
    ClientContext context;

    auto* blockInfo = new BlockIndex();
    blockInfo->set_height(nHeight);
    blockInfo->set_hash(block.GetHash().ToString());

    request.set_allocated_blockindex(blockInfo);

    for (const auto& vtb : publications.vtbs) {
        std::string* pub = request.add_veriblockpublications();
        *pub = std::string(vtb.begin(), vtb.end());
    }
    std::string* pub = request.add_altpublications();
    *pub = std::string(publications.atv.begin(), publications.atv.end());

    Status status = grpcPopService->AddPayloads(&context, request, &reply);
    if (!status.ok()) {
        throw PopServiceException(status);
    }
}

void PopServiceImpl::removePayloads(const CBlock& block, const int& nHeight)
{
    EmptyReply reply;
    ClientContext context;
    RemovePayloadsRequest request;

    auto* blockInfo = new BlockIndex();
    blockInfo->set_height(nHeight);
    blockInfo->set_hash(block.GetHash().ToString());

    request.set_allocated_blockindex(blockInfo);

    Status status = grpcPopService->RemovePayloads(&context, request, &reply);
    if (!status.ok()) {
        throw PopServiceException(status);
    }
}

void PopServiceImpl::savePopTxToDatabase(const CBlock& block, const int& nHeight)
{
    SaveBlockPopTxRequest request;
    EmptyReply reply;
    ClientContext context;

    auto* b1 = new AltChainBlock();
    BlockToProtoAltChainBlock(block, nHeight, *b1);
    request.set_allocated_containingblock(b1);

    for (const auto& tx : block.vtx) {
        if (!isPopTx(*tx)) {
            continue;
        }

        PopTxData* popTxData = request.add_popdata();


        Publications publications;
        PopTxType type;
        assert(parsePopTx(tx, &publications, nullptr, &type) && "scriptSig of pop tx is invalid in savePopTxToDatabase");

        // skip all non-publications txes
        if (type != PopTxType::PUBLICATIONS) {
            continue;
        }

        // Fill the proto objects with pop data
        popTxData->set_poptxhash(tx->GetHash().ToString());
        popTxData->set_altpublication(publications.atv.data(), publications.atv.size());

        PublicationData publicationData;
        getPublicationsData(publications, publicationData);

        CDataStream stream(std::vector<unsigned char>(publicationData.header().begin(), publicationData.header().end()), SER_NETWORK, PROTOCOL_VERSION);
        CBlockHeader endorsedBlock;
        stream >> endorsedBlock;

        CBlockIndex* endorsedBlockIndex;
        {
            LOCK(cs_main);
            endorsedBlockIndex = LookupBlockIndex(endorsedBlock.GetHash());
        }

        auto* b2 = new AltChainBlock();
        BlockToProtoAltChainBlock(endorsedBlock, endorsedBlockIndex->nHeight, *b2);
        popTxData->set_allocated_endorsedblock(b2);

        for (const auto& vtb : publications.vtbs) {
            std::string* data = popTxData->add_veriblockpublications();
            *data = std::string(vtb.begin(), vtb.end());
        }
    }

    if (request.popdata_size() != 0) {
        Status status = grpcPopService->SaveBlockPopTxToDatabase(&context, request, &reply);
        if (!status.ok()) {
            throw PopServiceException(status);
        }
    }
}

std::vector<BlockBytes> PopServiceImpl::getLastKnownVBKBlocks(size_t blocks)
{
    GetLastKnownBlocksRequest request;
    request.set_maxblockcount(blocks);
    GetLastKnownBlocksReply reply;
    ClientContext context;

    Status status = grpcPopService->GetLastKnownVBKBlocks(&context, request, &reply);
    if (!status.ok()) {
        throw PopServiceException(status);
    }

    std::vector<BlockBytes> result;

    result.resize(reply.blocks_size());
    for (size_t i = 0, size = reply.blocks_size(); i < size; i++) {
        auto& block = reply.blocks(i);
        result[i] = std::vector<uint8_t>{block.begin(), block.end()};
    }

    return result;
}

std::vector<BlockBytes> PopServiceImpl::getLastKnownBTCBlocks(size_t blocks)
{
    GetLastKnownBlocksRequest request;
    request.set_maxblockcount(blocks);
    GetLastKnownBlocksReply reply;
    ClientContext context;

    Status status = grpcPopService->GetLastKnownBTCBlocks(&context, request, &reply);
    if (!status.ok()) {
        throw PopServiceException(status);
    }

    std::vector<BlockBytes> result;

    result.resize(reply.blocks_size());
    for (size_t i = 0, size = reply.blocks_size(); i < size; i++) {
        auto& block = reply.blocks(i);
        result[i] = std::vector<uint8_t>{block.begin(), block.end()};
    }
    return result;
}

bool PopServiceImpl::checkVTBinternally(const std::vector<uint8_t>& bytes)
{
    VeriBlock::BytesArrayRequest request;
    request.set_data(bytes.data(), bytes.size());

    CheckReply reply;
    ClientContext context;

    Status status = grpcPopService->CheckVTBInternally(&context, request, &reply);
    if (!status.ok()) {
        throw PopServiceException(status);
    }

    return reply.result();
}

bool PopServiceImpl::checkATVinternally(const std::vector<uint8_t>& bytes)
{
    VeriBlock::BytesArrayRequest request;
    request.set_data(bytes.data(), bytes.size());

    CheckReply reply;
    ClientContext context;

    Status status = grpcPopService->CheckATVInternally(&context, request, &reply);
    if (!status.ok()) {
        throw PopServiceException(status);
    }

    return reply.result();
}

void PopServiceImpl::updateContext(const std::vector<std::vector<uint8_t>>& veriBlockBlocks, const std::vector<std::vector<uint8_t>>& bitcoinBlocks)
{
    UpdateContextRequest request;
    EmptyReply reply;
    ClientContext context;

    for (const auto& bitcoin_block : bitcoinBlocks) {
        std::string* pb = request.add_bitcoinblocks();
        *pb = std::string(bitcoin_block.begin(), bitcoin_block.end());
    }

    for (const auto& veriblock_block : veriBlockBlocks) {
        std::string* pb = request.add_veriblockblocks();
        *pb = std::string(veriblock_block.begin(), veriblock_block.end());
    }

    Status status = grpcPopService->UpdateContext(&context, request, &reply);
    if (!status.ok()) {
        throw PopServiceException(status);
    }
}

// Forkresolution
int PopServiceImpl::compareTwoBranches(const CBlockIndex* commonKeystone, const CBlockIndex* leftForkTip, const CBlockIndex* rightForkTip)
{
    TwoBranchesRequest request;
    CompareTwoBranchesReply reply;
    ClientContext context;

    const CBlockIndex* workingLeft = leftForkTip;
    const CBlockIndex* workingRight = rightForkTip;

    while (true) {
        AltChainBlock* b = request.add_leftfork();
        ::BlockToProtoAltChainBlock(*workingLeft, *b);

        if (workingLeft == commonKeystone)
            break;
        workingLeft = workingLeft->pprev;
    }

    while (true) {
        AltChainBlock* b = request.add_rightfork();
        ::BlockToProtoAltChainBlock(*workingRight, *b);

        if (workingRight == commonKeystone)
            break;
        workingRight = workingRight->pprev;
    }

    Status status = grpcPopService->CompareTwoBranches(&context, request, &reply);
    if (!status.ok()) {
        throw PopServiceException(status);
    }

    return reply.compareresult();
}

// Pop rewards
void PopServiceImpl::rewardsCalculateOutputs(const int& blockHeight, const CBlockIndex& endorsedBlock, const CBlockIndex& contaningBlocksTip, const CBlockIndex* difficulty_start_interval, const CBlockIndex* difficulty_end_interval, std::map<CScript, int64_t>& outputs)
{
    RewardsCalculateRequest request;
    RewardsCalculateReply reply;
    ClientContext context;

    const CBlockIndex* workingBlock = &contaningBlocksTip;

    while (workingBlock != &endorsedBlock) {
        AltChainBlock* b = request.add_endorsmentblocks();
        ::BlockToProtoAltChainBlock(*workingBlock, *b);
        workingBlock = workingBlock->pprev;
    }

    workingBlock = difficulty_end_interval;

    CBlockIndex* pprev_difficulty_start_interval = difficulty_start_interval != nullptr ? difficulty_start_interval->pprev : nullptr;

    while (workingBlock != pprev_difficulty_start_interval) // including the start_interval block
    {
        AltChainBlock* b = request.add_difficultyblocks();
        ::BlockToProtoAltChainBlock(*workingBlock, *b);
        workingBlock = workingBlock->pprev;
    }

    auto* b = new AltChainBlock();
    ::BlockToProtoAltChainBlock(endorsedBlock, *b);

    request.set_allocated_endorsedblock(b);
    request.set_blockaltheight(blockHeight);

    Status status = grpcPopService->RewardsCalculateOutputs(&context, request, &reply);
    if (!status.ok()) {
        throw PopServiceException(status);
    }

    for (int i = 0, size = reply.outputs_size(); i < size; ++i) {
        const VeriBlock::RewardOutput& output = reply.outputs(i);
        std::vector<unsigned char> payout_bytes(output.payoutinfo().begin(), output.payoutinfo().end());
        CScript script(payout_bytes.begin(), payout_bytes.end());

        try {
            int64_t amount = std::stoll(output.reward());
            if (MoneyRange(amount)) {
                outputs[script] = amount;
            } else {
                outputs[script] = MAX_MONEY;
            }
        } catch (const std::invalid_argument&) {
            throw VeriBlock::PopServiceException("cannot convert the value received from the service");
        } catch (const std::out_of_range&) {
            outputs[script] = MAX_MONEY;
        }
    }
}

bool PopServiceImpl::parsePopTx(const CTransactionRef& tx, Publications* pub, Context* ctx, PopTxType* type)
{
    std::vector<std::vector<uint8_t>> stack;
    return getService<UtilService>().EvalScript(tx->vin[0].scriptSig, stack, nullptr, pub, ctx, type, false);
}

void PopServiceImpl::getPublicationsData(const Publications& data, PublicationData& pub)
{
    ClientContext context;
    BytesArrayRequest request;

    request.set_data(data.atv.data(), data.atv.size());

    Status status = grpcPopService->GetPublicationDataFromAltPublication(&context, request, &pub);

    if (!status.ok()) {
        throw PopServiceException(status);
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

void PopServiceImpl::setConfig()
{
    auto& config = getService<Config>();

    ClientContext context;
    SetConfigRequest request;
    EmptyReply reply;

    //AltChainConfig
    AltChainConfigRequest* altChainConfig = new AltChainConfigRequest();
    altChainConfig->set_keystoneinterval(config.keystone_interval);

    // CalculatorConfig
    CalculatorConfig* calculatorConfig = new CalculatorConfig();
    calculatorConfig->set_basicreward(std::to_string(COIN));
    calculatorConfig->set_payoutrounds(config.payoutRounds);
    calculatorConfig->set_keystoneround(config.keystoneRound);

    RoundRatioConfig* roundRatioConfig = new RoundRatioConfig();
    for (size_t i = 0; i < config.roundRatios.size(); ++i) {
        std::string* round = roundRatioConfig->add_roundratio();
        *round = config.roundRatios[i];
    }
    calculatorConfig->set_allocated_roundratios(roundRatioConfig);

    RewardCurveConfig* rewardCurveConfig = new RewardCurveConfig();
    rewardCurveConfig->set_startofdecreasingline(config.startOfDecreasingLine);
    rewardCurveConfig->set_widthofdecreasinglinenormal(config.widthOfDecreasingLineNormal);
    rewardCurveConfig->set_widthofdecreasinglinekeystone(config.widthOfDecreasingLineKeystone);
    rewardCurveConfig->set_aboveintendedpayoutmultipliernormal(config.aboveIntendedPayoutMultiplierNormal);
    rewardCurveConfig->set_aboveintendedpayoutmultiplierkeystone(config.aboveIntendedPayoutMultiplierKeystone);
    calculatorConfig->set_allocated_rewardcurve(rewardCurveConfig);

    calculatorConfig->set_maxrewardthresholdnormal(config.maxRewardThresholdNormal);
    calculatorConfig->set_maxrewardthresholdkeystone(config.maxRewardThresholdKeystone);

    RelativeScoreConfig* relativeScoreConfig = new RelativeScoreConfig();
    for (size_t i = 0; i < config.relativeScoreLookupTable.size(); ++i) {
        std::string* score = relativeScoreConfig->add_score();
        *score = config.relativeScoreLookupTable[i];
    }
    calculatorConfig->set_allocated_relativescorelookuptable(relativeScoreConfig);

    FlatScoreRoundConfig* flatScoreRoundConfig = new FlatScoreRoundConfig();
    flatScoreRoundConfig->set_round(config.flatScoreRound);
    flatScoreRoundConfig->set_active(config.flatScoreRoundUse);
    calculatorConfig->set_allocated_flatscoreround(flatScoreRoundConfig);

    calculatorConfig->set_popdifficultyaveraginginterval(config.POP_DIFFICULTY_AVERAGING_INTERVAL);
    calculatorConfig->set_poprewardsettlementinterval(config.POP_REWARD_SETTLEMENT_INTERVAL);

    //ForkresolutionConfig
    ForkresolutionConfigRequest* forkresolutionConfig = new ForkresolutionConfigRequest();
    forkresolutionConfig->set_keystonefinalitydelay(config.keystone_finality_delay);
    forkresolutionConfig->set_amnestyperiod(config.amnesty_period);

    //VeriBlockBootstrapConfig
    VeriBlockBootstrapConfig* veriBlockBootstrapBlocks = new VeriBlockBootstrapConfig();
    for (size_t i = 0; i < config.bootstrap_veriblock_blocks.size(); ++i) {
        std::string* block = veriBlockBootstrapBlocks->add_blocks();
        std::vector<uint8_t> bytes = ParseHex(config.bootstrap_veriblock_blocks[i]);
        *block = std::string(bytes.begin(), bytes.end());
    }

    //BitcoinBootstrapConfig
    BitcoinBootstrapConfig* bitcoinBootstrapBlocks = new BitcoinBootstrapConfig();
    for (size_t i = 0; i < config.bootstrap_bitcoin_blocks.size(); ++i) {
        std::string* block = bitcoinBootstrapBlocks->add_blocks();
        std::vector<uint8_t> bytes = ParseHex(config.bootstrap_bitcoin_blocks[i]);
        *block = std::string(bytes.begin(), bytes.end());
    }
    bitcoinBootstrapBlocks->set_firstblockheight(config.bitcoin_first_block_height);

    request.set_allocated_altchainconfig(altChainConfig);
    request.set_allocated_calculatorconfig(calculatorConfig);
    request.set_allocated_forkresolutionconfig(forkresolutionConfig);
    request.set_allocated_veriblockbootstrapconfig(veriBlockBootstrapBlocks);
    request.set_allocated_bitcoinbootstrapconfig(bitcoinBootstrapBlocks);

    Status status = grpcPopService->SetConfig(&context, request, &reply);

    if (!status.ok()) {
        throw PopServiceException(status);
    }
}

bool blockPopValidationImpl(PopServiceImpl& pop, const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    bool isValid = true;
    const auto& config = getService<Config>();
    size_t numOfPopTxes = 0;
    std::string error_message;

    LOCK(mempool.cs);
    AssertLockHeld(mempool.cs);
    AssertLockHeld(cs_main);
    for (const auto& tx : block.vtx) {
        if (!isPopTx(*tx)) {
            continue;
        }

        ++numOfPopTxes;
        Publications publications;
        Context context;
        PopTxType type = PopTxType::UNKNOWN;
        if (!pop.parsePopTx(tx, &publications, &context, &type)) {
            error_message += " tx hash: (" + tx->GetHash().GetHex() + ") reason : can not parse pop tx data \n";
            isValid = false;
            mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
            continue;
        }

        switch (type) {
        case PopTxType::CONTEXT: {
            try {
                pop.updateContext(context.vbk, context.btc);
            } catch (const PopServiceException& e) {
                error_message += " tx hash: (" + tx->GetHash().GetHex() + ") reason : updateContext failed, " + e.what() + "\n";
                isValid = false;
                mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
                continue;
            }
            break;
        }

        case PopTxType::PUBLICATIONS: {
            PublicationData popEndorsement;
            pop.getPublicationsData(publications, popEndorsement);

            CDataStream stream(std::vector<unsigned char>(popEndorsement.header().begin(), popEndorsement.header().end()), SER_NETWORK, PROTOCOL_VERSION);
            CBlockHeader popEndorsementHeader;

            try {
                stream >> popEndorsementHeader;
            } catch (const std::exception&) {
                error_message += " tx hash: (" + tx->GetHash().GetHex() + ") reason : invalid endorsed block header \n";
                isValid = false;
                mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
                continue;
            }

            if (!pop.determineATVPlausibilityWithBTCRules(AltchainId(popEndorsement.identifier()), popEndorsementHeader, params)) {
                error_message += " tx hash: (" + tx->GetHash().GetHex() + ") reason : invalid alt-chain index or bad PoW of endorsed block header \n";
                isValid = false;
                mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
                continue;
            }

            const CBlockIndex* popEndorsementIdnex = LookupBlockIndex(popEndorsementHeader.GetHash());
            if (popEndorsementIdnex == nullptr || pindexPrev.GetAncestor(popEndorsementIdnex->nHeight) == nullptr) {
                error_message += " tx hash: (" + tx->GetHash().GetHex() + ") reason : endorsed block not from this chain \n";
                isValid = false;
                mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
                continue;
            }

            CBlock popEndorsementBlock;
            if (!ReadBlockFromDisk(popEndorsementBlock, popEndorsementIdnex, params)) {
                error_message += " tx hash: (" + tx->GetHash().GetHex() + ") reason : cant read endorsed block from disk \n";
                isValid = false;
                mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
                continue;
            }

            if (!VeriBlock::VerifyTopLevelMerkleRoot(popEndorsementBlock, state, popEndorsementIdnex->pprev)) {
                error_message += " tx hash: (" + tx->GetHash().GetHex() + ") reason : invalid top merkle root of the endorsed block \n";
                isValid = false;
                mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
                continue;
            }

            if (pindexPrev.nHeight + 1 - popEndorsementIdnex->nHeight > config.POP_REWARD_SETTLEMENT_INTERVAL) {
                error_message += " tx hash: (" + tx->GetHash().GetHex() + ") reason : endorsed block is too old for this chain \n";
                isValid = false;
                mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
                continue;
            }

            try {
                pop.addPayloads(block, pindexPrev.nHeight + 1, publications);
            } catch (const PopServiceException& e) {
                error_message += " tx hash: (" + tx->GetHash().GetHex() + ") reason : addPayloads failed, " + e.what() + "\n";
                isValid = false;
                mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
                continue;
            }
            break;
        }
        default:
            error_message += " tx hash: (" + tx->GetHash().GetHex() + ") reason : pop tx type is unknown \n";
            isValid = false;
            mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
            continue;
        }
    }

    if (numOfPopTxes > config.max_pop_tx_amount) {
        error_message += strprintf("too many pop transactions: actual %d > %d expected \n", numOfPopTxes, config.max_pop_tx_amount);
        isValid = false;
    }

    if (!isValid) {
        pop.removePayloads(block, pindexPrev.nHeight + 1);
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, strprintf("blockPopValidation(): pop check is failed"), "bad pop data \n" + error_message);
    }

    return true;
}

bool PopServiceImpl::blockPopValidation(const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    return blockPopValidationImpl(*this, block, pindexPrev, params, state);
}
} // namespace VeriBlock
