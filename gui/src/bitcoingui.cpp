/*
 * Qt4 bitcoin GUI.
 *
 * W.J. van der Laan 2011
 */
#include "bitcoingui.h"
#include "transactiontablemodel.h"
#include "addressbookdialog.h"
#include "sendcoinsdialog.h"
#include "optionsdialog.h"
#include "aboutdialog.h"
#include "clientmodel.h"

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
#include <QClipboard>

#include <QDebug>

#include <iostream>

BitcoinGUI::BitcoinGUI(QWidget *parent):
    QMainWindow(parent), trayIcon(0)
{
    resize(850, 550);
    setWindowTitle(tr("Bitcoin"));
    setWindowIcon(QIcon(":icons/bitcoin"));

    createActions();

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
    address = new QLineEdit();
    address->setReadOnly(true);
    hbox_address->addWidget(address);
    
    QPushButton *button_new = new QPushButton(tr("&New..."));
    QPushButton *button_clipboard = new QPushButton(tr("&Copy to clipboard"));
    hbox_address->addWidget(button_new);
    hbox_address->addWidget(button_clipboard);
    
    /* Balance: <balance> */
    QHBoxLayout *hbox_balance = new QHBoxLayout();
    hbox_balance->addWidget(new QLabel(tr("Balance:")));
    hbox_balance->addSpacing(5);/* Add some spacing between the label and the text */

    labelBalance = new QLabel();
    labelBalance->setFont(QFont("Teletype"));
    hbox_balance->addWidget(labelBalance);
    hbox_balance->addStretch(1);
    
    QVBoxLayout *vbox = new QVBoxLayout();
    vbox->addLayout(hbox_address);
    vbox->addLayout(hbox_balance);
    
    transaction_model = new TransactionTableModel(this);

    vbox->addWidget(createTabs());

    QWidget *centralwidget = new QWidget(this);
    centralwidget->setLayout(vbox);
    setCentralWidget(centralwidget);
    
    /* Create status bar */
    statusBar();
    
    labelConnections = new QLabel();
    labelConnections->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    labelConnections->setMinimumWidth(130);
    
    labelBlocks = new QLabel();
    labelBlocks->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    labelBlocks->setMinimumWidth(130);
    
    labelTransactions = new QLabel();
    labelTransactions->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    labelTransactions->setMinimumWidth(130);
    
    statusBar()->addPermanentWidget(labelConnections);
    statusBar()->addPermanentWidget(labelBlocks);
    statusBar()->addPermanentWidget(labelTransactions);
     
    /* Action bindings */
    connect(button_new, SIGNAL(clicked()), this, SLOT(newAddressClicked()));
    connect(button_clipboard, SIGNAL(clicked()), this, SLOT(copyClipboardClicked()));

    createTrayIcon();
}

void BitcoinGUI::createActions()
{
    quit = new QAction(QIcon(":/icons/quit"), tr("&Exit"), this);
    sendcoins = new QAction(QIcon(":/icons/send"), tr("&Send coins"), this);
    addressbook = new QAction(QIcon(":/icons/address-book"), tr("&Address Book"), this);
    about = new QAction(QIcon(":/icons/bitcoin"), tr("&About"), this);
    receiving_addresses = new QAction(QIcon(":/icons/receiving-addresses"), tr("Your &Receiving Addresses..."), this);
    options = new QAction(QIcon(":/icons/options"), tr("&Options..."), this);
    openBitCoin = new QAction(QIcon(":/icons/bitcoin"), "Open Bitcoin", this);

    connect(quit, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(sendcoins, SIGNAL(triggered()), this, SLOT(sendcoinsClicked()));
    connect(addressbook, SIGNAL(triggered()), this, SLOT(addressbookClicked()));
    connect(receiving_addresses, SIGNAL(triggered()), this, SLOT(receivingAddressesClicked()));
    connect(options, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(about, SIGNAL(triggered()), this, SLOT(aboutClicked()));
}

void BitcoinGUI::setModel(ClientModel *model)
{
    this->model = model;

    setBalance(model->getBalance());
    connect(model, SIGNAL(balanceChanged(double)), this, SLOT(setBalance(double)));

    setNumConnections(model->getNumConnections());
    connect(model, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

    setNumTransactions(model->getNumTransactions());
    connect(model, SIGNAL(numTransactionsChanged(int)), this, SLOT(setNumTransactions(int)));

    setNumBlocks(model->getNumBlocks());
    connect(model, SIGNAL(numBlocksChanged(int)), this, SLOT(setNumBlocks(int)));

    setAddress(model->getAddress());
    connect(model, SIGNAL(addressChanged(QString)), this, SLOT(setAddress(QString)));
}

void BitcoinGUI::createTrayIcon()
{
    QMenu *trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(openBitCoin);
    trayIconMenu->addAction(sendcoins);
    trayIconMenu->addAction(options);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quit);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(QIcon(":/icons/toolbar"));
    trayIcon->show();
}

QWidget *BitcoinGUI::createTabs()
{
    QStringList tab_filters, tab_labels;
    tab_filters << "^."
            << "^["+TransactionTableModel::Sent+TransactionTableModel::Received+"]"
            << "^["+TransactionTableModel::Sent+"]"
            << "^["+TransactionTableModel::Received+"]";
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
        proxy_model->setFilterRole(TransactionTableModel::TypeRole);
        proxy_model->setFilterRegExp(QRegExp(tab_filters.at(i)));

        QTableView *transaction_table = new QTableView(this);
        transaction_table->setModel(proxy_model);
        transaction_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        transaction_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
        transaction_table->setSortingEnabled(true);
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
    return tabs;
}

void BitcoinGUI::sendcoinsClicked()
{
    SendCoinsDialog dlg;
    dlg.exec();
}

void BitcoinGUI::addressbookClicked()
{
    AddressBookDialog dlg;
    dlg.setTab(AddressBookDialog::SendingTab);
    /* if an address accepted, do a 'send' to specified address */
    if(dlg.exec())
    {
        SendCoinsDialog send(0, dlg.getReturnValue());
        send.exec();
    }
}

void BitcoinGUI::receivingAddressesClicked()
{
    AddressBookDialog dlg;
    dlg.setTab(AddressBookDialog::ReceivingTab);
    dlg.exec();
}

void BitcoinGUI::optionsClicked()
{
    OptionsDialog dlg;
    dlg.exec();
}

void BitcoinGUI::aboutClicked()
{
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
    /* Copy text in address to clipboard */
    QApplication::clipboard()->setText(address->text());
}

void BitcoinGUI::setBalance(double balance)
{
    labelBalance->setText(QLocale::system().toString(balance, 8));
}

void BitcoinGUI::setAddress(const QString &addr)
{
    address->setText(addr);
}

void BitcoinGUI::setNumConnections(int count)
{
    labelConnections->setText(QLocale::system().toString(count)+" "+tr("connections"));
}

void BitcoinGUI::setNumBlocks(int count)
{
    labelBlocks->setText(QLocale::system().toString(count)+" "+tr("blocks"));
}

void BitcoinGUI::setNumTransactions(int count)
{
    labelTransactions->setText(QLocale::system().toString(count)+" "+tr("transactions"));
}
