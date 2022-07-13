// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/result.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(result_tests)

util::Result<void> VoidSuccessFn()
{
    return {};
}

util::Result<void> VoidFailFn()
{
    return {util::Error{}, Untranslated("void fail")};
}

util::Result<int> IntSuccessFn(int ret)
{
    return {ret};
}

util::Result<int> IntFailFn()
{
    return {util::Error{}, Untranslated("int fail")};
}

enum FnStatus { SUCCESS, ERR1, ERR2 };

util::Result<FnStatus> StatusSuccessFn(FnStatus ret)
{
    return {ret};
}

util::Result<FnStatus> StatusFailFn(FnStatus ret)
{
    return {util::Error{}, Untranslated("status fail"), ret};
}

util::Result<int> ChainedFailFn(FnStatus arg, int ret)
{
    return {util::ErrorChain{}, Untranslated("chained fail"), StatusFailFn(arg), ret};
}

template<typename T>
void ExpectSuccess(const util::Result<T>& result)
{
    BOOST_CHECK(result);
}

template<typename T>
void ExpectFail(const util::Result<T>& result, bilingual_str str)
{
    BOOST_CHECK(!result);
    BOOST_CHECK_EQUAL(ErrorDescription(result).original, str.original);
    BOOST_CHECK_EQUAL(ErrorDescription(result).translated, str.translated);
}

template<typename T, typename... Args>
void ExpectValue(const util::Result<T>& result, bool has_value, Args&&... args)
{
    BOOST_CHECK_EQUAL(result.value(), T{std::forward<Args>(args)...});
    BOOST_CHECK_EQUAL(result.has_value(), has_value);
    BOOST_CHECK_EQUAL(&result.value(), &*result);
}

template<typename T, typename... Args>
void ExpectSuccessValue(const util::Result<T>& result, Args&&... args)
{
    ExpectSuccess(result);
    ExpectValue(result, true, std::forward<Args>(args)...);
}

template<typename T, typename... Args>
void ExpectFailValue(const util::Result<T>& result, bilingual_str str, Args&&... args)
{
    ExpectFail(result, str);
    ExpectValue(result, false, std::forward<Args>(args)...);
}

BOOST_AUTO_TEST_CASE(check_returned)
{
    ExpectSuccess(VoidSuccessFn());
    ExpectFail(VoidFailFn(), Untranslated("void fail"));
    ExpectSuccessValue(IntSuccessFn(5), 5);
    ExpectFail(IntFailFn(), Untranslated("int fail"));
    ExpectSuccessValue(StatusSuccessFn(SUCCESS), SUCCESS);
    ExpectFailValue(StatusFailFn(ERR2), Untranslated("status fail"), ERR2);
    ExpectFailValue(ChainedFailFn(ERR1, 5), Untranslated("status fail, chained fail"), 5);
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

BOOST_AUTO_TEST_SUITE_END()
