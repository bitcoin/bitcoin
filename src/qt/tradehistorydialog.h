// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TRADEHISTORYDIALOG_H
#define TRADEHISTORYDIALOG_H

#include "guiutil.h"
#include "uint256.h"

#include <QDialog>

class WalletModel;
class ClientModel;

QT_BEGIN_NAMESPACE
class QMenu;
class QPoint;
class QResizeEvent;
class QString;
class QWidget;
QT_END_NAMESPACE

namespace Ui {
    class tradeHistoryDialog;
}

class TradeHistoryObject
{
public:
    TradeHistoryObject()
      : blockHeight(-1), valid(false) {};
    int blockHeight; // block transaction was mined in
    bool valid; // whether the transaction is valid from an Omni perspective
    uint32_t propertyIdForSale; // the property being sold
    uint32_t propertyIdDesired; // the property being requested
    int64_t amountForSale; // the amount being sold
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
    explicit TradeHistoryDialog(QWidget *parent = 0);
    ~TradeHistoryDialog();
    void setWalletModel(WalletModel *model);
    void setClientModel(ClientModel *model);
    GUIUtil::TableViewLastColumnResizingFixer *borrowedColumnResizingFixer;
    virtual void resizeEvent(QResizeEvent* event);

private:
    Ui::tradeHistoryDialog *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    QMenu *contextMenu;
    TradeHistoryMap tradeHistoryMap;

public Q_SLOTS:
    void contextualMenu(const QPoint &point);
    void showDetails();
    void copyTxID();

private Q_SLOTS:
    int PopulateTradeHistoryMap();
    void UpdateData();
    void UpdateTradeHistoryTable(bool forceUpdate = false);
    void RepopulateTradeHistoryTable(int hide);
    void ReinitTradeHistoryTable();

Q_SIGNALS:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // TRADEHISTORYDIALOG_H
