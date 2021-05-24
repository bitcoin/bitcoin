// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/settings.h>

#include <test/util/setup_common.h>
#include <test/util/str.h>


#include <boost/test/unit_test.hpp>
#include <univalue.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/system.h>
#include <vector>

inline bool operator==(const util::SettingsValue& a, const util::SettingsValue& b)
{
    return a.write() == b.write();
}

inline std::ostream& operator<<(std::ostream& os, const util::SettingsValue& value)
{
    os << value.write();
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const std::pair<std::string, util::SettingsValue>& kv)
{
    util::SettingsValue out(util::SettingsValue::VOBJ);
    out.__pushKV(kv.first, kv.second);
    os << out.write();
    return os;
}

inline void WriteText(const fs::path& path, const std::string& text)
{
    fsbridge::ofstream file;
    file.open(path);
    file << text;
}

BOOST_FIXTURE_TEST_SUITE(settings_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(ReadWrite)
{
    fs::path path = m_args.GetDataDirBase() / "settings.json";

    WriteText(path, R"({
        "string": "string",
        "num": 5,
        "bool": true,
        "null": null
    })");

    std::map<std::string, util::SettingsValue> expected{
        {"string", "string"},
        {"num", 5},
        {"bool", true},
        {"null", {}},
    };

    // Check file read.
    std::map<std::string, util::SettingsValue> values;
    std::vector<std::string> errors;
    BOOST_CHECK(util::ReadSettings(path, values, errors));
    BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), expected.begin(), expected.end());
    BOOST_CHECK(errors.empty());

    // Check no errors if file doesn't exist.
    fs::remove(path);
    BOOST_CHECK(util::ReadSettings(path, values, errors));
    BOOST_CHECK(values.empty());
    BOOST_CHECK(errors.empty());

    // Check duplicate keys not allowed
    WriteText(path, R"({
        "dupe": "string",
        "dupe": "dupe"
    })");
    BOOST_CHECK(!util::ReadSettings(path, values, errors));
    std::vector<std::string> dup_keys = {strprintf("Found duplicate key dupe in settings file %s", path.string())};
    BOOST_CHECK_EQUAL_COLLECTIONS(errors.begin(), errors.end(), dup_keys.begin(), dup_keys.end());

    // Check non-kv json files not allowed
    WriteText(path, R"("non-kv")");
    BOOST_CHECK(!util::ReadSettings(path, values, errors));
    std::vector<std::string> non_kv = {strprintf("Found non-object value \"non-kv\" in settings file %s", path.string())};
    BOOST_CHECK_EQUAL_COLLECTIONS(errors.begin(), errors.end(), non_kv.begin(), non_kv.end());

    // Check invalid json not allowed
    WriteText(path, R"(invalid json)");
    BOOST_CHECK(!util::ReadSettings(path, values, errors));
    std::vector<std::string> fail_parse = {strprintf("Unable to parse settings file %s", path.string())};
    BOOST_CHECK_EQUAL_COLLECTIONS(errors.begin(), errors.end(), fail_parse.begin(), fail_parse.end());
}

//! Check settings struct contents against expected json strings.
static void CheckValues(const util::Settings& settings, const std::string& single_val, const std::string& list_val)
{
    util::SettingsValue single_value = GetSetting(settings, "section", "name", false, false);
    util::SettingsValue list_value(util::SettingsValue::VARR);
    for (const auto& item : GetSettingsList(settings, "section", "name", false)) {
        list_value.push_back(item);
    }
    BOOST_CHECK_EQUAL(single_value.write().c_str(), single_val);
    BOOST_CHECK_EQUAL(list_value.write().c_str(), list_val);
};

// Simple settings merge test case.
BOOST_AUTO_TEST_CASE(Simple)
{
    util::Settings settings;
    settings.command_line_options["name"].push_back("val1");
    settings.command_line_options["name"].push_back("val2");
    settings.ro_config["section"]["name"].push_back(2);

    // The last given arg takes precedence when specified via commandline.
    CheckValues(settings, R"("val2")", R"(["val1","val2",2])");

    util::Settings settings2;
    settings2.ro_config["section"]["name"].push_back("val2");
    settings2.ro_config["section"]["name"].push_back("val3");

    // The first given arg takes precedence when specified via config file.
    CheckValues(settings2, R"("val2")", R"(["val2","val3"])");
}

// Confirm that a high priority setting overrides a lower priority setting even
// if the high priority setting is null. This behavior is useful for a high
// priority setting source to be able to effectively reset any setting back to
// its default value.
BOOST_AUTO_TEST_CASE(NullOverride)
{
    util::Settings settings;
    settings.command_line_options["name"].push_back("value");
    BOOST_CHECK_EQUAL(R"("value")", GetSetting(settings, "section", "name", false, false).write().c_str());
    settings.forced_settings["name"] = {};
    BOOST_CHECK_EQUAL(R"(null)", GetSetting(settings, "section", "name", false, false).write().c_str());
}

