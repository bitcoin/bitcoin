// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SENDCOINSDIALOG_H
#define BITCOIN_SENDCOINSDIALOG_H

#include "bitcoin_walletmodel.h"

#include <QDialog>
#include <QString>

class OptionsModel;
class Bitcoin_SendCoinsEntry;
class Bitcoin_SendCoinsRecipient;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Ui {
    class Bitcoin_SendCoinsDialog;
}

/** Dialog for sending bitcoins */
class Bitcoin_SendCoinsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit Bitcoin_SendCoinsDialog(QWidget *parent = 0);
    ~Bitcoin_SendCoinsDialog();

    void setModel(Bitcoin_WalletModel *model);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void setAddress(const QString &address);
    void pasteEntry(const Bitcoin_SendCoinsRecipient &rv);
    bool handlePaymentRequest(const Bitcoin_SendCoinsRecipient &recipient);

public slots:
    void clear();
    void reject();
    void accept();
    Bitcoin_SendCoinsEntry *addEntry();
    void updateTabsAndLabels();
    void setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance);

private:
    Ui::Bitcoin_SendCoinsDialog *ui;
    Bitcoin_WalletModel *model;
    bool fNewRecipientAllowed;

    // Process Bitcoin_WalletModel::SendCoinsReturn and generate a pair consisting
    // of a message and message flags for use in emit message().
    // Additional parameter msgArg can be used via .arg(msgArg).
    void processSendCoinsReturn(const Bitcoin_WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());

private slots:
    void on_sendButton_clicked();
    void removeEntry(Bitcoin_SendCoinsEntry* entry);
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

#endif // BITCOIN_SENDCOINSDIALOG_H
