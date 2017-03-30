#include "manageescrowdialog.h"
#include "ui_manageescrowdialog.h"

#include "guiutil.h"
#include "syscoingui.h"
#include "platformstyle.h"
#include "ui_interface.h"
#include <QMessageBox>
#include "rpc/server.h"
#include "walletmodel.h"
#include "qzecjsonrpcclient.h"
#include "qbtcjsonrpcclient.h"
#if QT_VERSION < 0x050000
#include <QUrl>
#else
#include <QUrlQuery>
#endif
using namespace std;
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
extern CRPCTable tableRPC;
ManageEscrowDialog::ManageEscrowDialog(WalletModel* model, const QString &escrow, QWidget *parent) :
    QDialog(parent),
	walletModel(model),
    ui(new Ui::ManageEscrowDialog), escrow(escrow)
{
    ui->setupUi(this);
	refundWarningStr = releaseWarningStr = "";
	QString theme = GUIUtil::getThemeName();  
	ui->aboutEscrow->setPixmap(QPixmap(":/images/" + theme + "/escrow"));
	QString buyer, seller, reseller, arbiter, status, offertitle, total;
	ui->primaryLabel->setVisible(false);
	ui->primaryRating->setVisible(false);
	ui->primaryFeedback->setVisible(false);
	ui->secondaryLabel->setVisible(false);
	ui->secondaryRating->setVisible(false);
	ui->secondaryFeedback->setVisible(false);
	if(!loadEscrow(escrow, buyer, seller, reseller, arbiter, status, offertitle, total, m_exttxid, m_paymentOption, m_redeemTxId))
	{
		ui->manageInfo2->setText(tr("Cannot find this escrow on the network, please try again later."));
		ui->releaseButton->setEnabled(false);
		ui->refundButton->setEnabled(false);
		return;
	}

	escrowRoleType = findYourEscrowRoleFromAliases(buyer, seller, reseller, arbiter);
	ui->manageInfo->setText(tr("You are managing escrow ID") + QString(" <b>%1</b>. ").arg(escrow) + tr("Offer:") + QString(" <b>%1</b> ").arg(offertitle) + tr("totalling") + QString(" <b>%1</b>. ").arg(total) + tr("The buyer:") + QString(" <b>%1</b>, ").arg(buyer) + tr("merchant:") + QString(" <b>%1</b>, ").arg(reseller.size() == 0? seller: reseller) + tr("arbiter:") + QString(" <b>%1</b>.").arg(arbiter));
	if(escrowRoleType == None)
	{
		ui->manageInfo2->setText(tr("You cannot manage this escrow because you do not own one of either the buyer, merchant or arbiter aliases."));
		ui->releaseButton->setEnabled(false);
		ui->refundButton->setEnabled(false);
	}
	else if(status.indexOf("in escrow") == 0)
	{
		if(escrowRoleType == Buyer)
		{
			ui->manageInfo2->setText(tr("You are the 'buyer' of the offer held in escrow, you may release the coins to the merchant once you have confirmed that you have recieved the item as per the description of the offer."));
			ui->refundButton->setEnabled(false);
		}
		else if(escrowRoleType == Seller)
		{
			ui->manageInfo2->setText(tr("You are the 'merchant' of the offer held in escrow, you may refund the coins back to the buyer."));
			ui->releaseButton->setEnabled(false);
		}
		else if(escrowRoleType == Arbiter)
		{
			ui->manageInfo2->setText(tr("You are the 'arbiter' of the offer held in escrow, you may refund the coins back to the buyer if you have evidence that the merchant did not honour the agreement to ship the offer item. You may also release the coins to the merchant if the buyer has not released in a timely manor. You may use Syscoin messages to communicate with the buyer and merchant to ensure you have adequate proof for your decision."));
		}

	}
	else if(status.indexOf("escrow released") == 0)
	{
		if(escrowRoleType == Buyer)
		{
			ui->manageInfo2->setText(tr("You are the 'buyer' of the offer held in escrow. The escrow has been released to the merchant. You may communicate with your arbiter or merchant via Syscoin messages. You may leave feedback after the money is claimed by the merchant."));
			ui->refundButton->setEnabled(false);
			ui->releaseButton->setEnabled(false);
		}
		else if(escrowRoleType == Seller)
		{
			ui->manageInfo2->setText(tr("You are the 'merchant' of the offer held in escrow. The payment of coins have been released to you, you may claim them now. After claiming, please return to this dialog and provide feedback for this escrow transaction."));
			ui->releaseButton->setText(tr("Claim Payment"));
			ui->refundButton->setEnabled(false);

		}
		else if(escrowRoleType == Arbiter)
		{
			ui->manageInfo2->setText(tr("You are the 'arbiter' of the offer held in escrow. The escrow has been released to the merchant. You may re-release this escrow if there are any problems claiming the coins by the merchant. If you were the one to release the coins you will recieve a commission as soon as the merchant claims his payment. You may leave feedback after the money is claimed by the merchant."));
			releaseWarningStr = tr("Warning: Payment has already been released, are you sure you wish to re-release payment to the merchant?");
			ui->refundButton->setEnabled(false);
		}
	}
	else if(status.indexOf("escrow refunded") == 0)
	{
		if(escrowRoleType == Buyer)
		{
			ui->manageInfo2->setText(tr("You are the 'buyer' of the offer held in escrow. The coins have been refunded back to you, you may claim them now. After claiming, please return to this dialog and provide feedback for this escrow transaction."));
			ui->refundButton->setText(tr("Claim Refund"));
			ui->releaseButton->setEnabled(false);
		}
		else if(escrowRoleType == Seller)
		{
			ui->manageInfo2->setText(tr("You are the 'merchant' of the offer held in escrow. The escrow has been refunded back to the buyer. You may leave feedback after the money is claimed by the buyer."));
			ui->refundButton->setEnabled(false);
			ui->releaseButton->setEnabled(false);

		}
		else if(escrowRoleType == Arbiter)
		{
			ui->manageInfo2->setText(tr("You are the 'arbiter' of the offer held in escrow. The escrow has been refunded back to the buyer. You may re-issue a refund if there are any problems claiming the coins by the buyer. If you were the one to refund the coins you will recieve a commission as soon as the buyer claims his refund. You may leave feedback after the money is claimed by the buyer."));
			ui->releaseButton->setEnabled(false);
			refundWarningStr = tr("Warning: Payment has already been refunded, are you sure you wish to re-refund payment back to the buyer?");	
		}
	}
	else if(status.indexOf("escrow release complete") == 0)
	{		
		ui->manageInfo2->setText(tr("The escrow has been successfully claimed by the merchant. The escrow is complete."));
		ui->refundButton->setEnabled(false);
		ui->refundButton->setVisible(false);
		ui->primaryLabel->setVisible(true);
		ui->primaryRating->setVisible(true);
		ui->primaryFeedback->setVisible(true);
		ui->secondaryLabel->setVisible(true);
		ui->secondaryRating->setVisible(true);
		ui->secondaryFeedback->setVisible(true);
		ui->releaseButton->setText(tr("Leave Feedback"));

		if(escrowRoleType == Buyer)
		{
			ui->primaryLabel->setText("Choose a rating for the merchant (1-5) or leave at 0 for no rating. Below please give feedback to the merchant.");
			ui->secondaryLabel->setText("Choose a rating for the arbiter (1-5) or leave at 0 for no rating. Below please give feedback to the arbiter. Skip if escrow arbiter was not involved.");
		}
		else if(escrowRoleType == ReSeller || escrowRoleType == Seller)
		{
			ui->primaryLabel->setText("Choose a rating for the buyer (1-5) or leave at 0 for no rating. Below please give feedback to the buyer.");
			ui->secondaryLabel->setText("Choose a rating for the arbiter (1-5) or leave at 0 for no rating. Below please give feedback to the arbiter. Skip if escrow arbiter was not involved.");

		}
		else if(escrowRoleType == Arbiter)
		{
			ui->primaryLabel->setText("Choose a rating for the buyer (1-5) or leave at 0 for no rating. Below please give feedback to the buyer.");
			ui->secondaryLabel->setText("Choose a rating for the merchant (1-5) or leave at 0 for no rating. Below please give feedback to the merchant.");	
		}
	}
	else if(status.indexOf("escrow refund complete") == 0)
	{	
		
	
		ui->manageInfo2->setText(tr("The escrow has been successfully refunded to the buyer. The escrow is complete."));
		ui->refundButton->setEnabled(false);
		ui->refundButton->setVisible(false);
		ui->primaryLabel->setVisible(true);
		ui->primaryRating->setVisible(true);
		ui->primaryFeedback->setVisible(true);
		ui->secondaryLabel->setVisible(true);
		ui->secondaryRating->setVisible(true);
		ui->secondaryFeedback->setVisible(true);
		ui->releaseButton->setText(tr("Leave Feedback"));
		if(escrowRoleType == Buyer)
		{
			ui->primaryLabel->setText("Choose a rating for the merchant (1-5) or leave at 0 for no rating. Below please give feedback to the merchant.");
			ui->secondaryLabel->setText("Choose a rating for the arbiter (1-5) or leave at 0 for no rating. Below please give feedback to the arbiter. Skip if escrow arbiter was not involved.");	
		}
		else if(escrowRoleType == ReSeller)
		{
			ui->primaryLabel->setText("Choose a rating for the buyer (1-5) or leave at 0 for no rating. Below please give feedback to the buyer.");
			ui->secondaryLabel->setText("Choose a rating for the arbiter (1-5) or leave at 0 for no rating. Below please give feedback to the arbiter. Skip if escrow arbiter was not involved.");
		}
		else if(escrowRoleType == Arbiter)
		{
			ui->primaryLabel->setText("Choose a rating for the buyer (1-5) or leave at 0 for no rating. Below please give feedback to the buyer.");
			ui->secondaryLabel->setText("Choose a rating for the merchant (1-5) or leave at 0 for no rating. Below please give feedback to the merchant.");	
		}

	}
	else
	{
		ui->manageInfo2->setText(tr("The escrow status was not recognized. Please contact the Syscoin team."));
		ui->refundButton->setEnabled(false);
		ui->releaseButton->setEnabled(false);
	}
}
bool ManageEscrowDialog::loadEscrow(const QString &escrow, QString &buyer, QString &seller,  QString &reseller, QString &arbiter, QString &status, QString &offertitle, QString &total, QString &exttxid, QString &paymentOption, QString &redeemtxid)
{
	QSettings settings;
	string strMethod = string("escrowinfo");
    UniValue params(UniValue::VARR); 
	params.push_back(escrow.toStdString());
	UniValue result ;
	string name_str;
	try {
		result = tableRPC.execute(strMethod, params);
		if (result.type() == UniValue::VOBJ)
		{
			const UniValue& o = result.get_obj();
	
			const UniValue& name_value = find_value(o, "escrow");
			if (name_value.type() == UniValue::VSTR)
				name_str = name_value.get_str();
			if(QString::fromStdString(name_str) != escrow)
				return false;
			const UniValue& seller_value = find_value(o, "seller");
			if (seller_value.type() == UniValue::VSTR)
				seller = QString::fromStdString(seller_value.get_str());
			const UniValue& reseller_value = find_value(o, "offerlink_seller");
			if (reseller_value.type() == UniValue::VSTR)
				reseller = QString::fromStdString(reseller_value.get_str());
			const UniValue& arbiter_value = find_value(o, "arbiter");
			if (arbiter_value.type() == UniValue::VSTR)
				arbiter = QString::fromStdString(arbiter_value.get_str());
			const UniValue& buyer_value = find_value(o, "buyer");
			if (buyer_value.type() == UniValue::VSTR)
				buyer = QString::fromStdString(buyer_value.get_str());
			const UniValue& offertitle_value = find_value(o, "offertitle");
			if (offertitle_value.type() == UniValue::VSTR)
				offertitle = QString::fromStdString(offertitle_value.get_str());
			string currency_str = "";
			const UniValue& currency_value = find_value(o, "currency");
			if (currency_value.type() == UniValue::VSTR)
				currency_str = currency_value.get_str();
			const UniValue& total_value = find_value(o, "total");
			if (total_value.type() == UniValue::VSTR)
				total = QString::fromStdString(total_value.get_str() + string(" ") + currency_str);
			const UniValue& status_value = find_value(o, "status");
			if (status_value.type() == UniValue::VSTR)
				status = QString::fromStdString(status_value.get_str());
			const UniValue& exttxid_value = find_value(o, "exttxid");
			if (exttxid_value.type() == UniValue::VSTR)
				exttxid = QString::fromStdString(exttxid_value.get_str());
			const UniValue& paymentOption_Value = find_value(o, "paymentoption_display");
			if (paymentOption_Value.type() == UniValue::VSTR)
				paymentOption = QString::fromStdString(paymentOption_Value.get_str());
			const UniValue& redeemtxid_value = find_value(o, "redeem_txid");
			if (redeemtxid_value.type() == UniValue::VSTR)
				redeemtxid = QString::fromStdString(redeemtxid_value.get_str());
			return true;
		}
	}
	catch (UniValue& objError)
	{
		return false;
	}
	catch(std::exception& e)
	{
		return false;
	}
	return false;
}
QString ManageEscrowDialog::EscrowRoleTypeToString(const EscrowRoleType& escrowType)
{
	if(escrowType == Arbiter)
		return tr("arbiter");
	else if(escrowType == Seller)
		return tr("seller");
	else if(escrowType == ReSeller)
		return tr("reseller");
	else if(escrowType == Buyer)
		return tr("buyer");
	else
		return tr("none");
}
void ManageEscrowDialog::on_cancelButton_clicked()
{
    reject();
}
void ManageEscrowDialog::on_extButton_clicked()
{
	if(m_paymentOption == QString("BTC"))
    	CheckPaymentInBTC();
	else if(m_paymentOption == QString("ZEC"))
		CheckPaymentInZEC();
}

