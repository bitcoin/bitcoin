// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/bitcoinkernel.h>
#include <kernel/bitcoinkernel_wrapper.h>

#include <test/kernel/block_data.h>

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <vector>

std::string random_string(uint32_t length)
{
    const std::string chars = "0123456789"
                              "abcdefghijklmnopqrstuvwxyz"
                              "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    static std::random_device rd;
    static std::default_random_engine dre{rd()};
    static std::uniform_int_distribution<> distribution(0, chars.size() - 1);

    std::string random;
    random.reserve(length);
    for (uint32_t i = 0; i < length; i++) {
        random += chars[distribution(dre)];
    }
    return random;
}

std::vector<unsigned char> hex_string_to_char_vec(const std::string& hex)
{
    std::vector<unsigned char> bytes;

    for (size_t i{0}; i < hex.length(); i += 2) {
        std::string byteString{hex.substr(i, 2)};
        unsigned char byte = (char)std::strtol(byteString.c_str(), nullptr, 16);
        bytes.push_back(byte);
    }

    return bytes;
}

void assert_is_error(kernel_Error& error, kernel_ErrorCode code)
{
    if (error.code != code) {
        std::cout << error.message << " error code: " << error.code << std::endl;
    }
    assert(error.code == code);
    error.code = kernel_ErrorCode::kernel_ERROR_OK;
    error.message[0] = '\0';
}

void assert_error_ok(kernel_Error& error)
{
    if (error.code != kernel_ErrorCode::kernel_ERROR_OK) {
        std::cout << error.message << " error code: " << error.code << std::endl;
        assert(error.code == kernel_ErrorCode::kernel_ERROR_OK);
    }
}

class TestLog
{
public:
    void LogMessage(const char* message)
    {
        std::cout << "kernel: " << message;
    }
};

struct TestDirectory {
    std::filesystem::path m_directory;
    TestDirectory(std::string directory_name)
        : m_directory{std::filesystem::temp_directory_path() / (directory_name + random_string(16))}
    {
        std::filesystem::create_directories(m_directory);
    }

    ~TestDirectory()
    {
        std::filesystem::remove_all(m_directory);
    }
};

class TestKernelNotifications : public KernelNotifications<TestKernelNotifications>
{
public:
    void BlockTipHandler(kernel_SynchronizationState state, kernel_BlockIndex* index) override
    {
        std::cout << "Block tip changed" << std::endl;
    }

    void HeaderTipHandler(kernel_SynchronizationState state, int64_t height, int64_t timestamp, bool presync) override
    {
        assert(timestamp > 0);
    }

    void ProgressHandler(const char* title, int progress_percent, bool resume_possible) override
    {
        std::cout << "Made progress: " << title << " " << progress_percent << "%" << std::endl;
    }

    void WarningSetHandler(kernel_Warning warning, const char* message) override
    {
        std::cout << "Kernel warning is set: " << message << std::endl;
    }

    void WarningUnsetHandler(kernel_Warning warning) override
    {
        std::cout << "Kernel warning was unset." << std::endl;
    }

    void FlushErrorHandler(const char* error) override
    {
        std::cout << error << std::endl;
    }

    void FatalErrorHandler(const char* error) override
    {
        std::cout << error << std::endl;
    }
};

class TestTaskRunner : public TaskRunner<TestTaskRunner>
{
};

class TestValidationInterface : public ValidationInterface<TestValidationInterface>
{
public:
    TestValidationInterface() : ValidationInterface() {}

    std::optional<std::vector<unsigned char>> m_expected_valid_block = std::nullopt;

