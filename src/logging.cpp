// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <util/fs.h>
#include <util/string.h>
#include <util/threadnames.h>
#include <util/time.h>

#include <algorithm>
#include <array>
#include <mutex>
// SYSCOIN
#include <optional>
#include <common/args.h>
const char * const DEFAULT_DEBUGLOGFILE = "debug.log";
constexpr auto MAX_USER_SETABLE_SEVERITY_LEVEL{BCLog::Level::Info};

BCLog::Logger& LogInstance()
{
/**
 * NOTE: the logger instances is leaked on exit. This is ugly, but will be
 * cleaned up by the OS/libc. Defining a logger as a global object doesn't work
 * since the order of destruction of static/global objects is undefined.
 * Consider if the logger gets destroyed, and then some later destructor calls
 * LogPrintf, maybe indirectly, and you get a core dump at shutdown trying to
 * access the logger. When the shutdown sequence is fully audited and tested,
 * explicit destruction of these objects can be implemented by changing this
 * from a raw pointer to a std::unique_ptr.
 * Since the ~Logger() destructor is never called, the Logger class and all
 * its subclasses must have implicitly-defined destructors.
 *
 * This method of initialization was originally introduced in
 * ee3374234c60aba2cc4c5cd5cac1c0aefc2d817c.
 */
    static BCLog::Logger* g_logger{new BCLog::Logger()};
    return *g_logger;
}

bool fLogIPs = DEFAULT_LOGIPS;

static int FileWriteStr(const std::string &str, FILE *fp)
{
    return fwrite(str.data(), 1, str.size(), fp);
}

bool BCLog::Logger::StartLogging()
{
    StdLockGuard scoped_lock(m_cs);

    assert(m_buffering);
    assert(m_fileout == nullptr);

    if (m_print_to_file) {
        assert(!m_file_path.empty());
        m_fileout = fsbridge::fopen(m_file_path, "a");
        if (!m_fileout) {
            return false;
        }

        setbuf(m_fileout, nullptr); // unbuffered

        // Add newlines to the logfile to distinguish this execution from the
        // last one.
        FileWriteStr("\n\n\n\n\n", m_fileout);
    }

    // dump buffered messages from before we opened the log
    m_buffering = false;
    while (!m_msgs_before_open.empty()) {
        const std::string& s = m_msgs_before_open.front();

        if (m_print_to_file) FileWriteStr(s, m_fileout);
        if (m_print_to_console) fwrite(s.data(), 1, s.size(), stdout);
        for (const auto& cb : m_print_callbacks) {
            cb(s);
        }

        m_msgs_before_open.pop_front();
    }
    if (m_print_to_console) fflush(stdout);

    return true;
}

void BCLog::Logger::DisconnectTestLogger()
{
    StdLockGuard scoped_lock(m_cs);
    m_buffering = true;
    if (m_fileout != nullptr) fclose(m_fileout);
    m_fileout = nullptr;
    m_print_callbacks.clear();
}

void BCLog::Logger::EnableCategory(BCLog::LogFlags flag)
{
    m_categories |= flag;
}

bool BCLog::Logger::EnableCategory(const std::string& str)
{
    BCLog::LogFlags flag;
    if (!GetLogCategory(flag, str)) return false;
    EnableCategory(flag);
    return true;
}

void BCLog::Logger::DisableCategory(BCLog::LogFlags flag)
{
    m_categories &= ~flag;
}

bool BCLog::Logger::DisableCategory(const std::string& str)
{
    BCLog::LogFlags flag;
    if (!GetLogCategory(flag, str)) return false;
    DisableCategory(flag);
    return true;
}

bool BCLog::Logger::WillLogCategory(BCLog::LogFlags category) const
{
    return (m_categories.load(std::memory_order_relaxed) & category) != 0;
}

bool BCLog::Logger::WillLogCategoryLevel(BCLog::LogFlags category, BCLog::Level level) const
{
    // Log messages at Warning and Error level unconditionally, so that
    // important troubleshooting information doesn't get lost.
    if (level >= BCLog::Level::Warning) return true;

    if (!WillLogCategory(category)) return false;

    StdLockGuard scoped_lock(m_cs);
    const auto it{m_category_log_levels.find(category)};
    return level >= (it == m_category_log_levels.end() ? LogLevel() : it->second);
}

