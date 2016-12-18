/*
 * Syscoin Developers 2016
 */
#include "myescrowlistpage.h"
#include "ui_myescrowlistpage.h"
#include "escrowtablemodel.h"
#include "newmessagedialog.h"
#include "manageescrowdialog.h"
#include "escrowinfodialog.h"
#include "clientmodel.h"
#include "platformstyle.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "syscoingui.h"
#include "csvmodelwriter.h"
#include "guiutil.h"

#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMessageBox>
#include <QSettings>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "rpc/server.h"

#include "qzecjsonrpcclient.h"
#include "qbtcjsonrpcclient.h"
using namespace std;

extern CRPCTable tableRPC;
MyEscrowListPage::MyEscrowListPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MyEscrowListPage),
    model(0),
    optionsModel(0),
	platformStyle(platformStyle)
{
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();  
	if (!platformStyle->getImagesOnButtons())
	{
		ui->exportButton->setIcon(QIcon());
		ui->arbiterMessageButton->setIcon(QIcon());
		ui->sellerMessageButton->setIcon(QIcon());
		ui->buyerMessageButton->setIcon(QIcon());
		ui->manageButton->setIcon(QIcon());
		ui->copyEscrow->setIcon(QIcon());
		ui->refreshButton->setIcon(QIcon());
		ui->detailButton->setIcon(QIcon());
		ui->ackButton->setIcon(QIcon());
		ui->extButton->setIcon(QIcon());
	}
	else
	{
		ui->exportButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/export"));
		ui->arbiterMessageButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/outmail"));
		ui->sellerMessageButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/outmail"));
		ui->buyerMessageButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/outmail"));
		ui->manageButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/escrow1"));
		ui->copyEscrow->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/editcopy"));
		ui->refreshButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/refresh"));
		ui->detailButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/details"));
		ui->ackButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/synced"));
		ui->extButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/search"));
	}

    ui->labelExplanation->setText(tr("These are your registered Syscoin Escrows. Escrow operations (create, release, refund, complete) take 2-5 minutes to become active. You can choose which aliases to view related escrows using the dropdown to the right."));
	
	connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(on_detailButton_clicked()));
    // Context menu actions
    QAction *copyEscrowAction = new QAction(ui->copyEscrow->text(), this);
	QAction *copyOfferAction = new QAction(tr("Copy Offer ID"), this);
    QAction *manageAction = new QAction(tr("Manage Escrow"), this);
	QAction *ackAction = new QAction(tr("Acknowledge Payment"), this);
	QAction *detailsAction = new QAction(tr("Details"), this);
	QAction *extAction = new QAction(tr("Check External Payment"), this);
    QAction *buyerMessageAction = new QAction(tr("Send Msg To Buyer"), this);
	QAction *sellerMessageAction = new QAction(tr("Send Msg To Seller"), this);
	QAction *arbiterMessageAction = new QAction(tr("Send Msg To Arbiter"), this);

    // Build context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyEscrowAction);
	contextMenu->addAction(copyOfferAction);
	contextMenu->addAction(buyerMessageAction);
	contextMenu->addAction(sellerMessageAction);
	contextMenu->addAction(arbiterMessageAction);
    contextMenu->addSeparator();
	contextMenu->addAction(detailsAction);
	contextMenu->addAction(manageAction);
	contextMenu->addAction(ackAction);
	contextMenu->addAction(extAction);

    // Connect signals for context menu actions
    connect(copyEscrowAction, SIGNAL(triggered()), this, SLOT(on_copyEscrow_clicked()));
	connect(copyOfferAction, SIGNAL(triggered()), this, SLOT(on_copyOffer_clicked()));
	connect(manageAction, SIGNAL(triggered()), this, SLOT(on_manageButton_clicked()));
	connect(ackAction, SIGNAL(triggered()), this, SLOT(on_ackButton_clicked()));
	connect(extAction, SIGNAL(triggered()), this, SLOT(on_extButton_clicked()));

	connect(buyerMessageAction, SIGNAL(triggered()), this, SLOT(on_buyerMessageButton_clicked()));
	connect(sellerMessageAction, SIGNAL(triggered()), this, SLOT(on_sellerMessageButton_clicked()));
	connect(arbiterMessageAction, SIGNAL(triggered()), this, SLOT(on_arbiterMessageButton_clicked()));
	connect(detailsAction, SIGNAL(triggered()), this, SLOT(on_detailButton_clicked()));
	

    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));
	connect(ui->completeCheck,SIGNAL(clicked(bool)),SLOT(onToggleShowComplete(bool)));

	connect(ui->displayListAlias,SIGNAL(currentIndexChanged(const QString&)),this,SLOT(displayListChanged(const QString&)));
	loadAliasList();

}
void MyEscrowListPage::loadAliasList()
{
	QSettings settings;
	QString oldListAlias = settings.value("defaultListAlias", "").toString();
	ui->displayListAlias->clear();
	ui->displayListAlias->addItem(tr("All"));
	
	
	UniValue aliasList(UniValue::VARR);
	appendListAliases(aliasList, true);
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
bool MyEscrowListPage::lookup(const QString &escrow, QString& address, QString& price, QString& extTxId, QString& redeemtxid, QString& paymentOption)
{
	QSettings settings;
	string strMethod = string("escrowinfo");
    UniValue params(UniValue::VARR); 
	params.push_back(escrow.toStdString());
	UniValue result ;
	string name_str;
	try {
		result = tableRPC.execute(strMethod, params);
		if (result.type() == UniValue::VOBJ)
		{
			const UniValue& o = result.get_obj();
	
			const UniValue& name_value = find_value(o, "escrow");
			if (name_value.type() == UniValue::VSTR)
				name_str = name_value.get_str();
			if(QString::fromStdString(name_str) != escrow)
				return false;
			
			const UniValue& address_value = find_value(o, "escrowaddress");
			if (address_value.type() == UniValue::VSTR)
				address = QString::fromStdString(address_value.get_str());
		
			const UniValue& total_value = find_value(o, "totalwithfee");
			if (total_value.type() == UniValue::VNUM)
				price = QString::number(ValueFromAmount(total_value.get_int64()).get_real());

			const UniValue& exttxid_value = find_value(o, "exttxid");
			if (exttxid_value.type() == UniValue::VSTR)
				extTxId = QString::fromStdString(exttxid_value.get_str());
			const UniValue& paymentOption_Value = find_value(o, "paymentoption_display");
			if (paymentOption_Value.type() == UniValue::VSTR)
				paymentOption = QString::fromStdString(paymentOption_Value.get_str());
			const UniValue& redeemtxid_value = find_value(o, "redeem_txid");
			if (redeemtxid_value.type() == UniValue::VSTR)
				redeemtxid = QString::fromStdString(redeemtxid_value.get_str());
			return true;
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
void MyEscrowListPage::displayListChanged(const QString& alias)
{
	QSettings settings;
	settings.setValue("defaultListAlias", alias);
	settings.sync();
}
MyEscrowListPage::~MyEscrowListPage()
{
    delete ui;
}
void MyEscrowListPage::onToggleShowComplete(bool toggled)
{
	if(!model)
		return;
	model->showComplete(toggled);
}
void MyEscrowListPage::showEvent ( QShowEvent * event )
{
    if(!walletModel) return;

}
void MyEscrowListPage::setModel(WalletModel *walletModel, EscrowTableModel *model)
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
	proxyModel->setFilterRole(EscrowTableModel::TypeRole);

	ui->tableView->setModel(proxyModel);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);

    // Set column widths
    ui->tableView->setColumnWidth(0, 50); //escrow id
    ui->tableView->setColumnWidth(1, 50); //time
    ui->tableView->setColumnWidth(2, 150); //seller
    ui->tableView->setColumnWidth(3, 150); //arbiter
    ui->tableView->setColumnWidth(4, 150); //buyer
    ui->tableView->setColumnWidth(5, 80); //offer
	ui->tableView->setColumnWidth(6, 250); //offer title
	ui->tableView->setColumnWidth(7, 80); //total
	ui->tableView->setColumnWidth(8, 150); //rating
    ui->tableView->setColumnWidth(9, 50); //status
	

    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    // Select row for newly created escrow
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewEscrow(QModelIndex,int,int)));
}

