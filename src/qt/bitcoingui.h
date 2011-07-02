#ifndef BITCOINGUI_H
#define BITCOINGUI_H

#include <QMainWindow>
#include <QSystemTrayIcon>

class TransactionTableModel;
class ClientModel;
class WalletModel;
class TransactionView;

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QTableView;
class QAbstractItemModel;
class QModelIndex;
class QProgressBar;
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

private:
    ClientModel *clientModel;
    WalletModel *walletModel;

    QLabel *labelBalance;
    QLabel *labelConnections;
    QLabel *labelConnectionsIcon;
    QLabel *labelBlocks;
    QLabel *labelTransactions;
    QLabel *progressBarLabel;
    QProgressBar *progressBar;

    QAction *quit;
    QAction *sendCoins;
    QAction *addressbook;
    QAction *about;
    QAction *receiveCoins;
    QAction *options;
    QAction *openBitcoin;

    QSystemTrayIcon *trayIcon;
    TransactionView *transactionView;

    void createActions();
    QWidget *createTabs();
    void createTrayIcon();

public slots:
    void setBalance(qint64 balance);
    void setNumConnections(int count);
    void setNumBlocks(int count);
    void setNumTransactions(int count);
    void error(const QString &title, const QString &message);
    /* It is currently not possible to pass a return value to another thread through
       BlockingQueuedConnection, so use an indirected pointer.
       http://bugreports.qt.nokia.com/browse/QTBUG-10440
    */
    void askFee(qint64 nFeeRequired, bool *payFee);

private slots:
    void sendCoinsClicked();
    void addressbookClicked();
    void optionsClicked();
    void receiveCoinsClicked();
    void aboutClicked();
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void transactionDetails(const QModelIndex& idx);
    void incomingTransaction(const QModelIndex & parent, int start, int end);
};

#endif
