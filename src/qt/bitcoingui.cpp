/*
 * Qt4 bitcoin GUI.
 *
 * W.J. van der Laan 2011
 */
#include "bitcoingui.h"
#include "transactiontablemodel.h"
#include "addressbookpage.h"
#include "sendcoinsdialog.h"
#include "optionsdialog.h"
#include "aboutdialog.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "guiutil.h"
#include "editaddressdialog.h"
#include "optionsmodel.h"
#include "transactiondescdialog.h"
#include "addresstablemodel.h"
#include "transactionview.h"
#include "overviewpage.h"

#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QIcon>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QLocale>
#include <QMessageBox>
#include <QProgressBar>
#include <QStackedWidget>

#include <QDebug>

#include <iostream>

BitcoinGUI::BitcoinGUI(QWidget *parent):
    QMainWindow(parent),
    clientModel(0),
    walletModel(0),
    trayIcon(0)
{
    resize(850, 550);
    setWindowTitle(tr("Bitcoin Wallet"));
    setWindowIcon(QIcon(":icons/bitcoin"));

    createActions();

    // Menus
    QMenu *file = menuBar()->addMenu("&File");
    file->addAction(sendCoinsAction);
    file->addAction(receiveCoinsAction);
    file->addSeparator();
    file->addAction(quitAction);
    
    QMenu *settings = menuBar()->addMenu("&Settings");
    settings->addAction(optionsAction);

    QMenu *help = menuBar()->addMenu("&Help");
    help->addAction(aboutAction);
    
    // Toolbar
    QToolBar *toolbar = addToolBar("Main toolbar");
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->addAction(overviewAction);
    toolbar->addAction(sendCoinsAction);
    toolbar->addAction(receiveCoinsAction);
    toolbar->addAction(historyAction);
    toolbar->addAction(addressBookAction);

    QToolBar *toolbar2 = addToolBar("Transactions toolbar");
    toolbar2->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar2->addAction(exportAction);

    // Overview page
    overviewPage = new OverviewPage();
    QVBoxLayout *vbox = new QVBoxLayout();

    transactionView = new TransactionView(this);
    connect(transactionView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(transactionDetails(const QModelIndex&)));
    vbox->addWidget(transactionView);

    transactionsPage = new QWidget(this);
    transactionsPage->setLayout(vbox);

    addressBookPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::SendingTab);

    receiveCoinsPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::ReceivingTab);

    sendCoinsPage = new SendCoinsDialog(this);

    centralWidget = new QStackedWidget(this);
    centralWidget->addWidget(overviewPage);
    centralWidget->addWidget(transactionsPage);
    centralWidget->addWidget(addressBookPage);
    centralWidget->addWidget(receiveCoinsPage);
    centralWidget->addWidget(sendCoinsPage);
    setCentralWidget(centralWidget);
    
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

    // Progress bar for blocks download
    progressBarLabel = new QLabel(tr("Synchronizing with network..."));
    progressBarLabel->setVisible(false);
    progressBar = new QProgressBar();
    progressBar->setToolTip(tr("Block chain synchronization in progress"));
    progressBar->setVisible(false);

    statusBar()->addWidget(progressBarLabel);
    statusBar()->addWidget(progressBar);
    statusBar()->addPermanentWidget(labelConnections);
    statusBar()->addPermanentWidget(labelBlocks);

    createTrayIcon();

    gotoOverviewPage();
}

