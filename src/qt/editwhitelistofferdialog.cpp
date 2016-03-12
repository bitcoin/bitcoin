/*
 * Syscoin Developers 2015
 */
#include <boost/assign/list_of.hpp>
#include "editwhitelistofferdialog.h"
#include "ui_editwhitelistofferdialog.h"
#include "offertablemodel.h"
#include "offerwhitelisttablemodel.h"
#include "walletmodel.h"
#include "syscoingui.h"
#include "platformstyle.h"
#include "newwhitelistdialog.h"
#include "csvmodelwriter.h"
#include "guiutil.h"
#include "ui_interface.h"
#include <QTableView>
#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMessageBox>
#include <QModelIndex>
#include <QMenu>
#include <QItemSelection>
#include "rpcserver.h"
#include "tinyformat.h"

using namespace std;



extern const CRPCTable tableRPC;
EditWhitelistOfferDialog::EditWhitelistOfferDialog(const PlatformStyle *platformStyle, QModelIndex *idx, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditWhitelistOfferDialog),
    model(0)
{
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();  
	if (!platformStyle->getImagesOnButtons())
	{
		ui->exportButton->setIcon(QIcon());
		ui->exclusiveButton->setIcon(QIcon());
		ui->removeAllButton->setIcon(QIcon());
		ui->removeButton->setIcon(QIcon());
		ui->newEntry->setIcon(QIcon());
		ui->refreshButton->setIcon(QIcon());
	}
	else
	{
		ui->exportButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/export"));
		ui->exclusiveButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/key"));
		ui->removeAllButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/remove"));
		ui->removeButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/remove"));
		ui->newEntry->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/add"));
		ui->refreshButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/refresh"));
	}

	offerGUID = idx->data(OfferTableModel::NameRole).toString();
	exclusiveWhitelist = idx->data(OfferTableModel::ExclusiveWhitelistRole).toString();
	offerCategory = idx->data(OfferTableModel::CategoryRole).toString();
	offerTitle = idx->data(OfferTableModel::TitleRole).toString();
	offerQty = idx->data(OfferTableModel::QtyRole).toString();
	offerPrice = idx->data(OfferTableModel::PriceRole).toString();
	offerDescription = idx->data(OfferTableModel::DescriptionRole).toString();
	offerPrivate = idx->data(OfferTableModel::PrivateRole).toString();
	
	ui->buttonBox->setVisible(false);
	ui->removeAllButton->setEnabled(false);
    ui->labelExplanation->setText(tr("These are the affiliates for your offer. Affiliate operations take 2-5 minutes to become active. You may specify discount levels for each affiliate or control who may resell your offer if you are in Exclusive Resell Mode. If Exclusive Resell Mode is off anyone can resell your offers, although discounts will still be applied if they own an alias that you've added to your affiliate list. Click the button at the bottom of this dialog to toggle the exclusive mode."));
	
    // Context menu actions
    QAction *removeAction = new QAction(tr("&Remove"), this);
	QAction *copyAction = new QAction(tr("&Copy Alias"), this);
    // Build context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyAction);
	contextMenu->addSeparator();
	contextMenu->addAction(removeAction);

    connect(copyAction, SIGNAL(triggered()), this, SLOT(on_copy()));


    // Connect signals for context menu actions
    connect(removeAction, SIGNAL(triggered()), this, SLOT(on_removeButton_clicked()));

    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

    // Pass through accept action from button box
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	
}

EditWhitelistOfferDialog::~EditWhitelistOfferDialog()
{
    delete ui;
}

