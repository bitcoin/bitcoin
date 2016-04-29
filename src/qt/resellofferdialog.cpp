#include "resellofferdialog.h"
#include "ui_resellofferdialog.h"
#include "offertablemodel.h"
#include "guiutil.h"
#include "syscoingui.h"
#include "ui_interface.h"
#include <QDataWidgetMapper>
#include <QMessageBox>
#include <QStringList>
#include "rpcserver.h"
using namespace std;
extern const CRPCTable tableRPC;
ResellOfferDialog::ResellOfferDialog(QModelIndex *idx, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ResellOfferDialog)
{
    ui->setupUi(this);

	QString offerGUID = idx->data(OfferTableModel::NameRole).toString();
	ui->descriptionEdit->setPlainText(idx->data(OfferTableModel::DescriptionRole).toString());
	ui->offerGUIDLabel->setText(offerGUID);
	ui->commissionDisclaimer->setText(tr("<font color='blue'>Enter the <b>percentage</b>  amount(without the % sign) that you would like to mark-up the price to.</font>"));
	loadAliases();
}

ResellOfferDialog::~ResellOfferDialog()
{
    delete ui;
}

bool ResellOfferDialog::saveCurrentRow()
{

	UniValue params(UniValue::VARR);
	string strMethod;

	strMethod = string("offerlink");
	params.push_back(ui->aliasEdit->currentText().toStdString());
	params.push_back(ui->offerGUIDLabel->text().toStdString());
	params.push_back(ui->commissionEdit->text().toStdString());
	params.push_back(ui->descriptionEdit->toPlainText().toStdString());

	try {
        UniValue result = tableRPC.execute(strMethod, params);

		QMessageBox::information(this, windowTitle(),
        tr("Offer resold successfully! Check the <b>Selling</b> tab to see it after it has confirmed."),
			QMessageBox::Ok, QMessageBox::Ok);
		return true;	
		
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
		tr("Error creating new linked offer: \"%1\"").arg(QString::fromStdString(strError)),
			QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("General exception creating new linked offer: \"%1\"").arg(QString::fromStdString(e.what())),
			QMessageBox::Ok, QMessageBox::Ok);
	}							

    return false;
}
void ResellOfferDialog::loadAliases()
{
	ui->aliasEdit->clear();
	string strMethod = string("aliaslist");
    UniValue params(UniValue::VARR); 
	UniValue result ;
	string name_str;
	int expired = 0;
	
	try {
		result = tableRPC.execute(strMethod, params);

		if (result.type() == UniValue::VARR)
		{
			name_str = "";
			expired = 0;


	
			const UniValue &arr = result.get_array();
		    for (unsigned int idx = 0; idx < arr.size(); idx++) {
			    const UniValue& input = arr[idx];
				if (input.type() != UniValue::VOBJ)
					continue;
				const UniValue& o = input.get_obj();
				name_str = "";

				expired = 0;


		
				const UniValue& name_value = find_value(o, "name");
				if (name_value.type() == UniValue::VSTR)
					name_str = name_value.get_str();		
				const UniValue& expired_value = find_value(o, "expired");
				if (expired_value.type() == UniValue::VNUM)
					expired = expired_value.get_int();
				
				if(expired == 0)
				{
					QString name = QString::fromStdString(name_str);
					ui->aliasEdit->addItem(name);					
				}
				
			}
		}
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
			tr("Could not refresh alias list: %1").arg(QString::fromStdString(strError)),
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to refresh the alias list: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
	}         
 
}
void ResellOfferDialog::accept()
{
    

    if(!saveCurrentRow())
    {
        return;
    }
    QDialog::accept();
}


