#ifndef CLIENTMODEL_H
#define CLIENTMODEL_H

#include <QObject>

class ClientModel : public QObject
{
    Q_OBJECT
public:
    explicit ClientModel(QObject *parent = 0);

    qint64 getBalance();
    QString getAddress();
    int getNumConnections();
    int getNumBlocks();
    int getNumTransactions();

signals:
    void balanceChanged(qint64 balance);
    void addressChanged(const QString &address);
    void numConnectionsChanged(int count);
    void numBlocksChanged(int count);
    void numTransactionsChanged(int count);

public slots:

private slots:
    void update();
};

#endif // CLIENTMODEL_H
