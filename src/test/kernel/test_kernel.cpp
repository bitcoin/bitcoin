// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/bitcoinkernel.h>
#include <kernel/bitcoinkernel_wrapper.h>

#define BOOST_TEST_MODULE Bitcoin Kernel Test Suite
#include <boost/test/included/unit_test.hpp>

#include <test/kernel/block_data.h>

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace btck;

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

std::vector<std::byte> hex_string_to_byte_vec(std::string_view hex)
{
    std::vector<std::byte> bytes;
    bytes.reserve(hex.length() / 2);

    for (size_t i{0}; i < hex.length(); i += 2) {
        uint8_t byte_value;
        auto [ptr, ec] = std::from_chars(hex.data() + i, hex.data() + i + 2, byte_value, 16);

        if (ec != std::errc{} || ptr != hex.data() + i + 2) {
            throw std::invalid_argument("Invalid hex character");
        }
        bytes.push_back(static_cast<std::byte>(byte_value));
    }
    return bytes;
}

std::span<const std::byte> as_bytes(std::vector<unsigned char> data)
{
    return std::span{reinterpret_cast<const std::byte*>(data.data()), data.size()};
}

constexpr auto VERIFY_ALL_PRE_SEGWIT{btck_SCRIPT_FLAGS_VERIFY_P2SH | btck_SCRIPT_FLAGS_VERIFY_DERSIG |
                                     btck_SCRIPT_FLAGS_VERIFY_NULLDUMMY | btck_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY |
                                     btck_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY};
constexpr auto VERIFY_ALL_PRE_TAPROOT{VERIFY_ALL_PRE_SEGWIT | btck_SCRIPT_FLAGS_VERIFY_WITNESS};

void check_equal(std::span<const std::byte> _actual, std::span<const std::byte> _expected)
{
    std::span<const uint8_t> actual{reinterpret_cast<const unsigned char*>(_actual.data()), _actual.size()};
    std::span<const uint8_t> expected{reinterpret_cast<const unsigned char*>(_expected.data()), _expected.size()};
    BOOST_CHECK_EQUAL_COLLECTIONS(
        actual.begin(), actual.end(),
        expected.begin(), expected.end());
}

class TestLog
{
public:
    void LogMessage(std::string_view message)
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
    void HeaderTipHandler(btck_SynchronizationState state, int64_t height, int64_t timestamp, bool presync) override
    {
        BOOST_CHECK_GT(timestamp, 0);
    }

    void WarningSetHandler(btck_Warning warning, std::string_view message) override
    {
        std::cout << "Kernel warning is set: " << message << std::endl;
    }

    void WarningUnsetHandler(btck_Warning warning) override
    {
        std::cout << "Kernel warning was unset." << std::endl;
    }

    void FlushErrorHandler(std::string_view error) override
    {
        std::cout << error << std::endl;
    }

    void FatalErrorHandler(std::string_view error) override
    {
        std::cout << error << std::endl;
    }
};

class TestValidationInterface : public ValidationInterface<TestValidationInterface>
{
public:
    TestValidationInterface() : ValidationInterface() {}

    std::optional<std::vector<std::byte>> m_expected_valid_block = std::nullopt;