    void BlockChecked(const UnownedBlock block, const BlockValidationState state) override
    {
        {
            auto serialized_block{block.GetBlockData()};
            if (m_expected_valid_block.has_value()) {
                assert(m_expected_valid_block.value() == serialized_block);
            }
        }

        auto mode{state.ValidationMode()};
        switch (mode) {
        case kernel_ValidationMode::kernel_VALIDATION_STATE_VALID: {
            std::cout << "Valid block" << std::endl;
            return;
        }
        case kernel_ValidationMode::kernel_VALIDATION_STATE_INVALID: {
            std::cout << "Invalid block: ";
            auto result{state.BlockValidationResult()};
            switch (result) {
            case kernel_BlockValidationResult::kernel_BLOCK_RESULT_UNSET:
                std::cout << "initial value. Block has not yet been rejected" << std::endl;
                break;
            case kernel_BlockValidationResult::kernel_BLOCK_HEADER_LOW_WORK:
                std::cout << "the block header may be on a too-little-work chain" << std::endl;
                break;
            case kernel_BlockValidationResult::kernel_BLOCK_CONSENSUS:
                std::cout << "invalid by consensus rules (excluding any below reasons)" << std::endl;
                break;
            case kernel_BlockValidationResult::kernel_BLOCK_RECENT_CONSENSUS_CHANGE:
                std::cout << "Invalid by a change to consensus rules more recent than SegWit." << std::endl;
                break;
            case kernel_BlockValidationResult::kernel_BLOCK_CACHED_INVALID:
                std::cout << "this block was cached as being invalid and we didn't store the reason why" << std::endl;
                break;
            case kernel_BlockValidationResult::kernel_BLOCK_INVALID_HEADER:
                std::cout << "invalid proof of work or time too old" << std::endl;
                break;
            case kernel_BlockValidationResult::kernel_BLOCK_MUTATED:
                std::cout << "the block's data didn't match the data committed to by the PoW" << std::endl;
                break;
            case kernel_BlockValidationResult::kernel_BLOCK_MISSING_PREV:
                std::cout << "We don't have the previous block the checked one is built on" << std::endl;
                break;
            case kernel_BlockValidationResult::kernel_BLOCK_INVALID_PREV:
                std::cout << "A block this one builds on is invalid" << std::endl;
                break;
            case kernel_BlockValidationResult::kernel_BLOCK_TIME_FUTURE:
                std::cout << "block timestamp was > 2 hours in the future (or our clock is bad)" << std::endl;
                break;
            case kernel_BlockValidationResult::kernel_BLOCK_CHECKPOINT:
                std::cout << "the block failed to meet one of our checkpoints" << std::endl;
                break;
            }
            return;
        }
        case kernel_ValidationMode::kernel_VALIDATION_STATE_ERROR: {
            std::cout << "Internal error" << std::endl;
            return;
        }
        }
    }
};

constexpr auto VERIFY_ALL_PRE_SEGWIT{kernel_SCRIPT_FLAGS_VERIFY_P2SH | kernel_SCRIPT_FLAGS_VERIFY_DERSIG |
                                     kernel_SCRIPT_FLAGS_VERIFY_NULLDUMMY | kernel_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY |
                                     kernel_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY};
constexpr auto VERIFY_ALL_PRE_TAPROOT{VERIFY_ALL_PRE_SEGWIT | kernel_SCRIPT_FLAGS_VERIFY_WITNESS};

void run_verify_test(
    std::vector<unsigned char> spent_script_pubkey,
    std::vector<unsigned char> spending_tx,
    std::vector<kernel_TransactionOutput> spent_outputs,
    int64_t amount,
    unsigned int input_index,
    bool taproot)
{
    kernel_Error error{};
    error.code = kernel_ErrorCode::kernel_ERROR_OK;

    if (taproot) {
        verify_script(
            spent_script_pubkey,
            amount,
            spending_tx,
            spent_outputs,
            input_index,
            kernel_SCRIPT_FLAGS_VERIFY_ALL,
            error);
        assert_error_ok(error);
    } else {
        assert(!verify_script(
            spent_script_pubkey,
            amount,
            spending_tx,
            spent_outputs,
            input_index,
            kernel_SCRIPT_FLAGS_VERIFY_ALL,
            error));
        assert_is_error(error, kernel_ERROR_SPENT_OUTPUTS_REQUIRED);
    }

    assert(verify_script(
        spent_script_pubkey,
        amount,
        spending_tx,
        spent_outputs,
        input_index,
        VERIFY_ALL_PRE_TAPROOT,
        error));
    assert_error_ok(error);

    assert(verify_script(
        spent_script_pubkey,
        0,
        spending_tx,
        spent_outputs,
        input_index,
        VERIFY_ALL_PRE_SEGWIT,
        error));
    assert_error_ok(error);

    assert(!verify_script(
        spent_script_pubkey,
        amount,
        spending_tx,
        spent_outputs,
        input_index,
        VERIFY_ALL_PRE_TAPROOT << 2,
        error));
    assert_is_error(error, kernel_ERROR_INVALID_FLAGS);

    assert(!verify_script(
        spent_script_pubkey,
        amount,
        spending_tx,
        spent_outputs,
        5,
        VERIFY_ALL_PRE_TAPROOT,
        error));
    assert_is_error(error, kernel_ERROR_TX_INDEX);

    auto broken_tx = std::span<unsigned char>{spending_tx.begin(), spending_tx.begin() + 10};
    assert(!verify_script(
        spent_script_pubkey,
        amount,
        broken_tx,
        spent_outputs,
        input_index,
        VERIFY_ALL_PRE_TAPROOT,
        error));
    assert_is_error(error, kernel_ERROR_TX_DESERIALIZE);
}

