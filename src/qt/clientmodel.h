#ifndef CLIENTMODEL_H
#define CLIENTMODEL_H

#include <QObject>

class OptionsModel;
class AddressTableModel;
class TransactionTableModel;
class CWallet;

// Interface to Bitcoin network client
class ClientModel : public QObject
{
    Q_OBJECT
public:
    // The only reason that this constructor takes a wallet is because
    // the global client settings are stored in the main wallet.
    explicit ClientModel(CWallet *wallet, QObject *parent = 0);

    OptionsModel *getOptionsModel();

    int getNumConnections() const;
    int getNumBlocks() const;

    // Return true if client connected to testnet
    bool isTestNet() const;
    // Return true if core is doing initial block download
    bool inInitialBlockDownload() const;
    // Return conservative estimate of total number of blocks, or 0 if unknown
    int getTotalBlocksEstimate() const;

    QString formatFullVersion() const;

private:
    CWallet *wallet;

    OptionsModel *optionsModel;

signals:
    void numConnectionsChanged(int count);
    void numBlocksChanged(int count);

    // Asynchronous error notification
    void error(const QString &title, const QString &message);

public slots:

private slots:
    void update();
};

#endif // CLIENTMODEL_H