    void BlockChecked(const UnownedBlock block, const BlockValidationState state) override
    {
        {
            auto ser_block{block.ToBytes()};
            if (m_expected_valid_block.has_value()) {
                check_equal(m_expected_valid_block.value(), ser_block);
            }
        }

        auto mode{state.ValidationMode()};
        switch (mode) {
        case btck_ValidationMode::btck_VALIDATION_STATE_VALID: {
            std::cout << "Valid block" << std::endl;
            return;
        }
        case btck_ValidationMode::btck_VALIDATION_STATE_INVALID: {
            std::cout << "Invalid block: ";
            auto result{state.BlockValidationResult()};
            switch (result) {
            case btck_BlockValidationResult::btck_BLOCK_RESULT_UNSET:
                std::cout << "initial value. Block has not yet been rejected" << std::endl;
                break;
            case btck_BlockValidationResult::btck_BLOCK_HEADER_LOW_WORK:
                std::cout << "the block header may be on a too-little-work chain" << std::endl;
                break;
            case btck_BlockValidationResult::btck_BLOCK_CONSENSUS:
                std::cout << "invalid by consensus rules (excluding any below reasons)" << std::endl;
                break;
            case btck_BlockValidationResult::btck_BLOCK_CACHED_INVALID:
                std::cout << "this block was cached as being invalid and we didn't store the reason why" << std::endl;
                break;
            case btck_BlockValidationResult::btck_BLOCK_INVALID_HEADER:
                std::cout << "invalid proof of work or time too old" << std::endl;
                break;
            case btck_BlockValidationResult::btck_BLOCK_MUTATED:
                std::cout << "the block's data didn't match the data committed to by the PoW" << std::endl;
                break;
            case btck_BlockValidationResult::btck_BLOCK_MISSING_PREV:
                std::cout << "We don't have the previous block the checked one is built on" << std::endl;
                break;
            case btck_BlockValidationResult::btck_BLOCK_INVALID_PREV:
                std::cout << "A block this one builds on is invalid" << std::endl;
                break;
            case btck_BlockValidationResult::btck_BLOCK_TIME_FUTURE:
                std::cout << "block timestamp was > 2 hours in the future (or our clock is bad)" << std::endl;
                break;
            }
            return;
        }
        case btck_ValidationMode::btck_VALIDATION_STATE_ERROR: {
            std::cout << "Internal error" << std::endl;
            return;
        }
        }
    }
};

void run_verify_test(
    const ScriptPubkey& spent_script_pubkey,
    const Transaction& spending_tx,
    std::span<TransactionOutput> spent_outputs,
    int64_t amount,
    unsigned int input_index,
    bool taproot)
{
    auto status = btck_ScriptVerifyStatus::btck_SCRIPT_VERIFY_OK;

    if (taproot) {
        BOOST_CHECK(spent_script_pubkey.Verify(
            amount,
            spending_tx,
            spent_outputs,
            input_index,
            btck_SCRIPT_FLAGS_VERIFY_ALL,
            status));
        BOOST_CHECK_EQUAL(status, btck_SCRIPT_VERIFY_OK);
    } else {
        BOOST_CHECK(!spent_script_pubkey.Verify(
            amount,
            spending_tx,
            spent_outputs,
            input_index,
            btck_SCRIPT_FLAGS_VERIFY_ALL,
            status));
        BOOST_CHECK_EQUAL(status, btck_SCRIPT_VERIFY_ERROR_SPENT_OUTPUTS_REQUIRED);
        status = btck_SCRIPT_VERIFY_OK;
    }

    BOOST_CHECK(spent_script_pubkey.Verify(
        amount,
        spending_tx,
        spent_outputs,
        input_index,
        VERIFY_ALL_PRE_TAPROOT,
        status));
    BOOST_CHECK_EQUAL(status, btck_SCRIPT_VERIFY_OK);

    BOOST_CHECK(spent_script_pubkey.Verify(
        0,
        spending_tx,
        spent_outputs,
        input_index,
        VERIFY_ALL_PRE_SEGWIT,
        status));
    BOOST_CHECK_EQUAL(status, btck_SCRIPT_VERIFY_OK);

    BOOST_CHECK(!spent_script_pubkey.Verify(
        amount,
        spending_tx,
        spent_outputs,
        input_index,
        VERIFY_ALL_PRE_TAPROOT << 2,
        status));
    BOOST_CHECK_EQUAL(status, btck_SCRIPT_VERIFY_ERROR_INVALID_FLAGS);

    status = btck_SCRIPT_VERIFY_OK;
}

