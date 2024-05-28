// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/bitcoinkernel.h>
#include <kernel/bitcoinkernel_wrapper.h>

#define BOOST_TEST_MODULE Bitcoin Kernel Test Suite
#include <boost/test/included/unit_test.hpp>

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <random>
#include <ranges>
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

constexpr auto VERIFY_ALL_PRE_SEGWIT{ScriptVerificationFlags::P2SH | ScriptVerificationFlags::DERSIG |
                                     ScriptVerificationFlags::NULLDUMMY | ScriptVerificationFlags::CHECKLOCKTIMEVERIFY |
                                     ScriptVerificationFlags::CHECKSEQUENCEVERIFY};
constexpr auto VERIFY_ALL_PRE_TAPROOT{VERIFY_ALL_PRE_SEGWIT | ScriptVerificationFlags::WITNESS};

void check_equal(std::span<const std::byte> _actual, std::span<const std::byte> _expected, bool equal = true)
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
    void HeaderTipHandler(SynchronizationState state, int64_t height, int64_t timestamp, bool presync) override
    {
        BOOST_CHECK_GT(timestamp, 0);
    }

    void WarningSetHandler(Warning warning, std::string_view message) override
    {
        std::cout << "Kernel warning is set: " << message << std::endl;
    }

    void WarningUnsetHandler(Warning warning) override
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

void run_verify_test(
    const ScriptPubkey& spent_script_pubkey,
    const Transaction& spending_tx,
    std::span<TransactionOutput> spent_outputs,
    int64_t amount,
    unsigned int input_index,
    bool taproot)
{
    auto status = ScriptVerifyStatus::OK;

    if (taproot) {
        BOOST_CHECK(spent_script_pubkey.Verify(
            amount,
            spending_tx,
            spent_outputs,
            input_index,
            ScriptVerificationFlags::ALL,
            status));
        BOOST_CHECK(status == ScriptVerifyStatus::OK);
    } else {
        BOOST_CHECK(!spent_script_pubkey.Verify(
            amount,
            spending_tx,
            spent_outputs,
            input_index,
            ScriptVerificationFlags::ALL,
            status));
        BOOST_CHECK(status == ScriptVerifyStatus::ERROR_SPENT_OUTPUTS_REQUIRED);
        status = ScriptVerifyStatus::OK;
    }

    BOOST_CHECK(spent_script_pubkey.Verify(
        amount,
        spending_tx,
        spent_outputs,
        input_index,
        VERIFY_ALL_PRE_TAPROOT,
        status));
    BOOST_CHECK(status == ScriptVerifyStatus::OK);

    BOOST_CHECK(spent_script_pubkey.Verify(
        0,
        spending_tx,
        spent_outputs,
        input_index,
        VERIFY_ALL_PRE_SEGWIT,
        status));
    BOOST_CHECK(status == ScriptVerifyStatus::OK);

    status = ScriptVerifyStatus::OK;
}

template <typename T>
concept HasToBytes = requires(T t) {
    { t.ToBytes() } -> std::convertible_to<std::vector<std::byte>>;
};

template <typename T>
void CheckHandle(T object, T distinct_object)
{
    BOOST_CHECK(object.get() != nullptr);
    BOOST_CHECK(distinct_object.get() != nullptr);
    BOOST_CHECK(object.get() != distinct_object.get());

    if constexpr (HasToBytes<T>) {
        BOOST_CHECK_NE(object.ToBytes().size(), distinct_object.ToBytes().size());
    }

    // Copy constructor
    T object2(distinct_object);
    BOOST_CHECK_NE(object.get(), object2.get());
    if constexpr (HasToBytes<T>) {
        BOOST_CHECK_NE(object.ToBytes().size(), object2.ToBytes().size());
    }

    // Copy assignment
    T object3{distinct_object};
    object2 = object3;
    BOOST_CHECK_NE(object3.get(), object2.get());
    if constexpr (HasToBytes<T>) {
        BOOST_CHECK_NE(object.ToBytes().size(), object2.ToBytes().size());
        check_equal(object3.ToBytes(), object2.ToBytes());
    }

    // Move constructor
    auto* original_ptr = object2.get();
    T object4{std::move(object2)};
    BOOST_CHECK_EQUAL(object4.get(), original_ptr);
    BOOST_CHECK_EQUAL(object2.get(), nullptr); // NOLINT(bugprone-use-after-move)
    if constexpr (HasToBytes<T>) {
        check_equal(object4.ToBytes(), object3.ToBytes());
    }

    // Move assignment
    original_ptr = object4.get();
    object2 = std::move(object4);
    BOOST_CHECK_EQUAL(object2.get(), original_ptr);
    BOOST_CHECK_EQUAL(object4.get(), nullptr); // NOLINT(bugprone-use-after-move)
    if constexpr (HasToBytes<T>) {
        check_equal(object2.ToBytes(), object3.ToBytes());
    }
}

template <typename RangeType>
    requires std::ranges::random_access_range<RangeType>
