#ifndef BITCOIN_UTIL_DATE_DATEPARSE_H
#define BITCOIN_UTIL_DATE_DATEPARSE_H

// Try to use the std library if possible.
// gcc 14.1 and clang 19 advertize >= 202110.
//
// This header should be removed and the std namespace used directly once
// we require sufficient compiler support.

#if defined(__cpp_lib_format) && __cpp_lib_format >= 202110
namespace chrono_parse = std::chrono;
#else
#include <util/date/date.h>
namespace chrono_parse = date;
#endif

#endif // BITCOIN_UTIL_DATE_DATEPARSE_H
