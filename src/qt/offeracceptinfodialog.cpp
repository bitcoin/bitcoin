#include "offeracceptinfodialog.h"
#include "ui_offeracceptinfodialog.h"
#include "init.h"
#include "util.h"
#include "offer.h"
#include "guiutil.h"
#include "syscoingui.h"
#include "offeraccepttablemodel.h"
#include "platformstyle.h"
#include <QMessageBox>
#include <QModelIndex>
#include <QDateTime>
#include <QDataWidgetMapper>
#include <QLineEdit>
#include <QTextEdit>
#include <QGroupBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "rpc/server.h"
using namespace std;

extern CRPCTable tableRPC;

OfferAcceptInfoDialog::OfferAcceptInfoDialog(const PlatformStyle *platformStyle, const QModelIndex &idx, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OfferAcceptInfoDialog)
{
    ui->setupUi(this);

    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
	offerGUID = idx.data(OfferAcceptTableModel::NameRole).toString();
	offerAcceptGUID = idx.data(OfferAcceptTableModel::GUIDRole).toString();
	ui->linkGUIDEdit->setVisible(false);
	ui->linkGUIDLabel->setVisible(false);
	ui->commissionEdit->setVisible(false);
	ui->commissionLabel->setVisible(false);
	ui->exttxidEdit->setVisible(false);
	ui->exttxidLabel->setVisible(false);
	ui->certEdit->setVisible(false);
	ui->certLabel->setVisible(false);
	QString theme = GUIUtil::getThemeName();  
	ui->offerguidEdit->setText(offerGUID);
	ui->paymentguidEdit->setText(offerAcceptGUID);
	if (!platformStyle->getImagesOnButtons())
	{
		ui->okButton->setIcon(QIcon());

	}
	else
	{
		ui->okButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/synced"));
	}
	lookup();
}

