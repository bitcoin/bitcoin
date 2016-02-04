#ifndef GAMEUNITSBRIDGE_H
#define GAMEUNITSBRIDGE_H

class GameunitsGUI;
class TransactionModel;
class BlockExplorerModel;
class AddressModel;
class MessageThread;
class SendCoinsRecipient;

#include <stdint.h>
#include <QObject>
#include <QModelIndex>

class GameunitsBridge : public QObject
{
    Q_OBJECT

    /** Information about the client */
    Q_PROPERTY(QVariantMap info READ getInfo);
public:
    explicit GameunitsBridge(GameunitsGUI *window, QObject *parent = 0);
    ~GameunitsBridge();

    void setClientModel();
    void setWalletModel();
    void setMessageModel();

    Q_INVOKABLE void copy(QString text);
    Q_INVOKABLE void paste();

    /** Get the label belonging to an address */
    Q_INVOKABLE QString getAddressLabel(QString address);
    /** Create a new address or add an existing address to your Address book */
    Q_INVOKABLE QString newAddress(bool own);
    /** Get the full transaction details */
    Q_INVOKABLE QString transactionDetails(QString txid);
    /** Get the pubkey for an address */
    Q_INVOKABLE QString getPubKey(QString address, QString label);

    /** Show debug dialog */
    Q_INVOKABLE QVariantMap userAction(QVariantMap action);

    Q_INVOKABLE void populateTransactionTable();

    Q_INVOKABLE void updateAddressLabel(QString address, QString label);
    Q_INVOKABLE bool validateAddress(QString own);
    Q_INVOKABLE bool deleteAddress(QString address);

    Q_INVOKABLE bool deleteMessage(QString key);
    Q_INVOKABLE bool markMessageAsRead(QString key);

    Q_INVOKABLE void openAddressBook(bool sending);
    Q_INVOKABLE void openCoinControl();

    Q_INVOKABLE bool addRecipient(QString address, QString label, QString narration, qint64 amount, int txnType, int nRingSize);
    Q_INVOKABLE bool sendCoins(bool fUseCoinControl, QString sChangeAddr);
    Q_INVOKABLE bool setPubKey(QString address, QString pubkey);
    Q_INVOKABLE bool sendMessage(const QString &address, const QString &message, const QString &from);

    Q_INVOKABLE void updateCoinControlAmount(qint64 amount);
    Q_INVOKABLE void updateCoinControlLabels(unsigned int &quantity, int64_t &amount, int64_t &fee, int64_t &afterfee, unsigned int &bytes, QString &priority, QString low, int64_t &change);

    Q_INVOKABLE QVariantMap listAnonOutputs();

    Q_INVOKABLE QVariantMap findBlock(QString searchID);
    Q_INVOKABLE QVariantMap listLatestBlocks();
    Q_INVOKABLE QVariantMap blockDetails(QString blkid);
    Q_INVOKABLE QVariantMap listTransactionsForBlock(QString blkid);
    Q_INVOKABLE QVariantMap txnDetails(QString blkHash, QString txnHash);

signals:
    void emitPaste(QString text);
    void emitTransactions(QVariantList transactions);
    void emitAddresses(QVariantList addresses);
    void emitMessages(QString messages, bool reset);
    void emitMessage(QString id, QString type, qint64 sent, qint64 received, QString label_v, QString label, QString to, QString from, bool read, QString message);
    void emitCoinControlUpdate(unsigned int quantity, qint64 amount, qint64 fee, qint64 afterfee, unsigned int bytes, QString priority, QString low, qint64 change);
    void emitAddressBookReturn(QString address, QString label);
    void emitReceipient(QString address, QString label, QString narration, qint64 amount);
    void triggerElement(QString element, QString trigger);
    void networkAlert(QString alert);

private:
    GameunitsGUI * window;
    TransactionModel * transactionModel;
    AddressModel * addressModel;
    MessageThread * thMessage;
    QList<SendCoinsRecipient> recipients;
    QVariantMap * info;
    QThread * async;

    friend class GameunitsGUI;

    inline QVariantMap getInfo() const { return *info; };

    void populateOptions();
    void populateAddressTable();
    void connectSignals();
    void clearRecipients();

    void appendMessage(int row);

private slots:
    void updateTransactions(QModelIndex topLeft, QModelIndex bottomRight);
    void updateAddresses(QModelIndex topLeft, QModelIndex bottomRight);
    void insertTransactions(const QModelIndex &parent, int start, int end);
    void insertAddresses(const QModelIndex &parent, int start, int end);
    void insertMessages(const QModelIndex &parent, int start, int end);

    void appendMessages(QString messages, bool reset);

    void populateMessageTable();

};

#endif // GAMEUNITSBRIDGE_H
