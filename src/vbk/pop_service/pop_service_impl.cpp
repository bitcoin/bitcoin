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

inline uint32_t getReservedBlockIndexBegin(const VeriBlock::Config& config)
{
    return std::numeric_limits<uint32_t>::max() - config.max_pop_tx_amount;
}

inline uint32_t getReservedBlockIndexEnd(const VeriBlock::Config& config)
{
    return std::numeric_limits<uint32_t>::max();
}

inline std::string heightToHash(uint32_t height)
{
    return arith_uint256(height).GetHex();
}

void clearTemporaryPayloadsImpl(VeriBlock::PopService& pop, uint32_t begin, uint32_t end)
{
    for (uint32_t height = end - 1; height >= begin; --height) {
        try {
            pop.removePayloads(heightToHash(height), height);
        } catch (const std::exception& /*ignore*/) {
        }
    }
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

    clearTemporaryPayloads();
}

void PopServiceImpl::addPayloads(std::string blockHash, const int& nHeight, const Publications& publications)
{
    std::lock_guard<std::mutex> lock(mutex);
    AddPayloadsDataRequest request;
    EmptyReply reply;
    ClientContext context;

    auto* blockInfo = new BlockIndex();
    blockInfo->set_height(nHeight);
    blockInfo->set_hash(blockHash);

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

void PopServiceImpl::addPayloads(const CBlockIndex & blockIndex, const CBlock & block)
{
    std::lock_guard<std::mutex> lock(mutex);
    AddPayloadsDataRequest request;

    auto* index = new BlockIndex();
    index->set_height(blockIndex.nHeight);
    index->set_hash(blockIndex.GetBlockHash().ToString());
    request.set_allocated_blockindex(index);

    for (const auto & tx : block.vtx) {
        if (!isPopTx(*tx)) {
            continue;
        }

        Publications publications;
        PopTxType type;
        ScriptError serror;
        assert(parsePopTx(tx, &serror, &publications, nullptr, &type) && "scriptSig of pop tx is invalid in addPayloads");

        // skip non-publication transactions
        if (type != PopTxType::PUBLICATIONS) {
            continue;
        }

        // insert payloads
        request.add_altpublications(publications.atv.data(), publications.atv.size());
        for (const auto& vtb : publications.vtbs) {
            request.add_veriblockpublications(vtb.data(), vtb.size());
        }
    }

    EmptyReply reply;
    ClientContext context;
    Status status = grpcPopService->AddPayloads(&context, request, &reply);
    if (!status.ok()) {
        throw PopServiceException(status);
    }
}

void PopServiceImpl::removePayloads(const CBlockIndex & block)
{
    removePayloads(block.GetBlockHash().ToString(), block.nHeight);
}

void PopServiceImpl::removePayloads(std::string blockHash, const int& nHeight)
{
    std::lock_guard<std::mutex> lock(mutex);
    EmptyReply reply;
    ClientContext context;
    RemovePayloadsRequest request;

    auto* blockInfo = new BlockIndex();
    blockInfo->set_height(nHeight);
    blockInfo->set_hash(blockHash);

    request.set_allocated_blockindex(blockInfo);

    Status status = grpcPopService->RemovePayloads(&context, request, &reply);
    if (!status.ok()) {
        throw PopServiceException(status);
    }
}

void PopServiceImpl::savePopTxToDatabase(const CBlock& block, const int& nHeight)
{
    std::lock_guard<std::mutex> lock(mutex);
    SaveBlockPopTxRequest request;
    EmptyReply reply;

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
        ScriptError serror;
        assert(parsePopTx(tx, &serror, &publications, nullptr, &type) && "scriptSig of pop tx is invalid in savePopTxToDatabase");

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
            popTxData->add_veriblockpublications(vtb.data(), vtb.size());
        }
    }

    if (request.popdata_size() != 0) {
        // then, save pop txes to database
        {
            ClientContext context;
            Status status = grpcPopService->SaveBlockPopTxToDatabase(&context, request, &reply);
            if (!status.ok()) {
                throw PopServiceException(status);
            }
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
    std::lock_guard<std::mutex> lock(mutex);
    UpdateContextRequest request;
    EmptyReply reply;
    ClientContext context;

    for (const auto& bitcoin_block : bitcoinBlocks) {
        request.add_bitcoinblocks(bitcoin_block.data(), bitcoin_block.size());
    }

    for (const auto& veriblock_block : veriBlockBlocks) {
        request.add_veriblockblocks(veriblock_block.data(), veriblock_block.size());
    }

    Status status = grpcPopService->UpdateContext(&context, request, &reply);
    if (!status.ok()) {
        throw PopServiceException(status);
    }
}

// Forkresolution
int PopServiceImpl::compareTwoBranches(const CBlockIndex* commonKeystone, const CBlockIndex* leftForkTip, const CBlockIndex* rightForkTip)
{
    std::lock_guard<std::mutex> lock(mutex);
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
    std::lock_guard<std::mutex> lock(mutex);
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

bool PopServiceImpl::parsePopTx(const CTransactionRef& tx, ScriptError* serror, Publications* pub, Context* ctx, PopTxType* type)
{
    std::vector<std::vector<uint8_t>> stack;
    return getService<UtilService>().EvalScript(tx->vin[0].scriptSig, stack, serror, pub, ctx, type, false);
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

bool PopServiceImpl::determineATVPlausibilityWithBTCRules(AltchainId altChainIdentifier, const CBlockHeader& popEndorsementHeader, const Consensus::Params& params, TxValidationState& state)
{
    // Some additional checks could be done here to ensure that the context info container
    // is more apparently initially valid (such as checking both included block hashes
    // against the minimum PoW difficulty on the network).
    // However, the ATV will fail to validate upon attempted inclusion into a block anyway
    // if the context info container contains a bad block height, or nonexistent previous keystones
    auto expected = getService<Config>().index.unwrap();
    if (altChainIdentifier.unwrap() != expected) {
        return state.Invalid(TxValidationResult::TX_BAD_POP_DATA, "pop-tx-altchain-id", strprintf("wrong altchain ID. Expected %d, got %d.", expected, altChainIdentifier.unwrap()));
    }

    if (!CheckProofOfWork(popEndorsementHeader.GetHash(), popEndorsementHeader.nBits, params)) {
        return state.Invalid(TxValidationResult::TX_BAD_POP_DATA, "pop-tx-endorsed-block-pow", strprintf("endorsed block has invalid PoW: %s"));
    }

    return true;
}

void PopServiceImpl::setConfig()
{
    std::lock_guard<std::mutex> lock(mutex);
    auto& config = getService<Config>();

    ClientContext context;
    SetConfigRequest request;
    EmptyReply reply;

    //AltChainConfig
    auto* altChainConfig = new AltChainConfigRequest();
    altChainConfig->set_keystoneinterval(config.keystone_interval);

    // CalculatorConfig
    auto* calculatorConfig = new CalculatorConfig();
    calculatorConfig->set_basicreward(std::to_string(COIN));
    calculatorConfig->set_payoutrounds(config.payoutRounds);
    calculatorConfig->set_keystoneround(config.keystoneRound);

    auto* roundRatioConfig = new RoundRatioConfig();
    for (const auto& roundRatio : config.roundRatios) {
        std::string* round = roundRatioConfig->add_roundratio();
        *round = roundRatio;
    }
    calculatorConfig->set_allocated_roundratios(roundRatioConfig);

    auto* rewardCurveConfig = new RewardCurveConfig();
    rewardCurveConfig->set_startofdecreasingline(config.startOfDecreasingLine);
    rewardCurveConfig->set_widthofdecreasinglinenormal(config.widthOfDecreasingLineNormal);
    rewardCurveConfig->set_widthofdecreasinglinekeystone(config.widthOfDecreasingLineKeystone);
    rewardCurveConfig->set_aboveintendedpayoutmultipliernormal(config.aboveIntendedPayoutMultiplierNormal);
    rewardCurveConfig->set_aboveintendedpayoutmultiplierkeystone(config.aboveIntendedPayoutMultiplierKeystone);
    calculatorConfig->set_allocated_rewardcurve(rewardCurveConfig);

    calculatorConfig->set_maxrewardthresholdnormal(config.maxRewardThresholdNormal);
    calculatorConfig->set_maxrewardthresholdkeystone(config.maxRewardThresholdKeystone);

    auto* relativeScoreConfig = new RelativeScoreConfig();
    for (const auto& i : config.relativeScoreLookupTable) {
        relativeScoreConfig->add_score(i);
    }
    calculatorConfig->set_allocated_relativescorelookuptable(relativeScoreConfig);

    auto* flatScoreRoundConfig = new FlatScoreRoundConfig();
    flatScoreRoundConfig->set_round(config.flatScoreRound);
    flatScoreRoundConfig->set_active(config.flatScoreRoundUse);
    calculatorConfig->set_allocated_flatscoreround(flatScoreRoundConfig);

    calculatorConfig->set_popdifficultyaveraginginterval(config.POP_DIFFICULTY_AVERAGING_INTERVAL);
    calculatorConfig->set_poprewardsettlementinterval(config.POP_REWARD_SETTLEMENT_INTERVAL);

    //ForkresolutionConfig
    auto* forkresolutionConfig = new ForkresolutionConfigRequest();
    forkresolutionConfig->set_keystonefinalitydelay(config.keystone_finality_delay);
    forkresolutionConfig->set_amnestyperiod(config.amnesty_period);

    //VeriBlockBootstrapConfig
    auto* veriBlockBootstrapBlocks = new VeriBlockBootstrapConfig();
    for (const auto& bootstrap_veriblock_block : config.bootstrap_veriblock_blocks) {
        std::vector<uint8_t> bytes = ParseHex(bootstrap_veriblock_block);
        veriBlockBootstrapBlocks->add_blocks(bytes.data(), bytes.size());
    }

    //BitcoinBootstrapConfig
    auto* bitcoinBootstrapBlocks = new BitcoinBootstrapConfig();
    for (const auto& bootstrap_bitcoin_block : config.bootstrap_bitcoin_blocks) {
        std::vector<uint8_t> bytes = ParseHex(bootstrap_bitcoin_block);
        bitcoinBootstrapBlocks->add_blocks(bytes.data(), bytes.size());
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

void PopServiceImpl::clearTemporaryPayloads()
{
    // since we have no transactional interface (yet), we agreed to do the following instead:
    // 1. during block pop validation, execute addPayloads with heights from reserved block heights - this changes the state of alt-service and does all validations.
    // 2. since it changes state, after validation we want to rollback. execute removePayloads on all reserved heights to do that
    // 3. if node was shutdown during block pop validation, also clear all payloads within given height range
    // range is [uint32_t::max() - pop_tx_amount_per_block...uint32_t::max); total size is config.pop_tx_amount

    auto& config = getService<Config>();
    clearTemporaryPayloadsImpl(*this, getReservedBlockIndexBegin(config), getReservedBlockIndexEnd(config));
}


bool txPopValidation(PopServiceImpl& pop, const CTransactionRef& tx, const CBlockIndex& pindexPrev, const Consensus::Params& params, TxValidationState& state, uint32_t heightIndex) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    auto& config = VeriBlock::getService<VeriBlock::Config>();
    Publications publications;
    ScriptError serror = ScriptError::SCRIPT_ERR_UNKNOWN_ERROR;
    Context context;
    PopTxType type = PopTxType::UNKNOWN;
    if (!pop.parsePopTx(tx, &serror, &publications, &context, &type)) {
        return state.Invalid(TxValidationResult::TX_BAD_POP_DATA, "pop-tx-invalid-script", strprintf("[%s] scriptSig of POP tx is invalid: %s", tx->GetHash().ToString(), ScriptErrorString(serror)));
    }

    switch (type) {
    case PopTxType::CONTEXT: {
        try {
            pop.updateContext(context.vbk, context.btc);
        } catch (const PopServiceException& e) {
            return state.Invalid(TxValidationResult::TX_BAD_POP_DATA, "pop-tx-updatecontext-failed", strprintf("[%s] updatecontext failed: %s", tx->GetHash().ToString(), e.what()));
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
        } catch (const std::exception& e) {
            return state.Invalid(TxValidationResult::TX_BAD_POP_DATA, "pop-tx-alt-block-invalid", strprintf("[%s] can't deserialize endorsed block header: %s", tx->GetHash().ToString(), e.what()));
        }

        if (!pop.determineATVPlausibilityWithBTCRules(AltchainId(popEndorsement.identifier()), popEndorsementHeader, params, state)) {
            return false; // TxValidationState already set
        }

        AssertLockHeld(cs_main);
        const CBlockIndex* popEndorsementIdnex = LookupBlockIndex(popEndorsementHeader.GetHash());
        if (popEndorsementIdnex == nullptr) {
            return state.Invalid(TxValidationResult::TX_BAD_POP_DATA, "pop-tx-endorsed-block-not-known-orphan-block", strprintf("[%s] can not find endorsed block index: %s", tx->GetHash().ToString(), popEndorsementHeader.GetHash().ToString()));
        }
        const CBlockIndex* ancestor = pindexPrev.GetAncestor(popEndorsementIdnex->nHeight);
        if (ancestor == nullptr || ancestor->GetBlockHash() != popEndorsementIdnex->GetBlockHash()) {
            return state.Invalid(TxValidationResult::TX_BAD_POP_DATA, "pop-tx-endorsed-block-not-from-this-chain", strprintf("[%s] can not find endorsed block in the chain: %s", tx->GetHash().ToString(), popEndorsementHeader.GetHash().ToString()));
        }


        CBlock popEndorsementBlock;
        if (!ReadBlockFromDisk(popEndorsementBlock, popEndorsementIdnex, params)) {
            return state.Invalid(TxValidationResult::TX_BAD_POP_DATA, "pop-tx-endorsed-block-from-disk", strprintf("[%s] can not read endorsed block from disk: %s", tx->GetHash().ToString(), popEndorsementBlock.GetHash().ToString()));
        }

        BlockValidationState blockstate;
        if (!VeriBlock::VerifyTopLevelMerkleRoot(popEndorsementBlock, blockstate, popEndorsementIdnex->pprev)) {
            return state.Invalid(
                TxValidationResult::TX_BAD_POP_DATA,
                blockstate.GetRejectReason(),
                strprintf("[%s] top level merkle root is invalid: %s",
                    tx->GetHash().ToString(),
                    blockstate.GetDebugMessage()));
        }

        if (pindexPrev.nHeight + 1 - popEndorsementIdnex->nHeight > config.POP_REWARD_SETTLEMENT_INTERVAL) {
            return state.Invalid(TxValidationResult::TX_BAD_POP_DATA,
                "pop-tx-endorsed-block-too-old",
                strprintf("[%s] endorsed block is too old for this chain: %s. (last block height: %d, endorsed block height: %d, settlement interval: %d)",
                    tx->GetHash().ToString(),
                    popEndorsementBlock.GetHash().ToString(),
                    pindexPrev.nHeight + 1,
                    popEndorsementIdnex->nHeight,
                    config.POP_REWARD_SETTLEMENT_INTERVAL));
        }

        try {
            pop.addPayloads(heightToHash(heightIndex), heightIndex, publications);
        } catch (const PopServiceException& e) {
            return state.Invalid(TxValidationResult::TX_BAD_POP_DATA, "pop-tx-add-payloads-failed", strprintf("[%s] addPayloads failed: %s", tx->GetHash().ToString(), e.what()));
        }
        break;
    }
    default:
        return state.Invalid(TxValidationResult::TX_BAD_POP_DATA, "pop-tx-eval-script-failed", strprintf("[%s] EvalScript returned unexpected type", tx->GetHash().ToString()));
    }

    return true;
}


bool blockPopValidationImpl(PopServiceImpl& pop, const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    const auto& config = getService<Config>();
    size_t numOfPopTxes = 0;

    // block index here works as a database transaction ID
    // if given tx is invalid, we need to rollback the alt-service state
    // and to know which exactly - we use BlockIndex
    const auto blockIndexBegin = getReservedBlockIndexBegin(config);
    auto blockIndexIt = blockIndexBegin;
    const auto blockIndexEnd = getReservedBlockIndexEnd(config);

    LOCK(mempool.cs);
    AssertLockHeld(mempool.cs);
    AssertLockHeld(cs_main);
    for (const auto& tx : block.vtx) {
        if (!isPopTx(*tx)) {
            // do not even consider regular txes here
            continue;
        }

        if (++numOfPopTxes > config.max_pop_tx_amount) {
            pop.clearTemporaryPayloads();
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "pop-block-num-pop-tx", "too many pop transactions in a block");
        }

        TxValidationState txstate;
        assert(blockIndexBegin <= blockIndexEnd && "oh no, programming error");

        if (!txPopValidation(pop, tx, pindexPrev, params, txstate, blockIndexIt++)) {
            clearTemporaryPayloadsImpl(pop, getReservedBlockIndexBegin(config), blockIndexIt);
            mempool.removeRecursive(*tx, MemPoolRemovalReason::BLOCK);
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, txstate.GetRejectReason(), txstate.GetDebugMessage());
        }
    }

    // because this is validation, we need to clear current temporary payloads
    // actual addPayloads call is performed in savePopTxToDatabase
    clearTemporaryPayloadsImpl(pop, getReservedBlockIndexBegin(config), blockIndexIt);

    return true;
}

bool PopServiceImpl::blockPopValidation(const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    return blockPopValidationImpl(*this, block, pindexPrev, params, state);
}
} // namespace VeriBlock