void CheckRange(const RangeType& range, size_t expected_size)
{
    using value_type = std::ranges::range_value_t<RangeType>;

    BOOST_CHECK_EQUAL(range.size(), expected_size);
    BOOST_CHECK_EQUAL(range.empty(), (expected_size == 0));

    BOOST_CHECK(range.begin() != range.end());
    BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), static_cast<std::ptrdiff_t>(expected_size));
    BOOST_CHECK(range.cbegin() == range.begin());
    BOOST_CHECK(range.cend() == range.end());

    for (size_t i = 0; i < range.size(); ++i) {
        BOOST_CHECK_EQUAL(range[i].get(), (*(range.begin() + i)).get());
    }

    BOOST_CHECK_NE(range.at(0).get(), range.at(expected_size - 1).get());
    BOOST_CHECK_THROW(range.at(expected_size), std::out_of_range);

    BOOST_CHECK_EQUAL(range.front().get(), range[0].get());
    BOOST_CHECK_EQUAL(range.back().get(), range[expected_size - 1].get());

    auto it = range.begin();
    auto it_copy = it;
    ++it;
    BOOST_CHECK(it != it_copy);
    --it;
    BOOST_CHECK(it == it_copy);
    it = range.begin();
    auto old_it = it++;
    BOOST_CHECK(old_it == range.begin());
    BOOST_CHECK(it == range.begin() + 1);
    old_it = it--;
    BOOST_CHECK(old_it == range.begin() + 1);
    BOOST_CHECK(it == range.begin());

    it = range.begin();
    it += 2;
    BOOST_CHECK(it == range.begin() + 2);
    it -= 2;
    BOOST_CHECK(it == range.begin());

    BOOST_CHECK(range.begin() < range.end());
    BOOST_CHECK(range.begin() <= range.end());
    BOOST_CHECK(range.end() > range.begin());
    BOOST_CHECK(range.end() >= range.begin());
    BOOST_CHECK(range.begin() == range.begin());

    BOOST_CHECK_EQUAL(range.begin()[0].get(), range[0].get());

    size_t count = 0;
    for (auto rit = range.end(); rit != range.begin();) {
        --rit;
        ++count;
    }
    BOOST_CHECK_EQUAL(count, expected_size);

    std::vector<value_type> collected;
    for (const auto& elem : range) {
        collected.push_back(elem);
    }
    BOOST_CHECK_EQUAL(collected.size(), expected_size);

    BOOST_CHECK_EQUAL(std::ranges::size(range), expected_size);

    it = range.begin();
    auto it2 = 1 + it;
    BOOST_CHECK(it2 == it + 1);
}

BOOST_AUTO_TEST_CASE(btck_transaction_tests)
{
    auto tx_data{hex_string_to_byte_vec("02000000013f7cebd65c27431a90bba7f796914fe8cc2ddfc3f2cbd6f7e5f2fc854534da95000000006b483045022100de1ac3bcdfb0332207c4a91f3832bd2c2915840165f876ab47c5f8996b971c3602201c6c053d750fadde599e6f5c4e1963df0f01fc0d97815e8157e3d59fe09ca30d012103699b464d1d8bc9e47d4fb1cdaa89a1c5783d68363c4dbc4b524ed3d857148617feffffff02836d3c01000000001976a914fc25d6d5c94003bf5b0c7b640a248e2c637fcfb088ac7ada8202000000001976a914fbed3d9b11183209a57999d54d59f67c019e756c88ac6acb0700")};
    auto tx{Transaction{tx_data}};
    auto tx_data_2{hex_string_to_byte_vec("02000000000101904f4ee5c87d20090b642f116e458cd6693292ad9ece23e72f15fb6c05b956210500000000fdffffff02e2010000000000002251200839a723933b56560487ec4d67dda58f09bae518ffa7e148313c5696ac837d9f10060000000000002251205826bcdae7abfb1c468204170eab00d887b61ab143464a4a09e1450bdc59a3340140f26e7af574e647355830772946356c27e7bbc773c5293688890f58983499581be84de40be7311a14e6d6422605df086620e75adae84ff06b75ce5894de5e994a00000000")};
    auto tx2{Transaction{tx_data_2}};
    CheckHandle(tx, tx2);

    BOOST_CHECK_EQUAL(tx.CountOutputs(), 2);
    BOOST_CHECK_EQUAL(tx.CountInputs(), 1);
    auto broken_tx_data{std::span<std::byte>{tx_data.begin(), tx_data.begin() + 10}};
    BOOST_CHECK_THROW(Transaction{broken_tx_data}, std::runtime_error);
    auto output{tx.GetOutput(tx.CountOutputs() - 1)};
    BOOST_CHECK_EQUAL(output.Amount(), 42130042);
    auto script_pubkey{output.GetScriptPubkey()};
    {
        auto tx_new{Transaction{tx_data}};
        // This is safe, because we now use copy assignment
        TransactionOutput output = tx_new.GetOutput(tx_new.CountOutputs() - 1);
        ScriptPubkey script = output.GetScriptPubkey();

        TransactionOutputView output2 = tx_new.GetOutput(tx_new.CountOutputs() - 1);
        BOOST_CHECK_NE(output.get(), output2.get());
        BOOST_CHECK_EQUAL(output.Amount(), output2.Amount());
        TransactionOutput output3 = output2;
        BOOST_CHECK_NE(output3.get(), output2.get());
        BOOST_CHECK_EQUAL(output3.Amount(), output2.Amount());

        // Non-owned view
        ScriptPubkeyView script2 = output.GetScriptPubkey();
        BOOST_CHECK_NE(script.get(), script2.get());
        check_equal(script.ToBytes(), script2.ToBytes());

        // Non-owned to owned
        ScriptPubkey script3 = script2;
        BOOST_CHECK_NE(script3.get(), script2.get());
        check_equal(script3.ToBytes(), script2.ToBytes());
    }
    BOOST_CHECK_EQUAL(output.Amount(), 42130042);

    auto tx_roundtrip{Transaction{tx.ToBytes()}};
    check_equal(tx_roundtrip.ToBytes(), tx_data);

    // The following code is unsafe, but left here to show limitations of the
    // API, because we preserve the output view beyond the lifetime of the
    // transaction. The view type wrapper should make this clear to the user.
    // auto get_output = [&]() -> TransactionOutputView {
    //     auto tx{Transaction{tx_data}};
    //     return tx.GetOutput(0);
    // };
    // auto output_new = get_output();
    // BOOST_CHECK_EQUAL(output_new.Amount(), 20737411);

    int64_t total_amount{0};
    for (const auto output : tx.Outputs()) {
        total_amount += output.Amount();
    }
    BOOST_CHECK_EQUAL(total_amount, 62867453);

    auto amount = *(tx.Outputs() | std::ranges::views::filter([](const auto& output) {
                        return output.Amount() == 42130042;
                    }) |
                    std::views::transform([](const auto& output) {
                        return output.Amount();
                    })).begin();
    BOOST_REQUIRE(amount);
    BOOST_CHECK_EQUAL(amount, 42130042);

    CheckRange(tx.Outputs(), tx.CountOutputs());

    ScriptPubkey script_pubkey_roundtrip{script_pubkey.ToBytes()};
    check_equal(script_pubkey_roundtrip.ToBytes(), script_pubkey.ToBytes());
}

