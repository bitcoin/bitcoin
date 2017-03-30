/*
 * Syscoin Developers 2015
 */
#include "myacceptedofferlistpage.h"
#include "ui_myacceptedofferlistpage.h"
#include "offeraccepttablemodel.h"
#include "offeracceptinfodialog.h"
#include "newmessagedialog.h"
#include "offerfeedbackdialog.h"
#include "clientmodel.h"
#include "platformstyle.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "syscoingui.h"
#include "csvmodelwriter.h"
#include "guiutil.h"
#include "rpc/server.h"
#include "util.h"
#include "utilmoneystr.h"
using namespace std;

extern CRPCTable tableRPC;
#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMessageBox>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSettings>
#include "qzecjsonrpcclient.h"
#include "qbtcjsonrpcclient.h"
MyAcceptedOfferListPage::MyAcceptedOfferListPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MyAcceptedOfferListPage),
    model(0),
    optionsModel(0),
	platformStyle(platformStyle)
{
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();  
	if (!platformStyle->getImagesOnButtons())
	{
		ui->exportButton->setIcon(QIcon());
		ui->messageButton->setIcon(QIcon());
		ui->detailButton->setIcon(QIcon());
		ui->copyOffer->setIcon(QIcon());
		ui->refreshButton->setIcon(QIcon());
		ui->extButton->setIcon(QIcon());
		ui->feedbackButton->setIcon(QIcon());
		ui->ackButton->setIcon(QIcon());

	}
	else
	{
		ui->exportButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/export"));
		ui->messageButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/outmail"));
		ui->detailButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/details"));
		ui->copyOffer->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/editcopy"));
		ui->refreshButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/refresh"));
		ui->extButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/search"));
		ui->feedbackButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/thumbsup"));
		ui->ackButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/synced"));
		
	}


    ui->labelExplanation->setText(tr("These are offers you have sold to others. Offer operations take 2-5 minutes to become active. Right click on an offer for more info including buyer message, quantity, date, etc. You can choose which aliases to view sales information for using the dropdown to the right."));
	
	connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(on_detailButton_clicked()));	
    // Context menu actions
    QAction *copyOfferAction = new QAction(ui->copyOffer->text(), this);
    QAction *copyOfferValueAction = new QAction(tr("Copy OfferAccept ID"), this);
	QAction *detailsAction = new QAction(tr("Details"), this);
	QAction *messageAction = new QAction(tr("Message Buyer"), this);
	QAction *feedbackAction = new QAction(tr("Leave Feedback For Buyer"), this);
	QAction *ackAction = new QAction(tr("Acknowledge Payment"), this);
	QAction *extAction = new QAction(tr("Check External Payment"), this);

    // Build context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyOfferAction);
    contextMenu->addAction(copyOfferValueAction);
	contextMenu->addSeparator();
	contextMenu->addAction(detailsAction);
	contextMenu->addAction(messageAction);
	contextMenu->addAction(feedbackAction);
	contextMenu->addAction(ackAction);
	contextMenu->addAction(extAction);
    // Connect signals for context menu actions
    connect(copyOfferAction, SIGNAL(triggered()), this, SLOT(on_copyOffer_clicked()));
    connect(copyOfferValueAction, SIGNAL(triggered()), this, SLOT(onCopyOfferValueAction()));
	connect(detailsAction, SIGNAL(triggered()), this, SLOT(on_detailButton_clicked()));
	connect(messageAction, SIGNAL(triggered()), this, SLOT(on_messageButton_clicked()));
	connect(feedbackAction, SIGNAL(triggered()), this, SLOT(on_feedbackButton_clicked()));
	connect(ackAction, SIGNAL(triggered()), this, SLOT(on_ackButton_clicked()));
	connect(extAction, SIGNAL(triggered()), this, SLOT(on_extButton_clicked()));

    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));
	connect(ui->displayListAlias,SIGNAL(currentIndexChanged(const QString&)),this,SLOT(displayListChanged(const QString&)));
	loadAliasList();

}
void MyAcceptedOfferListPage::loadAliasList()
{
	QSettings settings;
	QString oldListAlias = settings.value("defaultListAlias", "").toString();
	ui->displayListAlias->clear();
	ui->displayListAlias->addItem(tr("All"));
	
	
	UniValue aliasList(UniValue::VARR);
	appendListAliases(aliasList);
	for(unsigned int i = 0;i<aliasList.size();i++)
	{
		const QString& aliasName = QString::fromStdString(aliasList[i].get_str());
		ui->displayListAlias->addItem(aliasName);
	}
	int currentIndex = ui->displayListAlias->findText(oldListAlias);
	if(currentIndex >= 0)
		ui->displayListAlias->setCurrentIndex(currentIndex);
	settings.setValue("defaultListAlias", oldListAlias);
}
void MyAcceptedOfferListPage::displayListChanged(const QString& alias)
{
	QSettings settings;
	settings.setValue("defaultListAlias", alias);
	settings.sync();
}
bool MyAcceptedOfferListPage::lookup(const QString &lookupid, const QString &acceptid, QString& address, QString& price, QString& extTxId, QString& paymentOption)
{
	
	string strError;
	string strMethod = string("offeracceptlist");
	UniValue params(UniValue::VARR); 
	UniValue listAliases(UniValue::VARR);
	appendListAliases(listAliases);
	params.push_back(listAliases);
	UniValue offerAcceptsValue;
	QString offerAcceptHash;
	params.push_back(acceptid.toStdString());

    try {
        offerAcceptsValue = tableRPC.execute(strMethod, params);
		const UniValue &offerAccepts = offerAcceptsValue.get_array();
		QDateTime timestamp;
	    for (unsigned int idx = 0; idx < offerAccepts.size(); idx++) {
		    const UniValue& accept = offerAccepts[idx];				
			const UniValue& acceptObj = accept.get_obj();
			offerAcceptHash = QString::fromStdString(find_value(acceptObj, "id").get_str());
			extTxId = QString::fromStdString(find_value(acceptObj, "exttxid").get_str());	
			paymentOption = QString::fromStdString(find_value(acceptObj, "paymentoption_display").get_str());	
			const string &strPrice = find_value(acceptObj, "total").get_str();
			price = QString::fromStdString(strPrice);
			break;
		}
		if(offerAcceptHash != acceptid)
		{
			return false;
		}		
	}
	
	catch (UniValue& objError)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("Could not find this offer purchase, please ensure it has been confirmed by the blockchain: ") + lookupid,
				QMessageBox::Ok, QMessageBox::Ok);
		return true;

	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to locate this offer purchase, please ensure it has been confirmed by the blockchain: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
		return true;
	}
	UniValue result(UniValue::VOBJ);
	strMethod = string("offerinfo");
	UniValue params1(UniValue::VARR);
	params1.push_back(lookupid.toStdString());
    try {
        result = tableRPC.execute(strMethod, params1);

		if (result.type() == UniValue::VOBJ)
		{
			const string &strAddress = find_value(result, "address").get_str();				
			address = QString::fromStdString(strAddress);
			return true;
		}
	}
	catch (UniValue& objError)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("Could not find this offer, please ensure the offer has been confirmed by the blockchain: ") + lookupid,
				QMessageBox::Ok, QMessageBox::Ok);
		return true;

	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to locate this offer, please ensure the has been confirmed by the blockchain: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
		return true;
	}
	return false;


} 
QString MyAcceptedOfferListPage::convertAddress(const QString &sysAddress)
{
	UniValue params(UniValue::VARR);
	params.push_back(sysAddress.toStdString());
	UniValue resCreate;
	try
	{
		resCreate = tableRPC.execute("getzaddress", params);
		return QString::fromStdString(resCreate.get_str());
	}
	catch (UniValue& objError)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("Failed to generate ZCash address, please close this screen and try again"),
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to generate ZCash address, please close this screen and try again: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
	}
	return QString("");

}
void MyAcceptedOfferListPage::on_ackButton_clicked()
{
 	if(!model)	
		return;
	if(!ui->tableView->selectionModel())
        return;
    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if(selection.isEmpty())
    {
        return;
    }
	QString offerid = selection.at(0).data(OfferAcceptTableModel::NameRole).toString();
	QString acceptid = selection.at(0).data(OfferAcceptTableModel::GUIDRole).toString();
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm Payment Acknowledgement"),
         tr("Warning: You are about to acknowledge this payment from the buyer. If you are shipping an item, please communicate a tracking number to the buyer via a Syscoin message.") + "<br><br>" + tr("Are you sure you wish to acknowledge this payment?"),
         QMessageBox::Yes|QMessageBox::Cancel,
         QMessageBox::Cancel);
    if(retval == QMessageBox::Yes)
    {
		UniValue params(UniValue::VARR);
		string strMethod = string("offeracceptacknowledge");
		params.push_back(offerid.toStdString());
		params.push_back(acceptid.toStdString());

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
				}
			}
		}
		catch (UniValue& objError)
		{
			string strError = find_value(objError, "message").get_str();
			QMessageBox::critical(this, windowTitle(),
			tr("Error acknowledging offer payment: ") + QString::fromStdString(strError),
				QMessageBox::Ok, QMessageBox::Ok);
		}
		catch(std::exception& e)
		{
			QMessageBox::critical(this, windowTitle(),
				tr("General exception acknowledging offer payment"),
				QMessageBox::Ok, QMessageBox::Ok);
		}
	}
}
void MyAcceptedOfferListPage::slotConfirmedFinished(QNetworkReply * reply){
	QString chain;
	if(m_paymentOption == "BTC")
		chain = tr("Bitcoin");
	else if(m_paymentOption == "ZEC")
		chain = tr("ZCash");
	if(reply->error() != QNetworkReply::NoError) {
		ui->extButton->setText(m_buttonText);
		ui->extButton->setEnabled(true);
        QMessageBox::critical(this, windowTitle(),
            tr("Error making request: ") + reply->errorString(),
                QMessageBox::Ok, QMessageBox::Ok);
		reply->deleteLater();
		return;
	}
	double valueAmount = 0;
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
			return;
		}
		UniValue resultObj = resultValue.get_obj();
		UniValue hexValue = find_value(resultObj, "hex");
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
			if(confirmations <= 0)
			{
				ui->extButton->setText(m_buttonText);
				ui->extButton->setEnabled(true);
				QMessageBox::critical(this, windowTitle(),
					tr("Payment transaction found but it has not been confirmed by the blockchain yet! Please try again later. Chain: ") + chain,
						QMessageBox::Ok, QMessageBox::Ok);
				reply->deleteLater();
				return;
			}
		}
		else
		{
			ui->extButton->setText(m_buttonText);
			ui->extButton->setEnabled(true);
			QMessageBox::critical(this, windowTitle(),
				tr("Payment transaction found but it has not been confirmed by the blockchain yet! Please try again later. Chain: ") + chain,
					QMessageBox::Ok, QMessageBox::Ok);
			reply->deleteLater();
			return;
		}
		UniValue outputsValue = find_value(resultObj, "vout");
		if (outputsValue.isArray())
		{
			UniValue outputs = outputsValue.get_array();
			for (unsigned int idx = 0; idx < outputs.size(); idx++) {
				const UniValue& output = outputs[idx].get_obj();	
				UniValue paymentValue = find_value(output, "value");
				UniValue scriptPubKeyObj = find_value(output, "scriptPubKey").get_obj();
				UniValue addressesValue = find_value(scriptPubKeyObj, "addresses");
				if(addressesValue.isArray() &&  addressesValue.get_array().size() == 1)
				{
					UniValue addressValue = addressesValue.get_array()[0];
					if(addressValue.get_str() == m_strAddress.toStdString())
					{
						if(paymentValue.isNum())
						{
							valueAmount += paymentValue.get_real();
							if(valueAmount >= dblPrice)
							{
								ui->extButton->setText(m_buttonText);
								ui->extButton->setEnabled(true);
								QMessageBox::information(this, windowTitle(),
									tr("Transaction was found in the blockchain! Full payment has been detected. It is recommended that you confirm payment by opening your wallet and seeing the funds in your account. Chain: ") + chain,
									QMessageBox::Ok, QMessageBox::Ok);
								reply->deleteLater();
								return;
							}
						}
					}
						
				}
			}
		}
	}
	else
	{
		ui->extButton->setText(m_buttonText);
		ui->extButton->setEnabled(true);
		QMessageBox::critical(this, windowTitle(),
			tr("Cannot parse JSON response: ") + str,
				QMessageBox::Ok, QMessageBox::Ok);
		reply->deleteLater();
		return;
	}
	
	reply->deleteLater();
	ui->extButton->setText(m_buttonText);
	ui->extButton->setEnabled(true);
	QMessageBox::warning(this, windowTitle(),
		tr("Payment not found in the blockchain! Please try again later. Chain: ") + chain,
			QMessageBox::Ok, QMessageBox::Ok);	
}
void MyAcceptedOfferListPage::CheckPaymentInBTC(const QString &strExtTxId, const QString& address, const QString& price)
{
	dblPrice = price.toDouble();
	m_buttonText = ui->extButton->text();
	ui->extButton->setText(tr("Please Wait..."));
	ui->extButton->setEnabled(false);
	m_strAddress = address;
	m_strExtTxId = strExtTxId;

	BtcRpcClient btcClient;
	QNetworkAccessManager *nam = new QNetworkAccessManager(this);  
	connect(nam, SIGNAL(finished(QNetworkReply *)), this, SLOT(slotConfirmedFinished(QNetworkReply *)));
	btcClient.sendRawTxRequest(nam, strExtTxId);
}
void MyAcceptedOfferListPage::CheckPaymentInZEC(const QString &strExtTxId, const QString& address, const QString& price)
{
	dblPrice = price.toDouble();
	m_buttonText = ui->extButton->text();
	ui->extButton->setText(tr("Please Wait..."));
	ui->extButton->setEnabled(false);
	m_strAddress = convertAddress(address);
	m_strExtTxId = strExtTxId;

	ZecRpcClient zecClient;
	QNetworkAccessManager *nam = new QNetworkAccessManager(this);  
	connect(nam, SIGNAL(finished(QNetworkReply *)), this, SLOT(slotConfirmedFinished(QNetworkReply *)));
	zecClient.sendRawTxRequest(nam, strExtTxId);

}

