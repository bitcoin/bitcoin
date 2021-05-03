#include <omnicore/log.h>

#include <chainparamsbase.h>
#include <fs.h>
#include <logging.h>
#include <util/system.h>
#include <util/time.h>

#include <assert.h>
#include <stdio.h>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

// Default log files
const std::string LOG_FILENAME    = "omnicore.log";

// Options
static const long LOG_BUFFERSIZE  =  8000000; //  8 MB
static const long LOG_SHRINKSIZE  = 50000000; // 50 MB

// Debug flags
bool msc_debug_parser_data        = 0;
bool msc_debug_parser_readonly    = 0;
//! Print information to potential DEx payments and outputs
bool msc_debug_parser_dex         = 1;
bool msc_debug_parser             = 0;
bool msc_debug_verbose            = 0;
bool msc_debug_verbose2           = 0;
bool msc_debug_verbose3           = 0;
bool msc_debug_vin                = 0;
bool msc_debug_script             = 0;
bool msc_debug_dex                = 1;
bool msc_debug_send               = 1;
bool msc_debug_tokens             = 0;
//! Print information about payloads with non-sequential sequence number
bool msc_debug_spec               = 0;
bool msc_debug_exo                = 0;
bool msc_debug_tally              = 1;
bool msc_debug_sp                 = 1;
bool msc_debug_sto                = 1;
bool msc_debug_txdb               = 0;
bool msc_debug_tradedb            = 1;
bool msc_debug_persistence        = 0;
bool msc_debug_ui                 = 0;
bool msc_debug_pending            = 1;
bool msc_debug_metadex1           = 0;
bool msc_debug_metadex2           = 0;
//! Print orderbook before and after each trade
bool msc_debug_metadex3           = 0;
//! Print transaction fields, when interpreting packets
bool msc_debug_packets            = 1;
//! Print transaction fields, when interpreting packets (in RPC mode)
bool msc_debug_packets_readonly   = 0;
bool msc_debug_walletcache        = 0;
//! Print each line added to consensus hash
bool msc_debug_consensus_hash     = 0;
//! Print consensus hashes for each block when parsing
bool msc_debug_consensus_hash_every_block = 0;
//! Print extra info on alert processing
bool msc_debug_alerts             = 1;
//! Print consensus hashes for each transaction when parsing
bool msc_debug_consensus_hash_every_transaction = 0;
//! Debug fees
bool msc_debug_fees               = 1;
//! Debug the non-fungible tokens database
bool msc_debug_nftdb              = 0;

/**
 * LogPrintf() has been broken a couple of times now
 * by well-meaning people adding mutexes in the most straightforward way.
 * It breaks because it may be called by global destructors during shutdown.
 * Since the order of destruction of static/global objects is undefined,
 * defining a mutex as a global object doesn't work (the mutex gets
 * destroyed, and then some later destructor calls OutputDebugStringF,
 * maybe indirectly, and you get a core dump at shutdown trying to lock
 * the mutex).
 */
static std::once_flag debugLogInitFlag;
/**
 * We use std::call_once() to make sure these are initialized
 * in a thread-safe manner the first time called:
 */
static FILE* fileout = nullptr;
static std::mutex* mutexDebugLog = nullptr;
/** Flag to indicate, whether the Omni Core log file should be reopened. */
extern std::atomic<bool> fReopenOmniCoreLog;
/**
 * Returns path for debug log file.
 *
 * The log file can be specified via startup option "--omnilogfile=/path/to/omnicore.log",
 * and if none is provided, then the client's datadir is used as default location.
 */
static fs::path GetLogPath()
{
    fs::path pathLogFile;
    std::string strLogPath = gArgs.GetArg("-omnilogfile", "");

    if (!strLogPath.empty()) {
        pathLogFile = fs::path(strLogPath);
        TryCreateDirectories(pathLogFile.parent_path());
    } else {
        pathLogFile = GetDataDir() / LOG_FILENAME;
    }

    return pathLogFile;
}

/**
 * Opens debug log file.
 */
static void DebugLogInit()
{
    assert(fileout == nullptr);
    assert(mutexDebugLog == nullptr);

    fs::path pathDebug = GetLogPath();
    fileout = fopen(pathDebug.string().c_str(), "a");

    if (fileout) {
        setbuf(fileout, nullptr); // Unbuffered
    } else {
        PrintToConsole("Failed to open debug log file: %s\n", pathDebug.string());
    }

    mutexDebugLog = new std::mutex();
}

