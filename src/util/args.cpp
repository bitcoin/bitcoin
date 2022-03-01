// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/args.h>

#include <logging.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <chainparamsbase.h>

#include <fstream>

/**
 * Interpret a string argument as a boolean.
 *
 * The definition of LocaleIndependentAtoi<int>() requires that non-numeric string values
 * like "foo", return 0. This means that if a user unintentionally supplies a
 * non-integer argument here, the return value is always false. This means that
 * -foo=false does what the user probably expects, but -foo=true is well defined
 * but does not do what they probably expected.
 *
 * The return value of LocaleIndependentAtoi<int>(...) is zero when given input not
 * representable as an int.
 *
 * For a more extensive discussion of this topic (and a wide range of opinions
 * on the Right Way to change this code), see PR12713.
 */
static bool InterpretBool(const std::string& strValue)
{
    if (strValue.empty())
        return true;
    return (LocaleIndependentAtoi<int>(strValue) != 0);
}

static std::string SettingName(const std::string& arg)
{
    return arg.size() > 0 && arg[0] == '-' ? arg.substr(1) : arg;
}

struct KeyInfo {
    std::string name;
    std::string section;
    bool negated{false};
};

/**
 * Parse "name", "section.name", "noname", "section.noname" settings keys.
 *
 * @note Where an option was negated can be later checked using the
 * IsArgNegated() method. One use case for this is to have a way to disable
 * options that are not normally boolean (e.g. using -nodebuglogfile to request
 * that debug log output is not sent to any file at all).
 */
KeyInfo InterpretKey(std::string key)
{
    KeyInfo result;
    // Split section name from key name for keys like "testnet.foo" or "regtest.bar"
    size_t option_index = key.find('.');
    if (option_index != std::string::npos) {
        result.section = key.substr(0, option_index);
        key.erase(0, option_index + 1);
    }
    if (key.substr(0, 2) == "no") {
        key.erase(0, 2);
        result.negated = true;
    }
    result.name = key;
    return result;
}

/**
 * Interpret settings value based on registered flags.
 *
 * @param[in]   key      key information to know if key was negated
 * @param[in]   value    string value of setting to be parsed
 * @param[in]   flags    ArgsManager registered argument flags
 * @param[out]  error    Error description if settings value is not valid
 *
 * @return parsed settings value if it is valid, otherwise nullopt accompanied
 * by a descriptive error string
 */
static std::optional<util::SettingsValue> InterpretValue(const KeyInfo& key, const std::string& value,
                                                         unsigned int flags, std::string& error)
{
    // Return negated settings as false values.
    if (key.negated) {
        if (flags & ArgsManager::DISALLOW_NEGATION) {
            error = strprintf("Negating of -%s is meaningless and therefore forbidden", key.name);
            return std::nullopt;
        }
        // Double negatives like -nofoo=0 are supported (but discouraged)
        if (!InterpretBool(value)) {
            LogPrintf("Warning: parsed potentially confusing double-negative -%s=%s\n", key.name, value);
            return true;
        }
        return false;
    }
    return value;
}

// Define default constructor and destructor that are not inline, so code instantiating this class doesn't need to
// #include class definitions for all members.
// For example, m_settings has an internal dependency on univalue.
ArgsManager::ArgsManager() {}
ArgsManager::~ArgsManager() {}

const std::set<std::string> ArgsManager::GetUnsuitableSectionOnlyArgs() const
{
    std::set<std::string> unsuitables;

    LOCK(cs_args);

    // if there's no section selected, don't worry
    if (m_network.empty()) return std::set<std::string> {};

    // if it's okay to use the default section for this network, don't worry
    if (m_network == CBaseChainParams::MAIN) return std::set<std::string> {};

    for (const auto& arg : m_network_only_args) {
        if (OnlyHasDefaultSectionSetting(m_settings, m_network, SettingName(arg))) {
            unsuitables.insert(arg);
        }
    }
    return unsuitables;
}

