// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/check.h>
#include <util/result.h>

#include <boost/test/unit_test.hpp>

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

util::Result<void> VoidFailFn()
{
    return util::Error{Untranslated("void fail.")};
}

util::Result<int> IntFn(int i, bool success)
{
    if (success) return i;
    return util::Error{Untranslated(strprintf("int %i error.", i))};
}

util::Result<bilingual_str> StrFn(bilingual_str s, bool success)
{
    if (success) return s;
    return util::Error{strprintf(Untranslated("str %s error."), s.original)};
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

util::Result<int, FnError> IntFailFn(int i, bool success)
{
    if (success) return i;
    return {util::Error{Untranslated(strprintf("int %i error.", i))}, i % 2 ? ERR1 : ERR2};
}

util::Result<NoCopyNoMove, FnError> EnumFailFn(FnError ret)
{
    return {util::Error{Untranslated("enum fail.")}, ret};
}

util::Result<int, int> TruthyFalsyFn(int i, bool success)
{
    if (success) return i;
    return {util::Error{Untranslated(strprintf("failure value %i.", i))}, i};
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
    BOOST_CHECK_EQUAL(result.value(), T{std::forward<Args>(args)...});
    BOOST_CHECK_EQUAL(&result.value(), &*result);
}

template <typename T, typename F, typename... Args>
void ExpectFail(const util::Result<T, F>& result, bilingual_str str, Args&&... args)
{
    ExpectResult(result, false, str);
    BOOST_CHECK_EQUAL(result.GetFailure(), F{std::forward<Args>(args)...});
}

BOOST_AUTO_TEST_CASE(check_returned)
{
    ExpectResult(VoidSuccessFn(), true, {});
    ExpectResult(VoidFailFn(), false, Untranslated("void fail."));
    ExpectSuccess(IntFn(5, true), {}, 5);
    ExpectResult(IntFn(5, false), false, Untranslated("int 5 error."));
    ExpectSuccess(NoCopyFn(5, true), {}, 5);
    ExpectFail(NoCopyFn(5, false), Untranslated("nocopy 5 error."), 5);
    ExpectSuccess(NoCopyNoMoveFn(5, true), {}, 5);
    ExpectFail(NoCopyNoMoveFn(5, false), Untranslated("nocopynomove 5 error."), 5);
    ExpectSuccess(StrFn(Untranslated("S"), true), {}, Untranslated("S"));
    ExpectResult(StrFn(Untranslated("S"), false), false, Untranslated("str S error."));
    ExpectFail(EnumFailFn(ERR2), Untranslated("enum fail."), ERR2);
    ExpectSuccess(TruthyFalsyFn(0, true), {}, 0);
    ExpectFail(TruthyFalsyFn(0, false), Untranslated("failure value 0."), 0);
    ExpectSuccess(TruthyFalsyFn(1, true), {}, 1);
    ExpectFail(TruthyFalsyFn(1, false), Untranslated("failure value 1."), 1);
}

BOOST_AUTO_TEST_CASE(check_set)
{
    // Test using Set method to change a result value from success -> failure,
    // and failure->success.
    util::Result<int, FnError> result;
    ExpectSuccess(result, {}, 0);
    result.Set({util::Error{Untranslated("error")}, ERR1});
    ExpectFail(result, Untranslated("error"), ERR1);
    result.Set(2);
    ExpectSuccess(result, Untranslated(""), 2);

    // Test the same thing but with non-copyable success and failure types.
    util::Result<NoCopy, NoCopy> result2{0};
    ExpectSuccess(result2, {}, 0);
    result2.Set({util::Error{Untranslated("error")}, 3});
    ExpectFail(result2, Untranslated("error"), 3);
    result2.Set(4);
    ExpectSuccess(result2, Untranslated(""), 4);
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