BOOST_AUTO_TEST_CASE(btck_transaction_tests)
{
    auto tx_data{hex_string_to_byte_vec("02000000013f7cebd65c27431a90bba7f796914fe8cc2ddfc3f2cbd6f7e5f2fc854534da95000000006b483045022100de1ac3bcdfb0332207c4a91f3832bd2c2915840165f876ab47c5f8996b971c3602201c6c053d750fadde599e6f5c4e1963df0f01fc0d97815e8157e3d59fe09ca30d012103699b464d1d8bc9e47d4fb1cdaa89a1c5783d68363c4dbc4b524ed3d857148617feffffff02836d3c01000000001976a914fc25d6d5c94003bf5b0c7b640a248e2c637fcfb088ac7ada8202000000001976a914fbed3d9b11183209a57999d54d59f67c019e756c88ac6acb0700")};
    auto tx{Transaction{tx_data}};
    BOOST_CHECK_EQUAL(tx.CountOutputs(), 2);
    BOOST_CHECK_EQUAL(tx.CountInputs(), 1);
    auto broken_tx_data{std::span<std::byte>{tx_data.begin(), tx_data.begin() + 10}};
    BOOST_CHECK_THROW(Transaction{broken_tx_data}, std::runtime_error);
    auto output{tx.GetOutput(tx.CountOutputs() - 1)};
    BOOST_CHECK_EQUAL(output.Get().GetAmount(), 42130042);
    auto script_pubkey{output.Get().GetScriptPubkey()};
    {
        auto tx_new{Transaction{tx_data}};
        // This is safe, because we now use copy assignment
        TransactionOutput output = tx_new.GetOutput(tx_new.CountOutputs() - 1).Get();
    }
    BOOST_CHECK_EQUAL(output.Get().GetAmount(), 42130042);

    auto tx_roundtrip{Transaction{tx.ToBytes()}};
    check_equal(tx_roundtrip.ToBytes(), tx_data);

    // The following code is unsafe, but left here to show limitations of the
    // API, because we RVO-move the output beyond the lifetime of the
    // transaction. The reference wrapper should make this clear to the user.
    auto get_output = [&]() -> RefWrapper<TransactionOutput> {
        auto tx{Transaction{tx_data}};
        return tx.GetOutput(0);
    };
    auto output_new = get_output();
    BOOST_CHECK_EQUAL(output_new.Get().GetAmount(), 20737411);

    ScriptPubkey script_pubkey_roundtrip{script_pubkey.Get().ToBytes()};
    check_equal(script_pubkey_roundtrip.ToBytes(), script_pubkey.Get().ToBytes());
}

BOOST_AUTO_TEST_CASE(btck_script_verify_tests)
{
    // Legacy transaction aca326a724eda9a461c10a876534ecd5ae7b27f10f26c3862fb996f80ea2d45d
    run_verify_test(
        /*spent_script_pubkey*/ ScriptPubkey{hex_string_to_byte_vec("76a9144bfbaf6afb76cc5771bc6404810d1cc041a6933988ac")},
        /*spending_tx*/ Transaction{hex_string_to_byte_vec("02000000013f7cebd65c27431a90bba7f796914fe8cc2ddfc3f2cbd6f7e5f2fc854534da95000000006b483045022100de1ac3bcdfb0332207c4a91f3832bd2c2915840165f876ab47c5f8996b971c3602201c6c053d750fadde599e6f5c4e1963df0f01fc0d97815e8157e3d59fe09ca30d012103699b464d1d8bc9e47d4fb1cdaa89a1c5783d68363c4dbc4b524ed3d857148617feffffff02836d3c01000000001976a914fc25d6d5c94003bf5b0c7b640a248e2c637fcfb088ac7ada8202000000001976a914fbed3d9b11183209a57999d54d59f67c019e756c88ac6acb0700")},
        /*spent_outputs*/ {},
        /*amount*/ 0,
        /*input_index*/ 0,
        /*is_taproot*/ false);

    // Segwit transaction 1a3e89644985fbbb41e0dcfe176739813542b5937003c46a07de1e3ee7a4a7f3
    run_verify_test(
        /*spent_script_pubkey*/ ScriptPubkey{hex_string_to_byte_vec("0020701a8d401c84fb13e6baf169d59684e17abd9fa216c8cc5b9fc63d622ff8c58d")},
        /*spending_tx*/ Transaction{hex_string_to_byte_vec("010000000001011f97548fbbe7a0db7588a66e18d803d0089315aa7d4cc28360b6ec50ef36718a0100000000ffffffff02df1776000000000017a9146c002a686959067f4866b8fb493ad7970290ab728757d29f0000000000220020701a8d401c84fb13e6baf169d59684e17abd9fa216c8cc5b9fc63d622ff8c58d04004730440220565d170eed95ff95027a69b313758450ba84a01224e1f7f130dda46e94d13f8602207bdd20e307f062594022f12ed5017bbf4a055a06aea91c10110a0e3bb23117fc014730440220647d2dc5b15f60bc37dc42618a370b2a1490293f9e5c8464f53ec4fe1dfe067302203598773895b4b16d37485cbe21b337f4e4b650739880098c592553add7dd4355016952210375e00eb72e29da82b89367947f29ef34afb75e8654f6ea368e0acdfd92976b7c2103a1b26313f430c4b15bb1fdce663207659d8cac749a0e53d70eff01874496feff2103c96d495bfdd5ba4145e3e046fee45e84a8a48ad05bd8dbb395c011a32cf9f88053ae00000000")},
        /*spent_outputs*/ {},
        /*amount*/ 18393430,
        /*input_index*/ 0,
        /*is_taproot*/ false);

    // Taproot transaction 33e794d097969002ee05d336686fc03c9e15a597c1b9827669460fac98799036
    auto taproot_spent_script_pubkey{ScriptPubkey{hex_string_to_byte_vec("5120339ce7e165e67d93adb3fef88a6d4beed33f01fa876f05a225242b82a631abc0")}};
    std::vector<TransactionOutput> spent_outputs;
    spent_outputs.emplace_back(taproot_spent_script_pubkey, 88480);
    run_verify_test(
        /*spent_script_pubkey*/ taproot_spent_script_pubkey,
        /*spending_tx*/ Transaction{hex_string_to_byte_vec("01000000000101d1f1c1f8cdf6759167b90f52c9ad358a369f95284e841d7a2536cef31c0549580100000000fdffffff020000000000000000316a2f49206c696b65205363686e6f7272207369677320616e6420492063616e6e6f74206c69652e204062697462756734329e06010000000000225120a37c3903c8d0db6512e2b40b0dffa05e5a3ab73603ce8c9c4b7771e5412328f90140a60c383f71bac0ec919b1d7dbc3eb72dd56e7aa99583615564f9f99b8ae4e837b758773a5b2e4c51348854c8389f008e05029db7f464a5ff2e01d5e6e626174affd30a00")},
        /*spent_outputs*/ spent_outputs,
        /*amount*/ 88480,
        /*input_index*/ 0,
        /*is_taproot*/ true);
}

