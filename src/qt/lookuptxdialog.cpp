// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "lookuptxdialog.h"
#include "ui_lookuptxdialog.h"

#include "omnicore/errors.h"
#include "omnicore/rpc.h"
#include "omnicore/rpctxobject.h"

#include "uint256.h"

#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"

#include <string>

#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QString>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

using std::string;
using namespace json_spirit;

LookupTXDialog::LookupTXDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LookupTXDialog)
{
    ui->setupUi(this);

#if QT_VERSION >= 0x040700
    ui->searchLineEdit->setPlaceholderText("Search transaction");
#endif

    // connect actions
    connect(ui->searchButton, SIGNAL(clicked()), this, SLOT(searchButtonClicked()));
}

LookupTXDialog::~LookupTXDialog()
{
    delete ui;
}

void LookupTXDialog::searchTX()
{
    // search function to lookup address
    string searchText = ui->searchLineEdit->text().toStdString();

    // first let's check if we have a searchText, if not do nothing
    if (searchText.empty()) return;

    uint256 hash;
    hash.SetHex(searchText);
    Object txobj;
    // make a request to new RPC populator function to populate a transaction object
    int populateResult = populateRPCTransactionObject(hash, txobj, "");
    if (0<=populateResult)
    {
        std::string strTXText = write_string(Value(txobj), false) + "\n";
        // clean up
        string from = ",";
        string to = ",\n    ";
        size_t start_pos = 0;
        while((start_pos = strTXText.find(from, start_pos)) != std::string::npos)
        {
            strTXText.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
        from = ":";
        to = "   :   ";
        start_pos = 0;
        while((start_pos = strTXText.find(from, start_pos)) != std::string::npos)
        {
            strTXText.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
        from = "{";
        to = "{\n    ";
        start_pos = 0;
        while((start_pos = strTXText.find(from, start_pos)) != std::string::npos)
        {
            strTXText.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
        from = "}";
        to = "\n}";
        start_pos = 0;
        while((start_pos = strTXText.find(from, start_pos)) != std::string::npos)
        {
            strTXText.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }

        QString txText = QString::fromStdString(strTXText);
        QDialog *txDlg = new QDialog;
        QLayout *dlgLayout = new QVBoxLayout;
        dlgLayout->setSpacing(12);
        dlgLayout->setMargin(12);
        QTextEdit *dlgTextEdit = new QTextEdit;
        dlgTextEdit->setText(txText);
        dlgTextEdit->setStatusTip("Transaction Information");
        dlgLayout->addWidget(dlgTextEdit);
        txDlg->setWindowTitle("Transaction Information");
        QPushButton *closeButton = new QPushButton(tr("&Close"));
        closeButton->setDefault(true);
        QDialogButtonBox *buttonBox = new QDialogButtonBox;
        buttonBox->addButton(closeButton, QDialogButtonBox::AcceptRole);
        dlgLayout->addWidget(buttonBox);
        txDlg->setLayout(dlgLayout);
        txDlg->resize(700, 360);
        connect(buttonBox, SIGNAL(accepted()), txDlg, SLOT(accept()));
        txDlg->setAttribute(Qt::WA_DeleteOnClose); //delete once it's closed
        if (txDlg->exec() == QDialog::Accepted) { } else { } //do nothing but close
    }
    else
    {
        // show error message
        string strText = "The transaction hash entered is ";
        switch(populateResult) {
            case MP_TX_NOT_FOUND:
                strText += "not a valid Bitcoin or Omni transaction.  Please check the transaction hash "
                           "entered and try again.";
            break;
            case MP_TX_UNCONFIRMED:
                strText += "unconfirmed.  Toolbox lookup of transactions is currently only available for "
                           "confirmed transactions.\n\nTip: You can view your own outgoing unconfirmed "
                           "transactions in the transactions tab.";
            break;
            case MP_TX_IS_NOT_MASTER_PROTOCOL:
                strText += "a Bitcoin transaction only.\n\nTip: You can use the debug console "
                           "'gettransaction' command to lookup specific Bitcoin transactions.";
            break;
        }
        QString strQText = QString::fromStdString(strText);
        QMessageBox errorDialog;
        errorDialog.setIcon(QMessageBox::Critical);
        errorDialog.setWindowTitle("TXID error");
        errorDialog.setText(strQText);
        errorDialog.setStandardButtons(QMessageBox::Ok);
        errorDialog.setDefaultButton(QMessageBox::Ok);
        if(errorDialog.exec() == QMessageBox::Ok) { } // no other button to choose, acknowledged
    }
}

void LookupTXDialog::searchButtonClicked()
{
    searchTX();
}