bool BCLog::Logger::DefaultShrinkDebugFile() const
{
    return m_categories == BCLog::NONE;
}

struct CLogCategoryDesc {
    BCLog::LogFlags flag;
    std::string category;
};

const CLogCategoryDesc LogCategories[] =
{
    {BCLog::NONE, "0"},
    {BCLog::NONE, ""},
    {BCLog::NET, "net"},
    {BCLog::TOR, "tor"},
    {BCLog::MEMPOOL, "mempool"},
    {BCLog::HTTP, "http"},
    {BCLog::BENCHMARK, "bench"},
    {BCLog::ZMQ, "zmq"},
    {BCLog::WALLETDB, "walletdb"},
    {BCLog::RPC, "rpc"},
    {BCLog::ESTIMATEFEE, "estimatefee"},
    {BCLog::ADDRMAN, "addrman"},
    {BCLog::SELECTCOINS, "selectcoins"},
    {BCLog::REINDEX, "reindex"},
    {BCLog::CMPCTBLOCK, "cmpctblock"},
    {BCLog::RANDOM, "rand"},
    {BCLog::PRUNE, "prune"},
    {BCLog::PROXY, "proxy"},
    {BCLog::MEMPOOLREJ, "mempoolrej"},
    {BCLog::LIBEVENT, "libevent"},
    {BCLog::COINDB, "coindb"},
    {BCLog::QT, "qt"},
    {BCLog::LEVELDB, "leveldb"},
    {BCLog::VALIDATION, "validation"},
    // SYSCOIN
    {BCLog::CHAINLOCKS, "chainlocks"},
    {BCLog::GOBJECT, "gobject"},
    {BCLog::LLMQ, "llmq"},
    {BCLog::LLMQ_DKG, "llmq-dkg"},
    {BCLog::LLMQ_SIGS, "llmq-sigs"},
    {BCLog::MNPAYMENTS, "mnpayments"},
    {BCLog::MNLIST, "mnlist"},
    {BCLog::MNSYNC, "mnsync"},
    {BCLog::SPORK, "spork"},
    {BCLog::SYS, "syscoin"},
    {BCLog::I2P, "i2p"},
    {BCLog::IPC, "ipc"},
#ifdef DEBUG_LOCKCONTENTION
    {BCLog::LOCK, "lock"},
#endif
    {BCLog::UTIL, "util"},
    {BCLog::BLOCKSTORAGE, "blockstorage"},
    {BCLog::TXRECONCILIATION, "txreconciliation"},
    {BCLog::SCAN, "scan"},
    {BCLog::TXPACKAGES, "txpackages"},
    {BCLog::ALL, "1"},
    {BCLog::ALL, "all"},
};

bool GetLogCategory(BCLog::LogFlags& flag, const std::string& str)
{
    if (str.empty()) {
        flag = BCLog::ALL;
        return true;
    }
    for (const CLogCategoryDesc& category_desc : LogCategories) {
        if (category_desc.category == str) {
            flag = category_desc.flag;
            return true;
        }
    }
    return false;
}

std::string BCLog::Logger::LogLevelToStr(BCLog::Level level) const
{
    switch (level) {
    case BCLog::Level::Trace:
        return "trace";
    case BCLog::Level::Debug:
        return "debug";
    case BCLog::Level::Info:
        return "info";
    case BCLog::Level::Warning:
        return "warning";
    case BCLog::Level::Error:
        return "error";
    case BCLog::Level::None:
        return "";
    }
    assert(false);
}

