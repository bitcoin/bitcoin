// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sync.h>
#include <util/system.h>

#ifdef HAVE_BOOST_PROCESS
#include <boost/process.hpp>
#endif // HAVE_BOOST_PROCESS

#include <chainparamsbase.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/translation.h>


#if (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
#include <pthread.h>
#include <pthread_np.h>
#endif

#ifndef WIN32
// for posix_fallocate, in configure.ac we check if it is present after this
#ifdef __linux__

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#define _POSIX_C_SOURCE 200112L

#endif // __linux__

#include <algorithm>
#include <cassert>
#include <fcntl.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/stat.h>

#else

#ifdef _MSC_VER
#pragma warning(disable:4786)
#pragma warning(disable:4804)
#pragma warning(disable:4805)
#pragma warning(disable:4717)
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <codecvt>

#include <io.h> /* for _commit */
#include <shellapi.h>
#include <shlobj.h>
#endif

#ifdef HAVE_MALLOPT_ARENA_MAX
#include <malloc.h>
#endif

#include <boost/algorithm/string/replace.hpp>
#include <thread>
// SYSCOIN only features
#include <chainparams.h>
#include <boost/algorithm/string.hpp>
#include <signal.h>
#include <rpc/server.h>
#include <util/time.h>
#include <ctpl.h>
#include <random.h>
#include <curl/curl.h>
bool fMasternodeMode = false;
bool bGethTestnet = false;
bool fDisableGovernance = false;
bool fRegTest = false;
bool fSigNet = false;
bool fTestNet = false;
bool fAssetIndex = false;
uint32_t fGethSyncHeight = 0;
uint32_t fGethCurrentHeight = 0;
pid_t gethPID = 0;
pid_t relayerPID = 0;
int64_t nRandomResetSec = 0;
int64_t nLastGethHeaderTime = 0;
std::string fGethSyncStatus = "waiting to sync...";
bool fGethSynced = false;
bool fLoaded = false;
int32_t DEFAULT_MN_COLLATERAL_REQUIRED = 100000;
int64_t DEFAULT_MAX_RECOVERED_SIGS_AGE = 60 * 60 * 24 * 7; // keep them for a week
CAmount nMNCollateralRequired = DEFAULT_MN_COLLATERAL_REQUIRED*COIN;
std::vector<JSONRPCRequest> vecTPSRawTransactions;
RecursiveMutex cs_relayer;
RecursiveMutex cs_geth;
#include <typeinfo>
#include <univalue.h>

// Application startup time (used for uptime calculation)
const int64_t nStartupTime = GetTime();
const char * const SYSCOIN_CONF_FILENAME = "syscoin.conf";
const char * const SYSCOIN_SETTINGS_FILENAME = "settings.json";