const std::list<SectionInfo> ArgsManager::GetUnrecognizedSections() const
{
    // Section names to be recognized in the config file.
    static const std::set<std::string> available_sections{
        CBaseChainParams::REGTEST,
        CBaseChainParams::SIGNET,
        CBaseChainParams::TESTNET,
        CBaseChainParams::MAIN
    };

    LOCK(cs_args);
    std::list<SectionInfo> unrecognized = m_config_sections;
    unrecognized.remove_if([](const SectionInfo& appeared){ return available_sections.find(appeared.m_name) != available_sections.end(); });
    return unrecognized;
}

void ArgsManager::SelectConfigNetwork(const std::string& network)
{
    LOCK(cs_args);
    m_network = network;
}

bool ArgsManager::ParseParameters(int argc, const char* const argv[], std::string& error)
{
    LOCK(cs_args);
    m_settings.command_line_options.clear();

    for (int i = 1; i < argc; i++) {
        std::string key(argv[i]);

#ifdef MAC_OSX
        // At the first time when a user gets the "App downloaded from the
        // internet" warning, and clicks the Open button, macOS passes
        // a unique process serial number (PSN) as -psn_... command-line
        // argument, which we filter out.
        if (key.substr(0, 5) == "-psn_") continue;
#endif

        if (key == "-") break; //bitcoin-tx using stdin
        std::string val;
        size_t is_index = key.find('=');
        if (is_index != std::string::npos) {
            val = key.substr(is_index + 1);
            key.erase(is_index);
        }
#ifdef WIN32
        key = ToLower(key);
        if (key[0] == '/')
            key[0] = '-';
#endif

        if (key[0] != '-') {
            if (!m_accept_any_command && m_command.empty()) {
                // The first non-dash arg is a registered command
                std::optional<unsigned int> flags = GetArgFlags(key);
                if (!flags || !(*flags & ArgsManager::COMMAND)) {
                    error = strprintf("Invalid command '%s'", argv[i]);
                    return false;
                }
            }
            m_command.push_back(key);
            while (++i < argc) {
                // The remaining args are command args
                m_command.push_back(argv[i]);
            }
            break;
        }

        // Transform --foo to -foo
        if (key.length() > 1 && key[1] == '-')
            key.erase(0, 1);

        // Transform -foo to foo
        key.erase(0, 1);
        KeyInfo keyinfo = InterpretKey(key);
        std::optional<unsigned int> flags = GetArgFlags('-' + keyinfo.name);

        // Unknown command line options and command line options with dot
        // characters (which are returned from InterpretKey with nonempty
        // section strings) are not valid.
        if (!flags || !keyinfo.section.empty()) {
            error = strprintf("Invalid parameter %s", argv[i]);
            return false;
        }

        std::optional<util::SettingsValue> value = InterpretValue(keyinfo, val, *flags, error);
        if (!value) return false;

        m_settings.command_line_options[keyinfo.name].push_back(*value);
    }

    // we do not allow -includeconf from command line, only -noincludeconf
    if (auto* includes = util::FindKey(m_settings.command_line_options, "includeconf")) {
        const util::SettingsSpan values{*includes};
        // Range may be empty if -noincludeconf was passed
        if (!values.empty()) {
            error = "-includeconf cannot be used from commandline; -includeconf=" + values.begin()->write();
            return false; // pick first value as example
        }
    }
    return true;
}

std::optional<unsigned int> ArgsManager::GetArgFlags(const std::string& name) const
{
    LOCK(cs_args);
    for (const auto& arg_map : m_available_args) {
        const auto search = arg_map.second.find(name);
        if (search != arg_map.second.end()) {
            return search->second.m_flags;
        }
    }
    return std::nullopt;
}

fs::path ArgsManager::GetPathArg(std::string arg, const fs::path& default_value) const
{
    if (IsArgNegated(arg)) return fs::path{};
    std::string path_str = GetArg(arg, "");
    if (path_str.empty()) return default_value;
    fs::path result = fs::PathFromString(path_str).lexically_normal();
    // Remove trailing slash, if present.
    return result.has_filename() ? result : result.parent_path();
}