void EditWhitelistOfferDialog::setModel(WalletModel *walletModel, OfferWhitelistTableModel *model)
{
    this->model = model;
	this->walletModel = walletModel;
    if(!model) return;
    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

	ui->tableView->setModel(proxyModel);
	ui->tableView->sortByColumn(0, Qt::AscendingOrder);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    // Set column widths
#if QT_VERSION < 0x050000
    ui->tableView->horizontalHeader()->setResizeMode(OfferWhitelistTableModel::Alias, QHeaderView::Stretch);
	ui->tableView->horizontalHeader()->setResizeMode(OfferWhitelistTableModel::Expires, QHeaderView::ResizeToContents);
	ui->tableView->horizontalHeader()->setResizeMode(OfferWhitelistTableModel::Discount, QHeaderView::ResizeToContents);
#else
    ui->tableView->horizontalHeader()->setSectionResizeMode(OfferWhitelistTableModel::Alias, QHeaderView::Stretch);
	ui->tableView->horizontalHeader()->setSectionResizeMode(OfferWhitelistTableModel::Expires, QHeaderView::ResizeToContents);
	ui->tableView->horizontalHeader()->setSectionResizeMode(OfferWhitelistTableModel::Discount, QHeaderView::ResizeToContents);
#endif

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged()));

    // Select row for newly created offer
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewEntry(QModelIndex,int,int)));
    selectionChanged();
	model->clear();

}


void EditWhitelistOfferDialog::showEvent ( QShowEvent * event )
{
	on_refreshButton_clicked();
}
void EditWhitelistOfferDialog::on_copy()
{
    GUIUtil::copyEntryData(ui->tableView, OfferWhitelistTableModel::Alias);
}
void EditWhitelistOfferDialog::on_exclusiveButton_clicked()
{
	QString tmpExclusiveWhitelist = exclusiveWhitelist;
	string strError;
	string strMethod = string("offerupdate");
	UniValue params(UniValue::VARR);
	UniValue result;
	try {
		params.push_back("");
		params.push_back(offerGUID.toStdString());
		params.push_back(offerCategory.toStdString());
		params.push_back(offerTitle.toStdString());
		params.push_back(offerQty.toStdString());
		params.push_back(offerPrice.toStdString());
		params.push_back(offerDescription.toStdString());
		// keep it the same as what's in the database
		params.push_back(offerPrivate.toStdString());
		params.push_back("");
		if(tmpExclusiveWhitelist == QString("ON"))
			params.push_back("0");
		else 
			params.push_back("1");
		result = tableRPC.execute(strMethod, params);
		QMessageBox::information(this, windowTitle(),
		tr("Affiliate exclusive mode changed successfully!"),
			QMessageBox::Ok, QMessageBox::Ok);
		if(tmpExclusiveWhitelist == QString("ON"))
			exclusiveWhitelist = QString("OFF");
		else
			exclusiveWhitelist = QString("ON");

		if(exclusiveWhitelist == QString("ON"))
		{
			ui->exclusiveButton->setText(tr("Exclusive Mode is ON"));
		}
		else
		{
			ui->exclusiveButton->setText(tr("Exclusive Mode is OFF"));
		}
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
			tr("Could not change the affiliate mode: %1").arg(QString::fromStdString(strError)),
				QMessageBox::Ok, QMessageBox::Ok);

	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to change the affiliate mode: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
	}
	
}

