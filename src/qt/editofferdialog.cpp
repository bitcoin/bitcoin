#include "editofferdialog.h"
#include "ui_editofferdialog.h"

#include "offertablemodel.h"
#include "guiutil.h"
#include "walletmodel.h"
#include "syscoingui.h"
#include "ui_interface.h"
#include <QDataWidgetMapper>
#include <QMessageBox>
#include <QStringList>
#include "rpcserver.h"
#include "main.h"
using namespace std;

extern const CRPCTable tableRPC;
string getCurrencyToSYSFromAlias(const vector<unsigned char> &vchCurrency, CAmount &nFee, const unsigned int &nHeightToFind, vector<string>& rateList, int &precision);
extern vector<unsigned char> vchFromString(const std::string &str);
EditOfferDialog::EditOfferDialog(Mode mode, const QString &strCert, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditOfferDialog), mapper(0), mode(mode), model(0)
{
    ui->setupUi(this);
	CAmount nFee;
	vector<string> rateList;
	int precision;
	if(getCurrencyToSYSFromAlias(vchFromString("SYS_RATES"), vchFromString("USD"), nFee, chainActive.Tip()->nHeight, rateList, precision) == "1")
	{
		QMessageBox::warning(this, windowTitle(),
			tr("Warning: SYS_RATES alias not found. No currency information available!"),
				QMessageBox::Ok, QMessageBox::Ok);
	}
	ui->aliasPegDisclaimer->setText(tr("<font color='blue'>Choose an alias which has peg information to allow exchange of currencies into SYS amounts based on the pegged values. Consumers will pay amounts based on this peg, the alias must be managed effectively or you may end up selling your offers for unexpected amounts.</font>"));
	ui->aliasDisclaimer->setText(tr("<font color='blue'>Select an alias to own this offer</font>"));
	ui->privateDisclaimer->setText(tr("<font color='blue'>All offers are first listed as private. If you would like your offer to be public, please edit it after it is created.</font>"));
	ui->offerLabel->setVisible(true);
	ui->offerEdit->setVisible(true);
	ui->offerEdit->setEnabled(false);
	ui->aliasEdit->setEnabled(true);
	ui->currencyDisclaimer->setVisible(true);
	ui->privateEdit->setEnabled(true);
	ui->privateEdit->clear();
	ui->privateEdit->addItem(QString("Yes"));
	ui->privateEdit->addItem(QString("No"));
	
	ui->acceptBTCOnlyEdit->clear();
	ui->acceptBTCOnlyEdit->addItem(QString("No"));
	ui->acceptBTCOnlyEdit->addItem(QString("Yes"));
	ui->btcOnlyDisclaimer->setText(tr("<font color='blue'>You will receive payment in Bitcoin if you have selected <b>Yes</b> to this option and <b>BTC</b> as the currency for the offer.</font>"));
	for(int i =0;i<rateList.size();i++)
	{
		ui->currencyEdit->addItem(QString::fromStdString(rateList[i]));
	}
	cert = strCert;
	ui->certEdit->clear();
	ui->certEdit->addItem(tr("Select Certificate (optional)"));
	connect(ui->certEdit, SIGNAL(currentIndexChanged(int)), this, SLOT(certChanged(int)));
	loadAliases();
	loadCerts();
	
	ui->descriptionEdit->setStyleSheet("color: rgb(0, 0, 0); background-color: rgb(255, 255, 255)");
    switch(mode)
    {
    case NewOffer:
		ui->offerLabel->setVisible(false);
		ui->offerEdit->setVisible(false);
		ui->aliasPegEdit->setText(tr("SYS_RATES"));
		ui->privateEdit->setCurrentIndex(ui->privateEdit->findText("Yes"));
		ui->privateEdit->setEnabled(false);
        setWindowTitle(tr("New Offer"));
		ui->currencyDisclaimer->setText(tr("<font color='blue'>You will receive payment in Syscoin equivalent to the Market-value of the currency you have selected.</font>"));
        break;
    case EditOffer:
		ui->currencyEdit->setEnabled(false);
		ui->currencyDisclaimer->setVisible(false);
        setWindowTitle(tr("Edit Offer"));
        break;
    case NewCertOffer:
		ui->aliasEdit->setEnabled(false);
		ui->offerLabel->setVisible(false);
		ui->privateEdit->setCurrentIndex(ui->privateEdit->findText("Yes"));
		ui->privateEdit->setEnabled(false);
		ui->offerEdit->setVisible(false);
        setWindowTitle(tr("New Offer(Certificate)"));
		ui->qtyEdit->setText("1");
		ui->qtyEdit->setEnabled(false);
		
		ui->currencyDisclaimer->setText(tr("<font color='blue'>You will receive payment in Syscoin equivalent to the Market-value of the currency you have selected.</font>"));
        break;
	}
    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
}
void EditOfferDialog::certChanged(int index)
{
	if(index > 0)
	{
		ui->qtyEdit->setText("1");
		ui->qtyEdit->setEnabled(false);
		ui->aliasEdit->setEnabled(false);
		ui->aliasDisclaimer->setText(tr("<font color='blue'>This will automatically use the alias which owns the certificate you are selling</font>"));
	}
	else if(index == 0)
	{
		ui->aliasDisclaimer->setText(tr("<font color='blue'>Select an alias to own this offer</font>"));
		ui->aliasEdit->setEnabled(true);
		ui->qtyEdit->setEnabled(true);
	}
}
void EditOfferDialog::loadCerts()
{
	string strMethod = string("certlist");
    UniValue params(UniValue::VARR); 
	UniValue result;
	string name_str;
	string title_str;
	string alias_str;
	int expired = 0;
	
	try {
		result = tableRPC.execute(strMethod, params);

		if (result.type() == UniValue::VARR)
		{
			name_str = "";
			title_str = "";
			alias_str = "";
			expired = 0;


	
			const UniValue &arr = result.get_array();
		    for (unsigned int idx = 0; idx < arr.size(); idx++) {
			    const UniValue& input = arr[idx];
				if (input.type() != UniValue::VOBJ)
					continue;
				const UniValue& o = input.get_obj();
				name_str = "";

				expired = 0;


		
				const UniValue& name_value = find_value(o, "cert");
				if (name_value.type() == UniValue::VSTR)
					name_str = name_value.get_str();
				const UniValue& title_value = find_value(o, "title");
				if (title_value.type() == UniValue::VSTR)
					title_str = title_value.get_str();	
				const UniValue& alias_value = find_value(o, "alias");
				if (alias_value.type() == UniValue::VSTR)
					alias_str = alias_value.get_str();	
				const UniValue& expired_value = find_value(o, "expired");
				if (expired_value.type() == UniValue::VNUM)
					expired = expired_value.get_int();
				
				if(expired == 0)
				{
					QString name = QString::fromStdString(name_str);
					QString title = QString::fromStdString(title_str);
					QString alias = QString::fromStdString(alias_str);
					QString certText = name + " - " + title;
					ui->certEdit->addItem(certText,name);
					if(name == cert)
					{
						int index = ui->certEdit->findData(name);
						if ( index != -1 ) 
						{
						    ui->certEdit->setCurrentIndex(index);
							ui->aliasEdit->setEnabled(false);
							ui->aliasDisclaimer->setText(tr("<font color='blue'>This will automatically use the alias which owns the certificate you are selling</font>"));
						}
						index = ui->aliasEdit->findData(alias);
						if ( index != -1 ) 
						{
						    ui->aliasEdit->setCurrentIndex(index);
							ui->aliasEdit->setEnabled(false);
							ui->aliasDisclaimer->setText(tr("<font color='blue'>This will automatically use the alias which owns the certificate you are selling</font>"));
						}
					}
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
void EditOfferDialog::loadAliases()
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
EditOfferDialog::~EditOfferDialog()
{
    delete ui;
}

void EditOfferDialog::setModel(WalletModel* walletModel, OfferTableModel *model)
{
    this->model = model;
	this->walletModel = walletModel;
    if(!model) return;

    mapper->setModel(model);
	mapper->addMapping(ui->offerEdit, OfferTableModel::Name);
	mapper->addMapping(ui->certEdit, OfferTableModel::Cert);
    mapper->addMapping(ui->nameEdit, OfferTableModel::Title);
	mapper->addMapping(ui->categoryEdit, OfferTableModel::Category);
    mapper->addMapping(ui->priceEdit, OfferTableModel::Price);
	mapper->addMapping(ui->qtyEdit, OfferTableModel::Qty);	
	mapper->addMapping(ui->descriptionEdit, OfferTableModel::Description);		
	mapper->addMapping(ui->aliasPegEdit, OfferTableModel::AliasPeg);	
    
}

void EditOfferDialog::loadRow(int row)
{
	const QModelIndex tmpIndex;
	if(model)
	{
		mapper->setCurrentIndex(row);
		QModelIndex indexCurrency = model->index(row, OfferTableModel::Currency, tmpIndex);
		QModelIndex indexPrivate = model->index(row, OfferTableModel::Private, tmpIndex);	
		QModelIndex indexAlias = model->index(row, OfferTableModel::Alias, tmpIndex);
		QModelIndex indexQty = model->index(row, OfferTableModel::Qty, tmpIndex);
		QModelIndex indexBTCOnly = model->index(row, OfferTableModel::AcceptBTCOnly, tmpIndex);
		if(indexPrivate.isValid())
		{
			QString privateStr = indexPrivate.data(OfferTableModel::PrivateRole).toString();
			ui->privateEdit->setCurrentIndex(ui->privateEdit->findText(privateStr));
		}
		if(indexCurrency.isValid())
		{
			QString currencyStr = indexCurrency.data(OfferTableModel::CurrencyRole).toString();
			ui->currencyEdit->setCurrentIndex(ui->currencyEdit->findText(currencyStr));
		}
		if(indexBTCOnly.isValid())
		{
			QString btcOnlyStr = indexBTCOnly.data(OfferTableModel::BTCOnlyRole).toString();
			ui->acceptBTCOnlyEdit->setCurrentIndex(ui->acceptBTCOnlyEdit->findText(btcOnlyStr));
		}
		if(indexAlias.isValid())
		{
			QString aliasStr = indexAlias.data(OfferTableModel::AliasRole).toString();
			ui->aliasEdit->setCurrentIndex(ui->aliasEdit->findText(aliasStr));
		}
		if(indexQty.isValid())
		{
			QString qtyStr = indexBTCOnly.data(OfferTableModel::QtyRole).toString();
			if(qtyStr == tr("unlimited"))
				ui->qtyEdit->setText("-1");
			else
				ui->qtyEdit->setText(qtyStr);
		}
	}
}

bool EditOfferDialog::saveCurrentRow()
{

    if(!walletModel) return false;
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if(!ctx.isValid())
    {
		if(model)
			model->editStatus = OfferTableModel::WALLET_UNLOCK_FAILURE;
        return false;
    }
	UniValue params(UniValue::VARR);
	string strMethod;
    switch(mode)
    {
    case NewOffer:
	case NewCertOffer:
        if (ui->nameEdit->text().trimmed().isEmpty()) {
            ui->nameEdit->setText("");
            QMessageBox::information(this, windowTitle(),
            tr("Empty name for Offer not allowed. Please try again"),
                QMessageBox::Ok, QMessageBox::Ok);
            return false;
        }
		 if (ui->aliasPegEdit->text() != "SYS_RATES") {
			QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm Alias Peg"),
                 tr("Warning: By default the system peg is <b>SYS_RATES</b>. You have chosen a peg managed by someone other than the Syscoin team!") + "<br><br>" + tr("Are you sure you wish to choose this alias as your offer peg?"),
                 QMessageBox::Yes|QMessageBox::Cancel,
                 QMessageBox::Cancel);
			if(retval == QMessageBox::Cancel)
				return false;
		}
		strMethod = string("offernew");
		params.push_back(ui->aliasPegEdit->text().toStdString());
		params.push_back(ui->aliasEdit->currentText().toStdString());
		params.push_back(ui->categoryEdit->text().toStdString());
		params.push_back(ui->nameEdit->text().toStdString());
		params.push_back(ui->qtyEdit->text().toStdString());
		params.push_back(ui->priceEdit->text().toStdString());
		params.push_back(ui->descriptionEdit->toPlainText().toStdString());
		params.push_back(ui->currencyEdit->currentText().toStdString());
		if(ui->certEdit->currentIndex() > 0)
			params.push_back(ui->certEdit->itemData(ui->certEdit->currentIndex()).toString().toStdString());
		else
			params.push_back("");
		params.push_back("1");
		params.push_back(ui->acceptBTCOnlyEdit->currentText() == QString("Yes")? "1": "0");
		try {
            UniValue result = tableRPC.execute(strMethod, params);
			const UniValue &arr = result.get_array();
			string strResult = arr[0].get_str();
			offer = ui->nameEdit->text();
			
		}
		catch (UniValue& objError)
		{
			string strError = find_value(objError, "message").get_str();
			QMessageBox::critical(this, windowTitle(),
			tr("Error creating new Offer: \"%1\"").arg(QString::fromStdString(strError)),
				QMessageBox::Ok, QMessageBox::Ok);
			break;
		}
		catch(std::exception& e)
		{
			QMessageBox::critical(this, windowTitle(),
				tr("General exception creating new Offer: \"%1\"").arg(QString::fromStdString(e.what())),
				QMessageBox::Ok, QMessageBox::Ok);
			break;
		}							

        break;
    case EditOffer:
		 if (ui->aliasPegEdit->text() != "SYS_RATES") {
			QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm Alias Peg"),
                 tr("Warning: By default the system peg is <b>SYS_RATES</b>. You have chosen a peg managed by someone other than the Syscoin team!") + "<br><br>" + tr("Are you sure you wish to choose this alias as your offer peg?"),
                 QMessageBox::Yes|QMessageBox::Cancel,
                 QMessageBox::Cancel);
			if(retval == QMessageBox::Cancel)
				return false;
		}
        if(mapper->submit())
        {
			strMethod = string("offerupdate");
			params.push_back(ui->aliasPegEdit->text().toStdString());
			params.push_back(ui->aliasEdit->currentText().toStdString());
			params.push_back(ui->offerEdit->text().toStdString());
			params.push_back(ui->categoryEdit->text().toStdString());
			params.push_back(ui->nameEdit->text().toStdString());
			params.push_back(ui->qtyEdit->text().toStdString());
			params.push_back(ui->priceEdit->text().toStdString());
			params.push_back(ui->descriptionEdit->toPlainText().toStdString());
			params.push_back(ui->privateEdit->currentText() == QString("Yes")? "1": "0");
			if(ui->certEdit->currentIndex() > 0)
				params.push_back(ui->certEdit->itemData(ui->certEdit->currentIndex()).toString().toStdString());
			try {
				UniValue result = tableRPC.execute(strMethod, params);
				if (result.type() != UniValue::VNULL)
				{
					offer = ui->nameEdit->text() + ui->offerEdit->text();

				}
			}
			catch (UniValue& objError)
			{
				string strError = find_value(objError, "message").get_str();
				QMessageBox::critical(this, windowTitle(),
				tr("Error updating Offer: \"%1\"").arg(QString::fromStdString(strError)),
					QMessageBox::Ok, QMessageBox::Ok);
				break;
			}
			catch(std::exception& e)
			{
				QMessageBox::critical(this, windowTitle(),
					tr("General exception updating Offer: \"%1\"").arg(QString::fromStdString(e.what())),
					QMessageBox::Ok, QMessageBox::Ok);
				break;
			}	
        }
        break;
    }
    return !offer.isEmpty();
}

void EditOfferDialog::accept()
{
    if(!saveCurrentRow())
    {
		if(model)
		{
			switch(model->getEditStatus())
			{
			case OfferTableModel::OK:
				// Failed with unknown reason. Just reject.
				break;
			case OfferTableModel::NO_CHANGES:
				// No changes were made during edit operation. Just reject.
				break;
			case OfferTableModel::INVALID_OFFER:
				QMessageBox::warning(this, windowTitle(),
					tr("The entered offer \"%1\" is not a valid Syscoin Offer.").arg(ui->offerEdit->text()),
					QMessageBox::Ok, QMessageBox::Ok);
				break;
			case OfferTableModel::DUPLICATE_OFFER:
				QMessageBox::warning(this, windowTitle(),
					tr("The entered offer \"%1\" is already taken.").arg(ui->offerEdit->text()),
					QMessageBox::Ok, QMessageBox::Ok);
				break;
			case OfferTableModel::WALLET_UNLOCK_FAILURE:
				QMessageBox::critical(this, windowTitle(),
					tr("Could not unlock wallet."),
					QMessageBox::Ok, QMessageBox::Ok);
				break;

			}
			return;
		}
    }
    QDialog::accept();
}

QString EditOfferDialog::getOffer() const
{
    return offer;
}

void EditOfferDialog::setOffer(const QString &offer)
{
    this->offer = offer;
    ui->offerEdit->setText(offer);
}
