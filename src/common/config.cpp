// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>

#include <common/settings.h>
#include <logging.h>
#include <sync.h>
#include <tinyformat.h>
#include <univalue.h>
#include <util/chaintype.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/string.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using util::TrimString;
using util::TrimStringView;

bool CreateDataDir(const fs::path& datadir, std::string& error)
{
    std::error_code ec;
    fs::file_status status{fs::status(datadir, ec)};
    if (!ec && status.type() == fs::file_type::directory && !fs::is_empty(datadir, ec)) return true;
    // When creating a *new* datadir, also create a "wallets" subdirectory,
    // whether or not the wallet is enabled now, so if the wallet is enabled
    // in the future, it will use the "wallets" subdirectory for creating
    // and listing wallets, rather than the top-level directory where
    // wallets could be mixed up with other files. For backwards
    // compatibility, wallet code will use the "wallets" subdirectory only
    // if it already exists, but never create it itself. There is discussion
    // in https://github.com/bitcoin/bitcoin/issues/16220 about ways to
    // change wallet code so it would no longer be necessary to create
    // "wallets" subdirectories here.
    std::filesystem::create_directories(fs::path{datadir / "wallets"}, ec);
    if (!ec) return true;
    error = strprintf("Failed to create data directory %s", fs::quoted(fs::PathToString(datadir)));
    if (ec) error = strprintf("%s: %s", error, ec.message());
    return false;
}

static bool GetConfigOptions(std::istream& stream, const std::string& filepath, std::string& error,
                             std::vector<std::pair<std::string, std::string>>& options,
                             std::list<SectionInfo>& sections)
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
                std::string name = prefix + TrimString(std::string_view{str}.substr(0, pos), pattern);
                std::string_view value = TrimStringView(std::string_view{str}.substr(pos + 1), pattern);
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
                if (str.size() >= 2 && str.starts_with("no")) {
                    error += strprintf(", if you intended to specify a negated option, use %s=1 instead", str);
                }
                return false;
            }
        }
        ++linenr;
    }
    return true;
}