BOOST_AUTO_TEST_CASE(btck_logging_tests)
{
    btck_LoggingOptions logging_options = {
        .log_timestamps = true,
        .log_time_micros = true,
        .log_threadnames = false,
        .log_sourcelocations = false,
        .always_print_category_levels = true,
    };

    btck_logging_set_level_category(btck_LogCategory::btck_LOG_BENCH, btck_LogLevel::btck_LOG_TRACE);
    btck_logging_disable_category(btck_LogCategory::btck_LOG_BENCH);
    btck_logging_enable_category(btck_LogCategory::btck_LOG_VALIDATION);
    btck_logging_disable_category(btck_LogCategory::btck_LOG_VALIDATION);

    // Check that connecting, connecting another, and then disconnecting and connecting a logger again works.
    {
        btck_logging_set_level_category(btck_LogCategory::btck_LOG_KERNEL, btck_LogLevel::btck_LOG_TRACE);
        btck_logging_enable_category(btck_LogCategory::btck_LOG_KERNEL);
        Logger logger{std::make_unique<TestLog>(TestLog{}), logging_options};
        Logger logger_2{std::make_unique<TestLog>(TestLog{}), logging_options};
    }
    Logger logger{std::make_unique<TestLog>(TestLog{}), logging_options};
}

BOOST_AUTO_TEST_CASE(btck_context_tests)
{
    { // test default context
        Context context{};
    }

    { // test with context options, but not options set
        ContextOptions options{};
        Context context{options};
    }

    { // test with context options
        TestKernelNotifications notifications{};
        ContextOptions options{};
        ChainParams params{btck_ChainType::btck_CHAIN_TYPE_MAINNET};
        options.SetChainParams(params);
        options.SetNotifications(notifications);
        Context context{options};
    }
}

Context create_context(TestKernelNotifications& notifications, btck_ChainType chain_type, TestValidationInterface* validation_interface = nullptr)
{
    ContextOptions options{};
    ChainParams params{chain_type};
    options.SetChainParams(params);
    options.SetNotifications(notifications);
    if (validation_interface) {
        options.SetValidationInterface(*validation_interface);
    }
    auto context{Context{options}};
    return context;
}