void MyAcceptedOfferListPage::on_extButton_clicked()
{
 	if(!model)	
		return;
	if(!ui->tableView->selectionModel())
        return;
    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if(selection.isEmpty())
    {
        return;
    }
	QString address, price, extTxId;
	QString offerid = selection.at(0).data(OfferAcceptTableModel::NameRole).toString();
	QString acceptid = selection.at(0).data(OfferAcceptTableModel::GUIDRole).toString();
	if(!lookup(offerid, acceptid, address, price, extTxId, m_paymentOption))
	{
        QMessageBox::critical(this, windowTitle(),
        tr("Could not find this offer, please ensure the offer has been confirmed by the blockchain: ") + offerid,
            QMessageBox::Ok, QMessageBox::Ok);
        return;
	}
	if(extTxId.isEmpty())
	{
        QMessageBox::critical(this, windowTitle(),
        tr("This payment was not done using another coin, please select an offer that was accepted by paying with another blockchain."),
            QMessageBox::Ok, QMessageBox::Ok);
        return;
	}
	if(m_paymentOption == QString("BTC"))
		CheckPaymentInBTC(extTxId, address, price);
	else if(m_paymentOption == QString("ZEC"))
		CheckPaymentInZEC(extTxId, address, price);


}
void MyAcceptedOfferListPage::on_messageButton_clicked()
{
 	if(!model)	
		return;
	if(!ui->tableView->selectionModel())
        return;
    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if(selection.isEmpty())
    {
        return;
    }
	QString buyer = selection.at(0).data(OfferAcceptTableModel::BuyerRole).toString();
	// send message to buyer
	NewMessageDialog dlg(NewMessageDialog::NewMessage, buyer);   
	dlg.exec();


}