const fs::path& ArgsManager::GetBlocksDirPath() const
{
    LOCK(cs_args);
    fs::path& path = m_cached_blocks_path;

    // Cache the path to avoid calling fs::create_directories on every call of
    // this function
    if (!path.empty()) return path;

    if (IsArgSet("-blocksdir")) {
        path = fs::absolute(GetPathArg("-blocksdir"));
        if (!fs::is_directory(path)) {
            path = "";
            return path;
        }
    } else {
        path = GetDataDirBase();
    }

    path /= fs::PathFromString(BaseParams().DataDir());
    path /= "blocks";
    fs::create_directories(path);
    return path;
}

const fs::path& ArgsManager::GetDataDir(bool net_specific) const
{
    LOCK(cs_args);
    fs::path& path = net_specific ? m_cached_network_datadir_path : m_cached_datadir_path;

    // Cache the path to avoid calling fs::create_directories on every call of
    // this function
    if (!path.empty()) return path;

    const fs::path datadir{GetPathArg("-datadir")};
    if (!datadir.empty()) {
        path = fs::absolute(datadir);
        if (!fs::is_directory(path)) {
            path = "";
            return path;
        }
    } else {
        path = GetDefaultDataDir();
    }

    if (!fs::exists(path)) {
        fs::create_directories(path / "wallets");
    }

    if (net_specific && !BaseParams().DataDir().empty()) {
        path /= fs::PathFromString(BaseParams().DataDir());
        if (!fs::exists(path)) {
            fs::create_directories(path / "wallets");
        }
    }

    return path;
}

void ArgsManager::ClearPathCache()
{
    LOCK(cs_args);

    m_cached_datadir_path = fs::path();
    m_cached_network_datadir_path = fs::path();
    m_cached_blocks_path = fs::path();
}

std::optional<const ArgsManager::Command> ArgsManager::GetCommand() const
{
    Command ret;
    LOCK(cs_args);
    auto it = m_command.begin();
    if (it == m_command.end()) {
        // No command was passed
        return std::nullopt;
    }
    if (!m_accept_any_command) {
        // The registered command
        ret.command = *(it++);
    }
    while (it != m_command.end()) {
        // The unregistered command and args (if any)
        ret.args.push_back(*(it++));
    }
    return ret;
}

std::vector<std::string> ArgsManager::GetArgs(const std::string& strArg) const
{
    std::vector<std::string> result;
    for (const util::SettingsValue& value : GetSettingsList(strArg)) {
        result.push_back(value.isFalse() ? "0" : value.isTrue() ? "1" : value.get_str());
    }
    return result;
}

bool ArgsManager::IsArgSet(const std::string& strArg) const
{
    return !GetSetting(strArg).isNull();
}

bool ArgsManager::InitSettings(std::string& error)
{
    if (!GetSettingsPath()) {
        return true; // Do nothing if settings file disabled.
    }

    std::vector<std::string> errors;
    if (!ReadSettingsFile(&errors)) {
        error = strprintf("Failed loading settings file:\n%s\n", MakeUnorderedList(errors));
        return false;
    }
    if (!WriteSettingsFile(&errors)) {
        error = strprintf("Failed saving settings file:\n%s\n", MakeUnorderedList(errors));
        return false;
    }
    return true;
}

bool ArgsManager::GetSettingsPath(fs::path* filepath, bool temp) const
{
    fs::path settings = GetPathArg("-settings", fs::path{BITCOIN_SETTINGS_FILENAME});
    if (settings.empty()) {
        return false;
    }
    if (filepath) {
        *filepath = fsbridge::AbsPathJoin(GetDataDirNet(), temp ? settings + ".tmp" : settings);
    }
    return true;
}