BOOST_AUTO_TEST_CASE(btck_chainman_tests)
{
    btck_LoggingOptions logging_options = {
        .log_timestamps = true,
        .log_time_micros = true,
        .log_threadnames = false,
        .log_sourcelocations = false,
        .always_print_category_levels = true,
    };
    Logger logger{std::make_unique<TestLog>(TestLog{}), logging_options};
    auto test_directory{TestDirectory{"chainman_test_bitcoin_kernel"}};

    { // test with default context
        Context context{};
        ChainstateManagerOptions chainman_opts{context, test_directory.m_directory.string(), (test_directory.m_directory / "blocks").string()};
        ChainMan chainman{context, chainman_opts};
    }

    { // test with default context options
        ContextOptions options{};
        Context context{options};
        ChainstateManagerOptions chainman_opts{context, test_directory.m_directory.string(), (test_directory.m_directory / "blocks").string()};
        ChainMan chainman{context, chainman_opts};
    }

    TestKernelNotifications notifications{};
    auto context{create_context(notifications, btck_ChainType::btck_CHAIN_TYPE_MAINNET)};

    ChainstateManagerOptions chainman_opts{context, test_directory.m_directory.string(), (test_directory.m_directory / "blocks").string()};
    chainman_opts.SetWorkerThreads(4);
    BOOST_CHECK(chainman_opts.SetWipeDbs(/*wipe_block_tree=*/false, /*wipe_chainstate=*/false));
    BOOST_CHECK(!chainman_opts.SetWipeDbs(/*wipe_block_tree=*/true, /*wipe_chainstate=*/false));
    ChainMan chainman{context, chainman_opts};
}

std::unique_ptr<ChainMan> create_chainman(TestDirectory& test_directory,
                                          bool reindex,
                                          bool wipe_chainstate,
                                          bool block_tree_db_in_memory,
                                          bool chainstate_db_in_memory,
                                          Context& context)
{
    auto mainnet_test_directory{TestDirectory{"mainnet_test_bitcoin_kernel"}};

    ChainstateManagerOptions chainman_opts{context, test_directory.m_directory.string(), (test_directory.m_directory / "blocks").string()};

    if (reindex) {
        chainman_opts.SetWipeDbs(/*wipe_block_tree=*/reindex, /*wipe_chainstate=*/reindex);
    }
    if (wipe_chainstate) {
        chainman_opts.SetWipeDbs(/*wipe_block_tree=*/false, /*wipe_chainstate=*/wipe_chainstate);
    }
    if (block_tree_db_in_memory) {
        chainman_opts.SetBlockTreeDbInMemory(block_tree_db_in_memory);
    }
    if (chainstate_db_in_memory) {
        chainman_opts.SetChainstateDbInMemory(chainstate_db_in_memory);
    }

    auto chainman{std::make_unique<ChainMan>(context, chainman_opts)};
    return chainman;
}

void chainman_reindex_test(TestDirectory& test_directory)
{
    TestKernelNotifications notifications{};
    auto context{create_context(notifications, btck_ChainType::btck_CHAIN_TYPE_MAINNET)};
    auto chainman{create_chainman(test_directory, true, false, false, false, context)};

    std::vector<std::string> import_files;
    BOOST_CHECK(chainman->ImportBlocks(import_files));

    // Sanity check some block retrievals
    auto chain{chainman->GetChain()};
    auto genesis_index{chain.Get().GetGenesis()};
    auto genesis_block_raw{chainman->ReadBlock(genesis_index).value().ToBytes()};
    auto first_index{chain.Get().GetByHeight(0).value()};
    auto first_block_raw{chainman->ReadBlock(genesis_index).value().ToBytes()};
    check_equal(genesis_block_raw, first_block_raw);
    auto height{first_index.GetHeight()};
    BOOST_CHECK_EQUAL(height, 0);

    auto next_index{chain.Get().GetNextBlockTreeEntry(first_index).value()};
    BOOST_CHECK(chain.Get().Contains(next_index));
    auto next_block_data{chainman->ReadBlock(next_index).value().ToBytes()};
    auto tip_index{chain.Get().GetTip()};
    auto tip_block_data{chainman->ReadBlock(tip_index).value().ToBytes()};
    auto second_index{chain.Get().GetByHeight(1).value()};
    auto second_block{chainman->ReadBlock(second_index).value()};
    auto second_block_data{second_block.ToBytes()};
    auto second_height{second_index.GetHeight()};
    BOOST_CHECK_EQUAL(second_height, 1);
    check_equal(next_block_data, tip_block_data);
    check_equal(next_block_data, second_block_data);

    auto hash{second_index.GetHash()};
    auto another_second_index{chainman->GetBlockTreeEntry(hash.get())};
    auto another_second_height{another_second_index.GetHeight()};
    auto block_hash{second_block.GetHash()};
    BOOST_CHECK(std::equal(std::begin(block_hash->hash), std::end(block_hash->hash), std::begin(hash->hash)));
    BOOST_CHECK_EQUAL(second_height, another_second_height);
}

