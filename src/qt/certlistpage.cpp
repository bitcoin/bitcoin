#include "certlistpage.h"
#include "ui_certlistpage.h"
#include "platformstyle.h"
#include "certtablemodel.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "syscoingui.h"
#include "editcertdialog.h"
#include "csvmodelwriter.h"
#include "guiutil.h"
#include "ui_interface.h"
#include <QClipboard>
#include <QMessageBox>
#include <QKeyEvent>
#include <QMenu>
#include "main.h"
#include "rpc/server.h"
#include <QStandardItemModel>
#include "qcomboboxdelegate.h"
#include <boost/algorithm/string.hpp>
#include <QSettings>
using namespace std;
extern CRPCTable tableRPC;
extern bool getCategoryList(vector<string>& categoryList);
CertListPage::CertListPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CertListPage),
    model(0),
    optionsModel(0),
	currentPage(0)
{
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();  
	if (!platformStyle->getImagesOnButtons())
	{
		ui->copyCert->setIcon(QIcon());
		ui->searchCert->setIcon(QIcon());
	}
	else
	{
		ui->copyCert->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/editcopy"));
		ui->searchCert->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/search"));
	}
    ui->labelExplanation->setText(tr("Search for Syscoin Certificates. Select Safe Search from wallet options if you wish to omit potentially offensive Certificates(On by default)"));
	
    // Context menu actions
    QAction *copyCertAction = new QAction(ui->copyCert->text(), this);
    QAction *copyCertValueAction = new QAction(tr("Copy Value"), this);


    // Build context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyCertAction);
    contextMenu->addAction(copyCertValueAction);

    // Connect signals for context menu actions
    connect(copyCertAction, SIGNAL(triggered()), this, SLOT(on_copyCert_clicked()));
    connect(copyCertValueAction, SIGNAL(triggered()), this, SLOT(onCopyCertValueAction()));
   
    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));


	ui->lineEditCertSearch->setPlaceholderText(tr("Enter search term, regex accepted (ie: ^name returns all Certificates starting with 'name'). Empty will search for all."));
}

CertListPage::~CertListPage()
{
    delete ui;
}

void CertListPage::addParentItem( QStandardItemModel * model, const QString& text, const QVariant& data )
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

void CertListPage::addChildItem( QStandardItemModel * model, const QString& text, const QVariant& data )
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
void CertListPage::loadCategories()
{
    QStandardItemModel * model = new QStandardItemModel;
	vector<string> categoryList;
	if(!getCategoryList(categoryList))
	{
		return;
	}
	addParentItem(model, tr("All Certificates"), tr("certificates"));
	for(unsigned int i = 0;i< categoryList.size(); i++)
	{
		vector<string> categories;
		boost::split(categories,categoryList[i],boost::is_any_of(">"));
		if(categories.size() > 0 && categories.size() <= 2)
		{
			for(unsigned int j = 0;j< categories.size(); j++)
			{
				boost::algorithm::trim(categories[j]);
				if(categories[0] != "certificates")
					continue;
				if(j == 1)
				{
					addChildItem(model, QString::fromStdString(categories[1]), QVariant(QString::fromStdString(categoryList[i])));
				}
			}
		}
	}
    ui->categoryEdit->setModel(model);
    ui->categoryEdit->setItemDelegate(new ComboBoxDelegate);
}
void CertListPage::showEvent ( QShowEvent * event )
{
    if(!walletModel) return;
    loadCategories();
}
void CertListPage::setModel(WalletModel* walletModel, CertTableModel *model)
{
    this->model = model;
	this->walletModel = walletModel;
    if(!model) return;

    ui->tableView->setModel(model);
	ui->tableView->setSortingEnabled(false);
    // Set column widths
    ui->tableView->setColumnWidth(0, 75); //cert
    ui->tableView->setColumnWidth(1, 100); //title
    ui->tableView->setColumnWidth(2, 100); //data
	ui->tableView->setColumnWidth(3, 200); //pubdata
	ui->tableView->setColumnWidth(4, 150); //category
    ui->tableView->setColumnWidth(5, 150); //expires on
    ui->tableView->setColumnWidth(6, 100); //cert state
    ui->tableView->setColumnWidth(7, 0); //owner

    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged()));


    // Select row for newly created cert
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewCert(QModelIndex,int,int)));

    selectionChanged();

}

void CertListPage::setOptionsModel(OptionsModel *optionsModel)
{
    this->optionsModel = optionsModel;
}
void CertListPage::on_copyCert_clicked()
{
   
    GUIUtil::copyEntryData(ui->tableView, CertTableModel::Name);
}

void CertListPage::onCopyCertValueAction()
{
    GUIUtil::copyEntryData(ui->tableView, CertTableModel::Title);
}



void CertListPage::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->copyCert->setEnabled(true);
    }
    else
    {
        ui->copyCert->setEnabled(false);
    }
}
void CertListPage::keyPressEvent(QKeyEvent * event)
{
  if( event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter )
  {
	on_searchCert_clicked();
    event->accept();
  }
  else
    QDialog::keyPressEvent( event );
}

void CertListPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid()) {
        contextMenu->exec(QCursor::pos());
    }
}