void MyEscrowListPage::setOptionsModel(ClientModel* clientmodel, OptionsModel *optionsModel)
{
    this->optionsModel = optionsModel;
	this->clientModel = clientmodel;
}

void MyEscrowListPage::on_copyEscrow_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, EscrowTableModel::Escrow);
}
void MyEscrowListPage::slotConfirmedFinished(QNetworkReply * reply){
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
		if(m_strAddress.size() > 0)
		{
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
										tr("Transaction was found in the blockchain! Escrow funding payment has been detected. Chain: ") + chain,
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
				QMessageBox::information(this, windowTitle(),
					tr("Transaction was found in the blockchain! Escrow payment has been detected. It is recommended that you confirm payment by opening your wallet and seeing the funds in your account. Chain: ") + chain,
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
void MyEscrowListPage::CheckPaymentInBTC(const QString &strExtTxId, const QString& address, const QString& price)
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
void MyEscrowListPage::CheckPaymentInZEC(const QString &strExtTxId, const QString& address, const QString& price)
{
	dblPrice = price.toDouble();
	m_buttonText = ui->extButton->text();
	ui->extButton->setText(tr("Please Wait..."));
	ui->extButton->setEnabled(false);
	m_strAddress = address;
	m_strExtTxId = strExtTxId;

	ZecRpcClient zecClient;
	QNetworkAccessManager *nam = new QNetworkAccessManager(this);  
	connect(nam, SIGNAL(finished(QNetworkReply *)), this, SLOT(slotConfirmedFinished(QNetworkReply *)));
	zecClient.sendRawTxRequest(nam, strExtTxId);

}
void MyEscrowListPage::on_extButton_clicked()
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
	QString address, price, extTxId, redeemTxId;
	QString escrow = selection.at(0).data(EscrowTableModel::EscrowRole).toString();
	if(!lookup(escrow, address, price, extTxId, redeemTxId, m_paymentOption))
	{
        QMessageBox::critical(this, windowTitle(),
        tr("Could not find this escrow, please ensure the escrow has been confirmed by the blockchain: ") + escrow,
            QMessageBox::Ok, QMessageBox::Ok);
        return;
	}
	if(extTxId.isEmpty())
	{
        QMessageBox::critical(this, windowTitle(),
        tr("This payment was not done using another coin, please select an escrow that was created by paying with another blockchain."),
            QMessageBox::Ok, QMessageBox::Ok);
        return;
	}
	if(m_paymentOption == QString("BTC"))
		CheckPaymentInBTC(redeemTxId.size() > 0? redeemTxId: extTxId, redeemTxId.size() > 0? "": address, price);
	else if(m_paymentOption == QString("ZEC"))
		CheckPaymentInZEC(redeemTxId.size() > 0? redeemTxId: extTxId, redeemTxId.size() > 0? "": address, price);


}
void MyEscrowListPage::on_ackButton_clicked()
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
	QString escrow = selection.at(0).data(EscrowTableModel::EscrowRole).toString();
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm Escrow Acknowledgement"),
         tr("Warning: You are about to acknowledge this payment from the buyer. If you are shipping an item, please communicate a tracking number to the buyer via a Syscoin message.") + "<br><br>" + tr("Are you sure you wish to acknowledge this payment?"),
         QMessageBox::Yes|QMessageBox::Cancel,
         QMessageBox::Cancel);
    if(retval == QMessageBox::Yes)
    {
		UniValue params(UniValue::VARR);
		string strMethod = string("escrowacknowledge");
		params.push_back(escrow.toStdString());

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
			tr("Error acknowledging escrow payment: ") + QString::fromStdString(strError),
				QMessageBox::Ok, QMessageBox::Ok);
		}
		catch(std::exception& e)
		{
			QMessageBox::critical(this, windowTitle(),
				tr("General exception acknowledging escrow payment"),
				QMessageBox::Ok, QMessageBox::Ok);
		}
	}     
}
void MyEscrowListPage::on_copyOffer_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, EscrowTableModel::Offer);
}
void MyEscrowListPage::on_manageButton_clicked()
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
	QString escrow = selection.at(0).data(EscrowTableModel::EscrowRole).toString();
	ManageEscrowDialog dlg(walletModel, escrow);   
	dlg.exec();
}
void MyEscrowListPage::on_buyerMessageButton_clicked()
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
	QString buyer = selection.at(0).data(EscrowTableModel::BuyerRole).toString();
	// send message to seller
	NewMessageDialog dlg(NewMessageDialog::NewMessage, buyer);   
	dlg.exec();
}
void MyEscrowListPage::on_sellerMessageButton_clicked()
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
	QString sellerAlias = selection.at(0).data(EscrowTableModel::SellerRole).toString();
	// send message to seller
	NewMessageDialog dlg(NewMessageDialog::NewMessage, sellerAlias);   
	dlg.exec();
}
void MyEscrowListPage::on_arbiterMessageButton_clicked()
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
	QString arbAlias = selection.at(0).data(EscrowTableModel::ArbiterRole).toString();
	// send message to arbiter
	NewMessageDialog dlg(NewMessageDialog::NewMessage, arbAlias);   
	dlg.exec();
}
void MyEscrowListPage::on_refreshButton_clicked()
{
    if(!model)
        return;
	loadAliasList();
    model->refreshEscrowTable();
}

