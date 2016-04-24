#include <boost/assign/list_of.hpp>

#include "escrowlistpage.h"
#include "ui_escrowlistpage.h"
#include "platformstyle.h"
#include "escrowtablemodel.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "syscoingui.h"
#include "csvmodelwriter.h"
#include "guiutil.h"
#include "ui_interface.h"
#include "platformstyle.h"
#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMessageBox>
#include <QKeyEvent>
#include <QDateTime>
#include <QMenu>
#include "main.h"
#include "rpcserver.h"
using namespace std;


extern const CRPCTable tableRPC;

extern int GetEscrowExpirationDepth();
EscrowListPage::EscrowListPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EscrowListPage),
    model(0),
    optionsModel(0)
{
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();  
	if (!platformStyle->getImagesOnButtons())
	{
		ui->exportButton->setIcon(QIcon());
		ui->copyEscrow->setIcon(QIcon());
		ui->searchEscrow->setIcon(QIcon());
	}
	else
	{
		ui->exportButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/export"));
		ui->copyEscrow->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/editcopy"));
		ui->searchEscrow->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/search"));
	}
    ui->labelExplanation->setText(tr("Search for Syscoin Escrows. Select the number of results desired from the dropdown box and click Search."));
	
    // Context menu actions
    QAction *copyEscrowAction = new QAction(ui->copyEscrow->text(), this);
    QAction *copyOfferAction = new QAction(tr("&Copy Offer ID"), this);


    // Build context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyEscrowAction);
    contextMenu->addAction(copyOfferAction);

    // Connect signals for context menu actions
    connect(copyEscrowAction, SIGNAL(triggered()), this, SLOT(on_copyEscrow_clicked()));
    connect(copyOfferAction, SIGNAL(triggered()), this, SLOT(on_copyOffer_clicked()));
   
    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));


	ui->lineEditEscrowSearch->setPlaceholderText(tr("Enter search term. Search for arbiter/seller or escrow GUID. Empty will search for all."));
}

EscrowListPage::~EscrowListPage()
{
    delete ui;
}
void EscrowListPage::showEvent ( QShowEvent * event )
{
    if(!walletModel) return;

}
void EscrowListPage::setModel(WalletModel* walletModel, EscrowTableModel *model)
{
    this->model = model;
	this->walletModel = walletModel;
    if(!model) return;

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterRole(EscrowTableModel::TypeRole);
    ui->tableView->setModel(proxyModel);
    ui->tableView->sortByColumn(1, Qt::DescendingOrder);

    // Set column widths
    ui->tableView->setColumnWidth(0, 50); //escrow id
    ui->tableView->setColumnWidth(1, 50); //time
    ui->tableView->setColumnWidth(2, 150); //seller
    ui->tableView->setColumnWidth(3, 150); //arbiter
    ui->tableView->setColumnWidth(4, 150); //buyer
    ui->tableView->setColumnWidth(5, 80); //offer
	ui->tableView->setColumnWidth(6, 250); //offer title
    ui->tableView->setColumnWidth(7, 80); //offeraccept
    ui->tableView->setColumnWidth(8, 0); //status

    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged()));


    // Select row for newly created escrow
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewEscrow(QModelIndex,int,int)));

    selectionChanged();

}

void EscrowListPage::setOptionsModel(OptionsModel *optionsModel)
{
    this->optionsModel = optionsModel;
}

void EscrowListPage::on_copyEscrow_clicked()
{
   
    GUIUtil::copyEntryData(ui->tableView, EscrowTableModel::Escrow);
}

void EscrowListPage::on_copyOffer_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, EscrowTableModel::Offer);
}


void EscrowListPage::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->copyEscrow->setEnabled(true);
    }
    else
    {
        ui->copyEscrow->setEnabled(false);
    }
}
void EscrowListPage::keyPressEvent(QKeyEvent * event)
{
  if( event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter )
  {
	on_searchEscrow_clicked();
    event->accept();
  }
  else
    QDialog::keyPressEvent( event );
}
void EscrowListPage::on_exportButton_clicked()
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
	writer.addColumn("Confirmation", EscrowTableModel::OfferAccept, Qt::EditRole);
	writer.addColumn("Total", EscrowTableModel::Total, Qt::EditRole);
	writer.addColumn("Status", EscrowTableModel::Status, Qt::EditRole);
    if(!writer.write())
    {
        QMessageBox::critical(this, tr("Error exporting"), tr("Could not write to file %1.").arg(filename),
                              QMessageBox::Abort, QMessageBox::Abort);
    }
}



void EscrowListPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid()) {
        contextMenu->exec(QCursor::pos());
    }
}

void EscrowListPage::selectNewEscrow(const QModelIndex &parent, int begin, int /*end*/)
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

void EscrowListPage::on_searchEscrow_clicked()
{
    if(!walletModel) return;
       UniValue params(UniValue::VARR);
        UniValue valError;
        UniValue valResult;
        UniValue valId;
        UniValue result ;
        string strReply;
        string strError;
        string strMethod = string("escrowfilter");
		string name_str;
		string time_str;
		string seller_str;
		string arbiter_str;
		string status_str;
		string offeraccept_str;
		string offer_str;
		string offertitle_str;
		string total_str;	
		string buyer_str;
		int unixTime;
		QDateTime dateTime;
        params.push_back(ui->lineEditEscrowSearch->text().toStdString());
        params.push_back(GetEscrowExpirationDepth());
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
            tr("Error searching Escrow: \"%1\"").arg(QString::fromStdString(strError)),
                QMessageBox::Ok, QMessageBox::Ok);
            return;
        }
        catch(std::exception& e)
        {
            QMessageBox::critical(this, windowTitle(),
                tr("General exception when searching escrow"),
                QMessageBox::Ok, QMessageBox::Ok);
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
				time_str = "";
				seller_str = "";
				arbiter_str = "";
				status_str = "";
				offeraccept_str = "";
				offer_str = "";
				offertitle_str = "";
				total_str = "";
				buyer_str = "";
				
				const UniValue& name_value = find_value(o, "escrow");
				if (name_value.type() == UniValue::VSTR)
					name_str = name_value.get_str();
				const UniValue& time_value = find_value(o, "time");
				if (time_value.type() == UniValue::VSTR)
					time_str = time_value.get_str();
				const UniValue& seller_value = find_value(o, "seller");
				if (seller_value.type() == UniValue::VSTR)
					seller_str = seller_value.get_str();
				const UniValue& arbiter_value = find_value(o, "arbiter");
				if (arbiter_value.type() == UniValue::VSTR)
					arbiter_str = arbiter_value.get_str();
				const UniValue& buyer_value = find_value(o, "buyer");
				if (buyer_value.type() == UniValue::VSTR)
					buyer_str = buyer_value.get_str();
				const UniValue& offer_value = find_value(o, "offer");
				if (offer_value.type() == UniValue::VSTR)
					offer_str = offer_value.get_str();
				const UniValue& offertitle_value = find_value(o, "offertitle");
				if (offertitle_value.type() == UniValue::VSTR)
					offertitle_str = offertitle_value.get_str();

				const UniValue& offeraccept_value = find_value(o, "offeracceptlink");
				if (offeraccept_value.type() == UniValue::VSTR)
					offeraccept_str = offeraccept_value.get_str();
				const UniValue& total_value = find_value(o, "total");
				if (total_value.type() == UniValue::VSTR)
					total_str = total_value.get_str();
				const UniValue& status_value = find_value(o, "status");
				if (status_value.type() == UniValue::VSTR)
					status_str = status_value.get_str();

				unixTime = atoi(time_str.c_str());
				dateTime.setTime_t(unixTime);
				time_str = dateTime.toString().toStdString();

				model->addRow(QString::fromStdString(name_str), QString::fromStdString(time_str),
						QString::fromStdString(seller_str),
						QString::fromStdString(arbiter_str),
						QString::fromStdString(offer_str),
						QString::fromStdString(offertitle_str),
						QString::fromStdString(offeraccept_str),
						QString::fromStdString(total_str),
						QString::fromStdString(status_str),
						QString::fromStdString(buyer_str));
					this->model->updateEntry(QString::fromStdString(name_str), QString::fromStdString(time_str),
						QString::fromStdString(seller_str),
						QString::fromStdString(arbiter_str),
						QString::fromStdString(offer_str),
						QString::fromStdString(offertitle_str),
						QString::fromStdString(offeraccept_str),
						QString::fromStdString(total_str),
						QString::fromStdString(status_str), 
						QString::fromStdString(buyer_str), AllEscrow, CT_NEW);	
			  }

            
         }   
        else
        {
            QMessageBox::critical(this, windowTitle(),
                tr("Error: Invalid response from escrowfilter command"),
                QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

}
