// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/validation.h>

#include <consensus/validation.h>
#include <tinyformat.h>

/** Convert ValidationState to a human-readable message for logging */
std::string FormatStateMessage(const ValidationState &state)
{
    if (state.IsValid()) {
        return "Valid";
    }

    return strprintf("%s%s",
        state.GetRejectReason(),
        state.GetDebugMessage().empty() ? "" : ", "+state.GetDebugMessage());
}

const std::string strMessageMagic = "Bitcoin Signed Message:\n";
