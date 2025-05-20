// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <memusage.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/string.h>
#include <util/threadnames.h>
#include <util/time.h>

#include <array>
#include <map>
#include <optional>

using util::Join;
using util::RemovePrefixView;

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

static int FileWriteStr(std::string_view str, FILE *fp)
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
    if (m_buffer_lines_discarded > 0) {
        LogPrintStr_(strprintf("Early logging buffer overflowed, %d log lines discarded.\n", m_buffer_lines_discarded), __func__, __FILE__, __LINE__, BCLog::ALL, Level::Info);
    }
    while (!m_msgs_before_open.empty()) {
        const auto& buflog = m_msgs_before_open.front();
        std::string s{buflog.str};
        FormatLogStrInPlace(s, buflog.category, buflog.level, buflog.source_file, buflog.source_line, buflog.logging_function, buflog.threadname, buflog.now, buflog.mocktime);
        m_msgs_before_open.pop_front();

        if (m_print_to_file) FileWriteStr(s, m_fileout);
        if (m_print_to_console) fwrite(s.data(), 1, s.size(), stdout);
        for (const auto& cb : m_print_callbacks) {
            cb(s);
        }
    }
    m_cur_buffer_memusage = 0;
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
    m_max_buffer_memusage = DEFAULT_MAX_LOG_BUFFER;
    m_cur_buffer_memusage = 0;
    m_buffer_lines_discarded = 0;
    m_msgs_before_open.clear();
}

void BCLog::Logger::DisableLogging()
{
    {
        StdLockGuard scoped_lock(m_cs);
        assert(m_buffering);
        assert(m_print_callbacks.empty());
    }
    m_print_to_file = false;
    m_print_to_console = false;
    StartLogging();
}

void BCLog::Logger::EnableCategory(BCLog::LogFlags flag)
{
    m_categories |= flag;
}

bool BCLog::Logger::EnableCategory(std::string_view str)
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

bool BCLog::Logger::DisableCategory(std::string_view str)
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
    // Log messages at Info, Warning and Error level unconditionally, so that
    // important troubleshooting information doesn't get lost.
    if (level >= BCLog::Level::Info) return true;

    if (!WillLogCategory(category)) return false;

    StdLockGuard scoped_lock(m_cs);
    const auto it{m_category_log_levels.find(category)};
    return level >= (it == m_category_log_levels.end() ? LogLevel() : it->second);
}

bool BCLog::Logger::DefaultShrinkDebugFile() const
{
    return m_categories == BCLog::NONE;
}

static const std::map<std::string, BCLog::LogFlags, std::less<>> LOG_CATEGORIES_BY_STR{
    {"net", BCLog::NET},
    {"tor", BCLog::TOR},
    {"mempool", BCLog::MEMPOOL},
    {"http", BCLog::HTTP},
    {"bench", BCLog::BENCH},
    {"zmq", BCLog::ZMQ},
    {"walletdb", BCLog::WALLETDB},
    {"rpc", BCLog::RPC},
    {"estimatefee", BCLog::ESTIMATEFEE},
    {"addrman", BCLog::ADDRMAN},
    {"selectcoins", BCLog::SELECTCOINS},
    {"reindex", BCLog::REINDEX},
    {"cmpctblock", BCLog::CMPCTBLOCK},
    {"rand", BCLog::RAND},
    {"prune", BCLog::PRUNE},
    {"proxy", BCLog::PROXY},
    {"mempoolrej", BCLog::MEMPOOLREJ},
    {"libevent", BCLog::LIBEVENT},
    {"coindb", BCLog::COINDB},
    {"qt", BCLog::QT},
    {"leveldb", BCLog::LEVELDB},
    {"validation", BCLog::VALIDATION},
    {"i2p", BCLog::I2P},
    {"ipc", BCLog::IPC},
#ifdef DEBUG_LOCKCONTENTION
    {"lock", BCLog::LOCK},
#endif
    {"blockstorage", BCLog::BLOCKSTORAGE},
    {"txreconciliation", BCLog::TXRECONCILIATION},
    {"scan", BCLog::SCAN},
    {"txpackages", BCLog::TXPACKAGES},
};

