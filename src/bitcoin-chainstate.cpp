#include <kernel/bitcoinkernel_wrapper.h>

#include <cassert>
#include <charconv>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#ifdef WIN32
// clang-format off
#include <windows.h>
// clang-format on
#include <codecvt>
#include <locale>
#include <shellapi.h>
#endif

using namespace btck;

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

class KernelLog
{
public:
    void LogMessage(std::string_view message)
    {
        std::cout << "kernel: " << message;
    }
};

class TestValidationInterface : public ValidationInterface<TestValidationInterface>
{
public:
    TestValidationInterface() = default;

    std::optional<std::string> m_expected_valid_block = std::nullopt;

    void BlockChecked(const UnownedBlock block, const BlockValidationState state) override
    {
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

class TestKernelNotifications : public KernelNotifications<TestKernelNotifications>
{
public:
    void BlockTipHandler(btck_SynchronizationState, const BlockTreeEntry, double) override
    {
        std::cout << "Block tip changed" << std::endl;
    }

    void ProgressHandler(std::string_view title, int progress_percent, bool resume_possible) override
    {
        std::cout << "Made progress: " << title << " " << progress_percent << "%" << std::endl;
    }

    void WarningSetHandler(btck_Warning warning, std::string_view message) override
    {
        std::cout << message << std::endl;
    }

    void WarningUnsetHandler(btck_Warning warning) override
    {
        std::cout << "Warning unset: " << warning << std::endl;
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

int main(int argc, char* argv[])
{
    // SETUP: Argument parsing and handling
    if (argc != 2) {
        std::cerr
            << "Usage: " << argv[0] << " DATADIR" << std::endl
            << "Display DATADIR information, and process hex-encoded blocks on standard input." << std::endl
            << std::endl
            << "IMPORTANT: THIS EXECUTABLE IS EXPERIMENTAL, FOR TESTING ONLY, AND EXPECTED TO" << std::endl
            << "           BREAK IN FUTURE VERSIONS. DO NOT USE ON YOUR ACTUAL DATADIR." << std::endl;
        return 1;
    }

#ifdef WIN32
    int win_argc;
    wchar_t** wargv = CommandLineToArgvW(GetCommandLineW(), &win_argc);
    std::vector<std::string> utf8_args(win_argc);
    std::vector<char*> win_argv(win_argc);
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf8_cvt;
    for (int i = 0; i < win_argc; i++) {
        utf8_args[i] = utf8_cvt.to_bytes(wargv[i]);
        win_argv[i] = &utf8_args[i][0];
    }
    LocalFree(wargv);
    argc = win_argc;
    argv = win_argv.data();
#endif

    std::filesystem::path abs_datadir{std::filesystem::absolute(argv[1])};
    std::filesystem::create_directories(abs_datadir);

    btck_LoggingOptions logging_options = {
        .log_timestamps = true,
        .log_time_micros = false,
        .log_threadnames = false,
        .log_sourcelocations = false,
        .always_print_category_levels = true,
    };

    Logger logger{std::make_unique<KernelLog>(KernelLog{}), logging_options};

    ContextOptions options{};
    ChainParams params{btck_ChainType::btck_CHAIN_TYPE_MAINNET};
    options.SetChainParams(params);

    TestKernelNotifications notifications{};
    options.SetNotifications(notifications);
    TestValidationInterface validation_interface{};
    options.SetValidationInterface(validation_interface);

    Context context{options};

    ChainstateManagerOptions chainman_opts{context, abs_datadir.string(), (abs_datadir / "blocks").string()};
    chainman_opts.SetWorkerThreads(4);

    std::unique_ptr<ChainMan> chainman;
    try {
        chainman = std::make_unique<ChainMan>(context, chainman_opts);
    } catch (std::exception&) {
        std::cerr << "Failed to instantiate ChainMan, exiting" << std::endl;
        return 1;
    }

    std::cout << "Enter the block you want to validate on the next line:" << std::endl;

    for (std::string line; std::getline(std::cin, line);) {
        if (line.empty()) {
            std::cerr << "Empty line found, try again:" << std::endl;
            continue;
        }

        auto raw_block{hex_string_to_byte_vec(line)};
        std::unique_ptr<Block> block;
        try {
            block = std::make_unique<Block>(raw_block);
        } catch (std::exception&) {
            std::cerr << "Block decode failed, try again:" << std::endl;
            continue;
        }

        bool new_block = false;
        bool accepted = chainman->ProcessBlock(*block, &new_block);
        if (accepted) {
            std::cerr << "Block has not yet been rejected" << std::endl;
        } else {
            std::cerr << "Block was not accepted" << std::endl;
        }
        if (!new_block) {
            std::cerr << "Block is a duplicate" << std::endl;
        }
    }
}
