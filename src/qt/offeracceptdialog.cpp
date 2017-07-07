#include "offeracceptdialog.h"
#include "ui_offeracceptdialog.h"
#include "init.h"
#include "util.h"
#include "offerpaydialog.h"
#include "platformstyle.h"
#include "offeracceptdialogbtc.h"
#include "offeracceptdialogzec.h"
#include "offerescrowdialog.h"
#include "offer.h"
#include "alias.h"
#include "cert.h"
#include "guiutil.h"
#include "syscoingui.h"
#include <QMessageBox>
#include "rpc/server.h"
#include "pubkey.h"
#include "wallet/wallet.h"
#include "main.h"
using namespace std;

extern CRPCTable tableRPC;
OfferAcceptDialog::OfferAcceptDialog(WalletModel* model, const PlatformStyle *platformStyle, QString aliaspeg, QString alias, QString encryptionkey, QString offer, QString quantity, QString notes, QString title, QString currencyCode, QString qstrPrice, QString sellerAlias, QString address, unsigned char paymentOptions, float nQtyUnits,bool bCoinOffer,QWidget *parent) :
    QDialog(parent),
	walletModel(model),
    ui(new Ui::OfferAcceptDialog), platformStyle(platformStyle), aliaspeg(aliaspeg), qstrPrice(qstrPrice), alias(alias), m_encryptionkey(encryptionkey), offer(offer), notes(notes), quantity(quantity), title(title), currency(currencyCode), seller(sellerAlias), address(address),  nQtyUnits(nQtyUnits)
{
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();
	if (!platformStyle->getImagesOnButtons())
	{
		ui->acceptButton->setIcon(QIcon());
		ui->acceptBtcButton->setIcon(QIcon());
		ui->acceptZecButton->setIcon(QIcon());
		ui->cancelButton->setIcon(QIcon());

	}
	else
	{
		ui->acceptButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/send"));
		ui->acceptBtcButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/send"));
		ui->acceptZecButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/send"));
		ui->cancelButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/quit"));
	}
	ui->aboutShade->setPixmap(QPixmap(":/images/" + theme + "/about_horizontal"));
	int sysprecision = 0;
	double dblPrice = qstrPrice.toDouble();
	string strCurrencyCode = currencyCode.toStdString();
	string strAliasPeg = aliaspeg.toStdString();
	ui->acceptBtcButton->setEnabled(false);
	ui->acceptBtcButton->setVisible(false);
	ui->acceptZecButton->setEnabled(false);
	ui->acceptZecButton->setVisible(false);
	CAmount sysPrice = convertCurrencyCodeToSyscoin(vchFromString(strAliasPeg), vchFromString(strCurrencyCode), dblPrice, chainActive.Tip()->nHeight, sysprecision);
	if(bCoinOffer)
	{
		sysPrice = nQtyUnits*COIN;
		if(sysPrice == 0)
		{
			QMessageBox::critical(this, windowTitle(),
				tr("Unit is not defined or is invalid in this offer")
					,QMessageBox::Ok, QMessageBox::Ok);
			reject();
			return;
		}
	}
	else
	{
		
		if(sysPrice == 0)
		{
			QMessageBox::critical(this, windowTitle(),
				tr("Could not find currency in the rates peg for this offer. Currency: ") + QString::fromStdString(strCurrencyCode)
					,QMessageBox::Ok, QMessageBox::Ok);
			reject();
			return;
		}
	}

	strSYSPrice = QString::fromStdString(strprintf("%.*f", 8, ValueFromAmount(sysPrice).get_real()));
	QString strTotalSYSPrice = QString::fromStdString(strprintf("%.*f", sysprecision, ValueFromAmount(sysPrice).get_real()*quantity.toUInt()));
	ui->escrowDisclaimer->setText(QString("<font color='blue'>") + tr("Enter a Syscoin arbiter that is mutally trusted between yourself and the merchant") + QString("</font>"));
	QString priceStr;
	if(bCoinOffer)
		priceStr = QString(" <b>%1 SYS</b>").arg(strTotalSYSPrice);
	else
		priceStr = QString(" <b>%1 %2 (%3 SYS)</b>").arg(qstrPrice).arg(currencyCode).arg(strTotalSYSPrice);
	
	ui->acceptMessage->setText(tr("Are you sure you want to purchase") + QString(" <b>%1</b> ").arg(quantity) + tr("of") +  QString(" <b>%1</b> ").arg(title) + tr("from merchant") + QString(" <b>%1</b>").arg(sellerAlias) + QString("? ") + tr("You will be charged") + priceStr);
	if(IsPaymentOptionInMask(paymentOptions, PAYMENTOPTION_ZEC))
	{
		ui->acceptZecButton->setEnabled(true);
		ui->acceptZecButton->setVisible(true);
	}
	if(IsPaymentOptionInMask(paymentOptions, PAYMENTOPTION_BTC))
	{
		ui->acceptBtcButton->setEnabled(true);
		ui->acceptBtcButton->setVisible(true);
	}
	connect(ui->checkBox,SIGNAL(clicked(bool)),SLOT(onEscrowCheckBoxChanged(bool)));
	this->offerPaid = false;
	connect(ui->acceptButton, SIGNAL(clicked()), this, SLOT(acceptPayment()));
	connect(ui->acceptBtcButton, SIGNAL(clicked()), this, SLOT(acceptBTCPayment()));
	connect(ui->acceptZecButton, SIGNAL(clicked()), this, SLOT(acceptZECPayment()));
	setupEscrowCheckboxState();

}
void OfferAcceptDialog::on_cancelButton_clicked()
{
    reject();
}
OfferAcceptDialog::~OfferAcceptDialog()
{
    delete ui;
}
void OfferAcceptDialog::setupEscrowCheckboxState()
{
	if(ui->checkBox->isChecked())
	{
		ui->escrowDisclaimer->setVisible(true);
		ui->escrowEdit->setEnabled(true);
		ui->acceptButton->setText(tr("Pay Escrow"));
		if(ui->checkBox->isChecked())
			ui->acceptButton->setEnabled(true);
		else
			ui->acceptButton->setEnabled(false);
	}
	else
	{
		ui->escrowDisclaimer->setVisible(false);
		ui->escrowEdit->setEnabled(false);
		ui->acceptButton->setText(tr("Pay For Item"));
	}
}
void OfferAcceptDialog::onEscrowCheckBoxChanged(bool toggled)
{
	setupEscrowCheckboxState();
	ui->cancelButton->setDefault(false);
	ui->acceptButton->setDefault(true);
}
void OfferAcceptDialog::acceptZECPayment()
{
	if(!walletModel)
		return;
	OfferAcceptDialogZEC dlg(walletModel, platformStyle, this->aliaspeg, this->alias, this->m_encryptionkey, this->offer, this->quantity, this->notes, this->title, this->currency, this->strSYSPrice, this->seller, this->address, ui->checkBox->isChecked()? ui->escrowEdit->text(): "", nQtyUnits,bCoinOffer,this);
	if(dlg.exec())
	{
		this->offerPaid = dlg.getPaymentStatus();
		if(this->offerPaid)
		{
			OfferAcceptDialog::accept();
		}
	}
}
void OfferAcceptDialog::acceptBTCPayment()
{
	if(!walletModel)
		return;
	OfferAcceptDialogBTC dlg(walletModel, platformStyle, this->aliaspeg, this->alias, this->m_encryptionkey, this->offer, this->quantity, this->notes, this->title, this->currency, this->strSYSPrice, this->seller, this->address, ui->checkBox->isChecked()? ui->escrowEdit->text(): "",nQtyUnits,bCoinOffer,this);
	if(dlg.exec())
	{
		this->offerPaid = dlg.getPaymentStatus();
		if(this->offerPaid)
		{
			OfferAcceptDialog::accept();
		}
	}
}
void OfferAcceptDialog::acceptPayment()
{
	if(ui->checkBox->isChecked())
		acceptEscrow();
	else
		acceptOffer();
}
// send offeraccept with offer guid/qty as params and then send offerpay with wtxid (first param of response) as param, using RPC commands.
void OfferAcceptDialog::acceptOffer()
{

}
void OfferAcceptDialog::acceptEscrow()
{
	


}
bool OfferAcceptDialog::getPaymentStatus()
{
	return this->offerPaid;
}
