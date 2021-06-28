// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fs.h>
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

// Remove the oldest rotated files if there are too many.
void BCLog::Logger::RemoveRotate()
{
    while (static_cast<int>(m_rotated_files.size()) > m_rotate_keep) {
        try {
            fs::remove(m_rotated_files.front());
        } catch (const fs::filesystem_error&) {}
        m_rotated_files.pop_front();
    }
}

void BCLog::Logger::StartRotate()
{
    // Find all the rotated log files and add them to our list.
    fs::path parent = m_file_path.parent_path();
    if (parent.empty()) parent = ".";
    for (fs::directory_iterator it(parent); it != fs::directory_iterator(); it++) {
        std::string fn{it->path().filename().string()};
        if (fs::is_regular_file(*it) &&
            fn.length() == 30 &&
            fn.substr(0,5) == "debug" &&
            fn.substr(26,4) == ".log")
        {
            m_rotated_files.push_back(it->path());
        }
    }
    std::sort(m_rotated_files.begin(), m_rotated_files.end());

    // There may be extra rotation files if m_rotate_keep config decreased.
    RemoveRotate();
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
        // Special files (e.g. device nodes) may not have a size.
        try {
            m_file_size = fs::file_size(m_file_path);
        } catch (const fs::filesystem_error&) {
            // We can't do log rotation if it's not a regular file.
            m_rotate_keep = 0;
        }
        // It gets complicated to add a timestamp to an arbitrary file name,
        // so require "debug.log".
        if (m_file_path.filename() != "debug.log") m_rotate_keep = 0;
        if (m_rotate_keep > 0) StartRotate();

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

void BCLog::Logger::LogPrintStr(const std::string& str, const std::string& logging_function, const std::string& source_file, const int source_line)
{
    StdLockGuard scoped_lock(m_cs);
    std::string str_prefixed = LogEscapeMessage(str);

    if (m_log_sourcelocations && m_started_new_line) {
        str_prefixed.insert(0, "[" + RemovePrefix(source_file, "./") + ":" + ToString(source_line) + "] [" + logging_function + "] ");
    }

    if (m_log_threadnames && m_started_new_line) {
        str_prefixed.insert(0, "[" + util::ThreadGetInternalName() + "] ");
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
            ReopenFile();
        }
        FileWriteStr(str_prefixed, m_fileout);
        m_file_size += str_prefixed.size();

        // If debug.log is large, rotate it (rename and recreate).
        if (m_rotate_keep > 0 && m_file_size > m_file_limit * 1e6) {
            // Rename debug.log to a name like debug-2021-06-25T22:57:54.log
            int64_t seconds = GetTimeMicros()/1e6;
            // Don't rotate within the same second to avoid name conflict.
            if (m_last_rotate_time < seconds) {
                m_last_rotate_time = seconds;
                std::string now = FormatISO8601DateTime(seconds);
                fs::path rotate{m_file_path};
                rotate.remove_filename();
                rotate /= "debug-" +  now + ".log";
                try {
                    fs::rename(m_file_path, rotate);
                } catch (const fs::filesystem_error&) {};
                m_rotated_files.push_back(rotate);
                RemoveRotate();

                // Recreate debug.log as an empty file.
                ReopenFile();
            }
        }
    }
}

void BCLog::Logger::ReopenFile()
{
    FILE* new_fileout = fsbridge::fopen(m_file_path, "a");
    if (new_fileout) {
        setbuf(new_fileout, nullptr); // unbuffered
        fclose(m_fileout);
        m_fileout = new_fileout;
        m_file_size = 0;
    }
}

void BCLog::Logger::ShrinkDebugFile()
{
    // Amount of debug.log to save at end when shrinking (must fit in memory)
    constexpr size_t RECENT_DEBUG_HISTORY_SIZE = 10 * 1000000;

    assert(!m_file_path.empty());

    // File rotation makes this unnecessary.
    if (m_rotate_keep > 0) return;

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
