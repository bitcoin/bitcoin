#include "newmessagedialog.h"
#include "ui_newmessagedialog.h"
#include "cert.h"
#include "alias.h"
#include "messagetablemodel.h"
#include "guiutil.h"
#include "walletmodel.h"
#include "syscoingui.h"
#include "ui_interface.h"
#include <QDataWidgetMapper>
#include <QMessageBox>
#include "rpc/server.h"
#include <QSettings>
using namespace std;

extern CRPCTable tableRPC;
NewMessageDialog::NewMessageDialog(Mode mode, const QString &to, const QString &title, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewMessageDialog), mapper(0), mode(mode), model(0), walletModel(0)
{
    ui->setupUi(this);
	loadAliases();
	if(to != QString(""))
	{
		ui->toEdit->setEnabled(false);
		ui->toEdit->setText(to);
	}
	if(title != QString(""))
	{
		ui->topicEdit->setText(title);
	}
	QSettings settings;
	QString defaultMessageAlias;
	int aliasIndex;
    switch(mode)
    {
    case NewMessage:
        setWindowTitle(tr("New Message"));   
		ui->replyEdit->setVisible(false);
		ui->replyLabel->setVisible(false);
		ui->fromDisclaimer->setText(QString("<font color='blue'>") + tr("Select an Alias") + QString("</font>"));
		defaultMessageAlias = settings.value("defaultAlias", "").toString();
		aliasIndex = ui->aliasEdit->findText(defaultMessageAlias);
		if(aliasIndex >= 0)
			ui->aliasEdit->setCurrentIndex(aliasIndex);
        break;
    case ReplyMessage:
        setWindowTitle(tr("Reply Message"));
		ui->replyEdit->setVisible(true);
		ui->replyLabel->setVisible(true);
		ui->toEdit->setEnabled(false);
		ui->aliasEdit->setEnabled(false);
        break;
	}
    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
}

NewMessageDialog::~NewMessageDialog()
{
    delete ui;
}

void NewMessageDialog::loadAliases()
{
	ui->aliasEdit->clear();
	string strMethod = string("aliaslist");
    UniValue params(UniValue::VARR); 
	UniValue result ;
	string name_str;
	bool expired = false;
	
	try {
		result = tableRPC.execute(strMethod, params);

		if (result.type() == UniValue::VARR)
		{
			name_str = "";
			expired = false;


	
			const UniValue &arr = result.get_array();
		    for (unsigned int idx = 0; idx < arr.size(); idx++) {
			    const UniValue& input = arr[idx];
				if (input.type() != UniValue::VOBJ)
					continue;
				const UniValue& o = input.get_obj();
				name_str = "";

				expired = false;


		
				const UniValue& name_value = find_value(o, "alias");
				if (name_value.type() == UniValue::VSTR)
					name_str = name_value.get_str();		
				const UniValue& expired_value = find_value(o, "expired");
				if (expired_value.type() == UniValue::VBOOL)
					expired = expired_value.get_bool();
				
				if(expired == false)
				{
					QString name = QString::fromStdString(name_str);
					ui->aliasEdit->addItem(name);					
				}
				
			}
		}
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
			tr("Could not refresh alias list: ") + QString::fromStdString(strError),
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to refresh the alias list: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
	}         
 
}
void NewMessageDialog::setModel(WalletModel* walletModel, MessageTableModel *model)
{
    this->model = model;
	this->walletModel = walletModel;
    if(!model || mode != ReplyMessage) return;

    mapper->setModel(model);
	mapper->addMapping(ui->replyEdit, MessageTableModel::Message);
    mapper->addMapping(ui->toEdit, MessageTableModel::From);
	mapper->addMapping(ui->topicEdit, MessageTableModel::Subject); 
}

void NewMessageDialog::loadRow(int row)
{
	const QModelIndex tmpIndex;
	if(model)
	{
		if(mode == ReplyMessage)	
			mapper->setCurrentIndex(row);
		QModelIndex indexAlias = model->index(row, MessageTableModel::To, tmpIndex);
		if(indexAlias.isValid())
		{
			QString aliasStr = indexAlias.data(MessageTableModel::ToRole).toString();
			int indexInComboBox = ui->aliasEdit->findText(aliasStr);
			if(indexInComboBox < 0)
				indexInComboBox = 0;
			ui->aliasEdit->setCurrentIndex(indexInComboBox);
		}
	}

}
void NewMessageDialog::GetEncryptionKeys(const QString& fromalias, const QString& toalias)
{
	string strMethod = string("aliasinfo");
    UniValue params(UniValue::VARR); 
	params.push_back(fromalias.toStdString());
	UniValue result ;

	try {
		result = tableRPC.execute(strMethod, params);

		if (result.type() == UniValue::VOBJ)
		{
	
			const UniValue& o = result.get_obj();
			m_encryptionkeyfrom = QString::fromStdString(find_value(o, "encryption_publickey").get_str());	
		}
	}
	catch (UniValue& objError)
	{
        QMessageBox::information(this, windowTitle(),
			tr("Could not find alias: ") + toalias,
            QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
        QMessageBox::information(this, windowTitle(),
        tr("General exception when trying to get alias information"),
            QMessageBox::Ok, QMessageBox::Ok);
	} 
	string strMethod1 = string("aliasinfo");
    UniValue params1(UniValue::VARR); 
	params1.push_back(toalias.toStdString());
	UniValue result1 ;

	try {
		result1 = tableRPC.execute(strMethod1, params1);

		if (result.type() == UniValue::VOBJ)
		{
	
			const UniValue& o = result.get_obj();
			m_encryptionkeyto = QString::fromStdString(find_value(o, "encryption_publickey").get_str());	
		}
	}
	catch (UniValue& objError)
	{
        QMessageBox::information(this, windowTitle(),
			tr("Could not find alias: ") + toalias,
            QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
        QMessageBox::information(this, windowTitle(),
        tr("General exception when trying to get alias information"),
            QMessageBox::Ok, QMessageBox::Ok);
	} 
}
bool NewMessageDialog::saveCurrentRow()
{
	return true;
}

void NewMessageDialog::accept()
{
	bool saveState = saveCurrentRow();
    if(!saveState && model)
    {
        switch(model->getEditStatus())
        {
        case MessageTableModel::OK:
            // Failed with unknown reason. Just reject.
            break;
        case MessageTableModel::NO_CHANGES:
            // No changes were made during edit operation. Just reject.
            break;
        case MessageTableModel::INVALID_MESSAGE:
            QMessageBox::warning(this, windowTitle(),
                tr("The entered message is not a valid Syscoin message"),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case MessageTableModel::WALLET_UNLOCK_FAILURE:
            QMessageBox::critical(this, windowTitle(),
                tr("Could not unlock wallet."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        }
    }
	else if(saveState)
		QDialog::accept();
}

