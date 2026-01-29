// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <tinyformat.h>
#include <util/check.h>
#include <util/result.h>
#include <util/translation.h>

#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

inline bool operator==(const bilingual_str& a, const bilingual_str& b)
{
    return a.original == b.original && a.translated == b.translated;
}

inline std::ostream& operator<<(std::ostream& os, const bilingual_str& s)
{
    return os << "bilingual_str('" << s.original << "' , '" << s.translated << "')";
}

BOOST_AUTO_TEST_SUITE(result_tests)

struct NoCopy {
    NoCopy(int n) : m_n{std::make_unique<int>(n)} {}
    std::unique_ptr<int> m_n;
};

bool operator==(const NoCopy& a, const NoCopy& b)
{
    return *a.m_n == *b.m_n;
}

std::ostream& operator<<(std::ostream& os, const NoCopy& o)
{
    return os << "NoCopy(" << *o.m_n << ")";
}

struct NoCopyNoMove {
    NoCopyNoMove(int n) : m_n{n} {}
    NoCopyNoMove(const NoCopyNoMove&) = delete;
    NoCopyNoMove(NoCopyNoMove&&) = delete;
    int m_n;
};

bool operator==(const NoCopyNoMove& a, const NoCopyNoMove& b)
{
    return a.m_n == b.m_n;
}

std::ostream& operator<<(std::ostream& os, const NoCopyNoMove& o)
{
    os << "NoCopyNoMove(" << o.m_n << ")";
    return os;
}

util::Result<void> VoidSuccessFn()
{
    return {};
}

util::Result<void> VoidErrorFn()
{
    return util::Error{Untranslated("void error.")};
}

util::Result<int> IntFn(int i, bool success)
{
    if (success) return i;
    return util::Error{Untranslated(strprintf("int %i error.", i))};
}

util::Result<bilingual_str> StrFn(bilingual_str s, bool success)
{
    if (success) return s;
    return util::Error{Untranslated(strprintf("str %s error.", s.original))};
}

util::Result<NoCopy, NoCopy> NoCopyFn(int i, bool success)
{
    if (success) return {i};
    return {util::Error{Untranslated(strprintf("nocopy %i error.", i))}, i};
}

util::Result<NoCopyNoMove, NoCopyNoMove> NoCopyNoMoveFn(int i, bool success)
{
    if (success) return {i};
    return {util::Error{Untranslated(strprintf("nocopynomove %i error.", i))}, i};
}

enum FnError { ERR1, ERR2 };

util::Result<int, FnError> IntErrorFn(int i, bool success)
{
    if (success) return {util::Warning{Untranslated(strprintf("int %i warn.", i))}, i};
    return {util::Error{Untranslated(strprintf("int %i error.", i))}, i % 2 ? ERR1 : ERR2};
}

util::Result<std::string, FnError> StrErrorFn(int i, bool success)
{
    util::Result<std::string, FnError> result;
    if (auto int_result{IntErrorFn(i, success) >> result}) {
        result.update(strprintf("%i", *int_result));
    } else {
        result.update({util::Error{Untranslated("str error")}, int_result.error()});
    }
    return result;
}

util::Result<NoCopyNoMove, FnError> EnumErrorFn(FnError ret)
{
    return {util::Error{Untranslated("enum error.")}, ret};
}

util::Result<void> WarnFn()
{
    return {util::Warning{Untranslated("warn.")}};
}

util::Result<int> MultiWarnFn(int ret)
{
    util::Result<int> result;
    for (int i = 0; i < ret; ++i) {
        result.messages().warnings.push_back(Untranslated(strprintf("warn %i.", i)));
    }
    result.update(ret);
    return result;
}

util::Result<void, int> ChainedErrorFn(FnError arg, int ret)
{
    util::Result<void, int> result{util::Error{Untranslated("chained error.")}, ret};
    EnumErrorFn(arg) >> result;
    WarnFn() >> result;
    return result;
}

util::Result<int, FnError> AccumulateFn(bool success)
{
    util::Result<int, FnError> result;
    util::Result<int> x = MultiWarnFn(1) >> result;
    BOOST_REQUIRE(x);
    util::Result<int> y = MultiWarnFn(2) >> result;
    BOOST_REQUIRE(y);
    result.update(IntErrorFn(*x + *y, success));
    return result;
}

util::Result<int, int> TruthyFalsyFn(int i, bool success)
{
    if (success) return i;
    return {util::Error{Untranslated(strprintf("error value %i.", i))}, i};
}

template <typename T, typename F>
void ExpectResult(const util::Result<T, F>& result, bool success, const bilingual_str& str)
{
    BOOST_CHECK_EQUAL(bool(result), success);
    BOOST_CHECK_EQUAL(util::ErrorString(result), str);
}

template <typename T, typename F, typename... Args>
void ExpectSuccess(const util::Result<T, F>& result, const bilingual_str& str, Args&&... args)
{
    ExpectResult(result, true, str);
    BOOST_CHECK_EQUAL(result.has_value(), true);
    T expected{std::forward<Args>(args)...};
    BOOST_CHECK_EQUAL(result.value(), expected);
    BOOST_CHECK_EQUAL(&result.value(), &*result);
}

template <typename T, typename F, typename... Args>
void ExpectError(const util::Result<T, F>& result, bilingual_str str, Args&&... args)
{
    ExpectResult(result, false, str);
    F expect_error{std::forward<Args>(args)...};
    BOOST_CHECK_EQUAL(result.error(), expect_error);
}