void MyEscrowListPage::on_exportButton_clicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(
            this,
            tr("Export Escrow Data"), QString(),
            tr("Comma separated file (*.csv)"), NULL);

    if (filename.isNull()) return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(proxyModel);
    writer.addColumn("Escrow", EscrowTableModel::Escrow, Qt::EditRole);
	writer.addColumn("Time", EscrowTableModel::Time, Qt::EditRole);
    writer.addColumn("Arbiter", EscrowTableModel::Arbiter, Qt::EditRole);
	writer.addColumn("Seller", EscrowTableModel::Seller, Qt::EditRole);
	writer.addColumn("Offer", EscrowTableModel::Offer, Qt::EditRole);
	writer.addColumn("OfferTitle", EscrowTableModel::OfferTitle, Qt::EditRole);
	writer.addColumn("Total", EscrowTableModel::Total, Qt::EditRole);
	writer.addColumn("Rating", EscrowTableModel::Rating, EscrowTableModel::RatingRole);
	writer.addColumn("Status", EscrowTableModel::Status, Qt::EditRole);
    if(!writer.write())
    {
		QMessageBox::critical(this, tr("Error exporting"), tr("Could not write to file: ") + filename,
                              QMessageBox::Abort, QMessageBox::Abort);
    }
}

void MyEscrowListPage::on_detailButton_clicked()
{
    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        EscrowInfoDialog dlg(platformStyle, selection.at(0));
        dlg.exec();
    }
}
void MyEscrowListPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid()) {
        contextMenu->exec(QCursor::pos());
    }
}

void MyEscrowListPage::selectNewEscrow(const QModelIndex &parent, int begin, int /*end*/)
{
    QModelIndex idx = proxyModel->mapFromSource(model->index(begin, EscrowTableModel::Escrow, parent));
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() == newEscrowToSelect))
    {
        // Select row of newly created escrow, once
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newEscrowToSelect.clear();
    }
}