static void SaveErrors(const std::vector<std::string> errors, std::vector<std::string>* error_out)
{
    for (const auto& error : errors) {
        if (error_out) {
            error_out->emplace_back(error);
        } else {
            LogPrintf("%s\n", error);
        }
    }
}

bool ArgsManager::ReadSettingsFile(std::vector<std::string>* errors)
{
    fs::path path;
    if (!GetSettingsPath(&path, /* temp= */ false)) {
        return true; // Do nothing if settings file disabled.
    }

    LOCK(cs_args);
    m_settings.rw_settings.clear();
    std::vector<std::string> read_errors;
    if (!util::ReadSettings(path, m_settings.rw_settings, read_errors)) {
        SaveErrors(read_errors, errors);
        return false;
    }
    for (const auto& setting : m_settings.rw_settings) {
        KeyInfo key = InterpretKey(setting.first); // Split setting key into section and argname
        if (!GetArgFlags('-' + key.name)) {
            LogPrintf("Ignoring unknown rw_settings value %s\n", setting.first);
        }
    }
    return true;
}

bool ArgsManager::WriteSettingsFile(std::vector<std::string>* errors) const
{
    fs::path path, path_tmp;
    if (!GetSettingsPath(&path, /* temp= */ false) || !GetSettingsPath(&path_tmp, /* temp= */ true)) {
        throw std::logic_error("Attempt to write settings file when dynamic settings are disabled.");
    }

    LOCK(cs_args);
    std::vector<std::string> write_errors;
    if (!util::WriteSettings(path_tmp, m_settings.rw_settings, write_errors)) {
        SaveErrors(write_errors, errors);
        return false;
    }
    if (!RenameOver(path_tmp, path)) {
        SaveErrors({strprintf("Failed renaming settings file %s to %s\n", fs::PathToString(path_tmp), fs::PathToString(path))}, errors);
        return false;
    }
    return true;
}

bool ArgsManager::IsArgNegated(const std::string& strArg) const
{
    return GetSetting(strArg).isFalse();
}

std::string ArgsManager::GetArg(const std::string& strArg, const std::string& strDefault) const
{
    const util::SettingsValue value = GetSetting(strArg);
    return value.isNull() ? strDefault : value.isFalse() ? "0" : value.isTrue() ? "1" : value.isNum() ? value.getValStr() : value.get_str();
}

int64_t ArgsManager::GetIntArg(const std::string& strArg, int64_t nDefault) const
{
    const util::SettingsValue value = GetSetting(strArg);
    return value.isNull() ? nDefault : value.isFalse() ? 0 : value.isTrue() ? 1 : value.isNum() ? value.get_int64() : LocaleIndependentAtoi<int64_t>(value.get_str());
}

bool ArgsManager::GetBoolArg(const std::string& strArg, bool fDefault) const
{
    const util::SettingsValue value = GetSetting(strArg);
    return value.isNull() ? fDefault : value.isBool() ? value.get_bool() : InterpretBool(value.get_str());
}

bool ArgsManager::SoftSetArg(const std::string& strArg, const std::string& strValue)
{
    LOCK(cs_args);
    if (IsArgSet(strArg)) return false;
    ForceSetArg(strArg, strValue);
    return true;
}

bool ArgsManager::SoftSetBoolArg(const std::string& strArg, bool fValue)
{
    if (fValue)
        return SoftSetArg(strArg, std::string("1"));
    else
        return SoftSetArg(strArg, std::string("0"));
}

void ArgsManager::ForceSetArg(const std::string& strArg, const std::string& strValue)
{
    LOCK(cs_args);
    m_settings.forced_settings[SettingName(strArg)] = strValue;
}

void ArgsManager::AddCommand(const std::string& cmd, const std::string& help)
{
    Assert(cmd.find('=') == std::string::npos);
    Assert(cmd.at(0) != '-');

    LOCK(cs_args);
    m_accept_any_command = false; // latch to false
    std::map<std::string, Arg>& arg_map = m_available_args[OptionsCategory::COMMANDS];
    auto ret = arg_map.emplace(cmd, Arg{"", help, ArgsManager::COMMAND});
    Assert(ret.second); // Fail on duplicate commands
}