OfferAcceptInfoDialog::~OfferAcceptInfoDialog()
{
    delete ui;
}
void OfferAcceptInfoDialog::on_okButton_clicked()
{
    mapper->submit();
    accept();
}
void OfferAcceptInfoDialog::SetFeedbackUI(const UniValue &feedbackObj, const QString &userType, const QString& buyer, const QString& seller)
{
	if(feedbackObj.size() <= 0)
	{
		QLabel *noFeedback = new QLabel(tr("No Feedback Found"));
		noFeedback->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		if(userType == tr("Buyer"))
			ui->buyerFeedbackLayout->addWidget(noFeedback);
		else if(userType == tr("Seller"))
			ui->sellerFeedbackLayout->addWidget(noFeedback);
		return;
	}
	for(unsigned int i = 0;i<feedbackObj.size(); i++)
	{
		UniValue feedbackValue = feedbackObj[i].get_obj();
		int rating =  find_value(feedbackValue, "rating").get_int();
		int user =  find_value(feedbackValue, "feedbackuser").get_int();
		string feedback =  find_value(feedbackValue, "feedback").get_str();
		QString time =  QString::fromStdString(find_value(feedbackValue, "time").get_str());
		QGroupBox *groupBox = new QGroupBox(QString("%1 ").arg(userType) + tr("Feedback #") + QString("%1").arg(QString::number(i+1)));
		QTextEdit *feedbackText;
		if(feedback.size() > 0)
			feedbackText = new QTextEdit(QString::fromStdString(feedback));
		else
			feedbackText = new QTextEdit(tr("No Feedback"));

		QVBoxLayout *vbox = new QVBoxLayout;
		QHBoxLayout *timeBox = new QHBoxLayout;
		QLabel *timeLabel = new QLabel(tr("Time:"));
		QDateTime dateTime;	
		int unixTime = time.toInt();
		dateTime.setTime_t(unixTime);
		time = dateTime.toString();	
		QLabel *timeText = new QLabel(QString("<b>%1</b>").arg(time));
		timeBox->addWidget(timeLabel);
		timeBox->addWidget(timeText);
		timeBox->addStretch(1);
		vbox->addLayout(timeBox);

		QHBoxLayout *userBox = new QHBoxLayout;
		QLabel *userLabel = new QLabel(tr("From:"));

		QString userStr = "";
		if(user == FEEDBACKBUYER)
		{
			userStr = QString("%1").arg(buyer) + tr("(Buyer)");
		}
		else if(user == FEEDBACKSELLER)
		{
			userStr = QString("%1").arg(seller) + tr("(Merchant)");
		}
		QLabel *userText = new QLabel(QString("<b>%1</b>").arg(userStr));
		userBox->addWidget(userLabel);
		userBox->addWidget(userText);
		userBox->addStretch(1);
		vbox->addLayout(userBox);
	
		QHBoxLayout *ratingBox = new QHBoxLayout;
		QLabel *ratingLabel = new QLabel(tr("Rating:"));
		QLabel *ratingText;
		if(rating > 0)
			ratingText = new QLabel( QString("<b>%1 %2</b>").arg(QString::number(rating)).arg(tr("Stars")));
		else
			ratingText = new QLabel( QString("<b>%1</b>").arg(tr("No Rating")));

		ratingBox->addWidget(ratingLabel);
		ratingBox->addWidget(ratingText);
		ratingBox->addStretch(1);
		vbox->addLayout(ratingBox);
		
		
		vbox->addWidget(feedbackText);

		groupBox->setLayout(vbox);
		if(userType == tr("Buyer"))
			ui->buyerFeedbackLayout->addWidget(groupBox);
		else if(userType == tr("Seller"))
			ui->sellerFeedbackLayout->addWidget(groupBox);
	}
}
bool OfferAcceptInfoDialog::lookup()
{
	string strError;
	string strMethod = string("offerinfo");
	UniValue params(UniValue::VARR);
	UniValue result(UniValue::VOBJ);
	params.push_back(offerGUID.toStdString());
	QString sellerStr;
    try {
        result = tableRPC.execute(strMethod, params);

		if (result.type() == UniValue::VOBJ)
		{
			
			sellerStr = QString::fromStdString(find_value(result.get_obj(), "alias").get_str());
			ui->sellerEdit->setText(sellerStr);
			QString linkedStr = QString::fromStdString(find_value(result.get_obj(), "offerlink").get_str());
			if(linkedStr == QString("true"))
			{

				ui->linkGUIDEdit->setVisible(true);
				ui->linkGUIDLabel->setVisible(true);
				ui->commissionEdit->setVisible(true);
				ui->commissionLabel->setVisible(true);
				ui->linkGUIDEdit->setText(QString::fromStdString(find_value(result.get_obj(), "offerlink_guid").get_str()));
				ui->commissionEdit->setText(QString::fromStdString(find_value(result.get_obj(), "commission").get_str()) + "%");
			}
			ui->titleEdit->setText(QString::fromStdString(find_value(result.get_obj(), "title").get_str()));
			QString certStr = QString::fromStdString(find_value(result.get_obj(), "cert").get_str());
			if(certStr != "")
			{
				ui->certEdit->setVisible(true);
				ui->certLabel->setVisible(true);
				ui->certEdit->setText(certStr);
			}	
		}
		 

	}
	catch (UniValue& objError)
	{
		QMessageBox::critical(this, windowTitle(),
				tr("Could not find this offer, please ensure offer has been confirmed by the blockchain"),
				QMessageBox::Ok, QMessageBox::Ok);

	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to locate this offer, please ensure offer has been confirmed by the blockchain: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
	}
	strMethod = string("offeracceptlist");
	UniValue params1(UniValue::VARR);
	params1.push_back(sellerStr.toStdString());
	params1.push_back(offerAcceptGUID.toStdString());
	UniValue offerAcceptsValue;

    try {
        offerAcceptsValue = tableRPC.execute(strMethod, params1);

		if (offerAcceptsValue.type() == UniValue::VARR)
		{
			const UniValue &offerAccepts = offerAcceptsValue.get_array();
			COfferAccept myAccept;
			QString currencyStr;
			QDateTime timestamp;
		    for (unsigned int idx = 0; idx < offerAccepts.size(); idx++) {
			    const UniValue& accept = offerAccepts[idx];				
				const UniValue& acceptObj = accept.get_obj();
			
				ui->txidEdit->setText(QString::fromStdString(find_value(acceptObj, "txid").get_str()));
				
				ui->heightEdit->setText(QString::fromStdString(find_value(acceptObj, "height").get_str()));
				int unixTime = atoi(find_value(acceptObj, "time").get_str().c_str());
				timestamp.setTime_t(unixTime);
				ui->timeEdit->setText(timestamp.toString());

				ui->quantityEdit->setText(QString::fromStdString(find_value(acceptObj, "quantity").get_str()));
				currencyStr = QString::fromStdString(find_value(acceptObj, "currency").get_str());
		
				QString exttxidStr = QString::fromStdString(find_value(acceptObj, "exttxid").get_str());
				if(exttxidStr != "")
				{
					ui->exttxidEdit->setVisible(true);
					ui->exttxidLabel->setVisible(true);
					ui->exttxidEdit->setText(exttxidStr);
				}
				QString paymentOption = QString::fromStdString(find_value(acceptObj, "paymentoption_display").get_str());	
				ui->priceEdit->setText(QString("%1 %2").arg(QString::fromStdString(find_value(acceptObj, "price").get_str())).arg(currencyStr));
				ui->totalEdit->setText(QString("%1 %2").arg(QString::fromStdString(find_value(acceptObj, "total").get_str())).arg(exttxidStr != ""? paymentOption: currencyStr));
				ui->discountEdit->setText(QString::fromStdString(find_value(acceptObj, "offer_discount_percentage").get_str()));
				ui->paidEdit->setText(QString::fromStdString(find_value(acceptObj, "status").get_str()));
				QString buyerStr = QString::fromStdString(find_value(acceptObj.get_obj(), "buyer").get_str());
				ui->buyerEdit->setText(buyerStr);
				ui->paymessageEdit->setText(QString::fromStdString(find_value(acceptObj, "pay_message").get_str()));
				ui->ratingEdit->setText(QString::fromStdString(find_value(acceptObj, "avg_rating_display").get_str()));
				const UniValue &buyerFeedback = find_value(acceptObj.get_obj(), "buyer_feedback").get_array();
				const UniValue &sellerFeedback = find_value(acceptObj.get_obj(), "seller_feedback").get_array();
				SetFeedbackUI(buyerFeedback, tr("Buyer"), buyerStr, sellerStr);
				SetFeedbackUI(sellerFeedback, tr("Seller"), buyerStr, sellerStr);
				break;
			}
			return true;
		}
		 

	}
	catch (UniValue& objError)
	{
		QMessageBox::critical(this, windowTitle(),
				tr("Could not find this offer purchase, please ensure it has been confirmed by the blockchain"),
				QMessageBox::Ok, QMessageBox::Ok);

	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to locate this offer purchase, please ensure it has been confirmed by the blockchain: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
	}

	return false;


}

