#ifndef BITCOIN_OMNICORE_RPCMBSTRING_H
#define BITCOIN_OMNICORE_RPCMBSTRING_H

#include <stdint.h>
#include <string>

namespace mastercore
{
/** Replaces invalid UTF-8 characters or character sequences with question marks. */
std::string SanitizeInvalidUTF8(const std::string& s);
}

#endif // BITCOIN_OMNICORE_RPCMBSTRING_H
