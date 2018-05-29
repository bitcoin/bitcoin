// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_ISMINE_H
#define BITCOIN_SCRIPT_ISMINE_H

#include <script/standard.h>

#include <stdint.h>

class CKeyStore;
class CScript;

/** IsMine() return codes */
enum isminetype : uint8_t
{
    ISMINE_NO = 0,
    ISMINE_WATCH_ONLY_ = 1,
    ISMINE_SPENDABLE = 2,
    ISMINE_WATCH_COLDSTAKE = (1 << 7),
    ISMINE_WATCH_ONLY = ISMINE_WATCH_ONLY_ | ISMINE_WATCH_COLDSTAKE,
    ISMINE_HARDWARE_DEVICE = (1 << 6), // Pivate key is on external device
    ISMINE_ALL = ISMINE_WATCH_ONLY | ISMINE_SPENDABLE
};
/** used for bitflags of isminetype */
typedef uint8_t isminefilter;

typedef std::vector<unsigned char> valtype;
bool HaveKeys(const std::vector<valtype>& pubkeys, const CKeyStore& keystore);

/* isInvalid becomes true when the script is found invalid by consensus or policy. This will terminate the recursion
 * and return ISMINE_NO immediately, as an invalid script should never be considered as "mine". This is needed as
 * different SIGVERSION may have different network rules. Currently the only use of isInvalid is indicate uncompressed
 * keys in SigVersion::WITNESS_V0 script, but could also be used in similar cases in the future
 */
isminetype IsMine(const CKeyStore& keystore, const CScript& scriptPubKey, bool& isInvalid);
isminetype IsMine(const CKeyStore& keystore, const CScript& scriptPubKey);
isminetype IsMine(const CKeyStore& keystore, const CTxDestination& dest);
isminetype IsMineP2SH(const CKeyStore& keystore, const CScript& scriptPubKey);

#endif // BITCOIN_SCRIPT_ISMINE_H
