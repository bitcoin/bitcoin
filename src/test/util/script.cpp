// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/interpreter.h>
#include <script/script.h>
#include <test/util/random.h>
#include <test/util/script.h>

bool IsValidFlagCombination(unsigned flags)
{
    if (flags & SCRIPT_VERIFY_CLEANSTACK && ~flags & (SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS)) return false;
    if (flags & SCRIPT_VERIFY_WITNESS && ~flags & SCRIPT_VERIFY_P2SH) return false;
    return true;
}

CScript RandScript(FastRandomContext& rng, uint16_t length)
{
    assert(length <= MAX_SCRIPT_SIZE && length >= 1);
    // CScript's << operator pushes data onto the script with a 1-byte PUSH_DATA
    return CScript() << rng.randbytes(length - 1);
}
