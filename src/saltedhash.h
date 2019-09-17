// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SALTEDHASH_H
#define BITCOIN_SALTEDHASH_H

#include <crypto/siphash.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/standard.h>
#include <uint256.h>

class SaltedTxidHasher
{
private:
    /** Salt */
    const uint64_t k0, k1;

public:
    SaltedTxidHasher();

    size_t operator()(const uint256& txid) const {
        return SipHashUint256(k0, k1, txid);
    }
};

class SaltedOutpointHasher
{
private:
    /** Salt */
    const uint64_t k0, k1;

public:
    SaltedOutpointHasher();

    /**
     * This *must* return size_t. With Boost 1.46 on 32-bit systems the
     * unordered_map will behave unpredictably if the custom hasher returns a
     * uint64_t, resulting in failures when syncing the chain (#4634).
     *
     * Having the hash noexcept allows libstdc++'s unordered_map to recalculate
     * the hash during rehash, so it does not have to cache the value. This
     * reduces node's memory by sizeof(size_t). The required recalculation has
     * a slight performance penalty (around 1.6%), but this is compensated by
     * memory savings of about 9% which allow for a larger dbcache setting.
     *
     * @see https://gcc.gnu.org/onlinedocs/gcc-9.2.0/libstdc++/manual/manual/unordered_associative.html
     */
    size_t operator()(const COutPoint& id) const noexcept {
        return SipHashUint256Extra(k0, k1, id.hash, id.n);
    }
};

class SaltedKeyIDHasher
{
private:
    /** Salt */
    uint64_t m_k0, m_k1;

public:
    SaltedKeyIDHasher();

    size_t operator()(const CKeyID& id) const;
};

class SaltedScriptIDHasher
{
private:
    /** Salt */
    const uint64_t m_k0, m_k1;

public:
    SaltedScriptIDHasher();

    size_t operator()(const CScriptID& id) const;
};

class SaltedScriptHasher
{
private:
    /** Salt */
    const uint64_t m_k0, m_k1;

public:
    SaltedScriptHasher();

    size_t operator()(const CScript& script) const;
};

class SaltedPubkeyHasher
{
private:
    /** Salt */
    const uint64_t m_k0, m_k1;

public:
    SaltedPubkeyHasher();

    size_t operator()(const CPubKey& pubkey) const;
};

#endif // BITCOIN_SALTEDHASH_H
