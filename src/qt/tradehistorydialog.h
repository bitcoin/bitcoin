// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TRADEHISTORYDIALOG_H
#define TRADEHISTORYDIALOG_H

#include "guiutil.h"
#include "walletmodel.h"

#include <QDialog>
#include <QString>
#include <QTableWidget>
#include <QMenu>
#include <QDialogButtonBox>
#include <QTextEdit>

class OptionsModel;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Ui {
    class tradeHistoryDialog;
}

class TradeHistoryObject
{

public:
  int blockHeight; // block transaction was mined in
  int blockByteOffset; // byte offset the tx is stored in the block (used for ordering multiple txs same block)
  bool valid; // whether the transaction is valid from an Omni perspective
  std::string status; // string containing status of trade
  std::string info; // string containing human readable description of trade
  std::string amountOut; // string containing formatted amount out
  std::string amountIn; // string containing formatted amount in

};

typedef std::map<uint256, TradeHistoryObject> TradeHistoryMap;

/** Dialog for looking up Master Protocol tokens */
class TradeHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    //void FullRefresh();
    explicit TradeHistoryDialog(QWidget *parent = 0);
    void setModel(WalletModel *model);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);
 //   QDialog *txDlg;
    QTableWidgetItem *iconCell;
    QTableWidgetItem *dateCell;
    QTableWidgetItem *statusCell;
    QTableWidgetItem *Cell;
    QTableWidgetItem *amountOutCell;
    QTableWidgetItem *amountInCell;
    QTableWidgetItem *txidCell;
    QLayout *dlgLayout;
    QTextEdit *dlgTextEdit;
    QDialogButtonBox *buttonBox;
    QPushButton *closeButton;

    GUIUtil::TableViewLastColumnResizingFixer *borrowedColumnResizingFixer;
    virtual void resizeEvent(QResizeEvent* event);

    TradeHistoryMap tradeHistoryMap;

public slots:
    void Update();
    void contextualMenu(const QPoint &);
    void showDetails();
    void copyTxID();

private:
    Ui::tradeHistoryDialog *ui;
    WalletModel *model;
    QMenu *contextMenu;

private slots:
    int PopulateTradeHistoryMap();

signals:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // TRADEHISTORYDIALOG_H
