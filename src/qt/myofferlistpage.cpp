/*
 * Syscoin Developers 2015
 */
#include "myofferlistpage.h"
#include "ui_myofferlistpage.h"
#include "offertablemodel.h"
#include "offerwhitelisttablemodel.h"
#include "editwhitelistofferdialog.h"
#include "clientmodel.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "walletmodel.h"
#include "syscoingui.h"
#include "editofferdialog.h"
#include "csvmodelwriter.h"
#include "guiutil.h"


#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMessageBox>
#include <QSettings>
#include <QMenu>
MyOfferListPage::MyOfferListPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MyOfferListPage),
    model(0),
    optionsModel(0),
	platformStyle(platformStyle)
{
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();  
	if (!platformStyle->getImagesOnButtons())
	{
		ui->exportButton->setIcon(QIcon());
		ui->newOffer->setIcon(QIcon());
		ui->whitelistButton->setIcon(QIcon());
		ui->editButton->setIcon(QIcon());
		ui->copyOffer->setIcon(QIcon());
		ui->refreshButton->setIcon(QIcon());

	}
	else
	{
		ui->exportButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/export"));
		ui->newOffer->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/add"));
		ui->whitelistButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/address-book"));
		ui->editButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/editsys"));
		ui->copyOffer->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/editcopy"));
		ui->refreshButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/refresh"));
		
	}

    ui->labelExplanation->setText(tr("These are your registered Syscoin Offers. Offer operations (create, update) take 2-5 minutes to become active. You can choose which aliases to view related offers using the dropdown to the right."));
	
	
    // Context menu actions
    QAction *copyOfferAction = new QAction(ui->copyOffer->text(), this);
    QAction *copyOfferValueAction = new QAction(tr("Copy Title"), this);
	QAction *copyOfferDescriptionAction = new QAction(tr("Copy Description"), this);
    QAction *editAction = new QAction(tr("Edit"), this);
	QAction *editWhitelistAction = new QAction(tr("Manage Affiliates"), this);
    // Build context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyOfferAction);
    contextMenu->addAction(copyOfferValueAction);
	contextMenu->addAction(copyOfferDescriptionAction);
    contextMenu->addSeparator();
	contextMenu->addAction(editAction);
	contextMenu->addAction(editWhitelistAction);

    // Connect signals for context menu actions
    connect(copyOfferAction, SIGNAL(triggered()), this, SLOT(on_copyOffer_clicked()));
    connect(copyOfferValueAction, SIGNAL(triggered()), this, SLOT(onCopyOfferValueAction()));
	connect(copyOfferDescriptionAction, SIGNAL(triggered()), this, SLOT(onCopyOfferDescriptionAction()));
    connect(editAction, SIGNAL(triggered()), this, SLOT(on_editButton_clicked()));
	connect(editWhitelistAction, SIGNAL(triggered()), this, SLOT(onEditWhitelistAction()));
	connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(on_editButton_clicked()));
    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));
	connect(ui->soldOutCheck,SIGNAL(clicked(bool)),SLOT(onToggleShowSoldOut(bool)));
	connect(ui->showDigitalOffers,SIGNAL(clicked(bool)),SLOT(onToggleShowDigitalOffers(bool)));


	offerWhitelistTableModel = 0;
	connect(ui->displayListAlias,SIGNAL(currentIndexChanged(const QString&)),this,SLOT(displayListChanged(const QString&)));
	loadAliasList();

}
void MyOfferListPage::loadAliasList()
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
void MyOfferListPage::displayListChanged(const QString& alias)
{
	QSettings settings;
	settings.setValue("defaultListAlias", alias);
	settings.sync();
}
MyOfferListPage::~MyOfferListPage()
{
    delete ui;
}
void MyOfferListPage::onToggleShowSoldOut(bool toggled)
{
	if(!model)
		return;
	model->filterOffers(ui->soldOutCheck->isChecked(), ui->showDigitalOffers->isChecked());
}
void MyOfferListPage::onToggleShowDigitalOffers(bool toggled)
{
	if(!model)
		return;
	model->filterOffers(ui->soldOutCheck->isChecked(), ui->showDigitalOffers->isChecked());
}
void MyOfferListPage::showEvent ( QShowEvent * event )
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
void MyOfferListPage::setModel(WalletModel *walletModel, OfferTableModel *model)
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
		proxyModel->setFilterRole(OfferTableModel::TypeRole);
		proxyModel->setFilterFixedString(OfferTableModel::Offer);
       
    
		ui->tableView->setModel(proxyModel);
        ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);

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
	if(offerWhitelistTableModel)
		delete offerWhitelistTableModel;
	offerWhitelistTableModel = new OfferWhitelistTableModel(walletModel);
}

void MyOfferListPage::setOptionsModel(ClientModel* clientmodel, OptionsModel *optionsModel)
{
    this->optionsModel = optionsModel;
	this->clientModel = clientmodel;
}

