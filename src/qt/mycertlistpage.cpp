/*
 * Syscoin Developers 2015
 */
#include "mycertlistpage.h"
#include "ui_mycertlistpage.h"
#include "certtablemodel.h"
#include "clientmodel.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "platformstyle.h"
#include "syscoingui.h"
#include "editcertdialog.h"
#include "editofferdialog.h"
#include "csvmodelwriter.h"
#include "guiutil.h"

#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMessageBox>
#include <QSettings>
#include <QMenu>
MyCertListPage::MyCertListPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MyCertListPage),
    model(0),
    optionsModel(0)
{
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();  
	if (!platformStyle->getImagesOnButtons())
	{
		ui->exportButton->setIcon(QIcon());
		ui->newCert->setIcon(QIcon());
		ui->sellCertButton->setIcon(QIcon());
		ui->transferButton->setIcon(QIcon());
		ui->editButton->setIcon(QIcon());
		ui->copyCert->setIcon(QIcon());
		ui->refreshButton->setIcon(QIcon());

	}
	else
	{
		ui->exportButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/export"));
		ui->newCert->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/add"));
		ui->sellCertButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/cart"));
		ui->transferButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/cert"));
		ui->editButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/editsys"));
		ui->copyCert->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/editcopy"));
		ui->refreshButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/refresh"));
		
	}

    ui->labelExplanation->setText(tr("These are your registered Syscoin Certificates. Certificate operations (create, update, transfer) take 2-5 minutes to become active.  You can choose which aliases to view related certificates using the dropdown to the right."));
	
	
    // Context menu actions
    QAction *copyCertAction = new QAction(ui->copyCert->text(), this);
    QAction *copyCertValueAction = new QAction(tr("Copy Title"), this);
    QAction *editAction = new QAction(tr("Edit"), this);
    QAction *transferCertAction = new QAction(tr("Transfer"), this);
	QAction *sellCertAction = new QAction(tr("Sell"), this);

    // Build context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyCertAction);
    contextMenu->addAction(copyCertValueAction);
    contextMenu->addAction(editAction);
    contextMenu->addSeparator();
    contextMenu->addAction(transferCertAction);
	contextMenu->addAction(sellCertAction);

    // Connect signals for context menu actions
    connect(copyCertAction, SIGNAL(triggered()), this, SLOT(on_copyCert_clicked()));
    connect(copyCertValueAction, SIGNAL(triggered()), this, SLOT(onCopyCertValueAction()));
    connect(editAction, SIGNAL(triggered()), this, SLOT(on_editButton_clicked()));
    connect(transferCertAction, SIGNAL(triggered()), this, SLOT(on_transferButton_clicked()));
	connect(sellCertAction, SIGNAL(triggered()), this, SLOT(on_sellCertButton_clicked()));
	connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(on_editButton_clicked()));
    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

	connect(ui->displayListAlias,SIGNAL(currentIndexChanged(const QString&)),this,SLOT(displayListChanged(const QString&)));
	loadAliasList();

}
void MyCertListPage::loadAliasList()
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
void MyCertListPage::displayListChanged(const QString& alias)
{
	QSettings settings;
	settings.setValue("defaultListAlias", alias);
	settings.sync();
}
MyCertListPage::~MyCertListPage()
{
    delete ui;
}
void MyCertListPage::on_sellCertButton_clicked()
{
    if(!model || !walletModel)
        return;
    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;

	QString certGUID = indexes.at(0).data(CertTableModel::NameRole).toString();
	QString status = indexes.at(0).data(CertTableModel::ExpiredRole).toString();
	QString category = indexes.at(0).data(CertTableModel::CategoryRole).toString();
	QString alias = indexes.at(0).data(CertTableModel::AliasRole).toString();
	if(status == QString("expired"))
	{
           QMessageBox::information(this, windowTitle(),
           tr("You cannot sell this certificate because it has expired"),
               QMessageBox::Ok, QMessageBox::Ok);
		   return;
	}
    EditOfferDialog dlg(EditOfferDialog::NewCertOffer, "", certGUID, alias, category);
    dlg.setModel(walletModel,0);
    dlg.exec();
}
void MyCertListPage::showEvent ( QShowEvent * event )
{
    if(!walletModel) return;
    /*if(walletModel->getEncryptionStatus() == WalletModel::Locked)
	{
        ui->labelExplanation->setText(tr("<font color='blue'>WARNING: Your wallet is currently locked. For security purposes you'll need to enter your passphrase in order to interact with Syscoin Certs. Because your wallet is locked you must manually refresh this table after creating or updating an Cert. </font> <a href=\"http://lockedwallet.syscoin.org\">more info</a><br><br>These are your registered Syscoin Certs. Cert updates take 1 confirmation to appear in this table."));
		ui->labelExplanation->setTextFormat(Qt::RichText);
		ui->labelExplanation->setTextInteractionFlags(Qt::TextBrowserInteraction);
		ui->labelExplanation->setOpenExternalLinks(true);
    }*/
}
void MyCertListPage::setModel(WalletModel *walletModel, CertTableModel *model)
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
    proxyModel->setFilterRole(CertTableModel::TypeRole);
    proxyModel->setFilterFixedString(CertTableModel::Cert);


    ui->tableView->setModel(proxyModel);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);

    // Set column widths
    ui->tableView->setColumnWidth(0, 75); //cert
    ui->tableView->setColumnWidth(1, 150); //title
    ui->tableView->setColumnWidth(2, 100); //data
	ui->tableView->setColumnWidth(3, 200); //pubdata
	ui->tableView->setColumnWidth(4, 150); //category
    ui->tableView->setColumnWidth(5, 150); //expires on
    ui->tableView->setColumnWidth(6, 75); //cert state
    ui->tableView->setColumnWidth(7, 0); //owner

    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged()));

    // Select row for newly created cert
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewCert(QModelIndex,int,int)));

    selectionChanged();
}