ManageEscrowDialog::~ManageEscrowDialog()
{
    delete ui;
}
void ManageEscrowDialog::onLeaveFeedback()
{
	UniValue params(UniValue::VARR);
	string strMethod = string("escrowfeedback");
	params.push_back(escrow.toStdString());
	params.push_back(EscrowRoleTypeToString(escrowRoleType).toStdString());
	params.push_back(ui->primaryFeedback->toPlainText().toStdString());
	params.push_back(ui->primaryRating->cleanText().toStdString());
	params.push_back(ui->secondaryFeedback->toPlainText().toStdString());
	params.push_back(ui->secondaryRating->cleanText().toStdString());
	try {
		UniValue result = tableRPC.execute(strMethod, params);
		const UniValue& resArray = result.get_array();
		if(resArray.size() > 1)
		{
			const UniValue& complete_value = resArray[1];
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
		QMessageBox::information(this, windowTitle(),
		tr("Thank you for your feedback!"),
			QMessageBox::Ok, QMessageBox::Ok);
		ManageEscrowDialog::accept();
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
        tr("Error sending feedback: ") + QString::fromStdString(strError),
			QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
            tr("General exception sending feedbackescrow"),
			QMessageBox::Ok, QMessageBox::Ok);
	}	
}
bool ManageEscrowDialog::CompleteEscrowRelease()
{
	QString chain;
	if(m_paymentOption == "BTC")
		chain = tr("Bitcoin");
	else if(m_paymentOption == "ZEC")
		chain = tr("ZCash");
	UniValue params(UniValue::VARR);
	string strMethod = string("escrowcompleterelease");
	params.push_back(escrow.toStdString());
	params.push_back(this->m_rawTx.toStdString());

	try {
		UniValue result = tableRPC.execute(strMethod, params);

		
		const UniValue& resArray = result.get_array();
		if(resArray.size() > 1)
		{
			const UniValue& complete_value = resArray[1];
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
				return true;
			}	
		}
		if(m_exttxid.size() > 0)
		{
			QMessageBox::information(this, windowTitle(),
				tr("Escrow release completed successfully! Escrow spending payment was found on the blockchain. You may click on the 'Check External Payment' button to check to see if it has confirmed. Chain: ") + chain,
				QMessageBox::Ok, QMessageBox::Ok);
		}
		else
		{
			QMessageBox::information(this, windowTitle(),
			tr("Escrow release completed successfully! "),
				QMessageBox::Ok, QMessageBox::Ok);
		}
		ManageEscrowDialog::accept();
		return true;
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
        tr("Error completing release: ") + QString::fromStdString(strError),
			QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
            tr("General exception completing release"),
			QMessageBox::Ok, QMessageBox::Ok);
	}
	return false;
}
bool ManageEscrowDialog::CompleteEscrowRefund()
{
	QString chain;
	if(m_paymentOption == "BTC")
		chain = tr("Bitcoin");
	else if(m_paymentOption == "ZEC")
		chain = tr("ZCash");
	UniValue params(UniValue::VARR);
	string strMethod = string("escrowcompleterefund");
	params.push_back(escrow.toStdString());
	params.push_back(this->m_rawTx.toStdString());

	try {
		UniValue result = tableRPC.execute(strMethod, params);

		const UniValue& resArray = result.get_array();
		if(resArray.size() > 1)
		{
			const UniValue& complete_value = resArray[1];
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
				return true;
			}
		}
		if(m_exttxid.size() > 0)
		{
			QMessageBox::information(this, windowTitle(),
				tr("Escrow refund completed successfully! Escrow spending payment was found on the blockchain. You may click on the 'Check External Payment' button to check to see if the payment has confirmed. Chain: ") + chain,
				QMessageBox::Ok, QMessageBox::Ok);
		}
		else
		{
			QMessageBox::information(this, windowTitle(),
			tr("Escrow refund completed successfully!"),
				QMessageBox::Ok, QMessageBox::Ok);
		}
		ManageEscrowDialog::accept();
		return true;
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
        tr("Error completing refund: ") + QString::fromStdString(strError),
			QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
            tr("General exception completing refund"),
			QMessageBox::Ok, QMessageBox::Ok);
	}
	return false;
}
void ManageEscrowDialog::slotConfirmedFinished(QNetworkReply * reply){
	QString chain;
	if(m_paymentOption == "BTC")
		chain = tr("Bitcoin");
	else if(m_paymentOption == "ZEC")
		chain = tr("ZCash");
	QByteArray bytes = reply->readAll();
	QString str = QString::fromUtf8(bytes.data(), bytes.size());
	UniValue outerValue;
	int codeNum = 0;
	string errorMessage;
	bool read = outerValue.read(str.toStdString());
	if (read && outerValue.isObject())
	{
		UniValue outerObj = outerValue.get_obj();
		UniValue errorValue = find_value(outerObj, "error");
		if(errorValue.isObject())
		{
			UniValue errorObj = errorValue.get_obj();
			UniValue codeValue = find_value(errorObj, "code");
			UniValue messageValue = find_value(errorObj, "message");
			if(codeValue.isNum())
				codeNum = codeValue.get_int();
			if(messageValue.isStr())
				errorMessage = messageValue.get_str();
		}
	}
	
	if(codeNum < 0)
	{
		if(codeNum != -27 && errorMessage != "transaction already in block chain")
		{
			QMessageBox::critical(this, windowTitle(),
				tr("Could not send raw escrow transaction to the blockchain. Chain: ") + chain + QString(" error code: %1 message %2: ").arg(codeNum).arg(QString::fromStdString(errorMessage)),
					QMessageBox::Ok, QMessageBox::Ok);
		}
	}
	else if(reply->error() != QNetworkReply::NoError) {
		if(m_buttontext == tr("Claim Payment"))
		{
			ui->releaseButton->setText(m_buttontext);
			ui->releaseButton->setEnabled(true);
		}
		else if(m_buttontext == tr("Claim Refund"))
		{
			ui->refundButton->setText(m_buttontext);
			ui->refundButton->setEnabled(true);	
		}
        QMessageBox::critical(this, windowTitle(),
			tr("Could not send raw escrow transaction to the blockchain, error: ") + reply->errorString(),
                QMessageBox::Ok, QMessageBox::Ok);
		reply->deleteLater();
		return;
	}
	reply->deleteLater();
	if(m_buttontext == tr("Claim Payment"))
	{
		if(!CompleteEscrowRelease())
		{
			if(m_buttontext == tr("Claim Payment"))
			{
				ui->releaseButton->setText(m_buttontext);
				ui->releaseButton->setEnabled(true);
			}
			else if(m_buttontext == tr("Claim Refund"))
			{
				ui->refundButton->setText(m_buttontext);
				ui->refundButton->setEnabled(true);	
			}
			return;
		}
	}
	else if(m_buttontext == tr("Claim Refund"))
	{
		if(!CompleteEscrowRefund())
		{
			if(m_buttontext == tr("Claim Payment"))
			{
				ui->releaseButton->setText(m_buttontext);
				ui->releaseButton->setEnabled(true);
			}
			else if(m_buttontext == tr("Claim Refund"))
			{
				ui->refundButton->setText(m_buttontext);
				ui->refundButton->setEnabled(true);	
			}
			return;
		}
	}
	if(m_buttontext == tr("Claim Payment"))
	{
		ui->releaseButton->setText(m_buttontext);
		ui->releaseButton->setEnabled(false);
	}
	else if(m_buttontext == tr("Claim Refund"))
	{
		ui->refundButton->setText(m_buttontext);
		ui->refundButton->setEnabled(false);	
	}
}
void ManageEscrowDialog::SendRawTxBTC()
{
	BtcRpcClient btcClient;	
	QNetworkAccessManager *nam = new QNetworkAccessManager(this);  
	connect(nam, SIGNAL(finished(QNetworkReply *)), this, SLOT(slotConfirmedFinished(QNetworkReply *)));
	btcClient.sendRequest(nam, "sendrawtransaction", m_rawTx);
}
void ManageEscrowDialog::SendRawTxZEC()
{
	ZecRpcClient zecClient;	
	QNetworkAccessManager *nam = new QNetworkAccessManager(this);  
	connect(nam, SIGNAL(finished(QNetworkReply *)), this, SLOT(slotConfirmedFinished(QNetworkReply *)));
	zecClient.sendRequest(nam, "sendrawtransaction", m_rawTx);
}
void ManageEscrowDialog::slotConfirmedFinishedCheck(QNetworkReply * reply){
	QString chain;
	if(m_paymentOption == "BTC")
		chain = tr("Bitcoin");
	else if(m_paymentOption == "ZEC")
		chain = tr("ZCash");

	if(reply->error() != QNetworkReply::NoError) {

		GUIUtil::setClipboard(m_checkTxId);
        QMessageBox::critical(this, windowTitle(),
			tr("Could not find escrow payment on the blockchain, please ensure that the payment transaction has been confirmed on the network. Payment ID has been copied to your clipboard for your reference. error: ") + reply->errorString(),
                QMessageBox::Ok, QMessageBox::Ok);
		reply->deleteLater();
		m_bRelease = false;
		m_bRefund = false;
		return;
	}
	unsigned int time;
			
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
			m_bRelease = false;
			m_bRefund = false;
			return;
		}
		UniValue resultObj = resultValue.get_obj();
		UniValue timeValue = find_value(resultObj, "time");
		QDateTime timestamp;
		if (timeValue.isNum())
		{
			time = timeValue.get_int();
			timestamp.setTime_t(time);
		}

		UniValue unconfirmedValue = find_value(resultObj, "confirmations");
		if (unconfirmedValue.isNum())
		{
			int confirmations = unconfirmedValue.get_int();
			if(confirmations >= 1)
			{
				UniValue hexValue = find_value(resultObj, "hex");
				if (hexValue.isStr())
				{
					const QString &rawTx = QString::fromStdString(hexValue.get_str());		
					if(m_bRelease)
					{
						m_bRelease = false;
						doRelease(rawTx);
					}
					else if(m_bRefund)
					{
						m_bRefund = false;
						doRefund(rawTx);
					}
					reply->deleteLater();
					return;
				}
				
			}
		}
	}
	else
	{
		QMessageBox::critical(this, windowTitle(),
			tr("Cannot parse JSON response: ") + str,
				QMessageBox::Ok, QMessageBox::Ok);
		reply->deleteLater();
		m_bRelease = false;
		m_bRefund = false;
		return;
	}
	reply->deleteLater();	
	GUIUtil::setClipboard(m_checkTxId);
	QMessageBox::warning(this, windowTitle(),
		tr("Escrow payment found in the blockchain but it has not been confirmed yet. Please try again later. Payment ID has been copied to your clipboard for your reference. Chain: ") + chain,
			QMessageBox::Ok, QMessageBox::Ok);	
	m_bRelease = false;
	m_bRefund = false;
}

