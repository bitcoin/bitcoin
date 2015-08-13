#ifndef OMNICORE_MBSTRING_H
#define OMNICORE_MBSTRING_H

#include <stdint.h>
#include <string>

namespace mastercore
{
/** Replaces invalid UTF-8 characters or character sequences with question marks. */
std::string SanitizeInvalidUTF8(const std::string& s);
}

#endif // OMNICORE_MBSTRING_H
