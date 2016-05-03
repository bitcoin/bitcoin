#include <boost/assign/list_of.hpp>

#include "offerlistpage.h"
#include "ui_offerlistpage.h"
#include "offertablemodel.h"
#include "optionsmodel.h"
#include "offerview.h"
#include "walletmodel.h"
#include "syscoingui.h"
#include "resellofferdialog.h"
#include "newmessagedialog.h"
#include "csvmodelwriter.h"
#include "guiutil.h"
#include "platformstyle.h"
#include "ui_interface.h"
#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMessageBox>
#include <QKeyEvent>
#include <QMenu>
#include "main.h"
#include "rpcserver.h"
using namespace std;


extern const CRPCTable tableRPC;

extern int GetOfferExpirationDepth();
OfferListPage::OfferListPage(const PlatformStyle *platformStyle, OfferView *parent) :
    QDialog(0),
    ui(new Ui::OfferListPage),
    model(0),
    optionsModel(0),
	offerView(parent)
{
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();  
	if (!platformStyle->getImagesOnButtons())
	{
		ui->copyOffer->setIcon(QIcon());
		ui->exportButton->setIcon(QIcon());
		ui->resellButton->setIcon(QIcon());
		ui->purchaseButton->setIcon(QIcon());
		ui->messageButton->setIcon(QIcon());
		ui->searchOffer->setIcon(QIcon());
	}
	else
	{
		ui->copyOffer->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/editcopy"));
		ui->exportButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/export"));
		ui->resellButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/cart"));
		ui->purchaseButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/send"));
		ui->messageButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/outmail"));
		ui->searchOffer->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/search"));
	}
	

    ui->labelExplanation->setText(tr("Search for Syscoin Offers (double click on one to purchase). Select the number of results desired from the dropdown box and click Search."));
	
    // Context menu actions
    QAction *copyOfferAction = new QAction(ui->copyOffer->text(), this);
    QAction *copyOfferValueAction = new QAction(tr("&Copy Title"), this);
	QAction *copyOfferDescriptionAction = new QAction(tr("&Copy Description"), this);
	QAction *resellAction = new QAction(tr("&Resell"), this);
	QAction *purchaseAction = new QAction(tr("&Purchase"), this);
	QAction *messageSellerAction = new QAction(tr("&Message Seller"), this);

    // Build context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyOfferAction);
    contextMenu->addAction(copyOfferValueAction);
	contextMenu->addAction(copyOfferDescriptionAction);
	contextMenu->addSeparator();
	contextMenu->addAction(messageSellerAction);
	contextMenu->addAction(resellAction);
	contextMenu->addAction(purchaseAction);
    // Connect signals for context menu actions
    connect(copyOfferAction, SIGNAL(triggered()), this, SLOT(on_copyOffer_clicked()));
    connect(copyOfferValueAction, SIGNAL(triggered()), this, SLOT(onCopyOfferValueAction()));
	connect(copyOfferDescriptionAction, SIGNAL(triggered()), this, SLOT(onCopyOfferDescriptionAction()));
	connect(resellAction, SIGNAL(triggered()), this, SLOT(on_resellButton_clicked()));
	connect(purchaseAction, SIGNAL(triggered()), this, SLOT(on_purchaseButton_clicked()));
	connect(messageSellerAction, SIGNAL(triggered()), this, SLOT(on_messageButton_clicked()));
	connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(on_purchaseButton_clicked()));
    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));


	ui->lineEditOfferSearch->setPlaceholderText(tr("Enter search term, regex accepted (ie: ^name returns all Offer's starting with 'name'). Empty will search for all."));
}

