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
#include "rpcserver.h"
using namespace std;

extern const CRPCTable tableRPC;

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
	ui->btctxidEdit->setVisible(false);
	ui->btctxidLabel->setVisible(false);
	ui->certEdit->setVisible(false);
	ui->certLabel->setVisible(false);
	ui->escrowEdit->setVisible(false);
	ui->escrowLabel->setVisible(false);
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

OfferAcceptInfoDialog::~OfferAcceptInfoDialog()
{
    delete ui;
}
void OfferAcceptInfoDialog::on_okButton_clicked()
{
    mapper->submit();
    accept();
}
bool OfferAcceptInfoDialog::lookup()
{
	string strError;
	string strMethod = string("offerinfo");
	UniValue params(UniValue::VARR);
	UniValue result;
	params.push_back(offerGUID.toStdString());

    try {
        result = tableRPC.execute(strMethod, params);

		if (result.type() == UniValue::VOBJ)
		{
			UniValue offerAcceptsValue = find_value(result.get_obj(), "accepts");
			if(offerAcceptsValue.type() != UniValue::VARR)
				return false;
			QString linkedStr = QString::fromStdString(find_value(result.get_obj(), "offerlink").get_str());
			if(linkedStr == QString("true"))
			{

				ui->linkGUIDEdit->setVisible(true);
				ui->linkGUIDLabel->setVisible(true);
				ui->commissionEdit->setVisible(true);
				ui->commissionLabel->setVisible(true);
				ui->linkGUIDEdit->setText(QString::fromStdString(find_value(result.get_obj(), "offerlink_guid").get_str()));
				ui->commissionEdit->setText(QString::fromStdString(find_value(result.get_obj(), "commission").get_str()));
			}
			const UniValue &offerAccepts = offerAcceptsValue.get_array();
			COfferAccept myAccept;
			QDateTime timestamp;
		    for (unsigned int idx = 0; idx < offerAccepts.size(); idx++) {
			    const UniValue& accept = offerAccepts[idx];				
				const UniValue& acceptObj = accept.get_obj();
				QString offerAcceptHash = QString::fromStdString(find_value(acceptObj, "id").get_str());
				if(offerAcceptHash != offerAcceptGUID)
					continue;
				ui->guidEdit->setText(offerAcceptHash);
				ui->txidEdit->setText(QString::fromStdString(find_value(acceptObj, "txid").get_str()));
				
				ui->heightEdit->setText(QString::fromStdString(find_value(acceptObj, "height").get_str()));
				int unixTime = atoi(find_value(acceptObj, "time").get_str().c_str());
				timestamp.setTime_t(unixTime);
				ui->timeEdit->setText(timestamp.toString());

				ui->quantityEdit->setText(QString::fromStdString(find_value(acceptObj, "quantity").get_str()));
				QString currencyStr = QString::fromStdString(find_value(acceptObj, "currency").get_str());
				if(currencyStr == "BTC")
				{
					QString btctxidStr = QString::fromStdString(find_value(acceptObj, "btctxid").get_str());
					if(btctxidStr != "")
					{
						ui->btctxidEdit->setVisible(true);
						ui->btctxidLabel->setVisible(true);
						ui->btctxidEdit->setText(btctxidStr);
					}
				}
				ui->currencyEdit->setText(currencyStr);
				
				ui->totalEdit->setText(QString::fromStdString(find_value(acceptObj, "total").get_str()));
				ui->discountEdit->setText(QString::fromStdString(find_value(acceptObj, "offer_discount_percentage").get_str()));
				ui->paidEdit->setText(QString::fromStdString(find_value(acceptObj, "paid").get_str()));
				
				ui->paymessageEdit->setText(QString::fromStdString(find_value(acceptObj, "pay_message").get_str()));
				QString escrowStr = QString::fromStdString(find_value(acceptObj.get_obj(), "escrowlink").get_str());
				if(escrowStr != "")
				{
					ui->escrowEdit->setVisible(true);
					ui->escrowLabel->setVisible(true);
					ui->escrowEdit->setText(escrowStr);
				}
			}

			ui->titleEdit->setText(QString::fromStdString(find_value(result.get_obj(), "title").get_str()));
			ui->priceEdit->setText(QString::fromStdString(find_value(result.get_obj(), "price").get_str()));
			QString certStr = QString::fromStdString(find_value(result.get_obj(), "cert").get_str());
			if(certStr != "")
			{
				ui->certEdit->setVisible(true);
				ui->certLabel->setVisible(true);
				ui->certEdit->setText(certStr);
			}	
			return true;
		}
		 

	}
	catch (UniValue& objError)
	{
		QMessageBox::critical(this, windowTitle(),
				tr("Could not find this offer or offer accept, please check the offer ID and that it has been confirmed by the blockchain"),
				QMessageBox::Ok, QMessageBox::Ok);

	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to locate this offer or offeraccept, please check the offer ID and that it has been confirmed by the blockchain: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
	}
	return false;


}

