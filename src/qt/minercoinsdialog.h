// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MINERCOINSDIALOG_H
#define MINERCOINSDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QString>

class OptionsModel;
class UneditableCoinsEntry;
class Bitcredit_SendCoinsRecipient;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Ui {
    class MinerCoinsDialog;
}

/** Dialog for sending bitcoins */
class MinerCoinsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MinerCoinsDialog(QWidget *parent = 0);
    ~MinerCoinsDialog();

    void setModel(Bitcredit_WalletModel *bitcredit_model, Bitcredit_WalletModel *deposit_model);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

public slots:
    void clear();
    void reject();
    void accept();
    UneditableCoinsEntry *addEntry(qint64 value=-1);
    void updateTabsAndLabels();
    void setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance, qint64 preparedDepositBalance, qint64 inDepositBalance);
    void refreshBalance();
    void setMinerStatistics(qint64 reqDeposit1, qint64 reqDeposit2, qint64 reqDeposit3, qint64 reqDeposit4, qint64 reqDeposit5, qint64 maxSubsidy1, qint64 maxSubsidy2, qint64 maxSubsidy3, qint64 maxSubsidy4, qint64 maxSubsidy5, unsigned int blockHeight, qint64 totalMonetaryBase, qint64 totalDepositBase, int nextSubsidyUpdateHeight);
    void refreshStatistics();

private:
    Ui::MinerCoinsDialog *ui;
    Bitcredit_WalletModel *bitcredit_model;
    Bitcredit_WalletModel *deposit_model;

    // Process WalletModel::SendCoinsReturn and generate a pair consisting
    // of a message and message flags for use in emit message().
    // Additional parameter msgArg can be used via .arg(msgArg).
    void processSendCoinsReturn(const Bitcredit_WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());

private slots:
    void on_sendButton_clicked();
    void removeEntry(UneditableCoinsEntry* entry);
    void updateDisplayUnit();
    void coinControlButtonClicked();
    void coinControlUpdateLabels();
    void coinControlClipboardQuantity();
    void coinControlClipboardAmount();

signals:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // MINERCOINSDIALOG_H