void script_verify_test()
{
    // Legacy transaction aca326a724eda9a461c10a876534ecd5ae7b27f10f26c3862fb996f80ea2d45d
    run_verify_test(
        /*spent_script_pubkey*/ hex_string_to_char_vec("76a9144bfbaf6afb76cc5771bc6404810d1cc041a6933988ac"),
        /*spending_tx*/ hex_string_to_char_vec("02000000013f7cebd65c27431a90bba7f796914fe8cc2ddfc3f2cbd6f7e5f2fc854534da95000000006b483045022100de1ac3bcdfb0332207c4a91f3832bd2c2915840165f876ab47c5f8996b971c3602201c6c053d750fadde599e6f5c4e1963df0f01fc0d97815e8157e3d59fe09ca30d012103699b464d1d8bc9e47d4fb1cdaa89a1c5783d68363c4dbc4b524ed3d857148617feffffff02836d3c01000000001976a914fc25d6d5c94003bf5b0c7b640a248e2c637fcfb088ac7ada8202000000001976a914fbed3d9b11183209a57999d54d59f67c019e756c88ac6acb0700"),
        /*spent_outputs*/ {},
        /*amount*/ 0,
        /*input_index*/ 0,
        /*is_taproot*/ false);

    // Segwit transaction 1a3e89644985fbbb41e0dcfe176739813542b5937003c46a07de1e3ee7a4a7f3
    run_verify_test(
        /*spent_script_pubkey*/ hex_string_to_char_vec("0020701a8d401c84fb13e6baf169d59684e17abd9fa216c8cc5b9fc63d622ff8c58d"),
        /*spending_tx*/ hex_string_to_char_vec("010000000001011f97548fbbe7a0db7588a66e18d803d0089315aa7d4cc28360b6ec50ef36718a0100000000ffffffff02df1776000000000017a9146c002a686959067f4866b8fb493ad7970290ab728757d29f0000000000220020701a8d401c84fb13e6baf169d59684e17abd9fa216c8cc5b9fc63d622ff8c58d04004730440220565d170eed95ff95027a69b313758450ba84a01224e1f7f130dda46e94d13f8602207bdd20e307f062594022f12ed5017bbf4a055a06aea91c10110a0e3bb23117fc014730440220647d2dc5b15f60bc37dc42618a370b2a1490293f9e5c8464f53ec4fe1dfe067302203598773895b4b16d37485cbe21b337f4e4b650739880098c592553add7dd4355016952210375e00eb72e29da82b89367947f29ef34afb75e8654f6ea368e0acdfd92976b7c2103a1b26313f430c4b15bb1fdce663207659d8cac749a0e53d70eff01874496feff2103c96d495bfdd5ba4145e3e046fee45e84a8a48ad05bd8dbb395c011a32cf9f88053ae00000000"),
        /*spent_outputs*/ {},
        /*amount*/ 18393430,
        /*input_index*/ 0,
        /*is_taproot*/ false);

    // Taproot transaction 33e794d097969002ee05d336686fc03c9e15a597c1b9827669460fac98799036
    auto taproot_spent_script_pubkey{hex_string_to_char_vec("5120339ce7e165e67d93adb3fef88a6d4beed33f01fa876f05a225242b82a631abc0")};
    run_verify_test(
        /*spent_script_pubkey*/ taproot_spent_script_pubkey,
        /*spending_tx*/ hex_string_to_char_vec("01000000000101b9cb0da76784960e000d63f0453221aeeb6df97f2119d35c3051065bc9881eab0000000000fdffffff020000000000000000186a16546170726f6f74204654572120406269746275673432a059010000000000225120339ce7e165e67d93adb3fef88a6d4beed33f01fa876f05a225242b82a631abc00247304402204bf50f2fea3a2fbf4db8f0de602d9f41665fe153840c1b6f17c0c0abefa42f0b0220631fe0968b166b00cb3027c8817f50ce8353e9d5de43c29348b75b6600f231fc012102b14f0e661960252f8f37486e7fe27431c9f94627a617da66ca9678e6a2218ce1ffd30a00"),
        /*spent_outputs*/ {
            kernel_TransactionOutput{.value = 88480, .script_pubkey = taproot_spent_script_pubkey.data(), .script_pubkey_len = taproot_spent_script_pubkey.size()},
        },
        /*amount*/ 88480,
        /*input_index*/ 0,
        /*is_taproot*/ true);
}

