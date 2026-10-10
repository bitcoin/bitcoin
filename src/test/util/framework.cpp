/** Copyright (c) 2026-present The Bitcoin Core developers
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php. */

#include <test/util/framework.h>

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <optional>
#include <random>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace framework {

/** Construct-on-first-use list of test cases. Prevents use of a global variable before initialization. */
std::vector<TestCase>& registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

/** Construct-on-first-use suite name. */
const char*& current_test_suite()
{
    static const char* s = "";
    return s;
}

Registrar::Registrar(const char* name, void (*fn)())
{
    registry().emplace_back(TestCase{current_test_suite(), name, fn});
}

/** Path to the test binary */
const char*& executable_path()
{
    static const char* prog = "";
    return prog;
}

LogLevel& current_log_level()
{
    static LogLevel level = LogLevel::Error;
    return level;
}

std::mutex& log_mutex()
{
    static std::mutex mutex;
    return mutex;
}

/** Simple bytes-to-hex function that avoids including crypto/hex_base.h, which is not linked in test_kernel. */
std::string to_hex(std::span<const unsigned char> bytes)
{
    static constexpr char digits[]{"0123456789abcdef"};
    std::string out(bytes.size() * 2, '\0');
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        out[2 * i] = digits[bytes[i] >> 4];
        out[2 * i + 1] = digits[bytes[i] & 0x0f];
    }
    return out;
}

std::string to_hex(std::span<const char> s) { return to_hex(MakeUCharSpan(s)); }
std::string to_hex(std::span<const std::byte> s) { return to_hex(MakeUCharSpan(s)); }

/** Stringify overloads for common types. */
std::string stringify(bool v) { return v ? "true" : "false"; }
std::string stringify(std::nullptr_t) { return "nullptr"; }
std::string stringify(std::nullopt_t) { return "nullopt"; }

std::string stringify(const char* s)
{
    return s ? std::string("\"") + s + "\"" : std::string{"nullptr"};
}

std::string stringify(const std::u8string& v)
{
    return "\"" + std::string(v.begin(), v.end()) + "\"";
}

std::string stringify(std::partial_ordering v)
{
    if (v == std::partial_ordering::less) return "less";
    if (v == std::partial_ordering::equivalent) return "equivalent";
    if (v == std::partial_ordering::greater) return "greater";
    return "unordered";
}

std::atomic<TestContext*>& test_context_store()
{
    static std::atomic<TestContext*> ctx{nullptr};
    return ctx;
}

TestContext::TestContext(std::string name) : m_full_name{std::move(name)}
{
    TestContext* ctx{nullptr};
    if (!test_context_store().compare_exchange_strong(ctx, this, std::memory_order_release, std::memory_order_relaxed)) {
        throw std::logic_error{"only one active test case is allowed per process."};
    }
}

TestContext::~TestContext() noexcept
{
    auto* prev{test_context_store().exchange(nullptr, std::memory_order_release)};
    assert(prev == this);
    (void)prev;
}

void TestContext::push_scoped_info(std::string info)
{
    std::scoped_lock lock{m_scoped_info_mutex};
    m_scoped_info_map[std::this_thread::get_id()].emplace_back(std::move(info));
}

void TestContext::pop_scoped_info()
{
    std::scoped_lock lock{m_scoped_info_mutex};
    auto it = m_scoped_info_map.find(std::this_thread::get_id());
    assert(it != m_scoped_info_map.end() && !it->second.empty());
    it->second.pop_back();
    if (it->second.empty()) m_scoped_info_map.erase(it);
}

std::optional<std::string> TestContext::render_scoped_info() const
{
    std::scoped_lock lock{m_scoped_info_mutex};
    auto it = m_scoped_info_map.find(std::this_thread::get_id());
    if (it == m_scoped_info_map.end() || it->second.empty()) return std::nullopt;
    std::string out;
    const auto& stack = it->second;
    for (std::size_t i = 0; i < stack.size(); ++i) {
        if (i > 0) out += '\n';
        out += "[INFO: ";
        out += stack[i];
        out += "]";
    }
    return out;
}

