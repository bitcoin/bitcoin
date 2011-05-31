#ifndef CLIENTMODEL_H
#define CLIENTMODEL_H

#include <QObject>
class OptionsModel;

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

    qint64 getBalance();
    QString getAddress();
    int getNumConnections();
    int getNumBlocks();
    int getNumTransactions();

    qint64 getTransactionFee();

    StatusCode sendCoins(const QString &payTo, qint64 payAmount);
private:
    OptionsModel *options_model;

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
