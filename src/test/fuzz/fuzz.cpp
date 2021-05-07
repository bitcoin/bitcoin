// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>

#include <test/util/setup_common.h>
#include <util/check.h>

#include <cstdint>
#include <unistd.h>
#include <vector>

const std::function<void(const std::string&)> G_TEST_LOG_FUN{};

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

static TypeTestOneInput* g_test_one_input{nullptr};

void initialize()
{
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
        std::ofstream out_stream(out_path, std::ios::binary);
        for (const auto& t : FuzzTargets()) {
            if (std::get<2>(t.second)) continue;
            out_stream << t.first << std::endl;
        }
        should_abort = true;
    }
    Assert(!should_abort);
    std::string_view fuzz_target{Assert(std::getenv("FUZZ"))};
    const auto it = FuzzTargets().find(fuzz_target);
    Assert(it != FuzzTargets().end());
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
    if (!read_stdin(buffer)) {
        return 0;
    }
    test_one_input(buffer);
#endif
    return 0;
}
#endif
