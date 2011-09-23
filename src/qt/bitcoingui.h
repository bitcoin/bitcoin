#ifndef BITCOINGUI_H
#define BITCOINGUI_H

#include <QMainWindow>
#include <QSystemTrayIcon>

class TransactionTableModel;
class ClientModel;
class WalletModel;
class TransactionView;
class OverviewPage;
class AddressBookPage;
class SendCoinsDialog;
class Notificator;

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QTableView;
class QAbstractItemModel;
class QModelIndex;
class QProgressBar;
class QStackedWidget;
class QUrl;
QT_END_NAMESPACE

class BitcoinGUI : public QMainWindow
{
    Q_OBJECT
public:
    explicit BitcoinGUI(QWidget *parent = 0);
    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    
    /* Transaction table tab indices */
    enum {
        AllTransactions = 0,
        SentReceived = 1,
        Sent = 2,
        Received = 3
    } TabIndex;

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private:
    ClientModel *clientModel;
    WalletModel *walletModel;

    QStackedWidget *centralWidget;

    OverviewPage *overviewPage;
    QWidget *transactionsPage;
    AddressBookPage *addressBookPage;
    AddressBookPage *receiveCoinsPage;
    SendCoinsDialog *sendCoinsPage;

    QLabel *labelEncryptionIcon;
    QLabel *labelConnectionsIcon;
    QLabel *labelBlocksIcon;
    QLabel *progressBarLabel;
    QProgressBar *progressBar;

    QAction *overviewAction;
    QAction *historyAction;
    QAction *quitAction;
    QAction *sendCoinsAction;
    QAction *addressBookAction;
    QAction *aboutAction;
    QAction *receiveCoinsAction;
    QAction *optionsAction;
    QAction *openBitcoinAction;
    QAction *exportAction;
    QAction *encryptWalletAction;
    QAction *changePassphraseAction;

    QSystemTrayIcon *trayIcon;
    Notificator *notificator;
    TransactionView *transactionView;

    QMovie *syncIconMovie;

    void createActions();
    QWidget *createTabs();
    void createTrayIcon();

public slots:
    void setNumConnections(int count);
    void setNumBlocks(int count);
    void setEncryptionStatus(int status);

    void error(const QString &title, const QString &message);
    /* It is currently not possible to pass a return value to another thread through
       BlockingQueuedConnection, so use an indirected pointer.
       http://bugreports.qt.nokia.com/browse/QTBUG-10440
    */
    void askFee(qint64 nFeeRequired, bool *payFee);

private slots:
    // UI pages
    void gotoOverviewPage();
    void gotoHistoryPage();
    void gotoAddressBookPage();
    void gotoReceiveCoinsPage();
    void gotoSendCoinsPage();

    // Misc actions
    void optionsClicked();
    void aboutClicked();
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void incomingTransaction(const QModelIndex & parent, int start, int end);
    void encryptWallet(bool status);
    void changePassphrase();
    void unlockWallet();
};

#endif
