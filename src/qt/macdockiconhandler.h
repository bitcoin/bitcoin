// Copyright (c) 2011-2018 The Bitcoin Core developers
// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org

// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MACDOCKICONHANDLER_H
#define BITCOIN_QT_MACDOCKICONHANDLER_H

#include <QObject>

/** macOS-specific Dock icon handler.
 */
class MacDockIconHandler : public QObject
{
    Q_OBJECT

public:
    static MacDockIconHandler *instance();
    static void cleanup();

Q_SIGNALS:
    void dockIconClicked();

private:
    MacDockIconHandler();
};

#endif // BITCOIN_QT_MACDOCKICONHANDLER_H
