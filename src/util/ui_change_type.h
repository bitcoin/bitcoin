// Copyright (c) 2012-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_UI_CHANGE_TYPE_H
#define BITCOIN_UTIL_UI_CHANGE_TYPE_H

/** General change type (added, updated, removed). */
enum ChangeType {
    CT_NEW,
    CT_UPDATED,
    CT_DELETED
};

#endif // BITCOIN_UTIL_UI_CHANGE_TYPE_H
