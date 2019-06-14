// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METADEXCANCELDIALOG_H
#define METADEXCANCELDIALOG_H

#include <QDialog>

class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QWidget;
class QString;
QT_END_NAMESPACE

namespace Ui {
    class MetaDExCancelDialog;
}

/** Dialog for sending Master Protocol tokens */
class MetaDExCancelDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MetaDExCancelDialog(QWidget *parent = 0);
    ~MetaDExCancelDialog();

    void setClientModel(ClientModel *model);
    void setWalletModel(WalletModel *model);

public Q_SLOTS:
    void SendCancelTransaction();
    void UpdateAddressSelector();
    void UpdateCancelCombo();
    void RefreshUI();
    void ReinitUI();
    void fromAddressComboBoxChanged(int);

private:
    Ui::MetaDExCancelDialog *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;

private Q_SLOTS:
    // None!

Q_SIGNALS:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // METADEXCANCELDIALOG_H
