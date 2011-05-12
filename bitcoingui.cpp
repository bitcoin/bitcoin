/*
 * Qt4 bitcoin GUI.
 *
 * W.J. van der Laan 2011
 */
#include "BitcoinGUI.h"
#include "TransactionTableModel.h"
#include "addressbookdialog.h"
#include "SendCoinsDialog.h"
#include "OptionsDialog.h"
#include "AboutDialog.h"

#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QIcon>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QTableView>
#include <QLineEdit>
#include <QPushButton>
#include <QHeaderView>
#include <QLocale>
#include <QSortFilterProxyModel>

#include <QDebug>

#include <iostream>

BitcoinGUI::BitcoinGUI(QWidget *parent):
    QMainWindow(parent)
{
    resize(850, 550);
    setWindowTitle(tr("Bitcoin"));
    setWindowIcon(QIcon(":icons/bitcoin"));
    
    QAction *quit = new QAction(QIcon(":/icons/quit"), tr("&Quit"), this);
    QAction *sendcoins = new QAction(QIcon(":/icons/send"), tr("&Send coins"), this);
    QAction *addressbook = new QAction(QIcon(":/icons/address-book"), tr("&Address book"), this);
    QAction *about = new QAction(QIcon(":/icons/bitcoin"), tr("&About"), this);
    QAction *receiving_addresses = new QAction(QIcon(":/icons/receiving-addresses"), tr("Your &Receiving Addresses..."), this);
    QAction *options = new QAction(QIcon(":/icons/options"), tr("&Options..."), this);

    /* Menus */
    QMenu *file = menuBar()->addMenu("&File");
    file->addAction(sendcoins);
    file->addSeparator();
    file->addAction(quit);
    
    QMenu *settings = menuBar()->addMenu("&Settings");
    settings->addAction(receiving_addresses);
    settings->addAction(options);

    QMenu *help = menuBar()->addMenu("&Help");
    help->addAction(about);
    
    /* Toolbar */
    QToolBar *toolbar = addToolBar("Main toolbar");
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->addAction(sendcoins);
    toolbar->addAction(addressbook);

    /* Address: <address>: New... : Paste to clipboard */
    QHBoxLayout *hbox_address = new QHBoxLayout();
    hbox_address->addWidget(new QLabel(tr("Your Bitcoin Address:")));
    QLineEdit *edit_address = new QLineEdit();
    edit_address->setReadOnly(true);
    hbox_address->addWidget(edit_address);
    
    QPushButton *button_new = new QPushButton(tr("&New..."));
    QPushButton *button_clipboard = new QPushButton(tr("&Copy to clipboard"));
    hbox_address->addWidget(button_new);
    hbox_address->addWidget(button_clipboard);
    
    /* Balance: <balance> */
    QHBoxLayout *hbox_balance = new QHBoxLayout();
    hbox_balance->addWidget(new QLabel(tr("Balance:")));
    hbox_balance->addSpacing(5);/* Add some spacing between the label and the text */

    QLabel *label_balance = new QLabel(QLocale::system().toString(1345.54));
    label_balance->setFont(QFont("Teletype"));
    hbox_balance->addWidget(label_balance);
    hbox_balance->addStretch(1);
    
    /* Tab widget */
    QVBoxLayout *vbox = new QVBoxLayout();
    vbox->addLayout(hbox_address);
    vbox->addLayout(hbox_balance);
    
    TransactionTableModel *transaction_model = new TransactionTableModel(this);

    /* setupTabs */
    QStringList tab_filters, tab_labels;
    tab_filters << "^."
                << "^[sr]"
                << "^[s]"
                << "^[r]";
    tab_labels  << tr("All transactions")
                << tr("Sent/Received")
                << tr("Sent")
                << tr("Received");
    QTabWidget *tabs = new QTabWidget(this);

    for(int i = 0; i < tab_labels.size(); ++i)
    {
        QSortFilterProxyModel *proxy_model = new QSortFilterProxyModel(this);
        proxy_model->setSourceModel(transaction_model);
        proxy_model->setDynamicSortFilter(true);
        proxy_model->setFilterRole(Qt::UserRole);
        proxy_model->setFilterRegExp(QRegExp(tab_filters.at(i)));

        QTableView *transaction_table = new QTableView(this);
        transaction_table->setModel(proxy_model);
        transaction_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        transaction_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
        transaction_table->verticalHeader()->hide();

        transaction_table->horizontalHeader()->resizeSection(
                TransactionTableModel::Status, 112);
        transaction_table->horizontalHeader()->resizeSection(
                TransactionTableModel::Date, 112);
        transaction_table->horizontalHeader()->setResizeMode(
                TransactionTableModel::Description, QHeaderView::Stretch);
        transaction_table->horizontalHeader()->resizeSection(
                TransactionTableModel::Debit, 79);
        transaction_table->horizontalHeader()->resizeSection(
                TransactionTableModel::Credit, 79);

        tabs->addTab(transaction_table, tab_labels.at(i));
    }
   
    vbox->addWidget(tabs);

    QWidget *centralwidget = new QWidget(this);
    centralwidget->setLayout(vbox);
    setCentralWidget(centralwidget);
    
    /* Status bar */
    statusBar();
    
    QLabel *label_connections = new QLabel("6 connections");
    label_connections->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    label_connections->setMinimumWidth(100);
    
    QLabel *label_blocks = new QLabel("6 blocks");
    label_blocks->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    label_blocks->setMinimumWidth(100);
    
    QLabel *label_transactions = new QLabel("6 transactions");
    label_transactions->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    label_transactions->setMinimumWidth(100);
    
    statusBar()->addPermanentWidget(label_connections);
    statusBar()->addPermanentWidget(label_blocks);
    statusBar()->addPermanentWidget(label_transactions);
     
    /* Action bindings */
    connect(quit, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(sendcoins, SIGNAL(triggered()), this, SLOT(sendcoinsClicked()));
    connect(addressbook, SIGNAL(triggered()), this, SLOT(addressbookClicked()));
    connect(receiving_addresses, SIGNAL(triggered()), this, SLOT(receivingAddressesClicked()));
    connect(options, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(button_new, SIGNAL(clicked()), this, SLOT(newAddressClicked()));
    connect(button_clipboard, SIGNAL(clicked()), this, SLOT(copyClipboardClicked()));
    connect(about, SIGNAL(triggered()), this, SLOT(aboutClicked()));
}

void BitcoinGUI::sendcoinsClicked()
{
    qDebug() << "Send coins clicked";
    SendCoinsDialog dlg;
    dlg.exec();
}

void BitcoinGUI::addressbookClicked()
{
    qDebug() << "Address book clicked";
    AddressBookDialog dlg;
    dlg.setTab(AddressBookDialog::SendingTab);
    dlg.exec();
}

void BitcoinGUI::receivingAddressesClicked()
{
    qDebug() << "Receiving addresses clicked";
    AddressBookDialog dlg;
    dlg.setTab(AddressBookDialog::ReceivingTab);
    dlg.exec();
}

void BitcoinGUI::optionsClicked()
{
    qDebug() << "Options clicked";
    OptionsDialog dlg;
    dlg.exec();
}

void BitcoinGUI::aboutClicked()
{
    qDebug() << "About clicked";
    AboutDialog dlg;
    dlg.exec();
}

void BitcoinGUI::newAddressClicked()
{
    qDebug() << "New address clicked";
    /* TODO: generate new address */
}

void BitcoinGUI::copyClipboardClicked()
{
    qDebug() << "Copy to clipboard";
    /* TODO: copy to clipboard */
}