MyAcceptedOfferListPage::~MyAcceptedOfferListPage()
{
    delete ui;
}
void MyAcceptedOfferListPage::on_detailButton_clicked()
{
    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        OfferAcceptInfoDialog dlg(platformStyle, selection.at(0));
        dlg.exec();
    }
}
void MyAcceptedOfferListPage::showEvent ( QShowEvent * event )
{
    if(!walletModel) return;
    /*if(walletModel->getEncryptionStatus() == WalletModel::Locked)
	{
        ui->labelExplanation->setText(tr("<font color='blue'>WARNING: Your wallet is currently locked. For security purposes you'll need to enter your passphrase in order to interact with Syscoin Offers. Because your wallet is locked you must manually refresh this table after creating or updating an Offer. </font> <a href=\"http://lockedwallet.syscoin.org\">more info</a><br><br>These are your registered Syscoin Offers. Offer updates take 1 confirmation to appear in this table."));
		ui->labelExplanation->setTextFormat(Qt::RichText);
		ui->labelExplanation->setTextInteractionFlags(Qt::TextBrowserInteraction);
		ui->labelExplanation->setOpenExternalLinks(true);
    }*/
}
void MyAcceptedOfferListPage::setModel(WalletModel *walletModel, OfferAcceptTableModel *model)
{
    this->model = model;
	this->walletModel = walletModel;
    if(!model) return;
    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

  
		// Receive filter
		proxyModel->setFilterRole(OfferAcceptTableModel::TypeRole);
		proxyModel->setFilterFixedString(OfferAcceptTableModel::Offer);
       
    
		ui->tableView->setModel(proxyModel);
        ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);

        // Set column widths
        ui->tableView->setColumnWidth(0, 50); //offer
        ui->tableView->setColumnWidth(1, 50); //accept
        ui->tableView->setColumnWidth(2, 500); //title
        ui->tableView->setColumnWidth(3, 50); //height
        ui->tableView->setColumnWidth(4, 50); //price
        ui->tableView->setColumnWidth(5, 75); //currency
        ui->tableView->setColumnWidth(6, 75); //qty
        ui->tableView->setColumnWidth(7, 50); //total
        ui->tableView->setColumnWidth(8, 150); //seller alias
        ui->tableView->setColumnWidth(9, 150); //buyer
        ui->tableView->setColumnWidth(10, 40); //private
        ui->tableView->setColumnWidth(11, 0); //status

        ui->tableView->horizontalHeader()->setStretchLastSection(true);

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged()));

    // Select row for newly created offer
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewOffer(QModelIndex,int,int)));

    selectionChanged();
}

