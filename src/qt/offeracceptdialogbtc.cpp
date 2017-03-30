#include "offeracceptdialogbtc.h"
#include "ui_offeracceptdialogbtc.h"
#include "init.h"
#include "util.h"
#include "offerpaydialog.h"
#include "offerescrowdialog.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "platformstyle.h"
#include "syscoingui.h"
#include <QMessageBox>
#include "rpc/server.h"
#include "pubkey.h"
#include "wallet/wallet.h"
#include "walletmodel.h"
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
#include "qbtcjsonrpcclient.h"
extern CRPCTable tableRPC;
OfferAcceptDialogBTC::OfferAcceptDialogBTC(WalletModel* model, const PlatformStyle *platformStyle, QString strAliasPeg, QString alias, QString offer, QString quantity, QString notes, QString title, QString currencyCode, QString sysPrice, QString sellerAlias, QString address, QString arbiter, QWidget *parent) :
    QDialog(parent),
	walletModel(model),
    ui(new Ui::OfferAcceptDialogBTC), platformStyle(platformStyle), alias(alias), offer(offer), notes(notes), quantity(quantity), title(title), sellerAlias(sellerAlias), address(address), arbiter(arbiter)
{
	
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();
	ui->aboutShadeBTC->setPixmap(QPixmap(":/images/" + theme + "/about_btc"));
	ui->checkBox->setEnabled(false);
	ui->checkBox->setChecked(false);
	if(arbiter.size() > 0)
	{
		ui->checkBox->setEnabled(true);
		ui->checkBox->setChecked(true);
		ui->escrowEdit->setText(arbiter);
	}
    int btcprecision;
    CAmount btcPrice = convertSyscoinToCurrencyCode(vchFromString(strAliasPeg.toStdString()), vchFromString("BTC"), AmountFromValue(sysPrice.toStdString()), chainActive.Tip()->nHeight, btcprecision);
	if(btcPrice > 0)
		priceBtc = QString::fromStdString(strprintf("%.*f", btcprecision, ValueFromAmount(btcPrice).get_real()*quantity.toUInt()));
	else
	{
        QMessageBox::critical(this, windowTitle(),
            tr("Could not find BTC currency in the rates peg for this offer")
                ,QMessageBox::Ok, QMessageBox::Ok);
		reject();
		return;
	}

	string strCurrencyCode = currencyCode.toStdString();
	ui->bitcoinInstructionLabel->setText(tr("After paying for this item, please enter the Bitcoin Transaction ID and click on the confirm button below."));

	ui->escrowDisclaimer->setText(QString("<font color='blue'>") + tr("Enter a Syscoin arbiter that is mutally trusted between yourself and the merchant. Then enable the 'Use Escrow' checkbox") + QString("</font>"));
	ui->escrowDisclaimer->setVisible(true);
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
	connect(ui->checkBox,SIGNAL(clicked(bool)),SLOT(onEscrowCheckBoxChanged(bool)));
	connect(ui->confirmButton, SIGNAL(clicked()), this, SLOT(tryAcceptOffer()));
	connect(ui->openBtcWalletButton, SIGNAL(clicked()), this, SLOT(openBTCWallet()));
	setupEscrowCheckboxState(ui->checkBox->isChecked());
}
void OfferAcceptDialogBTC::on_escrowEdit_textChanged(const QString & text)
{
	if(ui->escrowEdit->text().size() > 0)
		ui->checkBox->setEnabled(true);
	else
		ui->checkBox->setEnabled(false);
}
void OfferAcceptDialogBTC::SetupQRCode(const QString& price)
{

#ifdef USE_QRCODE
	QString message = tr("Payment on Syscoin Decentralized Marketplace. Offer ID: ") + this->offer;
	SendCoinsRecipient info;
	info.address = this->multisigaddress.size() > 0? this->multisigaddress: this->address;
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
bool OfferAcceptDialogBTC::setupEscrowCheckboxState(bool desiredStateEnabled)
{
	double total = 0;
	if(desiredStateEnabled)
	{
		ui->checkBox->setChecked(true);
		// get new multisig address from escrow service
		UniValue params(UniValue::VARR);
		params.push_back(this->alias.toStdString());
		params.push_back(this->offer.toStdString());
		params.push_back(this->quantity.toStdString());
		params.push_back(ui->escrowEdit->text().trimmed().toStdString());
		params.push_back("BTC");
		UniValue resCreate;
		try
		{
			resCreate = tableRPC.execute("generateescrowmultisig", params);
		}
		catch (UniValue& objError)
		{
			ui->escrowDisclaimer->setText(QString("<font color='red'>") + tr("Failed to generate multisig address: ") + QString::fromStdString(find_value(objError, "message").get_str()) + QString("</font>"));
			return false;
		}
		if (!resCreate.isObject())
		{
			ui->escrowDisclaimer->setText(QString("<font color='red'>") + tr("Could not generate escrow multisig address: Invalid response from generateescrowmultisig") + QString("</font>"));
			return false;
		}

		const UniValue &o = resCreate.get_obj();
		const UniValue& redeemScript_value = find_value(o, "redeemScript");
		const UniValue& address_value = find_value(o, "address");
		const UniValue& height_value = find_value(o, "height");
		const UniValue& total_value = find_value(o, "total");
		if(total_value.isNum())
			total = total_value.get_real();
		if(height_value.isNum())
			m_height = height_value.get_int64();
		if (redeemScript_value.isStr())
		{
			m_redeemScript = QString::fromStdString(redeemScript_value.get_str());
		}
		else
		{
			ui->escrowDisclaimer->setText(QString("<font color='red'>") + tr("Could not create escrow transaction: could not find redeem script in response") + QString("</font>"));
			return false;
		}

		if (address_value.isStr())
		{
			multisigaddress = QString::fromStdString(address_value.get_str());
		}
		else
		{
			ui->escrowDisclaimer->setText(QString("<font color='red'>") + tr("Could not create escrow transaction: could not find multisig address in response") + QString("</font>"));
			return false;
		}
		qstrPrice = QString::number(total);
		ui->acceptMessage->setText(tr("Are you sure you want to purchase") + QString(" <b>%1</b> ").arg(quantity) + tr("of") +  QString(" <b>%1</b> ").arg(title) + tr("from merchant") + QString(" <b>%1</b>").arg(sellerAlias) + QString("? ") + tr("Follow the steps below to successfully pay via Bitcoin:") + QString("<br/><br/>") + tr("1. If you are using escrow, please enter your escrow arbiter in the input box below and check the 'Use Escrow' checkbox. Leave the escrow checkbox unchecked if you do not wish to use escrow.") + QString("<br/>") + tr("2. Open your Bitcoin wallet. You may use the QR Code to the left to scan the payment request into your wallet or click on 'Open BTC Wallet' if you are on the desktop and have Bitcoin Core installed.") + QString("<br/>") + tr("3. Pay") + QString(" <b>%1 BTC</b> ").arg(qstrPrice) + tr("to") + QString(" <b>%5</b> ").arg(multisigaddress) + tr("using your Bitcoin wallet. Please enable dynamic fees in your BTC wallet upon payment for confirmation in a timely manner.") + QString("<br/>") + tr("4. Enter the Transaction ID and then click on the 'Confirm Payment' button once you have paid."));
		ui->escrowDisclaimer->setText(QString("<font color='green'>") + tr("Escrow created successfully! Please fund using BTC address ") + QString("<b>%1</b></font>").arg(multisigaddress));

	}
	else
	{
		ui->escrowDisclaimer->setText(QString("<font color='blue'>") + tr("Enter a Syscoin arbiter that is mutally trusted between yourself and the merchant. Then enable the 'Use Escrow' checkbox") + QString("</font>"));
		qstrPrice = priceBtc;
		ui->acceptMessage->setText(tr("Are you sure you want to purchase") + QString(" <b>%1</b> ").arg(quantity) + tr("of") +  QString(" <b>%1</b> ").arg(title) + tr("from merchant") + QString(" <b>%1</b>").arg(sellerAlias) + QString("? ") + tr("Follow the steps below to successfully pay via Bitcoin:") + QString("<br/><br/>") + tr("1. If you are using escrow, please enter your escrow arbiter in the input box below and check the 'Use Escrow' checkbox. Leave the escrow checkbox unchecked if you do not wish to use escrow.") + QString("<br/>") + tr("2. Open your Bitcoin wallet. You may use the QR Code to the left to scan the payment request into your wallet or click on 'Open BTC Wallet' if you are on the desktop and have Bitcoin Core installed.") + QString("<br/>") + tr("3. Pay") + QString(" <b>%1 BTC</b> ").arg(qstrPrice) + tr("to") + QString(" <b>%5</b> ").arg(address) + tr("using your Bitcoin wallet. Please enable dynamic fees in your BTC wallet upon payment for confirmation in a timely manner.") + QString("<br/>") + tr("4. Enter the Transaction ID and then click on the 'Confirm Payment' button once you have paid."));
		SetupQRCode(qstrPrice);
		return false;
	}
	SetupQRCode(qstrPrice);
	return true;
}
void OfferAcceptDialogBTC::onEscrowCheckBoxChanged(bool toggled)
{
	ui->checkBox->setChecked(setupEscrowCheckboxState(toggled));
}
void OfferAcceptDialogBTC::slotConfirmedFinished(QNetworkReply * reply){
	if(reply->error() != QNetworkReply::NoError) {
		ui->confirmButton->setText(m_buttonText);
		ui->confirmButton->setEnabled(true);
        QMessageBox::critical(this, windowTitle(),
            tr("Error making request: ") + reply->errorString(),
                QMessageBox::Ok, QMessageBox::Ok);
		reply->deleteLater();
		return;
	}
	double valueAmount = 0;

	QByteArray bytes = reply->readAll();
	QString str = QString::fromUtf8(bytes.data(), bytes.size());
	UniValue outerValue;
	bool read = outerValue.read(str.toStdString());
	if (read && outerValue.isObject())
	{
		UniValue outerObj = outerValue.get_obj();
		UniValue resultValue = find_value(outerObj, "result");
		if(!resultValue.isObject())
		{
			QMessageBox::critical(this, windowTitle(),
				tr("Cannot parse JSON results"),
					QMessageBox::Ok, QMessageBox::Ok);
			reply->deleteLater();
			return;
		}
		UniValue resultObj = resultValue.get_obj();
		UniValue outputsValue = find_value(resultObj, "vout");
		if (outputsValue.isArray())
		{
			UniValue outputs = outputsValue.get_array();
			for (unsigned int idx = 0; idx < outputs.size(); idx++) {
				const UniValue& output = outputs[idx].get_obj();
				UniValue paymentValue = find_value(output, "value");
				UniValue scriptPubKeyObj = find_value(output, "scriptPubKey").get_obj();
				UniValue addressesValue = find_value(scriptPubKeyObj, "addresses");
				if(addressesValue.isArray() &&  addressesValue.get_array().size() == 1)
				{
					UniValue addressValue = addressesValue.get_array()[0];
					if(addressValue.get_str() == m_address.toStdString())
					{
						if(paymentValue.isNum())
						{
							valueAmount += paymentValue.get_real();
							if(valueAmount >= this->qstrPrice.toDouble())
							{
								ui->confirmButton->setText(m_buttonText);
								ui->confirmButton->setEnabled(true);
								QMessageBox::information(this, windowTitle(),
									tr("Transaction was found in the Bitcoin blockchain! Full payment has been detected. TXID: ") + ui->exttxidEdit->text().trimmed(),
									QMessageBox::Ok, QMessageBox::Ok);
								reply->deleteLater();
								if(ui->checkBox->isChecked())
									acceptEscrow();
								else
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
		ui->confirmButton->setEnabled(true);
		QMessageBox::critical(this, windowTitle(),
			tr("Cannot parse JSON response: ") + str,
				QMessageBox::Ok, QMessageBox::Ok);
		reply->deleteLater();
		return;
	}

	reply->deleteLater();
	ui->confirmButton->setText(m_buttonText);
	ui->confirmButton->setEnabled(true);
	QMessageBox::warning(this, windowTitle(),
		tr("Payment not found in the Bitcoin blockchain! Please try again later"),
			QMessageBox::Ok, QMessageBox::Ok);
}
void OfferAcceptDialogBTC::CheckPaymentInBTC()
{
	if(!ui->checkBox->isChecked())
		m_address = address;
	else
		m_address = multisigaddress;
	m_buttonText = ui->confirmButton->text();
	ui->confirmButton->setText(tr("Please Wait..."));
	ui->confirmButton->setEnabled(false);

	BtcRpcClient btcClient;
	QNetworkAccessManager *nam = new QNetworkAccessManager(this);  
	connect(nam, SIGNAL(finished(QNetworkReply *)), this, SLOT(slotConfirmedFinished(QNetworkReply *)));
	btcClient.sendRawTxRequest(nam, ui->exttxidEdit->text().trimmed());

}
// send offeraccept with offer guid/qty as params and then send offerpay with wtxid (first param of response) as param, using RPC commands.
void OfferAcceptDialogBTC::tryAcceptOffer()
{
	if (ui->exttxidEdit->text().trimmed().isEmpty()) {
        ui->exttxidEdit->setText("");
        QMessageBox::critical(this, windowTitle(),
        tr("Please enter a valid Bitcoin Transaction ID into the input box and try again"),
            QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
	CheckPaymentInBTC();
}
void OfferAcceptDialogBTC::acceptOffer(){
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
		params.push_back(ui->exttxidEdit->text().trimmed().toStdString());
		params.push_back("BTC");


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
					OfferPayDialog dlg(platformStyle, this->title, this->quantity, this->qstrPrice, "BTC", this);
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
void OfferAcceptDialogBTC::acceptEscrow()
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
		params.push_back(ui->exttxidEdit->text().trimmed().toStdString());
		params.push_back("BTC");
		params.push_back(m_redeemScript.toStdString());
		params.push_back(QString::number(m_height).toStdString());

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
					OfferEscrowDialog dlg(platformStyle, this->title, this->quantity, this->qstrPrice, "BTC", this);
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
void OfferAcceptDialogBTC::openBTCWallet()
{
	QString message = tr("Payment on Syscoin Decentralized Marketplace. Offer ID: ") + this->offer;
	SendCoinsRecipient info;
	info.address = this->multisigaddress.size() > 0? this->multisigaddress: this->address;
	info.label = this->sellerAlias;
	info.message = message;
	ParseMoney(this->qstrPrice.toStdString(), info.amount);
	QString uri = GUIUtil::formatBitcoinURI(info);
	QDesktopServices::openUrl(QUrl(uri, QUrl::TolerantMode));
}
bool OfferAcceptDialogBTC::getPaymentStatus()
{
	return this->offerPaid;
}