void MyCertListPage::setOptionsModel(ClientModel* clientmodel, OptionsModel *optionsModel)
{
    this->optionsModel = optionsModel;
	this->clientModel = clientmodel;
}

void MyCertListPage::on_copyCert_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, CertTableModel::Name);
}

void MyCertListPage::onCopyCertValueAction()
{
    GUIUtil::copyEntryData(ui->tableView, CertTableModel::Title);
}

void MyCertListPage::on_editButton_clicked()
{
    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;
	QString status = indexes.at(0).data(CertTableModel::ExpiredRole).toString();
	if(status == QString("expired"))
	{
           QMessageBox::information(this, windowTitle(),
           tr("You cannot edit this certificate because it has expired"),
               QMessageBox::Ok, QMessageBox::Ok);
		   return;
	}
    EditCertDialog dlg(EditCertDialog::EditCert);
    dlg.setModel(walletModel, model);
    QModelIndex origIndex = proxyModel->mapToSource(indexes.at(0));
    dlg.loadRow(origIndex.row());
    dlg.exec();
}

void MyCertListPage::on_transferButton_clicked()
{
    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;
	QString status = indexes.at(0).data(CertTableModel::ExpiredRole).toString();
	if(status == QString("expired"))
	{
           QMessageBox::information(this, windowTitle(),
           tr("You cannot transfer this certificate because it has expired"),
               QMessageBox::Ok, QMessageBox::Ok);
		   return;
	}
    EditCertDialog dlg(EditCertDialog::TransferCert);
    dlg.setModel(walletModel, model);
    QModelIndex origIndex = proxyModel->mapToSource(indexes.at(0));
    dlg.loadRow(origIndex.row());
    dlg.exec();
}
void MyCertListPage::on_refreshButton_clicked()
{
    if(!model)
        return;
	loadAliasList();
    model->refreshCertTable();
}
void MyCertListPage::on_newCert_clicked()
{
    if(!model)
        return;

    EditCertDialog dlg(EditCertDialog::NewCert);
    dlg.setModel(walletModel,model);
    if(dlg.exec())
    {
        newCertToSelect = dlg.getCert();
    }
}
void MyCertListPage::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->copyCert->setEnabled(true);
		ui->sellCertButton->setEnabled(true);
		ui->editButton->setEnabled(true);
		ui->transferButton->setEnabled(true);
    }
    else
    {
        ui->copyCert->setEnabled(false);
		ui->sellCertButton->setEnabled(false);
		ui->editButton->setEnabled(false);
		ui->transferButton->setEnabled(false);
    }
}

void MyCertListPage::done(int retval)
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel() || !table->model())
        return;

    // Figure out which cert was selected, and return it
    QModelIndexList indexes = table->selectionModel()->selectedRows(CertTableModel::Name);

    Q_FOREACH (const QModelIndex& index, indexes)
    {
        QVariant cert = table->model()->data(index);
        returnValue = cert.toString();
    }

    if(returnValue.isEmpty())
    {
        // If no cert entry selected, return rejected
        retval = Rejected;
    }

    QDialog::done(retval);
}

void MyCertListPage::on_exportButton_clicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(
            this,
            tr("Export Certificate Data"), QString(),
            tr("Comma separated file (*.csv)"), NULL);

    if (filename.isNull()) return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(proxyModel);
    writer.addColumn(tr("Cert"), CertTableModel::Name, Qt::EditRole);
    writer.addColumn(tr("Title"), CertTableModel::Title, Qt::EditRole);
	writer.addColumn(tr("Private Data"), CertTableModel::Data, Qt::EditRole);
	writer.addColumn(tr("Public Data"), CertTableModel::PubData, Qt::EditRole);
	writer.addColumn(tr("Category"), CertTableModel::Category, Qt::EditRole);
	writer.addColumn(tr("Expires On"), CertTableModel::ExpiresOn, Qt::EditRole);
	writer.addColumn(tr("Status"), CertTableModel::Expired, Qt::EditRole);
	writer.addColumn(tr("Owner"), CertTableModel::Alias, Qt::EditRole);
    if(!writer.write())
    {
		QMessageBox::critical(this, tr("Error exporting"), tr("Could not write to file: ") + filename,
                              QMessageBox::Abort, QMessageBox::Abort);
    }
}



void MyCertListPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid()) {
        contextMenu->exec(QCursor::pos());
    }
}

void MyCertListPage::selectNewCert(const QModelIndex &parent, int begin, int /*end*/)
{
    QModelIndex idx = proxyModel->mapFromSource(model->index(begin, CertTableModel::Name, parent));
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() == newCertToSelect))
    {
        // Select row of newly created cert, once
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newCertToSelect.clear();
    }
}
