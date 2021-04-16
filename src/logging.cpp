// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <util/threadnames.h>
#include <util/string.h>
#include <util/time.h>

#include <mutex>

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

bool BCLog::Logger::DefaultShrinkDebugFile() const
{
    return m_categories == BCLog::NONE;
}

struct CLogCategoryDesc
{
    BCLog::LogFlags flag;
    std::string category;
};

const CLogCategoryDesc LogCategories[] =
{
    {BCLog::NONE, "0"},
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
    {BCLog::ALL, "1"},
    {BCLog::ALL, "all"},
};

bool GetLogCategory(BCLog::LogFlags& flag, const std::string& str)
{
    if (str == "") {
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

std::vector<LogCategory> BCLog::Logger::LogCategoriesList() const
{
    std::vector<LogCategory> ret;
    for (const CLogCategoryDesc& category_desc : LogCategories) {
        // Omit the special cases.
        if (category_desc.flag != BCLog::NONE && category_desc.flag != BCLog::ALL) {
            LogCategory catActive;
            catActive.category = category_desc.category;
            catActive.active = WillLogCategory(category_desc.flag);
            ret.push_back(catActive);
        }
    }
    return ret;
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
}

static constexpr uint64_t HOURLY_LOG_QUOTA_IN_BYTES_PER_SOURCE_LOCATION{1024 * 1024};

void ComputeQuotaUsageStats(const std::unordered_map<SourceLocation, QuotaUsage, SourceLocationHasher>& quota_usage_per_source_location,
                            QuotaUsage& total_usage, std::string& all_locations, std::string& excessive_locations)
{
    for (const auto& quota_map_entry : quota_usage_per_source_location) {
        const SourceLocation& location = quota_map_entry.first;
        const QuotaUsage& location_usage = quota_map_entry.second;

        total_usage.m_bytes_dropped += location_usage.m_bytes_dropped;
        total_usage.m_messages_dropped += location_usage.m_messages_dropped;
        total_usage.m_bytes_logged += location_usage.m_bytes_logged;

        if (location_usage.m_messages_dropped > 0) {
            // Append all locations that dropped at least one message.
            all_locations = strprintf("%s%s:%d, ", all_locations, location.first, location.second);

            if (location_usage.m_bytes_logged > HOURLY_LOG_QUOTA_IN_BYTES_PER_SOURCE_LOCATION ||
                location_usage.m_bytes_dropped > HOURLY_LOG_QUOTA_IN_BYTES_PER_SOURCE_LOCATION) {
                // Append all locations that either logged or dropped excessively.
                excessive_locations = strprintf("%s%s:%d, ", excessive_locations, location.first, location.second);
            }
        }
    }
}

void BCLog::Logger::LogPrintStr(const std::string& str, const std::string& logging_function, const SourceLocation& source_location, const bool skip_disk_usage_rate_limiting)
{
    StdLockGuard scoped_lock(m_cs);
    std::string str_prefixed = LogEscapeMessage(str);

    if (m_log_sourcelocations && m_started_new_line) {
        str_prefixed.insert(0, "[" + RemovePrefix(source_location.first, "./") + ":" + ToString(source_location.second) + "] [" + logging_function + "] ");
    }

    if (m_log_threadnames && m_started_new_line) {
        str_prefixed.insert(0, "[" + util::ThreadGetInternalName() + "] ");
    }

    str_prefixed = LogTimestampStr(str_prefixed);

    // Rate limit logging to disk to avoid disk filling attacks.
    bool log_rate_limit_message = false;
    if (!skip_disk_usage_rate_limiting && m_rate_limiting) {
        // Every hour the quota usage for a all source locations is reset.
        const std::chrono::seconds now = GetTime<std::chrono::seconds>();
        if ((now - m_last_quota_usage_reset) > std::chrono::hours{1}) {
            if (m_num_excessive_locations > 0) {
                // There are source locations that exceeded the rate limits in the last hour.

                // The total usage (bytes/messages dropped/logged) for the last hour.
                QuotaUsage total_usage;
                // Comma speparated list of source locations.
                // - all_locations: All locations that were dropped.
                // - excessive_locations: the subset of locations that tried to log excessively.
                std::string all_locations, excessive_locations;
                ComputeQuotaUsageStats(m_quota_usage_per_source_location, total_usage, all_locations, excessive_locations);
                // Log the new message as well as the "restart" message.
                str_prefixed = LogTimestampStr(
                    strprintf(
                        "Restarting logging! A total of %d messages (%.1f MiB) were dropped from these locations: %s. "
                        "Rate limiting was triggered because %d locations tried to log excessively: %s.\n%s",
                        total_usage.m_messages_dropped, total_usage.m_bytes_dropped / (1024.0 * 1024),
                        all_locations, m_num_excessive_locations, excessive_locations, str_prefixed)),

                m_num_excessive_locations = 0;
                log_rate_limit_message = true;
            }

            // Clear the quota usage stats for all locations.
            m_quota_usage_per_source_location.clear();
            m_last_quota_usage_reset = now;
        }

        QuotaUsage& quota_usage = m_quota_usage_per_source_location[source_location];

        bool quota_exceeded_before = quota_usage.m_bytes_logged > HOURLY_LOG_QUOTA_IN_BYTES_PER_SOURCE_LOCATION;
        if (!log_rate_limit_message && !quota_exceeded_before) quota_usage.m_bytes_logged += str_prefixed.size();
        bool quota_exceeded_after = quota_usage.m_bytes_logged > HOURLY_LOG_QUOTA_IN_BYTES_PER_SOURCE_LOCATION;

        if (!quota_exceeded_before && quota_exceeded_after) {
            if (m_num_excessive_locations == 0) {
                // This is the first excessively logging location.
                // We start dropping all logging to disk for up to one hour.
                str_prefixed = LogTimestampStr(
                    strprintf("Excessive logging detected from %s:%d: >%d MiB logged during the last hour. "
                              "Suppressing all logging to disk for up to one hour. "
                              "Console logging unaffected. Last log entry: %s",
                              source_location.first, source_location.second,
                              HOURLY_LOG_QUOTA_IN_BYTES_PER_SOURCE_LOCATION / (1024 * 1024), str_prefixed));
                log_rate_limit_message = true;
            }

            ++m_num_excessive_locations;
        } else if (quota_exceeded_after) {
            quota_usage.m_messages_dropped++;
            quota_usage.m_bytes_dropped += str_prefixed.size();
        }
    }

    m_started_new_line = !str.empty() && str[str.size() - 1] == '\n';

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
    if (m_print_to_file &&
        // We skip logging to disk if there is at least one source location that is trying to log excessively.
        (m_num_excessive_locations == 0 ||
         // We make an exception for messages that originate from the rate limiting logic.
         log_rate_limit_message ||
         // Ensure that the rate limiting skip flag is always respected.
         skip_disk_usage_rate_limiting)) {
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