std::string LogCategoryToStr(BCLog::LogFlags category)
{
    // Each log category string representation should sync with LogCategories
    switch (category) {
    case BCLog::LogFlags::NONE:
        return "";
    case BCLog::LogFlags::NET:
        return "net";
    case BCLog::LogFlags::TOR:
        return "tor";
    case BCLog::LogFlags::MEMPOOL:
        return "mempool";
    case BCLog::LogFlags::HTTP:
        return "http";
    case BCLog::LogFlags::BENCHMARK:
        return "bench";
    case BCLog::LogFlags::ZMQ:
        return "zmq";
    case BCLog::LogFlags::WALLETDB:
        return "walletdb";
    case BCLog::LogFlags::RPC:
        return "rpc";
    case BCLog::LogFlags::ESTIMATEFEE:
        return "estimatefee";
    case BCLog::LogFlags::ADDRMAN:
        return "addrman";
    case BCLog::LogFlags::SELECTCOINS:
        return "selectcoins";
    case BCLog::LogFlags::REINDEX:
        return "reindex";
    case BCLog::LogFlags::CMPCTBLOCK:
        return "cmpctblock";
    case BCLog::LogFlags::RANDOM:
        return "rand";
    case BCLog::LogFlags::PRUNE:
        return "prune";
    case BCLog::LogFlags::PROXY:
        return "proxy";
    case BCLog::LogFlags::MEMPOOLREJ:
        return "mempoolrej";
    case BCLog::LogFlags::LIBEVENT:
        return "libevent";
    case BCLog::LogFlags::COINDB:
        return "coindb";
    case BCLog::LogFlags::QT:
        return "qt";
    case BCLog::LogFlags::LEVELDB:
        return "leveldb";
    case BCLog::LogFlags::VALIDATION:
        return "validation";
    case BCLog::LogFlags::CHAINLOCKS:
        return "chainlocks";
    case BCLog::LogFlags::GOBJECT:
        return "gobject";
    case BCLog::LogFlags::LLMQ:
        return "llmq";
    case BCLog::LogFlags::LLMQ_DKG:
        return "llmq-dkg";
    case BCLog::LogFlags::LLMQ_SIGS:
        return "llmq-sigs";
    case BCLog::LogFlags::MNPAYMENTS:
        return "mnpayments";
    case BCLog::LogFlags::MNSYNC:
        return "mnsync";
    case BCLog::LogFlags::MNLIST:
        return "mnlist";
    case BCLog::LogFlags::SYS:
        return "syscoin";
    case BCLog::LogFlags::SPORK:
        return "spork";
    case BCLog::LogFlags::I2P:
        return "i2p";
    case BCLog::LogFlags::IPC:
        return "ipc";
#ifdef DEBUG_LOCKCONTENTION
    case BCLog::LogFlags::LOCK:
        return "lock";
#endif
    case BCLog::LogFlags::UTIL:
        return "util";
    case BCLog::LogFlags::BLOCKSTORAGE:
        return "blockstorage";
    case BCLog::LogFlags::TXRECONCILIATION:
        return "txreconciliation";
    case BCLog::LogFlags::SCAN:
        return "scan";
    case BCLog::LogFlags::TXPACKAGES:
        return "txpackages";
    case BCLog::LogFlags::ALL:
        return "all";
    }
    assert(false);
}

static std::optional<BCLog::Level> GetLogLevel(const std::string& level_str)
{
    if (level_str == "trace") {
        return BCLog::Level::Trace;
    } else if (level_str == "debug") {
        return BCLog::Level::Debug;
    } else if (level_str == "info") {
        return BCLog::Level::Info;
    } else if (level_str == "warning") {
        return BCLog::Level::Warning;
    } else if (level_str == "error") {
        return BCLog::Level::Error;
    } else if (level_str == "none") {
        return BCLog::Level::None;
    } else {
        return std::nullopt;
    }
}

std::vector<LogCategory> BCLog::Logger::LogCategoriesList() const
{
    // Sort log categories by alphabetical order.
    std::array<CLogCategoryDesc, std::size(LogCategories)> categories;
    std::copy(std::begin(LogCategories), std::end(LogCategories), categories.begin());
    std::sort(categories.begin(), categories.end(), [](auto a, auto b) { return a.category < b.category; });

    std::vector<LogCategory> ret;
    for (const CLogCategoryDesc& category_desc : categories) {
        if (category_desc.flag == BCLog::NONE || category_desc.flag == BCLog::ALL) continue;
        LogCategory catActive;
        catActive.category = category_desc.category;
        catActive.active = WillLogCategory(category_desc.flag);
        ret.push_back(catActive);
    }
    return ret;
}

