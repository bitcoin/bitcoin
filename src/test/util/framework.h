/** Copyright (c) 2026-present The Bitcoin Core developers
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php. */
#ifndef BITCOIN_TEST_UTIL_FRAMEWORK_H
#define BITCOIN_TEST_UTIL_FRAMEWORK_H
#pragma once

#include <tinyformat.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <optional>
#include <ostream>
#include <random>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace framework {

struct TestCase {
    const char* suite;
    const char* name;
    void (*fn)();
};

/** Construct-on-first-use list of test cases. Prevents use of a global variable before initialization. */
inline std::vector<TestCase>& registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

/** Construct-on-first-use suite name. */
inline const char*& current_test_suite()
{
    static const char* s = "";
    return s;
}

/** Place-holder fixture that is overridden by test suites. */
struct EmptyFixture {
};

/** Convenience struct for adding functions to the registry. */
struct Registrar {
    Registrar(const char* name, void (*fn)())
    {
        registry().emplace_back(TestCase{current_test_suite(), name, fn});
    }
};

/** Path that the test binary */
inline const char*& executable_path()
{
    static const char* prog = "";
    return prog;
}

/** Single test metadata. */
struct TestStats {
    int checks = 0;
    int failed_checks = 0;
};

struct RunSummary {
    int passed = 0;
    int failed = 0;
    int skipped = 0;
    int total_checks = 0;
};

enum class LogLevel {
    None = 0,
    Error = 1,
    Info = 2,
    TestSuite = 3,
    All = 4,
    Unknown = 99,
};

inline LogLevel& current_log_level()
{
    static LogLevel level = LogLevel::Error;
    return level;
}

inline std::mutex& log_mutex()
{
    static std::mutex mutex;
    return mutex;
}

template <typename... Args>
inline void log(LogLevel level, const char* fmt, Args&&... args)
{
    if (current_log_level() >= level) {
        std::scoped_lock lock{log_mutex()};
        tfm::vformat(std::cout, fmt, tfm::makeFormatList(std::forward<Args>(args)...));
    }
}

/** Types that implement the `<<` operator. Pointers are excluded: operator<< for
 * char-like pointers calls strlen on the pointee, which is unsafe for raw memory. */
template <typename T>
concept streamable = !std::is_pointer_v<T> && requires(std::ostream& os, const T& value) { os << value; };

/** Types that implement a `ToString()` member returning `std::string`. */
template <typename T>
concept has_to_string = requires(const T& value) {
    { value.ToString() } -> std::same_as<std::string>;
};

template <typename T>
concept is_cstring = std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, char*>;

template <typename T>
std::string stringify(const T& value)
{
    if constexpr (streamable<T>) {
        std::ostringstream os;
        os << value;
        return os.str();
    } else if constexpr (has_to_string<T>) {
        return value.ToString();
    } else {
        return typeid(T).name();
    }
}

inline std::string stringify(bool v) { return v ? "true" : "false"; }
inline std::string stringify(std::nullptr_t) { return "nullptr"; }
inline std::string stringify(const char* s)
{
    return s ? std::string("\"") + s + "\"" : std::string("nullptr");
}

/** `CHECK(opt)` checks the optional holds a value. */
template <typename T>
std::string stringify(const std::optional<T>& v)
{
    return v.has_value() ? std::string{"optional<"} + typeid(T).name() + ">" : "std::nullopt";
}

struct Result {
    std::optional<std::string> failed_expression;

    static Result ok()
    {
        return {std::nullopt};
    }

    static Result failed(std::string&& expression)
    {
        return {std::move(expression)};
    }

    bool is_ok() const noexcept
    {
        return !failed_expression.has_value();
    }
};

/** Single test context shared across a test and child threads of that test. */
class TestContext
{
private:
    std::atomic<int> m_checks{0};
    std::atomic<int> m_failed{0};
    std::atomic<bool> m_did_throw{false};
    const std::string m_full_name;
    const std::thread::id m_owner_thread{std::this_thread::get_id()};

public:
    explicit TestContext(std::string name);
    ~TestContext() noexcept;

    TestContext(const TestContext&) = delete;
    TestContext& operator=(const TestContext&) = delete;
    TestContext(TestContext&&) = delete;
    TestContext& operator=(TestContext&&) = delete;

