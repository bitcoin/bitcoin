// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_QT_CREATEWALLETDIALOG_H
#define SYSCOIN_QT_CREATEWALLETDIALOG_H

#include <QDialog>

class ExternalSigner;
class WalletModel;

namespace Ui {
    class CreateWalletDialog;
}

/** Dialog for creating wallets
 */
class CreateWalletDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateWalletDialog(QWidget* parent);
    virtual ~CreateWalletDialog();

    void setSigners(const std::vector<ExternalSigner>& signers);

    QString walletName() const;
    bool isEncryptWalletChecked() const;
    bool isDisablePrivateKeysChecked() const;
    bool isMakeBlankWalletChecked() const;
    bool isDescriptorWalletChecked() const;
    bool isExternalSignerChecked() const;

private:
    Ui::CreateWalletDialog *ui;
};

#endif // SYSCOIN_QT_CREATEWALLETDIALOG_H
