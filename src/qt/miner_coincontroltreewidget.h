// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MINER_COINCONTROLTREEWIDGET_H
#define MINER_COINCONTROLTREEWIDGET_H

#include <QKeyEvent>
#include <QTreeWidget>

class Miner_CoinControlTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    explicit Miner_CoinControlTreeWidget(QWidget *parent = 0);

protected:
    virtual void keyPressEvent(QKeyEvent *event);
};

#endif // MINER_COINCONTROLTREEWIDGET_H
