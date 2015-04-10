#ifndef MASTERCOIN_SCRIPT_H
#define MASTERCOIN_SCRIPT_H

#include <string>
#include <vector>

class CScript;

#include "script/standard.h"

/** Identifies standard output types based on a scriptPubKey. */
bool GetOutputType(const CScript& scriptPubKey, txnouttype& whichTypeRet);

/** Extracts the pushed data as hex-encoded string from a script. */
bool GetScriptPushes(const CScript& script, std::vector<std::string>& vstrRet, bool fSkipFirst = false);

#endif // MASTERCOIN_SCRIPT_H
