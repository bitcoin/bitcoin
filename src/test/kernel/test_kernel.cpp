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

std::string byte_span_to_hex_string_reversed(std::span<const std::byte> bytes)
{
    std::ostringstream oss;

    // Iterate in reverse order
    for (auto it = bytes.rbegin(); it != bytes.rend(); ++it) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<unsigned int>(static_cast<uint8_t>(*it));
    }

    return oss.str();
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

class TestKernelNotifications : public KernelNotifications
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

class TestValidationInterface : public ValidationInterface
{
public:
    std::optional<std::vector<std::byte>> m_expected_valid_block = std::nullopt;

    void BlockChecked(Block block, const BlockValidationState state) override
    {
        if (m_expected_valid_block.has_value()) {
            auto ser_block{block.ToBytes()};
            check_equal(m_expected_valid_block.value(), ser_block);
        }

        auto mode{state.GetValidationMode()};
        switch (mode) {
        case ValidationMode::VALID: {
            std::cout << "Valid block" << std::endl;
            return;
        }
        case ValidationMode::INVALID: {
            std::cout << "Invalid block: ";
            auto result{state.GetBlockValidationResult()};
            switch (result) {
            case BlockValidationResult::UNSET:
                std::cout << "initial value. Block has not yet been rejected" << std::endl;
                break;
            case BlockValidationResult::HEADER_LOW_WORK:
                std::cout << "the block header may be on a too-little-work chain" << std::endl;
                break;
            case BlockValidationResult::CONSENSUS:
                std::cout << "invalid by consensus rules (excluding any below reasons)" << std::endl;
                break;
            case BlockValidationResult::CACHED_INVALID:
                std::cout << "this block was cached as being invalid and we didn't store the reason why" << std::endl;
                break;
            case BlockValidationResult::INVALID_HEADER:
                std::cout << "invalid proof of work or time too old" << std::endl;
                break;
            case BlockValidationResult::MUTATED:
                std::cout << "the block's data didn't match the data committed to by the PoW" << std::endl;
                break;
            case BlockValidationResult::MISSING_PREV:
                std::cout << "We don't have the previous block the checked one is built on" << std::endl;
                break;
            case BlockValidationResult::INVALID_PREV:
                std::cout << "A block this one builds on is invalid" << std::endl;
                break;
            case BlockValidationResult::TIME_FUTURE:
                std::cout << "block timestamp was > 2 hours in the future (or our clock is bad)" << std::endl;
                break;
            }
            return;
        }
        case ValidationMode::INTERNAL_ERROR: {
            std::cout << "Internal error" << std::endl;
            return;
        }
        }
    }

    void BlockConnected(Block block, BlockTreeEntry entry) override
    {
        std::cout << "Block connected." << std::endl;
    }

    void PowValidBlock(BlockTreeEntry entry, Block block) override
    {
        std::cout << "Block passed pow verification" << std::endl;
    }

