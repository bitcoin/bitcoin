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
#include "guiutil.h"
#include "editaddressdialog.h"
#include "optionsmodel.h"
#include "transactiondescdialog.h"

#include "main.h"

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
#include <QMessageBox>

#include <QDebug>

#include <iostream>

BitcoinGUI::BitcoinGUI(QWidget *parent):
    QMainWindow(parent), trayIcon(0)
{
    resize(850, 550);
    setWindowTitle(tr("Bitcoin"));
    setWindowIcon(QIcon(":icons/bitcoin"));

    createActions();

    // Menus
    QMenu *file = menuBar()->addMenu("&File");
    file->addAction(sendcoins);
    file->addSeparator();
    file->addAction(quit);
    
    QMenu *settings = menuBar()->addMenu("&Settings");
    settings->addAction(receivingAddresses);
    settings->addAction(options);

    QMenu *help = menuBar()->addMenu("&Help");
    help->addAction(about);
    
    // Toolbar
    QToolBar *toolbar = addToolBar("Main toolbar");
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->addAction(sendcoins);
    toolbar->addAction(addressbook);

    // Address: <address>: New... : Paste to clipboard
    QHBoxLayout *hbox_address = new QHBoxLayout();
    hbox_address->addWidget(new QLabel(tr("Your Bitcoin address:")));
    address = new QLineEdit();
    address->setReadOnly(true);
    address->setFont(GUIUtil::bitcoinAddressFont());
    address->setToolTip(tr("Your current default receiving address"));
    hbox_address->addWidget(address);
    
    QPushButton *button_new = new QPushButton(tr("&New..."));
    button_new->setToolTip(tr("Create new receiving address"));
    QPushButton *button_clipboard = new QPushButton(tr("&Copy to clipboard"));
    button_clipboard->setToolTip(tr("Copy current receiving address to the system clipboard"));
    hbox_address->addWidget(button_new);
    hbox_address->addWidget(button_clipboard);
    
    // Balance: <balance>
    QHBoxLayout *hbox_balance = new QHBoxLayout();
    hbox_balance->addWidget(new QLabel(tr("Balance:")));
    hbox_balance->addSpacing(5);/* Add some spacing between the label and the text */

    labelBalance = new QLabel();
    labelBalance->setFont(QFont("Monospace"));
    labelBalance->setToolTip(tr("Your current balance"));
    hbox_balance->addWidget(labelBalance);
    hbox_balance->addStretch(1);
    
    QVBoxLayout *vbox = new QVBoxLayout();
    vbox->addLayout(hbox_address);
    vbox->addLayout(hbox_balance);

    vbox->addWidget(createTabs());

    QWidget *centralwidget = new QWidget(this);
    centralwidget->setLayout(vbox);
    setCentralWidget(centralwidget);
    
    // Create status bar
    statusBar();

    labelConnections = new QLabel();
    labelConnections->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    labelConnections->setMinimumWidth(150);
    labelConnections->setToolTip(tr("Number of connections to other clients"));

    labelBlocks = new QLabel();
    labelBlocks->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    labelBlocks->setMinimumWidth(130);
    labelBlocks->setToolTip(tr("Number of blocks in the block chain"));

    labelTransactions = new QLabel();
    labelTransactions->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    labelTransactions->setMinimumWidth(130);
    labelTransactions->setToolTip(tr("Number of transactions in your wallet"));

    statusBar()->addPermanentWidget(labelConnections);
    statusBar()->addPermanentWidget(labelBlocks);
    statusBar()->addPermanentWidget(labelTransactions);
     
    // Action bindings
    connect(button_new, SIGNAL(clicked()), this, SLOT(newAddressClicked()));
    connect(button_clipboard, SIGNAL(clicked()), this, SLOT(copyClipboardClicked()));

    createTrayIcon();
}