BOOST_AUTO_TEST_CASE(btck_script_pubkey)
{
    auto script_data{hex_string_to_byte_vec("76a9144bfbaf6afb76cc5771bc6404810d1cc041a6933988ac")};
    std::vector<std::byte> script_data_2 = script_data;
    script_data_2.push_back(std::byte{0x51});
    ScriptPubkey script{script_data};
    ScriptPubkey script2{script_data_2};
    CheckHandle(script, script2);
}

BOOST_AUTO_TEST_CASE(btck_transaction_output)
{
    ScriptPubkey script{hex_string_to_byte_vec("76a9144bfbaf6afb76cc5771bc6404810d1cc041a6933988ac")};
    TransactionOutput output{script, 1};
    TransactionOutput output2{script, 2};
    CheckHandle(output, output2);
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

BOOST_AUTO_TEST_CASE(logging_tests)
{
    btck_LoggingOptions logging_options = {
        .log_timestamps = true,
        .log_time_micros = true,
        .log_threadnames = false,
        .log_sourcelocations = false,
        .always_print_category_levels = true,
    };

    logging_set_level_category(LogCategory::BENCH, LogLevel::TRACE_LEVEL);
    logging_disable_category(LogCategory::BENCH);
    logging_enable_category(LogCategory::VALIDATION);
    logging_disable_category(LogCategory::VALIDATION);

    // Check that connecting, connecting another, and then disconnecting and connecting a logger again works.
    {
        logging_set_level_category(LogCategory::KERNEL, LogLevel::TRACE_LEVEL);
        logging_enable_category(LogCategory::KERNEL);
        Logger logger{std::make_unique<TestLog>(TestLog{}), logging_options};
        Logger logger_2{std::make_unique<TestLog>(TestLog{}), logging_options};
    }
    Logger logger{std::make_unique<TestLog>(TestLog{}), logging_options};
}

BOOST_AUTO_TEST_CASE(btck_context_tests)
{
    { // test default context
        Context context{};
        Context context2{};
        CheckHandle(context, context2);
    }

    { // test with context options, but not options set
        ContextOptions options{};
        Context context{options};
    }

    { // test with context options
        ContextOptions options{};
        ChainParams params{ChainType::MAINNET};
        ChainParams regtest_params{ChainType::REGTEST};
        CheckHandle(params, regtest_params);
        options.SetChainParams(params);
        options.SetNotifications(std::make_shared<TestKernelNotifications>());
        Context context{options};
    }
}

Context create_context(std::shared_ptr<TestKernelNotifications> notifications, ChainType chain_type)
{
    ContextOptions options{};
    ChainParams params{chain_type};
    options.SetChainParams(params);
    options.SetNotifications(notifications);
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

    auto notifications{std::make_shared<TestKernelNotifications>()};
    auto context{create_context(notifications, ChainType::MAINNET)};

    ChainstateManagerOptions chainman_opts{context, test_directory.m_directory.string(), (test_directory.m_directory / "blocks").string()};
    ChainMan chainman{context, chainman_opts};
}
