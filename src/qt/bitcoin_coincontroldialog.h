// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINCONTROLDIALOG_H
#define BITCOIN_COINCONTROLDIALOG_H

#include <QAbstractButton>
#include <QAction>
#include <QDialog>
#include <QList>
#include <QMenu>
#include <QPoint>
#include <QString>
#include <QTreeWidgetItem>

namespace Ui {
    class Bitcoin_CoinControlDialog;
}
class Bitcoin_WalletModel;
class Bitcredit_WalletModel;
class CCoinControl;

class Bitcoin_CoinControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit Bitcoin_CoinControlDialog(QWidget *parent = 0);
    ~Bitcoin_CoinControlDialog();

    void setModel(Bitcoin_WalletModel *bitcoin_model, Bitcredit_WalletModel *bitcredit_model);

    // static because also called from sendcoinsdialog
    static void updateLabels(Bitcoin_WalletModel*, Bitcredit_WalletModel *bitcredit_model, QDialog*);
    static QString getPriorityLabel(double);

    static qint64 amountAfterFee;
    static QList<qint64> payAmounts;
    static CCoinControl *coinControl;

private:
    Ui::Bitcoin_CoinControlDialog *ui;
    Bitcoin_WalletModel *bitcoin_model;
    Bitcredit_WalletModel *bitcredit_model;
    int sortColumn;
    Qt::SortOrder sortOrder;

    QMenu *contextMenu;
    QTreeWidgetItem *contextMenuItem;
    QAction *copyTransactionHashAction;
    QAction *lockAction;
    QAction *unlockAction;

    QString strPad(QString, int, QString);
    void sortView(int, Qt::SortOrder);
    void updateView();

    enum
    {
        COLUMN_CHECKBOX,
        COLUMN_AMOUNT,
        COLUMN_LABEL,
        COLUMN_ADDRESS,
        COLUMN_DATE,
        COLUMN_CONFIRMATIONS,
        COLUMN_PRIORITY,
        COLUMN_TXHASH,
        COLUMN_VOUT_INDEX,
        COLUMN_AMOUNT_INT64,
        COLUMN_PRIORITY_INT64,
        COLUMN_DATE_INT64
    };

    // some columns have a hidden column containing the value used for sorting
    int getMappedColumn(int column, bool fVisibleColumn = true)
    {
        if (fVisibleColumn)
        {
            if (column == COLUMN_AMOUNT_INT64)
                return COLUMN_AMOUNT;
            else if (column == COLUMN_PRIORITY_INT64)
                return COLUMN_PRIORITY;
            else if (column == COLUMN_DATE_INT64)
                return COLUMN_DATE;
        }
        else
        {
            if (column == COLUMN_AMOUNT)
                return COLUMN_AMOUNT_INT64;
            else if (column == COLUMN_PRIORITY)
                return COLUMN_PRIORITY_INT64;
            else if (column == COLUMN_DATE)
                return COLUMN_DATE_INT64;
        }

        return column;
    }

private slots:
    void showMenu(const QPoint &);
    void copyAmount();
    void copyLabel();
    void copyAddress();
    void copyTransactionHash();
    void lockCoin();
    void unlockCoin();
    void clipboardQuantity();
    void clipboardAmount();
    void clipboardFee();
    void clipboardAfterFee();
    void clipboardBytes();
    void clipboardPriority();
    void clipboardLowOutput();
    void radioTreeMode(bool);
    void radioListMode(bool);
    void viewItemChanged(QTreeWidgetItem*, int);
    void headerSectionClicked(int);
    void buttonBoxClicked(QAbstractButton*);
    void buttonSelectAllClicked();
    void updateLabelLocked();
};

#endif // BITCOIN_COINCONTROLDIALOG_H
