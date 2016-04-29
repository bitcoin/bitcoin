#include "manageescrowdialog.h"
#include "ui_manageescrowdialog.h"

#include "guiutil.h"
#include "syscoingui.h"
#include "ui_interface.h"
#include <QMessageBox>
#include "rpcserver.h"
using namespace std;

extern const CRPCTable tableRPC;
ManageEscrowDialog::ManageEscrowDialog(const QString &escrow, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ManageEscrowDialog), escrow(escrow)
{
    ui->setupUi(this);
	QString buyer, seller, arbiter, status, offertitle, total;
	if(!loadEscrow(escrow, buyer, seller, arbiter, status, offertitle, total))
	{
		ui->manageInfo2->setText(tr("Cannot find this escrow on the network, please try again later."));
		ui->releaseButton->setEnabled(false);
		ui->refundButton->setEnabled(false);
		return;
	}
	EscrowType escrowType = findYourEscrowRoleFromAliases(buyer, seller, arbiter);
	ui->manageInfo->setText(tr("You are managing escrow ID: <b>%1</b> of an offer for <b>%2</b> totalling <b>%3</b>. The buyer is <b>%4</b>, seller is <b>%5</b> and arbiter is <b>%6</b>").arg(escrow).arg(offertitle).arg(total).arg(buyer).arg(seller).arg(arbiter));
	if(escrowType == None)
	{
		ui->manageInfo2->setText(tr("You cannot manage this escrow because you do not own one of either the buyer, seller or arbiter aliases."));
		ui->releaseButton->setEnabled(false);
		ui->refundButton->setEnabled(false);
	}
	else if(status == "in-escrow")
	{
		if(escrowType == Buyer)
		{
			ui->manageInfo2->setText(tr("You are the <b>buyer</b> of the offer held in escrow, you may release the coins to the seller once you have confirmed that you have recieved the item as per the description of the offer."));
			ui->refundButton->setEnabled(false);
		}
		else if(escrowType == Seller)
		{
			ui->manageInfo2->setText(tr("You are the <b>seller</b> of the offer held in escrow, you may refund the coins back to the buyer."));
			ui->releaseButton->setEnabled(false);
		}
		else if(escrowType == Arbiter)
		{
			ui->manageInfo2->setText(tr("You are the <b>arbiter</b> of the offer held in escrow, you may refund the coins back to the buyer if you have evidence that the seller did not honour the agreement to ship the offer item. You may also release the coins to the seller if the buyer has not released. You may use Syscoin messages to communicate with the buyer and seller to ensure you have adequeate proof for your decision."));
		}

	}
	else if(status == "escrow released")
	{
		if(escrowType == Buyer)
		{
			ui->manageInfo2->setText(tr("You are the <b>buyer</b> of the offer held in escrow. The escrow has been released to the seller. You may communicate with your arbiter or seller via Syscoin messages."));
			ui->refundButton->setEnabled(false);
			ui->releaseButton->setEnabled(false);
		}
		else if(escrowType == Seller)
		{
			ui->manageInfo2->setText(tr("You are the <b>seller</b> of the offer held in escrow. The payment of coins have been released to you, you may claim them now."));
			ui->refundButton->setEnabled(false);
			ui->releaseButton->setText(tr("Claim Payment"));
		}
		else if(escrowType == Arbiter)
		{
			ui->manageInfo2->setText(tr("You are the <b>arbiter</b> of the offer held in escrow. The escrow has been released to the seller. You're job is done, if you were the one to release the coins you will recieve a commission as soon as the seller claims his payment."));
			ui->refundButton->setEnabled(false);
			ui->releaseButton->setEnabled(false);
		}
	}
	else if(status == "escrow refunded")
	{
		if(escrowType == Buyer)
		{
			ui->manageInfo2->setText(tr("You are the <b>buyer</b> of the offer held in escrow. The coins have been refunded back to you, you may claim them now."));
			ui->refundButton->setText(tr("Claim Refund"));
			ui->releaseButton->setEnabled(false);
		}
		else if(escrowType == Seller)
		{
			ui->manageInfo2->setText(tr("You are the <b>seller</b> of the offer held in escrow. The escrow has been refunded back to the buyer."));
			ui->refundButton->setEnabled(false);
			ui->releaseButton->setEnabled(false);
		}
		else if(escrowType == Arbiter)
		{
			ui->manageInfo2->setText(tr("You are the <b>arbiter</b> of the offer held in escrow. The escrow has been refunded back to the buyer. You're job is done, if you were the one to refund the coins you will recieve a commission as soon as the buyer claims his refund."));
			ui->refundButton->setEnabled(false);
			ui->releaseButton->setEnabled(false);
		}
	}
	else if(status == "complete")
	{		
		ui->manageInfo2->setText(tr("The escrow has been successfully claimed by the seller. The escrow is complete."));
		ui->refundButton->setEnabled(false);
		ui->releaseButton->setEnabled(false);
	}
	else if(status == "pending")
	{		
		ui->manageInfo2->setText(tr("The escrow is still pending a confirmation by the network. Please try again later."));
		ui->refundButton->setEnabled(false);
		ui->releaseButton->setEnabled(false);
	}
	else
	{
		ui->manageInfo2->setText(tr("The escrow status was not recognized. Please contact the Syscoin team."));
		ui->refundButton->setEnabled(false);
		ui->releaseButton->setEnabled(false);
	}
}
bool ManageEscrowDialog::loadEscrow(const QString &escrow, QString &buyer, QString &seller, QString &arbiter, QString &status, QString &offertitle, QString &total)
{
	string strMethod = string("escrowlist");
    UniValue params(UniValue::VARR); 
	UniValue result ;
	string name_str;
	try {
		result = tableRPC.execute(strMethod, params);
		if (result.type() == UniValue::VARR)
		{
			const UniValue &arr = result.get_array();
		    for (unsigned int idx = 0; idx < arr.size(); idx++) {
				name_str = "";
			    const UniValue& input = arr[idx];
				if (input.type() != UniValue::VOBJ)
					continue;
				const UniValue& o = input.get_obj();
		
				const UniValue& name_value = find_value(o, "escrow");
				if (name_value.type() == UniValue::VSTR)
					name_str = name_value.get_str();
				if(QString::fromStdString(name_str) != escrow)
					continue;
				const UniValue& seller_value = find_value(o, "seller");
				if (seller_value.type() == UniValue::VSTR)
					seller = QString::fromStdString(seller_value.get_str());
				const UniValue& arbiter_value = find_value(o, "arbiter");
				if (arbiter_value.type() == UniValue::VSTR)
					arbiter = QString::fromStdString(arbiter_value.get_str());
				const UniValue& buyer_value = find_value(o, "buyer");
				if (buyer_value.type() == UniValue::VSTR)
					buyer = QString::fromStdString(buyer_value.get_str());
				const UniValue& offertitle_value = find_value(o, "offertitle");
				if (offertitle_value.type() == UniValue::VSTR)
					offertitle = QString::fromStdString(offertitle_value.get_str());
				const UniValue& total_value = find_value(o, "total");
				if (total_value.type() == UniValue::VSTR)
					total = QString::fromStdString(total_value.get_str());
				const UniValue& status_value = find_value(o, "status");
				if (status_value.type() == UniValue::VSTR)
					status = QString::fromStdString(status_value.get_str());
				return true;
				
			}
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
void ManageEscrowDialog::on_cancelButton_clicked()
{
    reject();
}
ManageEscrowDialog::~ManageEscrowDialog()
{
    delete ui;
}
void ManageEscrowDialog::on_releaseButton_clicked()
{
	UniValue params(UniValue::VARR);
	string strMethod = string("escrowrelease");
	params.push_back(escrow.toStdString());
	try {
		UniValue result = tableRPC.execute(strMethod, params);
		QMessageBox::information(this, windowTitle(),
		tr("Escrow released successfully!"),
			QMessageBox::Ok, QMessageBox::Ok);
		ManageEscrowDialog::accept();
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
        tr("Error releasing escrow: \"%1\"").arg(QString::fromStdString(strError)),
			QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
            tr("General exception releasing escrow"),
			QMessageBox::Ok, QMessageBox::Ok);
	}	
}
void ManageEscrowDialog::on_refundButton_clicked()
{
	UniValue params(UniValue::VARR);
	string strMethod = string("escrowrefund");
	params.push_back(escrow.toStdString());
	try {
		UniValue result = tableRPC.execute(strMethod, params);
		QMessageBox::information(this, windowTitle(),
		tr("Escrow refunded successfully!"),
			QMessageBox::Ok, QMessageBox::Ok);
		ManageEscrowDialog::accept();
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
        tr("Error refunding escrow: \"%1\"").arg(QString::fromStdString(strError)),
			QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
            tr("General exception refunding escrow"),
			QMessageBox::Ok, QMessageBox::Ok);
	}
}
ManageEscrowDialog::EscrowType ManageEscrowDialog::findYourEscrowRoleFromAliases(const QString &buyer, const QString &seller, const QString &arbiter)
{
	if(isYourAlias(buyer))
		return Buyer;
	else if(isYourAlias(seller))
		return Seller;
	else if(isYourAlias(arbiter))
		return Arbiter;
	else
		return None;
    
 
}
bool ManageEscrowDialog::isYourAlias(const QString &alias)
{
	string strMethod = string("aliasinfo");
    UniValue params(UniValue::VARR); 
	UniValue result ;
	string name_str;
	int expired = 0;
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
			tr("Could not refresh cert list: %1").arg(QString::fromStdString(strError)),
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to refresh the cert list: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
	}   
	return false;
}