    void BlockDisconnected(Block block, BlockTreeEntry entry) override
    {
        std::cout << "Block disconnected." << std::endl;
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
    BOOST_CHECK_NE(distinct_object.get(), object2.get());
    if constexpr (HasToBytes<T>) {
        check_equal(distinct_object.ToBytes(), object2.ToBytes());
    }

    // Copy assignment
    T object3{distinct_object};
    object2 = object3;
    BOOST_CHECK_NE(object3.get(), object2.get());
    if constexpr (HasToBytes<T>) {
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

    auto invalid_data = hex_string_to_byte_vec("012300");
    BOOST_CHECK_THROW(Transaction{invalid_data}, std::runtime_error);
    auto empty_data = hex_string_to_byte_vec("");
    BOOST_CHECK_THROW(Transaction{empty_data}, std::runtime_error);

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

    std::span<std::byte> empty_data{};
    ScriptPubkey empty_script{empty_data};
    CheckHandle(script, empty_script);
}

BOOST_AUTO_TEST_CASE(btck_transaction_output)
{
    ScriptPubkey script{hex_string_to_byte_vec("76a9144bfbaf6afb76cc5771bc6404810d1cc041a6933988ac")};
    TransactionOutput output{script, 1};
    TransactionOutput output2{script, 2};
    CheckHandle(output, output2);
}

BOOST_AUTO_TEST_CASE(btck_transaction_input)
{
    Transaction tx{hex_string_to_byte_vec("020000000248c03e66fd371c7033196ce24298628e59ebefa00363026044e0f35e0325a65d000000006a473044022004893432347f39beaa280e99da595681ddb20fc45010176897e6e055d716dbfa022040a9e46648a5d10c33ef7cee5e6cf4b56bd513eae3ae044f0039824b02d0f44c012102982331a52822fd9b62e9b5d120da1d248558fac3da3a3c51cd7d9c8ad3da760efeffffffb856678c6e4c3c84e39e2ca818807049d6fba274b42af3c6d3f9d4b6513212d2000000006a473044022068bcedc7fe39c9f21ad318df2c2da62c2dc9522a89c28c8420ff9d03d2e6bf7b0220132afd752754e5cb1ea2fd0ed6a38ec666781e34b0e93dc9a08f2457842cf5660121033aeb9c079ea3e08ea03556182ab520ce5c22e6b0cb95cee6435ee17144d860cdfeffffff0260d50b00000000001976a914363cc8d55ea8d0500de728ef6d63804ddddbdc9888ac67040f00000000001976a914c303bdc5064bf9c9a8b507b5496bd0987285707988ac6acb0700")};
    TransactionInput input_0 = tx.GetInput(0);
    TransactionInput input_1 = tx.GetInput(1);
    CheckHandle(input_0, input_1);
    CheckRange(tx.Inputs(), tx.CountInputs());
    OutPoint point_0 = input_0.OutPoint();
    OutPoint point_1 = input_1.OutPoint();
    CheckHandle(point_0, point_1);
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

    logging_set_options(logging_options);
    logging_set_level_category(LogCategory::BENCH, LogLevel::TRACE_LEVEL);
    logging_disable_category(LogCategory::BENCH);
    logging_enable_category(LogCategory::VALIDATION);
    logging_disable_category(LogCategory::VALIDATION);

    // Check that connecting, connecting another, and then disconnecting and connecting a logger again works.
    {
        logging_set_level_category(LogCategory::KERNEL, LogLevel::TRACE_LEVEL);
        logging_enable_category(LogCategory::KERNEL);
        Logger logger{std::make_unique<TestLog>()};
        Logger logger_2{std::make_unique<TestLog>()};
    }
    Logger logger{std::make_unique<TestLog>()};
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

BOOST_AUTO_TEST_CASE(btck_block)
{
    Block block{hex_string_to_byte_vec(REGTEST_BLOCK_DATA[0])};
    Block block_100{hex_string_to_byte_vec(REGTEST_BLOCK_DATA[100])};
    CheckHandle(block, block_100);
    Block block_tx{hex_string_to_byte_vec(REGTEST_BLOCK_DATA[205])};
    CheckRange(block_tx.Transactions(), block_tx.CountTransactions());
    auto invalid_data = hex_string_to_byte_vec("012300");
    BOOST_CHECK_THROW(Block{invalid_data}, std::runtime_error);
    auto empty_data = hex_string_to_byte_vec("");
    BOOST_CHECK_THROW(Block{empty_data}, std::runtime_error);
}

Context create_context(std::shared_ptr<TestKernelNotifications> notifications, ChainType chain_type, std::shared_ptr<TestValidationInterface> validation_interface = nullptr)
{
    ContextOptions options{};
    ChainParams params{chain_type};
    options.SetChainParams(params);
    options.SetNotifications(notifications);
    if (validation_interface) {
        options.SetValidationInterface(validation_interface);
    }
    auto context{Context{options}};
    return context;
}

BOOST_AUTO_TEST_CASE(btck_chainman_tests)
{
    Logger logger{std::make_unique<TestLog>()};
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
    { // null or empty data_directory or blocks_directory are not allowed
        Context context{};
        auto valid_dir{test_directory.m_directory.string()};
        std::vector<std::pair<std::string_view, std::string_view>> illegal_cases{
            {"", valid_dir},
            {valid_dir, {nullptr, 0}},
            {"", ""},
            {{nullptr, 0}, {nullptr, 0}},
        };
        for (auto& [data_dir, blocks_dir] : illegal_cases) {
            BOOST_CHECK_THROW(ChainstateManagerOptions(context, data_dir, blocks_dir),
                              std::runtime_error);
        };
    }

    auto notifications{std::make_shared<TestKernelNotifications>()};
    auto context{create_context(notifications, ChainType::MAINNET)};

    ChainstateManagerOptions chainman_opts{context, test_directory.m_directory.string(), (test_directory.m_directory / "blocks").string()};
    chainman_opts.SetWorkerThreads(4);
    BOOST_CHECK(!chainman_opts.SetWipeDbs(/*wipe_block_tree=*/true, /*wipe_chainstate=*/false));
    BOOST_CHECK(chainman_opts.SetWipeDbs(/*wipe_block_tree=*/true, /*wipe_chainstate=*/true));
    BOOST_CHECK(chainman_opts.SetWipeDbs(/*wipe_block_tree=*/false, /*wipe_chainstate=*/true));
    BOOST_CHECK(chainman_opts.SetWipeDbs(/*wipe_block_tree=*/false, /*wipe_chainstate=*/false));
    ChainMan chainman{context, chainman_opts};
}

std::unique_ptr<ChainMan> create_chainman(TestDirectory& test_directory,
                                          bool reindex,
                                          bool wipe_chainstate,
                                          bool block_tree_db_in_memory,
                                          bool chainstate_db_in_memory,
                                          Context& context)
{
    ChainstateManagerOptions chainman_opts{context, test_directory.m_directory.string(), (test_directory.m_directory / "blocks").string()};

    if (reindex) {
        chainman_opts.SetWipeDbs(/*wipe_block_tree=*/reindex, /*wipe_chainstate=*/reindex);
    }
    if (wipe_chainstate) {
        chainman_opts.SetWipeDbs(/*wipe_block_tree=*/false, /*wipe_chainstate=*/wipe_chainstate);
    }
    if (block_tree_db_in_memory) {
        chainman_opts.UpdateBlockTreeDbInMemory(block_tree_db_in_memory);
    }
    if (chainstate_db_in_memory) {
        chainman_opts.UpdateChainstateDbInMemory(chainstate_db_in_memory);
    }

    auto chainman{std::make_unique<ChainMan>(context, chainman_opts)};
    return chainman;
}

void chainman_reindex_test(TestDirectory& test_directory)
{
    auto notifications{std::make_shared<TestKernelNotifications>()};
    auto context{create_context(notifications, ChainType::MAINNET)};
    auto chainman{create_chainman(test_directory, true, false, false, false, context)};

    std::vector<std::string> import_files;
    BOOST_CHECK(chainman->ImportBlocks(import_files));

    // Sanity check some block retrievals
    auto chain{chainman->GetChain()};
    BOOST_CHECK_THROW(chain.GetByHeight(1000), std::runtime_error);
    auto genesis_index{chain.Entries().front()};
    BOOST_CHECK(!genesis_index.GetPrevious());
    auto genesis_block_raw{chainman->ReadBlock(genesis_index).value().ToBytes()};
    auto first_index{chain.GetByHeight(0)};
    auto first_block_raw{chainman->ReadBlock(genesis_index).value().ToBytes()};
    check_equal(genesis_block_raw, first_block_raw);
    auto height{first_index.GetHeight()};
    BOOST_CHECK_EQUAL(height, 0);

    auto next_index{chain.GetByHeight(first_index.GetHeight() + 1)};
    BOOST_CHECK(chain.Contains(next_index));
    auto next_block_data{chainman->ReadBlock(next_index).value().ToBytes()};
    auto tip_index{chain.Entries().back()};
    auto tip_block_data{chainman->ReadBlock(tip_index).value().ToBytes()};
    auto second_index{chain.GetByHeight(1)};
    auto second_block{chainman->ReadBlock(second_index).value()};
    auto second_block_data{second_block.ToBytes()};
    auto second_height{second_index.GetHeight()};
    BOOST_CHECK_EQUAL(second_height, 1);
    check_equal(next_block_data, tip_block_data);
    check_equal(next_block_data, second_block_data);

    auto second_hash{second_index.GetHash()};
    auto another_second_index{chainman->GetBlockTreeEntry(second_hash)};
    BOOST_CHECK(another_second_index);
    auto another_second_height{another_second_index->GetHeight()};
    auto second_block_hash{second_block.GetHash()};
    check_equal(second_block_hash.ToBytes(), second_hash.ToBytes());
    BOOST_CHECK_EQUAL(second_height, another_second_height);
}

void chainman_reindex_chainstate_test(TestDirectory& test_directory)
{
    auto notifications{std::make_shared<TestKernelNotifications>()};
    auto context{create_context(notifications, ChainType::MAINNET)};
    auto chainman{create_chainman(test_directory, false, true, false, false, context)};

    std::vector<std::string> import_files;
    import_files.push_back((test_directory.m_directory / "blocks" / "blk00000.dat").string());
    BOOST_CHECK(chainman->ImportBlocks(import_files));
}

void chainman_mainnet_validation_test(TestDirectory& test_directory)
{
    auto notifications{std::make_shared<TestKernelNotifications>()};
    auto validation_interface{std::make_shared<TestValidationInterface>()};
    auto context{create_context(notifications, ChainType::MAINNET, validation_interface)};
    auto chainman{create_chainman(test_directory, false, false, false, false, context)};

    // mainnet block 1
    auto raw_block = hex_string_to_byte_vec("010000006fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000982051fd1e4ba744bbbe680e1fee14677ba1a3c3540bf7b1cdb606e857233e0e61bc6649ffff001d01e362990101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d0104ffffffff0100f2052a0100000043410496b538e853519c726a2c91e61ec11600ae1390813a627c66fb8be7947be63c52da7589379515d4e0a604f8141781e62294721166bf621e73a82cbf2342c858eeac00000000");
    Block block{raw_block};
    TransactionView tx{block.GetTransaction(block.CountTransactions() - 1)};
    BOOST_CHECK_EQUAL(byte_span_to_hex_string_reversed(tx.Txid().ToBytes()), "0e3e2357e806b6cdb1f70b54c3a3a17b6714ee1f0e68bebb44a74b1efd512098");
    BOOST_CHECK_EQUAL(tx.CountInputs(), 1);
    Transaction tx2 = tx;
    BOOST_CHECK_EQUAL(tx2.CountInputs(), 1);
    for (auto transaction : block.Transactions()) {
        BOOST_CHECK_EQUAL(transaction.CountInputs(), 1);
    }
    auto output_counts = *(block.Transactions() | std::views::transform([](const auto& tx) {
                               return tx.CountOutputs();
                           })).begin();
    BOOST_CHECK_EQUAL(output_counts, 1);

    validation_interface->m_expected_valid_block.emplace(raw_block);
    auto ser_block{block.ToBytes()};
    check_equal(ser_block, raw_block);
    bool new_block = false;
    BOOST_CHECK(chainman->ProcessBlock(block, &new_block));
    BOOST_CHECK(new_block);

    validation_interface->m_expected_valid_block = std::nullopt;
    new_block = false;
    Block invalid_block{hex_string_to_byte_vec(REGTEST_BLOCK_DATA[REGTEST_BLOCK_DATA.size() - 1])};
    BOOST_CHECK(!chainman->ProcessBlock(invalid_block, &new_block));
    BOOST_CHECK(!new_block);

    auto chain{chainman->GetChain()};
    BOOST_CHECK_EQUAL(chain.Height(), 1);
    auto tip{chain.Entries().back()};
    auto read_block{chainman->ReadBlock(tip)};
    BOOST_REQUIRE(read_block);
    check_equal(read_block.value().ToBytes(), raw_block);

    // Check that we can read the previous block
    BlockTreeEntry tip_2{*tip.GetPrevious()};
    Block read_block_2{*chainman->ReadBlock(tip_2)};
    BOOST_CHECK_EQUAL(chainman->ReadBlockSpentOutputs(tip_2).Count(), 0);
    BOOST_CHECK_EQUAL(chainman->ReadBlockSpentOutputs(tip).Count(), 0);

    // It should be an error if we go another block back, since the genesis has no ancestor
    BOOST_CHECK(!tip_2.GetPrevious());

    // If we try to validate it again, it should be a duplicate
    BOOST_CHECK(chainman->ProcessBlock(block, &new_block));
    BOOST_CHECK(!new_block);
}

BOOST_AUTO_TEST_CASE(btck_chainman_mainnet_tests)
{
    auto test_directory{TestDirectory{"mainnet_test_bitcoin_kernel"}};
    chainman_mainnet_validation_test(test_directory);
    chainman_reindex_test(test_directory);
    chainman_reindex_chainstate_test(test_directory);
}

BOOST_AUTO_TEST_CASE(btck_block_hash_tests)
{
    std::array<std::byte, 32> test_hash;
    std::array<std::byte, 32> test_hash_2;
    for (int i = 0; i < 32; ++i) {
        test_hash[i] = static_cast<std::byte>(i);
        test_hash_2[i] = static_cast<std::byte>(i + 1);
    }
    BlockHash block_hash{test_hash};
    BlockHash block_hash_2{test_hash_2};
    BOOST_CHECK(block_hash != block_hash_2);
    BOOST_CHECK(block_hash == block_hash);
    CheckHandle(block_hash, block_hash_2);
}

BOOST_AUTO_TEST_CASE(btck_block_tree_entry_tests)
{
    auto test_directory{TestDirectory{"block_tree_entry_test_bitcoin_kernel"}};
    auto notifications{std::make_shared<TestKernelNotifications>()};
    auto context{create_context(notifications, ChainType::REGTEST)};
    auto chainman{create_chainman(
        test_directory,
        /*reindex=*/false,
        /*wipe_chainstate=*/false,
        /*block_tree_db_in_memory=*/true,
        /*chainstate_db_in_memory=*/true,
        context)};

    // Process a couple of blocks
    for (size_t i{0}; i < 3; i++) {
        Block block{hex_string_to_byte_vec(REGTEST_BLOCK_DATA[i])};
        bool new_block{false};
        chainman->ProcessBlock(block, &new_block);
        BOOST_CHECK(new_block);
    }

    auto chain{chainman->GetChain()};
    auto entry_0{chain.GetByHeight(0)};
    auto entry_1{chain.GetByHeight(1)};
    auto entry_2{chain.GetByHeight(2)};

    // Test inequality
    BOOST_CHECK(entry_0 != entry_1);
    BOOST_CHECK(entry_1 != entry_2);
    BOOST_CHECK(entry_0 != entry_2);

    // Test equality with same entry
    BOOST_CHECK(entry_0 == chain.GetByHeight(0));
    BOOST_CHECK(entry_0 == BlockTreeEntry{entry_0});
    BOOST_CHECK(entry_1 == entry_1);

    // Test GetPrevious
    auto prev{entry_1.GetPrevious()};
    BOOST_CHECK(prev.has_value());
    BOOST_CHECK(prev.value() == entry_0);
}

BOOST_AUTO_TEST_CASE(btck_chainman_in_memory_tests)
{
    auto in_memory_test_directory{TestDirectory{"in-memory_test_bitcoin_kernel"}};

    auto notifications{std::make_shared<TestKernelNotifications>()};
    auto context{create_context(notifications, ChainType::REGTEST)};
    auto chainman{create_chainman(in_memory_test_directory, false, false, true, true, context)};

    for (auto& raw_block : REGTEST_BLOCK_DATA) {
        Block block{hex_string_to_byte_vec(raw_block)};
        bool new_block{false};
        chainman->ProcessBlock(block, &new_block);
        BOOST_CHECK(new_block);
    }

    BOOST_CHECK(std::filesystem::exists(in_memory_test_directory.m_directory / "blocks"));
    BOOST_CHECK(!std::filesystem::exists(in_memory_test_directory.m_directory / "blocks" / "index"));
    BOOST_CHECK(!std::filesystem::exists(in_memory_test_directory.m_directory / "chainstate"));

    BOOST_CHECK(context.interrupt());
}

BOOST_AUTO_TEST_CASE(btck_chainman_regtest_tests)
{
    auto test_directory{TestDirectory{"regtest_test_bitcoin_kernel"}};

    auto notifications{std::make_shared<TestKernelNotifications>()};
    auto context{create_context(notifications, ChainType::REGTEST)};

    // Validate 206 regtest blocks in total.
    // Stop halfway to check that it is possible to continue validating starting
    // from prior state.
    const size_t mid{REGTEST_BLOCK_DATA.size() / 2};

    {
        auto chainman{create_chainman(test_directory, false, false, false, false, context)};
        for (size_t i{0}; i < mid; i++) {
            Block block{hex_string_to_byte_vec(REGTEST_BLOCK_DATA[i])};
            bool new_block{false};
            BOOST_CHECK(chainman->ProcessBlock(block, &new_block));
            BOOST_CHECK(new_block);
        }
    }

    auto chainman{create_chainman(test_directory, false, false, false, false, context)};

    for (size_t i{mid}; i < REGTEST_BLOCK_DATA.size(); i++) {
        Block block{hex_string_to_byte_vec(REGTEST_BLOCK_DATA[i])};
        bool new_block{false};
        BOOST_CHECK(chainman->ProcessBlock(block, &new_block));
        BOOST_CHECK(new_block);
    }

    auto chain = chainman->GetChain();
    auto tip = chain.Entries().back();
    auto read_block = chainman->ReadBlock(tip).value();
    check_equal(read_block.ToBytes(), hex_string_to_byte_vec(REGTEST_BLOCK_DATA[REGTEST_BLOCK_DATA.size() - 1]));

    auto tip_2 = tip.GetPrevious().value();
    auto read_block_2 = chainman->ReadBlock(tip_2).value();
    check_equal(read_block_2.ToBytes(), hex_string_to_byte_vec(REGTEST_BLOCK_DATA[REGTEST_BLOCK_DATA.size() - 2]));

    Txid txid = read_block.Transactions()[0].Txid();
    Txid txid_2 = read_block_2.Transactions()[0].Txid();
    BOOST_CHECK(txid != txid_2);
    BOOST_CHECK(txid == txid);
    CheckHandle(txid, txid_2);

    auto find_transaction = [&chainman](const TxidView& target_txid) -> std::optional<Transaction> {
        auto chain = chainman->GetChain();
        for (const auto block_tree_entry : chain.Entries()) {
            auto block{chainman->ReadBlock(block_tree_entry)};
            for (const TransactionView transaction : block->Transactions()) {
                if (transaction.Txid() == target_txid) {
                    return Transaction{transaction};
                }
            }
        }
        return std::nullopt;
    };

    for (const auto block_tree_entry : chain.Entries()) {
        auto block{chainman->ReadBlock(block_tree_entry)};
        for (const auto transaction : block->Transactions()) {
            std::vector<TransactionInput> inputs;
            std::vector<TransactionOutput> spent_outputs;
            for (const auto input : transaction.Inputs()) {
                OutPointView point = input.OutPoint();
                if (point.index() == std::numeric_limits<uint32_t>::max()) {
                    continue;
                }
                inputs.emplace_back(input);
                BOOST_CHECK(point.Txid() != transaction.Txid());
                std::optional<Transaction> tx = find_transaction(point.Txid());
                BOOST_CHECK(tx.has_value());
                BOOST_CHECK(point.Txid() == tx->Txid());
                spent_outputs.emplace_back(tx->GetOutput(point.index()));
            }
            BOOST_CHECK(inputs.size() == spent_outputs.size());
            ScriptVerifyStatus status = ScriptVerifyStatus::OK;
            for (size_t i{0}; i < inputs.size(); ++i) {
                BOOST_CHECK(spent_outputs[i].GetScriptPubkey().Verify(spent_outputs[i].Amount(), transaction, spent_outputs, i, ScriptVerificationFlags::ALL, status));
            }
        }
    }

    // Read spent outputs for current tip and its previous block
    BlockSpentOutputs block_spent_outputs{chainman->ReadBlockSpentOutputs(tip)};
    BlockSpentOutputs block_spent_outputs_prev{chainman->ReadBlockSpentOutputs(*tip.GetPrevious())};
    CheckHandle(block_spent_outputs, block_spent_outputs_prev);
    CheckRange(block_spent_outputs_prev.TxsSpentOutputs(), block_spent_outputs_prev.Count());
    BOOST_CHECK_EQUAL(block_spent_outputs.Count(), 1);

    // Get transaction spent outputs from the last transaction in the two blocks
    TransactionSpentOutputsView transaction_spent_outputs{block_spent_outputs.GetTxSpentOutputs(block_spent_outputs.Count() - 1)};
    TransactionSpentOutputs owned_transaction_spent_outputs{transaction_spent_outputs};
    TransactionSpentOutputs owned_transaction_spent_outputs_prev{block_spent_outputs_prev.GetTxSpentOutputs(block_spent_outputs_prev.Count() - 1)};
    CheckHandle(owned_transaction_spent_outputs, owned_transaction_spent_outputs_prev);
    CheckRange(transaction_spent_outputs.Coins(), transaction_spent_outputs.Count());

    // Get the last coin from the transaction spent outputs
    CoinView coin{transaction_spent_outputs.GetCoin(transaction_spent_outputs.Count() - 1)};
    BOOST_CHECK(!coin.IsCoinbase());
    Coin owned_coin{coin};
    Coin owned_coin_prev{owned_transaction_spent_outputs_prev.GetCoin(owned_transaction_spent_outputs_prev.Count() - 1)};
    CheckHandle(owned_coin, owned_coin_prev);

    // Validate coin properties
    TransactionOutputView output = coin.GetOutput();
    uint32_t coin_height = coin.GetConfirmationHeight();
    BOOST_CHECK_EQUAL(coin_height, 205);
    BOOST_CHECK_EQUAL(output.Amount(), 100000000);

    // Test script pubkey serialization
    auto script_pubkey = output.GetScriptPubkey();
    auto script_pubkey_bytes{script_pubkey.ToBytes()};
    BOOST_CHECK_EQUAL(script_pubkey_bytes.size(), 22);
    auto round_trip_script_pubkey{ScriptPubkey(script_pubkey_bytes)};
    BOOST_CHECK_EQUAL(round_trip_script_pubkey.ToBytes().size(), 22);

    for (const auto tx_spent_outputs : block_spent_outputs.TxsSpentOutputs()) {
        for (const auto coins : tx_spent_outputs.Coins()) {
            BOOST_CHECK_GT(coins.GetOutput().Amount(), 1);
        }
    }

    CheckRange(chain.Entries(), chain.CountEntries());

    for (const BlockTreeEntry entry : chain.Entries()) {
        std::optional<Block> block{chainman->ReadBlock(entry)};
        if (block) {
            for (const TransactionView transaction : block->Transactions()) {
                for (const TransactionOutputView output : transaction.Outputs()) {
                    // skip data carrier outputs
                    if ((unsigned char)output.GetScriptPubkey().ToBytes()[0] == 0x6a) {
                        continue;
                    }
                    BOOST_CHECK_GT(output.Amount(), 1);
                }
            }
        }
    }

    int32_t count{0};
    for (const auto entry : chain.Entries()) {
        BOOST_CHECK_EQUAL(entry.GetHeight(), count);
        ++count;
    }
    BOOST_CHECK_EQUAL(count, chain.CountEntries());


    std::filesystem::remove_all(test_directory.m_directory / "blocks" / "blk00000.dat");
    BOOST_CHECK(!chainman->ReadBlock(tip_2).has_value());
    std::filesystem::remove_all(test_directory.m_directory / "blocks" / "rev00000.dat");
    BOOST_CHECK_THROW(chainman->ReadBlockSpentOutputs(tip), std::runtime_error);
}