void BitcoinGUI::createActions()
{
    QActionGroup *tabGroup = new QActionGroup(this);

    overviewAction = new QAction(QIcon(":/icons/overview"), tr("&Overview"), this);
    overviewAction->setCheckable(true);
    tabGroup->addAction(overviewAction);

    historyAction = new QAction(QIcon(":/icons/history"), tr("&Transactions"), this);
    historyAction->setCheckable(true);
    tabGroup->addAction(historyAction);

    addressBookAction = new QAction(QIcon(":/icons/address-book"), tr("&Address Book"), this);
    addressBookAction->setToolTip(tr("Edit the list of stored addresses and labels"));
    addressBookAction->setCheckable(true);
    tabGroup->addAction(addressBookAction);

    receiveCoinsAction = new QAction(QIcon(":/icons/receiving_addresses"), tr("&Receive coins"), this);
    receiveCoinsAction->setToolTip(tr("Show the list of addresses for receiving payments"));
    receiveCoinsAction->setCheckable(true);
    tabGroup->addAction(receiveCoinsAction);

    sendCoinsAction = new QAction(QIcon(":/icons/send"), tr("&Send coins"), this);
    sendCoinsAction->setToolTip(tr("Send coins to a bitcoin address"));
    sendCoinsAction->setCheckable(true);
    tabGroup->addAction(sendCoinsAction);

    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));
    connect(addressBookAction, SIGNAL(triggered()), this, SLOT(gotoAddressBookPage()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));

    quitAction = new QAction(QIcon(":/icons/quit"), tr("&Exit"), this);
    quitAction->setToolTip(tr("Quit application"));
    aboutAction = new QAction(QIcon(":/icons/bitcoin"), tr("&About"), this);
    aboutAction->setToolTip(tr("Show information about Bitcoin"));
    optionsAction = new QAction(QIcon(":/icons/options"), tr("&Options..."), this);
    optionsAction->setToolTip(tr("Modify configuration options for bitcoin"));
    openBitcoinAction = new QAction(QIcon(":/icons/bitcoin"), tr("Open &Bitcoin"), this);
    openBitcoinAction->setToolTip(tr("Show the Bitcoin window"));
    exportAction = new QAction(QIcon(":/icons/export"), tr("&Export..."), this);
    exportAction->setToolTip(tr("Export data in current view to a file"));

    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    connect(openBitcoinAction, SIGNAL(triggered()), this, SLOT(show()));
    connect(exportAction, SIGNAL(triggered()), this, SLOT(exportClicked()));
}

void BitcoinGUI::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;

    if(clientModel->isTestNet())
    {
        QString title_testnet = windowTitle() + QString(" ") + tr("[testnet]");
        setWindowTitle(title_testnet);
        setWindowIcon(QIcon(":icons/bitcoin_testnet"));
        if(trayIcon)
        {
            trayIcon->setToolTip(title_testnet);
            trayIcon->setIcon(QIcon(":/icons/toolbar_testnet"));
        }
    }

    // Keep up to date with client
    setNumConnections(clientModel->getNumConnections());
    connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

    setNumBlocks(clientModel->getNumBlocks());
    connect(clientModel, SIGNAL(numBlocksChanged(int)), this, SLOT(setNumBlocks(int)));

    // Report errors from network/worker thread
    connect(clientModel, SIGNAL(error(QString,QString)), this, SLOT(error(QString,QString)));
}

void BitcoinGUI::setWalletModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;

    // Keep up to date with wallet
    setBalance(walletModel->getBalance());
    connect(walletModel, SIGNAL(balanceChanged(qint64)), this, SLOT(setBalance(qint64)));

    setNumTransactions(walletModel->getNumTransactions());
    connect(walletModel, SIGNAL(numTransactionsChanged(int)), this, SLOT(setNumTransactions(int)));

    // Report errors from wallet thread
    connect(walletModel, SIGNAL(error(QString,QString)), this, SLOT(error(QString,QString)));

    // Put transaction list in tabs
    transactionView->setModel(walletModel->getTransactionTableModel());

    addressBookPage->setModel(walletModel->getAddressTableModel());
    receiveCoinsPage->setModel(walletModel->getAddressTableModel());
    sendCoinsPage->setModel(walletModel);

    // Balloon popup for new transaction
    connect(walletModel->getTransactionTableModel(), SIGNAL(rowsInserted(const QModelIndex &, int, int)),
            this, SLOT(incomingTransaction(const QModelIndex &, int, int)));
}

