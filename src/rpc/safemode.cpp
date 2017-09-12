// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "safemode.h"

#include "protocol.h"

#include "core/util.h"
#include "core/warnings.h"

void ObserveSafeMode()
{
    std::string warning = GetWarnings("rpc");
    if (warning != "" && !gArgs.GetBoolArg("-disablesafemode", DEFAULT_DISABLE_SAFEMODE)) {
        throw JSONRPCError(RPC_FORBIDDEN_BY_SAFE_MODE, std::string("Safe mode: ") + warning);
    }
}

