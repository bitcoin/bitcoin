// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_TEST_SHAREDMNWIDGETTESTS_H
#define BITCOIN_QT_TEST_SHAREDMNWIDGETTESTS_H

#include <QObject>
#include <QTest>

class SharedMnWidgetTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void importDetection();
    void statusBoardCells();
    void termSheetEscapesUntrustedText();
    void termSheetWarnsWhenWePayMoreThanOurShare();
};

#endif // BITCOIN_QT_TEST_SHAREDMNWIDGETTESTS_H