BOOST_AUTO_TEST_CASE(check_sizes)
{
    static_assert(sizeof(util::Result<int>) == sizeof(void*)*2);
    static_assert(sizeof(util::Result<void>) == sizeof(void*));
}

BOOST_AUTO_TEST_CASE(check_returned)
{
    ExpectResult(VoidSuccessFn(), true, {});
    ExpectResult(VoidErrorFn(), false, Untranslated("void error."));
    ExpectSuccess(IntFn(5, true), {}, 5);
    ExpectResult(IntFn(5, false), false, Untranslated("int 5 error."));
    ExpectSuccess(NoCopyFn(5, true), {}, 5);
    ExpectError(NoCopyFn(5, false), Untranslated("nocopy 5 error."), 5);
    ExpectSuccess(NoCopyNoMoveFn(5, true), {}, 5);
    ExpectError(NoCopyNoMoveFn(5, false), Untranslated("nocopynomove 5 error."), 5);
    ExpectSuccess(StrFn(Untranslated("S"), true), {}, Untranslated("S"));
    ExpectResult(StrFn(Untranslated("S"), false), false, Untranslated("str S error."));
    ExpectSuccess(StrErrorFn(1, true), Untranslated("int 1 warn."), "1");
    ExpectError(StrErrorFn(2, false), Untranslated("int 2 error. str error"), ERR2);
    ExpectError(EnumErrorFn(ERR2), Untranslated("enum error."), ERR2);
    ExpectError(ChainedErrorFn(ERR1, 5), Untranslated("chained error. enum error. warn."), 5);
    ExpectSuccess(MultiWarnFn(3), Untranslated("warn 0. warn 1. warn 2."), 3);
    ExpectSuccess(AccumulateFn(true), Untranslated("warn 0. warn 0. warn 1. int 3 warn."), 3);
    ExpectError(AccumulateFn(false), Untranslated("int 3 error. warn 0. warn 0. warn 1."), ERR1);
    ExpectSuccess(TruthyFalsyFn(0, true), {}, 0);
    ExpectError(TruthyFalsyFn(0, false), Untranslated("error value 0."), 0);
    ExpectSuccess(TruthyFalsyFn(1, true), {}, 1);
    ExpectError(TruthyFalsyFn(1, false), Untranslated("error value 1."), 1);
}

BOOST_AUTO_TEST_CASE(check_update)
{
    // Test using Update method to change a result value from success -> error,
    // and error->success.
    util::Result<int, FnError> result;
    ExpectSuccess(result, {}, 0);
    result.update({util::Error{Untranslated("error")}, ERR1});
    ExpectError(result, Untranslated("error"), ERR1);
    result.update(2);
    ExpectSuccess(result, Untranslated("error"), 2);

    // Test the same thing but with non-copyable success and error types.
    util::Result<NoCopy, NoCopy> result2{0};
    ExpectSuccess(result2, {}, 0);
    result2.update({util::Error{Untranslated("error")}, 3});
    ExpectError(result2, Untranslated("error"), 3);
    result2.update(4);
    ExpectSuccess(result2, Untranslated("error"), 4);
}

BOOST_AUTO_TEST_CASE(check_dereference_operators)
{
    util::Result<std::pair<int, std::string>> mutable_result;
    const auto& const_result{mutable_result};
    mutable_result.value() = {1, "23"};
    BOOST_CHECK_EQUAL(mutable_result->first, 1);
    BOOST_CHECK_EQUAL(const_result->second, "23");
    (*mutable_result).first = 5;
    BOOST_CHECK_EQUAL((*const_result).first, 5);
}

BOOST_AUTO_TEST_CASE(check_value_or)
{
    BOOST_CHECK_EQUAL(IntFn(10, true).value_or(20), 10);
    BOOST_CHECK_EQUAL(IntFn(10, false).value_or(20), 20);
    BOOST_CHECK_EQUAL(NoCopyFn(10, true).value_or(20), 10);
    BOOST_CHECK_EQUAL(NoCopyFn(10, false).value_or(20), 20);
    BOOST_CHECK_EQUAL(StrFn(Untranslated("A"), true).value_or(Untranslated("B")), Untranslated("A"));
    BOOST_CHECK_EQUAL(StrFn(Untranslated("A"), false).value_or(Untranslated("B")), Untranslated("B"));
}

BOOST_AUTO_TEST_CASE(check_message_accessors)
{
    util::Result<void> result{util::Error{Untranslated("Error.")}, util::Warning{Untranslated("Warning.")}};
    BOOST_CHECK_EQUAL(Assert(result.messages_ptr())->errors.size(), 1);
    BOOST_CHECK_EQUAL(Assert(result.messages_ptr())->errors[0], Untranslated("Error."));
    BOOST_CHECK_EQUAL(Assert(result.messages_ptr())->warnings.size(), 1);
    BOOST_CHECK_EQUAL(Assert(result.messages_ptr())->warnings[0], Untranslated("Warning."));
    BOOST_CHECK_EQUAL(util::ErrorString(result), Untranslated("Error. Warning."));
}

struct Derived : NoCopyNoMove {
    using NoCopyNoMove::NoCopyNoMove;
};

util::Result<std::unique_ptr<NoCopyNoMove>> DerivedToBaseFn(util::Result<std::unique_ptr<Derived>> derived)
{
    return derived;
}

BOOST_AUTO_TEST_CASE(derived_to_base)
{
    BOOST_CHECK_EQUAL(*Assert(*Assert(DerivedToBaseFn(std::make_unique<Derived>(5)))), 5);
}

BOOST_AUTO_TEST_SUITE_END()