void logging_test()
{
    kernel_Error error;
    error.code = kernel_ErrorCode::kernel_ERROR_OK;

    kernel_LoggingOptions logging_options = {
        .log_timestamps = true,
        .log_time_micros = true,
        .log_threadnames = false,
        .log_sourcelocations = false,
        .always_print_category_levels = true,
    };

    kernel_add_log_level_category(kernel_LogCategory::kernel_LOG_BENCH, kernel_LogLevel::kernel_LOG_TRACE);
    kernel_disable_log_category(kernel_LogCategory::kernel_LOG_BENCH);
    kernel_enable_log_category(kernel_LogCategory::kernel_LOG_VALIDATION);
    kernel_disable_log_category(kernel_LogCategory::kernel_LOG_VALIDATION);

    // Check that connecting, connecting another, and then disconnecting and connecting a logger again works.
    {
        Logger logger{std::make_unique<TestLog>(TestLog{}), logging_options, error};
        assert_error_ok(error);
        Logger logger_2{std::make_unique<TestLog>(TestLog{}), logging_options, error};
        assert_error_ok(error);
    }
    Logger logger{std::make_unique<TestLog>(TestLog{}), logging_options, error};
    assert_error_ok(error);
}

void context_test()
{
    kernel_Error error;
    error.code = kernel_ErrorCode::kernel_ERROR_OK;

    { // test default context
        Context context{error};
        assert_error_ok(error);
    }

    { // test with context options
        TestKernelNotifications notifications{};
        ContextOptions options{};
        ChainParams params{kernel_ChainType::kernel_CHAIN_TYPE_MAINNET};
        options.SetChainParams(params, error);
        assert_error_ok(error);
        options.SetNotifications(notifications, error);
        assert_error_ok(error);
        Context context{options, error};
        assert_error_ok(error);
    }
}

Context create_context(TestKernelNotifications& notifications, kernel_Error& error, kernel_ChainType chain_type, TestTaskRunner* task_runner = nullptr)
{
    ContextOptions options{};
    ChainParams params{chain_type};
    options.SetChainParams(params, error);
    assert_error_ok(error);
    options.SetNotifications(notifications, error);
    assert_error_ok(error);
    if (task_runner) {
        options.SetTaskRunner(*task_runner, error);
        assert_error_ok(error);
    }

    return Context{options, error};
}

