// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>
#include <common/setting.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>
#include <string>
#include <vector>

using common::Disabled;
using common::Setting;
using common::SettingOptions;
using common::SettingsValue;
using common::Unset;

template<typename T>
class SettingTest
{
public:
    using S = Setting<"-s", T, SettingOptions{.legacy = true}, "">;

    SettingTest() {
        S::Register(m_args);
    }

    SettingTest& AddArg(std::string arg) {
        m_argv.push_back(std::move(arg));
        return *this;
    }

    SettingTest& Parse() {
        std::vector<const char*> argv;
        argv.reserve(m_argv.size());
        for (const auto& arg : m_argv) argv.push_back(arg.c_str());
        std::string error;
        bool result = m_args.ParseParameters(argv.size(), argv.data(), error);
        BOOST_CHECK_EQUAL(result, true);
        BOOST_CHECK_EQUAL(error, "");
        return *this;
    }

    T Get() {
        return S::Get(m_args);
    }

    SettingsValue Value() {
        return S::Value(m_args);
    }

private:
    ArgsManager m_args;
    std::vector<std::string> m_argv{"unused"};
};

BOOST_FIXTURE_TEST_SUITE(setting_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(GetOptional)
{
    BOOST_CHECK_EQUAL(SettingTest<std::optional<int>>{}.Parse().Get(), std::nullopt);
    BOOST_CHECK_EQUAL(SettingTest<std::optional<int>>{}.AddArg("-s=3").Parse().Get(), 3);
    BOOST_CHECK_EQUAL(SettingTest<std::optional<int>>{}.AddArg("-s=3").Parse().Value().write(), "\"3\"");
    BOOST_CHECK_EQUAL(SettingTest<std::optional<int>>{}.AddArg("-nos=1").Parse().Get(), 0);
    BOOST_CHECK_EQUAL(SettingTest<std::optional<int>>{}.AddArg("-nos").Parse().Get(), 0);

    BOOST_CHECK(!SettingTest<std::optional<Disabled>>{}.Parse().Get());
    BOOST_CHECK(!SettingTest<std::optional<Disabled>>{}.AddArg("-s=3").Parse().Get());
    BOOST_CHECK(SettingTest<std::optional<Disabled>>{}.AddArg("-nos=1").Parse().Get());
    BOOST_CHECK(SettingTest<std::optional<Disabled>>{}.AddArg("-nos").Parse().Get());

    BOOST_CHECK(SettingTest<std::optional<Unset>>{}.Parse().Get());
    BOOST_CHECK(!SettingTest<std::optional<Unset>>{}.AddArg("-s=3").Parse().Get());
    BOOST_CHECK(!SettingTest<std::optional<Unset>>{}.AddArg("-nos=1").Parse().Get());
    BOOST_CHECK(!SettingTest<std::optional<Unset>>{}.AddArg("-nos").Parse().Get());
}

BOOST_AUTO_TEST_SUITE_END()
