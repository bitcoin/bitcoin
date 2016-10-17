<<<<<<< HEAD
// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
=======
// Copyright (c) 2011-2013 The Crowncoin developers
>>>>>>> origin/dirty-merge-dash-0.11.0
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_CLIENTMODEL_H
#define BITCOIN_QT_CLIENTMODEL_H

#include <QObject>

class AddressTableModel;
class OptionsModel;
class PeerTableModel;
class TransactionTableModel;

class CWallet;

QT_BEGIN_NAMESPACE
class QDateTime;
class QTimer;
QT_END_NAMESPACE

enum BlockSource {
    BLOCK_SOURCE_NONE,
    BLOCK_SOURCE_REINDEX,
    BLOCK_SOURCE_DISK,
    BLOCK_SOURCE_NETWORK
};

enum NumConnections {
    CONNECTIONS_NONE = 0,
    CONNECTIONS_IN   = (1U << 0),
    CONNECTIONS_OUT  = (1U << 1),
    CONNECTIONS_ALL  = (CONNECTIONS_IN | CONNECTIONS_OUT),
};

<<<<<<< HEAD
/** Model for Dash network client. */
=======
/** Model for Crowncoin network client. */
>>>>>>> origin/dirty-merge-dash-0.11.0
class ClientModel : public QObject
{
    Q_OBJECT

public:
    explicit ClientModel(OptionsModel *optionsModel, QObject *parent = 0);
    ~ClientModel();

    OptionsModel *getOptionsModel();
    PeerTableModel *getPeerTableModel();

    //! Return number of connections, default is in- and outbound (total)
    int getNumConnections(unsigned int flags = CONNECTIONS_ALL) const;
<<<<<<< HEAD
    QString getMasternodeCountString() const;
=======
    QString getThroneCountString() const;
>>>>>>> origin/dirty-merge-dash-0.11.0
    int getNumBlocks() const;
    int getNumBlocksAtStartup();

    quint64 getTotalBytesRecv() const;
    quint64 getTotalBytesSent() const;

    double getVerificationProgress() const;
    QDateTime getLastBlockDate() const;

    //! Return true if core is doing initial block download
    bool inInitialBlockDownload() const;
    //! Return true if core is importing blocks
    enum BlockSource getBlockSource() const;
    //! Return warnings to be displayed in status bar
    QString getStatusBarWarnings() const;

    QString formatFullVersion() const;
    QString formatBuildDate() const;
    bool isReleaseVersion() const;
    QString clientName() const;
    QString formatClientStartupTime() const;

private:
    OptionsModel *optionsModel;
    PeerTableModel *peerTableModel;

    int cachedNumBlocks;
<<<<<<< HEAD
    QString cachedMasternodeCountString;
=======
    QString cachedThroneCountString;
>>>>>>> origin/dirty-merge-dash-0.11.0
    bool cachedReindexing;
    bool cachedImporting;

    int numBlocksAtStartup;

    QTimer *pollTimer;
    QTimer *pollMnTimer;

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

signals:
    void numConnectionsChanged(int count);
    void numBlocksChanged(int count);
<<<<<<< HEAD
    void strMasternodesChanged(const QString &strMasternodes);
=======
    void strThronesChanged(const QString &strThrones);
>>>>>>> origin/dirty-merge-dash-0.11.0
    void alertsChanged(const QString &warnings);
    void bytesChanged(quint64 totalBytesIn, quint64 totalBytesOut);

    //! Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);

    // Show progress dialog e.g. for verifychain
    void showProgress(const QString &title, int nProgress);

public slots:
    void updateTimer();
    void updateMnTimer();
    void updateNumConnections(int numConnections);
    void updateAlert(const QString &hash, int status);
};

#endif // BITCOIN_QT_CLIENTMODEL_H