void chainman_test()
{
    auto test_directory{TestDirectory{"chainman_test_bitcoin_kernel"}};
    kernel_Error error{};
    error.code = kernel_ErrorCode::kernel_ERROR_OK;

    TestKernelNotifications notifications{};
    auto context{create_context(notifications, error, kernel_ChainType::kernel_CHAIN_TYPE_MAINNET)};

    // Check that creating invalid options gives us an error
    {
        kernel_Error opts_error{};
        ChainstateManagerOptions opts{context, "////\\\\", opts_error};
        assert_is_error(opts_error, kernel_ERROR_INTERNAL);
    }

    {
        kernel_Error opts_error{};
        BlockManagerOptions opts{context, "////\\\\", opts_error};
        assert_is_error(opts_error, kernel_ERROR_INTERNAL);
    }

    ChainstateManagerOptions chainman_opts{context, test_directory.m_directory, error};
    assert_error_ok(error);
    BlockManagerOptions blockman_opts{context, test_directory.m_directory / "blocks", error};
    assert_error_ok(error);

    ChainMan chainman{context, chainman_opts, blockman_opts, error};
    assert_error_ok(error);

    ChainstateLoadOptions chainstate_load_opts{};
    chainman.LoadChainstate(chainstate_load_opts, error);
    assert_error_ok(error);
}

std::unique_ptr<ChainMan> create_chainman(TestDirectory& test_directory,
                                          bool reindex,
                                          bool wipe_chainstate,
                                          bool block_tree_db_in_memory,
                                          bool chainstate_db_in_memory,
                                          kernel_Error& error,
                                          Context& context)
{
    ChainstateManagerOptions chainman_opts{context, test_directory.m_directory, error};
    assert_error_ok(error);
    BlockManagerOptions blockman_opts{context, test_directory.m_directory / "blocks", error};
    assert_error_ok(error);

    auto chainman{std::make_unique<ChainMan>(context, chainman_opts, blockman_opts, error)};
    assert_error_ok(error);

    ChainstateLoadOptions chainstate_load_opts{};
    if (reindex) {
        chainstate_load_opts.SetWipeBlockTreeDb(reindex);
        assert_error_ok(error);
        chainstate_load_opts.SetWipeChainstateDb(reindex);
        assert_error_ok(error);
    }
    if (wipe_chainstate) {
        chainstate_load_opts.SetWipeChainstateDb(wipe_chainstate);
        assert_error_ok(error);
    }
    if (block_tree_db_in_memory) {
        chainstate_load_opts.SetBlockTreeDbInMemory(block_tree_db_in_memory);
        assert_error_ok(error);
    }
    if (chainstate_db_in_memory) {
        chainstate_load_opts.SetChainstateDbInMemory(chainstate_db_in_memory);
        assert_error_ok(error);
    }
    chainman->LoadChainstate(chainstate_load_opts, error);
    assert_error_ok(error);

    return chainman;
}

void chainman_in_memory_test()
{
    auto in_memory_test_directory{TestDirectory{"in-memory_test_bitcoin_kernel"}};
    kernel_Error error;
    error.code = kernel_ErrorCode::kernel_ERROR_OK;

    TestKernelNotifications notifications{};
    auto context{create_context(notifications, error, kernel_ChainType::kernel_CHAIN_TYPE_REGTEST)};
    assert_error_ok(error);
    auto chainman{create_chainman(in_memory_test_directory, false, false, true, true, error, context)};

    for (auto& raw_block : REGTEST_BLOCK_DATA) {
        Block block{raw_block, error};
        assert_error_ok(error);
        chainman->ProcessBlock(block, error);
        assert_error_ok(error);
    }

    assert(!std::filesystem::exists(in_memory_test_directory.m_directory / "blocks" / "index"));
    assert(!std::filesystem::exists(in_memory_test_directory.m_directory / "chainstate"));
}

