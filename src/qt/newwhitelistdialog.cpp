#include "newwhitelistdialog.h"
#include "ui_newwhitelistdialog.h"
#include "offertablemodel.h"
#include "offerwhitelisttablemodel.h"
#include "guiutil.h"
#include "walletmodel.h"
#include "syscoingui.h"
#include "ui_interface.h"
#include <QDataWidgetMapper>
#include <QMessageBox>
#include <QStringList>
#include "rpcserver.h"
using namespace std;


extern const CRPCTable tableRPC;
NewWhitelistDialog::NewWhitelistDialog(const QString &offerGUID, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewWhitelistDialog), model(0)
{
    ui->setupUi(this);
	ui->offerGUIDLabel->setText(offerGUID);
	ui->discountDisclaimer->setText(tr("<font color='blue'>This is a percentage of price for your offer you want to allow your reseller to purchase your offer for. Typically given to wholesalers or for special arrangements with a reseller.</font>"));
	ui->certEdit->clear();
	ui->certEdit->addItem(tr("Select Certificate"));
	loadCerts();
}

NewWhitelistDialog::~NewWhitelistDialog()
{
    delete ui;
}


void NewWhitelistDialog::setModel(WalletModel *walletModel, OfferWhitelistTableModel *model)
{
    this->model = model;
	this->walletModel = walletModel;
}
void NewWhitelistDialog::loadCerts()
{
	string strMethod = string("certlist");
    UniValue params(UniValue::VARR); 
	UniValue result ;
	string name_str;
	string title_str;
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
				const UniValue&o = input.get_obj();
				name_str = "";
				title_str = "";
				expired = 0;


		
				const UniValue& name_value = find_value(o, "cert");
				if (name_value.type() == UniValue::VSTR)
					name_str = name_value.get_str();
				const UniValue& title_value = find_value(o, "title");
				if (title_value.type() == UniValue::VSTR)
					title_str = title_value.get_str();					
				const UniValue& expired_value = find_value(o, "expired");
				if (expired_value.type() == UniValue::VNUM)
					expired = expired_value.get_int();
				
				if(expired == 0)
				{
					QString name = QString::fromStdString(name_str);
					QString title = QString::fromStdString(title_str);
					QString certText = name + " - " + title;
					ui->certEdit->addItem(certText,name);
				}
				
			}
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
 
}
bool NewWhitelistDialog::saveCurrentRow()
{

    if(!model || !walletModel) return false;
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if(!ctx.isValid())
    {
		model->editStatus = OfferWhitelistTableModel::WALLET_UNLOCK_FAILURE;
        return false;
    }
	UniValue params(UniValue::VARR);
	string strMethod;

	strMethod = string("offeraddwhitelist");
	params.push_back(ui->offerGUIDLabel->text().toStdString());
	if(ui->certEdit->currentIndex() > 0)
		params.push_back(ui->certEdit->itemData(ui->certEdit->currentIndex()).toString().toStdString());
	params.push_back(ui->discountEdit->text().toStdString());

	try {
        UniValue result = tableRPC.execute(strMethod, params);
		if(ui->certEdit->currentIndex() > 0)
			entry = ui->certEdit->itemData(ui->certEdit->currentIndex()).toString();

		QMessageBox::information(this, windowTitle(),
        tr("New whitelist entry added successfully!"),
			QMessageBox::Ok, QMessageBox::Ok);
			
		
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
		tr("Error creating new whitelist entry: \"%1\"").arg(QString::fromStdString(strError)),
			QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("General exception creating new whitelist entry: \"%1\"").arg(QString::fromStdString(e.what())),
			QMessageBox::Ok, QMessageBox::Ok);
	}							

    return !entry.isEmpty();
}

void NewWhitelistDialog::accept()
{
    if(!model) return;

    if(!saveCurrentRow())
    {
        switch(model->getEditStatus())
        {
        case OfferWhitelistTableModel::OK:
            // Failed with unknown reason. Just reject.
            break;
        case OfferWhitelistTableModel::NO_CHANGES:
            // No changes were made during edit operation. Just reject.
            break;
        case OfferWhitelistTableModel::INVALID_ENTRY:
            QMessageBox::warning(this, windowTitle(),
                tr("The entered entry \"%1\" is not a valid whitelist entry.").arg(ui->certEdit->currentText()),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case OfferWhitelistTableModel::DUPLICATE_ENTRY:
            QMessageBox::warning(this, windowTitle(),
                tr("The entered entry \"%1\" is already taken.").arg(ui->certEdit->currentText()),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case OfferWhitelistTableModel::WALLET_UNLOCK_FAILURE:
            QMessageBox::critical(this, windowTitle(),
                tr("Could not unlock wallet."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;

        }
        return;
    }
    QDialog::accept();
}

QString NewWhitelistDialog::getEntry() const
{
    return entry;
}