void MyOfferListPage::on_copyOffer_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, OfferTableModel::Name);
}
void MyOfferListPage::onCopyOfferDescriptionAction()
{
    GUIUtil::copyEntryData(ui->tableView, OfferTableModel::Description);
}


void MyOfferListPage::onCopyOfferValueAction()
{
    GUIUtil::copyEntryData(ui->tableView, OfferTableModel::Title);
}

void MyOfferListPage::on_editButton_clicked()
{
    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;
	QString offerGUID = indexes.at(0).data(OfferTableModel::NameRole).toString();
	QString certGUID = indexes.at(0).data(OfferTableModel::CertRole).toString();
	QString status = indexes.at(0).data(OfferTableModel::ExpiredRole).toString();
	if(status == QString("expired"))
	{
           QMessageBox::information(this, windowTitle(),
           tr("You cannot edit this offer because it has expired"),
               QMessageBox::Ok, QMessageBox::Ok);
		   return;
	}
    EditOfferDialog dlg(EditOfferDialog::EditOffer, offerGUID, certGUID);
    dlg.setModel(walletModel, model);
    QModelIndex origIndex = proxyModel->mapToSource(indexes.at(0));
    dlg.loadRow(origIndex.row());
    dlg.exec();
}
void MyOfferListPage::on_whitelistButton_clicked()
{
    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;
	QModelIndex origIndex = proxyModel->mapToSource(indexes.at(0));
    EditWhitelistOfferDialog dlg(platformStyle, &origIndex);
	dlg.setModel(walletModel, offerWhitelistTableModel);
    dlg.exec();
}
void MyOfferListPage::onEditWhitelistAction()
{
    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;
	QModelIndex origIndex = proxyModel->mapToSource(indexes.at(0));
    EditWhitelistOfferDialog dlg(platformStyle, &origIndex);
	dlg.setModel(walletModel, offerWhitelistTableModel);
    dlg.exec();
}
void MyOfferListPage::on_refreshButton_clicked()
{
    if(!model)
        return;
	loadAliasList();
    model->refreshOfferTable();
}
void MyOfferListPage::on_newOffer_clicked()
{
    if(!model)
        return;

    EditOfferDialog dlg(EditOfferDialog::NewOffer, "");
    dlg.setModel(walletModel,model);
    if(dlg.exec())
    {
        newOfferToSelect = dlg.getOffer();
    }
}
void MyOfferListPage::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->copyOffer->setEnabled(true);
		ui->whitelistButton->setEnabled(true);
		ui->editButton->setEnabled(true);
    }
    else
    {
        ui->copyOffer->setEnabled(false);
		ui->whitelistButton->setEnabled(false);
		ui->editButton->setEnabled(false);
    }
}

void MyOfferListPage::done(int retval)
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel() || !table->model())
        return;

    // Figure out which offer was selected, and return it
    QModelIndexList indexes = table->selectionModel()->selectedRows(OfferTableModel::Name);

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

void MyOfferListPage::on_exportButton_clicked()
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
    writer.addColumn(tr("Offer"), OfferTableModel::Name, Qt::EditRole);
	writer.addColumn(tr("Cert"), OfferTableModel::Cert, Qt::EditRole);
    writer.addColumn(tr("Title"), OfferTableModel::Title, Qt::EditRole);
	writer.addColumn(tr("Description"), OfferTableModel::Description, Qt::EditRole);
	writer.addColumn(tr("Category"), OfferTableModel::Category, Qt::EditRole);
	writer.addColumn(tr("Price"), OfferTableModel::Price, Qt::EditRole);
	writer.addColumn(tr("Currency"), OfferTableModel::Currency, Qt::EditRole);
	writer.addColumn(tr("Qty"), OfferTableModel::Qty, Qt::EditRole);
	writer.addColumn(tr("Sold"), OfferTableModel::Sold, Qt::EditRole);
	writer.addColumn(tr("Private"), OfferTableModel::Private, Qt::EditRole);
	writer.addColumn(tr("Expired"), OfferTableModel::Expired, Qt::EditRole);
	writer.addColumn(tr("Seller Alias"), OfferTableModel::Alias, Qt::EditRole);
	writer.addColumn(tr("Seller Rating"), OfferTableModel::AliasRating, OfferTableModel::AliasRatingRole);
	writer.addColumn(tr("Payment Options"), OfferTableModel::PaymentOptions, Qt::EditRole);
    if(!writer.write())
    {
		QMessageBox::critical(this, tr("Error exporting"), tr("Could not write to file: ") + filename,
                              QMessageBox::Abort, QMessageBox::Abort);
    }
}



void MyOfferListPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid()) {
        contextMenu->exec(QCursor::pos());
    }
}

void MyOfferListPage::selectNewOffer(const QModelIndex &parent, int begin, int /*end*/)
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