void chainman_reindex_chainstate_test(TestDirectory& test_directory)
{
    TestKernelNotifications notifications{};
    auto context{create_context(notifications, btck_ChainType::btck_CHAIN_TYPE_MAINNET)};
    auto chainman{create_chainman(test_directory, false, true, false, false, context)};

    std::vector<std::string> import_files;
    import_files.push_back((test_directory.m_directory / "blocks" / "blk00000.dat").string());
    BOOST_CHECK(chainman->ImportBlocks(import_files));
}

void chainman_mainnet_validation_test(TestDirectory& test_directory)
{
    TestKernelNotifications notifications{};
    TestValidationInterface validation_interface{};
    auto context{create_context(notifications, btck_ChainType::btck_CHAIN_TYPE_MAINNET, &validation_interface)};
    auto chainman{create_chainman(test_directory, false, false, false, false, context)};

    {
        // Process an invalid block
        auto raw_block = hex_string_to_byte_vec("012300");
        BOOST_CHECK_THROW(Block{raw_block}, std::runtime_error);
    }
    {
        // Process an empty block
        auto raw_block = hex_string_to_byte_vec("");
        BOOST_CHECK_THROW(Block{raw_block}, std::runtime_error);
    }

    // mainnet block 1
    auto raw_block = hex_string_to_byte_vec("010000006fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000982051fd1e4ba744bbbe680e1fee14677ba1a3c3540bf7b1cdb606e857233e0e61bc6649ffff001d01e362990101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d0104ffffffff0100f2052a0100000043410496b538e853519c726a2c91e61ec11600ae1390813a627c66fb8be7947be63c52da7589379515d4e0a604f8141781e62294721166bf621e73a82cbf2342c858eeac00000000");
    Block block{raw_block};
    Transaction tx{block.GetTransaction(block.CountOutputs() - 1)};

    validation_interface.m_expected_valid_block.emplace(raw_block);
    auto ser_block{block.ToBytes()};
    check_equal(ser_block, raw_block);
    bool new_block = false;
    BOOST_CHECK(chainman->ProcessBlock(block, &new_block));
    BOOST_CHECK(new_block);

    auto chain{chainman->GetChain()};
    auto tip{chain.Get().GetTip()};
    auto read_block{chainman->ReadBlock(tip)};
    BOOST_REQUIRE(read_block);
    check_equal(read_block.value().ToBytes(), raw_block);

    // Check that we can read the previous block
    auto tip_2{tip.GetPrevious()};
    auto read_block_2{chainman->ReadBlock(tip_2.value())};

    // It should be an error if we go another block back, since the genesis has no ancestor
    BOOST_CHECK(!tip_2.value().GetPrevious());

    // If we try to validate it again, it should be a duplicate
    BOOST_CHECK(chainman->ProcessBlock(block, &new_block));
    BOOST_CHECK(!new_block);
}

BOOST_AUTO_TEST_CASE(btck_chainman_mainnet_tests)
{
    btck_LoggingOptions logging_options = {
        .log_timestamps = true,
        .log_time_micros = true,
        .log_threadnames = false,
        .log_sourcelocations = false,
        .always_print_category_levels = true,
    };
    Logger logger{std::make_unique<TestLog>(TestLog{}), logging_options};

    auto test_directory{TestDirectory{"mainnet_test_bitcoin_kernel"}};
    chainman_mainnet_validation_test(test_directory);
    chainman_reindex_test(test_directory);
    chainman_reindex_chainstate_test(test_directory);
}