    const std::string& full_name() const noexcept { return m_full_name; }

    bool on_owner_thread() const noexcept
    {
        return m_owner_thread == std::this_thread::get_id();
    }

    void apply_result(const Result& result) noexcept
    {
        m_checks.fetch_add(1, std::memory_order_relaxed);
        if (!result.is_ok()) {
            m_failed.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void exception_occurred()
    {
        m_did_throw.store(true, std::memory_order_relaxed);
    }

    bool passed() const
    {
        return !m_did_throw.load(std::memory_order_relaxed) && m_failed.load(std::memory_order_relaxed) == 0;
    }

    TestStats stats() const noexcept
    {
        return TestStats{m_checks.load(std::memory_order_relaxed), m_failed.load(std::memory_order_relaxed)};
    }
};

inline std::atomic<TestContext*>& test_context_store()
{
    static std::atomic<TestContext*> ctx{nullptr};
    return ctx;
}

inline TestContext::TestContext(std::string name) : m_full_name{std::move(name)}
{
    TestContext* ctx{nullptr};
    if (!test_context_store().compare_exchange_strong(ctx, this, std::memory_order_release, std::memory_order_relaxed)) {
        throw std::logic_error{"only one active test case is allowed per process."};
    }
}

inline TestContext::~TestContext() noexcept
{
    auto* prev{test_context_store().exchange(nullptr, std::memory_order_release)};
    assert(prev == this);
    (void)prev;
}

inline TestContext& test_context()
{
    if (auto* context{test_context_store().load(std::memory_order_acquire)}) return *context;
    throw std::logic_error{"test framework assertion used outside an active test case."};
}

/** Name of the test currently running, in `suite::case` form. */
inline std::string current_test_full_name()
{
    return test_context().full_name();
}


/** A captured expression is used to stringify the result before evaluation.
 * When an expression evaluates to `false`, the string is reported to the user. */
template <typename T>
struct CapturedExpression {
    const T& lhs;

    /** No right-hand-side exists to evaluate. */
    operator Result() const
    {
        return static_cast<bool>(lhs) ? Result::ok() : Result::failed(stringify(lhs));
    }

    template <typename U>
    auto operator&&(const U&) = delete;

    template <typename U>
    auto operator||(const U&) = delete;

#define DECOMPOSE_OP(op, cmp)                                                                                  \
    template <typename U>                                                                                      \
    Result operator op(const U& rhs) const                                                                     \
    {                                                                                                          \
        bool btc_test_result;                                                                                  \
        if constexpr (is_cstring<T> && is_cstring<U>) {                                                        \
            btc_test_result = (std::strcmp(lhs, rhs) op 0);                                                    \
        } else if constexpr (std::is_integral_v<T> && std::is_integral_v<U> &&                                 \
                             std::is_signed_v<T> != std::is_signed_v<U>) {                                     \
            btc_test_result = cmp(lhs, rhs);                                                                   \
        } else {                                                                                               \
            btc_test_result = static_cast<bool>(lhs op rhs);                                                   \
        }                                                                                                      \
        return btc_test_result ? Result::ok() : Result::failed(stringify(lhs) + " " #op " " + stringify(rhs)); \
    }

    DECOMPOSE_OP(==, std::cmp_equal)
    DECOMPOSE_OP(!=, std::cmp_not_equal)
    DECOMPOSE_OP(<, std::cmp_less)
    DECOMPOSE_OP(<=, std::cmp_less_equal)
    DECOMPOSE_OP(>, std::cmp_greater)
    DECOMPOSE_OP(>=, std::cmp_greater_equal)
#undef DECOMPOSE_OP
};

/** Decomposer overloads a high-precedence operator so the expression is captured before evalution. */
struct Decomposer {
    template <typename T>
    CapturedExpression<T> operator<=(const T& lhs) const
    {
        return {lhs};
    }
};

inline LogLevel to_log_level(std::string_view s)
{
    static const std::unordered_map<std::string_view, LogLevel> levels{
        {"none", LogLevel::None},
        {"error", LogLevel::Error},
        {"info", LogLevel::Info},
        {"test_suite", LogLevel::TestSuite},
        {"all", LogLevel::All},
    };
    if (auto it = levels.find(s); it != levels.end())
        return it->second;
    return LogLevel::Unknown;
}

inline void record_check(TestContext& ctx, const Result& result, const char* kind, const char* expr,
                         const char* file, int line, const std::string& message = {})
{
    ctx.apply_result(result);
    if (!result.is_ok()) {
        log(LogLevel::Error, "[FAIL]: %s:%d: %s(%s)\n%s\n%s\n", file, line, kind, expr, *result.failed_expression, message);
        return;
    }
    log(LogLevel::All, "[PASS]: %s:%d: %s(%s)\n", file, line, kind, expr);
}

template <std::ranges::range R1, std::ranges::range R2>
    requires std::equality_comparable_with<std::ranges::range_value_t<R1>, std::ranges::range_value_t<R2>>
Result check_equal_ranges(const R1& r1, const R2& r2)
{
    if (std::ranges::equal(r1, r2)) return Result::ok();

    using E = std::ranges::range_value_t<R1>;
    /** Byte-like types are printed as hex; everything else falls back to stringify. */
    auto fmt = [](const E& v) -> std::string {
        if constexpr (std::same_as<E, std::byte>) {
            return tfm::format("0x%02x", std::to_integer<unsigned>(v));
        } else if constexpr (std::same_as<E, unsigned char> || std::same_as<E, signed char>) {
            return tfm::format("0x%02x", static_cast<unsigned>(v) & 0xffU);
        } else {
            return stringify(v);
        }
    };

    std::size_t idx = 0;
    auto it1 = std::ranges::begin(r1);
    auto it2 = std::ranges::begin(r2);
    const auto end1 = std::ranges::end(r1);
    const auto end2 = std::ranges::end(r2);

    while (it1 != end1 && it2 != end2) {
        if (!(*it1 == *it2)) {
            return Result::failed("[index " + stringify(idx) + "]: " + fmt(*it1) + " != " + fmt(*it2));
        }
        ++it1;
        ++it2;
        ++idx;
    }
    if (it1 != end1 || it2 != end2) {
        const auto sz1 = idx + static_cast<std::size_t>(std::ranges::distance(it1, end1));
        const auto sz2 = idx + static_cast<std::size_t>(std::ranges::distance(it2, end2));
        return Result::failed("size mismatch: " + stringify(sz1) + " != " + stringify(sz2));
    }
    return Result::ok();
}

/** Thrown by REQUIRE on failure to abort the current test. */
struct RequireFailed {
};

inline std::string test_name(const TestCase& test_case)
{
    if (test_case.suite && test_case.suite[0] != '\0') {
        return std::string{test_case.suite} + "/" + test_case.name;
    }
    return test_case.name;
}

inline bool matches_filter(const TestCase& tc, std::string_view filter)
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

inline void list_tests(const std::string_view& filter = {})
{
    for (const auto& test_case : registry()) {
        if (matches_filter(test_case, filter)) {
            tfm::format(std::cout, "%s\n", test_name(test_case));
        }
    }
}

inline void print_usage(const std::string& prog)
{
    tfm::format(std::cout, "Usage: %s [--run_test|-t <filter[,filter...]>] [--log_level|-l <none|error|info|test_suite|all>] [--list_content] [-h|--help] [-- USER_ARGS...]\n"
                           "  filter: <suite> | <case> | <suite>/<case>\n",
                prog);
}

inline std::vector<const char*>& user_args()
{
    static std::vector<const char*> ua;
    return ua;
}

enum ExitStatus {
    Success = 0,
    Failure = 1,
    UnknownArgs = 2,
};

struct RuntimeOptions {
    const char* prog{nullptr};
    std::string_view filter;
    LogLevel log_level = LogLevel::Error;
    bool show_list{false};
    bool show_help{false};
    std::vector<const char*> passthrough;
};

inline ExitStatus parse_args(RuntimeOptions& opts, int argc, char** argv)
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
        tfm::format(std::cout, "Unknown argument: %s\n", argv[i]);
        opts.show_help = true;
        return ExitStatus::UnknownArgs;
    }

    return ExitStatus::Success;
}

inline int run(int argc, char** argv)
{
    RuntimeOptions opts{};
    auto status = parse_args(opts, argc, argv);
    executable_path() = opts.prog;
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
    // To avoid breaking user scripts unexpectedly, reuse boost naming here.
    if (std::getenv("BOOST_TEST_RANDOM")) {
        std::ranges::shuffle(registry(), std::mt19937{std::random_device{}()});
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
    tfm::format(std::cout, "\n%d tests: %d passed, %d failed, %d skipped (%d checks)\n",
                total_tests,
                summary.passed, summary.failed, summary.skipped,
                summary.total_checks);
    /** When a filter matches nothing, emit the marker line ctest looks for
     * via SKIP_REGULAR_EXPRESSION so the test is reported as SKIP, not PASS. */
    if (!opts.filter.empty() && summary.passed + summary.failed == 0) {
        tfm::format(std::cout, "no test cases matching filter\n");
        return ExitStatus::Failure;
    }
    return summary.failed == 0 ? ExitStatus::Success : ExitStatus::Failure;
}


} // namespace framework

using btc_suite_fixture = ::framework::EmptyFixture;

/** The `Decomposer` uses operator precedence to capture expressions, which
 * gives a warning that the `<=` will be evaluated before `==`. This is by design.
 */
#if defined(__GNUC__) || defined(__clang__)
#define BITCOIN_TEST_DIAG_PUSH     \
    _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wparentheses\"")