ArgsManager gArgs;
// SYSCOIN
struct DescriptorDetails {
    std::string binURL;
    std::string sha256Sum;
};
#ifdef WIN32
    #include <windows.h>
    #include <winnt.h>
    #include <winternl.h>
    #include <stdio.h>
    #include <errno.h>
    #include <assert.h>
    #include <process.h>
    pid_t fork(std::string app, std::string arg)
    {
        std::string appQuoted = "\"" + app + "\"";
        PROCESS_INFORMATION pi;
        STARTUPINFOW si;
        ZeroMemory(&pi, sizeof(pi));
        ZeroMemory(&si, sizeof(si));
        GetStartupInfoW (&si);
        si.cb = sizeof(si); 
        size_t start_pos = 0;
        //Prepare CreateProcess args
        std::wstring appQuoted_w(appQuoted.length(), L' '); // Make room for characters
        std::copy(appQuoted.begin(), appQuoted.end(), appQuoted_w.begin()); // Copy string to wstring.

        std::wstring app_w(app.length(), L' '); // Make room for characters
        std::copy(app.begin(), app.end(), app_w.begin()); // Copy string to wstring.

        std::wstring arg_w(arg.length(), L' '); // Make room for characters
        std::copy(arg.begin(), arg.end(), arg_w.begin()); // Copy string to wstring.

        std::wstring input = appQuoted_w + L" " + arg_w;
        wchar_t* arg_concat = const_cast<wchar_t*>( input.c_str() );
        const wchar_t* app_const = app_w.c_str();
        LogPrintf("CreateProcessW app %s %s\n",app,arg);
        int result = CreateProcessW(app_const, arg_concat, NULL, NULL, FALSE, 
              CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
        if(!result)
        {
            LogPrintf("CreateProcess failed (%d)\n", GetLastError());
            return 0;
        }
        pid_t pid = (pid_t)pi.dwProcessId;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return pid;
    }
#endif
/** Mutex to protect dir_locks. */
static Mutex cs_dir_locks;
/** A map that contains all the currently held directory locks. After
 * successful locking, these will be held here until the global destructor
 * cleans them up and thus automatically unlocks them, or ReleaseDirectoryLocks
 * is called.
 */
static std::map<std::string, std::unique_ptr<fsbridge::FileLock>> dir_locks GUARDED_BY(cs_dir_locks);

bool LockDirectory(const fs::path& directory, const std::string lockfile_name, bool probe_only)
{
    LOCK(cs_dir_locks);
    fs::path pathLockFile = directory / lockfile_name;

    // If a lock for this directory already exists in the map, don't try to re-lock it
    if (dir_locks.count(pathLockFile.string())) {
        return true;
    }

    // Create empty lock file if it doesn't exist.
    FILE* file = fsbridge::fopen(pathLockFile, "a");
    if (file) fclose(file);
    auto lock = MakeUnique<fsbridge::FileLock>(pathLockFile);
    if (!lock->TryLock()) {
        return error("Error while attempting to lock directory %s: %s", directory.string(), lock->GetReason());
    }
    if (!probe_only) {
        // Lock successful and we're not just probing, put it into the map
        dir_locks.emplace(pathLockFile.string(), std::move(lock));
    }
    return true;
}

void UnlockDirectory(const fs::path& directory, const std::string& lockfile_name)
{
    LOCK(cs_dir_locks);
    dir_locks.erase((directory / lockfile_name).string());
}

void ReleaseDirectoryLocks()
{
    LOCK(cs_dir_locks);
    dir_locks.clear();
}

bool DirIsWritable(const fs::path& directory)
{
    fs::path tmpFile = directory / fs::unique_path();

    FILE* file = fsbridge::fopen(tmpFile, "a");
    if (!file) return false;

    fclose(file);
    remove(tmpFile);

    return true;
}

bool CheckDiskSpace(const fs::path& dir, uint64_t additional_bytes)
{
    constexpr uint64_t min_disk_space = 52428800; // 50 MiB

    uint64_t free_bytes_available = fs::space(dir).available;
    return free_bytes_available >= min_disk_space + additional_bytes;
}

std::streampos GetFileSize(const char* path, std::streamsize max) {
    std::ifstream file(path, std::ios::binary);
    file.ignore(max);
    return file.gcount();
}

/**
 * Interpret a string argument as a boolean.
 *
 * The definition of atoi() requires that non-numeric string values like "foo",
 * return 0. This means that if a user unintentionally supplies a non-integer
 * argument here, the return value is always false. This means that -foo=false
 * does what the user probably expects, but -foo=true is well defined but does
 * not do what they probably expected.
 *
 * The return value of atoi() is undefined when given input not representable as
 * an int. On most systems this means string value between "-2147483648" and
 * "2147483647" are well defined (this method will return true). Setting
 * -txindex=2147483648 on most systems, however, is probably undefined.
 *
 * For a more extensive discussion of this topic (and a wide range of opinions
 * on the Right Way to change this code), see PR12713.
 */
static bool InterpretBool(const std::string& strValue)
{
    if (strValue.empty())
        return true;
    return (atoi(strValue) != 0);
}

static std::string SettingName(const std::string& arg)
{
    return arg.size() > 0 && arg[0] == '-' ? arg.substr(1) : arg;
}

/**
 * Interpret -nofoo as if the user supplied -foo=0.
 *
 * This method also tracks when the -no form was supplied, and if so,
 * checks whether there was a double-negative (-nofoo=0 -> -foo=1).
 *
 * If there was not a double negative, it removes the "no" from the key
 * and returns false.
 *
 * If there was a double negative, it removes "no" from the key, and
 * returns true.
 *
 * If there was no "no", it returns the string value untouched.
 *
 * Where an option was negated can be later checked using the
 * IsArgNegated() method. One use case for this is to have a way to disable
 * options that are not normally boolean (e.g. using -nodebuglogfile to request
 * that debug log output is not sent to any file at all).
 */

static util::SettingsValue InterpretOption(std::string& section, std::string& key, const std::string& value)
{
    // Split section name from key name for keys like "testnet.foo" or "regtest.bar"
    size_t option_index = key.find('.');
    if (option_index != std::string::npos) {
        section = key.substr(0, option_index);
        key.erase(0, option_index + 1);
    }
    if (key.substr(0, 2) == "no") {
        key.erase(0, 2);
        // Double negatives like -nofoo=0 are supported (but discouraged)
        if (!InterpretBool(value)) {
            LogPrintf("Warning: parsed potentially confusing double-negative -%s=%s\n", key, value);
            return true;
        }
        return false;
    }
    return value;
}

/**
 * Check settings value validity according to flags.
 *
 * TODO: Add more meaningful error checks here in the future
 * See "here's how the flags are meant to behave" in
 * https://github.com/bitcoin/bitcoin/pull/16097#issuecomment-514627823
 */
static bool CheckValid(const std::string& key, const util::SettingsValue& val, unsigned int flags, std::string& error)
{
    if (val.isBool() && !(flags & ArgsManager::ALLOW_BOOL)) {
        error = strprintf("Negating of -%s is meaningless and therefore forbidden", key);
        return false;
    }
    return true;
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

        if (key == "-") break; //syscoin-tx using stdin
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

        if (key[0] != '-')
            break;

        // Transform --foo to -foo
        if (key.length() > 1 && key[1] == '-')
            key.erase(0, 1);

        // Transform -foo to foo
        key.erase(0, 1);
        std::string section;
        util::SettingsValue value = InterpretOption(section, key, val);
        Optional<unsigned int> flags = GetArgFlags('-' + key);

        // Unknown command line options and command line options with dot
        // characters (which are returned from InterpretOption with nonempty
        // section strings) are not valid.
        if (!flags || !section.empty()) {
            error = strprintf("Invalid parameter %s", argv[i]);
            return false;
        }

        if (!CheckValid(key, value, *flags, error)) return false;

        m_settings.command_line_options[key].push_back(value);
    }

    // we do not allow -includeconf from command line
    bool success = true;
    if (auto* includes = util::FindKey(m_settings.command_line_options, "includeconf")) {
        for (const auto& include : util::SettingsSpan(*includes)) {
            error += "-includeconf cannot be used from commandline; -includeconf=" + include.get_str() + "\n";
            success = false;
        }
    }
    return success;
}

Optional<unsigned int> ArgsManager::GetArgFlags(const std::string& name) const
{
    LOCK(cs_args);
    for (const auto& arg_map : m_available_args) {
        const auto search = arg_map.second.find(name);
        if (search != arg_map.second.end()) {
            return search->second.m_flags;
        }
    }
    return nullopt;
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
        error = strprintf("Failed loading settings file:\n- %s\n", Join(errors, "\n- "));
        return false;
    }
    if (!WriteSettingsFile(&errors)) {
        error = strprintf("Failed saving settings file:\n- %s\n", Join(errors, "\n- "));
        return false;
    }
    return true;
}

