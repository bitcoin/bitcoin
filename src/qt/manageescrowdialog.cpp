#include "manageescrowdialog.h"
#include "ui_manageescrowdialog.h"

#include "guiutil.h"
#include "syscoingui.h"
#include "ui_interface.h"
#include <QDataWidgetMapper>
#include <QMessageBox>
#include "rpcserver.h"
using namespace std;

extern const CRPCTable tableRPC;
ManageEscrowDialog::ManageEscrowDialog(const QString &escrow, const QString &buyer, const QString &seller, const QString &arbiter, const QString &status, const QString &offertitle, const QString &total,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ManageEscrowDialog), mapper(0), escrow(escrow), buyer(buyer), seller(seller), arbiter(arbiter), status(status),offertitle(offertitle), total(total), model(0), walletModel(0)
{
    ui->setupUi(this);
	EscrowType escrowType = findYourEscrowRoleFromAliases(buyer, seller, arbiter);
	if(escrowType == EscrowType::None)
	{
		ui->manageInfo2->setText(tr("You cannot manage this escrow because you are do not own one of either the buyer, seller or arbiter aliases."));
		return;
	}
	ui->manageInfo->setText(tr("You are managing escrow ID: <b>%1</b> of an offer for <b>%2</b> totalling <b>%3</b>SYS. The buyer is <b>%4</b>, seller is <b>%5</b> and arbiter is <b>%6</b>").arg(QString::fromStdString(escrow).arg(QString::fromStdString(offertitle).arg(QString::fromStdString(total).arg(QString::fromStdString(buyer).arg(QString::fromStdString(seller).arg(QString::fromStdString(arbiter));
	if(escrowType == EscrowType::None)
	{
		ui->manageInfo2->setText(tr("You cannot manage this escrow because you are do not own one of either the buyer, seller or arbiter aliases."));
		ui->releaseButton->setEnabled(false);
		ui->refundButton->setEnabled(false);
	}
	else if(status == "in-escrow")
	{
		if(escrowType == EscrowType::Buyer)
		{
			ui->manageInfo2->setText(tr("You are the <b>buyer</b> of the offer held in escrow, you may release the coins to the seller once you have confirmed that you have recieved the item as per the description of the offer."));
			ui->refundButton->setEnabled(false);
		}
		else if(escrowType == EscrowType::Seller)
		{
			ui->manageInfo2->setText(tr("You are the <b>seller</b> of the offer held in escrow, you may refund the coins back to the buyer."));
			ui->releaseButton->setEnabled(false);
		}
		else if(escrowType == EscrowType::Arbiter)
		{
			ui->manageInfo2->setText(tr("You are the <b>arbiter</b> of the offer held in escrow, you may refund the coins back to the buyer if you have evidence that the seller did not honour the agreement to ship the offer item. You may also release the coins to the seller if the buyer has not released. You may use Syscoin messages to communicate with the buyer and seller to ensure you have adequeate proof for your decision."));
		}

	}
	else if(status == "escrow released")
	{
		if(escrowType == EscrowType::Buyer)
		{
			ui->manageInfo2->setText(tr("The escrow has already been released to the seller. You may communicate with your arbiter or seller via Syscoin messages."));
			ui->refundButton->setEnabled(false);
			ui->releaseButton->setEnabled(false);
		}
		else if(escrowType == EscrowType::Seller)
		{
			ui->manageInfo2->setText(tr("You are the <b>seller</b> of the offer held in escrow. The payment of coins have been released to you, you may claim them now."));
			ui->refundButton->setEnabled(false);
			ui->releaseButton->setText(tr("Claim Payment"));
		}
		else if(escrowType == EscrowType::Arbiter)
		{
			ui->manageInfo2->setText(tr("The escrow has already been released to the seller. You're job is done, if you were the one to release the coins you will recieve a commission as soon as the seller claims his payment."));
			ui->refundButton->setEnabled(false);
			ui->releaseButton->setEnabled(false);
		}
	}
	else if(status == "escrow refunded")
	{
		if(escrowType == EscrowType::Buyer)
		{
			ui->manageInfo2->setText(tr("You are the <b>buyer</b> of the offer held in escrow. The coins have been refunded back to you, you may claim them now."));
			ui->refundButton->setText(tr("Claim Refund"));
			ui->releaseButton->setEnabled(false);
		}
		else if(escrowType == EscrowType::Seller)
		{
			ui->manageInfo2->setText(tr("The escrow has already been refunded back to the buyer."));
			ui->refundButton->setEnabled(false);
			ui->releaseButton->setEnabled(false);
		}
		else if(escrowType == EscrowType::Arbiter)
		{
			ui->manageInfo2->setText(tr("The escrow has already been refunded back to the buyer. You're job is done, if you were the one to refund the coins you will recieve a commission as soon as the buyer claims his refund."));
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
	else
	{
		ui->manageInfo2->setText(tr("The escrow status was not recognized. Please contact the Syscoin team."));
		ui->refundButton->setEnabled(false);
		ui->releaseButton->setEnabled(false);
	}
    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
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
EscrowType ManageEscrowDialog::findYourEscrowRoleFromAliases(const QString &buyer, const QString &seller, const QString &arbiter)
{
	if(isYourAlias(buyer))
		return EscrowType::Buyer;
	else if(isYourAlias(seller))
		return EscrowType::Seller;
	else if(isYourArbiter(arbiter))
		return EscrowType::Arbiter;
	else
		return EscrowType::None;
    
 
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