void EditWhitelistOfferDialog::on_removeButton_clicked()
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
	QString aliasGUID = selection.at(0).data(OfferWhitelistTableModel::Alias).toString();


	string strError;
	string strMethod = string("offerremovewhitelist");
	UniValue params(UniValue::VARR);
	UniValue result;
	params.push_back(offerGUID.toStdString());
	params.push_back(aliasGUID.toStdString());

	try {
		result = tableRPC.execute(strMethod, params);
		QMessageBox::information(this, windowTitle(),
		tr("Entry removed successfully!"),
			QMessageBox::Ok, QMessageBox::Ok);
		model->updateEntry(aliasGUID, aliasGUID, aliasGUID, CT_DELETED); 
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
			tr("Could not remove this entry: %1").arg(QString::fromStdString(strError)),
				QMessageBox::Ok, QMessageBox::Ok);

	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to remove this entry: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
	}

	
}
void EditWhitelistOfferDialog::on_removeAllButton_clicked()
{
	if(!model)
		return;
	string strError;
	string strMethod = string("offerclearwhitelist");
	UniValue params(UniValue::VARR);
	UniValue result;
	params.push_back(offerGUID.toStdString());
	try {
		result = tableRPC.execute(strMethod, params);
		QMessageBox::information(this, windowTitle(),
		tr("Affiliate list cleared successfully!"),
			QMessageBox::Ok, QMessageBox::Ok);
		model->clear();
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
			tr("Could not clear the affiliate list: %1").arg(QString::fromStdString(strError)),
				QMessageBox::Ok, QMessageBox::Ok);

	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to clear the affiliate list: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
	}
}
void EditWhitelistOfferDialog::on_refreshButton_clicked()
{
	if(!model) return;
	string strError;
	string strMethod = string("offerwhitelist");
	UniValue params(UniValue::VARR);
	UniValue result;
	
	try {
		params.push_back(offerGUID.toStdString());
		result = tableRPC.execute(strMethod, params);
		if (result.type() == UniValue::VARR)
		{
			this->model->clear();
			string alias_str = "";
			string expiresin_str = "";
			string offer_discount_percentage_str = "";
			int expiresin = 0;
			const UniValue &arr = result.get_array();
		    for (unsigned int idx = 0; idx < arr.size(); idx++) {
			    const UniValue& input = arr[idx];
				if (input.type() != UniValue::VOBJ)
					continue;
				const UniValue& o = input.get_obj();
				const UniValue& alias_value = find_value(o, "alias");
				if (alias_value.type() == UniValue::VSTR)
					alias_str = alias_value.get_str();
				const UniValue& expiresin_value = find_value(o, "expiresin");
				if (expiresin_value.type() == UniValue::VNUM)
					expiresin = expiresin_value.get_int();
				const UniValue& offer_discount_percentage_value = find_value(o, "offer_discount_percentage");
				if (offer_discount_percentage_value.type() == UniValue::VSTR)
					offer_discount_percentage_str = offer_discount_percentage_value.get_str();
				expiresin_str = strprintf("%d Blocks", expiresin);
				model->addRow(QString::fromStdString(alias_str), QString::fromStdString(expiresin_str), QString::fromStdString(offer_discount_percentage_str));
				model->updateEntry(QString::fromStdString(alias_str), QString::fromStdString(expiresin_str), QString::fromStdString(offer_discount_percentage_str), CT_NEW); 
				ui->removeAllButton->setEnabled(true);
			}
		}
		if(exclusiveWhitelist == QString("ON"))
		{
			ui->exclusiveButton->setText(tr("Exclusive Mode is ON"));
		}
		else
		{
			ui->exclusiveButton->setText(tr("Exclusive Mode is OFF"));
		}
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
			tr("Could not refresh the affiliate list: %1").arg(QString::fromStdString(strError)),
				QMessageBox::Ok, QMessageBox::Ok);

	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to refresh the affiliate list: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
	}


}
void EditWhitelistOfferDialog::on_newEntry_clicked()
{
    if(!model)
        return;
    NewWhitelistDialog dlg(offerGUID);   
	dlg.setModel(walletModel, model);
    dlg.exec();
}
void EditWhitelistOfferDialog::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->removeButton->setEnabled(true);
    }
    else
    {
        ui->removeButton->setEnabled(false);
    }
}


void EditWhitelistOfferDialog::on_exportButton_clicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(
            this,
            tr("Export Affiliate Data"), QString(),
            tr("Comma separated file (*.csv)"), NULL);

    if (filename.isNull()) return;

    CSVModelWriter writer(filename);
    // name, column, role
    writer.setModel(proxyModel);
	writer.addColumn("Alias", OfferWhitelistTableModel::Alias, Qt::EditRole);
	writer.addColumn("Expires", OfferWhitelistTableModel::Expires, Qt::EditRole);
	writer.addColumn("Discount", OfferWhitelistTableModel::Discount, Qt::EditRole);
	
    if(!writer.write())
    {
        QMessageBox::critical(this, tr("Error exporting"), tr("Could not write to file %1.").arg(filename),
                              QMessageBox::Abort, QMessageBox::Abort);
    }
}



void EditWhitelistOfferDialog::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid()) {
        contextMenu->exec(QCursor::pos());
    }
}

void EditWhitelistOfferDialog::selectNewEntry(const QModelIndex &parent, int begin, int /*end*/)
{
    QModelIndex idx = proxyModel->mapFromSource(model->index(begin, OfferWhitelistTableModel::Alias, parent));
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() == newEntryToSelect))
    {
        // Select row of newly created offer, once
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newEntryToSelect.clear();
    }
}
