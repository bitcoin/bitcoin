// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>

#include <netaddress.h>
#include <netbase.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/sock.h>
#include <util/time.h>

#include <csignal>
#include <cstdint>
#include <exception>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <unistd.h>
#include <vector>

const std::function<void(const std::string&)> G_TEST_LOG_FUN{};

/**
 * A copy of the command line arguments that start with `--`.
 * First `LLVMFuzzerInitialize()` is called, which saves the arguments to `g_args`.
 * Later, depending on the fuzz test, `G_TEST_COMMAND_LINE_ARGUMENTS()` may be
 * called by `BasicTestingSetup` constructor to fetch those arguments and store
 * them in `BasicTestingSetup::m_node::args`.
 */
static std::vector<const char*> g_args;

static void SetArgs(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        // Only take into account arguments that start with `--`. The others are for the fuzz engine:
        // `fuzz -runs=1 fuzz_seed_corpus/address_deserialize_v2 --checkaddrman=5`
        if (strlen(argv[i]) > 2 && argv[i][0] == '-' && argv[i][1] == '-') {
            g_args.push_back(argv[i]);
        }
    }
}

const std::function<std::vector<const char*>()> G_TEST_COMMAND_LINE_ARGUMENTS = []() {
    return g_args;
};

std::map<std::string_view, std::tuple<TypeTestOneInput, TypeInitialize, TypeHidden>>& FuzzTargets()
{
    static std::map<std::string_view, std::tuple<TypeTestOneInput, TypeInitialize, TypeHidden>> g_fuzz_targets;
    return g_fuzz_targets;
}

void FuzzFrameworkRegisterTarget(std::string_view name, TypeTestOneInput target, TypeInitialize init, TypeHidden hidden)
{
    const auto it_ins = FuzzTargets().try_emplace(name, std::move(target), std::move(init), hidden);
    Assert(it_ins.second);
}

static std::string_view g_fuzz_target;
static TypeTestOneInput* g_test_one_input{nullptr};

void initialize()
{
    // Terminate immediately if a fuzzing harness ever tries to create a TCP socket.
    CreateSock = [](const CService&) -> std::unique_ptr<Sock> { std::terminate(); };

    // Terminate immediately if a fuzzing harness ever tries to perform a DNS lookup.
    g_dns_lookup = [](const std::string& name, bool allow_lookup) {
        if (allow_lookup) {
            std::terminate();
        }
        return WrappedGetAddrInfo(name, false);
    };

    bool should_abort{false};
    if (std::getenv("PRINT_ALL_FUZZ_TARGETS_AND_ABORT")) {
        for (const auto& t : FuzzTargets()) {
            if (std::get<2>(t.second)) continue;
            std::cout << t.first << std::endl;
        }
        should_abort = true;
    }
    if (const char* out_path = std::getenv("WRITE_ALL_FUZZ_TARGETS_AND_ABORT")) {
        std::cout << "Writing all fuzz target names to '" << out_path << "'." << std::endl;
        std::ofstream out_stream{out_path, std::ios::binary};
        for (const auto& t : FuzzTargets()) {
            if (std::get<2>(t.second)) continue;
            out_stream << t.first << std::endl;
        }
        should_abort = true;
    }
    Assert(!should_abort);
    g_fuzz_target = Assert(std::getenv("FUZZ"));
    const auto it = FuzzTargets().find(g_fuzz_target);
    if (it == FuzzTargets().end()) {
        std::cerr << "No fuzzer for " << g_fuzz_target << "." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    Assert(!g_test_one_input);
    g_test_one_input = &std::get<0>(it->second);
    std::get<1>(it->second)();
}

#if defined(PROVIDE_FUZZ_MAIN_FUNCTION)
static bool read_stdin(std::vector<uint8_t>& data)
{
    uint8_t buffer[1024];
    ssize_t length = 0;
    while ((length = read(STDIN_FILENO, buffer, 1024)) > 0) {
        data.insert(data.end(), buffer, buffer + length);
    }
    return length == 0;
}
#endif

#if defined(PROVIDE_FUZZ_MAIN_FUNCTION) && !defined(__AFL_LOOP)
static bool read_file(fs::path p, std::vector<uint8_t>& data)
{
    uint8_t buffer[1024];
    FILE* f = fsbridge::fopen(p, "rb");
    if (f == nullptr) return false;
    do {
        const size_t length = fread(buffer, sizeof(uint8_t), sizeof(buffer), f);
        if (ferror(f)) return false;
        data.insert(data.end(), buffer, buffer + length);
    } while (!feof(f));
    fclose(f);
    return true;
}
#endif

#if defined(PROVIDE_FUZZ_MAIN_FUNCTION) && !defined(__AFL_LOOP)
static fs::path g_input_path;
void signal_handler(int signal)
{
    if (signal == SIGABRT) {
        std::cerr << "Error processing input " << g_input_path << std::endl;
    } else {
        std::cerr << "Unexpected signal " << signal << " received\n";
    }
    std::_Exit(EXIT_FAILURE);
}
#endif

// This function is used by libFuzzer
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    static const auto& test_one_input = *Assert(g_test_one_input);
    test_one_input({data, size});
    return 0;
}

// This function is used by libFuzzer
extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
    SetArgs(*argc, *argv);
    initialize();
    return 0;
}

#if defined(PROVIDE_FUZZ_MAIN_FUNCTION)
int main(int argc, char** argv)
{
    initialize();
    static const auto& test_one_input = *Assert(g_test_one_input);
#ifdef __AFL_INIT
    // Enable AFL deferred forkserver mode. Requires compilation using
    // afl-clang-fast++. See fuzzing.md for details.
    __AFL_INIT();
#endif

#ifdef __AFL_LOOP
    // Enable AFL persistent mode. Requires compilation using afl-clang-fast++.
    // See fuzzing.md for details.
    while (__AFL_LOOP(1000)) {
        std::vector<uint8_t> buffer;
        if (!read_stdin(buffer)) {
            continue;
        }
        test_one_input(buffer);
    }
#else
    std::vector<uint8_t> buffer;
    if (argc <= 1) {
        if (!read_stdin(buffer)) {
            return 0;
        }
        test_one_input(buffer);
        return 0;
    }
    std::signal(SIGABRT, signal_handler);
    const auto start_time{Now<SteadySeconds>()};
    int tested = 0;
    for (int i = 1; i < argc; ++i) {
        fs::path input_path(*(argv + i));
        if (fs::is_directory(input_path)) {
            for (fs::directory_iterator it(input_path); it != fs::directory_iterator(); ++it) {
                if (!fs::is_regular_file(it->path())) continue;
                g_input_path = it->path();
                Assert(read_file(it->path(), buffer));
                test_one_input(buffer);
                ++tested;
                buffer.clear();
            }
        } else {
            g_input_path = input_path;
            Assert(read_file(input_path, buffer));
            test_one_input(buffer);
            ++tested;
            buffer.clear();
        }
    }
    const auto end_time{Now<SteadySeconds>()};
    std::cout << g_fuzz_target << ": succeeded against " << tested << " files in " << count_seconds(end_time - start_time) << "s." << std::endl;
#endif
    return 0;
}
#endif
