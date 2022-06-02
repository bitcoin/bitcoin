// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fs.h>
#include <logging.h>
#include <util/string.h>
#include <util/threadnames.h>
#include <util/time.h>

#include <algorithm>
#include <array>
#include <mutex>
#include <optional>
#include <unordered_map>

const char * const DEFAULT_DEBUGLOGFILE = "debug.log";

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
    auto category = GetLogCategory(str);
    if (!category) return false;
    EnableCategory(category.value());
    return true;
}

void BCLog::Logger::DisableCategory(BCLog::LogFlags flag)
{
    m_categories &= ~flag;
}

bool BCLog::Logger::DisableCategory(const std::string& str)
{
    auto category = GetLogCategory(str);
    if (!category) return false;
    DisableCategory(category.value());
    return true;
}

bool BCLog::Logger::WillLogCategory(BCLog::LogFlags category) const
{
    return (m_categories.load(std::memory_order_relaxed) & category) != 0;
}

bool BCLog::Logger::WillLogCategoryLevel(BCLog::LogFlags category, BCLog::Level level) const
{
    // Log the Info, Warning, and Error message levels unconditionally, so that
    // important troubleshooting information isn't lost.
    if (level >= BCLog::Level::Info) return true;

    if (!WillLogCategory(category)) return false;

    const auto it{m_category_log_levels.find(category)};
    return level >= (it == m_category_log_levels.end() ? m_log_level : it->second);
}

bool BCLog::Logger::DefaultShrinkDebugFile() const
{
    return m_categories == BCLog::NONE;
}

struct CLogCategoryDesc {
    BCLog::LogFlags flag;
    std::string category;
    // Allow CLogCategoryDesc to be converted to a key value pair in a map.
    operator std::pair<const BCLog::LogFlags, std::string>() const
    {
        return {flag, category};
    }
};

static const CLogCategoryDesc LogCategories[]{
    {BCLog::NONE, "none"},
    {BCLog::NET, "net"},
    {BCLog::TOR, "tor"},
    {BCLog::MEMPOOL, "mempool"},
    {BCLog::HTTP, "http"},
    {BCLog::BENCH, "bench"},
    {BCLog::ZMQ, "zmq"},
    {BCLog::WALLETDB, "walletdb"},
    {BCLog::RPC, "rpc"},
    {BCLog::ESTIMATEFEE, "estimatefee"},
    {BCLog::ADDRMAN, "addrman"},
    {BCLog::SELECTCOINS, "selectcoins"},
    {BCLog::REINDEX, "reindex"},
    {BCLog::CMPCTBLOCK, "cmpctblock"},
    {BCLog::RAND, "rand"},
    {BCLog::PRUNE, "prune"},
    {BCLog::PROXY, "proxy"},
    {BCLog::MEMPOOLREJ, "mempoolrej"},
    {BCLog::LIBEVENT, "libevent"},
    {BCLog::COINDB, "coindb"},
    {BCLog::QT, "qt"},
    {BCLog::LEVELDB, "leveldb"},
    {BCLog::VALIDATION, "validation"},
    {BCLog::I2P, "i2p"},
    {BCLog::IPC, "ipc"},
#ifdef DEBUG_LOCKCONTENTION
    {BCLog::LOCK, "lock"},
#endif
    {BCLog::UTIL, "util"},
    {BCLog::BLOCKSTORE, "blockstorage"},
    {BCLog::ALL, "all"},
    {BCLog::NONE, "0"},
    {BCLog::ALL, "1"},
};

static const std::unordered_map<BCLog::LogFlags, std::string> LogCategoryToStr{
    LogCategories,
    LogCategories + std::size(LogCategories) - 2, // ignore last 2 extra mappings of NONE and ALL
};

std::optional<BCLog::LogFlags> GetLogCategory(const std::string& str)
{
    if (str.empty()) return BCLog::ALL;
    for (const CLogCategoryDesc& category_desc : LogCategories) {
        if (category_desc.category == str) {
            return category_desc.flag;
        }
    }
    return std::nullopt;
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

static std::vector<BCLog::Level> LogLevelsList()
{
    std::vector<BCLog::Level> levels;
    for (int n = 0; n < static_cast<int>(BCLog::Level::None); ++n) {
        levels.emplace_back(static_cast<BCLog::Level>(n));
    }
    return levels;
}

std::string BCLog::Logger::LogLevelsString() const
{
    return Join(LogLevelsList(), ", ", [this](BCLog::Level level) { return LogLevelToStr(level); });
}

std::string BCLog::Logger::LogTimestampStr(const std::string& str)
{
    std::string strStamped;

    if (!m_log_timestamps)
        return str;

    if (m_started_new_line) {
        int64_t nTimeMicros = GetTimeMicros();
        strStamped = FormatISO8601DateTime(nTimeMicros/1000000);
        if (m_log_time_micros) {
            strStamped.pop_back();
            strStamped += strprintf(".%06dZ", nTimeMicros%1000000);
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
            auto it = LogCategoryToStr.find(category);
            s += it == LogCategoryToStr.end() ? "unknown" : it->second;
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
        const auto threadname = util::ThreadGetInternalName();
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
}

bool BCLog::Logger::SetLogLevel(const std::string& level_str)
{
    const auto level = GetLogLevel(level_str);
    if (!level) return false;
    m_log_level = level.value();
    return true;
}

bool BCLog::Logger::SetCategoryLogLevel(const std::string& category_str, const std::string& level_str)
{
    const auto category = GetLogCategory(category_str);
    if (!category) return false;
    const auto level = GetLogLevel(level_str);
    if (!level) return false;
    m_category_log_levels[category.value()] = level.value();
    return true;
}