#define BITCOIN_TEST_DIAG_POP \
    _Pragma("GCC diagnostic pop")
#else
#define BITCOIN_TEST_DIAG_PUSH
#define BITCOIN_TEST_DIAG_POP
#endif

/** Without this internal concatenation logic, the preprocessor would evaluate
 * `example_test_fn` + `__LINE__` as `example_test_fn__LINE__` rather than the
 * actual line number. */
#define BITCOIN_TEST_CAT_EVAL(a, b) a##b
#define BITCOIN_TEST_CAT(a, b) BITCOIN_TEST_CAT_EVAL(a, b)

/** For the case with no fixtures, we forward declare the function, store the function pointer
 * in the registry, and populate the function with the user's behavior.
 *
 * Example:
 * TEST_CASE(my_test):
 *
 * static void btc_test_fn34();
 * static Registrar btc_test_reg{my_test, &btc_test_fn34};
 * static void btc_test_fn34()
 *
 * ... user defines behavior */
#define TEST_CASE(name)                                                                                                      \
    static void BITCOIN_TEST_CAT(btc_test_fn, __LINE__)();                                                                   \
    static ::framework::Registrar BITCOIN_TEST_CAT(btc_test_reg, __LINE__){#name, &BITCOIN_TEST_CAT(btc_test_fn, __LINE__)}; \
    static void BITCOIN_TEST_CAT(btc_test_fn, __LINE__)()


/** This is a similar mechanism as above, this time with a few extra steps.
 * The first anon namespace is so two translation units may declare a
 * `btc_test_fixture64` if they both happen to define tests at line 64.
 * The following namespace is to give a per-test isolation, so one translation
 * unit may have multiple fixture tests, each defining `Impl`. The `Impl`
 * inheritance from `Fixture` allows access to the members of `Fixture` from
 * the test case. Now the usual forward declaration of the test function
 * is implement with the `runner()` function. That runner is registered.
 * Finally the function body is defined by the user. */
#define FIXTURE_TEST_CASE(name, Fixture)                     \
    namespace {                                              \
    namespace BITCOIN_TEST_CAT(btc_test_fixture, __LINE__) { \
    struct Impl : Fixture {                                  \
        void btc_test_run();                                 \
    };                                                       \
    static void runner()                                     \
    {                                                        \
        Impl impl;                                           \
        impl.btc_test_run();                                 \
    }                                                        \
    static ::framework::Registrar reg{#name, &runner};       \
    }                                                        \
    }                                                        \
    void BITCOIN_TEST_CAT(btc_test_fixture, __LINE__)::Impl::btc_test_run()

/** Set the current test suite and override the `EmptyFixture`. Add a suite level namespace as well. */
#define TEST_SUITE_BEGIN(suite, ...)                   \
    namespace suite {                                  \
    __VA_OPT__(using btc_suite_fixture = __VA_ARGS__;) \
    static const char* BITCOIN_TEST_CAT(btc_test_suite, __LINE__) = (::framework::current_test_suite() = #suite);

/** Reset the test suite. */
#define TEST_SUITE_END()                                                                                      \
    static const char* BITCOIN_TEST_CAT(btc_test_suite, __LINE__) = (::framework::current_test_suite() = ""); \
    }

/** The `do... while` syntax is to allow users to write `CHECK(expr);`
 * in a `if` block, for instance. Otherwise, there would be a trailing
 * `;` after expansion. The compiler can optimize this behavior away.
 *
 * An optional stream-style failure message is supported as a second argument.
 *
 * Example: CHECK(a == b, "context: a=" << a << " b=" << b); */
#define CHECK(expr, ...)                                                                            \
    do {                                                                                            \
        auto& btc_test_ctx_{::framework::test_context()};                                           \
        BITCOIN_TEST_DIAG_PUSH                                                                      \
        ::framework::Result btc_test_res_ = ::framework::Decomposer{} <= expr;                      \
        BITCOIN_TEST_DIAG_POP                                                                       \
        std::ostringstream btc_test_os_;                                                            \
        __VA_OPT__(if (!btc_test_res_.is_ok()) btc_test_os_ << __VA_ARGS__;)                        \
        ::framework::record_check(btc_test_ctx_, btc_test_res_, "CHECK", #expr, __FILE__, __LINE__, \
                                  btc_test_res_.is_ok() ? std::string{} : btc_test_os_.str());      \
    } while (false)

/** Like CHECK, but aborts the current test on failure by throwing.
 * Subsequent checks in the test are skipped. Use CHECK from child threads,
 * because throwing in a child thread cannot abort the parent test.
 *
 * Accepts the same optional stream-style failure message as CHECK. */
#define REQUIRE(expr, ...)                                                                                 \
    do {                                                                                                   \
        auto& btc_test_ctx_{::framework::test_context()};                                                  \
        if (!btc_test_ctx_.on_owner_thread()) {                                                            \
            ::framework::Result btc_test_res_{::framework::Result::failed(                                 \
                "REQUIRE used from a child thread; use CHECK in child threads.")};                         \
            ::framework::record_check(btc_test_ctx_, btc_test_res_, "REQUIRE", #expr, __FILE__, __LINE__); \
        } else {                                                                                           \
            BITCOIN_TEST_DIAG_PUSH                                                                         \
            ::framework::Result btc_test_res_ = ::framework::Decomposer{} <= expr;                         \
            BITCOIN_TEST_DIAG_POP                                                                          \
            std::ostringstream btc_test_os_;                                                               \
            __VA_OPT__(if (!btc_test_res_.is_ok()) btc_test_os_ << __VA_ARGS__;)                           \
            ::framework::record_check(btc_test_ctx_, btc_test_res_, "REQUIRE", #expr, __FILE__, __LINE__,  \
                                      btc_test_res_.is_ok() ? std::string{} : btc_test_os_.str());         \
            if (!btc_test_res_.is_ok()) throw ::framework::RequireFailed{};                                \
        }                                                                                                  \
    } while (false)

/** Passes if `expr` throws any exception. */
#define CHECK_THROWS(expr)                                                      \
    do {                                                                        \
        auto& btc_test_ctx_{::framework::test_context()};                       \
        ::framework::Result btc_test_res_{::framework::Result::failed(          \
            "expression did not throw")};                                       \
        try {                                                                   \
            expr;                                                               \
        } catch (...) {                                                         \
            btc_test_res_ = ::framework::Result::ok();                          \
        }                                                                       \
        ::framework::record_check(btc_test_ctx_, btc_test_res_, "CHECK_THROWS", \
                                  #expr, __FILE__, __LINE__);                   \
    } while (false)

/** Passes if `expr` does NOT throw any exception. */
#define CHECK_NOTHROW(expr)                                                      \
    do {                                                                         \
        auto& btc_test_ctx_{::framework::test_context()};                        \
        ::framework::Result btc_test_res_{::framework::Result::ok()};            \
        try {                                                                    \
            expr;                                                                \
        } catch (...) {                                                          \
            btc_test_res_ = ::framework::Result::failed("unexpectedly threw");   \
        }                                                                        \
        ::framework::record_check(btc_test_ctx_, btc_test_res_, "CHECK_NOTHROW", \
                                  #expr, __FILE__, __LINE__);                    \
    } while (false)

/** Like CHECK_NOTHROW, but aborts the current test on failure. Use
 * CHECK_NOTHROW from child threads. */
#define REQUIRE_NOTHROW(expr)                                                                                      \
    do {                                                                                                           \
        auto& btc_test_ctx_{::framework::test_context()};                                                          \
        if (!btc_test_ctx_.on_owner_thread()) {                                                                    \
            ::framework::Result btc_test_res_{::framework::Result::failed(                                         \
                "REQUIRE_NOTHROW used from a child thread; use CHECK_NOTHROW in child threads.")};                 \
            ::framework::record_check(btc_test_ctx_, btc_test_res_, "REQUIRE_NOTHROW", #expr, __FILE__, __LINE__); \
        } else {                                                                                                   \
            ::framework::Result btc_test_res_{::framework::Result::ok()};                                          \
            try {                                                                                                  \
                expr;                                                                                              \
            } catch (...) {                                                                                        \
                btc_test_res_ = ::framework::Result::failed("unexpectedly threw");                                 \
            }                                                                                                      \
            ::framework::record_check(btc_test_ctx_, btc_test_res_, "REQUIRE_NOTHROW",                             \
                                      #expr, __FILE__, __LINE__);                                                  \
            if (!btc_test_res_.is_ok()) throw ::framework::RequireFailed{};                                        \
        }                                                                                                          \
    } while (false)

/** Passes only if `expr` throws an exception derived from `ExceptionType`. */
#define CHECK_THROWS_AS(expr, ExceptionType)                                                \
    do {                                                                                    \
        auto& btc_test_ctx_{::framework::test_context()};                                   \
        ::framework::Result btc_test_res_{::framework::Result::failed(                      \
            "expression did not throw")};                                                   \
        try {                                                                               \
            expr;                                                                           \
        } catch (ExceptionType const&) {                                                    \
            btc_test_res_ = ::framework::Result::ok();                                      \
        } catch (...) {                                                                     \
            btc_test_res_ = ::framework::Result::failed("threw unexpected exception type"); \
        }                                                                                   \
        ::framework::record_check(btc_test_ctx_, btc_test_res_, "CHECK_THROWS_AS",          \
                                  #expr, __FILE__, __LINE__);                               \
    } while (false)

/** Passes only if `expr` throws an exception derived from `ExceptionType`
 * and `Predicate(caught)` returns true. Useful for inspecting e.what() or
 * other fields of the caught exception.
 *
 * Example:
 *   CHECK_EXCEPTION(parse(input), ParseError,
 *                   [](const ParseError& e) { return e.line == 3; }); */
#define CHECK_EXCEPTION(expr, ExceptionType, Predicate)                                                                                      \
    do {                                                                                                                                     \
        auto& btc_test_ctx_{::framework::test_context()};                                                                                    \
        ::framework::Result btc_test_res_{::framework::Result::failed(                                                                       \
            "expression did not throw expected type")};                                                                                      \
        try {                                                                                                                                \
            expr;                                                                                                                            \
        } catch (ExceptionType const& e) {                                                                                                   \
            btc_test_res_ = Predicate(e) ? ::framework::Result::ok() : ::framework::Result::failed("threw expected type, predicate failed"); \
        } catch (...) {                                                                                                                      \
            btc_test_res_ = ::framework::Result::failed(                                                                                     \
                "threw unexpected exception type");                                                                                          \
        }                                                                                                                                    \
        ::framework::record_check(btc_test_ctx_, btc_test_res_, "CHECK_EXCEPTION",                                                           \
                                  #expr, __FILE__, __LINE__);                                                                                \
    } while (false)

/** Passes if two ranges contain equal elements. Accepts any two types that
 * satisfy std::ranges::range with comparable element types. */
#define CHECK_EQUAL_RANGES(a, b)                                                      \
    do {                                                                              \
        auto& btc_test_ctx_{::framework::test_context()};                             \
        ::framework::Result btc_test_res_{::framework::check_equal_ranges((a), (b))}; \
        ::framework::record_check(btc_test_ctx_, btc_test_res_, "CHECK_EQUAL_RANGES", \
                                  #a " == " #b, __FILE__, __LINE__);                  \
    } while (false)

/** Unconditionally records a failure with `msg`. Does not abort the test. */
#define RECORD_ERROR(msg)                                    \
    do {                                                     \
        auto& btc_test_ctx_{::framework::test_context()};    \
        std::ostringstream btc_test_os_;                     \
        btc_test_os_ << msg;                                 \
        ::framework::record_check(                           \
            btc_test_ctx_,                                   \
            ::framework::Result::failed(btc_test_os_.str()), \
            "RECORD_ERROR", "", __FILE__, __LINE__);         \
    } while (false)

/** Emits a diagnostic message when the log level is Info or higher.
 * Accepts stream-style expressions: TEST_MESSAGE("x=" << x); */
#define TEST_MESSAGE(msg)                                             \
    do {                                                              \
        std::ostringstream btc_test_os_;                              \
        btc_test_os_ << msg;                                          \
        ::framework::log(::framework::LogLevel::Info, "[INFO]: %s\n", \
                         btc_test_os_.str());                         \
    } while (false)

/** Emits a warning message when `expr` is false. Does not fail the test */
#define WARN_MESSAGE(expr, msg)                                            \
    do {                                                                   \
        if (!(expr)) {                                                     \
            std::ostringstream btc_test_os_;                               \
            btc_test_os_ << msg;                                           \
            ::framework::log(::framework::LogLevel::Error, "[WARN]: %s\n", \
                             btc_test_os_.str());                          \
        }                                                                  \
    } while (false)

#define BOOST_CHECK(expr) CHECK(expr)
#define BOOST_REQUIRE(expr) REQUIRE(expr)
#define BOOST_TEST(expr) CHECK(expr)
#define BOOST_TEST_REQUIRE(expr) REQUIRE(expr)
#define BOOST_CHECK_MESSAGE(expr, msg) CHECK(expr, msg)
#define BOOST_REQUIRE_MESSAGE(expr, msg) REQUIRE(expr, msg)
#define BOOST_CHECK_EQUAL(a, b) CHECK((a) == (b))
#define BOOST_CHECK_NE(a, b) CHECK((a) != (b))
#define BOOST_CHECK_LT(a, b) CHECK((a) < (b))
#define BOOST_CHECK_LE(a, b) CHECK((a) <= (b))
#define BOOST_CHECK_GT(a, b) CHECK((a) > (b))
#define BOOST_CHECK_GE(a, b) CHECK((a) >= (b))
#define BOOST_REQUIRE_EQUAL(a, b) REQUIRE((a) == (b))
#define BOOST_REQUIRE_NE(a, b) REQUIRE((a) != (b))
#define BOOST_REQUIRE_LT(a, b) REQUIRE((a) < (b))
#define BOOST_REQUIRE_LE(a, b) REQUIRE((a) <= (b))
#define BOOST_REQUIRE_GT(a, b) REQUIRE((a) > (b))
#define BOOST_REQUIRE_GE(a, b) REQUIRE((a) >= (b))
#define BOOST_CHECK_THROW(expr, T) CHECK_THROWS_AS(expr, T)
#define BOOST_CHECK_EXCEPTION(expr, T, P) CHECK_EXCEPTION(expr, T, P)
#define BOOST_CHECK_NO_THROW(expr) CHECK_NOTHROW(expr)
#define BOOST_REQUIRE_NO_THROW(expr) REQUIRE_NOTHROW(expr)
#define BOOST_ERROR(msg) RECORD_ERROR(msg)
#define BOOST_TEST_MESSAGE(msg) TEST_MESSAGE(msg)
#define BOOST_WARN_MESSAGE(expr, msg) WARN_MESSAGE(expr, msg)
#define BOOST_CHECK_EQUAL_COLLECTIONS(ab, ae, bb, be) \
    CHECK_EQUAL_RANGES(std::ranges::subrange((ab), (ae)), std::ranges::subrange((bb), (be)))
#define BOOST_AUTO_TEST_CASE(name) FIXTURE_TEST_CASE(name, btc_suite_fixture)
#define BOOST_FIXTURE_TEST_CASE(name, F) \
    using name = F;                      \
    FIXTURE_TEST_CASE(name, F)
#define BOOST_AUTO_TEST_SUITE(name) TEST_SUITE_BEGIN(name)
#define BOOST_FIXTURE_TEST_SUITE(name, F) TEST_SUITE_BEGIN(name, F)
#define BOOST_AUTO_TEST_SUITE_END() TEST_SUITE_END()

#ifdef BITCOIN_TEST_MAIN
int main(int argc, char** argv)
{
    return ::framework::run(argc, argv);
}
#endif
#endif // BITCOIN_TEST_UTIL_FRAMEWORK_H
