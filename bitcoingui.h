#ifndef BITCOINGUI_H
#define BITCOINGUI_H

#include <QMainWindow>
#include <QSystemTrayIcon>

/* Forward declarations */
class TransactionTableModel;
class QLabel;
class QLineEdit;

class BitcoinGUI : public QMainWindow
{
    Q_OBJECT
public:
    explicit BitcoinGUI(QWidget *parent = 0);
    
    /* Transaction table tab indices */
    enum {
        AllTransactions = 0,
        SentReceived = 1,
        Sent = 2,
        Received = 3
    } TabIndex;
private:
    TransactionTableModel *transaction_model;

    QLineEdit *address;
    QLabel *labelBalance;

    QAction *quit;
    QAction *sendcoins;
    QAction *addressbook;
    QAction *about;
    QAction *receiving_addresses;
    QAction *options;
    QAction *openBitCoin;

    QSystemTrayIcon *trayIcon;

    void createActions();
    QWidget *createTabs();
    void createTrayIcon();

private slots:
    void sendcoinsClicked();
    void addressbookClicked();
    void optionsClicked();
    void receivingAddressesClicked();
    void aboutClicked();

    void newAddressClicked();
    void copyClipboardClicked();
};

#endif
