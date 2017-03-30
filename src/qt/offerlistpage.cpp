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
#include <QClipboard>
#include <QMessageBox>
#include <QKeyEvent>
#include <QMenu>
#include "main.h"
#include "rpc/server.h"
#include "qcomboboxdelegate.h"
#include <QSettings>
#include <QStandardItemModel>
#include <boost/algorithm/string.hpp>
#include "offer.h"
using namespace std;


extern CRPCTable tableRPC;
extern bool getCategoryList(vector<string>& categoryList);
OfferListPage::OfferListPage(const PlatformStyle *platformStyle, OfferView *parent) :
    QDialog(0),
    ui(new Ui::OfferListPage),
    model(0),
    optionsModel(0),
	currentPage(0),
	offerView(parent)
{
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();  
	if (!platformStyle->getImagesOnButtons())
	{
		ui->copyOffer->setIcon(QIcon());
		ui->resellButton->setIcon(QIcon());
		ui->purchaseButton->setIcon(QIcon());
		ui->messageButton->setIcon(QIcon());
		ui->searchOffer->setIcon(QIcon());
	}
	else
	{
		ui->copyOffer->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/editcopy"));
		ui->resellButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/cart"));
		ui->purchaseButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/send"));
		ui->messageButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/outmail"));
		ui->searchOffer->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/search"));
	}
	

    ui->labelExplanation->setText(tr("Search for Syscoin Offers (double click on one to purchase). Select Safe Search from wallet options if you wish to omit potentially offensive Offers(On by default)"));
	
    // Context menu actions
    QAction *copyOfferAction = new QAction(ui->copyOffer->text(), this);
    QAction *copyOfferValueAction = new QAction(tr("Copy Title"), this);
	QAction *copyOfferDescriptionAction = new QAction(tr("Copy Description"), this);
	QAction *resellAction = new QAction(tr("Resell"), this);
	QAction *purchaseAction = new QAction(tr("Purchase"), this);
	QAction *messageSellerAction = new QAction(tr("Message Seller"), this);

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

void OfferListPage::addParentItem( QStandardItemModel * model, const QString& text, const QVariant& data )
{
	QList<QStandardItem*> lst = model->findItems(text,Qt::MatchExactly);
	for(unsigned int i=0; i<lst.count(); ++i )
	{ 
		if(lst[i]->data(Qt::UserRole) == data)
			return;
	}
    QStandardItem* item = new QStandardItem( text );
	item->setData( data, Qt::UserRole );
    item->setData( "parent", Qt::AccessibleDescriptionRole );
    QFont font = item->font();
    font.setBold( true );
    item->setFont( font );
    model->appendRow( item );
}

void OfferListPage::addChildItem( QStandardItemModel * model, const QString& text, const QVariant& data )
{
	QList<QStandardItem*> lst = model->findItems(text,Qt::MatchExactly);
	for(unsigned int i=0; i<lst.count(); ++i )
	{ 
		if(lst[i]->data(Qt::UserRole) == data)
			return;
	}

    QStandardItem* item = new QStandardItem( text + QString( 4, QChar( ' ' ) ) );
    item->setData( data, Qt::UserRole );
    item->setData( "child", Qt::AccessibleDescriptionRole );
    model->appendRow( item );
}
void OfferListPage::loadCategories()
{
    QStandardItemModel * model = new QStandardItemModel;
	vector<string> categoryList;
	if(!getCategoryList(categoryList))
	{
		return;
	}
	addParentItem(model, tr("All Categories"), tr("All Categories"));
	for(unsigned int i = 0;i< categoryList.size(); i++)
	{
		vector<string> categories;
		boost::split(categories,categoryList[i],boost::is_any_of(">"));
		if(categories.size() > 0)
		{
			if(categories.size() <= 2)
			{
				for(unsigned int j = 0;j< categories.size(); j++)
				{
					boost::algorithm::trim(categories[j]);
					// only support 2 levels in qt GUI for categories
					if(j == 0)
					{
						addParentItem(model, QString::fromStdString(categories[0]), QVariant(QString::fromStdString(categories[0])));
					}
					else if(j == 1)
					{
						addChildItem(model, QString::fromStdString(categories[1]), QVariant(QString::fromStdString(categoryList[i])));
					}
				}
			}
		}
		else
		{
			addParentItem(model, QString::fromStdString(categoryList[i]), QVariant(QString::fromStdString(categoryList[i])));
		}
	}
    ui->categoryEdit->setModel(model);
    ui->categoryEdit->setItemDelegate(new ComboBoxDelegate);
}
void OfferListPage::showEvent ( QShowEvent * event )
{
    if(!walletModel) return;
    loadCategories();
}
void OfferListPage::setModel(WalletModel* walletModel, OfferTableModel *model)
{
    this->model = model;
	this->walletModel = walletModel;
    if(!model) return;

    ui->tableView->setModel(model);
	ui->tableView->setSortingEnabled(false);

    // Set column widths
    ui->tableView->setColumnWidth(0, 75); //offer
    ui->tableView->setColumnWidth(1, 75); //cert
    ui->tableView->setColumnWidth(2, 250); //title
    ui->tableView->setColumnWidth(3, 300); //description
    ui->tableView->setColumnWidth(4, 75); //category
    ui->tableView->setColumnWidth(5, 50); //price
    ui->tableView->setColumnWidth(6, 75); //currency
    ui->tableView->setColumnWidth(7, 75); //qty
	ui->tableView->setColumnWidth(8, 75); //sold
    ui->tableView->setColumnWidth(9, 50); //status
    ui->tableView->setColumnWidth(10, 50); //private
    ui->tableView->setColumnWidth(11, 100); //seller alias
	ui->tableView->setColumnWidth(12, 150); //seller rating
    ui->tableView->setColumnWidth(13, 0); //btc only
	

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
 	if(!model || !walletModel)	
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
    ResellOfferDialog dlg((QModelIndex*)&selection.at(0), walletModel);   
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

void OfferListPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid()) {
        contextMenu->exec(QCursor::pos());
    }
}

