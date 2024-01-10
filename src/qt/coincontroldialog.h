// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_COINCONTROLDIALOG_H
#define BITCOIN_QT_COINCONTROLDIALOG_H

#include <consensus/amount.h>

#include <QAbstractButton>
#include <QAction>
#include <QDialog>
#include <QList>
#include <QMenu>
#include <QPoint>
#include <QString>
#include <QTreeWidgetItem>

class PlatformStyle;
class WalletModel;

namespace wallet {
class CCoinControl;
} // namespace wallet

namespace Ui {
    class CoinControlDialog;
}

#define ASYMP_UTF8 "\xE2\x89\x88"

class CCoinControlWidgetItem : public QTreeWidgetItem
{
public:
    explicit CCoinControlWidgetItem(QTreeWidget *parent, int type = Type) : QTreeWidgetItem(parent, type) {}
    explicit CCoinControlWidgetItem(QTreeWidgetItem *parent, int type = Type) : QTreeWidgetItem(parent, type) {}

    bool operator<(const QTreeWidgetItem &other) const override;
};


class CoinControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CoinControlDialog(wallet::CCoinControl& coin_control, WalletModel* model, const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~CoinControlDialog();

    // static because also called from sendcoinsdialog
    static void updateLabels(wallet::CCoinControl& m_coin_control, WalletModel*, QDialog*);

    static QList<CAmount> payAmounts;
    static bool fSubtractFeeFromAmount;

protected:
    void changeEvent(QEvent* e) override;

private:
    Ui::CoinControlDialog *ui;
    wallet::CCoinControl& m_coin_control;
    WalletModel *model;
    int sortColumn;
    Qt::SortOrder sortOrder;

    QMenu *contextMenu;
    QTreeWidgetItem *contextMenuItem;
    QAction* m_copy_transaction_outpoint_action;
    QAction *lockAction;
    QAction *unlockAction;

    const PlatformStyle *platformStyle;

    void sortView(int, Qt::SortOrder);
    void updateView();

    enum
    {
        COLUMN_CHECKBOX = 0,
        COLUMN_AMOUNT,
        COLUMN_LABEL,
        COLUMN_ADDRESS,
        COLUMN_DATE,
        COLUMN_CONFIRMATIONS,
    };

    enum
    {
        TxHashRole = Qt::UserRole,
        VOutRole
    };

    friend class CCoinControlWidgetItem;

private Q_SLOTS:
    void showMenu(const QPoint &);
    void copyAmount();
    void copyLabel();
    void copyAddress();
    void copyTransactionOutpoint();
    void lockCoin();
    void unlockCoin();
    void clipboardQuantity();
    void clipboardAmount();
    void clipboardFee();
    void clipboardAfterFee();
    void clipboardBytes();
    void clipboardChange();
    void radioTreeMode(bool);
    void radioListMode(bool);
    void viewItemChanged(QTreeWidgetItem*, int);
    void headerSectionClicked(int);
    void buttonBoxClicked(QAbstractButton*);
    void buttonSelectAllClicked();
    void updateLabelLocked();
};

#endif // BITCOIN_QT_COINCONTROLDIALOG_H