OfferListPage::~OfferListPage()
{
    delete ui;
}
void OfferListPage::showEvent ( QShowEvent * event )
{
    if(!walletModel) return;
    /*if(walletModel->getEncryptionStatus() == WalletModel::Locked)
	{
        ui->labelExplanation->setText(tr("<font color='blue'>WARNING: Your wallet is currently locked. For security purposes you'll need to enter your passphrase in order to search Syscoin Offers.</font> <a href=\"http://lockedwallet.syscoin.org\">more info</a>"));
		ui->labelExplanation->setTextFormat(Qt::RichText);
		ui->labelExplanation->setTextInteractionFlags(Qt::TextBrowserInteraction);
		ui->labelExplanation->setOpenExternalLinks(true);
    }*/
}
void OfferListPage::setModel(WalletModel* walletModel, OfferTableModel *model)
{
    this->model = model;
	this->walletModel = walletModel;
    if(!model) return;

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterRole(OfferTableModel::TypeRole);
    proxyModel->setFilterFixedString(OfferTableModel::Offer);
    ui->tableView->setModel(proxyModel);
    ui->tableView->sortByColumn(0, Qt::AscendingOrder);

    // Set column widths
    ui->tableView->setColumnWidth(0, 75); //offer
    ui->tableView->setColumnWidth(1, 75); //cert
    ui->tableView->setColumnWidth(2, 250); //title
    ui->tableView->setColumnWidth(3, 300); //description
    ui->tableView->setColumnWidth(4, 75); //category
    ui->tableView->setColumnWidth(5, 50); //price
    ui->tableView->setColumnWidth(6, 75); //currency
    ui->tableView->setColumnWidth(7, 75); //qty
    ui->tableView->setColumnWidth(8, 50); //status
    ui->tableView->setColumnWidth(9, 75); //exclusive resell
    ui->tableView->setColumnWidth(10, 50); //private
    ui->tableView->setColumnWidth(11, 100); //seller alias
    ui->tableView->setColumnWidth(12, 0); //btc only

    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged()));


    // Select row for newly created offer
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewOffer(QModelIndex,int,int)));

    selectionChanged();

}

void OfferListPage::setOptionsModel(OptionsModel *optionsModel)
{
    this->optionsModel = optionsModel;
}

void OfferListPage::on_copyOffer_clicked()
{
   
    GUIUtil::copyEntryData(ui->tableView, OfferTableModel::Name);
}
void OfferListPage::on_messageButton_clicked()
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
	QString offerAlias = selection.at(0).data(OfferTableModel::AliasRole).toString();
	QString offerTitle = selection.at(0).data(OfferTableModel::TitleRole).toString();
	// send message to seller
	NewMessageDialog dlg(NewMessageDialog::NewMessage, offerAlias, offerTitle);   
	dlg.exec();


}
void OfferListPage::on_resellButton_clicked()
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
	unsigned int qtyRemain = selection.at(0).data(OfferTableModel::QtyRole).toUInt();
	if(qtyRemain <= 0 && selection.at(0).data(OfferTableModel::QtyRole).toString() != tr("unlimited"))
	{
        QMessageBox::critical(this, windowTitle(),
        tr("Sorry, you cannot not resell this offer, it is sold out!"),
            QMessageBox::Ok, QMessageBox::Ok);
		return;
	}
    ResellOfferDialog dlg((QModelIndex*)&selection.at(0));   
    dlg.exec();
}
void OfferListPage::on_purchaseButton_clicked()
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
	unsigned int qtyRemain = selection.at(0).data(OfferTableModel::QtyRole).toUInt();
	if(qtyRemain <= 0 && selection.at(0).data(OfferTableModel::QtyRole).toString() != tr("unlimited"))
	{
        QMessageBox::critical(this, windowTitle(),
        tr("Sorry, you cannot not purchase this offer, it is sold out!"),
            QMessageBox::Ok, QMessageBox::Ok);
		return;
	}
	QString offerGUID = selection.at(0).data(OfferTableModel::NameRole).toString();
	QString URI = QString("syscoin://") + offerGUID;
    SendCoinsRecipient recipient;
    if (GUIUtil::parseSyscoinURI(URI, &recipient))
		offerView->handlePaymentRequest(&recipient);
}
void OfferListPage::onCopyOfferDescriptionAction()
{
    GUIUtil::copyEntryData(ui->tableView, OfferTableModel::Description);
}


void OfferListPage::onCopyOfferValueAction()
{
    GUIUtil::copyEntryData(ui->tableView, OfferTableModel::Title);
}