void ManageEscrowDialog::CheckPaymentInBTC()
{
	BtcRpcClient btcClient;
	QNetworkAccessManager *nam = new QNetworkAccessManager(this);  
	connect(nam, SIGNAL(finished(QNetworkReply *)), this, SLOT(slotConfirmedFinishedCheck(QNetworkReply *)));
	m_checkTxId = m_redeemTxId.size() > 0? m_redeemTxId: m_exttxid;
	btcClient.sendRawTxRequest(nam, m_checkTxId);
}
void ManageEscrowDialog::CheckPaymentInZEC()
{
	ZecRpcClient zecClient;

	QNetworkAccessManager *nam = new QNetworkAccessManager(this);  
	connect(nam, SIGNAL(finished(QNetworkReply *)), this, SLOT(slotConfirmedFinishedCheck(QNetworkReply *)));
	m_checkTxId = m_redeemTxId.size() > 0? m_redeemTxId: m_exttxid;
	zecClient.sendRawTxRequest(nam, m_checkTxId);
}
void ManageEscrowDialog::onRelease()
{
	if(this->m_exttxid.size() > 0)
	{
		m_bRelease = true;
		m_bRefund = false;
		on_extButton_clicked();
	}
	else
		doRelease();
}
void ManageEscrowDialog::onRefund()
{
	if(this->m_exttxid.size() > 0)
	{
		m_bRelease = false;
		m_bRefund = true;
		on_extButton_clicked();
	}
	else
		doRefund();
}
void ManageEscrowDialog::doRelease(const QString &rawTx)
{
	UniValue params(UniValue::VARR);
	params.push_back(escrow.toStdString());
	string strMethod;
	if(ui->releaseButton->text() == tr("Claim Payment"))
		strMethod = string("escrowclaimrelease");
	else
	{
		strMethod = string("escrowrelease");
		params.push_back(EscrowRoleTypeToString(escrowRoleType).toStdString());
	}
	
	try {
		params.push_back(rawTx.toStdString());
		UniValue result = tableRPC.execute(strMethod, params);
		const UniValue& retarray = result.get_array();
		if(ui->releaseButton->text() == tr("Claim Payment"))
		{
			m_rawTx = QString::fromStdString(retarray[0].get_str());	
			if(this->m_exttxid.size() > 0)
			{
				m_buttontext = tr("Claim Payment");
				ui->releaseButton->setText(tr("Please Wait..."));
				ui->releaseButton->setEnabled(false);
				m_redeemTxId = QString::fromStdString(retarray[1].get_str());
				if(m_paymentOption == QString("BTC"))
					SendRawTxBTC();
				else if(m_paymentOption == QString("ZEC"))
					SendRawTxZEC();
			}
			else
			{
				CompleteEscrowRelease();
			}
		}
		else
		{
			const UniValue& resArray = result.get_array();
			if(resArray.size() > 1)
			{
				const UniValue& complete_value = resArray[1];
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
			QMessageBox::information(this, windowTitle(),
			tr("Escrow released successfully!"),
				QMessageBox::Ok, QMessageBox::Ok);
			ManageEscrowDialog::accept();
		}
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
        tr("Error releasing escrow: ") + QString::fromStdString(strError),
			QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
            tr("General exception releasing escrow"),
			QMessageBox::Ok, QMessageBox::Ok);
	}
}
void ManageEscrowDialog::on_releaseButton_clicked()
{
    if(!walletModel) return;
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if(!ctx.isValid())
    {
		return;
    }
	if(ui->releaseButton->text() == tr("Leave Feedback"))
	{
		onLeaveFeedback();
		return;
	}
	if (releaseWarningStr.size() > 0) {
		QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm Escrow Release"),
             releaseWarningStr,
             QMessageBox::Yes|QMessageBox::Cancel,
             QMessageBox::Cancel);
		if(retval == QMessageBox::Cancel)
			return;
	}
	onRelease();
	
}
void ManageEscrowDialog::doRefund(const QString &rawTx)
{
	UniValue params(UniValue::VARR);
	params.push_back(escrow.toStdString());
	string strMethod;
	if(ui->refundButton->text() == tr("Claim Refund"))
		strMethod = string("escrowclaimrefund");
	else
	{
		strMethod = string("escrowrefund");
		params.push_back(EscrowRoleTypeToString(escrowRoleType).toStdString());
	}
	
	try {
		params.push_back(rawTx.toStdString());
		UniValue result = tableRPC.execute(strMethod, params);
		const UniValue& retarray = result.get_array();
		if(ui->refundButton->text() == tr("Claim Refund"))
		{
			m_rawTx = QString::fromStdString(retarray[0].get_str());	
			if(this->m_exttxid.size() > 0)
			{
				m_buttontext = tr("Claim Refund");
				ui->refundButton->setText(tr("Please Wait..."));
				ui->refundButton->setEnabled(false);
				m_redeemTxId = QString::fromStdString(retarray[1].get_str());
				if(m_paymentOption == QString("BTC"))
					SendRawTxBTC();		
				else if(m_paymentOption == QString("ZEC"))
					SendRawTxZEC();	
			}
			else
			{
				CompleteEscrowRefund();
			}
		}
		else
		{
			const UniValue& resArray = result.get_array();
			if(resArray.size() > 1)
			{
				const UniValue& complete_value = resArray[1];
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
			QMessageBox::information(this, windowTitle(),
			tr("Escrow refunded successfully!"),
				QMessageBox::Ok, QMessageBox::Ok);
			ManageEscrowDialog::accept();
		}
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
        tr("Error refunding escrow: ") + QString::fromStdString(strError),
			QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
            tr("General exception refunding escrow"),
			QMessageBox::Ok, QMessageBox::Ok);
	}
}
void ManageEscrowDialog::on_refundButton_clicked()
{
    if(!walletModel) return;
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if(!ctx.isValid())
    {
		return;
    }
	if (refundWarningStr.size() > 0) {
		QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm Escrow Refund"),
             refundWarningStr,
             QMessageBox::Yes|QMessageBox::Cancel,
             QMessageBox::Cancel);
		if(retval == QMessageBox::Cancel)
			return;
	}
	onRefund();
}
EscrowRoleType ManageEscrowDialog::findYourEscrowRoleFromAliases(const QString &buyer, const QString &seller, const QString &reseller, const QString &arbiter)
{
	if(isYourAlias(buyer))
		return Buyer;
	else if(isYourAlias(seller))
		return Seller;
	else if(isYourAlias(reseller))
		return ReSeller;
	else if(isYourAlias(arbiter))
		return Arbiter;
	else
		return None;
    
 
}
bool ManageEscrowDialog::isYourAlias(const QString &alias)
{
	if(alias.size() <= 0)
		return false;
	string strMethod = string("aliasinfo");
    UniValue params(UniValue::VARR); 
	UniValue result ;
	string name_str;
	params.push_back(alias.toStdString());	
	try {
		result = tableRPC.execute(strMethod, params);

		if (result.type() == UniValue::VOBJ)
		{
			const UniValue& o = result.get_obj();
			const UniValue& mine_value = find_value(o, "ismine");
			if (mine_value.type() == UniValue::VBOOL)
				return mine_value.get_bool();		

		}
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
			tr("Could not get alias information: ") + QString::fromStdString(strError),
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to get alias information: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
	}   
	return false;
}