void ArgsManager::AddArg(const std::string& name, const std::string& help, unsigned int flags, const OptionsCategory& cat)
{
    Assert((flags & ArgsManager::COMMAND) == 0); // use AddCommand

    // Split arg name from its help param
    size_t eq_index = name.find('=');
    if (eq_index == std::string::npos) {
        eq_index = name.size();
    }
    std::string arg_name = name.substr(0, eq_index);

    LOCK(cs_args);
    std::map<std::string, Arg>& arg_map = m_available_args[cat];
    auto ret = arg_map.emplace(arg_name, Arg{name.substr(eq_index, name.size() - eq_index), help, flags});
    assert(ret.second); // Make sure an insertion actually happened

    if (flags & ArgsManager::NETWORK_ONLY) {
        m_network_only_args.emplace(arg_name);
    }
}

void ArgsManager::AddHiddenArgs(const std::vector<std::string>& names)
{
    for (const std::string& name : names) {
        AddArg(name, "", ArgsManager::ALLOW_ANY, OptionsCategory::HIDDEN);
    }
}

std::string ArgsManager::GetHelpMessage() const
{
    const bool show_debug = GetBoolArg("-help-debug", false);

    std::string usage = "";
    LOCK(cs_args);
    for (const auto& arg_map : m_available_args) {
        switch(arg_map.first) {
            case OptionsCategory::OPTIONS:
                usage += HelpMessageGroup("Options:");
                break;
            case OptionsCategory::CONNECTION:
                usage += HelpMessageGroup("Connection options:");
                break;
            case OptionsCategory::ZMQ:
                usage += HelpMessageGroup("ZeroMQ notification options:");
                break;
            case OptionsCategory::DEBUG_TEST:
                usage += HelpMessageGroup("Debugging/Testing options:");
                break;
            case OptionsCategory::NODE_RELAY:
                usage += HelpMessageGroup("Node relay options:");
                break;
            case OptionsCategory::BLOCK_CREATION:
                usage += HelpMessageGroup("Block creation options:");
                break;
            case OptionsCategory::RPC:
                usage += HelpMessageGroup("RPC server options:");
                break;
            case OptionsCategory::WALLET:
                usage += HelpMessageGroup("Wallet options:");
                break;
            case OptionsCategory::WALLET_DEBUG_TEST:
                if (show_debug) usage += HelpMessageGroup("Wallet debugging/testing options:");
                break;
            case OptionsCategory::CHAINPARAMS:
                usage += HelpMessageGroup("Chain selection options:");
                break;
            case OptionsCategory::GUI:
                usage += HelpMessageGroup("UI Options:");
                break;
            case OptionsCategory::COMMANDS:
                usage += HelpMessageGroup("Commands:");
                break;
            case OptionsCategory::REGISTER_COMMANDS:
                usage += HelpMessageGroup("Register Commands:");
                break;
            default:
                break;
        }

        // When we get to the hidden options, stop
        if (arg_map.first == OptionsCategory::HIDDEN) break;

        for (const auto& arg : arg_map.second) {
            if (show_debug || !(arg.second.m_flags & ArgsManager::DEBUG_ONLY)) {
                std::string name;
                if (arg.second.m_help_param.empty()) {
                    name = arg.first;
                } else {
                    name = arg.first + arg.second.m_help_param;
                }
                usage += HelpMessageOpt(name, arg.second.m_help_text);
            }
        }
    }
    return usage;
}

bool HelpRequested(const ArgsManager& args)
{
    return args.IsArgSet("-?") || args.IsArgSet("-h") || args.IsArgSet("-help") || args.IsArgSet("-help-debug");
}

