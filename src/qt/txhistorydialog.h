// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TXHISTORYDIALOG_H
#define TXHISTORYDIALOG_H

#include "guiutil.h"
#include "uint256.h"

#include <QDialog>
#include <QString>
#include <QTableWidget>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QPushButton>

class ClientModel;
class OptionsModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QUrl;
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
    //void FullRefresh();
    explicit TXHistoryDialog(QWidget *parent = 0);
    void setClientModel(ClientModel *model);
    void setWalletModel(WalletModel *model);

    void accept();
    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QDialog *txDlg;
    QWidget *setupTabChain(QWidget *prev);
    QTableWidgetItem *iconCell;
    QTableWidgetItem *dateCell;
    QTableWidgetItem *typeCell;
    QTableWidgetItem *amountCell;
    QTableWidgetItem *addressCell;
    QTableWidgetItem *txidCell;
    QTableWidgetItem *sortKeyCell;
    QLayout *dlgLayout;
    QTextEdit *dlgTextEdit;
    QDialogButtonBox *buttonBox;
    QPushButton *closeButton;

    GUIUtil::TableViewLastColumnResizingFixer *borrowedColumnResizingFixer;

    virtual void resizeEvent(QResizeEvent* event);

    HistoryMap txHistoryMap;
    std::string shrinkTxType(int txType, bool *fundsMoved);

public slots:
    //void switchButtonClicked();

private:
    Ui::txHistoryDialog *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    QMenu *contextMenu;

private slots:
    void contextualMenu(const QPoint &);
    void showDetails();
    void copyAddress();
    void copyAmount();
    void copyTxID();
    void UpdateHistory();
    int PopulateHistoryMap();
    void UpdateConfirmations();
    void checkSort(int column);

signals:
    void doubleClicked(const QModelIndex&);
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // TXHISTORYDIALOG_H