/** Log severity levels that can be selected by the user. */
static constexpr std::array<BCLog::Level, 3> LogLevelsList()
{
    return {BCLog::Level::Info, BCLog::Level::Debug, BCLog::Level::Trace};
}

std::string BCLog::Logger::LogLevelsString() const
{
    const auto& levels = LogLevelsList();
    return Join(std::vector<BCLog::Level>{levels.begin(), levels.end()}, ", ", [this](BCLog::Level level) { return LogLevelToStr(level); });
}

std::string BCLog::Logger::LogTimestampStr(const std::string& str)
{
    std::string strStamped;

    if (!m_log_timestamps)
        return str;

    if (m_started_new_line) {
        const auto now{SystemClock::now()};
        const auto now_seconds{std::chrono::time_point_cast<std::chrono::seconds>(now)};
        strStamped = FormatISO8601DateTime(TicksSinceEpoch<std::chrono::seconds>(now_seconds));
        if (m_log_time_micros && !strStamped.empty()) {
            strStamped.pop_back();
            strStamped += strprintf(".%06dZ", Ticks<std::chrono::microseconds>(now - now_seconds));
        }
        std::chrono::seconds mocktime = GetMockTime();
        if (mocktime > 0s) {
            strStamped += " (mocktime: " + FormatISO8601DateTime(count_seconds(mocktime)) + ")";
        }
        strStamped += ' ' + str;
    } else
        strStamped = str;

    return strStamped;
}

namespace BCLog {
    /** Belts and suspenders: make sure outgoing log messages don't contain
     * potentially suspicious characters, such as terminal control codes.
     *
     * This escapes control characters except newline ('\n') in C syntax.
     * It escapes instead of removes them to still allow for troubleshooting
     * issues where they accidentally end up in strings.
     */
    std::string LogEscapeMessage(const std::string& str) {
        std::string ret;
        for (char ch_in : str) {
            uint8_t ch = (uint8_t)ch_in;
            if ((ch >= 32 || ch == '\n') && ch != '\x7f') {
                ret += ch_in;
            } else {
                ret += strprintf("\\x%02x", ch);
            }
        }
        return ret;
    }
} // namespace BCLog

void BCLog::Logger::LogPrintStr(const std::string& str, const std::string& logging_function, const std::string& source_file, int source_line, BCLog::LogFlags category, BCLog::Level level)
{
    StdLockGuard scoped_lock(m_cs);
    std::string str_prefixed = LogEscapeMessage(str);

    if ((category != LogFlags::NONE || level != Level::None) && m_started_new_line) {
        std::string s{"["};

        if (category != LogFlags::NONE) {
            s += LogCategoryToStr(category);
        }

        if (category != LogFlags::NONE && level != Level::None) {
            // Only add separator if both flag and level are not NONE
            s += ":";
        }

        if (level != Level::None) {
            s += LogLevelToStr(level);
        }

        s += "] ";
        str_prefixed.insert(0, s);
    }

    if (m_log_sourcelocations && m_started_new_line) {
        str_prefixed.insert(0, "[" + RemovePrefix(source_file, "./") + ":" + ToString(source_line) + "] [" + logging_function + "] ");
    }

    if (m_log_threadnames && m_started_new_line) {
        const auto& threadname = util::ThreadGetInternalName();
        str_prefixed.insert(0, "[" + (threadname.empty() ? "unknown" : threadname) + "] ");
    }

    str_prefixed = LogTimestampStr(str_prefixed);

    m_started_new_line = !str.empty() && str[str.size()-1] == '\n';

    if (m_buffering) {
        // buffer if we haven't started logging yet
        m_msgs_before_open.push_back(str_prefixed);
        return;
    }

    if (m_print_to_console) {
        // print to console
        fwrite(str_prefixed.data(), 1, str_prefixed.size(), stdout);
        fflush(stdout);
    }
    for (const auto& cb : m_print_callbacks) {
        cb(str_prefixed);
    }
    if (m_print_to_file) {
        assert(m_fileout != nullptr);

        // reopen the log file, if requested
        if (m_reopen_file) {
            m_reopen_file = false;
            FILE* new_fileout = fsbridge::fopen(m_file_path, "a");
            if (new_fileout) {
                setbuf(new_fileout, nullptr); // unbuffered
                fclose(m_fileout);
                m_fileout = new_fileout;
            }
        }
        FileWriteStr(str_prefixed, m_fileout);
    }
}

