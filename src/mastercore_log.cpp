#include "mastercore_log.h"

#include "chainparamsbase.h"
#include "util.h"
#include "utiltime.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/once.hpp>

#include <assert.h>
#include <stdio.h>
#include <string>

// Log files
const std::string LOG_FILENAME    = "mastercore.log";
const std::string INFO_FILENAME   = "mastercore_crowdsales.log";
const std::string OWNERS_FILENAME = "mastercore_owners.log";

// Options
static const long LOG_BUFFERSIZE  =  8000000; //  8 MB
static const long LOG_SHRINKSIZE  = 50000000; // 50 MB

// Debug flags
bool msc_debug_parser_data        = 0;
bool msc_debug_parser             = 0;
bool msc_debug_verbose            = 0;
bool msc_debug_verbose2           = 0;
bool msc_debug_verbose3           = 0;
bool msc_debug_vin                = 0;
bool msc_debug_script             = 0;
bool msc_debug_dex                = 1;
bool msc_debug_send               = 1;
bool msc_debug_tokens             = 0;
bool msc_debug_spec               = 1;
bool msc_debug_exo                = 0;
bool msc_debug_tally              = 1;
bool msc_debug_sp                 = 1;
bool msc_debug_sto                = 1;
bool msc_debug_txdb               = 0;
bool msc_debug_tradedb            = 1;
bool msc_debug_persistence        = 0;
bool msc_debug_ui                 = 0;
bool msc_debug_metadex1           = 0;
bool msc_debug_metadex2           = 0;
// Print orderbook before and after each transaction
bool msc_debug_metadex3           = 0;

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
static boost::once_flag debugLogInitFlag = BOOST_ONCE_INIT;
/**
 * We use boost::call_once() to make sure these are initialized
 * in a thread-safe manner the first time called:
 */
static FILE* fileout = NULL;
static boost::mutex* mutexDebugLog = NULL;

/**
 * Open debug log file.
 */
static void DebugLogInit()
{
    assert(fileout == NULL);
    assert(mutexDebugLog == NULL);

    boost::filesystem::path pathDebug = GetDataDir() / LOG_FILENAME;
    fileout = fopen(pathDebug.string().c_str(), "a");
    if (fileout) setbuf(fileout, NULL); // Unbuffered

    mutexDebugLog = new boost::mutex();
}

/**
 * @return The current timestamp in the format: 2015-04-15 11:18:01
 */
static std::string GetTimestamp()
{
    return DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime());
}

/**
 * Print to debug log file or console.
 *
 * The configuration options "-printtoconsole" and "-logtimestamps" can be used
 * to indicate, whether to add log entries to a file or print to the console,
 * and if the message should be prepended with a timestamp.
 *
 * @param str[in]  The message to log
 * @return The total number of characters written
 */
int DebugLogPrint(const std::string& str)
{
    int ret = 0; // Number of characters written
    if (fPrintToConsole) {
        // Print to console
        ret = StatusLogPrint(str);
    }
    else if (fPrintToDebugLog && AreBaseParamsConfigured()) {
        static bool fStartedNewLine = true;
        boost::call_once(&DebugLogInit, debugLogInitFlag);

        if (fileout == NULL) {
            return ret;
        }
        boost::mutex::scoped_lock scoped_lock(*mutexDebugLog);

        // Reopen the log file, if requested
        if (fReopenDebugLog) {
            fReopenDebugLog = false;
            boost::filesystem::path pathDebug = GetDataDir() / LOG_FILENAME;
            if (freopen(pathDebug.string().c_str(), "a", fileout) != NULL) {
                setbuf(fileout, NULL); // Unbuffered
            }
        }

        // Printing log timestamps can be useful for profiling
        if (fLogTimestamps && fStartedNewLine) {
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
 * Print to the console.
 *
 * The configuration options "-logtimestamps" can be used to indicate, whether
 * the message should be prepended with a timestamp.
 *
 * @param str[in]  The message to log
 * @return The total number of characters written
 */
int StatusLogPrint(const std::string& str)
{
    int ret = 0; // Number of characters written
    static bool fStartedNewLine = true;

    if (fLogTimestamps && fStartedNewLine) {
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
 * Scroll debug log, if it's getting too big.
 */
void ShrinkDebugLog()
{
    boost::filesystem::path pathLog = GetDataDir() / LOG_FILENAME;
    FILE* file = fopen(pathLog.string().c_str(), "r");

    if (file && boost::filesystem::file_size(pathLog) > LOG_SHRINKSIZE) {
        // Restart the file with some of the end
        char* pch = new char[LOG_BUFFERSIZE];
        if (NULL != pch) {
            fseek(file, -LOG_BUFFERSIZE, SEEK_END);
            int nBytes = fread(pch, 1, LOG_BUFFERSIZE, file);
            fclose(file);
            file = NULL;

            file = fopen(pathLog.string().c_str(), "w");
            if (file) {
                fwrite(pch, 1, nBytes, file);
                fclose(file);
                file = NULL;
            }
            delete[] pch;
        }
    } else if (NULL != file) {
        fclose(file);
        file = NULL;
    }
}

