#ifndef OMNICORE_UTILS_H
#define OMNICORE_UTILS_H

class CPubKey;
class CTxOut;

#include <string>
#include "script/script.h"
#include "script/standard.h"

void prepareObfuscatedHashes(const string &address, string (&ObfsHashes)[1+MAX_SHA256_OBFUSCATION_TIMES]);
int64_t GetDustLimit(const CScript& scriptPubKey);

#endif // OMNICORE_UTILS_H