bool IsConfSupported(KeyInfo& key, std::string& error) {
    if (key.name == "conf") {
        error = "conf cannot be set in the configuration file; use includeconf= if you want to include additional config files";
        return false;
    }
    if (key.name == "reindex") {
        // reindex can be set in a config file but it is strongly discouraged as this will cause the node to reindex on
        // every restart. Allow the config but throw a warning
        LogPrintf("Warning: reindex=1 is set in the configuration file, which will significantly slow down startup. Consider removing or commenting out this option for better performance, unless there is currently a condition which makes rebuilding the indexes necessary\n");
        return true;
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
        if (!IsConfSupported(key, error)) return false;
        if (flags) {
            std::optional<common::SettingsValue> value = InterpretValue(key, &option.second, *flags, error);
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

static bool GetExplicitDataDir(const ArgsManager& args, fs::path& datadir, std::string& error)
{
    fs::path datadir_arg = args.GetPathArg("-datadir");
    if (!CheckDataDirOption(args)) {
        error = strprintf("specified data directory %s does not exist.", fs::quoted(fs::PathToString(datadir_arg)));
        return false;
    }

    // Return unmodified datadir path if -datadir argument value or config file
    // value is not specified. If any value is specified, return it as the
    // datadir path. Call fs::absolute to treat relative datadir arguments and
    // datadir= lines in configuration files as being relative to the current
    // working directory. Probably it would make more sense to treat relative
    // datadir lines in configuration files as relative to the configuration
    // file, not the working directory, but current behavior is being kept for
    // compatibility.
    if (!datadir_arg.empty()) datadir = fs::absolute(std::move(datadir_arg));
    return true;
}

static bool GetInitialDataDir(fs::path& datadir, std::string& error, bool* aborted)
{
    assert(datadir.empty());
    datadir = GetDefaultDataDir();

    std::error_code ec;
    std::filesystem::file_status status = fs::status(datadir);
    if (ec) {
        error = strprintf("Could not access %s: %s\n", fs::quoted(fs::PathToString(datadir)), ec.message());
        return false;
    }

    // Allow a text file that points to another directory to be placed at
    // the default datadir location. It can be useful to point the default
    // datadir at an external location so bitcoin tools can be started
    // without -datadir arguments while the external location is still used
    // for storage. The most straightforward way of pointing the default
    // datadir at another location is with a symlink, but a text file is
    // allowed here to work on file systems that don't support symlinks.
    if (status.type() == fs::file_type::regular) {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        std::string line;
        try {
            file.open(datadir);
            std::getline(file, line);
        } catch (std::system_error& e) {
            error = strprintf("Could not read %s: %s", fs::quoted(fs::PathToString(datadir)), ec.message());
            return false;
        }
        fs::path path = fs::PathFromString(line);
        if (!path.is_absolute() || fs::is_directory(path, ec)) {
            error = "Invalid datadir path %s in file %s", fs::quoted(line), fs::quoted(fs::PathToString(datadir));
            if (ec) error = strprintf("%s: %s", error, ec.message());
            return false;
        }
        datadir = std::move(path);
    }
    assert (datadir.is_absolute());
    return true;
}

bool ArgsManager::ReadConfigFiles(std::string& error, bool ignore_invalid_keys, InitialDataDirFn initial_datadir_fn,
                                  fs::path* config_file, fs::path* initial_datadir, bool* aborted)
{
    // Save initial datadir value in case -conf path or any -includeconf paths
    // are relative paths, and need to be evaluated relative to the
    // datadir. The final datadir can change while parsing the config file if
    // it contains a datadir= line. Avoid calling initial_datadir_fn() yet if not
    // needed because it accesses the default datadir filesystem path, which
    // might be slow or off-limits due to permissions.
    fs::path datadir_path;
    if (!GetExplicitDataDir(*this, datadir_path, error)) return false;

    // Determine config file path relative to the initial datadir.
    if (!initial_datadir_fn) initial_datadir_fn = GetInitialDataDir;
    fs::path conf_path{GetPathArg("-conf", BITCOIN_CONF_FILENAME)};
    if (!conf_path.is_absolute()) {
        if (datadir_path.empty() && !initial_datadir_fn(datadir_path, error, aborted)) return false;
        assert(datadir_path.is_absolute());
        conf_path = datadir_path / std::move(conf_path);
    }

    {
        LOCK(cs_args);
        m_settings.ro_config.clear();
        m_config_sections.clear();
        m_config_path = conf_path;
    }

    if (config_file) *config_file = conf_path;
    if (initial_datadir) *initial_datadir = datadir_path;

    std::ifstream stream;
    if (!conf_path.empty()) { // path is empty when -noconf is specified
        if (fs::is_directory(conf_path)) {
            error = strprintf("Config file \"%s\" is a directory.", fs::PathToString(conf_path));
            return false;
        }
        stream = std::ifstream{conf_path};
        // If the file is explicitly specified, it must be readable
        if (IsArgSet("-conf") && !stream.good()) {
            error = strprintf("specified config file \"%s\" could not be opened.", fs::PathToString(conf_path));
            return false;
        }
    }
    // ok to not have a config file
    if (stream.good()) {
        if (!ReadConfigStream(stream, fs::PathToString(conf_path), error, ignore_invalid_keys)) {
            return false;
        }
        // `-includeconf` cannot be included in the command line arguments except
        // as `-noincludeconf` (which indicates that no included conf file should be used).
        bool use_conf_file{true};
        {
            LOCK(cs_args);
            if (auto* includes = common::FindKey(m_settings.command_line_options, "includeconf")) {
                // ParseParameters() fails if a non-negated -includeconf is passed on the command-line
                assert(common::SettingsSpan(*includes).last_negated());
                use_conf_file = false;
            }
        }
        if (use_conf_file) {
            std::string chain_id = GetChainTypeString();
            std::vector<std::string> conf_file_names;

            auto add_includes = [&](const std::string& network, size_t skip = 0) {
                size_t num_values = 0;
                LOCK(cs_args);
                if (auto* section = common::FindKey(m_settings.ro_config, network)) {
                    if (auto* values = common::FindKey(*section, "includeconf")) {
                        for (size_t i = std::max(skip, common::SettingsSpan(*values).negated()); i < values->size(); ++i) {
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
                fs::path include_path = fs::PathFromString(conf_file_name);
                if (!include_path.is_absolute()) {
                    if (datadir_path.empty() && !initial_datadir_fn(datadir_path, error, aborted)) return false;
                    assert(datadir_path.is_absolute());
                    include_path = datadir_path / std::move(include_path);
                }
                if (fs::is_directory(include_path)) {
                    error = strprintf("Included config file \"%s\" is a directory.", fs::PathToString(include_path));
                    return false;
                }
                std::ifstream conf_file_stream{include_path};
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
            std::string chain_id_final = GetChainTypeString();
            if (chain_id_final != chain_id) {
                // Also warn about recursive includeconf for the chain that was specified in one of the includeconfs
                add_includes(chain_id_final);
            }
            for (const std::string& conf_file_name : conf_file_names) {
                tfm::format(std::cerr, "warning: -includeconf cannot be used from included files; ignoring -includeconf=%s\n", conf_file_name);
            }
        }
    }

    // Update datadir if case .conf file set a new datadir location.
    if (!GetExplicitDataDir(*this, datadir_path, error) ||
        (datadir_path.empty() && !initial_datadir_fn(datadir_path, error, aborted)))
        return false;

    WITH_LOCK(cs_args, m_datadir = std::move(datadir_path));

    return true;
}

fs::path AbsPathForConfigVal(const ArgsManager& args, const fs::path& path, bool net_specific)
{
    if (path.is_absolute() || path.empty()) {
        return path;
    }
    return fsbridge::AbsPathJoin(net_specific ? args.GetDataDirNet() : args.GetDataDirBase(), path);
}