void OfferListPage::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->copyOffer->setEnabled(true);
		ui->resellButton->setEnabled(true);
		ui->purchaseButton->setEnabled(true);
		ui->messageButton->setEnabled(true);	
    }
    else
    {
        ui->copyOffer->setEnabled(false);
		ui->resellButton->setEnabled(false);
		ui->purchaseButton->setEnabled(false);
		ui->messageButton->setEnabled(false);
    }
}
void OfferListPage::keyPressEvent(QKeyEvent * event)
{
  if( event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter )
  {
	on_searchOffer_clicked();
    event->accept();
  }
  else
    QDialog::keyPressEvent( event );
}
void OfferListPage::on_exportButton_clicked()
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
    writer.addColumn("Offer", OfferTableModel::Name, Qt::EditRole);
	writer.addColumn("Cert", OfferTableModel::Cert, Qt::EditRole);
    writer.addColumn("Title", OfferTableModel::Title, Qt::EditRole);
	writer.addColumn("Description", OfferTableModel::Title, Qt::EditRole);
	writer.addColumn("Category", OfferTableModel::Category, Qt::EditRole);
	writer.addColumn("Price", OfferTableModel::Price, Qt::EditRole);
	writer.addColumn("Currency", OfferTableModel::Currency, Qt::EditRole);
	writer.addColumn("Qty", OfferTableModel::Qty, Qt::EditRole);
	writer.addColumn("Exclusive Resell", OfferTableModel::ExclusiveResell, Qt::EditRole);
	writer.addColumn("Private", OfferTableModel::Private, Qt::EditRole);
	writer.addColumn("Expired", OfferTableModel::Expired, Qt::EditRole);
    if(!writer.write())
    {
        QMessageBox::critical(this, tr("Error exporting"), tr("Could not write to file %1.").arg(filename),
                              QMessageBox::Abort, QMessageBox::Abort);
    }
}



void OfferListPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid()) {
        contextMenu->exec(QCursor::pos());
    }
}

void OfferListPage::selectNewOffer(const QModelIndex &parent, int begin, int /*end*/)
{
    QModelIndex idx = proxyModel->mapFromSource(model->index(begin, OfferTableModel::Name, parent));
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() == newOfferToSelect))
    {
        // Select row of newly created offer, once
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newOfferToSelect.clear();
    }
}

