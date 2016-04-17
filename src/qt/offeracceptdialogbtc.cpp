#include "offeracceptdialogbtc.h"
#include "ui_offeracceptdialogbtc.h"
#include "init.h"
#include "util.h"
#include "offerpaydialog.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "platformstyle.h"
#include "syscoingui.h"
#include <QMessageBox>
#include "rpcserver.h"
#include "pubkey.h"
#include "wallet/wallet.h"
#include "main.h"
#include "utilmoneystr.h"
#include <QDesktopServices>
#if QT_VERSION < 0x050000
#include <QUrl>
#else
#include <QUrlQuery>
#endif
#include <QPixmap>
#if defined(HAVE_CONFIG_H)
#include "config/syscoin-config.h" /* for USE_QRCODE */
#endif
#ifdef USE_QRCODE
#include <qrencode.h>
#endif
using namespace std;
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
extern const CRPCTable tableRPC;
OfferAcceptDialogBTC::OfferAcceptDialogBTC(const PlatformStyle *platformStyle, QString alias, QString offer, QString quantity, QString notes, QString title, QString currencyCode, QString qstrPrice, QString sellerAlias, QString address, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OfferAcceptDialogBTC), platformStyle(platformStyle), alias(alias), offer(offer), notes(notes), quantity(quantity), title(title), sellerAlias(sellerAlias), address(address)
{
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();  
	ui->aboutShadeBTC->setPixmap(QPixmap(":/images/" + theme + "/about_btc"));
	dblPrice = qstrPrice.toDouble()*quantity.toUInt();
	string strfPrice = strprintf("%f", dblPrice);
	QString fprice = QString::fromStdString(strfPrice);
	string strCurrencyCode = currencyCode.toStdString();
	ui->escrowDisclaimer->setText(tr("<font color='blue'>Please note escrow is not available since you are paying in BTC, only SYS payments can be escrowed. </font>"));
	ui->bitcoinInstructionLabel->setText(tr("After paying for this item, please enter the Bitcoin Transaction ID and click on the confirm button below. You may use the QR Code to the left to scan the payment request into your wallet or click on the Open BTC Wallet if you are on the desktop and have Bitcoin Core installed."));
	ui->acceptMessage->setText(tr("Are you sure you want to purchase %1 of '%2' from merchant: '%3'? To complete your purchase please pay %4 BTC to %5 using your Bitcoin wallet.").arg(quantity).arg(title).arg(sellerAlias).arg(fprice).arg(address));
	string strPrice = strprintf("%f", dblPrice);
	price = QString::fromStdString(strPrice);

	if (!platformStyle->getImagesOnButtons())
	{
		ui->confirmButton->setIcon(QIcon());
		ui->openBtcWalletButton->setIcon(QIcon());
		ui->cancelButton->setIcon(QIcon());

	}
	else
	{
		ui->confirmButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/transaction_confirmed"));
		ui->openBtcWalletButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/send"));
		ui->cancelButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/quit"));
	}	
	this->offerPaid = false;
	connect(ui->confirmButton, SIGNAL(clicked()), this, SLOT(tryAcceptOffer()));
	connect(ui->openBtcWalletButton, SIGNAL(clicked()), this, SLOT(openBTCWallet()));

#ifdef USE_QRCODE
	QString message = "Payment for offer ID " + this->offer + " on Syscoin Decentralized Marketplace.";
	SendCoinsRecipient info;
	info.address = this->address;
	info.label = this->sellerAlias;
	info.message = message;
	ParseMoney(price.toStdString(), info.amount);
	QString uri = GUIUtil::formatBitcoinURI(info);

	ui->lblQRCode->setText("");
    if(!uri.isEmpty())
    {
        // limit URI length
        if (uri.length() > MAX_URI_LENGTH)
        {
            ui->lblQRCode->setText(tr("Resulting URI too long, try to reduce the text for label / message."));
        } else {
            QRcode *code = QRcode_encodeString(uri.toUtf8().constData(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
            if (!code)
            {
                ui->lblQRCode->setText(tr("Error encoding URI into QR Code."));
                return;
            }
            QImage myImage = QImage(code->width + 8, code->width + 8, QImage::Format_RGB32);
            myImage.fill(0xffffff);
            unsigned char *p = code->data;
            for (int y = 0; y < code->width; y++)
            {
                for (int x = 0; x < code->width; x++)
                {
                    myImage.setPixel(x + 4, y + 4, ((*p & 1) ? 0x0 : 0xffffff));
                    p++;
                }
            }
            QRcode_free(code);
            ui->lblQRCode->setPixmap(QPixmap::fromImage(myImage).scaled(128, 128));
        }
    }
#endif
}
void OfferAcceptDialogBTC::on_cancelButton_clicked()
{
    reject();
}
OfferAcceptDialogBTC::~OfferAcceptDialogBTC()
{
    delete ui;
}

void OfferAcceptDialogBTC::slotConfirmedFinished(QNetworkReply * reply){
	if(reply->error() != QNetworkReply::NoError) {
		ui->confirmButton->setText(m_buttonText);
        QMessageBox::critical(this, windowTitle(),
            tr("Error making request: ") + reply->errorString(),
                QMessageBox::Ok, QMessageBox::Ok);
		return;
	}
	double valueAmount = 0;
	QString time;
	int height;
		
	QByteArray bytes = reply->readAll();
	QString str = QString::fromUtf8(bytes.data(), bytes.size());
	UniValue outerValue;
	bool read = outerValue.read(str.toStdString());
	if (read)
	{
		UniValue outerObj = outerValue.get_obj();
		UniValue statusValue = find_value(outerObj, "status");
		if (statusValue.isStr())
		{
			if(statusValue.get_str() != "success")
			{
				ui->confirmButton->setText(m_buttonText);
				QMessageBox::critical(this, windowTitle(),
					tr("Transaction status not successful: ") + QString::fromStdString(statusValue.get_str()),
						QMessageBox::Ok, QMessageBox::Ok);
				return;
			}
		}
		UniValue dataObj = find_value(outerObj, "data");
		UniValue heightValue = find_value(dataObj, "block");
		if (heightValue.isNum())
			height = heightValue.get_int();
		UniValue timeValue = find_value(dataObj, "time_utc");
		if (timeValue.isStr())
			time = QString::fromStdString(timeValue.get_str());
		UniValue outputsValue = find_value(dataObj, "vouts");
		if (outputsValue.isArray())
		{
			UniValue outputs = outputsValue.get_array();
			for (unsigned int idx = 0; idx < outputs.size(); idx++) {
				const UniValue& output = outputs[idx];	
				UniValue addressValue = find_value(output, "address");
				if(addressValue.isStr())
				{
					if(/*addressValue.get_str() == address.toStdString()*/1)
					{
						UniValue paymentValue = find_value(output, "amount");
						if(paymentValue.isStr())
						{
							valueAmount += QString::fromStdString(paymentValue.get_str()).toDouble();
	
							if(valueAmount >= dblPrice)
							{
								ui->confirmButton->setText(m_buttonText);
								QMessageBox::information(this, windowTitle(),
									tr("Transaction ID %1 was found in the Bitcoin blockchain! Full payment has been detected in block %2 at %3.").arg(ui->btctxidEdit->text().trimmed()).arg(height).arg(time),
									QMessageBox::Ok, QMessageBox::Ok);
								acceptOffer();
								return;
							}
						}
					}
						
				}
			}
		}
	}
	else
	{
		ui->confirmButton->setText(m_buttonText);
		QMessageBox::critical(this, windowTitle(),
			tr("Cannot parse JSON response: ") + str,
				QMessageBox::Ok, QMessageBox::Ok);
		return;
	}
	
	reply->deleteLater();
	ui->confirmButton->setText(m_buttonText);
	QMessageBox::warning(this, windowTitle(),
		tr("Payment not found in the Bitcoin blockchain! Please try again later."),
			QMessageBox::Ok, QMessageBox::Ok);	
}
void OfferAcceptDialogBTC::CheckPaymentInBTC()
{
	m_buttonText = ui->confirmButton->text();
	ui->confirmButton->setText(tr("Please Wait..."));
	QNetworkAccessManager *nam = new QNetworkAccessManager(this); 
	connect(nam, SIGNAL(finished(QNetworkReply *)), this, SLOT(slotConfirmedFinished(QNetworkReply *)));
	QUrl url("http://btc.blockr.io/api/v1/tx/info/" + ui->btctxidEdit->text().trimmed());
	QNetworkRequest request(url);
	nam->get(request);
}
// send offeraccept with offer guid/qty as params and then send offerpay with wtxid (first param of response) as param, using RPC commands.
void OfferAcceptDialogBTC::tryAcceptOffer()
{
	if (ui->btctxidEdit->text().trimmed().isEmpty()) {
        ui->btctxidEdit->setText("");
        QMessageBox::critical(this, windowTitle(),
        tr("Please enter a valid Bitcoin Transaction ID into the input box and try again"),
            QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
	CheckPaymentInBTC();		
}
void OfferAcceptDialogBTC::acceptOffer(){
		UniValue params(UniValue::VARR);
		UniValue valError;
		UniValue valResult;
		UniValue valId;
		UniValue result ;
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
		params.push_back(ui->btctxidEdit->text().trimmed().toStdString());

	    try {
            result = tableRPC.execute(strMethod, params);
			if (result.type() != UniValue::VNULL)
			{
				const UniValue &arr = result.get_array();
				string strResult = arr[0].get_str();
				QString offerAcceptTXID = QString::fromStdString(strResult);
				if(offerAcceptTXID != QString(""))
				{
					OfferPayDialog dlg(platformStyle, this->title, this->quantity, this->price, "BTC", this);
					dlg.exec();
					this->offerPaid = true;
					OfferAcceptDialogBTC::accept();
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
void OfferAcceptDialogBTC::openBTCWallet()
{
	QString message = "Payment for offer ID " + this->offer + " on Syscoin Decentralized Marketplace.";
	SendCoinsRecipient info;
	info.address = this->address;
	info.label = this->sellerAlias;
	info.message = message;
	ParseMoney(price.toStdString(), info.amount);
	QString uri = GUIUtil::formatBitcoinURI(info);
	QDesktopServices::openUrl(QUrl(uri, QUrl::TolerantMode));
}
bool OfferAcceptDialogBTC::getPaymentStatus()
{
	return this->offerPaid;
}
