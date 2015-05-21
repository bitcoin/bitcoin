#ifndef OMNICORE_LOG_H
#define OMNICORE_LOG_H

#include "util.h"
#include "tinyformat.h"

#include <string>

/** Prints to the log file. */
int LogFilePrint(const std::string& str);

/** Prints to the console. */
int ConsolePrint(const std::string& str);

/** Determine whether to override compiled debug levels. */
void InitDebugLogLevels();

/** Scrolls log file, if it's getting too big. */
void ShrinkDebugLog();

// Log files
extern const std::string LOG_FILENAME;
extern const std::string INFO_FILENAME;
extern const std::string OWNERS_FILENAME;

// Debug flags
extern bool msc_debug_parser_data;
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

/* When we switch to C++11, this can be switched to variadic templates instead
 * of this macro-based construction (see tinyformat.h).
 */
#define MAKE_OMNI_CORE_ERROR_AND_LOG_FUNC(n)                                    \
    /** Log, if -debug=category switch is given OR category is NULL. */         \
    template<TINYFORMAT_ARGTYPES(n)>                                            \
    static inline int mp_category_log(const char* category, const char* format, TINYFORMAT_VARARGS(n)) \
    {                                                                           \
        if (!LogAcceptCategory(category)) return 0;                             \
        return LogFilePrint(tfm::format(format, TINYFORMAT_PASSARGS(n)));       \
    }                                                                           \
    template<TINYFORMAT_ARGTYPES(n)>                                            \
    static inline int file_log(const char* format, TINYFORMAT_VARARGS(n))       \
    {                                                                           \
        return LogFilePrint(tfm::format(format, TINYFORMAT_PASSARGS(n)));       \
    }                                                                           \
    template<TINYFORMAT_ARGTYPES(n)>                                            \
    static inline int file_log(TINYFORMAT_VARARGS(n))                           \
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

        
#endif // OMNICORE_LOG_H
