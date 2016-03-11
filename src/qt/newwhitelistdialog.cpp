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
	ui->discountDisclaimer->setText(tr("<font color='blue'>Enter the alias and discount level of your affiliate. This is a percentage of price for your offer you want to allow your affiliate to purchase your offer for. Typically given to wholesalers or for special arrangements with an affiliate. </font>"));
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
	params.push_back(ui->aliasEdit->text().toStdString());
	params.push_back(ui->discountEdit->text().toStdString());

	try {
        UniValue result = tableRPC.execute(strMethod, params);

		entry = ui->aliasEdit->text();

		QMessageBox::information(this, windowTitle(),
        tr("New affiliate added successfully!"),
			QMessageBox::Ok, QMessageBox::Ok);
			
		
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
		tr("Error creating new affiliate: \"%1\"").arg(QString::fromStdString(strError)),
			QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("General exception creating new affiliate: \"%1\"").arg(QString::fromStdString(e.what())),
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
                tr("The entered entry \"%1\" is not a valid affiliate.").arg(ui->aliasEdit->currentText()),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case OfferWhitelistTableModel::DUPLICATE_ENTRY:
            QMessageBox::warning(this, windowTitle(),
                tr("The entered entry \"%1\" is already taken.").arg(ui->aliasEdit->currentText()),
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

