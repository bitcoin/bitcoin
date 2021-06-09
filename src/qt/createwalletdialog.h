// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_CREATEWALLETDIALOG_H
#define BITCOIN_QT_CREATEWALLETDIALOG_H

#include <QDialog>

class WalletModel;

#ifdef ENABLE_EXTERNAL_SIGNER
class ExternalSigner;
#endif

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

#ifdef ENABLE_EXTERNAL_SIGNER
    void setSigners(std::vector<ExternalSigner>& signers);
#endif

    QString walletName() const;
    bool isEncryptWalletChecked() const;
    bool isDisablePrivateKeysChecked() const;
    bool isMakeBlankWalletChecked() const;
    bool isDescriptorWalletChecked() const;
    bool isExternalSignerChecked() const;

private:
    Ui::CreateWalletDialog *ui;
};

#endif // BITCOIN_QT_CREATEWALLETDIALOG_H
