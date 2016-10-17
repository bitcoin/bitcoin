<<<<<<< HEAD
// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
=======
// Copyright (c) 2011-2013 The Crowncoin developers
>>>>>>> origin/dirty-merge-dash-0.11.0
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "openuridialog.h"
#include "ui_openuridialog.h"

#include "guiutil.h"
#include "walletmodel.h"

#include <QUrl>

OpenURIDialog::OpenURIDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OpenURIDialog)
{
    ui->setupUi(this);
#if QT_VERSION >= 0x040700
<<<<<<< HEAD
    ui->uriEdit->setPlaceholderText("dash:");
=======
    ui->uriEdit->setPlaceholderText("crowncoin:");
>>>>>>> origin/dirty-merge-dash-0.11.0
#endif
}

OpenURIDialog::~OpenURIDialog()
{
    delete ui;
}

QString OpenURIDialog::getURI()
{
    return ui->uriEdit->text();
}

void OpenURIDialog::accept()
{
    SendCoinsRecipient rcp;
    if(GUIUtil::parseCrowncoinURI(getURI(), &rcp))
    {
        /* Only accept value URIs */
        QDialog::accept();
    } else {
        ui->uriEdit->setValid(false);
    }
}

void OpenURIDialog::on_selectFileButton_clicked()
{
    QString filename = GUIUtil::getOpenFileName(this, tr("Select payment request file to open"), "", "", NULL);
    if(filename.isEmpty())
        return;
    QUrl fileUri = QUrl::fromLocalFile(filename);
<<<<<<< HEAD
    ui->uriEdit->setText("dash:?r=" + QUrl::toPercentEncoding(fileUri.toString()));
=======
    ui->uriEdit->setText("crowncoin:?r=" + QUrl::toPercentEncoding(fileUri.toString()));
>>>>>>> origin/dirty-merge-dash-0.11.0
}