static const std::unordered_map<BCLog::LogFlags, std::string> LOG_CATEGORIES_BY_FLAG{
    // Swap keys and values from LOG_CATEGORIES_BY_STR.
    [](const auto& in) {
        std::unordered_map<BCLog::LogFlags, std::string> out;
        for (const auto& [k, v] : in) {
            const bool inserted{out.emplace(v, k).second};
            assert(inserted);
        }
        return out;
    }(LOG_CATEGORIES_BY_STR)
};

bool GetLogCategory(BCLog::LogFlags& flag, std::string_view str)
{
    if (str.empty() || str == "1" || str == "all") {
        flag = BCLog::ALL;
        return true;
    }
    auto it = LOG_CATEGORIES_BY_STR.find(str);
    if (it != LOG_CATEGORIES_BY_STR.end()) {
        flag = it->second;
        return true;
    }
    return false;
}

std::string BCLog::Logger::LogLevelToStr(BCLog::Level level)
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
    }
    assert(false);
}

static std::string LogCategoryToStr(BCLog::LogFlags category)
{
    if (category == BCLog::ALL) {
        return "all";
    }
    auto it = LOG_CATEGORIES_BY_FLAG.find(category);
    assert(it != LOG_CATEGORIES_BY_FLAG.end());
    return it->second;
}

static std::optional<BCLog::Level> GetLogLevel(std::string_view level_str)
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
    } else {
        return std::nullopt;
    }
}