// Test different ways settings can be merged, and verify results. This test can
// be used to confirm that updates to settings code don't change behavior
// unintentionally.
struct MergeTestingSetup : public BasicTestingSetup {
    //! Max number of actions to sequence together. Can decrease this when
    //! debugging to make test results easier to understand.
    static constexpr int MAX_ACTIONS = 3;

    enum Action { END, SET, NEGATE, SECTION_SET, SECTION_NEGATE };
    using ActionList = Action[MAX_ACTIONS];

    //! Enumerate all possible test configurations.
    template <typename Fn>
    void ForEachMergeSetup(Fn&& fn)
    {
        ActionList arg_actions = {};
        // command_line_options do not have sections. Only iterate over SET and NEGATE
        ForEachNoDup(arg_actions, SET, NEGATE, [&]{
            ActionList conf_actions = {};
            ForEachNoDup(conf_actions, SET, SECTION_NEGATE, [&]{
                for (bool force_set : {false, true}) {
                    for (bool ignore_default_section_config : {false, true}) {
                        fn(arg_actions, conf_actions, force_set, ignore_default_section_config);
                    }
                }
            });
        });
    }
};

// Regression test covering different ways config settings can be merged. The
// test parses and merges settings, representing the results as strings that get
// compared against an expected hash. To debug, the result strings can be dumped
// to a file (see comments below).
BOOST_FIXTURE_TEST_CASE(Merge, MergeTestingSetup)
{
    CHash256 out_sha;
    FILE* out_file = nullptr;
    if (const char* out_path = getenv("SETTINGS_MERGE_TEST_OUT")) {
        out_file = fsbridge::fopen(out_path, "w");
        if (!out_file) throw std::system_error(errno, std::generic_category(), "fopen failed");
    }

    const std::string& network = CBaseChainParams::MAIN;
    ForEachMergeSetup([&](const ActionList& arg_actions, const ActionList& conf_actions, bool force_set,
                          bool ignore_default_section_config) {
        std::string desc;
        int value_suffix = 0;
        util::Settings settings;

        const std::string& name = ignore_default_section_config ? "wallet" : "server";
        auto push_values = [&](Action action, const char* value_prefix, const std::string& name_prefix,
                               std::vector<util::SettingsValue>& dest) {
            if (action == SET || action == SECTION_SET) {
                for (int i = 0; i < 2; ++i) {
                    dest.push_back(value_prefix + ToString(++value_suffix));
                    desc += " " + name_prefix + name + "=" + dest.back().get_str();
                }
            } else if (action == NEGATE || action == SECTION_NEGATE) {
                dest.push_back(false);
                desc += " " + name_prefix + "no" + name;
            }
        };

        if (force_set) {
            settings.forced_settings[name] = "forced";
            desc += " " + name + "=forced";
        }
        for (Action arg_action : arg_actions) {
            push_values(arg_action, "a", "-", settings.command_line_options[name]);
        }
        for (Action conf_action : conf_actions) {
            bool use_section = conf_action == SECTION_SET || conf_action == SECTION_NEGATE;
            push_values(conf_action, "c", use_section ? network + "." : "",
                settings.ro_config[use_section ? network : ""][name]);
        }

        desc += " || ";
        desc += GetSetting(settings, network, name, ignore_default_section_config, /* get_chain_name= */ false).write();
        desc += " |";
        for (const auto& s : GetSettingsList(settings, network, name, ignore_default_section_config)) {
            desc += " ";
            desc += s.write();
        }
        desc += " |";
        if (OnlyHasDefaultSectionSetting(settings, network, name)) desc += " ignored";
        desc += "\n";

        out_sha.Write(MakeUCharSpan(desc));
        if (out_file) {
            BOOST_REQUIRE(fwrite(desc.data(), 1, desc.size(), out_file) == desc.size());
        }
    });

    if (out_file) {
        if (fclose(out_file)) throw std::system_error(errno, std::generic_category(), "fclose failed");
        out_file = nullptr;
    }

    unsigned char out_sha_bytes[CSHA256::OUTPUT_SIZE];
    out_sha.Finalize(out_sha_bytes);
    std::string out_sha_hex = HexStr(out_sha_bytes);

    // If check below fails, should manually dump the results with:
    //
    //   SETTINGS_MERGE_TEST_OUT=results.txt ./test_syscoin --run_test=settings_tests/Merge
    //
    // And verify diff against previous results to make sure the changes are expected.
    //
    // Results file is formatted like:
    //
    //   <input> || GetSetting() | GetSettingsList() | OnlyHasDefaultSectionSetting()
    BOOST_CHECK_EQUAL(out_sha_hex, "79db02d74e3e193196541b67c068b40ebd0c124a24b3ecbe9cbf7e85b1c4ba7a");
}

BOOST_AUTO_TEST_SUITE_END()
