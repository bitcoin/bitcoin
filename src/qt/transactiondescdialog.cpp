#include "transactiondescdialog.h"
#include "ui_transactiondescdialog.h"

#include "transactiontablemodel.h"
#include "dialogwindowflags.h"

#include <QModelIndex>
#include <QKeyEvent>

TransactionDescDialog::TransactionDescDialog(const QModelIndex &idx, QWidget *parent) :
    QWidget(parent, DIALOGWINDOWHINTS),
    ui(new Ui::TransactionDescDialog)
{
    ui->setupUi(this);
    QString desc = idx.data(TransactionTableModel::LongDescriptionRole).toString();
    ui->detailText->setHtml(desc);
}

TransactionDescDialog::~TransactionDescDialog()
{
    delete ui;
}

void TransactionDescDialog::keyPressEvent(QKeyEvent *event)
{
#ifdef ANDROID
    if(event->key() == Qt::Key_Back)
    {
        close();
    }
#else
    if(event->key() == Qt::Key_Escape)
    {
        close();
    }
#endif
}

void TransactionDescDialog::closeEvent(QCloseEvent *e)
{
    emit(stopExec());
    QWidget::closeEvent(e);
}