void SetupHelpOptions(ArgsManager& args)
{
    args.AddArg("-?", "Print this help message and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    args.AddHiddenArgs({"-h", "-help"});
}

static const int screenWidth = 79;
static const int optIndent = 2;
static const int msgIndent = 7;

std::string HelpMessageGroup(const std::string &message) {
    return std::string(message) + std::string("\n\n");
}

std::string HelpMessageOpt(const std::string &option, const std::string &message) {
    return std::string(optIndent,' ') + std::string(option) +
           std::string("\n") + std::string(msgIndent,' ') +
           FormatParagraph(message, screenWidth - msgIndent, msgIndent) +
           std::string("\n\n");
}

bool CheckDataDirOption(const ArgsManager& args)
{
    const fs::path datadir{args.GetPathArg("-datadir")};
    return datadir.empty() || fs::is_directory(fs::absolute(datadir));
}

fs::path GetConfigFile(const ArgsManager& args, const std::string& confPath)
{
    return AbsPathForConfigVal(args, fs::PathFromString(confPath), false);
}

static bool GetConfigOptions(std::istream& stream, const std::string& filepath, std::string& error, std::vector<std::pair<std::string, std::string>>& options, std::list<SectionInfo>& sections)
{
    std::string str, prefix;
    std::string::size_type pos;
    int linenr = 1;
    while (std::getline(stream, str)) {
        bool used_hash = false;
        if ((pos = str.find('#')) != std::string::npos) {
            str = str.substr(0, pos);
            used_hash = true;
        }
        const static std::string pattern = " \t\r\n";
        str = TrimString(str, pattern);
        if (!str.empty()) {
            if (*str.begin() == '[' && *str.rbegin() == ']') {
                const std::string section = str.substr(1, str.size() - 2);
                sections.emplace_back(SectionInfo{section, filepath, linenr});
                prefix = section + '.';
            } else if (*str.begin() == '-') {
                error = strprintf("parse error on line %i: %s, options in configuration file must be specified without leading -", linenr, str);
                return false;
            } else if ((pos = str.find('=')) != std::string::npos) {
                std::string name = prefix + TrimString(str.substr(0, pos), pattern);
                std::string value = TrimString(str.substr(pos + 1), pattern);
                if (used_hash && name.find("rpcpassword") != std::string::npos) {
                    error = strprintf("parse error on line %i, using # in rpcpassword can be ambiguous and should be avoided", linenr);
                    return false;
                }
                options.emplace_back(name, value);
                if ((pos = name.rfind('.')) != std::string::npos && prefix.length() <= pos) {
                    sections.emplace_back(SectionInfo{name.substr(0, pos), filepath, linenr});
                }
            } else {
                error = strprintf("parse error on line %i: %s", linenr, str);
                if (str.size() >= 2 && str.substr(0, 2) == "no") {
                    error += strprintf(", if you intended to specify a negated option, use %s=1 instead", str);
                }
                return false;
            }
        }
        ++linenr;
    }
    return true;
}

bool ArgsManager::ReadConfigStream(std::istream& stream, const std::string& filepath, std::string& error, bool ignore_invalid_keys)
{
    LOCK(cs_args);
    std::vector<std::pair<std::string, std::string>> options;
    if (!GetConfigOptions(stream, filepath, error, options, m_config_sections)) {
        return false;
    }
    for (const std::pair<std::string, std::string>& option : options) {
        KeyInfo key = InterpretKey(option.first);
        std::optional<unsigned int> flags = GetArgFlags('-' + key.name);
        if (flags) {
            std::optional<util::SettingsValue> value = InterpretValue(key, option.second, *flags, error);
            if (!value) {
                return false;
            }
            m_settings.ro_config[key.section][key.name].push_back(*value);
        } else {
            if (ignore_invalid_keys) {
                LogPrintf("Ignoring unknown configuration value %s\n", option.first);
            } else {
                error = strprintf("Invalid configuration value %s", option.first);
                return false;
            }
        }
    }
    return true;
}

bool ArgsManager::ReadConfigFiles(std::string& error, bool ignore_invalid_keys)
{
    {
        LOCK(cs_args);
        m_settings.ro_config.clear();
        m_config_sections.clear();
    }

    const std::string confPath = GetArg("-conf", BITCOIN_CONF_FILENAME);
    std::ifstream stream{GetConfigFile(*this, confPath)};

    // not ok to have a config file specified that cannot be opened
    if (IsArgSet("-conf") && !stream.good()) {
        error = strprintf("specified config file \"%s\" could not be opened.", confPath);
        return false;
    }
    // ok to not have a config file
    if (stream.good()) {
        if (!ReadConfigStream(stream, confPath, error, ignore_invalid_keys)) {
            return false;
        }
        // `-includeconf` cannot be included in the command line arguments except
        // as `-noincludeconf` (which indicates that no included conf file should be used).
        bool use_conf_file{true};
        {
            LOCK(cs_args);
            if (auto* includes = util::FindKey(m_settings.command_line_options, "includeconf")) {
                // ParseParameters() fails if a non-negated -includeconf is passed on the command-line
                assert(util::SettingsSpan(*includes).last_negated());
                use_conf_file = false;
            }
        }
        if (use_conf_file) {
            std::string chain_id = GetChainName();
            std::vector<std::string> conf_file_names;

            auto add_includes = [&](const std::string& network, size_t skip = 0) {
                size_t num_values = 0;
                LOCK(cs_args);
                if (auto* section = util::FindKey(m_settings.ro_config, network)) {
                    if (auto* values = util::FindKey(*section, "includeconf")) {
                        for (size_t i = std::max(skip, util::SettingsSpan(*values).negated()); i < values->size(); ++i) {
                            conf_file_names.push_back((*values)[i].get_str());
                        }
                        num_values = values->size();
                    }
                }
                return num_values;
            };

            // We haven't set m_network yet (that happens in SelectParams()), so manually check
            // for network.includeconf args.
            const size_t chain_includes = add_includes(chain_id);
            const size_t default_includes = add_includes({});

            for (const std::string& conf_file_name : conf_file_names) {
                std::ifstream conf_file_stream{GetConfigFile(*this, conf_file_name)};
                if (conf_file_stream.good()) {
                    if (!ReadConfigStream(conf_file_stream, conf_file_name, error, ignore_invalid_keys)) {
                        return false;
                    }
                    LogPrintf("Included configuration file %s\n", conf_file_name);
                } else {
                    error = "Failed to include configuration file " + conf_file_name;
                    return false;
                }
            }

            // Warn about recursive -includeconf
            conf_file_names.clear();
            add_includes(chain_id, /* skip= */ chain_includes);
            add_includes({}, /* skip= */ default_includes);
            std::string chain_id_final = GetChainName();
            if (chain_id_final != chain_id) {
                // Also warn about recursive includeconf for the chain that was specified in one of the includeconfs
                add_includes(chain_id_final);
            }
            for (const std::string& conf_file_name : conf_file_names) {
                tfm::format(std::cerr, "warning: -includeconf cannot be used from included files; ignoring -includeconf=%s\n", conf_file_name);
            }
        }
    }

    // If datadir is changed in .conf file:
    ClearPathCache();
    if (!CheckDataDirOption(*this)) {
        error = strprintf("specified data directory \"%s\" does not exist.", GetArg("-datadir", ""));
        return false;
    }
    return true;
}

std::string ArgsManager::GetChainName() const
{
    auto get_net = [&](const std::string& arg) {
        LOCK(cs_args);
        util::SettingsValue value = util::GetSetting(m_settings, /* section= */ "", SettingName(arg),
            /* ignore_default_section_config= */ false,
            /* get_chain_name= */ true);
        return value.isNull() ? false : value.isBool() ? value.get_bool() : InterpretBool(value.get_str());
    };

    const bool fRegTest = get_net("-regtest");
    const bool fSigNet  = get_net("-signet");
    const bool fTestNet = get_net("-testnet");
    const bool is_chain_arg_set = IsArgSet("-chain");

    if ((int)is_chain_arg_set + (int)fRegTest + (int)fSigNet + (int)fTestNet > 1) {
        throw std::runtime_error("Invalid combination of -regtest, -signet, -testnet and -chain. Can use at most one.");
    }
    if (fRegTest)
        return CBaseChainParams::REGTEST;
    if (fSigNet) {
        return CBaseChainParams::SIGNET;
    }
    if (fTestNet)
        return CBaseChainParams::TESTNET;

    return GetArg("-chain", CBaseChainParams::MAIN);
}

bool ArgsManager::UseDefaultSection(const std::string& arg) const
{
    return m_network == CBaseChainParams::MAIN || m_network_only_args.count(arg) == 0;
}

util::SettingsValue ArgsManager::GetSetting(const std::string& arg) const
{
    LOCK(cs_args);
    return util::GetSetting(
        m_settings, m_network, SettingName(arg), !UseDefaultSection(arg), /* get_chain_name= */ false);
}

std::vector<util::SettingsValue> ArgsManager::GetSettingsList(const std::string& arg) const
{
    LOCK(cs_args);
    return util::GetSettingsList(m_settings, m_network, SettingName(arg), !UseDefaultSection(arg));
}

void ArgsManager::logArgsPrefix(
    const std::string& prefix,
    const std::string& section,
    const std::map<std::string, std::vector<util::SettingsValue>>& args) const
{
    std::string section_str = section.empty() ? "" : "[" + section + "] ";
    for (const auto& arg : args) {
        for (const auto& value : arg.second) {
            std::optional<unsigned int> flags = GetArgFlags('-' + arg.first);
            if (flags) {
                std::string value_str = (*flags & SENSITIVE) ? "****" : value.write();
                LogPrintf("%s %s%s=%s\n", prefix, section_str, arg.first, value_str);
            }
        }
    }
}

void ArgsManager::LogArgs() const
{
    LOCK(cs_args);
    for (const auto& section : m_settings.ro_config) {
        logArgsPrefix("Config file arg:", section.first, section.second);
    }
    for (const auto& setting : m_settings.rw_settings) {
        LogPrintf("Setting file arg: %s = %s\n", setting.first, setting.second.write());
    }
    logArgsPrefix("Command-line arg:", "", m_settings.command_line_options);
}

fs::path AbsPathForConfigVal(const ArgsManager& args, const fs::path& path, bool net_specific)
{
    if (path.is_absolute()) {
        return path;
    }
    return fsbridge::AbsPathJoin(net_specific ? args.GetDataDirNet() : args.GetDataDirBase(), path);
}

void SetupChainParamsBaseOptions(ArgsManager& argsman)
{
    argsman.AddArg("-chain=<chain>", "Use the chain <chain> (default: main). Allowed values: main, test, signet, regtest", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-regtest", "Enter regression test mode, which uses a special chain in which blocks can be solved instantly. "
                 "This is intended for regression testing tools and app development. Equivalent to -chain=regtest.", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-testactivationheight=name@height.", "Set the activation height of 'name' (segwit, bip34, dersig, cltv, csv). (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-testnet", "Use the test chain. Equivalent to -chain=test.", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-vbparams=deployment:start:end[:min_activation_height]", "Use given start/end times and min_activation_height for specified version bits deployment (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-signet", "Use the signet chain. Equivalent to -chain=signet. Note that the network is defined by the -signetchallenge parameter", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-signetchallenge", "Blocks must satisfy the given script to be considered valid (only for signet networks; defaults to the global default signet test network challenge)", ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_NEGATION, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-signetseednode", "Specify a seed node for the signet network, in the hostname[:port] format, e.g. sig.net:1234 (may be used multiple times to specify multiple seed nodes; defaults to the global default signet test network seed node(s))", ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_NEGATION, OptionsCategory::CHAINPARAMS);
}

ArgsManager gArgs;
