/*
 * Syscoin Developers 2015
 */
#include "acceptedofferlistpage.h"
#include "ui_acceptedofferlistpage.h"
#include "offeraccepttablemodel.h"
#include "offeracceptinfodialog.h"
#include "newmessagedialog.h"
#include "offerfeedbackdialog.h"
#include "clientmodel.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "syscoingui.h"
#include "csvmodelwriter.h"
#include "guiutil.h"
#include "platformstyle.h"
#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMessageBox>
#include <QSettings>
#include <QMenu>
AcceptedOfferListPage::AcceptedOfferListPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AcceptedOfferListPage),
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
		ui->feedbackButton->setIcon(QIcon());
	}
	else
	{
		ui->exportButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/export"));
		ui->messageButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/outmail"));
		ui->detailButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/details"));
		ui->copyOffer->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/editcopy"));
		ui->refreshButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/refresh"));
		ui->feedbackButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/thumbsup"));
	}
#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->copyOffer->setIcon(QIcon());
    ui->exportButton->setIcon(QIcon());
	ui->refreshButton->setIcon(QIcon());
	ui->messageButton->setIcon(QIcon());
	ui->detailButton->setIcon(QIcon());
#endif


    ui->labelExplanation->setText(tr("These are offers you have purchased. Offer operations take 2-5 minutes to become active. Right click on an offer to view more info such as the message you sent to the seller, quantity, date, etc.  You can choose which aliases to view sales information for using the dropdown to the right."));
	
	connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(on_detailButton_clicked()));
    // Context menu actions
    QAction *copyOfferAction = new QAction(ui->copyOffer->text(), this);
    QAction *copyOfferValueAction = new QAction(tr("Copy OfferAccept ID"), this);
	QAction *detailsAction = new QAction(tr("Details"), this);
	QAction *messageAction = new QAction(tr("Message Merchant"), this);
	QAction *feedbackAction = new QAction(tr("Leave Feedback For Merchant"), this);

    // Build context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyOfferAction);
    contextMenu->addAction(copyOfferValueAction);
	contextMenu->addSeparator();
	contextMenu->addAction(detailsAction);
	contextMenu->addAction(messageAction);
	contextMenu->addAction(feedbackAction);
    // Connect signals for context menu actions
    connect(copyOfferAction, SIGNAL(triggered()), this, SLOT(on_copyOffer_clicked()));
    connect(copyOfferValueAction, SIGNAL(triggered()), this, SLOT(onCopyOfferValueAction()));
	connect(detailsAction, SIGNAL(triggered()), this, SLOT(on_detailButton_clicked()));
	connect(messageAction, SIGNAL(triggered()), this, SLOT(on_messageButton_clicked()));
	connect(feedbackAction, SIGNAL(triggered()), this, SLOT(on_feedbackButton_clicked()));

    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));
	connect(ui->displayListAlias,SIGNAL(currentIndexChanged(const QString&)),this,SLOT(displayListChanged(const QString&)));
	loadAliasList();

}
void AcceptedOfferListPage::loadAliasList()
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
void AcceptedOfferListPage::displayListChanged(const QString& alias)
{
	QSettings settings;
	settings.setValue("defaultListAlias", alias);
	settings.sync();
}
AcceptedOfferListPage::~AcceptedOfferListPage()
{
    delete ui;
}
void AcceptedOfferListPage::on_detailButton_clicked()
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
void AcceptedOfferListPage::showEvent ( QShowEvent * event )
{
    if(!walletModel) return;
}
void AcceptedOfferListPage::setModel(WalletModel *walletModel, OfferAcceptTableModel *model)
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

void AcceptedOfferListPage::setOptionsModel(ClientModel* clientmodel, OptionsModel *optionsModel)
{
    this->optionsModel = optionsModel;
	this->clientModel = clientmodel;
}
void AcceptedOfferListPage::on_messageButton_clicked()
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
	QString offerAlias = selection.at(0).data(OfferAcceptTableModel::AliasRole).toString();
	// send message to seller
	NewMessageDialog dlg(NewMessageDialog::NewMessage, offerAlias);   
	dlg.exec();


}
void AcceptedOfferListPage::on_feedbackButton_clicked()
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
void AcceptedOfferListPage::on_copyOffer_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, OfferAcceptTableModel::Name);
}

void AcceptedOfferListPage::onCopyOfferValueAction()
{
    GUIUtil::copyEntryData(ui->tableView, OfferAcceptTableModel::GUID);
}


void AcceptedOfferListPage::on_refreshButton_clicked()
{
    if(!model)
        return;
	loadAliasList();
    model->refreshOfferTable();
}

void AcceptedOfferListPage::selectionChanged()
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

void AcceptedOfferListPage::done(int retval)
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

void AcceptedOfferListPage::on_exportButton_clicked()
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



void AcceptedOfferListPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid()) {
        contextMenu->exec(QCursor::pos());
    }
}

void AcceptedOfferListPage::selectNewOffer(const QModelIndex &parent, int begin, int /*end*/)
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
