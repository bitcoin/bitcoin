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
#include "guiutil.h"
#include "syscoingui.h"
#include <QMessageBox>
#include "rpc/server.h"
#include "pubkey.h"
#include "wallet/wallet.h"
#include "main.h"
using namespace std;

extern CRPCTable tableRPC;
OfferAcceptDialog::OfferAcceptDialog(WalletModel* model, const PlatformStyle *platformStyle, QString aliaspeg, QString alias, QString offer, QString quantity, QString notes, QString title, QString currencyCode, QString qstrPrice, QString sellerAlias, QString address, unsigned char paymentOptions, QWidget *parent) :
    QDialog(parent),
	walletModel(model),
    ui(new Ui::OfferAcceptDialog), platformStyle(platformStyle), aliaspeg(aliaspeg), qstrPrice(qstrPrice), alias(alias), offer(offer), notes(notes), quantity(quantity), title(title), currency(currencyCode), seller(sellerAlias), address(address)
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
	if(sysPrice == 0)
	{
        QMessageBox::critical(this, windowTitle(),
			tr("Could not find currency in the rates peg for this offer. Currency: ") + QString::fromStdString(strCurrencyCode)
                ,QMessageBox::Ok, QMessageBox::Ok);
		reject();
		return;
	}
	strSYSPrice = QString::fromStdString(strprintf("%.*f", 8, ValueFromAmount(sysPrice).get_real()));
	QString strTotalSYSPrice = QString::fromStdString(strprintf("%.*f", sysprecision, ValueFromAmount(sysPrice).get_real()*quantity.toUInt()));
	ui->escrowDisclaimer->setText(QString("<font color='blue'>") + tr("Enter a Syscoin arbiter that is mutally trusted between yourself and the merchant") + QString("</font>"));
		
	ui->acceptMessage->setText(tr("Are you sure you want to purchase") + QString(" <b>%1</b> ").arg(quantity) + tr("of") +  QString(" <b>%1</b> ").arg(title) + tr("from merchant") + QString(" <b>%1</b>").arg(sellerAlias) + QString("? ") + tr("You will be charged") + QString(" <b>%1 %2 (%3 SYS)</b>").arg(qstrPrice).arg(currencyCode).arg(strTotalSYSPrice));
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
	OfferAcceptDialogZEC dlg(walletModel, platformStyle, this->aliaspeg, this->alias, this->offer, this->quantity, this->notes, this->title, this->currency, this->strSYSPrice, this->seller, this->address, ui->checkBox->isChecked()? ui->escrowEdit->text(): "", this);
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
	OfferAcceptDialogBTC dlg(walletModel, platformStyle, this->aliaspeg, this->alias, this->offer, this->quantity, this->notes, this->title, this->currency, this->strSYSPrice, this->seller, this->address, ui->checkBox->isChecked()? ui->escrowEdit->text(): "", this);
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
		if(!walletModel) return;
		WalletModel::UnlockContext ctx(walletModel->requestUnlock());
		if(!ctx.isValid())
		{
			return;
		}
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
		params.push_back(this->alias.toStdString());
		params.push_back(this->offer.toStdString());
		params.push_back(this->quantity.toStdString());
		params.push_back(this->notes.toStdString());


	    try {
            result = tableRPC.execute(strMethod, params);
			if (result.type() != UniValue::VNULL)
			{
				const UniValue& resArray = result.get_array();
				if(resArray.size() > 2)
				{
					const UniValue& complete_value = resArray[2];
					bool bComplete = false;
					if (complete_value.isStr())
						bComplete = complete_value.get_str() == "true";
					if(!bComplete)
					{
						string hex_str = resArray[0].get_str();
						GUIUtil::setClipboard(QString::fromStdString(hex_str));
						QMessageBox::information(this, windowTitle(),
							tr("This transaction requires more signatures. Transaction hex has been copied to your clipboard for your reference. Please provide it to a signee that has not yet signed."),
								QMessageBox::Ok, QMessageBox::Ok);
						return;
					}
				}
				const UniValue &arr = result.get_array();
				string strResult = arr[0].get_str();
				QString offerAcceptTXID = QString::fromStdString(strResult);
				if(offerAcceptTXID != QString(""))
				{
					OfferPayDialog dlg(platformStyle, this->title, this->quantity, this->qstrPrice, currency, this);
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
			tr("Error accepting offer: ") + QString::fromStdString(strError),
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
		if(!walletModel) return;
		WalletModel::UnlockContext ctx(walletModel->requestUnlock());
		if(!ctx.isValid())
		{
			return;
		}
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
				const UniValue& resArray = result.get_array();
				if(resArray.size() > 2)
				{
					const UniValue& complete_value = resArray[2];
					bool bComplete = false;
					if (complete_value.isStr())
						bComplete = complete_value.get_str() == "true";
					if(!bComplete)
					{
						string hex_str = resArray[0].get_str();
						GUIUtil::setClipboard(QString::fromStdString(hex_str));
						QMessageBox::information(this, windowTitle(),
							tr("This transaction requires more signatures. Transaction hex has been copied to your clipboard for your reference. Please provide it to a signee that has not yet signed."),
								QMessageBox::Ok, QMessageBox::Ok);
						return;
					}
				}
				const UniValue &arr = result.get_array();
				string strResult = arr[0].get_str();
				QString escrowTXID = QString::fromStdString(strResult);
				if(escrowTXID != QString(""))
				{
					OfferEscrowDialog dlg(platformStyle, this->title, this->quantity, this->qstrPrice, this->currency, this);
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
			tr("Error creating escrow: ") + QString::fromStdString(strError),
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
