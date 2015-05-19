// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SENDMPDIALOG_H
#define SENDMPDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QString>

class ClientModel;

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Ui {
    class SendMPDialog;
}

/** Dialog for sending Master Protocol tokens */
class SendMPDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendMPDialog(QWidget *parent = 0);
    ~SendMPDialog();

    void setClientModel(ClientModel *model);
    void setWalletModel(WalletModel *model);

    void clearFields();
    void sendMPTransaction();
    void updateFrom();
    void updateProperty();
    void updatePropSelector();

public slots:
//  void setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance);
    void propertyComboBoxChanged(int idx);
    void sendFromComboBoxChanged(int idx);
    void clearButtonClicked();
    void sendButtonClicked();
    void balancesUpdated();

private:
    Ui::SendMPDialog *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;

signals:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // SENDMPDIALOG_H