void chainman_mainnet_validation_test(TestDirectory& test_directory)
{
    kernel_Error error{};
    error.code = kernel_ErrorCode::kernel_ERROR_OK;

    TestKernelNotifications notifications{};
    TestTaskRunner task_runner{};

    auto context{create_context(notifications, error, kernel_ChainType::kernel_CHAIN_TYPE_MAINNET, &task_runner)};
    assert_error_ok(error);

    TestValidationInterface validation_interface{};
    validation_interface.Register(context, error);
    assert_error_ok(error);

    auto chainman{create_chainman(test_directory, false, false, false, false, error, context)};
    assert_error_ok(error);

    {
        // Process an invalid block
        auto raw_block = hex_string_to_char_vec("012300");
        Block block{raw_block, error};
        assert_is_error(error, kernel_ERROR_INTERNAL);
        error.code = kernel_ERROR_OK;
    }
    {
        // Process an empty block
        auto raw_block = hex_string_to_char_vec("");
        Block block{raw_block, error};
        assert_is_error(error, kernel_ERROR_INTERNAL);
        error.code = kernel_ERROR_OK;
    }

    // mainnet block 1
    auto raw_block = hex_string_to_char_vec("010000006fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000982051fd1e4ba744bbbe680e1fee14677ba1a3c3540bf7b1cdb606e857233e0e61bc6649ffff001d01e362990101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d0104ffffffff0100f2052a0100000043410496b538e853519c726a2c91e61ec11600ae1390813a627c66fb8be7947be63c52da7589379515d4e0a604f8141781e62294721166bf621e73a82cbf2342c858eeac00000000");
    Block block{raw_block, error};
    assert_error_ok(error);
    validation_interface.m_expected_valid_block.emplace(raw_block);
    assert(block.GetBlockData() == raw_block);
    assert(chainman->ProcessBlock(block, error));
    assert_error_ok(error);

    auto tip = chainman->GetBlockIndexFromTip();
    auto read_block = chainman->ReadBlock(tip, error);
    assert_error_ok(error);
    assert(read_block.GetBlockData() == raw_block);

    // Check that we can read the previous block
    auto tip_2 = tip.GetPreviousBlockIndex(error);
    assert_error_ok(error);
    auto read_block_2 = chainman->ReadBlock(tip_2, error);
    assert_error_ok(error);

    // It should be an error if we go another block back, since the genesis has no ancestor
    auto tip_3 = tip_2.GetPreviousBlockIndex(error);
    assert_is_error(error, kernel_ERROR_OUT_OF_BOUNDS);

    // If we try to validate it again, it should be a duplicate
    assert(!chainman->ProcessBlock(block, error));
    assert_is_error(error, kernel_ERROR_DUPLICATE_BLOCK);

    validation_interface.Unregister(context, error);
    assert_error_ok(error);
}

void chainman_regtest_validation_test()
{
    auto test_directory{TestDirectory{"regtest_test_bitcoin_kernel"}};
    kernel_Error error;
    error.code = kernel_ErrorCode::kernel_ERROR_OK;

    TestKernelNotifications notifications{};
    auto context{create_context(notifications, error, kernel_ChainType::kernel_CHAIN_TYPE_REGTEST)};
    assert_error_ok(error);

    // Validate 206 regtest blocks in total.
    // Stop halfway to check that it is possible to continue validating starting
    // from prior state.
    const size_t mid{REGTEST_BLOCK_DATA.size() / 2};

    {
        auto chainman{create_chainman(test_directory, false, false, false, false, error, context)};
        assert_error_ok(error);
        for (size_t i{0}; i < mid; i++) {
            Block block{REGTEST_BLOCK_DATA[i], error};
            assert_error_ok(error);
            chainman->ProcessBlock(block, error);
            assert_error_ok(error);
        }
    }

    auto chainman{create_chainman(test_directory, false, false, false, false, error, context)};
    assert_error_ok(error);

    for (size_t i{mid}; i < REGTEST_BLOCK_DATA.size(); i++) {
        Block block{REGTEST_BLOCK_DATA[i], error};
        assert_error_ok(error);
        chainman->ProcessBlock(block, error);
        assert_error_ok(error);
    }

    auto tip = chainman->GetBlockIndexFromTip();
    auto read_block = chainman->ReadBlock(tip, error);
    assert_error_ok(error);
    assert(read_block.GetBlockData() == REGTEST_BLOCK_DATA[REGTEST_BLOCK_DATA.size() - 1]);

    auto tip_2 = tip.GetPreviousBlockIndex(error);
    assert_error_ok(error);
    auto read_block_2 = chainman->ReadBlock(tip_2, error);
    assert_error_ok(error);
    assert(read_block_2.GetBlockData() == REGTEST_BLOCK_DATA[REGTEST_BLOCK_DATA.size() - 2]);

    auto block_undo = chainman->ReadBlockUndo(tip, error);
    assert_error_ok(error);
    auto tx_undo_size = block_undo.GetTxOutSize(block_undo.m_size - 1);
    assert_error_ok(error);
    auto output = block_undo.GetTxUndoPrevoutByIndex(block_undo.m_size - 1, tx_undo_size - 1, error);
    assert_error_ok(error);
    assert(output->script_pubkey_len == 22);
    assert(output->value == 100000000);
}

