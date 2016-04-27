#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>

#include "aliaslistpage.h"
#include "ui_aliaslistpage.h"
#include "platformstyle.h"
#include "aliastablemodel.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "syscoingui.h"
#include "editaliasdialog.h"
#include "newmessagedialog.h"
#include "csvmodelwriter.h"
#include "guiutil.h"
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

extern int GetAliasExpirationDepth();
AliasListPage::AliasListPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AliasListPage),
    model(0),
    optionsModel(0)
{
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();  
	if (!platformStyle->getImagesOnButtons())
	{
		ui->exportButton->setIcon(QIcon());
		ui->messageButton->setIcon(QIcon());
		ui->copyAlias->setIcon(QIcon());
		ui->searchAlias->setIcon(QIcon());
	}
	else
	{
		ui->exportButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/export"));
		ui->messageButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/outmail"));
		ui->copyAlias->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/editcopy"));
		ui->searchAlias->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/search"));
	}

    ui->labelExplanation->setText(tr("Search for Syscoin Aliases. Select the number of results desired from the dropdown box and click Search."));
	
    // Context menu actions
    QAction *copyAliasAction = new QAction(ui->copyAlias->text(), this);
    QAction *copyAliasValueAction = new QAction(tr("&Copy Value"), this);
	QAction *messageAction = new QAction(tr("&Send Msg"), this);

    // Build context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyAliasAction);
    contextMenu->addAction(copyAliasValueAction);
	contextMenu->addSeparator();
	contextMenu->addAction(messageAction);
    // Connect signals for context menu actions
    connect(copyAliasAction, SIGNAL(triggered()), this, SLOT(on_copyAlias_clicked()));
    connect(copyAliasValueAction, SIGNAL(triggered()), this, SLOT(onCopyAliasValueAction()));
	connect(messageAction, SIGNAL(triggered()), this, SLOT(on_messageButton_clicked()));

    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));


	ui->lineEditAliasSearch->setPlaceholderText(tr("Enter search term, regex accepted (ie: ^name returns all Aliases starting with 'name'). Empty will search for all."));
}

AliasListPage::~AliasListPage()
{
    delete ui;
}
void AliasListPage::showEvent ( QShowEvent * event )
{
    if(!walletModel) return;
    /*if(walletModel->getEncryptionStatus() == WalletModel::Locked)
	{
        ui->labelExplanation->setText(tr("<font color='blue'>WARNING: Your wallet is currently locked. For security purposes you'll need to enter your passphrase in order to search Syscoin Aliases.</font> <a href=\"http://lockedwallet.syscoin.org\">more info</a>"));
		ui->labelExplanation->setTextFormat(Qt::RichText);
		ui->labelExplanation->setTextInteractionFlags(Qt::TextBrowserInteraction);
		ui->labelExplanation->setOpenExternalLinks(true);
    }*/
}

void AliasListPage::setModel(WalletModel* walletModel, AliasTableModel *model)
{
    this->model = model;
	this->walletModel = walletModel;
    if(!model) return;

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterRole(AliasTableModel::TypeRole);
    proxyModel->setFilterFixedString(AliasTableModel::Alias);
    ui->tableView->setModel(proxyModel);
    ui->tableView->sortByColumn(0, Qt::AscendingOrder);

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

void AliasListPage::setOptionsModel(OptionsModel *optionsModel)
{
    this->optionsModel = optionsModel;
}

void AliasListPage::on_messageButton_clicked()
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
	QString alias = selection.at(0).data(AliasTableModel::NameRole).toString();
	// send message to seller
	NewMessageDialog dlg(NewMessageDialog::NewMessage, alias);   
	dlg.exec();
}

void AliasListPage::on_copyAlias_clicked()
{
   
    GUIUtil::copyEntryData(ui->tableView, AliasTableModel::Name);
}

void AliasListPage::onCopyAliasValueAction()
{
    GUIUtil::copyEntryData(ui->tableView, AliasTableModel::Value);
}


void AliasListPage::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->copyAlias->setEnabled(true);
		ui->messageButton->setEnabled(true);
    }
    else
    {
        ui->copyAlias->setEnabled(false);
		ui->messageButton->setEnabled(false);
    }
}
void AliasListPage::keyPressEvent(QKeyEvent * event)
{
  if( event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter )
  {
	on_searchAlias_clicked();
    event->accept();
  }
  else
    QDialog::keyPressEvent( event );
}
void AliasListPage::on_exportButton_clicked()
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



void AliasListPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid()) {
        contextMenu->exec(QCursor::pos());
    }
}

void AliasListPage::selectNewAlias(const QModelIndex &parent, int begin, int /*end*/)
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

void AliasListPage::on_searchAlias_clicked()
{
    if(!walletModel) return;
        UniValue params(UniValue::VARR);
        UniValue valError;
        UniValue valResult;
        UniValue valId;
        UniValue result ;
        string strReply;
        string strError;
        string strMethod = string("aliasfilter");
		string name_str;
		string value_str;
		string expires_in_str;
		string expires_on_str;
		string expired_str;
		int expired = 0;
		int expires_in = 0;
		int expires_on = 0;  
        params.push_back(ui->lineEditAliasSearch->text().toStdString());
        params.push_back(GetAliasExpirationDepth());
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
            tr("Error searching Alias: \"%1\"").arg(QString::fromStdString(strError)),
                QMessageBox::Ok, QMessageBox::Ok);
            return;
        }
        catch(std::exception& e)
        {
            QMessageBox::critical(this, windowTitle(),
                tr("General exception when searchig alias"),
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
				const UniValue &o = input.get_obj();
				name_str = "";
				value_str = "";
				expires_in_str = "";
				expires_on_str = "";
				expired = 0;
				expires_in = 0;
				expires_on = 0;

					const UniValue& name_value = find_value(o, "name");
					if (name_value.type() == UniValue::VSTR)
						name_str = name_value.get_str();
					const UniValue& value_value = find_value(o, "value");
					if (value_value.type() == UniValue::VSTR)
						value_str = value_value.get_str();
					const UniValue& expires_on_value = find_value(o, "expires_on");
					if (expires_on_value.type() == UniValue::VNUM)
						expires_on = expires_on_value.get_int();
					const UniValue& expires_in_value = find_value(o, "expires_in");
					if (expires_in_value.type() == UniValue::VNUM)
						expires_in = expires_in_value.get_int();
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
					expires_in_str = strprintf("%d Blocks", expires_in);
					expires_on_str = strprintf("Block %d", expires_on);
				}

	
				model->addRow(AliasTableModel::Alias,
						QString::fromStdString(name_str),
						QString::fromStdString(value_str),
						QString::fromStdString(expires_on_str),
						QString::fromStdString(expires_in_str),
						QString::fromStdString(expired_str));
					this->model->updateEntry(QString::fromStdString(name_str),
						QString::fromStdString(value_str),
						QString::fromStdString(expires_on_str),
						QString::fromStdString(expires_in_str),
						QString::fromStdString(expired_str), AllAlias, CT_NEW);	
			  }

            
         }   
        else
        {
            QMessageBox::critical(this, windowTitle(),
                tr("Error: Invalid response from aliasfilter command"),
                QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

}