void OfferListPage::selectNewOffer(const QModelIndex &parent, int begin, int /*end*/)
{
    QModelIndex idx = model->index(begin, OfferTableModel::Name, parent);
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() == newOfferToSelect))
    {
        // Select row of newly created offer, once
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newOfferToSelect.clear();
    }
}
void OfferListPage::on_prevButton_clicked()
{
	if(pageMap.empty())
	{
		ui->nextButton->setEnabled(false);
		ui->prevButton->setEnabled(false);
		return;
	}
	currentPage--;
	const pair<string, string> &offerPair = pageMap[currentPage];
	on_searchOffer_clicked(offerPair.first);
}
void OfferListPage::on_nextButton_clicked()
{
	if(pageMap.empty())
	{
		ui->nextButton->setEnabled(false);
		ui->prevButton->setEnabled(false);
		return;
	}
	const pair<string, string> &offerPair = pageMap[currentPage];
	currentPage++;
	on_searchOffer_clicked(offerPair.second);
	ui->prevButton->setEnabled(true);
}
void OfferListPage::on_searchOffer_clicked(string GUID)
{
    if(!walletModel ) 
	{
		selectionChanged();
		return;
	}
	if(GUID == "")
	{
		ui->nextButton->setEnabled(false);
		ui->prevButton->setEnabled(false);
		pageMap.clear();
		currentPage = 0;
	}
	QSettings settings;
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
	string firstOffer = "";
	string lastOffer = "";
	string category_str;
	string price_str;
	string currency_str;
	string qty_str;
	string expired_str;
	string private_str;
	string paymentoptions_str;
	string alias_peg_str;
	string alias_str;
	string safesearch_str;
	string geolocation_str;
	string aliasRating_str;
	int sold = 0;

	int expired = 0;
	int index;
    params.push_back(ui->lineEditOfferSearch->text().toStdString());
	params.push_back(GUID);
	params.push_back(settings.value("safesearch", "").toString().toStdString());
	QVariant currentCategory = ui->categoryEdit->itemData(ui->categoryEdit->currentIndex(), Qt::UserRole);
	if(ui->categoryEdit->currentIndex() > 0 &&  currentCategory != QVariant::Invalid)
		params.push_back(currentCategory.toString().toStdString());
	else if(ui->categoryEdit->currentText() != tr("All Categories"))
		params.push_back(ui->categoryEdit->currentText().toStdString());
    try {
        result = tableRPC.execute(strMethod, params);
    }
    catch (UniValue& objError)
    {
        strError = find_value(objError, "message").get_str();
        QMessageBox::critical(this, windowTitle(),
        tr("Error searching Offer: ") + QString::fromStdString(strError),
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
		  if(arr.size() >= 25)
			  ui->nextButton->setEnabled(true);
		  else
			  ui->nextButton->setEnabled(false);
		  if(currentPage <= 0)
			  ui->prevButton->setEnabled(false);
		  else
			  ui->prevButton->setEnabled(true);


		  
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
			alias_str = "";
			paymentoptions_str = "";
			alias_peg_str = "";
			safesearch_str = "";
			aliasRating_str = "";
			geolocation_str = "";
			expired = 0;
			sold = 0;


			const UniValue& name_value = find_value(o, "offer");
			if (name_value.type() == UniValue::VSTR)
				name_str = name_value.get_str();
			if(firstOffer == "")
				firstOffer = name_str;
			lastOffer = name_str;
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
			const UniValue& sold_value = find_value(o, "offers_sold");
			if (sold_value.type() == UniValue::VNUM)
				sold = sold_value.get_int();	
			const UniValue& private_value = find_value(o, "private");
			if (private_value.type() == UniValue::VSTR)
				private_str = private_value.get_str();	
			const UniValue& alias_value = find_value(o, "alias");
			if (alias_value.type() == UniValue::VSTR)
				alias_str = alias_value.get_str();
			const UniValue& aliasRating_value = find_value(o, "alias_rating_display");
			if (aliasRating_value.type() == UniValue::VSTR)
				aliasRating_str = aliasRating_value.get_str();
			const UniValue& alias_peg_value = find_value(o, "alias_peg");
			if (alias_peg_value.type() == UniValue::VSTR)
				alias_peg_str = alias_peg_value.get_str();
			const UniValue& expired_value = find_value(o, "expired");
			if (expired_value.type() == UniValue::VNUM)
				expired = expired_value.get_int();
			const UniValue& safesearch_value = find_value(o, "safesearch");
			if (safesearch_value.type() == UniValue::VSTR)
				safesearch_str = safesearch_value.get_str();
			const UniValue& geolocation_value = find_value(o, "geolocation");
			if (geolocation_value.type() == UniValue::VSTR)
				geolocation_str = geolocation_value.get_str();
			const UniValue& paymentoptions_value = find_value(o, "paymentoptions_display");
			if (paymentoptions_value.type() == UniValue::VSTR)
				paymentoptions_str = paymentoptions_value.get_str();
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
					QString::number(sold),
					QString::fromStdString(expired_str), 
					QString::fromStdString(private_str),
					QString::fromStdString(alias_str),
					QString::fromStdString(aliasRating_str),
					QString::fromStdString(paymentoptions_str),
					QString::fromStdString(alias_peg_str),
					QString::fromStdString(safesearch_str),
					QString::fromStdString(geolocation_str));
				this->model->updateEntry(QString::fromStdString(name_str),
					QString::fromStdString(cert_str),
					QString::fromStdString(value_str),
					QString::fromStdString(desc_str),
					QString::fromStdString(category_str),
					QString::fromStdString(price_str),
					QString::fromStdString(currency_str),
					QString::fromStdString(qty_str),
					QString::number(sold),
					QString::fromStdString(expired_str),
					QString::fromStdString(private_str), 
					QString::fromStdString(alias_str), 
					QString::fromStdString(aliasRating_str),
					QString::fromStdString(paymentoptions_str),
					QString::fromStdString(alias_peg_str), 
					QString::fromStdString(safesearch_str),
					QString::fromStdString(geolocation_str), AllOffer, CT_NEW);	
		  }

		  pageMap[currentPage] = make_pair(firstOffer, lastOffer);  
		  ui->labelPage->setText(tr("Current Page: ") + QString("<b>%1</b>").arg(currentPage+1));
     }   
    else
    {
		ui->nextButton->setEnabled(false);
		ui->prevButton->setEnabled(false);
		pageMap.clear();
		currentPage = 0;
        QMessageBox::critical(this, windowTitle(),
            tr("Error: Invalid response from offerfilter command"),
            QMessageBox::Ok, QMessageBox::Ok);
		selectionChanged();
        return;
    }
	selectionChanged();

}
