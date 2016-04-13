#include "offeracceptdialog.h"
#include "ui_offeracceptdialog.h"
#include "init.h"
#include "util.h"
#include "offerpaydialog.h"
#include "platformstyle.h"
#include "offeracceptdialogbtc.h"
#include "offerescrowdialog.h"
#include "offer.h"
#include "alias.h"
#include "guiutil.h"
#include "syscoingui.h"
#include <QMessageBox>
#include "rpcserver.h"
#include "pubkey.h"
#include "wallet/wallet.h"
#include "main.h"
using namespace std;

extern const CRPCTable tableRPC;
OfferAcceptDialog::OfferAcceptDialog(const PlatformStyle *platformStyle, QString aliaspeg, QString alias, QString offer, QString quantity, QString notes, QString title, QString currencyCode, QString qstrPrice, QString sellerAlias, QString address, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OfferAcceptDialog), platformStyle(platformStyle), aliaspeg(aliaspeg), alias(alias), offer(offer), notes(notes), quantity(quantity), title(title), currency(currencyCode), seller(sellerAlias), address(address)
{
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();  
	if (!platformStyle->getImagesOnButtons())
	{
		ui->acceptButton->setIcon(QIcon());
		ui->acceptBtcButton->setIcon(QIcon());
		ui->cancelButton->setIcon(QIcon());

	}
	else
	{
		ui->acceptButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/send"));
		ui->acceptBtcButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/send"));
		ui->cancelButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/quit"));
	}
	ui->aboutShade->setPixmap(QPixmap(":/images/" + theme + "/about_horizontal"));
	int precision, sysprecision;
	double dblPrice = qstrPrice.toDouble();
	string strCurrencyCode = currencyCode.toStdString();
	string strAliasPeg = aliaspeg.toStdString();
	ui->acceptBtcButton->setEnabled(false);
	ui->acceptBtcButton->setVisible(false);
	convertCurrencyCodeToSyscoin(vchFromString(strAliasPeg), vchFromString("SYS"), 0, chainActive.Tip()->nHeight, sysprecision);
	CAmount iPrice = convertCurrencyCodeToSyscoin(vchFromString(strAliasPeg), vchFromString(strCurrencyCode), dblPrice, chainActive.Tip()->nHeight, precision);
	string strPrice = strprintf("%.*f", sysprecision, ValueFromAmount(iPrice).get_real()*quantity.toUInt() );
	price = QString::fromStdString(strPrice);
	if(strCurrencyCode == "BTC")
	{
		string strfPrice = strprintf("%f", dblPrice*quantity.toUInt());
		fprice = QString::fromStdString(strfPrice);
		ui->acceptBtcButton->setEnabled(true);
		ui->acceptBtcButton->setVisible(true);
		ui->escrowDisclaimer->setText(tr("<font color='blue'>Select an arbiter that is mutally trusted between yourself and the merchant. Note that escrow is not available if you pay with BTC</font>"));
		ui->acceptMessage->setText(tr("Are you sure you want to purchase %1 of '%2' from merchant: '%3'? You will be charged %4 SYS (%5 BTC)").arg(quantity).arg(title).arg(sellerAlias).arg(price).arg(fprice));
	}
	else
	{
		ui->escrowDisclaimer->setText(tr("<font color='blue'>Select an arbiter that is mutally trusted between yourself and the merchant.</font>"));
		ui->acceptMessage->setText(tr("Are you sure you want to purchase %1 of '%2' from merchant: '%3'? You will be charged %4 SYS").arg(quantity).arg(title).arg(sellerAlias).arg(price));
	}
		
	connect(ui->checkBox,SIGNAL(clicked(bool)),SLOT(onEscrowCheckBoxChanged(bool)));
	this->offerPaid = false;
	connect(ui->acceptButton, SIGNAL(clicked()), this, SLOT(acceptPayment()));
	connect(ui->acceptBtcButton, SIGNAL(clicked()), this, SLOT(acceptBTCPayment()));
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
	ui->acceptBtcButton->setDefault(true);
}
void OfferAcceptDialog::acceptBTCPayment()
{
	OfferAcceptDialogBTC dlg(platformStyle, this->alias, this->offer, this->quantity, this->notes, this->title, this->currency, this->fprice, this->seller, this->address, this);
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

		UniValue params(UniValue::VARR);
		UniValue valError;
		UniValue valResult;
		UniValue valId;
		UniValue result;
		string strReply;
		string strError;

		string strMethod = string("offeraccept");
		if(this->quantity.toLong() <= 0)
		{
			QMessageBox::critical(this, windowTitle(),
				tr("Invalid quantity when trying to accept offer!"),
				QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
		this->offerPaid = false;
		params.push_back("");
		params.push_back(this->alias.toStdString());
		params.push_back(this->offer.toStdString());
		params.push_back(this->quantity.toStdString());
		params.push_back(this->notes.toStdString());
		

	    try {
            result = tableRPC.execute(strMethod, params);
			if (result.type() != UniValue::VNULL)
			{
				const UniValue &arr = result.get_array();
				string strResult = arr[0].get_str();
				QString offerAcceptTXID = QString::fromStdString(strResult);
				if(offerAcceptTXID != QString(""))
				{
					OfferPayDialog dlg(platformStyle, this->title, this->quantity, this->price, "SYS", this);
					dlg.exec();
					this->offerPaid = true;
					OfferAcceptDialog::accept();
					return;

				}
			}
		}
		catch (UniValue& objError)
		{
			strError = find_value(objError, "message").get_str();
			QMessageBox::critical(this, windowTitle(),
			tr("Error accepting offer: \"%1\"").arg(QString::fromStdString(strError)),
				QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
		catch(std::exception& e)
		{
			QMessageBox::critical(this, windowTitle(),
				tr("General exception when accepting offer"),
				QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
	
   

}
void OfferAcceptDialog::acceptEscrow()
{

		UniValue params(UniValue::VARR);
		UniValue valError;
		UniValue valResult;
		UniValue valId;
		UniValue result ;
		string strReply;
		string strError;

		string strMethod = string("escrownew");
		if(this->quantity.toLong() <= 0)
		{
			QMessageBox::critical(this, windowTitle(),
				tr("Invalid quantity when trying to create escrow!"),
				QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
		this->offerPaid = false;
		params.push_back(this->alias.toStdString());
		params.push_back(this->offer.toStdString());
		params.push_back(this->quantity.toStdString());
		params.push_back(this->notes.toStdString());
		params.push_back(ui->escrowEdit->text().toStdString());

	    try {
            result = tableRPC.execute(strMethod, params);
			if (result.type() != UniValue::VNULL)
			{
				const UniValue &arr = result.get_array();
				string strResult = arr[0].get_str();
				QString escrowTXID = QString::fromStdString(strResult);
				if(escrowTXID != QString(""))
				{
					OfferEscrowDialog dlg(platformStyle, this->title, this->quantity, this->price, this);
					dlg.exec();
					this->offerPaid = true;
					OfferAcceptDialog::accept();
					return;

				}
			}
		}
		catch (UniValue& objError)
		{
			strError = find_value(objError, "message").get_str();
			QMessageBox::critical(this, windowTitle(),
			tr("Error creating escrow: \"%1\"").arg(QString::fromStdString(strError)),
				QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
		catch(std::exception& e)
		{
			QMessageBox::critical(this, windowTitle(),
				tr("General exception when creating escrow"),
				QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
	
   

}
bool OfferAcceptDialog::getPaymentStatus()
{
	return this->offerPaid;
}
