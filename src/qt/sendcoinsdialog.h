// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SENDCOINSDIALOG_H
#define SENDCOINSDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QString>

class OptionsModel;
class Credits_SendCoinsEntry;
class Bitcredit_SendCoinsRecipient;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Ui {
    class Credits_SendCoinsDialog;
}

/** Dialog for sending bitcoins */
class Credits_SendCoinsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit Credits_SendCoinsDialog(QWidget *parent = 0);
    ~Credits_SendCoinsDialog();

    void setModel(Bitcredit_WalletModel *bitcredit_model, Bitcredit_WalletModel *deposit_model);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void setAddress(const QString &address);
    void pasteEntry(const Bitcredit_SendCoinsRecipient &rv);
    bool handlePaymentRequest(const Bitcredit_SendCoinsRecipient &recipient);

public slots:
    void clear();
    void reject();
    void accept();
    Credits_SendCoinsEntry *addEntry();
    void updateTabsAndLabels();
    void setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance, qint64 preparedDepositBalance, qint64 inDepositBalance);

private:
    Ui::Credits_SendCoinsDialog *ui;
    Bitcredit_WalletModel *bitcredit_model;
    Bitcredit_WalletModel *deposit_model;
    bool fNewRecipientAllowed;

    // Process WalletModel::SendCoinsReturn and generate a pair consisting
    // of a message and message flags for use in emit message().
    // Additional parameter msgArg can be used via .arg(msgArg).
    void processSendCoinsReturn(const Bitcredit_WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());

private slots:
    void on_sendButton_clicked();
    void removeEntry(Credits_SendCoinsEntry* entry);
    void updateDisplayUnit();
    void coinControlFeatureChanged(bool);
    void coinControlButtonClicked();
    void coinControlChangeChecked(int);
    void coinControlChangeEdited(const QString &);
    void coinControlUpdateLabels();
    void coinControlClipboardQuantity();
    void coinControlClipboardAmount();
    void coinControlClipboardFee();
    void coinControlClipboardAfterFee();
    void coinControlClipboardBytes();
    void coinControlClipboardPriority();
    void coinControlClipboardLowOutput();
    void coinControlClipboardChange();

signals:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // SENDCOINSDIALOG_H