void BitcoinGUI::createTrayIcon()
{
    QMenu *trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(openBitcoinAction);
    trayIconMenu->addAction(sendCoinsAction);
    trayIconMenu->addAction(optionsAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setToolTip("Bitcoin client");
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
        openBitcoinAction->trigger();
    }
}

void BitcoinGUI::optionsClicked()
{
    OptionsDialog dlg;
    dlg.setModel(clientModel->getOptionsModel());
    dlg.exec();
}

void BitcoinGUI::aboutClicked()
{
    AboutDialog dlg;
    dlg.setModel(clientModel);
    dlg.exec();
}

void BitcoinGUI::setBalance(qint64 balance)
{
    overviewPage->setBalance(balance);
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
    labelConnections->setText("<img src=\""+icon+"\"> " + tr("%n connection(s)", "", count));
}

void BitcoinGUI::setNumBlocks(int count)
{
    int total = clientModel->getTotalBlocksEstimate();
    if(count < total)
    {
        progressBarLabel->setVisible(true);
        progressBar->setVisible(true);
        progressBar->setMaximum(total);
        progressBar->setValue(count);
    }
    else
    {
        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);
    }

    labelBlocks->setText(tr("%n block(s)", "", count));
}

void BitcoinGUI::setNumTransactions(int count)
{
    overviewPage->setNumTransactions(count);
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
        if(clientModel->getOptionsModel()->getMinimizeToTray())
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
    if(!clientModel->getOptionsModel()->getMinimizeToTray() &&
       !clientModel->getOptionsModel()->getMinimizeOnClose())
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
          "Do you want to pay the fee?").arg(GUIUtil::formatMoney(nFeeRequired));
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
    TransactionTableModel *ttm = walletModel->getTransactionTableModel();
    qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent)
                    .data(Qt::EditRole).toULongLong();
    if(amount>0 && !clientModel->inInitialBlockDownload())
    {
        // On incoming transaction, make an info balloon
        // Unless the initial block download is in progress, to prevent balloon-spam
        QString date = ttm->index(start, TransactionTableModel::Date, parent)
                        .data().toString();
        QString type = ttm->index(start, TransactionTableModel::Type, parent)
                        .data().toString();
        QString address = ttm->index(start, TransactionTableModel::ToAddress, parent)
                        .data().toString();

        trayIcon->showMessage(tr("Incoming transaction"),
                              tr("Date: ") + date + "\n" +
                              tr("Amount: ") + GUIUtil::formatMoney(amount, true) + "\n" +
                              tr("Type: ") + type + "\n" +
                              tr("Address: ") + address + "\n",
                              QSystemTrayIcon::Information);
    }
}

void BitcoinGUI::gotoOverviewPage()
{
    overviewAction->setChecked(true);
    centralWidget->setCurrentWidget(overviewPage);
    exportAction->setEnabled(false);
}

void BitcoinGUI::gotoHistoryPage()
{
    historyAction->setChecked(true);
    centralWidget->setCurrentWidget(transactionsPage);
    exportAction->setEnabled(true);
}

void BitcoinGUI::gotoAddressBookPage()
{
    addressBookAction->setChecked(true);
    centralWidget->setCurrentWidget(addressBookPage);
    exportAction->setEnabled(false); // TODO
}

void BitcoinGUI::gotoReceiveCoinsPage()
{
    receiveCoinsAction->setChecked(true);
    centralWidget->setCurrentWidget(receiveCoinsPage);
    exportAction->setEnabled(false); // TODO
}

void BitcoinGUI::gotoSendCoinsPage()
{
    sendCoinsAction->setChecked(true);
    centralWidget->setCurrentWidget(sendCoinsPage);
    exportAction->setEnabled(false);
}

void BitcoinGUI::exportClicked()
{
    // Redirect to the right view, as soon as export for other views
    // (such as address book) is implemented.
    transactionView->exportClicked();
}

