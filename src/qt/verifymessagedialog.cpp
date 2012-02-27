#include "verifymessagedialog.h"
#include "ui_verifymessagedialog.h"

#include <string>
#include <vector>

#include <QDialogButtonBox>
#include <QAbstractButton>
#include <QClipboard>
#include <QMessageBox>

#include "main.h"
#include "wallet.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "guiutil.h"

VerifyMessageDialog::VerifyMessageDialog(AddressTableModel *addressModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VerifyMessageDialog),
    model(addressModel)
{
    ui->setupUi(this);

    GUIUtil::setupAddressWidget(ui->lnAddress, this);
}

VerifyMessageDialog::~VerifyMessageDialog()
{
    delete ui;
}

bool VerifyMessageDialog::checkAddress()
{
    CDataStream ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << ui->edMessage->document()->toPlainText().toStdString();
    uint256 hash = Hash(ss.begin(), ss.end());

    bool invalid = true;
    std::vector<unsigned char> vchSig = DecodeBase64(ui->lnSig->text().toStdString().c_str(), &invalid);

    if(invalid)
    {
        QMessageBox::warning(this, tr("Invalid Signature"), tr("The signature could not be decoded. Please check the signature and try again."));
        return false;
    }

    CKey key;
    if(!key.SetCompactSignature(hash, vchSig))
    {
        QMessageBox::warning(this, tr("Invalid Signature"), tr("The signature did not match the message digest. Please check the signature and try again."));
        return false;
    }

    CBitcoinAddress address(key.GetPubKey());
    QString qStringAddress = QString::fromStdString(address.ToString());
    ui->lnAddress->setText(qStringAddress);
    ui->copyToClipboard->setEnabled(true);

    QString label = model->labelForAddress(qStringAddress);
    ui->lblStatus->setText(label.isEmpty() ? tr("Address not found in address book.") : tr("Address found in address book: %1").arg(label));
    return true;
}

void VerifyMessageDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if(ui->buttonBox->buttonRole(button) == QDialogButtonBox::ApplyRole)
        checkAddress();
}

void VerifyMessageDialog::on_copyToClipboard_clicked()
{
    QApplication::clipboard()->setText(ui->lnAddress->text());
}
