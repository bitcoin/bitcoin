// Copyright (c) 2015-2016 Silk Network Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MULTISIGDIALOG_H
#define MULTISIGDIALOG_H

#include <QDialog>

#include "multisigaddressentry.h"
#include "multisiginputentry.h"
#include "sendcoinsentry.h"
#include "walletmodel.h"

#include <vector>

namespace Ui
{
    class MultisigDialog;
}

class MultisigDialog : public QDialog
{
    Q_OBJECT;

  public:
    explicit MultisigDialog(QWidget *parent = 0);
    ~MultisigDialog();

    void setModel(WalletModel *model);
    bool AdvertisePublicKeyForMultiSig(const std::string& address, const std::string& publickey);

    typedef std::vector<CScript> redeemScripts;

  public Q_SLOTS:
    MultisigAddressEntry * addPubKey();
    void clear();
    void showTab(bool fShow);
    void updateRemoveEnabled();
    MultisigInputEntry * addInput();
    SendCoinsEntry * addOutput();

  private:
    Ui::MultisigDialog *ui;
    WalletModel *model;
    bool fSetTxString;

  private Q_SLOTS:
    void on_createAddressButton_clicked();
    void on_copyMultisigAddressButton_clicked();
    void on_copyRedeemScriptButton_clicked();
    void removeEntry(MultisigAddressEntry *entry);
    void on_createTransactionButton_clicked();
    void on_transaction_textChanged();
    void on_copyTransactionButton_clicked();
    void on_pasteTransactionButton_clicked();
    void on_signTransactionButton_clicked();
    void on_copySignedTransactionButton_clicked();
    void on_sendTransactionButton_clicked();
    void removeEntry(MultisigInputEntry *entry);
    void removeEntry(SendCoinsEntry *entry);
    void updateAmounts();
};

#endif // MULTISIGDIALOG_H
