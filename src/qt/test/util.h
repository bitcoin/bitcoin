// Copyright (c) 2018-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_TEST_UTIL_H
#define BITCOIN_QT_TEST_UTIL_H

#include <chrono>

#include <qglobal.h>

QT_BEGIN_NAMESPACE
class QObject;
class QString;
QT_END_NAMESPACE

/**
 * Press "Ok" button in message box dialog.
 *
 * @param text - Optionally store dialog text.
 * @param msec - Number of milliseconds to pause before triggering the callback.
 * @param context - Optionally cancel the callback when this object is destroyed,
 *                  so it cannot answer a later message box or outlive `text`.
 */
void ConfirmMessage(QString* text, std::chrono::milliseconds msec, QObject* context = nullptr);

#endif // BITCOIN_QT_TEST_UTIL_H