/**
 * @return The current timestamp in the format: 2009-01-03 18:15:05
 */
static std::string GetTimestamp()
{
    return FormatISO8601DateTime(GetTime());
}

/**
 * Prints to log file.
 *
 * The configuration options "-logtimestamps" can be used to indicate, whether
 * the message to log should be prepended with a timestamp.
 *
 * If "-printtoconsole" is enabled, then the message is written to the standard
 * output, usually the console, instead of a log file.
 *
 * @param str[in]  The message to log
 * @return The total number of characters written
 */
int LogFilePrint(const std::string& str)
{
    int ret = 0; // Number of characters written
    if (LogInstance().m_print_to_console || fOmniCoreConsoleLog) {
        // Print to console
        ret = ConsolePrint(str);
    }
    else if (LogInstance().m_print_to_file) {
        static bool fStartedNewLine = true;
        std::call_once(debugLogInitFlag, &DebugLogInit);

        if (fileout == nullptr) {
            return ret;
        }
        std::lock_guard<std::mutex> lock(*mutexDebugLog);

        // Reopen the log file, if requested
        if (fReopenOmniCoreLog) {
            fReopenOmniCoreLog = false;
            fs::path pathDebug = GetLogPath();
            if (freopen(pathDebug.string().c_str(), "a", fileout) != nullptr) {
                setbuf(fileout, nullptr); // Unbuffered
            }
        }

        // Printing log timestamps can be useful for profiling
        if (LogInstance().m_log_timestamps && fStartedNewLine) {
            ret += fprintf(fileout, "%s ", GetTimestamp().c_str());
        }
        if (!str.empty() && str[str.size()-1] == '\n') {
            fStartedNewLine = true;
        } else {
            fStartedNewLine = false;
        }
        ret += fwrite(str.data(), 1, str.size(), fileout);
    }

    return ret;
}

/**
 * Prints to the standard output, usually the console.
 *
 * The configuration option "-logtimestamps" can be used to indicate, whether
 * the message should be prepended with a timestamp.
 *
 * @param str[in]  The message to print
 * @return The total number of characters written
 */
int ConsolePrint(const std::string& str)
{
    int ret = 0; // Number of characters written
    static bool fStartedNewLine = true;

    if (LogInstance().m_log_timestamps && fStartedNewLine) {
        ret = fprintf(stdout, "%s %s", GetTimestamp().c_str(), str.c_str());
    } else {
        ret = fwrite(str.data(), 1, str.size(), stdout);
    }
    if (!str.empty() && str[str.size()-1] == '\n') {
        fStartedNewLine = true;
    } else {
        fStartedNewLine = false;
    }
    fflush(stdout);

    return ret;
}

/**
 * Determine whether to override compiled debug levels via enumerating startup option --omnidebug.
 *
 * Example usage (granular categories)    : --omnidebug=parser --omnidebug=metadex1 --omnidebug=ui
 * Example usage (enable all categories)  : --omnidebug=all
 * Example usage (disable all debugging)  : --omnidebug=none
 * Example usage (disable all except XYZ) : --omnidebug=none --omnidebug=parser --omnidebug=sto
 */
