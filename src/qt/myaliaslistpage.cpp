/*
 * Syscoin Developers 2014
 */
#include "myaliaslistpage.h"
#include "ui_myaliaslistpage.h"
#include "aliastablemodel.h"
#include "clientmodel.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "walletmodel.h"
#include "syscoingui.h"
#include "editaliasdialog.h"
#include "mywhitelistofferdialog.h"
#include "csvmodelwriter.h"
#include "guiutil.h"

#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMessageBox>
#include <QMenu>
#include "rpcserver.h"
using namespace std;
extern const CRPCTable tableRPC;
MyAliasListPage::MyAliasListPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MyAliasListPage),
    model(0),
    optionsModel(0),
	platformStyle(platformStyle)
{
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();  
	if (!platformStyle->getImagesOnButtons())
	{
		ui->exportButton->setIcon(QIcon());
		ui->newAlias->setIcon(QIcon());
		ui->transferButton->setIcon(QIcon());
		ui->editButton->setIcon(QIcon());
		ui->copyAlias->setIcon(QIcon());
		ui->refreshButton->setIcon(QIcon());
		ui->newPubKey->setIcon(QIcon());
		ui->whitelistButton->setIcon(QIcon());

	}
	else
	{
		ui->exportButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/export"));
		ui->newAlias->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/add"));
		ui->transferButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/alias"));
		ui->editButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/editsys"));
		ui->copyAlias->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/editcopy"));
		ui->refreshButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/refresh"));
		ui->newPubKey->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/add"));
		ui->whitelistButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/address-book"));
		
	}

	ui->buttonBox->setVisible(false);

    ui->labelExplanation->setText(tr("These are your registered Syscoin Aliases. Alias operations (create, update, transfer) take 2-5 minutes to become active."));
	
	
    // Context menu actions
    QAction *copyAliasAction = new QAction(ui->copyAlias->text(), this);
    QAction *copyAliasValueAction = new QAction(tr("Copy Va&lue"), this);
    QAction *editAction = new QAction(tr("&Edit"), this);
    QAction *transferAliasAction = new QAction(tr("&Transfer"), this);

    // Build context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyAliasAction);
    contextMenu->addAction(copyAliasValueAction);
    contextMenu->addAction(editAction);
    contextMenu->addSeparator();
    contextMenu->addAction(transferAliasAction);

    // Connect signals for context menu actions
    connect(copyAliasAction, SIGNAL(triggered()), this, SLOT(on_copyAlias_clicked()));
    connect(copyAliasValueAction, SIGNAL(triggered()), this, SLOT(onCopyAliasValueAction()));
    connect(editAction, SIGNAL(triggered()), this, SLOT(on_editButton_clicked()));
    connect(transferAliasAction, SIGNAL(triggered()), this, SLOT(on_transferButton_clicked()));

	connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(on_editButton_clicked()));
    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

    // Pass through accept action from button box
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
}

MyAliasListPage::~MyAliasListPage()
{
    delete ui;
}
void MyAliasListPage::showEvent ( QShowEvent * event )
{
    if(!walletModel) return;
    /*if(walletModel->getEncryptionStatus() == WalletModel::Locked)
	{
        ui->labelExplanation->setText(tr("<font color='blue'>WARNING: Your wallet is currently locked. For security purposes you'll need to enter your passphrase in order to interact with Syscoin Aliases. Because your wallet is locked you must manually refresh this table after creating or updating an Alias. </font> <a href=\"http://lockedwallet.syscoin.org\">more info</a><br><br>These are your registered Syscoin Aliases. Alias updates take 1 confirmation to appear in this table."));
		ui->labelExplanation->setTextFormat(Qt::RichText);
		ui->labelExplanation->setTextInteractionFlags(Qt::TextBrowserInteraction);
		ui->labelExplanation->setOpenExternalLinks(true);
    }*/
}
void MyAliasListPage::setModel(WalletModel *walletModel, AliasTableModel *model)
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
    proxyModel->setFilterRole(AliasTableModel::TypeRole);
    proxyModel->setFilterFixedString(AliasTableModel::Alias);

    ui->tableView->setModel(proxyModel);
    ui->tableView->sortByColumn(0, Qt::AscendingOrder);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);

    // Set column widths
    ui->tableView->setColumnWidth(0, 500); //alias name
    ui->tableView->setColumnWidth(1, 500); //alias value
    ui->tableView->setColumnWidth(2, 75); //expires on
    ui->tableView->setColumnWidth(3, 75); //expires in
    ui->tableView->setColumnWidth(4, 75); //expired status

    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged()));

    // Select row for newly created alias
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewAlias(QModelIndex,int,int)));

    selectionChanged();
}

void MyAliasListPage::setOptionsModel(ClientModel* clientmodel, OptionsModel *optionsModel)
{
    this->optionsModel = optionsModel;
	this->clientModel = clientmodel;
}