void CertListPage::selectNewCert(const QModelIndex &parent, int begin, int /*end*/)
{
    QModelIndex idx = model->index(begin, CertTableModel::Name, parent);
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() == newCertToSelect))
    {
        // Select row of newly created cert, once
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newCertToSelect.clear();
    }
}
void CertListPage::on_prevButton_clicked()
{
	if(pageMap.empty())
	{
		ui->nextButton->setEnabled(false);
		ui->prevButton->setEnabled(false);
		return;
	}
	currentPage--;
	const pair<string, string> &certPair = pageMap[currentPage];
	on_searchCert_clicked(certPair.first);
}
void CertListPage::on_nextButton_clicked()
{
	if(pageMap.empty())
	{
		ui->nextButton->setEnabled(false);
		ui->prevButton->setEnabled(false);
		return;
	}
	const pair<string, string> &certPair = pageMap[currentPage];
	currentPage++;
	on_searchCert_clicked(certPair.second);
	ui->prevButton->setEnabled(true);
}
void CertListPage::on_searchCert_clicked(string GUID)
{
    if(!walletModel) return;
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
    string strMethod = string("certfilter");
	string firstCert = "";
	string lastCert = "";
	string name_str;
	string value_str;
	string data_str;
	string pubdata_str;
	string category_str;
	string alias_str;
	string expired_str;
	int expired = 0;
	int64_t expires_on = 0; 
    params.push_back(ui->lineEditCertSearch->text().toStdString());
	params.push_back(GUID);
	params.push_back(settings.value("safesearch", "").toString().toStdString());
	QVariant currentCategory = ui->categoryEdit->itemData(ui->categoryEdit->currentIndex(), Qt::UserRole);
	if(ui->categoryEdit->currentIndex() > 0 &&  currentCategory != QVariant::Invalid)
		params.push_back(currentCategory.toString().toStdString());
	else if(ui->categoryEdit->currentText() != tr("All Certificates"))
		params.push_back(ui->categoryEdit->currentText().toStdString());

    try {
        result = tableRPC.execute(strMethod, params);
    }
    catch (UniValue& objError)
    {
        strError = find_value(objError, "message").get_str();
        QMessageBox::critical(this, windowTitle(),
        tr("Error searching Certificate: ") + QString::fromStdString(strError),
            QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
    catch(std::exception& e)
    {
        QMessageBox::critical(this, windowTitle(),
            tr("General exception when searching certficiates"),
            QMessageBox::Ok, QMessageBox::Ok);
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
			const UniValue & o = input.get_obj();
			name_str = "";
			value_str = "";
			data_str = "";
			pubdata_str = "";
			category_str = "";
			alias_str = "";
			expired = 0;
			expires_on = 0;

			const UniValue& name_value = find_value(o, "cert");
			if (name_value.type() == UniValue::VSTR)
				name_str = name_value.get_str();
			if(firstCert == "")
				firstCert = name_str;
			lastCert = name_str;
			const UniValue& value_value = find_value(o, "title");
			if (value_value.type() == UniValue::VSTR)
				value_str = value_value.get_str();
			const UniValue& data_value = find_value(o, "data");
			if (data_value.type() == UniValue::VSTR)
				data_str = data_value.get_str();
			const UniValue& pubdata_value = find_value(o, "pubdata");
			if (pubdata_value.type() == UniValue::VSTR)
				pubdata_str = pubdata_value.get_str();
			const UniValue& category_value = find_value(o, "category");
			if (category_value.type() == UniValue::VSTR)
				category_str = category_value.get_str();
			const UniValue& alias_value = find_value(o, "alias");
			if (alias_value.type() == UniValue::VSTR)
				alias_str = alias_value.get_str();
			const UniValue& expires_on_value = find_value(o, "expires_on");
			if (expires_on_value.type() == UniValue::VNUM)
				expires_on = expires_on_value.get_int64();

			expired_str = "Valid";
		   const QString& dateTimeString = GUIUtil::dateTimeStr(expires_on);
		   model->addRow(CertTableModel::Cert,
					QString::fromStdString(name_str),
					QString::fromStdString(value_str),
					QString::fromStdString(data_str),
					QString::fromStdString(pubdata_str),
					QString::fromStdString(category_str),
					dateTimeString,
					QString::fromStdString(expired_str),
					QString::fromStdString(alias_str),
					settings.value("safesearch", "").toString());
		   this->model->updateEntry(QString::fromStdString(name_str),
					QString::fromStdString(value_str),
					QString::fromStdString(data_str),
					QString::fromStdString(pubdata_str),
					QString::fromStdString(category_str),
					dateTimeString,
					QString::fromStdString(expired_str), 
					QString::fromStdString(alias_str), 
					settings.value("safesearch", "").toString(), AllCert, CT_NEW);	
		  }
		  pageMap[currentPage] = make_pair(firstCert, lastCert);  
		  ui->labelPage->setText(tr("Current Page: ") + QString("<b>%1</b>").arg(currentPage+1));
	} 
    else
    {
        QMessageBox::critical(this, windowTitle(),
            tr("Error: Invalid response from certfilter command"),
            QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

}
