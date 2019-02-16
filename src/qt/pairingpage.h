// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PAIRINGPAGE_H
#define BITCOIN_QT_PAIRINGPAGE_H

#include <QWidget>

class PairingPage : public QWidget
{
    Q_OBJECT

public:
    explicit PairingPage(QWidget *parent = nullptr);
    ~PairingPage() {}
};

#endif // BITCOIN_QT_PAIRINGPAGE_H