TestContext& test_context()
{
    if (auto* context{test_context_store().load(std::memory_order_acquire)}) return *context;
    throw std::logic_error{"test framework assertion used outside an active test case."};
}

InfoScopeGuard::InfoScopeGuard(std::string info) : m_ctx{test_context()}
{
    m_ctx.push_scoped_info(std::move(info));
}

InfoScopeGuard::~InfoScopeGuard() noexcept { m_ctx.pop_scoped_info(); }

std::string current_test_full_name()
{
    return test_context().full_name();
}

void record_check(TestContext& ctx, const Result& result, const char* kind, const char* expr,
                  const char* file, int line, const std::string& message)
{
    ctx.apply_result(result);
    if (!result.is_ok()) {
        log(LogLevel::Error, "[FAIL]: %s:%d: %s(%s)\n%s\n%s\n", file, line, kind, expr, *result.failed_expression, message);
        if (auto context = ctx.render_scoped_info()) {
            log(LogLevel::Error, "%s\n", *context);
        }
        return;
    }
    log(LogLevel::All, "[PASS]: %s:%d: %s(%s)\n", file, line, kind, expr);
}

std::string test_name(const TestCase& test_case)
{
    if (test_case.suite && test_case.suite[0] != '\0') {
        return std::string{test_case.suite} + "/" + test_case.name;
    }
    return test_case.name;
}

bool matches_filter(const TestCase& tc, std::string_view filter)
{
    if (filter.empty()) return true;
    const std::string suite = tc.suite ? std::string{tc.suite} : std::string{};
    const std::string name{tc.name};
    return std::ranges::any_of(std::views::split(filter, ','), [&](auto piece) {
        const std::string_view token{piece.begin(), piece.end()};
        const auto pos = token.find('/');
        if (pos == std::string_view::npos) return (tc.suite && token == suite) || token == name;
        return tc.suite && token.substr(0, pos) == suite && token.substr(pos + 1) == name;
    });
}

void list_tests(const std::string_view& filter)
{
    for (const auto& test_case : registry()) {
        if (matches_filter(test_case, filter)) {
            tfm::format(std::cout, "%s\n", test_name(test_case));
        }
    }
}

LogLevel to_log_level(std::string_view s)
{
    if (s == "nothing") return LogLevel::Nothing;
    if (s == "error") return LogLevel::Error;
    if (s == "info") return LogLevel::Info;
    if (s == "test_suite") return LogLevel::TestSuite;
    if (s == "all") return LogLevel::All;
    return LogLevel::Unknown;
}

ReportLevel to_report_level(std::string_view s)
{
    if (s == "no") return ReportLevel::Off;
    if (s == "yes") return ReportLevel::Show;
    return ReportLevel::Unknown;
}

CatchSystemError to_catch_system_error(std::string_view s)
{
    if (s == "no") return CatchSystemError::No;
    if (s == "yes") return CatchSystemError::Yes;
    return CatchSystemError::Unknown;
}

void print_usage(const std::string& prog)
{
    tfm::format(std::cout, "Usage: %s [--run_test|-t <filter[,filter...]>] [--log_level|-l <nothing|error|info|test_suite|all>] [--report_level <no|yes>] [--catch_system_error <no>] [--list_content] [-h|--help] [-- USER_ARGS...]\n"
                           "  filter: <suite> | <case> | <suite>/<case>\n",
                prog);
}

std::vector<const char*>& user_args()
{
    static std::vector<const char*> ua;
    return ua;
}

