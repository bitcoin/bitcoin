#include "verifymessagedialog.h"
#include "ui_verifymessagedialog.h"

#include <string>
#include <vector>

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>

#include "main.h"
#include "wallet.h"
#include "walletmodel.h"
#include "guiutil.h"
#include "base58.h"

VerifyMessageDialog::VerifyMessageDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VerifyMessageDialog)
{
    ui->setupUi(this);

#if (QT_VERSION >= 0x040700)
    /* Do not move this to the XML file, Qt before 4.7 will choke on it */
    ui->lnAddress->setPlaceholderText(tr("Enter a Litecoin address (e.g. Ler4HNAEfwYhBmGXcFP2Po1NpRUEiK8km2)"));
    ui->lnSig->setPlaceholderText(tr("Enter Litecoin signature"));
#endif

    GUIUtil::setupAddressWidget(ui->lnAddress, this);
    ui->lnAddress->installEventFilter(this);

    ui->lnSig->setFont(GUIUtil::bitcoinAddressFont());

    ui->lnAddress->setFocus();
}

VerifyMessageDialog::~VerifyMessageDialog()
{
    delete ui;
}

void VerifyMessageDialog::on_verifyMessage_clicked()
{
    CBitcoinAddress addr(ui->lnAddress->text().toStdString());
    if (!addr.IsValid())
    {
        ui->lnAddress->setValid(false);
        ui->lblStatus->setStyleSheet("QLabel { color: red; }");
        ui->lblStatus->setText(tr("\"%1\" is not a valid address.").arg(ui->lnAddress->text()) + QString(" ") + tr("Please check the address and try again."));
        return;
    }
    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
    {
        ui->lnAddress->setValid(false);
        ui->lblStatus->setStyleSheet("QLabel { color: red; }");
        ui->lblStatus->setText(tr("\"%1\" does not refer to a key.").arg(ui->lnAddress->text()) + QString(" ") + tr("Please check the address and try again."));
        return;
    }

    bool fInvalid = false;
    std::vector<unsigned char> vchSig = DecodeBase64(ui->lnSig->text().toStdString().c_str(), &fInvalid);

    if (fInvalid)
    {
        ui->lnSig->setValid(false);
        ui->lblStatus->setStyleSheet("QLabel { color: red; }");
        ui->lblStatus->setText(tr("The signature could not be decoded.") + QString(" ") + tr("Please check the signature and try again."));
        return;
    }

    CDataStream ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << ui->edMessage->document()->toPlainText().toStdString();

    CKey key;
    if (!key.SetCompactSignature(Hash(ss.begin(), ss.end()), vchSig))
    {
        ui->lnSig->setValid(false);
        ui->lblStatus->setStyleSheet("QLabel { color: red; }");
        ui->lblStatus->setText(tr("The signature did not match the message digest.")+ QString(" ") + tr("Please check the signature and try again."));
        return;
    }

    if (!(CBitcoinAddress(key.GetPubKey().GetID()) == addr))
    {
        ui->lblStatus->setStyleSheet("QLabel { color: red; }");
        ui->lblStatus->setText(QString("<nobr>") + tr("Message verification failed.") + QString("</nobr>"));
        return;
    }

    ui->lblStatus->setStyleSheet("QLabel { color: green; }");
    ui->lblStatus->setText(QString("<nobr>") + tr("Message verified.") + QString("</nobr>"));
}

void VerifyMessageDialog::on_clearButton_clicked()
{
    ui->lnAddress->clear();
    ui->lnSig->clear();
    ui->edMessage->clear();
    ui->lblStatus->clear();

    ui->edMessage->setFocus();
}

bool VerifyMessageDialog::eventFilter(QObject *object, QEvent *event)
{
    if (object == ui->lnAddress && (event->type() == QEvent::MouseButtonPress ||
                                   event->type() == QEvent::FocusIn))
    {
        // set lnAddress to valid, as QEvent::FocusIn would not reach QValidatedLineEdit::focusInEvent
        ui->lnAddress->setValid(true);
        ui->lnAddress->selectAll();
        return true;
    }
    return QDialog::eventFilter(object, event);
}