void InitDebugLogLevels()
{
    if (!gArgs.IsArgSet("-omnidebug")) {
        return;
    }

    const std::vector<std::string>& debugLevels = gArgs.GetArgs("-omnidebug");

    for (std::vector<std::string>::const_iterator it = debugLevels.begin(); it != debugLevels.end(); ++it) {
        if (*it == "parser_data") msc_debug_parser_data = true;
        if (*it == "parser_readonly") msc_debug_parser_readonly = true;
        if (*it == "parser_dex") msc_debug_parser_dex = true;
        if (*it == "parser") msc_debug_parser = true;
        if (*it == "verbose") msc_debug_verbose = true;
        if (*it == "verbose2") msc_debug_verbose2 = true;
        if (*it == "verbose3") msc_debug_verbose3 = true;
        if (*it == "vin") msc_debug_vin = true;
        if (*it == "script") msc_debug_script = true;
        if (*it == "dex") msc_debug_dex = true;
        if (*it == "send") msc_debug_send = true;
        if (*it == "tokens") msc_debug_tokens = true;
        if (*it == "spec") msc_debug_spec = true;
        if (*it == "exo") msc_debug_exo = true;
        if (*it == "tally") msc_debug_tally = true;
        if (*it == "sp") msc_debug_sp = true;
        if (*it == "sto") msc_debug_sto = true;
        if (*it == "txdb") msc_debug_txdb = true;
        if (*it == "tradedb") msc_debug_tradedb = true;
        if (*it == "persistence") msc_debug_persistence = true;
        if (*it == "ui") msc_debug_ui = true;
        if (*it == "pending") msc_debug_pending = true;
        if (*it == "metadex1") msc_debug_metadex1 = true;
        if (*it == "metadex2") msc_debug_metadex2 = true;
        if (*it == "metadex3") msc_debug_metadex3 = true;
        if (*it == "packets") msc_debug_packets = true;
        if (*it == "packets_readonly") msc_debug_packets_readonly = true;
        if (*it == "walletcache") msc_debug_walletcache = true;
        if (*it == "consensus_hash") msc_debug_consensus_hash = true;
        if (*it == "consensus_hash_every_block") msc_debug_consensus_hash_every_block = true;
        if (*it == "alerts") msc_debug_alerts = true;
        if (*it == "consensus_hash_every_transaction") msc_debug_consensus_hash_every_transaction = true;
        if (*it == "fees") msc_debug_fees = true;
        if (*it == "nftdb") msc_debug_nftdb = true;
        if (*it == "none" || *it == "all") {
            bool allDebugState = false;
            if (*it == "all") allDebugState = true;
            msc_debug_parser_data = allDebugState;
            msc_debug_parser_readonly = allDebugState;
            msc_debug_parser_dex = allDebugState;
            msc_debug_parser = allDebugState;
            msc_debug_verbose = allDebugState;
            msc_debug_verbose2 = allDebugState;
            msc_debug_verbose3 = allDebugState;
            msc_debug_vin = allDebugState;
            msc_debug_script = allDebugState;
            msc_debug_dex = allDebugState;
            msc_debug_send = allDebugState;
            msc_debug_tokens = allDebugState;
            msc_debug_spec = allDebugState;
            msc_debug_exo = allDebugState;
            msc_debug_tally = allDebugState;
            msc_debug_sp = allDebugState;
            msc_debug_sto = allDebugState;
            msc_debug_txdb = allDebugState;
            msc_debug_tradedb = allDebugState;
            msc_debug_persistence = allDebugState;
            msc_debug_ui = allDebugState;
            msc_debug_pending = allDebugState;
            msc_debug_metadex1 = allDebugState;
            msc_debug_metadex2 = allDebugState;
            msc_debug_metadex3 = allDebugState;
            msc_debug_packets =  allDebugState;
            msc_debug_packets_readonly =  allDebugState;
            msc_debug_walletcache = allDebugState;
            msc_debug_consensus_hash = allDebugState;
            msc_debug_consensus_hash_every_block = allDebugState;
            msc_debug_alerts = allDebugState;
            msc_debug_consensus_hash_every_transaction = allDebugState;
            msc_debug_fees = allDebugState;
            msc_debug_nftdb = allDebugState;
        }
    }
}

/**
 * Scrolls debug log, if it's getting too big.
 */
void ShrinkDebugLog()
{
    fs::path pathLog = GetLogPath();
    FILE* file = fopen(pathLog.string().c_str(), "r");

    if (file && fs::file_size(pathLog) > LOG_SHRINKSIZE) {
        // Restart the file with some of the end
        char* pch = new char[LOG_BUFFERSIZE];
        if (nullptr != pch) {
            fseek(file, -LOG_BUFFERSIZE, SEEK_END);
            int nBytes = fread(pch, 1, LOG_BUFFERSIZE, file);
            fclose(file);
            file = nullptr;

            file = fopen(pathLog.string().c_str(), "w");
            if (file) {
                fwrite(pch, 1, nBytes, file);
                fclose(file);
                file = nullptr;
            }
            delete[] pch;
        }
    } else if (nullptr != file) {
        fclose(file);
        file = nullptr;
    }
}