ExitStatus parse_args(RuntimeOptions& opts, int argc, char** argv)
{
    opts.prog = argv[0];
    for (int i{1}; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--") {
            for (int j{i + 1}; j < argc; ++j) {
                opts.passthrough.emplace_back(argv[j]);
            }
            break;
        }
        if (arg == "--list_content") {
            opts.show_list = true;
            continue;
        }
        if (arg == "-h" || arg == "--help") {
            opts.show_help = true;
            return ExitStatus::Success;
        }
        if (arg.starts_with("--run_test=")) {
            arg.remove_prefix(std::string{"--run_test="}.size());
            opts.filter = arg;
            continue;
        }
        if (arg == std::string_view{"--run_test"}) {
            if (++i >= argc) {
                opts.show_help = true;
                return ExitStatus::Failure;
            }
            opts.filter = argv[i];
            continue;
        }
        if (arg == std::string_view{"-t"}) {
            if (++i >= argc) {
                opts.show_help = true;
                return ExitStatus::Failure;
            }
            opts.filter = argv[i];
            continue;
        }
        if (arg.starts_with("--log_level=")) {
            arg.remove_prefix(std::string{"--log_level="}.size());
            auto log_level = to_log_level(arg);
            if (log_level == LogLevel::Unknown) {
                opts.show_help = true;
                return ExitStatus::UnknownArgs;
            }
            opts.log_level = log_level;
            continue;
        }
        if (arg == std::string_view{"--log_level"}) {
            if (++i >= argc) {
                opts.show_help = true;
                return ExitStatus::Failure;
            }
            auto log_level = to_log_level(argv[i]);
            if (log_level == LogLevel::Unknown) {
                opts.show_help = true;
                return ExitStatus::UnknownArgs;
            }
            opts.log_level = log_level;
            continue;
        }
        if (arg == std::string_view{"-l"}) {
            if (++i >= argc) {
                opts.show_help = true;
                return ExitStatus::Failure;
            }
            auto log_level = to_log_level(argv[i]);
            if (log_level == LogLevel::Unknown) {
                opts.show_help = true;
                return ExitStatus::UnknownArgs;
            }
            opts.log_level = log_level;
            continue;
        }
        if (arg.starts_with("--report_level=")) {
            arg.remove_prefix(std::string{"--report_level="}.size());
            auto report_level = to_report_level(arg);
            if (report_level == ReportLevel::Unknown) {
                opts.show_help = true;
                return ExitStatus::UnknownArgs;
            }
            opts.show_run_report = report_level;
            continue;
        }
        if (arg == std::string_view{"--report_level"}) {
            if (++i >= argc) {
                opts.show_help = true;
                return ExitStatus::Failure;
            }
            auto report_level = to_report_level(argv[i]);
            if (report_level == ReportLevel::Unknown) {
                opts.show_help = true;
                return ExitStatus::UnknownArgs;
            }
            opts.show_run_report = report_level;
            continue;
        }
        if (arg.starts_with("--catch_system_error=")) {
            arg.remove_prefix(std::string{"--catch_system_error="}.size());
            auto cse = to_catch_system_error(arg);
            if (cse == CatchSystemError::Unknown) {
                opts.show_help = true;
                return ExitStatus::UnknownArgs;
            }
            if (cse == CatchSystemError::Yes) {
                opts.show_help = true;
                tfm::format(std::cout, "--catch_system_error=yes is not yet supported\n");
                return ExitStatus::Unsupported;
            }
            continue;
        }
        if (arg == std::string_view{"--catch_system_error"}) {
            if (++i >= argc) {
                opts.show_help = true;
                return ExitStatus::Failure;
            }
            auto cse = to_catch_system_error(argv[i]);
            if (cse == CatchSystemError::Unknown) {
                opts.show_help = true;
                return ExitStatus::UnknownArgs;
            }
            if (cse == CatchSystemError::Yes) {
                opts.show_help = true;
                tfm::format(std::cout, "--catch_system_error=yes is not yet supported\n");
                return ExitStatus::Unsupported;
            }
            continue;
        }
        tfm::format(std::cout, "Unknown argument: %s\n", argv[i]);
        opts.show_help = true;
        return ExitStatus::UnknownArgs;
    }

    return ExitStatus::Success;
}