void BitcoinGUI::createActions()
{
    quit = new QAction(QIcon(":/icons/quit"), tr("&Exit"), this);
    quit->setToolTip(tr("Quit application"));
    sendcoins = new QAction(QIcon(":/icons/send"), tr("&Send coins"), this);
    sendcoins->setToolTip(tr("Send coins to a bitcoin address"));
    addressbook = new QAction(QIcon(":/icons/address-book"), tr("&Address Book"), this);
    addressbook->setToolTip(tr("Edit the list of stored addresses and labels"));
    about = new QAction(QIcon(":/icons/bitcoin"), tr("&About"), this);
    about->setToolTip(tr("Show information about Bitcoin"));
    receivingAddresses = new QAction(QIcon(":/icons/receiving-addresses"), tr("Your &Receiving Addresses..."), this);
    receivingAddresses->setToolTip(tr("Show the list of receiving addresses and edit their labels"));
    options = new QAction(QIcon(":/icons/options"), tr("&Options..."), this);
    options->setToolTip(tr("Modify configuration options for bitcoin"));
    openBitcoin = new QAction(QIcon(":/icons/bitcoin"), "Open &Bitcoin", this);
    openBitcoin->setToolTip(tr("Show the Bitcoin window"));

    connect(quit, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(sendcoins, SIGNAL(triggered()), this, SLOT(sendcoinsClicked()));
    connect(addressbook, SIGNAL(triggered()), this, SLOT(addressbookClicked()));
    connect(receivingAddresses, SIGNAL(triggered()), this, SLOT(receivingAddressesClicked()));
    connect(options, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(about, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    connect(openBitcoin, SIGNAL(triggered()), this, SLOT(show()));
}

void BitcoinGUI::setModel(ClientModel *model)
{
    this->model = model;

    // Keep up to date with client
    setBalance(model->getBalance());
    connect(model, SIGNAL(balanceChanged(qint64)), this, SLOT(setBalance(qint64)));

    setNumConnections(model->getNumConnections());
    connect(model, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

    setNumTransactions(model->getNumTransactions());
    connect(model, SIGNAL(numTransactionsChanged(int)), this, SLOT(setNumTransactions(int)));

    setNumBlocks(model->getNumBlocks());
    connect(model, SIGNAL(numBlocksChanged(int)), this, SLOT(setNumBlocks(int)));

    setAddress(model->getAddress());
    connect(model, SIGNAL(addressChanged(QString)), this, SLOT(setAddress(QString)));

    // Report errors from network/worker thread
    connect(model, SIGNAL(error(QString,QString)), this, SLOT(error(QString,QString)));    

    // Put transaction list in tabs
    setTabsModel(model->getTransactionTableModel());
}

void BitcoinGUI::createTrayIcon()
{
    QMenu *trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(openBitcoin);
    trayIconMenu->addAction(sendcoins);
    trayIconMenu->addAction(options);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quit);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(QIcon(":/icons/toolbar"));
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon->show();
}

void BitcoinGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::DoubleClick)
    {
        // Doubleclick on system tray icon triggers "open bitcoin"
        openBitcoin->trigger();
    }
}

QWidget *BitcoinGUI::createTabs()
{
    QStringList tab_labels;
    tab_labels  << tr("All transactions")
                << tr("Sent/Received")
                << tr("Sent")
                << tr("Received");

    QTabWidget *tabs = new QTabWidget(this);
    for(int i = 0; i < tab_labels.size(); ++i)
    {
        QTableView *view = new QTableView(this);
        tabs->addTab(view, tab_labels.at(i));

        connect(view, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(transactionDetails(const QModelIndex&)));
        transactionViews.append(view);
    }

    return tabs;
}

void BitcoinGUI::setTabsModel(QAbstractItemModel *transaction_model)
{
    QStringList tab_filters;
    tab_filters << "^."
            << "^["+TransactionTableModel::Sent+TransactionTableModel::Received+"]"
            << "^["+TransactionTableModel::Sent+"]"
            << "^["+TransactionTableModel::Received+"]";

    for(int i = 0; i < transactionViews.size(); ++i)
    {
        QSortFilterProxyModel *proxy_model = new QSortFilterProxyModel(this);
        proxy_model->setSourceModel(transaction_model);
        proxy_model->setDynamicSortFilter(true);
        proxy_model->setFilterRole(TransactionTableModel::TypeRole);
        proxy_model->setFilterRegExp(QRegExp(tab_filters.at(i)));
        proxy_model->setSortRole(Qt::EditRole);

        QTableView *transaction_table = transactionViews.at(i);
        transaction_table->setModel(proxy_model);
        transaction_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        transaction_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
        transaction_table->setSortingEnabled(true);
        transaction_table->sortByColumn(TransactionTableModel::Status, Qt::DescendingOrder);
        transaction_table->verticalHeader()->hide();

        transaction_table->horizontalHeader()->resizeSection(
                TransactionTableModel::Status, 23);
        transaction_table->horizontalHeader()->resizeSection(
                TransactionTableModel::Date, 120);
        transaction_table->horizontalHeader()->setResizeMode(
                TransactionTableModel::Description, QHeaderView::Stretch);
        transaction_table->horizontalHeader()->resizeSection(
                TransactionTableModel::Debit, 79);
        transaction_table->horizontalHeader()->resizeSection(
                TransactionTableModel::Credit, 79);
    }

    connect(transaction_model, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
            this, SLOT(incomingTransaction(const QModelIndex &, int, int)));
}

void BitcoinGUI::sendcoinsClicked()
{
    SendCoinsDialog dlg;
    dlg.setModel(model);
    dlg.exec();
}

void BitcoinGUI::addressbookClicked()
{
    AddressBookDialog dlg(AddressBookDialog::ForEditing);
    dlg.setModel(model->getAddressTableModel());
    dlg.setTab(AddressBookDialog::SendingTab);
    dlg.exec();
}

void BitcoinGUI::receivingAddressesClicked()
{
    AddressBookDialog dlg(AddressBookDialog::ForEditing);
    dlg.setModel(model->getAddressTableModel());
    dlg.setTab(AddressBookDialog::ReceivingTab);
    dlg.exec();
}

void BitcoinGUI::optionsClicked()
{
    OptionsDialog dlg;
    dlg.setModel(model->getOptionsModel());
    dlg.exec();
}

void BitcoinGUI::aboutClicked()
{
    AboutDialog dlg;
    dlg.exec();
}

void BitcoinGUI::newAddressClicked()
{
    EditAddressDialog dlg(EditAddressDialog::NewReceivingAddress);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        QString newAddress = dlg.saveCurrentRow();
        // Set returned address as new default addres
        if(!newAddress.isEmpty())
        {
            model->setAddress(newAddress);
        }
    }
}