void BCLog::Logger::ShrinkDebugFile()
{
    // Amount of debug.log to save at end when shrinking (must fit in memory)
    constexpr size_t RECENT_DEBUG_HISTORY_SIZE = 10 * 1000000;

    assert(!m_file_path.empty());

    // Scroll debug.log if it's getting too big
    FILE* file = fsbridge::fopen(m_file_path, "r");

    // Special files (e.g. device nodes) may not have a size.
    size_t log_size = 0;
    try {
        log_size = fs::file_size(m_file_path);
    } catch (const fs::filesystem_error&) {}

    // If debug.log file is more than 10% bigger the RECENT_DEBUG_HISTORY_SIZE
    // trim it down by saving only the last RECENT_DEBUG_HISTORY_SIZE bytes
    if (file && log_size > 11 * (RECENT_DEBUG_HISTORY_SIZE / 10))
    {
        // Restart the file with some of the end
        std::vector<char> vch(RECENT_DEBUG_HISTORY_SIZE, 0);
        if (fseek(file, -((long)vch.size()), SEEK_END)) {
            LogPrintf("Failed to shrink debug log file: fseek(...) failed\n");
            fclose(file);
            return;
        }
        int nBytes = fread(vch.data(), 1, vch.size(), file);
        fclose(file);

        file = fsbridge::fopen(m_file_path, "w");
        if (file)
        {
            fwrite(vch.data(), 1, nBytes, file);
            fclose(file);
        }
    }
    else if (file != nullptr)
        fclose(file);

    // SYSCOIN
    // Scroll sysgeth.log if it's getting too big
    file = fsbridge::fopen(gArgs.GetDataDirNet() / "sysgeth.log", "r");

    // Special files (e.g. device nodes) may not have a size.
    log_size = 0;
    try {
        log_size = fs::file_size(gArgs.GetDataDirNet() / "sysgeth.log");
    } catch (const fs::filesystem_error&) {}

    // If debug.log file is more than 10% bigger the RECENT_DEBUG_HISTORY_SIZE
    // trim it down by saving only the last RECENT_DEBUG_HISTORY_SIZE bytes
    if (file && log_size > 11 * (RECENT_DEBUG_HISTORY_SIZE / 10))
    {
        // Restart the file with some of the end
        std::vector<char> vch(RECENT_DEBUG_HISTORY_SIZE, 0);
        if (fseek(file, -((long)vch.size()), SEEK_END)) {
            LogPrintf("Failed to shrink debug log file: fseek(...) failed\n");
            fclose(file);
            return;
        }
        int nBytes = fread(vch.data(), 1, vch.size(), file);
        fclose(file);

        file = fsbridge::fopen(gArgs.GetDataDirNet() / "sysgeth.log", "w");
        if (file)
        {
            fwrite(vch.data(), 1, nBytes, file);
            fclose(file);
        }
    }
    else if (file != nullptr)
        fclose(file);
}

bool BCLog::Logger::SetLogLevel(const std::string& level_str)
{
    const auto level = GetLogLevel(level_str);
    if (!level.has_value() || level.value() > MAX_USER_SETABLE_SEVERITY_LEVEL) return false;
    m_log_level = level.value();
    return true;
}

bool BCLog::Logger::SetCategoryLogLevel(const std::string& category_str, const std::string& level_str)
{
    BCLog::LogFlags flag;
    if (!GetLogCategory(flag, category_str)) return false;

    const auto level = GetLogLevel(level_str);
    if (!level.has_value() || level.value() > MAX_USER_SETABLE_SEVERITY_LEVEL) return false;

    StdLockGuard scoped_lock(m_cs);
    m_category_log_levels[flag] = level.value();
    return true;
}
