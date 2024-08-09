#include <kernel/bitcoinkernel_wrapper.h>

#include <cassert>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>

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

std::string char_vec_to_hex_string(std::vector<unsigned char> char_vec)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto& byte : char_vec) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

void assert_error_ok(kernel_Error& error)
{
    if (error.code != kernel_ErrorCode::kernel_ERROR_OK) {
        std::cout << error.message << " error code: " << error.message << std::endl;
        assert(error.code == kernel_ErrorCode::kernel_ERROR_OK);
    }
}

class KernelLog
{
public:
    void LogMessage(const char* message)
    {
        std::cout << "kernel: " << message;
    }
};

class TestTaskRunner : public TaskRunner<TestTaskRunner>
{
};

class TestValidationInterface : public ValidationInterface<TestValidationInterface>
{
public:
    TestValidationInterface() : ValidationInterface() {}

    std::optional<std::string> m_expected_valid_block = std::nullopt;

    void BlockChecked(const UnownedBlock block, const BlockValidationState state) override
    {
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

class TestKernelNotifications : public KernelNotifications<TestKernelNotifications>
{
public:
    void BlockTipHandler(kernel_SynchronizationState state, kernel_BlockIndex* index) override
    {
        std::cout << "Block tip changed" << std::endl;
    }

    void ProgressHandler(const char* title, int progress_percent, bool resume_possible) override
    {
        std::cout << "Made progress: " << title << " " << progress_percent << "%" << std::endl;
    }

    void WarningSetHandler(kernel_Warning warning, const char* message) override
    {
        std::cout << message << std::endl;
    }

    void WarningUnsetHandler(kernel_Warning warning) override
    {
        std::cout << "Warning unset: " << warning << std::endl;
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
    std::filesystem::path abs_datadir{std::filesystem::absolute(argv[1])};
    std::filesystem::create_directories(abs_datadir);

    kernel_Error error;
    error.code = kernel_ErrorCode::kernel_ERROR_OK;

    kernel_LoggingOptions logging_options = {
        .log_timestamps = true,
        .log_time_micros = false,
        .log_threadnames = false,
        .log_sourcelocations = false,
        .always_print_category_levels = true,
    };

    Logger logger{std::make_unique<KernelLog>(KernelLog{}), logging_options, error};

    ContextOptions options{};
    ChainParams params{kernel_ChainType::kernel_CHAIN_TYPE_REGTEST};
    options.SetChainParams(params, error);
    assert_error_ok(error);

    TestKernelNotifications notifications{};
    options.SetNotifications(notifications, error);
    assert_error_ok(error);
    TestTaskRunner task_runner{};
    options.SetTaskRunner(task_runner, error);
    assert_error_ok(error);

    Context context{options, error};

    ChainstateManagerOptions chainman_opts{context, abs_datadir, error};
    assert_error_ok(error);
    BlockManagerOptions blockman_opts{context, abs_datadir / "blocks", error};
    assert_error_ok(error);

    auto chainman{std::make_unique<ChainMan>(context, chainman_opts, blockman_opts, error)};
    assert_error_ok(error);

    ChainstateLoadOptions chainstate_load_opts{};
    chainman->LoadChainstate(chainstate_load_opts, error);
    assert_error_ok(error);

    std::cout << "Enter the block you want to validate on the next line:" << std::endl;

    for (std::string line; std::getline(std::cin, line);) {
        if (line.empty()) {
            std::cerr << "Empty line found, try again:" << std::endl;
            continue;
        }

        auto raw_block{hex_string_to_char_vec(line)};
        auto block = Block{raw_block, error};
        if (error.code != kernel_ErrorCode::kernel_ERROR_OK) {
            std::cout << "Failed to parse entered block, try again:" << std::endl;
            continue;
        }

        bool accepted = chainman->ProcessBlock(block, error);
        assert_error_ok(error);
        if (accepted) {
            std::cout << "Validated block successfully." << std::endl;
        } else {
            std::cout << "Block was not accepted" << std::endl;
        }
    }
}
