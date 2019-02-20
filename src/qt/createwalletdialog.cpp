// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/askpassphrasedialog.h>
#include <qt/createwalletdialog.h>
#include <qt/forms/ui_createwalletdialog.h>
#include <qt/guiconstants.h>
#include <support/allocators/secure.h>
#include <wallet/wallet.h>

#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QString>

CreateWalletDialog::CreateWalletDialog(QWidget* parent, WalletController* wallet_controller) :
    QDialog(parent),
    ui(new Ui::CreateWalletDialog)
{
    ui->setupUi(this);
    this->m_wallet_controller = wallet_controller;
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Create");
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->wallet_name_line_edit->setFocus(Qt::ActiveWindowFocusReason);

    connect(ui->wallet_name_line_edit, &QLineEdit::textEdited, this, &CreateWalletDialog::WalletNameChanged);
}

CreateWalletDialog::~CreateWalletDialog()
{
    delete ui;
}

void CreateWalletDialog::WalletNameChanged(const QString& text)
{
    if (text.isEmpty()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
}

void CreateWalletDialog::accept()
{
    // Disable the buttons
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);

    // Get the options
    std::string wallet_name = ui->wallet_name_line_edit->text().toStdString();
    bool disable_priv_keys = ui->disable_privkeys_checkbox->isChecked();
    bool blank = ui->blank_wallet_checkbox->isChecked();
    bool encrypt = ui->encrypt_wallet_checkbox->isChecked();

    // Get wallet creation flags
    uint64_t flags = 0;
    if (disable_priv_keys) {
        flags |= WALLET_FLAG_DISABLE_PRIVATE_KEYS;
    }
    if (blank) {
        flags |= WALLET_FLAG_BLANK_WALLET;
    }

    // Show a progress dialog
    QProgressDialog* dialog = new QProgressDialog(this);
    dialog->setLabelText(tr("Creating Wallet <b>%1</b>...").arg(QString(wallet_name.c_str()).toHtmlEscaped()));
    dialog->setRange(0, 0);
    dialog->setCancelButton(nullptr);
    dialog->setWindowModality(Qt::ApplicationModal);
    dialog->show();

    // Get the passphrase if encryption is used
    SecureString passphrase;
    passphrase.reserve(MAX_PASSPHRASE_SIZE);
    if (encrypt) {
        AskPassphraseDialog dlg(AskPassphraseDialog::Encrypt, this, &passphrase);
        dlg.exec();
        if (passphrase.empty()) {
            dialog->hide();
            QDialog::reject();
            return;
        }
    }

    // Create the wallet
    std::string error;
    std::string warning;
    std::unique_ptr<interfaces::Wallet> wallet = m_wallet_controller->createWallet(wallet_name, error, warning, passphrase, flags);

    if (!error.empty()) {
        QMessageBox::critical(this, tr("Wallet creation failed"), QString::fromStdString(error));
        return;
    }
    if (!warning.empty()) {
        QMessageBox::warning(this, tr("Wallet creation warning"), QString::fromStdString(warning));
    }
    dialog->hide();
    QDialog::accept();
}
