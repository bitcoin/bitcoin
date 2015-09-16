#include "secondauthdialog.h"
#include "ui_secondauthdialog.h"

#include "addressbookpage.h"
#include "base58.h"
#include "guiutil.h"
#include "dialogwindowflags.h"
#include "init.h"
#include "main.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "wallet.h"

#include <string>
#include <vector>

#include <QClipboard>
#include <QKeyEvent>

SecondAuthDialog::SecondAuthDialog(QWidget *parent) :
    QWidget(parent, DIALOGWINDOWHINTS),
    ui(new Ui::SecondAuthDialog),
    model(0)
{
    ui->setupUi(this);

#if (QT_VERSION >= 0x040700)
    /* Do not move this to the XML file, Qt before 4.7 will choke on it */
    ui->addressIn->setPlaceholderText(tr("Enter a NovaCoin address (e.g. 4Zo1ga6xuKuQ7JV7M9rGDoxdbYwV5zgQJ5)"));
    ui->signatureOut->setPlaceholderText(tr("Click \"Sign data\" to generate signature"));
#endif

    GUIUtil::setupAddressWidget(ui->addressIn, this);

    ui->addressIn->installEventFilter(this);
    ui->messageIn->installEventFilter(this);
    ui->signatureOut->installEventFilter(this);

    ui->signatureOut->setFont(GUIUtil::bitcoinAddressFont());
}

SecondAuthDialog::~SecondAuthDialog()
{
    delete ui;
}

void SecondAuthDialog::setModel(WalletModel *model)
{
    this->model = model;
}

void SecondAuthDialog::on_addressBookButton_clicked()
{
    if (model && model->getAddressTableModel())
    {
        AddressBookPage dlg(AddressBookPage::ForSending, AddressBookPage::ReceivingTab, this);
        dlg.setModel(model->getAddressTableModel());
        if (dlg.exec())
        {
            ui->addressIn->setText(dlg.getReturnValue());
        }
    }
}

void SecondAuthDialog::on_pasteButton_clicked()
{
    ui->messageIn->setText(QApplication::clipboard()->text());
}

void SecondAuthDialog::on_signMessageButton_clicked()
{
    /* Clear old signature to ensure users don't get confused on error with an old signature displayed */
    ui->signatureOut->clear();

    CBitcoinAddress addr(ui->addressIn->text().toStdString());
    if (!addr.IsValid())
    {
        ui->addressIn->setValid(false);
        ui->statusLabel->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel->setText(tr("The entered address is invalid.") + QString(" ") + tr("Please check the address and try again."));
        return;
    }

    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
    {
        ui->addressIn->setValid(false);
        ui->statusLabel->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel->setText(tr("The entered address does not refer to a key.") + QString(" ") + tr("Please check the address and try again."));
        return;
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if (!ctx.isValid())
    {
        ui->statusLabel->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel->setText(tr("Wallet unlock was cancelled."));
        return;
    }

    CKey key;
    if (!pwalletMain->GetKey(keyID, key))
    {
        ui->statusLabel->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel->setText(tr("Private key for the entered address is not available."));
        return;
    }

    uint256 hash;
    hash.SetHex(ui->messageIn->text().toStdString());
    CTransaction tx;
    uint256 hashBlock = 0;
    if (!GetTransaction(hash, tx, hashBlock) || !hashBlock) {
        ui->statusLabel->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel->setText(tr("No information available about transaction."));
        return;
    }

    CDataStream ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << ui->messageIn->text().toStdString();

    std::vector<unsigned char> vchSig;
    if (!key.SignCompact(Hash(ss.begin(), ss.end()), vchSig))
    {
        ui->statusLabel->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel->setText(QString("<nobr>") + tr("Message signing failed.") + QString("</nobr>"));
        return;
    }

    ui->statusLabel->setStyleSheet("QLabel { color: green; }");
    ui->statusLabel->setText(QString("<nobr>") + tr("Message signed.") + QString("</nobr>"));

    ui->signatureOut->setText(QString::fromStdString(EncodeBase64(&vchSig[0], vchSig.size())));
}

void SecondAuthDialog::on_copySignatureButton_clicked()
{
    QApplication::clipboard()->setText(ui->signatureOut->text());
}

void SecondAuthDialog::on_clearButton_clicked()
{
    ui->addressIn->clear();
    ui->messageIn->clear();
    ui->signatureOut->clear();
    ui->statusLabel->clear();

    ui->addressIn->setFocus();
}

bool SecondAuthDialog::eventFilter(QObject *object, QEvent *event)
{
    return QWidget::eventFilter(object, event);
}

void SecondAuthDialog::keyPressEvent(QKeyEvent *event)
{
#ifdef ANDROID
    if(event->key() == Qt::Key_Back)
    {
        close();
    }
#else
    if(event->key() == Qt::Key_Escape)
    {
        close();
    }
#endif
}
