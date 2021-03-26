// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/createwalletdialog.h>
#include <qt/forms/ui_createwalletdialog.h>

#include <qt/guiutil.h>

#include <QPushButton>

CreateWalletDialog::CreateWalletDialog(QWidget* parent) :
    QDialog(parent, GUIUtil::dialog_flags),
    ui(new Ui::CreateWalletDialog)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Create"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->wallet_name_line_edit->setFocus(Qt::ActiveWindowFocusReason);

    connect(ui->wallet_name_line_edit, &QLineEdit::textEdited, [this](const QString& text) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!text.isEmpty());
    });

    connect(ui->encrypt_wallet_checkbox, &QCheckBox::toggled, [this](bool checked) {
        // Disable the disable_privkeys_checkbox when isEncryptWalletChecked is
        // set to true, enable it when isEncryptWalletChecked is false.
        ui->disable_privkeys_checkbox->setEnabled(!checked);

        // When the disable_privkeys_checkbox is disabled, uncheck it.
        if (!ui->disable_privkeys_checkbox->isEnabled()) {
            ui->disable_privkeys_checkbox->setChecked(false);
        }
    });

    connect(ui->disable_privkeys_checkbox, &QCheckBox::toggled, [this](bool checked) {
        // Disable the encrypt_wallet_checkbox when isDisablePrivateKeysChecked is
        // set to true, enable it when isDisablePrivateKeysChecked is false.
        ui->encrypt_wallet_checkbox->setEnabled(!checked);

        // Wallets without private keys start out blank
        if (checked) {
            ui->blank_wallet_checkbox->setChecked(true);
        }

        // When the encrypt_wallet_checkbox is disabled, uncheck it.
        if (!ui->encrypt_wallet_checkbox->isEnabled()) {
            ui->encrypt_wallet_checkbox->setChecked(false);
        }
    });

    connect(ui->blank_wallet_checkbox, &QCheckBox::toggled, [this](bool checked) {
        if (!checked) {
          ui->disable_privkeys_checkbox->setChecked(false);
        }
    });

#ifndef USE_SQLITE
        ui->descriptor_checkbox->setToolTip(tr("Compiled without sqlite support (required for descriptor wallets)"));
        ui->descriptor_checkbox->setEnabled(false);
        ui->descriptor_checkbox->setChecked(false);
#endif
#ifndef USE_BDB
        ui->descriptor_checkbox->setEnabled(false);
        ui->descriptor_checkbox->setChecked(true);
#endif
}

CreateWalletDialog::~CreateWalletDialog()
{
    delete ui;
}

QString CreateWalletDialog::walletName() const
{
    return ui->wallet_name_line_edit->text();
}

bool CreateWalletDialog::isEncryptWalletChecked() const
{
    return ui->encrypt_wallet_checkbox->isChecked();
}

bool CreateWalletDialog::isDisablePrivateKeysChecked() const
{
    return ui->disable_privkeys_checkbox->isChecked();
}

bool CreateWalletDialog::isMakeBlankWalletChecked() const
{
    return ui->blank_wallet_checkbox->isChecked();
}

bool CreateWalletDialog::isDescriptorWalletChecked() const
{
    return ui->descriptor_checkbox->isChecked();
}
