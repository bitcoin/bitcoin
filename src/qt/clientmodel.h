#ifndef CLIENTMODEL_H
#define CLIENTMODEL_H

#include <QObject>

class OptionsModel;
class AddressTableModel;
class TransactionTableModel;

class ClientModel : public QObject
{
    Q_OBJECT
public:
    explicit ClientModel(QObject *parent = 0);

    enum StatusCode
    {
        OK,
        InvalidAmount,
        InvalidAddress,
        AmountExceedsBalance,
        AmountWithFeeExceedsBalance,
        Aborted,
        MiscError
    };

    OptionsModel *getOptionsModel();
    AddressTableModel *getAddressTableModel();
    TransactionTableModel *getTransactionTableModel();

    qint64 getBalance() const;
    QString getAddress() const;
    int getNumConnections() const;
    int getNumBlocks() const;
    int getNumTransactions() const;

    /* Return true if core is doing initial block download */
    bool inInitialBlockDownload() const;

    /* Set default address */
    void setAddress(const QString &defaultAddress);
    /* Send coins */
    StatusCode sendCoins(const QString &payTo, qint64 payAmount);
private:
    OptionsModel *optionsModel;
    AddressTableModel *addressTableModel;
    TransactionTableModel *transactionTableModel;

signals:
    void balanceChanged(qint64 balance);
    void addressChanged(const QString &address);
    void numConnectionsChanged(int count);
    void numBlocksChanged(int count);
    void numTransactionsChanged(int count);
    /* Asynchronous error notification */
    void error(const QString &title, const QString &message);

public slots:

private slots:
    void update();
};

#endif // CLIENTMODEL_H