std::vector<LogCategory> BCLog::Logger::LogCategoriesList() const
{
    std::vector<LogCategory> ret;
    ret.reserve(LOG_CATEGORIES_BY_STR.size());
    for (const auto& [category, flag] : LOG_CATEGORIES_BY_STR) {
        ret.push_back(LogCategory{.category = category, .active = WillLogCategory(flag)});
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
    return Join(std::vector<BCLog::Level>{levels.begin(), levels.end()}, ", ", [](BCLog::Level level) { return LogLevelToStr(level); });
}

std::string BCLog::Logger::LogTimestampStr(SystemClock::time_point now, std::chrono::seconds mocktime) const
{
    std::string strStamped;

    if (!m_log_timestamps)
        return strStamped;

    const auto now_seconds{std::chrono::time_point_cast<std::chrono::seconds>(now)};
    strStamped = FormatISO8601DateTime(TicksSinceEpoch<std::chrono::seconds>(now_seconds));
    if (m_log_time_micros && !strStamped.empty()) {
        strStamped.pop_back();
        strStamped += strprintf(".%06dZ", Ticks<std::chrono::microseconds>(now - now_seconds));
    }
    if (mocktime > 0s) {
        strStamped += " (mocktime: " + FormatISO8601DateTime(count_seconds(mocktime)) + ")";
    }
    strStamped += ' ';

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
    std::string LogEscapeMessage(std::string_view str) {
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

std::string BCLog::Logger::GetLogPrefix(BCLog::LogFlags category, BCLog::Level level) const
{
    if (category == LogFlags::NONE) category = LogFlags::ALL;

    const bool has_category{m_always_print_category_level || category != LogFlags::ALL};

    // If there is no category, Info is implied
    if (!has_category && level == Level::Info) return {};

    std::string s{"["};
    if (has_category) {
        s += LogCategoryToStr(category);
    }

    if (m_always_print_category_level || !has_category || level != Level::Debug) {
        // If there is a category, Debug is implied, so don't add the level

        // Only add separator if we have a category
        if (has_category) s += ":";
        s += Logger::LogLevelToStr(level);
    }

    s += "] ";
    return s;
}

static size_t MemUsage(const BCLog::Logger::BufferedLog& buflog)
{
    return buflog.str.size() + buflog.logging_function.size() + buflog.source_file.size() + buflog.threadname.size() + memusage::MallocUsage(sizeof(memusage::list_node<BCLog::Logger::BufferedLog>));
}

void BCLog::Logger::FormatLogStrInPlace(std::string& str, BCLog::LogFlags category, BCLog::Level level, std::string_view source_file, int source_line, std::string_view logging_function, std::string_view threadname, SystemClock::time_point now, std::chrono::seconds mocktime) const
{
    if (!str.ends_with('\n')) str.push_back('\n');

    str.insert(0, GetLogPrefix(category, level));

    if (m_log_sourcelocations) {
        str.insert(0, strprintf("[%s:%d] [%s] ", RemovePrefixView(source_file, "./"), source_line, logging_function));
    }

    if (m_log_threadnames) {
        str.insert(0, strprintf("[%s] ", (threadname.empty() ? "unknown" : threadname)));
    }

    str.insert(0, LogTimestampStr(now, mocktime));
}

void BCLog::Logger::LogPrintStr(std::string_view str, std::string_view logging_function, std::string_view source_file, int source_line, BCLog::LogFlags category, BCLog::Level level)
{
    StdLockGuard scoped_lock(m_cs);
    return LogPrintStr_(str, logging_function, source_file, source_line, category, level);
}

void BCLog::Logger::LogPrintStr_(std::string_view str, std::string_view logging_function, std::string_view source_file, int source_line, BCLog::LogFlags category, BCLog::Level level)
{
    std::string str_prefixed = LogEscapeMessage(str);

    if (m_buffering) {
        {
            BufferedLog buf{
                .now=SystemClock::now(),
                .mocktime=GetMockTime(),
                .str=str_prefixed,
                .logging_function=std::string(logging_function),
                .source_file=std::string(source_file),
                .threadname=util::ThreadGetInternalName(),
                .source_line=source_line,
                .category=category,
                .level=level,
            };
            m_cur_buffer_memusage += MemUsage(buf);
            m_msgs_before_open.push_back(std::move(buf));
        }

        while (m_cur_buffer_memusage > m_max_buffer_memusage) {
            if (m_msgs_before_open.empty()) {
                m_cur_buffer_memusage = 0;
                break;
            }
            m_cur_buffer_memusage -= MemUsage(m_msgs_before_open.front());
            m_msgs_before_open.pop_front();
            ++m_buffer_lines_discarded;
        }

        return;
    }

    FormatLogStrInPlace(str_prefixed, category, level, source_file, source_line, logging_function, util::ThreadGetInternalName(), SystemClock::now(), GetMockTime());

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

void BCLog::LogRateLimiter::MaybeReset()
{
    const auto now{NodeClock::now()};
    if ((now - m_last_reset) >= WINDOW_SIZE) {
        m_available_bytes = WINDOW_MAX_BYTES;
        m_last_reset = now;
        m_dropped_bytes = 0;
    }
}

bool BCLog::LogRateLimiter::Consume(uint64_t bytes)
{
    MaybeReset();

    if (bytes > m_available_bytes) {
        m_dropped_bytes += bytes;
        m_available_bytes = 0;
        return false;
    }

    m_available_bytes -= bytes;
    return true;
}

bool BCLog::Logger::SetLogLevel(std::string_view level_str)
{
    const auto level = GetLogLevel(level_str);
    if (!level.has_value() || level.value() > MAX_USER_SETABLE_SEVERITY_LEVEL) return false;
    m_log_level = level.value();
    return true;
}

bool BCLog::Logger::SetCategoryLogLevel(std::string_view category_str, std::string_view level_str)
{
    BCLog::LogFlags flag;
    if (!GetLogCategory(flag, category_str)) return false;

    const auto level = GetLogLevel(level_str);
    if (!level.has_value() || level.value() > MAX_USER_SETABLE_SEVERITY_LEVEL) return false;

    StdLockGuard scoped_lock(m_cs);
    m_category_log_levels[flag] = level.value();
    return true;
}
