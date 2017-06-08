// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TXHISTORYDIALOG_H
#define TXHISTORYDIALOG_H

#include "guiutil.h"
#include "uint256.h"

#include <map>

#include <QDialog>

class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QMenu;
class QModelIndex;
class QPoint;
class QResizeEvent;
class QString;
class QWidget;
QT_END_NAMESPACE

namespace Ui {
    class txHistoryDialog;
}

class HistoryTXObject
{
public:
    HistoryTXObject()
      : blockHeight(-1), blockByteOffset(0), valid(false), fundsMoved(true) {};
    int blockHeight; // block transaction was mined in
    int blockByteOffset; // byte offset the tx is stored in the block (used for ordering multiple txs same block)
    bool valid; // whether the transaction is valid from an Omni perspective
    bool fundsMoved; // whether tokens actually moved in this transaction
    std::string txType; // human readable string containing type
    std::string address; // the address to be displayed (usually sender or recipient)
    std::string amount; // string containing formatted amount
};

typedef std::map<uint256, HistoryTXObject> HistoryMap;

/** Dialog for looking up Master Protocol tokens */
class TXHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TXHistoryDialog(QWidget *parent = 0);
    ~TXHistoryDialog();

    void setClientModel(ClientModel *model);
    void setWalletModel(WalletModel *model);

    virtual void resizeEvent(QResizeEvent* event);
    std::string shrinkTxType(int txType, bool *fundsMoved);

private:
    Ui::txHistoryDialog *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    GUIUtil::TableViewLastColumnResizingFixer *borrowedColumnResizingFixer;
    QMenu *contextMenu;
    HistoryMap txHistoryMap;

private Q_SLOTS:
    void contextualMenu(const QPoint &point);
    void showDetails();
    void copyAddress();
    void copyAmount();
    void copyTxID();
    void UpdateHistory();
    int PopulateHistoryMap();
    void UpdateConfirmations();
    void checkSort(int column);

public Q_SLOTS:
    void focusTransaction(const uint256& txid);
    void ReinitTXHistoryTable();

Q_SIGNALS:
    void doubleClicked(const QModelIndex& idx);
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // TXHISTORYDIALOG_H
