#ifndef _MASTERCOIN_PARSE_STRING
#define _MASTERCOIN_PARSE_STRING

#include <stdint.h>
#include <string>

namespace mastercore
{
// Converts strings to 64 bit wide interger.
// Divisible and indivisible amounts are accepted.
// 1 indivisible unit equals 0.00000001 divisible units.
// If input string is not a accepted number, 0 is returned.
// Divisible amounts are truncated after 8 decimal places.
// Characters after decimal mark are ignored for indivisible
// amounts.
// Any minus sign invalidates.
int64_t StrToInt64(const std::string& str, bool divisible);
}

#endif // _MASTERCOIN_PARSE_STRRING
