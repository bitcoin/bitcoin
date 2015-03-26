// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BALANCESVIEW_H
#define BALANCESVIEW_H

#include "guiutil.h"

#include <QWidget>

class ClientModel;
class TransactionFilterProxy;
class WalletModel;

QT_BEGIN_NAMESPACE
class QComboBox;
class QDateTimeEdit;
class QFrame;
class QLineEdit;
class QMenu;
class QModelIndex;
class QSignalMapper;
class QTableView;
QT_END_NAMESPACE

/** Widget showing the transaction list for a wallet, including a filter row.
    Using the filter row, the user can view or export a subset of the transactions.
  */
class BalancesView : public QWidget
{
    Q_OBJECT

public:
    explicit BalancesView(QWidget *parent = 0);
    void setClientModel(ClientModel *model);
    void setWalletModel(WalletModel *model);

    void UpdateBalances();
    void UpdatePropSelector();
    // Date ranges for filter
    enum DateEnum
    {
        All,
        Today,
        ThisWeek,
        ThisMonth,
        LastMonth,
        ThisYear,
        Range
    };

    enum ColumnWidths {
        STATUS_COLUMN_WIDTH = 23,
        DATE_COLUMN_WIDTH = 120,
        TYPE_COLUMN_WIDTH = 120,
        AMOUNT_MINIMUM_COLUMN_WIDTH = 120,
        MINIMUM_COLUMN_WIDTH = 23
    };

private:
    ClientModel *clientModel;
    WalletModel *walletModel;
    TransactionFilterProxy *transactionProxyModel;
    QTableView *balancesView;
    QTableView *view;

    QLabel *propSelLabel;
    QComboBox *propSelectorWidget;
    QComboBox *typeWidget;
    QLineEdit *addressWidget;
    QLineEdit *amountWidget;

    QMenu *contextMenu;
    QSignalMapper *mapperThirdPartyTxUrls;

    QFrame *dateRangeWidget;
    QDateTimeEdit *dateFrom;
    QDateTimeEdit *dateTo;

    QWidget *createDateRangeWidget();

    GUIUtil::TableViewLastColumnResizingFixer *borrowedColumnResizingFixer;

    virtual void resizeEvent(QResizeEvent* event);

private slots:
    void contextualMenu(const QPoint &);
    //void dateRangeChanged();
    //void showDetails();
    //void copyAddress();
    //void editLabel();
    //void copyLabel();
    //void copyAmount();
    //void copyTxID();
    //void openThirdPartyTxUrl(QString url);

signals:
    void doubleClicked(const QModelIndex&);

    /**  Fired when a message should be reported to the user */
    void message(const QString &title, const QString &message, unsigned int style);

public slots:
    void propSelectorChanged(int idx);
    void balancesCopyAddress();
    void balancesCopyLabel();
    void balancesCopyAmount();
    void balancesUpdated();
};

#endif // BALANCESVIEW_H
