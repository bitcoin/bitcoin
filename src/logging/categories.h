// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LOGGING_CATEGORIES_H
#define BITCOIN_LOGGING_CATEGORIES_H

#include <cstdint>

namespace BCLog {

using CategoryMask = uint64_t;

enum LogFlags : CategoryMask {
    NONE = CategoryMask{0},
    NET = (CategoryMask{1} << 0),
    TOR = (CategoryMask{1} << 1),
    MEMPOOL = (CategoryMask{1} << 2),
    HTTP = (CategoryMask{1} << 3),
    BENCH = (CategoryMask{1} << 4),
    ZMQ = (CategoryMask{1} << 5),
    WALLETDB = (CategoryMask{1} << 6),
    RPC = (CategoryMask{1} << 7),
    ESTIMATEFEE = (CategoryMask{1} << 8),
    ADDRMAN = (CategoryMask{1} << 9),
    SELECTCOINS = (CategoryMask{1} << 10),
    REINDEX = (CategoryMask{1} << 11),
    CMPCTBLOCK = (CategoryMask{1} << 12),
    RAND = (CategoryMask{1} << 13),
    PRUNE = (CategoryMask{1} << 14),
    PROXY = (CategoryMask{1} << 15),
    MEMPOOLREJ = (CategoryMask{1} << 16),
    LIBEVENT = (CategoryMask{1} << 17),
    COINDB = (CategoryMask{1} << 18),
    QT = (CategoryMask{1} << 19),
    LEVELDB = (CategoryMask{1} << 20),
    VALIDATION = (CategoryMask{1} << 21),
    I2P = (CategoryMask{1} << 22),
    IPC = (CategoryMask{1} << 23),
#ifdef DEBUG_LOCKCONTENTION
    LOCK = (CategoryMask{1} << 24),
#endif
    BLOCKSTORAGE = (CategoryMask{1} << 25),
    TXRECONCILIATION = (CategoryMask{1} << 26),
    SCAN = (CategoryMask{1} << 27),
    TXPACKAGES = (CategoryMask{1} << 28),
    KERNEL = (CategoryMask{1} << 29),
    PRIVBROADCAST = (CategoryMask{1} << 30),
    ALL = ~NONE,
};

} // namespace BCLog

#endif // BITCOIN_LOGGING_CATEGORIES_H