BOOST_AUTO_TEST_CASE(btck_chainman_in_memory_tests)
{
    auto in_memory_test_directory{TestDirectory{"in-memory_test_bitcoin_kernel"}};

    TestKernelNotifications notifications{};
    auto context{create_context(notifications, btck_ChainType::btck_CHAIN_TYPE_REGTEST)};
    auto chainman{create_chainman(in_memory_test_directory, false, false, true, true, context)};

    for (auto& raw_block : REGTEST_BLOCK_DATA) {
        Block block{as_bytes(raw_block)};
        bool new_block{false};
        chainman->ProcessBlock(block, &new_block);
        BOOST_CHECK(new_block);
    }

    BOOST_CHECK(!std::filesystem::exists(in_memory_test_directory.m_directory / "blocks" / "index"));
    BOOST_CHECK(!std::filesystem::exists(in_memory_test_directory.m_directory / "chainstate"));
}

BOOST_AUTO_TEST_CASE(btck_chainman_regtest_tests)
{
    auto test_directory{TestDirectory{"regtest_test_bitcoin_kernel"}};

    TestKernelNotifications notifications{};
    auto context{create_context(notifications, btck_ChainType::btck_CHAIN_TYPE_REGTEST)};

    // Validate 206 regtest blocks in total.
    // Stop halfway to check that it is possible to continue validating starting
    // from prior state.
    const size_t mid{REGTEST_BLOCK_DATA.size() / 2};

    {
        auto chainman{create_chainman(test_directory, false, false, false, false, context)};
        for (size_t i{0}; i < mid; i++) {
            Block block{as_bytes(REGTEST_BLOCK_DATA[i])};
            bool new_block{false};
            BOOST_CHECK(chainman->ProcessBlock(block, &new_block));
            BOOST_CHECK(new_block);
        }
    }

    auto chainman{create_chainman(test_directory, false, false, false, false, context)};

    for (size_t i{mid}; i < REGTEST_BLOCK_DATA.size(); i++) {
        Block block{as_bytes(REGTEST_BLOCK_DATA[i])};
        bool new_block{false};
        BOOST_CHECK(chainman->ProcessBlock(block, &new_block));
        BOOST_CHECK(new_block);
    }

    auto chain = chainman->GetChain();
    auto tip = chain.Get().GetTip();
    auto read_block = chainman->ReadBlock(tip).value();
    check_equal(read_block.ToBytes(), as_bytes(REGTEST_BLOCK_DATA[REGTEST_BLOCK_DATA.size() - 1]));

    auto tip_2 = tip.GetPrevious().value();
    auto read_block_2 = chainman->ReadBlock(tip_2).value();
    check_equal(read_block_2.ToBytes(), as_bytes(REGTEST_BLOCK_DATA[REGTEST_BLOCK_DATA.size() - 2]));

    BlockSpentOutputs block_spent_outputs{chainman->ReadBlockSpentOutputs(tip)};
    BOOST_CHECK_EQUAL(block_spent_outputs.m_size, 1);
    RefWrapper<TransactionSpentOutputs> transaction_spent_outputs{block_spent_outputs.GetTxSpentOutputs(block_spent_outputs.m_size - 1)};
    RefWrapper<Coin> coin{transaction_spent_outputs.Get().GetCoin(transaction_spent_outputs.Get().m_size - 1)};
    RefWrapper<TransactionOutput> output = coin.Get().GetOutput();
    uint32_t coin_height = coin.Get().GetConfirmationHeight();
    BOOST_CHECK_EQUAL(coin_height, 205);
    BOOST_CHECK_EQUAL(output.Get().GetAmount(), 100000000);
    auto script_pubkey = output.Get().GetScriptPubkey();
    auto script_pubkey_bytes{script_pubkey.Get().ToBytes()};
    BOOST_CHECK_EQUAL(script_pubkey_bytes.size(), 22);
    auto round_trip_script_pubkey{ScriptPubkey(script_pubkey_bytes)};
    BOOST_CHECK_EQUAL(round_trip_script_pubkey.ToBytes().size(), 22);

    // Test that reading past the size returns null data
    // BOOST_CHECK_THROW(block_spent_outputs.GetTxSpentOutputs(block_spent_outputs.m_size), std::runtime_error);

    // std::filesystem::remove_all(test_directory.m_directory / "blocks" / "blk00000.dat");
    // BOOST_CHECK(!chainman->ReadBlock(tip_2).has_value());
    // std::filesystem::remove_all(test_directory.m_directory / "blocks" / "rev00000.dat");
    // BOOST_CHECK_THROW(chainman->ReadBlockSpentOutputs(tip), std::runtime_error);
}
