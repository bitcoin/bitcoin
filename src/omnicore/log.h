#ifndef BITCOIN_OMNICORE_LOG_H
#define BITCOIN_OMNICORE_LOG_H

#include <util/system.h>
#include <tinyformat.h>

#include <string>

/** Prints to the log file. */
int LogFilePrint(const std::string& str);

/** Prints to the console. */
int ConsolePrint(const std::string& str);

/** Determine whether to override compiled debug levels. */
void InitDebugLogLevels();

/** Scrolls log file, if it's getting too big. */
void ShrinkDebugLog();

// Debug flags
extern bool msc_debug_parser_data;
extern bool msc_debug_parser_readonly;
extern bool msc_debug_parser_dex;
extern bool msc_debug_parser;
extern bool msc_debug_verbose;
extern bool msc_debug_verbose2;
extern bool msc_debug_verbose3;
extern bool msc_debug_vin;
extern bool msc_debug_script;
extern bool msc_debug_dex;
extern bool msc_debug_send;
extern bool msc_debug_tokens;
extern bool msc_debug_spec;
extern bool msc_debug_exo;
extern bool msc_debug_tally;
extern bool msc_debug_sp;
extern bool msc_debug_sto;
extern bool msc_debug_txdb;
extern bool msc_debug_tradedb;
extern bool msc_debug_persistence;
extern bool msc_debug_ui;
extern bool msc_debug_pending;
extern bool msc_debug_metadex1;
extern bool msc_debug_metadex2;
extern bool msc_debug_metadex3;
extern bool msc_debug_packets;
extern bool msc_debug_packets_readonly;
extern bool msc_debug_walletcache;
extern bool msc_debug_consensus_hash;
extern bool msc_debug_consensus_hash_every_block;
extern bool msc_debug_alerts;
extern bool msc_debug_consensus_hash_every_transaction;
extern bool msc_debug_fees;
extern bool msc_debug_nftdb;

/* When we switch to C++11, this can be switched to variadic templates instead
 * of this macro-based construction (see tinyformat.h).
 */
#define MAKE_OMNI_CORE_ERROR_AND_LOG_FUNC(n)                                    \
    template<TINYFORMAT_ARGTYPES(n)>                                            \
    static inline int PrintToLog(const char* format, TINYFORMAT_VARARGS(n))     \
    {                                                                           \
        return LogFilePrint(tfm::format(format, TINYFORMAT_PASSARGS(n)));       \
    }                                                                           \
    template<TINYFORMAT_ARGTYPES(n)>                                            \
    static inline int PrintToLog(TINYFORMAT_VARARGS(n))                         \
    {                                                                           \
        return LogFilePrint(tfm::format("%s", TINYFORMAT_PASSARGS(n)));         \
    }                                                                           \
    template<TINYFORMAT_ARGTYPES(n)>                                            \
    static inline int PrintToConsole(const char* format, TINYFORMAT_VARARGS(n)) \
    {                                                                           \
        return ConsolePrint(tfm::format(format, TINYFORMAT_PASSARGS(n)));       \
    }                                                                           \
    template<TINYFORMAT_ARGTYPES(n)>                                            \
    static inline int PrintToConsole(TINYFORMAT_VARARGS(n))                     \
    {                                                                           \
        return ConsolePrint(tfm::format("%s", TINYFORMAT_PASSARGS(n)));         \
    }

TINYFORMAT_FOREACH_ARGNUM(MAKE_OMNI_CORE_ERROR_AND_LOG_FUNC)

#undef MAKE_OMNI_CORE_ERROR_AND_LOG_FUNC


#endif // BITCOIN_OMNICORE_LOG_H