void MyAcceptedOfferListPage::setOptionsModel(ClientModel* clientmodel, OptionsModel *optionsModel)
{
    this->optionsModel = optionsModel;
	this->clientModel = clientmodel;
}
void MyAcceptedOfferListPage::on_feedbackButton_clicked()
{
 	if(!model || !walletModel)	
		return;
	if(!ui->tableView->selectionModel())
        return;
    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if(selection.isEmpty())
    {
        return;
    }
	QString offer = selection.at(0).data(OfferAcceptTableModel::NameRole).toString();
	QString accept = selection.at(0).data(OfferAcceptTableModel::GUIDRole).toString();
	OfferFeedbackDialog dlg(walletModel, offer, accept);   
	dlg.exec();
}
void MyAcceptedOfferListPage::on_copyOffer_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, OfferAcceptTableModel::Name);
}

void MyAcceptedOfferListPage::onCopyOfferValueAction()
{
    GUIUtil::copyEntryData(ui->tableView, OfferAcceptTableModel::GUID);
}


void MyAcceptedOfferListPage::on_refreshButton_clicked()
{
    if(!model)
        return;
	loadAliasList();
    model->refreshOfferTable();
}

void MyAcceptedOfferListPage::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->copyOffer->setEnabled(true);
		ui->messageButton->setEnabled(true);
		ui->detailButton->setEnabled(true);
    }
    else
    {
        ui->copyOffer->setEnabled(false);
		ui->messageButton->setEnabled(false);
		ui->detailButton->setEnabled(false);
    }
}