void MyAliasListPage::on_copyAlias_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, AliasTableModel::Name);
}

void MyAliasListPage::onCopyAliasValueAction()
{
    GUIUtil::copyEntryData(ui->tableView, AliasTableModel::Value);
}

void MyAliasListPage::on_editButton_clicked()
{
    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;

    EditAliasDialog dlg(EditAliasDialog::EditAlias);
    dlg.setModel(walletModel, model);
    QModelIndex origIndex = proxyModel->mapToSource(indexes.at(0));
    dlg.loadRow(origIndex.row());
    dlg.exec();
}

void MyAliasListPage::on_transferButton_clicked()
{
    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;

    EditAliasDialog dlg(EditAliasDialog::TransferAlias);
    dlg.setModel(walletModel, model);
    QModelIndex origIndex = proxyModel->mapToSource(indexes.at(0));
    dlg.loadRow(origIndex.row());
    dlg.exec();
}
void MyAliasListPage::on_refreshButton_clicked()
{
    if(!model)
        return;
    model->refreshAliasTable();
}
void MyAliasListPage::on_whitelistButton_clicked()
{
    MyWhitelistOfferDialog dlg(platformStyle);
	dlg.setModel(walletModel);
    dlg.exec();    
}
void MyAliasListPage::on_newAlias_clicked()
{
    if(!model)
        return;

    EditAliasDialog dlg(EditAliasDialog::NewAlias);
    dlg.setModel(walletModel,model);
    if(dlg.exec())
    {
        newAliasToSelect = dlg.getAlias();
    }
}
void MyAliasListPage::on_newPubKey_clicked()
{
	UniValue params;
	UniValue result = tableRPC.execute("generatepublickey", params);
	if (result.type() == UniValue::VARR)
	{
		const UniValue &resultArray = result.get_array();
		const QString  &resQStr = QString::fromStdString(resultArray[0].get_str());
		QApplication::clipboard()->setText(resQStr, QClipboard::Clipboard);
		QApplication::clipboard()->setText(resQStr, QClipboard::Selection);
		QMessageBox::information(this, tr("New Public Key For Alias Transfer"),
			resQStr + tr(" has been copied to your clipboard! IMPORTANT: This key is for one-time use only! Do not re-use public keys for multiple aliases or transfers."),
			QMessageBox::Ok, QMessageBox::Ok);
		
	}
	else
	 	QMessageBox::critical(this, tr("New Public Key For Alias Transfer"),
			tr("Could not generate a new public key!"),
			QMessageBox::Ok, QMessageBox::Ok);
				
}
void MyAliasListPage::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->copyAlias->setEnabled(true);
		ui->transferButton->setEnabled(true);
		ui->editButton->setEnabled(true);
    }
    else
    {
        ui->copyAlias->setEnabled(false);
		ui->transferButton->setEnabled(false);
		ui->editButton->setEnabled(false);
    }
}

void MyAliasListPage::done(int retval)
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel() || !table->model())
        return;

    // Figure out which alias was selected, and return it
    QModelIndexList indexes = table->selectionModel()->selectedRows(AliasTableModel::Name);
    Q_FOREACH (const QModelIndex& index, indexes)
    {
        QVariant alias = table->model()->data(index);
        returnValue = alias.toString();
    }

    if(returnValue.isEmpty())
    {
        // If no alias entry selected, return rejected
        retval = Rejected;
    }

    QDialog::done(retval);
}

void MyAliasListPage::on_exportButton_clicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(
            this,
            tr("Export Alias Data"), QString(),
            tr("Comma separated file (*.csv)"), NULL);

    if (filename.isNull()) return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(proxyModel);
    writer.addColumn("Alias", AliasTableModel::Name, Qt::EditRole);
    writer.addColumn("Value", AliasTableModel::Value, Qt::EditRole);
	writer.addColumn("Expires On", AliasTableModel::ExpiresOn, Qt::EditRole);
	writer.addColumn("Expires In", AliasTableModel::ExpiresIn, Qt::EditRole);
	writer.addColumn("Expired", AliasTableModel::Expired, Qt::EditRole);
    if(!writer.write())
    {
        QMessageBox::critical(this, tr("Error exporting"), tr("Could not write to file %1.").arg(filename),
                              QMessageBox::Abort, QMessageBox::Abort);
    }
}



void MyAliasListPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid()) {
        contextMenu->exec(QCursor::pos());
    }
}

void MyAliasListPage::selectNewAlias(const QModelIndex &parent, int begin, int /*end*/)
{
    QModelIndex idx = proxyModel->mapFromSource(model->index(begin, AliasTableModel::Name, parent));
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() == newAliasToSelect))
    {
        // Select row of newly created alias, once
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newAliasToSelect.clear();
    }
}
