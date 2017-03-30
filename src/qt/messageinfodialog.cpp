#include "messageinfodialog.h"
#include "ui_messageinfodialog.h"

#include "messagetablemodel.h"
#include "guiutil.h"
#include "walletmodel.h"
#include "syscoingui.h"
#include "ui_interface.h"
#include <QDataWidgetMapper>
#include <QMessageBox>
#include "rpc/server.h"
using namespace std;

extern CRPCTable tableRPC;
MessageInfoDialog::MessageInfoDialog( QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MessageInfoDialog), mapper(0), model(0)
{
    ui->setupUi(this);
   
    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
}

MessageInfoDialog::~MessageInfoDialog()
{
    delete ui;
}

void MessageInfoDialog::setModel(WalletModel* walletModel, MessageTableModel *model)
{
    this->model = model;
	this->walletModel = walletModel;
    if(!model) return;

    mapper->setModel(model);
	mapper->addMapping(ui->timeLabel, MessageTableModel::Time);
	mapper->addMapping(ui->messageEdit, MessageTableModel::Message);
    mapper->addMapping(ui->toEdit, MessageTableModel::To);
    mapper->addMapping(ui->fromEdit, MessageTableModel::From);
	mapper->addMapping(ui->topicEdit, MessageTableModel::Subject); 
}

void MessageInfoDialog::loadRow(int row)
{

	mapper->setCurrentIndex(row);
}

void MessageInfoDialog::accept()
{
    
    QDialog::accept();
}

