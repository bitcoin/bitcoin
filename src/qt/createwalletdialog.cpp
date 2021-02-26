// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <external_signer.h>
#include <qt/createwalletdialog.h>
#include <qt/forms/ui_createwalletdialog.h>

#include <qt/guiutil.h>

#include <QMessageBox>
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
        // Disable the disable_privkeys_checkbox and external_signer_checkbox when isEncryptWalletChecked is
        // set to true, enable it when isEncryptWalletChecked is false.
        ui->disable_privkeys_checkbox->setEnabled(!checked);
#ifdef ENABLE_EXTERNAL_SIGNER
        ui->external_signer_checkbox->setEnabled(!checked);
#endif
        // When the disable_privkeys_checkbox is disabled, uncheck it.
        if (!ui->disable_privkeys_checkbox->isEnabled()) {
            ui->disable_privkeys_checkbox->setChecked(false);
        }

        // When the external_signer_checkbox box is disabled, uncheck it.
        if (!ui->external_signer_checkbox->isEnabled()) {
            ui->external_signer_checkbox->setChecked(false);
        }

    });

    connect(ui->external_signer_checkbox, &QCheckBox::toggled, [this](bool checked) {
        ui->encrypt_wallet_checkbox->setEnabled(!checked);
        ui->blank_wallet_checkbox->setEnabled(!checked);
        ui->disable_privkeys_checkbox->setEnabled(!checked);
        ui->descriptor_checkbox->setEnabled(!checked);

        // The external signer checkbox is only enabled when a device is detected.
        // In that case it is checked by default. Toggling it restores the other
        // options to their default.
        ui->descriptor_checkbox->setChecked(checked);
        ui->encrypt_wallet_checkbox->setChecked(false);
        ui->disable_privkeys_checkbox->setChecked(checked);
        // The blank check box is ambiguous. This flag is always true for a
        // watch-only wallet, even though we immedidately fetch keys from the
        // external signer.
        ui->blank_wallet_checkbox->setChecked(checked);
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
        ui->external_signer_checkbox->setEnabled(false);
        ui->external_signer_checkbox->setChecked(false);
#endif

#ifdef USE_BDB
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
#else
    connect(ui->buttonBox, &QDialogButtonBox::accepted, [this]() {
        if (!this->isDescriptorWalletChecked()) {
            QMessageBox::critical(this, tr("Cannot create wallet"), tr("This build was compiled without BDB support, so only experimental descriptor wallets are supported."));
            return;
        }
        this->accept();
    });
#endif

#ifndef ENABLE_EXTERNAL_SIGNER
        //: "External signing" means using devices such as hardware wallets.
        ui->external_signer_checkbox->setToolTip(tr("Compiled without external signing support (required for external signing)"));
        ui->external_signer_checkbox->setEnabled(false);
        ui->external_signer_checkbox->setChecked(false);
#endif

}

CreateWalletDialog::~CreateWalletDialog()
{
    delete ui;
}

void CreateWalletDialog::setSigners(const std::vector<ExternalSigner>& signers)
{
    if (!signers.empty()) {
        ui->external_signer_checkbox->setEnabled(true);
        ui->external_signer_checkbox->setChecked(true);
        ui->encrypt_wallet_checkbox->setEnabled(false);
        ui->encrypt_wallet_checkbox->setChecked(false);
        // The order matters, because connect() is called when toggling a checkbox:
        ui->blank_wallet_checkbox->setEnabled(false);
        ui->blank_wallet_checkbox->setChecked(false);
        ui->disable_privkeys_checkbox->setEnabled(false);
        ui->disable_privkeys_checkbox->setChecked(true);
        const std::string label = signers[0].m_name;
        ui->wallet_name_line_edit->setText(QString::fromStdString(label));
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->external_signer_checkbox->setEnabled(false);
    }
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

bool CreateWalletDialog::isExternalSignerChecked() const
{
    return ui->external_signer_checkbox->isChecked();
}