/** Boost.Test BOOST_TEST_RANDOM semantics: "0"/unset => nullopt, "1" => OS entropy, other N => N. */
std::optional<unsigned int> parse_shuffle_seed(const char* arg)
{
    if (!arg) return std::nullopt;
    const std::string_view value{arg};
    unsigned int seed{0};
    const auto result{std::from_chars(value.data(), value.data() + value.size(), seed)};
    if (result.ec != std::errc{} || seed == 0) return std::nullopt;
    if (seed == 1) return std::random_device{}();
    return seed;
}

int run(int argc, char** argv)
{
    RuntimeOptions opts{};
    auto status = parse_args(opts, argc, argv);
    executable_path() = opts.prog;
    if (const char* env_filter{std::getenv("BOOST_TEST_RUN_FILTERS")}; env_filter && opts.filter.empty()) {
        opts.filter = env_filter;
    }
    if (opts.show_help) {
        print_usage(opts.prog);
        return status;
    }
    if (opts.show_list) {
        list_tests(opts.filter);
        return status;
    }
    current_log_level() = opts.log_level;
    user_args() = std::move(opts.passthrough);
    if (auto seed{parse_shuffle_seed(std::getenv("BOOST_TEST_RANDOM"))}) {
        log(LogLevel::Error, "Test order shuffled with seed %u\n", *seed);
        std::ranges::shuffle(registry(), std::mt19937{*seed});
    }
    RunSummary summary{};
    int total_matching = 0;
    for (const auto& tc : registry()) {
        if (matches_filter(tc, opts.filter)) ++total_matching;
    }
    log(LogLevel::Error, "Running %d test cases...\n", total_matching);
    for (const auto& test_case : registry()) {
        if (!matches_filter(test_case, opts.filter)) {
            ++summary.skipped;
            continue;
        }
        TestContext ctx{test_name(test_case)};
        log(LogLevel::TestSuite, "Entering %s\n", ctx.full_name());
        try {
            test_case.fn();
        } catch (const RequireFailed&) {
        } catch (const std::exception& e) {
            ctx.exception_occurred();
            log(LogLevel::Error, "EXCEPTION in %s: %s\n", test_case.name, e.what());
        } catch (...) {
            ctx.exception_occurred();
            log(LogLevel::Error, "EXCEPTION in %s: %s\n", test_case.name, "unknown exception");
        }
        const auto stats = ctx.stats();
        summary.total_checks += stats.checks;
        if (ctx.passed()) {
            ++summary.passed;
            log(LogLevel::Info, "[ OK ] %s (%d checks)\n", test_case.name, stats.checks);
        } else {
            ++summary.failed;
            log(LogLevel::Error, "[FAIL] %s (%d/%d checks failed)\n",
                test_case.name, stats.failed_checks, stats.checks);
        }
    }
    const int total_tests = summary.passed + summary.failed + summary.skipped;
    if (opts.show_run_report == ReportLevel::Show) {
        tfm::format(std::cout, "\n%d tests: %d passed, %d failed, %d skipped (%d checks)\n",
                    total_tests,
                    summary.passed, summary.failed, summary.skipped,
                    summary.total_checks);
    }
    /** When a filter matches nothing, emit the marker line ctest looks for
     * via SKIP_REGULAR_EXPRESSION so the test is reported as SKIP, not PASS. */
    if (!opts.filter.empty() && summary.passed + summary.failed == 0) {
        tfm::format(std::cout, "no test cases matching filter\n");
        return ExitStatus::Failure;
    }
    return summary.failed == 0 ? ExitStatus::Success : ExitStatus::Failure;
}

} // namespace framework

int main(int argc, char** argv)
{
    return ::framework::run(argc, argv);
}
