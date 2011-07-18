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
#include <QDateTime>
#include <QMovie>

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
    connect(transactionView, SIGNAL(doubleClicked(const QModelIndex&)), transactionView, SLOT(showDetails()));
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

    // Status bar "Connections" notification
    QFrame *frameConnections = new QFrame();
    frameConnections->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    frameConnections->setMinimumWidth(150);
    frameConnections->setMaximumWidth(150);
    QHBoxLayout *frameConnectionsLayout = new QHBoxLayout(frameConnections);
    frameConnectionsLayout->setContentsMargins(3,0,3,0);
    frameConnectionsLayout->setSpacing(3);
    labelConnectionsIcon = new QLabel();
    labelConnectionsIcon->setToolTip(tr("Number of connections to other clients"));
    frameConnectionsLayout->addWidget(labelConnectionsIcon);
    labelConnections = new QLabel();
    labelConnections->setToolTip(tr("Number of connections to other clients"));
    frameConnectionsLayout->addWidget(labelConnections);
    frameConnectionsLayout->addStretch();

    // Status bar "Blocks" notification
    QFrame *frameBlocks = new QFrame();
    frameBlocks->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    frameBlocks->setMinimumWidth(150);
    frameBlocks->setMaximumWidth(150);
    QHBoxLayout *frameBlocksLayout = new QHBoxLayout(frameBlocks);
    frameBlocksLayout->setContentsMargins(3,0,3,0);
    frameBlocksLayout->setSpacing(3);
    labelBlocksIcon = new QLabel();
    frameBlocksLayout->addWidget(labelBlocksIcon);
    labelBlocks = new QLabel();
    frameBlocksLayout->addWidget(labelBlocks);
    frameBlocksLayout->addStretch();

    // Progress bar for blocks download
    progressBarLabel = new QLabel(tr("Synchronizing with network..."));
    progressBarLabel->setVisible(false);
    progressBar = new QProgressBar();
    progressBar->setToolTip(tr("Block chain synchronization in progress"));
    progressBar->setVisible(false);

    statusBar()->addWidget(progressBarLabel);
    statusBar()->addWidget(progressBar);
    statusBar()->addPermanentWidget(frameConnections);
    statusBar()->addPermanentWidget(frameBlocks);

    createTrayIcon();

    syncIconMovie = new QMovie(":/movies/update_spinner", "mng", this);

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

    // Report errors from wallet thread
    connect(walletModel, SIGNAL(error(QString,QString)), this, SLOT(error(QString,QString)));

    // Put transaction list in tabs
    transactionView->setModel(walletModel);

    overviewPage->setModel(walletModel);
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
    labelConnectionsIcon->setPixmap(QIcon(icon).pixmap(16,16));
    labelConnections->setText(tr("%n connection(s)", "", count));
}

void BitcoinGUI::setNumBlocks(int count)
{
    int total = clientModel->getTotalBlocksEstimate();
    QString tooltip;

    if(count < total)
    {
        progressBarLabel->setVisible(true);
        progressBar->setVisible(true);
        progressBar->setMaximum(total);
        progressBar->setValue(count);
        tooltip = tr("Downloaded %1 of %2 blocks of transaction history.").arg(count).arg(total);
    }
    else
    {
        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);
        tooltip = tr("Downloaded %1 blocks of transaction history.").arg(count);
    }

    QDateTime now = QDateTime::currentDateTime();
    QDateTime lastBlockDate = clientModel->getLastBlockDate();
    int secs = lastBlockDate.secsTo(now);
    QString text;

    // Represent time from last generated block in human readable text
    if(secs < 60)
    {
        text = tr("%n second(s) ago","",secs);
    }
    else if(secs < 60*60)
    {
        text = tr("%n minute(s) ago","",secs/60);
    }
    else if(secs < 24*60*60)
    {
        text = tr("%n hour(s) ago","",secs/(60*60));
    }
    else
    {
        text = tr("%n day(s) ago","",secs/(60*60*24));
    }

    // In the label we want to be less specific
    QString labelText = text;
    bool spinning = true;
    if(secs < 30*60)
    {
        labelText = "Up to date";
        spinning = false;
    }

    tooltip += QString("\n");
    tooltip += tr("Last received block was generated %1.").arg(text);

    if(spinning)
    {
        labelBlocksIcon->setMovie(syncIconMovie);
        syncIconMovie->start();
    }
    else
    {
        labelBlocksIcon->setPixmap(QIcon(":/icons/synced").pixmap(16,16));
    }
    labelBlocks->setText(labelText);

    labelBlocksIcon->setToolTip(tooltip);
    labelBlocks->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
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

void BitcoinGUI::incomingTransaction(const QModelIndex & parent, int start, int end)
{
    TransactionTableModel *ttm = walletModel->getTransactionTableModel();
    qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent)
                    .data(Qt::EditRole).toULongLong();
    if(!clientModel->inInitialBlockDownload())
    {
        // On incoming transaction, make an info balloon
        // Unless the initial block download is in progress, to prevent balloon-spam
        QString date = ttm->index(start, TransactionTableModel::Date, parent)
                        .data().toString();
        QString type = ttm->index(start, TransactionTableModel::Type, parent)
                        .data().toString();
        QString address = ttm->index(start, TransactionTableModel::ToAddress, parent)
                        .data().toString();

        trayIcon->showMessage((amount)<0 ? tr("Sent transaction") :
                                           tr("Incoming transaction"),
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
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitcoinGUI::gotoHistoryPage()
{
    historyAction->setChecked(true);
    centralWidget->setCurrentWidget(transactionsPage);

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), transactionView, SLOT(exportClicked()));
}

void BitcoinGUI::gotoAddressBookPage()
{
    addressBookAction->setChecked(true);
    centralWidget->setCurrentWidget(addressBookPage);

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), addressBookPage, SLOT(exportClicked()));
}

void BitcoinGUI::gotoReceiveCoinsPage()
{
    receiveCoinsAction->setChecked(true);
    centralWidget->setCurrentWidget(receiveCoinsPage);

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), receiveCoinsPage, SLOT(exportClicked()));
}

void BitcoinGUI::gotoSendCoinsPage()
{
    sendCoinsAction->setChecked(true);
    sendCoinsPage->clear();
    centralWidget->setCurrentWidget(sendCoinsPage);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