void chainman_reindex_test(TestDirectory& test_directory)
{
    kernel_Error error{};
    error.code = kernel_ErrorCode::kernel_ERROR_OK;

    TestKernelNotifications notifications{};
    auto context{create_context(notifications, error, kernel_ChainType::kernel_CHAIN_TYPE_MAINNET)};
    assert_error_ok(error);
    auto chainman{create_chainman(test_directory, true, false, false, false, error, context)};
    assert_error_ok(error);

    std::vector<std::string> import_files;
    chainman->ImportBlocks(import_files, error);
    assert_error_ok(error);

    // Sanity check some block retrievals
    auto genesis_index{chainman->GetBlockIndexFromGenesis()};
    auto genesis_block_raw{chainman->ReadBlock(genesis_index, error).GetBlockData()};
    auto first_index{chainman->GetBlockIndexByHeight(0, error)};
    assert_error_ok(error);
    auto first_block_raw{chainman->ReadBlock(genesis_index, error).GetBlockData()};
    assert(genesis_block_raw == first_block_raw);
    auto first_info{first_index.GetInfo()};
    assert(first_info->height == 0);

    auto next_index{chainman->GetNextBlockIndex(first_index, error)};
    assert_error_ok(error);
    auto next_block_string{chainman->ReadBlock(next_index, error).GetBlockData()};
    auto tip_index{chainman->GetBlockIndexFromTip()};
    auto tip_block_string{chainman->ReadBlock(tip_index, error).GetBlockData()};
    auto second_index{chainman->GetBlockIndexByHeight(1, error)};
    assert_error_ok(error);
    auto second_block_string{chainman->ReadBlock(second_index, error).GetBlockData()};
    auto second_info{second_index.GetInfo()};
    assert(second_info->height == 1);
    assert(next_block_string == tip_block_string);
    assert(next_block_string == second_block_string);
}

void chainman_reindex_chainstate_test(TestDirectory& test_directory)
{
    kernel_Error error{};
    error.code = kernel_ErrorCode::kernel_ERROR_OK;

    TestKernelNotifications notifications{};
    auto context{create_context(notifications, error, kernel_ChainType::kernel_CHAIN_TYPE_MAINNET)};
    assert_error_ok(error);
    auto chainman{create_chainman(test_directory, false, true, false, false, error, context)};
    assert_error_ok(error);

    std::vector<std::string> import_files;
    import_files.push_back(test_directory.m_directory / "blocks" / "blk00000.dat");
    chainman->ImportBlocks(import_files, error);
    assert_error_ok(error);
}

int main()
{
    script_verify_test();
    logging_test();

    kernel_Error error;
    error.code = kernel_ErrorCode::kernel_ERROR_OK;

    kernel_LoggingOptions logging_options = {
        .log_timestamps = true,
        .log_time_micros = true,
        .log_threadnames = false,
        .log_sourcelocations = false,
        .always_print_category_levels = true,
    };
    Logger logger{std::make_unique<TestLog>(TestLog{}), logging_options, error};
    assert_error_ok(error);

    context_test();

    chainman_test();

    auto mainnet_test_directory{TestDirectory{"mainnet_test_bitcoin_kernel"}};
    chainman_mainnet_validation_test(mainnet_test_directory);
    chainman_in_memory_test();

    chainman_regtest_validation_test();
    chainman_reindex_test(mainnet_test_directory);
    chainman_reindex_chainstate_test(mainnet_test_directory);

    std::cout << "Libbitcoinkernel test completed." << std::endl;
    return 0;
}