void OfferListPage::on_searchOffer_clicked()
{
    if(!walletModel) 
	{
		selectionChanged();
		return;
	}
        UniValue params(UniValue::VARR);
        UniValue valError;
        UniValue valResult;
        UniValue valId;
        UniValue result ;
        string strReply;
        string strError;
        string strMethod = string("offerfilter");
		string name_str;
		string cert_str;
		string value_str;
		string desc_str;
		string category_str;
		string price_str;
		string currency_str;
		string qty_str;
		string expired_str;
		string exclusive_resell_str;
		string private_str;
		string acceptBTCOnly_str;
		string alias_peg_str;
		string alias_str;
		int expired = 0;
        params.push_back(ui->lineEditOfferSearch->text().toStdString());
        params.push_back(GetOfferExpirationDepth());
		UniValue num;
		num.setInt(0);
		params.push_back(num);
		params.push_back(ui->comboBox->currentText().toInt());
        try {
            result = tableRPC.execute(strMethod, params);
        }
        catch (UniValue& objError)
        {
            strError = find_value(objError, "message").get_str();
            QMessageBox::critical(this, windowTitle(),
            tr("Error searching Offer: \"%1\"").arg(QString::fromStdString(strError)),
                QMessageBox::Ok, QMessageBox::Ok);
			selectionChanged();
            return;
        }
        catch(std::exception& e)
        {
            QMessageBox::critical(this, windowTitle(),
                tr("General exception when searching offer"),
                QMessageBox::Ok, QMessageBox::Ok);
			selectionChanged();
            return;
        }
		if (result.type() == UniValue::VARR)
			{
				this->model->clear();
			
			  const UniValue &arr = result.get_array();
		      for (unsigned int idx = 0; idx < arr.size(); idx++) {
			    const UniValue& input = arr[idx];
				if (input.type() != UniValue::VOBJ)
					continue;
				const UniValue& o = input.get_obj();
				name_str = "";
				cert_str = "";
				value_str = "";
				desc_str = "";
				private_str = "";
				exclusive_resell_str = "";
				alias_str = "";
				acceptBTCOnly_str = "";
				alias_peg_str = "";
				expired = 0;


				const UniValue& name_value = find_value(o, "offer");
				if (name_value.type() == UniValue::VSTR)
					name_str = name_value.get_str();
				const UniValue& cert_value = find_value(o, "cert");
				if (cert_value.type() == UniValue::VSTR)
					cert_str = cert_value.get_str();
				const UniValue& value_value = find_value(o, "title");
				if (value_value.type() == UniValue::VSTR)
					value_str = value_value.get_str();
				const UniValue& desc_value = find_value(o, "description");
				if (desc_value.type() == UniValue::VSTR)
					desc_str = desc_value.get_str();
				const UniValue& category_value = find_value(o, "category");
				if (category_value.type() == UniValue::VSTR)
					category_str = category_value.get_str();
				const UniValue& price_value = find_value(o, "price");
				if (price_value.type() == UniValue::VSTR)
					price_str = price_value.get_str();
				const UniValue& currency_value = find_value(o, "currency");
				if (currency_value.type() == UniValue::VSTR)
					currency_str = currency_value.get_str();
				const UniValue& qty_value = find_value(o, "quantity");
				if (qty_value.type() == UniValue::VSTR)
					qty_str = qty_value.get_str();
				const UniValue& exclusive_resell_value = find_value(o, "exclusive_resell");
				if (exclusive_resell_value.type() == UniValue::VSTR)
					exclusive_resell_str = exclusive_resell_value.get_str();		
				const UniValue& private_value = find_value(o, "private");
				if (private_value.type() == UniValue::VSTR)
					private_str = private_value.get_str();	
				const UniValue& alias_value = find_value(o, "alias");
				if (alias_value.type() == UniValue::VSTR)
					alias_str = alias_value.get_str();
				const UniValue& btconly_value = find_value(o, "btconly");
				if (btconly_value.type() == UniValue::VSTR)
					acceptBTCOnly_str = btconly_value.get_str();
				const UniValue& alias_peg_value = find_value(o, "alias_peg");
				if (alias_peg_value.type() == UniValue::VSTR)
					alias_peg_str = alias_peg_value.get_str();
				const UniValue& expired_value = find_value(o, "expired");
				if (expired_value.type() == UniValue::VNUM)
					expired = expired_value.get_int();

				if(expired == 1)
				{
					expired_str = "Expired";
				}
				else
				{
					expired_str = "Valid";
				}

				
				
				model->addRow(OfferTableModel::Offer,
						QString::fromStdString(name_str),
						QString::fromStdString(cert_str),
						QString::fromStdString(value_str),
						QString::fromStdString(desc_str),
						QString::fromStdString(category_str),
						QString::fromStdString(price_str),
						QString::fromStdString(currency_str),
						QString::fromStdString(qty_str),
						QString::fromStdString(expired_str), 
						QString::fromStdString(exclusive_resell_str),
						QString::fromStdString(private_str),
						QString::fromStdString(alias_str),
						QString::fromStdString(acceptBTCOnly_str),
						QString::fromStdString(alias_peg_str));
					this->model->updateEntry(QString::fromStdString(name_str),
						QString::fromStdString(cert_str),
						QString::fromStdString(value_str),
						QString::fromStdString(desc_str),
						QString::fromStdString(category_str),
						QString::fromStdString(price_str),
						QString::fromStdString(currency_str),
						QString::fromStdString(qty_str),
						QString::fromStdString(expired_str),
						QString::fromStdString(exclusive_resell_str),
						QString::fromStdString(private_str), 
						QString::fromStdString(alias_str), 
						QString::fromStdString(acceptBTCOnly_str),
						QString::fromStdString(alias_peg_str), AllOffer, CT_NEW);	
			  }

            
         }   
        else
        {
            QMessageBox::critical(this, windowTitle(),
                tr("Error: Invalid response from offerfilter command"),
                QMessageBox::Ok, QMessageBox::Ok);
			selectionChanged();
            return;
        }
		selectionChanged();

}
