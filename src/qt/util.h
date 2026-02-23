// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_UTIL_H
#define BITCOIN_QT_UTIL_H

#include <QByteArray>

namespace util {
template <typename T1>
inline QByteArray make_array(const T1& data)
{
    return QByteArray(reinterpret_cast<const char*>(data.data()), data.size());
}
} // namespace util

#endif // BITCOIN_QT_UTIL_H