bool ArgsManager::GetSettingsPath(fs::path* filepath, bool temp) const
{
    if (IsArgNegated("-settings")) {
        return false;
    }
    if (filepath) {
        std::string settings = GetArg("-settings", SYSCOIN_SETTINGS_FILENAME);
        *filepath = fs::absolute(temp ? settings + ".tmp" : settings, GetDataDir(/* net_specific= */ true));
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
        std::string section;
        std::string key = setting.first;
        (void)InterpretOption(section, key, /* value */ {}); // Split setting key into section and argname
        if (!GetArgFlags('-' + key)) {
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
        SaveErrors({strprintf("Failed renaming settings file %s to %s\n", path_tmp.string(), path.string())}, errors);
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
    return value.isNull() ? strDefault : value.isFalse() ? "0" : value.isTrue() ? "1" : value.get_str();
}

int64_t ArgsManager::GetArg(const std::string& strArg, int64_t nDefault) const
{
    const util::SettingsValue value = GetSetting(strArg);
    return value.isNull() ? nDefault : value.isFalse() ? 0 : value.isTrue() ? 1 : value.isNum() ? value.get_int64() : atoi64(value.get_str());
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

void ArgsManager::AddArg(const std::string& name, const std::string& help, unsigned int flags, const OptionsCategory& cat)
{
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

static std::string FormatException(const std::exception* pex, const char* pszThread)
{
#ifdef WIN32
    char pszModule[MAX_PATH] = "";
    GetModuleFileNameA(nullptr, pszModule, sizeof(pszModule));
#else
    const char* pszModule = "syscoin";
#endif
    if (pex)
        return strprintf(
            "EXCEPTION: %s       \n%s       \n%s in %s       \n", typeid(*pex).name(), pex->what(), pszModule, pszThread);
    else
        return strprintf(
            "UNKNOWN EXCEPTION       \n%s in %s       \n", pszModule, pszThread);
}

void PrintExceptionContinue(const std::exception* pex, const char* pszThread)
{
    std::string message = FormatException(pex, pszThread);
    LogPrintf("\n\n************************\n%s\n", message);
    tfm::format(std::cerr, "\n\n************************\n%s\n", message);
}

fs::path GetDefaultDataDir()
{
    // Windows: C:\Users\Username\AppData\Roaming\Syscoin
    // macOS: ~/Library/Application Support/Syscoin
    // Unix-like: ~/.syscoin
#ifdef WIN32
    // Windows
    return GetSpecialFolderPath(CSIDL_APPDATA) / "Syscoin";
#else
    fs::path pathRet;
    char* pszHome = getenv("HOME");
    if (pszHome == nullptr || strlen(pszHome) == 0)
        pathRet = fs::path("/");
    else
        pathRet = fs::path(pszHome);
#ifdef MAC_OSX
    // macOS
    return pathRet / "Library/Application Support/Syscoin";
#else
    // Unix-like
    return pathRet / ".syscoin";
#endif
#endif
}

namespace {
fs::path StripRedundantLastElementsOfPath(const fs::path& path)
{
    auto result = path;
    while (result.filename().string() == ".") {
        result = result.parent_path();
    }

    assert(fs::equivalent(result, path));
    return result;
}
} // namespace

static fs::path g_blocks_path_cache_net_specific;
static fs::path pathCached;
static fs::path pathCachedNetSpecific;
static RecursiveMutex csPathCached;

const fs::path &GetBlocksDir()
{
    LOCK(csPathCached);
    fs::path &path = g_blocks_path_cache_net_specific;

    // Cache the path to avoid calling fs::create_directories on every call of
    // this function
    if (!path.empty()) return path;

    if (gArgs.IsArgSet("-blocksdir")) {
        path = fs::system_complete(gArgs.GetArg("-blocksdir", ""));
        if (!fs::is_directory(path)) {
            path = "";
            return path;
        }
    } else {
        path = GetDataDir(false);
    }

    path /= BaseParams().DataDir();
    path /= "blocks";
    // SYSCOIN
    TryCreateDirectories(path);
    path = StripRedundantLastElementsOfPath(path);
    return path;
}

const fs::path &GetDataDir(bool fNetSpecific)
{
    LOCK(csPathCached);
    fs::path &path = fNetSpecific ? pathCachedNetSpecific : pathCached;

    // Cache the path to avoid calling fs::create_directories on every call of
    // this function
    if (!path.empty()) return path;

    std::string datadir = gArgs.GetArg("-datadir", "");
    if (!datadir.empty()) {
        path = fs::system_complete(datadir);
        if (!fs::is_directory(path)) {
            path = "";
            return path;
        }
    } else {
        path = GetDefaultDataDir();
    }
    if (fNetSpecific)
        path /= BaseParams().DataDir();

    // SYSCOIN
    if (TryCreateDirectories(path)) {
        // This is the first run, create wallets subdirectory too
        TryCreateDirectories(path / "wallets");
    }

    path = StripRedundantLastElementsOfPath(path);
    return path;
}

bool CheckDataDirOption()
{
    std::string datadir = gArgs.GetArg("-datadir", "");
    return datadir.empty() || fs::is_directory(fs::system_complete(datadir));
}

void ClearDatadirCache()
{
    LOCK(csPathCached);

    pathCached = fs::path();
    pathCachedNetSpecific = fs::path();
    g_blocks_path_cache_net_specific = fs::path();
}

fs::path GetConfigFile(const std::string& confPath)
{
    return AbsPathForConfigVal(fs::path(confPath), false);
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
        std::string section;
        std::string key = option.first;
        util::SettingsValue value = InterpretOption(section, key, option.second);
        Optional<unsigned int> flags = GetArgFlags('-' + key);
        if (flags) {
            if (!CheckValid(key, value, *flags, error)) {
                return false;
            }
            m_settings.ro_config[section][key].push_back(value);
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

    const std::string confPath = GetArg("-conf", SYSCOIN_CONF_FILENAME);
    fsbridge::ifstream stream(GetConfigFile(confPath));

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
                fsbridge::ifstream conf_file_stream(GetConfigFile(conf_file_name));
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
    ClearDatadirCache();
    if (!CheckDataDirOption()) {
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
// SYSCOIN
fs::path GetGethPidFile()
{
    return AbsPathForConfigVal(fs::path("geth.pid"));
}
void KillProcess(const pid_t& pid){
    if(pid <= 0)
        return;
    LogPrintf("%s: Trying to kill pid %d\n", __func__, pid);
    #ifdef WIN32
        HANDLE handy;
        handy =OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE, TRUE,pid);
        TerminateProcess(handy,0);
    #endif  
    #ifndef WIN32
        int result = 0;
        for(int i =0;i<10;i++){
            UninterruptibleSleep(std::chrono::milliseconds(500));
            result = kill( pid, SIGINT ) ;
            if(result == 0){
                LogPrintf("%s: Killing with SIGINT %d\n", __func__, pid);
                continue;
            }  
            LogPrintf("%s: Killed with SIGINT\n", __func__);
            return;
        }
        for(int i =0;i<10;i++){
            UninterruptibleSleep(std::chrono::milliseconds(500));
            result = kill( pid, SIGTERM ) ;
            if(result == 0){
                LogPrintf("%s: Killing with SIGTERM %d\n", __func__, pid);
                continue;
            }  
            LogPrintf("%s: Killed with SIGTERM\n", __func__);
            return;
        }
        for(int i =0;i<10;i++){
            UninterruptibleSleep(std::chrono::milliseconds(500));
            result = kill( pid, SIGKILL ) ;
            if(result == 0){
                LogPrintf("%s: Killing with SIGKILL %d\n", __func__, pid);
                continue;
            }  
            LogPrintf("%s: Killed with SIGKILL\n", __func__);
            return;
        }  
        LogPrintf("%s: Done trying to kill with SIGINT-SIGTERM-SIGKILL\n", __func__);            
    #endif 
}
std::string GetGethFilename(){
    // For Windows:
    #ifdef WIN32
       return "sysgeth.exe";
    #endif    
    #ifdef MAC_OSX
        // Mac
        return "sysgeth";
    #else
        // Linux
        return "sysgeth";
    #endif
}
bool StopGethNode(pid_t &pid)
{
    if(pid < 0)
        return false;
    if(pid){
        try{
            KillProcess(pid);
            LogPrintf("%s: Geth successfully exited from pid %d\n", __func__, pid);
        }
        catch(...){
            LogPrintf("%s: Geth failed to exit from pid %d\n", __func__, pid);
        }
    }
    {
        boost::filesystem::ifstream ifs(GetGethPidFile(), std::ios::in);
        pid_t pidFile = 0;
        while(ifs >> pidFile){
            if(pidFile && pidFile != pid){
                try{
                    KillProcess(pidFile);
                    LogPrintf("%s: Geth successfully exited from pid %d(from geth.pid)\n", __func__, pidFile);
                }
                catch(...){
                    LogPrintf("%s: Geth failed to exit from pid %d(from geth.pid)\n", __func__, pidFile);
                }
            } 
        }  
    }
    boost::filesystem::remove(GetGethPidFile());
    pid = -1;
    return true;
}
bool CheckSpecs(std::string &errMsg, bool bMiner){
    meminfo_t memInfo = parse_meminfo();
    LogPrintf("Total Memory(MB) %d (Total Free %d) Swap Total(MB) %d (Total Free %d)\n", memInfo.MemTotalMiB, memInfo.MemAvailableMiB, memInfo.SwapTotalMiB, memInfo.SwapFreeMiB);
    if(memInfo.MemTotalMiB < (bMiner? 8000: 3800))
        errMsg = _("Insufficient memory, you need at least 4GB RAM to run a masternode and be running in a Unix OS. Please see documentation.").translated;
    if(memInfo.MemTotalMiB < 7600 && memInfo.SwapTotalMiB < 3800)
        errMsg = _("Insufficient swap memory, you need at least 4GB swap RAM to run a masternode and be running in a Unix OS. Please see documentation.").translated;           
    LogPrintf("Total number of physical cores found %d\n", GetNumCores());
    if(GetNumCores() < (bMiner? 4: 2))
        errMsg = _("Insufficient CPU cores, you need at least 2 cores to run a masternode. Please see documentation.").translated;
   return errMsg.empty();         
}
bool GetDescriptorStats(const fs::path filePath, DescriptorDetails& details) {
    fsbridge::ifstream file(filePath);
    if (!file.is_open())
        return false;
    std::string fileBuffer;
    while (file.good()) {
        std::string input_buffer;
        file >> input_buffer;
        fileBuffer += input_buffer;
    }
    file.close();
    UniValue json(UniValue::VOBJ);
    std::string binArchitectureTag = "linux";
    #ifdef WIN32
        binArchitectureTag = "windows";
    #endif    
    #ifdef MAC_OSX
        binArchitectureTag = "darwin";
    #endif
    if(json.read(fileBuffer) && json.isObject()) {
        const UniValue& jsonObj = json.get_obj();
        const UniValue& architectureValue = find_value(jsonObj, "bins");
        if(architectureValue.isObject()) {
            const UniValue& binValue = find_value(architectureValue.get_obj(), binArchitectureTag);
            if(binValue.isObject()) {
                const UniValue& binURLValue = find_value(binValue, "url");
                if(binURLValue.isStr()) {
                    const UniValue& binChecksumValue = find_value(binValue, "sha256sum");
                    if(binChecksumValue.isStr()) {
                        details.binURL = binURLValue.get_str();
                        details.sha256Sum = binChecksumValue.get_str();
                        return true;
                    }
                }
            }
        }  
    }
    return false;
}
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}
bool DownloadFile(const std::string &url, const std::string &dest, const std::string &mode="wb", const std::string &checksum="") {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) {
        fp = fopen(dest.c_str(),mode.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 1);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        /* always cleanup */
        curl_easy_cleanup(curl);
        fclose(fp);
        if(mode == "wb")
            fs::permissions(dest,
                    fs::perms::owner_exe | fs::perms::group_exe |
                    fs::perms::others_exe | fs::perms::owner_read | fs::perms::group_read |
                    fs::perms::others_read);
    }
    if(!checksum.empty()) {
        LogPrintf("%s: Checking file checksum of %s\n", __func__, dest);
        fsbridge::ifstream file(dest, std::ios_base::binary);
        if (!file.is_open())
            return false;
        std::string fileBuffer;
        while (file.good()) {
            std::string input_buffer;
            file >> input_buffer;
            fileBuffer += input_buffer;
        }
        file.close();
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << fileBuffer;
        const uint256& calcHash = ss.GetSHA256();
        return calcHash.ToString() == checksum;
    }
    return true;
}
bool DownloadBinaryFromDescriptor(const std::string &descriptorDestPath, const std::string& binaryDestPath, const std::string& descriptorURL) {
    DescriptorDetails descriptorDetailsLocal, descriptorDetailsRemote;
    GetDescriptorStats(descriptorDestPath, descriptorDetailsLocal);
    // always download remote descriptor to check checksum, if remote doesn't exist use local. Both local and remote cannot be missing.
    if(!DownloadFile(descriptorURL, descriptorDestPath, "w"))
        return false;
    if(!GetDescriptorStats(descriptorDestPath, descriptorDetailsRemote)) {
        if(!descriptorDetailsLocal.binURL.empty()) {
            LogPrintf("%s: Could not download descriptor from %s but found local descriptor so using that...\n", __func__, descriptorURL);
            return true;
        }
        LogPrintf("%s: Could not download descriptor from %s\n", __func__, descriptorURL);
        return false;
    }
    if(descriptorDetailsLocal.sha256Sum.empty() || descriptorDetailsLocal.sha256Sum != descriptorDetailsRemote.sha256Sum) {
         LogPrintf("%s: New version available! Downloading from %s and saving to %s\n", __func__, descriptorDetailsRemote.binURL, binaryDestPath);
         if(!DownloadFile(descriptorDetailsRemote.binURL, binaryDestPath, "wb", descriptorDetailsRemote.sha256Sum)) {
             LogPrintf("%s: Could not download binary %s or checksum failed\n", __func__, descriptorDetailsRemote.binURL);
             return false;
         }
    }
    LogPrintf("%s: Version is up-to-date!\n", __func__);
    return true;
}
bool StartGethNode(const std::string &gethDescriptorURL, pid_t &pid, int websocketport, int ethrpcport, const std::string &mode)
{
    LOCK(cs_geth);
    if(mode == "disabled") {
        LogPrintf("%s: Geth is disabled, user chose to deploy their own Geth instance!\n", __func__);
        pid = -1;
        return true;
    }
    // stop any geth nodes before starting
    StopGethNode(pid);

    LogPrintf("%s: Downloading Geth descriptor from %s\n", __func__, gethDescriptorURL);
    fs::path descriptorPath = GetDataDir() / "gethdescriptor.json";
    fs::path binaryURL = GetDataDir() / GetGethFilename();
    // if either bin or descriptor not existing remove both files to download from scratch
    if (!fs::exists(binaryURL.string()) || !fs::exists(descriptorPath.string())) {
        if(fs::exists(binaryURL.string()))
            fs::remove(binaryURL.string());
        if(fs::exists(descriptorPath.string()))
            fs::remove(descriptorPath.string());          
    }
    if(!DownloadBinaryFromDescriptor(descriptorPath.string(), binaryURL.string(), gethDescriptorURL)) {
        if (fs::exists(descriptorPath.string())) {
            fs::remove(descriptorPath.string());
        }
        if (fs::exists(binaryURL.string())) {
            fs::remove(binaryURL.string());
        }
        return false;
    }
    LogPrintf("%s: Starting geth on wsport %d rpcport %d (testnet=%d)...\n", __func__, websocketport, ethrpcport, bGethTestnet? 1:0);
    

    fs::path attempt1 = binaryURL.string();
    attempt1 = attempt1.make_preferred();

    fs::path dataDir = GetDataDir(true) / "geth";
    #ifndef WIN32
    // Prevent killed child-processes remaining as "defunct"
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sa.sa_flags = SA_NOCLDWAIT;
        
    sigaction( SIGCHLD, &sa, NULL ) ;
        
    // Duplicate ("fork") the process. Will return zero in the child
    // process, and the child's PID in the parent (or negative on error).
    pid = fork() ;
    if( pid < 0 ) {
        LogPrintf("Could not start Geth, pid < 0 %d\n", pid);
        return false;
    }
	// TODO: sanitize environment variables as per
	// https://wiki.sei.cmu.edu/confluence/display/c/ENV03-C.+Sanitize+the+environment+when+invoking+external+programs
    if( pid == 0 ) {
        std::string portStr = itostr(websocketport);
        std::string rpcportStr = itostr(ethrpcport);
        char * argvAttempt1[20] = {(char*)attempt1.string().c_str(), 
                (char*)"--ws", (char*)"--ws.port", (char*)portStr.c_str(),
                (char*)"--http", (char*)"--http.api", (char*)"personal,eth", (char*)"--http.port", (char*)rpcportStr.c_str(),
                (char*)"--ws.origins", (char*)"*",
                (char*)"--syncmode", (char*)mode.c_str(), 
                (char*)"--datadir", (char*)dataDir.c_str(),
                (char*)"--allow-insecure-unlock",
                bGethTestnet?(char*)"--rinkeby": NULL,
                (char*)"--http.corsdomain",(char*)"*",
                NULL };                                                              
        execv(argvAttempt1[0], &argvAttempt1[0]); // current directory
        if (errno != 0) {
            LogPrintf("Geth not found at %s\n", argvAttempt1[0]);
        }
    } else {
        boost::filesystem::ofstream ofs(GetGethPidFile(), std::ios::out | std::ios::trunc);
        ofs << pid;
    }
    #else
        std::string portStr = itostr(websocketport);
        std::string rpcportStr = itostr(ethrpcport);
        std::string args =  std::string("--http --http.api personal,eth --http.corsdomain * --http.port ") + rpcportStr + std::string(" --ws --ws.port ") + portStr + std::string(" --ws.origins * --syncmode ") + mode + std::string(" --datadir ") +  dataDir.string();
        if(bGethTestnet) {
            args += std::string(" --rinkeby");
        }
        pid = fork(attempt1.string(), args);
        if( pid <= 0 ) {
            LogPrintf("Geth not found at %s\n", attempt1.string());
        }  
        boost::filesystem::ofstream ofs(GetGethPidFile(), std::ios::out | std::ios::trunc);
        ofs << pid;
    #endif
    if(pid > 0)
        LogPrintf("%s: Geth Started with pid %d\n", __func__, pid);
    return true;
}

// SYSCOIN - RELAYER
fs::path GetRelayerPidFile()
{
    return AbsPathForConfigVal(fs::path("relayer.pid"));
}
std::string GetRelayerFilename(){
    // For Windows:
    #ifdef WIN32
       return "sysrelayer.exe";
    #endif    
    #ifdef MAC_OSX
        // Mac
        return "sysrelayer";
    #else
        // Linux
        return "sysrelayer";
    #endif
}
bool StopRelayerNode(pid_t &pid)
{
    if(pid < 0)
        return false;
    if(pid){
        try{
            KillProcess(pid);
            LogPrintf("%s: Relayer successfully exited from pid %d\n", __func__, pid);
        }
        catch(...){
            LogPrintf("%s: Relayer failed to exit from pid %d\n", __func__, pid);
        }
    }
    {
        boost::filesystem::ifstream ifs(GetRelayerPidFile(), std::ios::in);
        pid_t pidFile = 0;
        while(ifs >> pidFile){
            if(pidFile && pidFile != pid){
                try{
                    KillProcess(pidFile);
                    LogPrintf("%s: Relayer successfully exited from pid %d(from relayer.pid)\n", __func__, pidFile);
                }
                catch(...){
                    LogPrintf("%s: Relayer failed to exit from pid %d(from relayer.pid)\n", __func__, pidFile);
                }
            } 
        }  
    }
    boost::filesystem::remove(GetRelayerPidFile());
    pid = -1;
    return true;
}

bool StartRelayerNode(const std::string &relayerDescriptorURL, pid_t &pid, int rpcport, int websocketport, int ethrpcport)
{   
    LOCK(cs_relayer);
    // stop any relayer process  before starting
    StopRelayerNode(pid);

    // Get RPC credentials
    std::string strRPCUserColonPass;
    if (gArgs.GetArg("-rpcpassword", "") != "" || !GetAuthCookie(&strRPCUserColonPass)) {
        strRPCUserColonPass = gArgs.GetArg("-rpcuser", "") + ":" + gArgs.GetArg("-rpcpassword", "");
    }

    LogPrintf("%s: Downloading Relayer descriptor from %s\n", __func__, relayerDescriptorURL);
    fs::path descriptorPath = GetDataDir() / "relayerdescriptor.json";
    fs::path binaryURL = GetDataDir() / GetRelayerFilename();
    if (!fs::exists(binaryURL.string()) || !fs::exists(descriptorPath.string())) {
        if(fs::exists(binaryURL.string()))
            fs::remove(binaryURL.string());
        if(fs::exists(descriptorPath.string()))
            fs::remove(descriptorPath.string());          
    }
    if(!DownloadBinaryFromDescriptor(descriptorPath.string(), binaryURL.string(), relayerDescriptorURL)) {
        if (fs::exists(descriptorPath.string())) {
            fs::remove(descriptorPath.string());
        }
        if (fs::exists(binaryURL.string())) {
            fs::remove(binaryURL.string());
        }
        return false;
    }
    LogPrintf("%s: Starting relayer on port %d, RPC credentials %s, wsport %d ethrpcport %d...\n", __func__, rpcport, strRPCUserColonPass, websocketport, ethrpcport);
    
    // stop any geth nodes before starting
    StopGethNode(pid);

    fs::path attempt1 = binaryURL.string();
    attempt1 = attempt1.make_preferred();
    fs::path dataDir = GetDataDir(true) / "geth";

    #ifndef WIN32
        // Prevent killed child-processes remaining as "defunct"
        struct sigaction sa;
        sa.sa_handler = SIG_DFL;
        sa.sa_flags = SA_NOCLDWAIT;
      
        sigaction( SIGCHLD, &sa, NULL ) ;
		// Duplicate ("fork") the process. Will return zero in the child
        // process, and the child's PID in the parent (or negative on error).
        pid = fork() ;
        if( pid < 0 ) {
            LogPrintf("Could not start Relayer, pid < 0 %d\n", pid);
            return false;
        }

        if( pid == 0 ) {
            // Order of looking for the relayer binary:
            // 1. current executable directory
            // 2. current executable directory/bin/[os]/syscoin_relayer
            // 3. $path
            // 4. $path/bin/[os]/syscoin_relayer
            // 5. /usr/local/bin/syscoin_relayer
            std::string portStr = itostr(websocketport);
            std::string rpcEthPortStr = itostr(ethrpcport);
            std::string rpcSysPortStr = itostr(rpcport);
            char * argvAttempt1[] = {(char*)attempt1.string().c_str(), 
					(char*)"--ethwsport", (char*)portStr.c_str(),
                    (char*)"--ethrpcport", (char*)rpcEthPortStr.c_str(),
                    (char*)"--datadir", (char*)dataDir.string().c_str(),
					(char*)"--sysrpcusercolonpass", (char*)strRPCUserColonPass.c_str(),
					(char*)"--sysrpcport", (char*)rpcSysPortStr.c_str(),  NULL };
            execv(argvAttempt1[0], &argvAttempt1[0]); // current directory
	        if (errno != 0) {
		        LogPrintf("Relayer not found at %s\n", argvAttempt1[0]);
	        }
        } else {
            boost::filesystem::ofstream ofs(GetRelayerPidFile(), std::ios::out | std::ios::trunc);
            ofs << pid;
        }
    #else
		std::string portStr = itostr(websocketport);
        std::string rpcEthPortStr = itostr(ethrpcport);
        std::string rpcSysPortStr = itostr(rpcport);
        std::string args =
				std::string("--ethwsport ") + portStr +
                std::string(" --ethrpcport ") + rpcEthPortStr +
                std::string(" --datadir ") + dataDir.string() +
				std::string(" --sysrpcusercolonpass ") + strRPCUserColonPass +
				std::string(" --sysrpcport ") + rpcSysPortStr; 
        pid = fork(attempt1.string(), args);
        if( pid <= 0 ) {
            LogPrintf("Relayer not found at %s\n", attempt1.string());
        }                         
        boost::filesystem::ofstream ofs(GetRelayerPidFile(), std::ios::out | std::ios::trunc);
        ofs << pid;
	#endif
    if(pid > 0)
        LogPrintf("%s: Relayer started with pid %d\n", __func__, pid);
    return true;
}
/* Parse the contents of /proc/meminfo (in buf), return value of "name"
 * (example: MemTotal) */
static long get_entry(const char* name, const char* buf)
{
    const char* hit = strstr(buf, name);
    if (hit == NULL) {
        return -1;
    }

    errno = 0;
    long val = strtol(hit + strlen(name), NULL, 10);
    if (errno != 0) {
        perror("get_entry: strtol() failed");
        return -1;
    }
    return val;
}

/* Like get_entry(), but exit if the value cannot be found */
static long get_entry_fatal(const char* name, const char* buf)
{
    long val = get_entry(name, buf);
    
    return val;
}

/* If the kernel does not provide MemAvailable (introduced in Linux 3.14),
 * approximate it using other data we can get */
static long available_guesstimate(const char* buf)
{
    long Cached = get_entry_fatal("Cached:", buf);
    long MemFree = get_entry_fatal("MemFree:", buf);
    long Buffers = get_entry_fatal("Buffers:", buf);
    long Shmem = get_entry_fatal("Shmem:", buf);

    return MemFree + Cached + Buffers - Shmem;
}

meminfo_t parse_meminfo()
{
    static FILE* fd;
    static char buf[8192];
    meminfo_t m;

    if (fd == NULL)
        fd = fopen("/proc/meminfo", "r");
    if (fd == NULL) {
        return m;
    }
    rewind(fd);

    size_t len = fread(buf, 1, sizeof(buf) - 1, fd);
    if (len == 0) {
       return m;
    }
    buf[len] = 0; // Make sure buf is zero-terminated

    m.MemTotalKiB = get_entry_fatal("MemTotal:", buf);
    m.SwapTotalKiB = get_entry_fatal("SwapTotal:", buf);
    long SwapFree = get_entry_fatal("SwapFree:", buf);

    long MemAvailable = get_entry("MemAvailable:", buf);
    if (MemAvailable <= -1) {
        MemAvailable = available_guesstimate(buf);
        LogPrintf("Warning: Your kernel does not provide MemAvailable data (needs 3.14+)\n"
                        "         Falling back to guesstimate\n");
    }

    // Calculate percentages
    m.MemAvailablePercent = MemAvailable * 100 / m.MemTotalKiB;
    if (m.SwapTotalKiB > 0) {
        m.SwapFreePercent = SwapFree * 100 / m.SwapTotalKiB;
    } else {
        m.SwapFreePercent = 0;
    }

    // Convert kiB to MiB
    m.MemTotalMiB = m.MemTotalKiB / 1024;
    m.MemAvailableMiB = MemAvailable / 1024;
    m.SwapTotalMiB = m.SwapTotalKiB / 1024;
    m.SwapFreeMiB = SwapFree / 1024;

    return m;
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
            Optional<unsigned int> flags = GetArgFlags('-' + arg.first);
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

bool RenameOver(fs::path src, fs::path dest)
{
#ifdef WIN32
    return MoveFileExW(src.wstring().c_str(), dest.wstring().c_str(),
                       MOVEFILE_REPLACE_EXISTING) != 0;
#else
    int rc = std::rename(src.string().c_str(), dest.string().c_str());
    return (rc == 0);
#endif /* WIN32 */
}

/**
 * Ignores exceptions thrown by Boost's create_directories if the requested directory exists.
 * Specifically handles case where path p exists, but it wasn't possible for the user to
 * write to the parent directory.
 */
bool TryCreateDirectories(const fs::path& p)
{
    try
    {
        if (fs::exists(p) && !fs::is_directory(p)) {
            fs::remove(p);
        }
        return fs::create_directories(p);
    } catch (const fs::filesystem_error&) {
        if (!fs::exists(p) || !fs::is_directory(p))
            throw;
    }

    // create_directories didn't create the directory, it had to have existed already
    return false;
}

bool FileCommit(FILE *file)
{
    if (fflush(file) != 0) { // harmless if redundantly called
        LogPrintf("%s: fflush failed: %d\n", __func__, errno);
        return false;
    }
#ifdef WIN32
    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
    if (FlushFileBuffers(hFile) == 0) {
        LogPrintf("%s: FlushFileBuffers failed: %d\n", __func__, GetLastError());
        return false;
    }
#else
    #if HAVE_FDATASYNC
    if (fdatasync(fileno(file)) != 0 && errno != EINVAL) { // Ignore EINVAL for filesystems that don't support sync
        LogPrintf("%s: fdatasync failed: %d\n", __func__, errno);
        return false;
    }
    #elif defined(MAC_OSX) && defined(F_FULLFSYNC)
    if (fcntl(fileno(file), F_FULLFSYNC, 0) == -1) { // Manpage says "value other than -1" is returned on success
        LogPrintf("%s: fcntl F_FULLFSYNC failed: %d\n", __func__, errno);
        return false;
    }
    #else
    if (fsync(fileno(file)) != 0 && errno != EINVAL) {
        LogPrintf("%s: fsync failed: %d\n", __func__, errno);
        return false;
    }
    #endif
#endif
    return true;
}

bool TruncateFile(FILE *file, unsigned int length) {
#if defined(WIN32)
    return _chsize(_fileno(file), length) == 0;
#else
    return ftruncate(fileno(file), length) == 0;
#endif
}

/**
 * this function tries to raise the file descriptor limit to the requested number.
 * It returns the actual file descriptor limit (which may be more or less than nMinFD)
 */
int RaiseFileDescriptorLimit(int nMinFD) {
#if defined(WIN32)
    return 2048;
#else
    struct rlimit limitFD;
    if (getrlimit(RLIMIT_NOFILE, &limitFD) != -1) {
        if (limitFD.rlim_cur < (rlim_t)nMinFD) {
            limitFD.rlim_cur = nMinFD;
            if (limitFD.rlim_cur > limitFD.rlim_max)
                limitFD.rlim_cur = limitFD.rlim_max;
            setrlimit(RLIMIT_NOFILE, &limitFD);
            getrlimit(RLIMIT_NOFILE, &limitFD);
        }
        return limitFD.rlim_cur;
    }
    return nMinFD; // getrlimit failed, assume it's fine
#endif
}

/**
 * this function tries to make a particular range of a file allocated (corresponding to disk space)
 * it is advisory, and the range specified in the arguments will never contain live data
 */
void AllocateFileRange(FILE *file, unsigned int offset, unsigned int length) {
#if defined(WIN32)
    // Windows-specific version
    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
    LARGE_INTEGER nFileSize;
    int64_t nEndPos = (int64_t)offset + length;
    nFileSize.u.LowPart = nEndPos & 0xFFFFFFFF;
    nFileSize.u.HighPart = nEndPos >> 32;
    SetFilePointerEx(hFile, nFileSize, 0, FILE_BEGIN);
    SetEndOfFile(hFile);
#elif defined(MAC_OSX)
    // OSX specific version
    // NOTE: Contrary to other OS versions, the OSX version assumes that
    // NOTE: offset is the size of the file.
    fstore_t fst;
    fst.fst_flags = F_ALLOCATECONTIG;
    fst.fst_posmode = F_PEOFPOSMODE;
    fst.fst_offset = 0;
    fst.fst_length = length; // mac os fst_length takes the # of free bytes to allocate, not desired file size
    fst.fst_bytesalloc = 0;
    if (fcntl(fileno(file), F_PREALLOCATE, &fst) == -1) {
        fst.fst_flags = F_ALLOCATEALL;
        fcntl(fileno(file), F_PREALLOCATE, &fst);
    }
    ftruncate(fileno(file), static_cast<off_t>(offset) + length);
#else
    #if defined(HAVE_POSIX_FALLOCATE)
    // Version using posix_fallocate
    off_t nEndPos = (off_t)offset + length;
    if (0 == posix_fallocate(fileno(file), 0, nEndPos)) return;
    #endif
    // Fallback version
    // TODO: just write one byte per block
    static const char buf[65536] = {};
    if (fseek(file, offset, SEEK_SET)) {
        return;
    }
    while (length > 0) {
        unsigned int now = 65536;
        if (length < now)
            now = length;
        fwrite(buf, 1, now, file); // allowed to fail; this function is advisory anyway
        length -= now;
    }
#endif
}

#ifdef WIN32
fs::path GetSpecialFolderPath(int nFolder, bool fCreate)
{
    WCHAR pszPath[MAX_PATH] = L"";

    if(SHGetSpecialFolderPathW(nullptr, pszPath, nFolder, fCreate))
    {
        return fs::path(pszPath);
    }

    LogPrintf("SHGetSpecialFolderPathW() failed, could not obtain requested path.\n");
    return fs::path("");
}
#endif

#ifndef WIN32
std::string ShellEscape(const std::string& arg)
{
    std::string escaped = arg;
    boost::replace_all(escaped, "'", "'\"'\"'");
    return "'" + escaped + "'";
}
#endif

#if HAVE_SYSTEM
void runCommand(const std::string& strCommand)
{
    if (strCommand.empty()) return;
#ifndef WIN32
    int nErr = ::system(strCommand.c_str());
#else
    int nErr = ::_wsystem(std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>,wchar_t>().from_bytes(strCommand).c_str());
#endif
    if (nErr)
        LogPrintf("runCommand error: system(%s) returned %d\n", strCommand, nErr);
}
#endif

#ifdef HAVE_BOOST_PROCESS
UniValue RunCommandParseJSON(const std::string& str_command, const std::string& str_std_in)
{
    namespace bp = boost::process;

    UniValue result_json;
    bp::opstream stdin_stream;
    bp::ipstream stdout_stream;
    bp::ipstream stderr_stream;

    if (str_command.empty()) return UniValue::VNULL;

    bp::child c(
        str_command,
        bp::std_out > stdout_stream,
        bp::std_err > stderr_stream,
        bp::std_in < stdin_stream
    );
    if (!str_std_in.empty()) {
        stdin_stream << str_std_in << std::endl;
    }
    stdin_stream.pipe().close();

    std::string result;
    std::string error;
    std::getline(stdout_stream, result);
    std::getline(stderr_stream, error);

    c.wait();
    const int n_error = c.exit_code();
    if (n_error) throw std::runtime_error(strprintf("RunCommandParseJSON error: process(%s) returned %d: %s\n", str_command, n_error, error));
    if (!result_json.read(result)) throw std::runtime_error("Unable to parse JSON: " + result);

    return result_json;
}
#endif // HAVE_BOOST_PROCESS

void SetupEnvironment()
{
#ifdef HAVE_MALLOPT_ARENA_MAX
    // glibc-specific: On 32-bit systems set the number of arenas to 1.
    // By default, since glibc 2.10, the C library will create up to two heap
    // arenas per core. This is known to cause excessive virtual address space
    // usage in our usage. Work around it by setting the maximum number of
    // arenas to 1.
    if (sizeof(void*) == 4) {
        mallopt(M_ARENA_MAX, 1);
    }
#endif
    // On most POSIX systems (e.g. Linux, but not BSD) the environment's locale
    // may be invalid, in which case the "C.UTF-8" locale is used as fallback.
#if !defined(WIN32) && !defined(MAC_OSX) && !defined(__FreeBSD__) && !defined(__OpenBSD__)
    try {
        std::locale(""); // Raises a runtime error if current locale is invalid
    } catch (const std::runtime_error&) {
        setenv("LC_ALL", "C.UTF-8", 1);
    }
#elif defined(WIN32)
    // Set the default input/output charset is utf-8
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
    // The path locale is lazy initialized and to avoid deinitialization errors
    // in multithreading environments, it is set explicitly by the main thread.
    // A dummy locale is used to extract the internal default locale, used by
    // fs::path, which is then used to explicitly imbue the path.
    std::locale loc = fs::path::imbue(std::locale::classic());
#ifndef WIN32
    fs::path::imbue(loc);
#else
    fs::path::imbue(std::locale(loc, new std::codecvt_utf8_utf16<wchar_t>()));
#endif
}

bool SetupNetworking()
{
#ifdef WIN32
    // Initialize Windows Sockets
    WSADATA wsadata;
    int ret = WSAStartup(MAKEWORD(2,2), &wsadata);
    if (ret != NO_ERROR || LOBYTE(wsadata.wVersion ) != 2 || HIBYTE(wsadata.wVersion) != 2)
        return false;
#endif
    return true;
}

int GetNumCores()
{
    return std::thread::hardware_concurrency();
}

std::string CopyrightHolders(const std::string& strPrefix)
{
    const auto copyright_devs = strprintf(_(COPYRIGHT_HOLDERS).translated, COPYRIGHT_HOLDERS_SUBSTITUTION);
    std::string strCopyrightHolders = strPrefix + copyright_devs;

    // Make sure Syscoin Core copyright is not removed by accident
    if (copyright_devs.find("Syscoin Core") == std::string::npos) {
        strCopyrightHolders += "\n" + strPrefix + "The Syscoin Core developers";
    }
    return strCopyrightHolders;
}

// Obtain the application startup time (used for uptime calculation)
int64_t GetStartupTime()
{
    return nStartupTime;
}

fs::path AbsPathForConfigVal(const fs::path& path, bool net_specific)
{
    if (path.is_absolute()) {
        return path;
    }
    return fs::absolute(path, GetDataDir(net_specific));
}

void ScheduleBatchPriority()
{
#ifdef SCHED_BATCH
    const static sched_param param{};
    const int rc = pthread_setschedparam(pthread_self(), SCHED_BATCH, &param);
    if (rc != 0) {
        LogPrintf("Failed to pthread_setschedparam: %s\n", strerror(rc));
    }
#endif
}

namespace util {
#ifdef WIN32
WinCmdLineArgs::WinCmdLineArgs()
{
    wchar_t** wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf8_cvt;
    argv = new char*[argc];
    args.resize(argc);
    for (int i = 0; i < argc; i++) {
        args[i] = utf8_cvt.to_bytes(wargv[i]);
        argv[i] = &*args[i].begin();
    }
    LocalFree(wargv);
}

WinCmdLineArgs::~WinCmdLineArgs()
{
    delete[] argv;
}

std::pair<int, char**> WinCmdLineArgs::get()
{
    return std::make_pair(argc, argv);
}
#endif
} // namespace util