void MyAcceptedOfferListPage::done(int retval)
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel() || !table->model())
        return;

    // Figure out which offer was selected, and return it
    QModelIndexList indexes = table->selectionModel()->selectedRows(OfferAcceptTableModel::Name);

    Q_FOREACH (const QModelIndex& index, indexes)
    {
        QVariant offer = table->model()->data(index);
        returnValue = offer.toString();
    }

    if(returnValue.isEmpty())
    {
        // If no offer entry selected, return rejected
        retval = Rejected;
    }

    QDialog::done(retval);
}

void MyAcceptedOfferListPage::on_exportButton_clicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(
            this,
            tr("Export Offer Data"), QString(),
            tr("Comma separated file (*.csv)"), NULL);

    if (filename.isNull()) return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(proxyModel);

    writer.addColumn(tr("Offer ID"), OfferAcceptTableModel::Name, Qt::EditRole);
    writer.addColumn(tr("Accept ID"), OfferAcceptTableModel::GUID, Qt::EditRole);
	writer.addColumn(tr("Title"), OfferAcceptTableModel::Title, Qt::EditRole);
	writer.addColumn(tr("Height"), OfferAcceptTableModel::Height, Qt::EditRole);
	writer.addColumn(tr("Price"), OfferAcceptTableModel::Price, Qt::EditRole);
	writer.addColumn(tr("Currency"), OfferAcceptTableModel::Currency, Qt::EditRole);
	writer.addColumn(tr("Qty"), OfferAcceptTableModel::Qty, Qt::EditRole);
	writer.addColumn(tr("Total"), OfferAcceptTableModel::Total, Qt::EditRole);
	writer.addColumn(tr("Seller"), OfferAcceptTableModel::Alias, Qt::EditRole);
	writer.addColumn(tr("Buyer"), OfferAcceptTableModel::Buyer, Qt::EditRole);
	writer.addColumn(tr("Status"), OfferAcceptTableModel::Status, Qt::EditRole);
    if(!writer.write())
    {
		QMessageBox::critical(this, tr("Error exporting"), tr("Could not write to file: ") + filename,
                              QMessageBox::Abort, QMessageBox::Abort);
    }
}



void MyAcceptedOfferListPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid()) {
        contextMenu->exec(QCursor::pos());
    }
}

void MyAcceptedOfferListPage::selectNewOffer(const QModelIndex &parent, int begin, int /*end*/)
{
    QModelIndex idx = proxyModel->mapFromSource(model->index(begin, OfferAcceptTableModel::Name, parent));
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() == newOfferToSelect))
    {
        // Select row of newly created offer, once
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newOfferToSelect.clear();
    }
}
