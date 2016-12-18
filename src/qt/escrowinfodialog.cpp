#include "escrowinfodialog.h"
#include "ui_escrowinfodialog.h"
#include "init.h"
#include "util.h"
#include "escrow.h"
#include "guiutil.h"
#include "syscoingui.h"
#include "escrowtablemodel.h"
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

EscrowInfoDialog::EscrowInfoDialog(const PlatformStyle *platformStyle, const QModelIndex &idx, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EscrowInfoDialog)
{
    ui->setupUi(this);

    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
	GUID = idx.data(EscrowTableModel::EscrowRole).toString();
	ui->exttxidEdit->setVisible(false);
	ui->exttxidLabel->setVisible(false);

	QString theme = GUIUtil::getThemeName();  
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

EscrowInfoDialog::~EscrowInfoDialog()
{
    delete ui;
}
void EscrowInfoDialog::on_okButton_clicked()
{
    mapper->submit();
    accept();
}
void EscrowInfoDialog::SetFeedbackUI(const UniValue &escrowFeedback, const QString &userType, const QString& buyer, const QString& seller, const QString& arbiter)
{
	if(escrowFeedback.size() <= 0)
	{
		QLabel *noFeedback = new QLabel(tr("No Feedback Found"));
		noFeedback->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		if(userType == tr("Buyer"))
			ui->buyerFeedbackLayout->addWidget(noFeedback);
		else if(userType == tr("Seller"))
			ui->sellerFeedbackLayout->addWidget(noFeedback);
		else if(userType == tr("Arbiter"))
			ui->arbiterFeedbackLayout->addWidget(noFeedback);
		return;
	}
	for(unsigned int i = 0;i<escrowFeedback.size(); i++)
	{
		UniValue feedbackObj = escrowFeedback[i].get_obj();
		int rating =  find_value(feedbackObj, "rating").get_int();
		int user =  find_value(feedbackObj, "feedbackuser").get_int();
		string feedback =  find_value(feedbackObj, "feedback").get_str();
		QString time =  QString::fromStdString(find_value(feedbackObj, "time").get_str());
		QGroupBox *groupBox = new QGroupBox(QString("%1 ").arg(userType) + tr("Feedback #") + QString("%2").arg(QString::number(i+1)));
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
			userStr = QString("%1 ").arg(buyer) + tr("(Buyer)");
		}
		else if(user == FEEDBACKSELLER)
		{
			userStr = QString("%1 ").arg(seller) + tr("(Merchant)"); 
		}
		else if(user == FEEDBACKARBITER)
		{
			userStr = QString("%1 ").arg(arbiter) + tr("(Arbiter)");
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
		else if(userType == tr("Arbiter"))
			ui->arbiterFeedbackLayout->addWidget(groupBox);
	}
}
bool EscrowInfoDialog::lookup()
{
	string strError;
	string strMethod = string("escrowinfo");
	UniValue params(UniValue::VARR);
	UniValue result;
	params.push_back(GUID.toStdString());
	QDateTime dateTime;	
	int unixTime = 0;
    try {
        result = tableRPC.execute(strMethod, params);

		if (result.type() == UniValue::VOBJ)
		{
			QString seller = QString::fromStdString(find_value(result.get_obj(), "seller").get_str());
			const UniValue& reseller_value = find_value(result.get_obj(), "offerlink_seller");
			if (reseller_value.type() == UniValue::VSTR)
				seller = QString::fromStdString(reseller_value.get_str());
			QString arbiter = QString::fromStdString(find_value(result.get_obj(), "arbiter").get_str());
			QString buyer = QString::fromStdString(find_value(result.get_obj(), "buyer").get_str());
			QString currency = QString::fromStdString(find_value(result.get_obj(), "currency").get_str());
			QString paymentOption = QString::fromStdString(find_value(result.get_obj(), "paymentoption_display").get_str());
			QString exttxidStr = QString::fromStdString(find_value(result.get_obj(), "exttxid").get_str());
			if(exttxidStr != "")
			{
				ui->exttxidEdit->setVisible(true);
				ui->exttxidLabel->setVisible(true);
				ui->exttxidEdit->setText(exttxidStr);
			}
			ui->redeemTxidEdit->setText(QString::fromStdString(find_value(result.get_obj(), "redeem_txid").get_str()));
			ui->guidEdit->setText(QString::fromStdString(find_value(result.get_obj(), "escrow").get_str()));
			ui->offerEdit->setText(QString::fromStdString(find_value(result.get_obj(), "offer").get_str()));
			
			ui->txidEdit->setText(QString::fromStdString(find_value(result.get_obj(), "txid").get_str()));
			ui->titleEdit->setText(QString::fromStdString(find_value(result.get_obj(), "offertitle").get_str()));
			ui->heightEdit->setText(QString::fromStdString(find_value(result.get_obj(), "height").get_str()));
			unixTime = atoi(find_value(result.get_obj(), "time").get_str().c_str());
			dateTime.setTime_t(unixTime);	
			ui->timeEdit->setText(dateTime.toString());
			ui->priceEdit->setText(QString("%1 %2").arg(QString::fromStdString(find_value(result.get_obj(), "price").get_str())).arg(currency));
			ui->feeEdit->setText(QString("%1 %2").arg(QString::fromStdString(find_value(result.get_obj(), "fee").get_str())).arg(exttxidStr != ""? paymentOption: tr("SYS")));
			ui->totalEdit->setText(QString("%1 %2").arg(QString::fromStdString(find_value(result.get_obj(), "total").get_str())).arg(exttxidStr != ""? paymentOption: currency));
			ui->paymessageEdit->setText(QString::fromStdString(find_value(result.get_obj(), "pay_message").get_str()));
			ui->ratingEdit->setText(QString::fromStdString(find_value(result.get_obj(), "avg_rating_display").get_str()));
			const UniValue &buyerFeedback = find_value(result.get_obj(), "buyer_feedback").get_array();
			const UniValue &sellerFeedback = find_value(result.get_obj(), "seller_feedback").get_array();
			const UniValue &arbiterFeedback = find_value(result.get_obj(), "arbiter_feedback").get_array();
			SetFeedbackUI(buyerFeedback, tr("Buyer"), buyer, seller, arbiter);
			SetFeedbackUI(sellerFeedback, tr("Seller"), buyer, seller, arbiter);
			SetFeedbackUI(arbiterFeedback, tr("Arbiter"), buyer, seller, arbiter);
			return true;
		}
		 

	}
	catch (UniValue& objError)
	{
		QMessageBox::critical(this, windowTitle(),
				tr("Could not find this escrow, please ensure the escrow has been confirmed by the blockchain"),
				QMessageBox::Ok, QMessageBox::Ok);

	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to locate this escrow, please ensure the escrow has been confirmed by the blockchain: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
	}
	return false;


}