void BitcoinGUI::copyClipboardClicked()
{
    // Copy text in address to clipboard
    QApplication::clipboard()->setText(address->text());
}

void BitcoinGUI::setBalance(qint64 balance)
{
    labelBalance->setText(QString::fromStdString(FormatMoney(balance)));
}

void BitcoinGUI::setAddress(const QString &addr)
{
    address->setText(addr);
}

void BitcoinGUI::setNumConnections(int count)
{
    QString icon;
    switch(count)
    {
    case 0: icon = ":/icons/connect_0"; break;
    case 1: case 2: case 3: icon = ":/icons/connect_1"; break;
    case 4: case 5: case 6: icon = ":/icons/connect_2"; break;
    case 7: case 8: case 9: icon = ":/icons/connect_3"; break;
    default: icon = ":/icons/connect_4"; break;
    }
    labelConnections->setTextFormat(Qt::RichText);
    labelConnections->setText("<img src=\""+icon+"\"> " + QLocale::system().toString(count)+" "+tr("connection(s)", "", count));
}

void BitcoinGUI::setNumBlocks(int count)
{
    labelBlocks->setText(QLocale::system().toString(count)+" "+tr("block(s)", "", count));
}

void BitcoinGUI::setNumTransactions(int count)
{
    labelTransactions->setText(QLocale::system().toString(count)+" "+tr("transaction(s)", "", count));
}

void BitcoinGUI::error(const QString &title, const QString &message)
{
    // Report errors from network/worker thread
    if(trayIcon->supportsMessages())
    {
        // Show as "balloon" message if possible
        trayIcon->showMessage(title, message, QSystemTrayIcon::Critical);
    }
    else
    {
        // Fall back to old fashioned popup dialog if not
        QMessageBox::critical(this, title,
            message,
            QMessageBox::Ok, QMessageBox::Ok);
    }
}

void BitcoinGUI::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::WindowStateChange)
    {
        if(model->getOptionsModel()->getMinimizeToTray())
        {
            if (isMinimized())
            {
                hide();
                e->ignore();
            }
            else
            {
                e->accept();
            }
        }
    }
    QMainWindow::changeEvent(e);
}

void BitcoinGUI::closeEvent(QCloseEvent *event)
{
    if(!model->getOptionsModel()->getMinimizeToTray() &&
       !model->getOptionsModel()->getMinimizeOnClose())
    {
        qApp->quit();
    }
    QMainWindow::closeEvent(event);
}

void BitcoinGUI::askFee(qint64 nFeeRequired, bool *payFee)
{
    QString strMessage =
        tr("This transaction is over the size limit.  You can still send it for a fee of %1, "
          "which goes to the nodes that process your transaction and helps to support the network.  "
          "Do you want to pay the fee?").arg(QString::fromStdString(FormatMoney(nFeeRequired)));
    QMessageBox::StandardButton retval = QMessageBox::question(
          this, tr("Sending..."), strMessage,
          QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
    *payFee = (retval == QMessageBox::Yes);
}

void BitcoinGUI::transactionDetails(const QModelIndex& idx)
{
    // A transaction is doubleclicked
    TransactionDescDialog dlg(idx);
    dlg.exec();
}

void BitcoinGUI::incomingTransaction(const QModelIndex & parent, int start, int end)
{
    TransactionTableModel *ttm = model->getTransactionTableModel();
    qint64 credit = ttm->index(start, TransactionTableModel::Credit, parent)
                    .data(Qt::EditRole).toULongLong();
    qint64 debit = ttm->index(start, TransactionTableModel::Debit, parent)
                    .data(Qt::EditRole).toULongLong();
    if((credit+debit)>0)
    {
        // On incoming transaction, make an info balloon
        QString date = ttm->index(start, TransactionTableModel::Date, parent)
                        .data().toString();
        QString description = ttm->index(start, TransactionTableModel::Description, parent)
                        .data().toString();

        trayIcon->showMessage(tr("Incoming transaction"),
                              "Date: " + date + "\n" +
                              "Amount: " + QString::fromStdString(FormatMoney(credit+debit, true)) + "\n" +
                              description,
                              QSystemTrayIcon::Information);
    }
}
