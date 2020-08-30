// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_CLIENTMODEL_H
#define BITCOIN_QT_CLIENTMODEL_H

#include <evo/deterministicmns.h>
#include <sync.h>

#include <QObject>
#include <QDateTime>

#include <atomic>

class BanTableModel;
class OptionsModel;
class PeerTableModel;

class CBlockIndex;

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

enum class BlockSource {
    NONE,
    REINDEX,
    DISK,
    NETWORK
};

enum NumConnections {
    CONNECTIONS_NONE = 0,
    CONNECTIONS_IN   = (1U << 0),
    CONNECTIONS_OUT  = (1U << 1),
    CONNECTIONS_ALL  = (CONNECTIONS_IN | CONNECTIONS_OUT),
};

/** Model for Dash network client. */
class ClientModel : public QObject
{
    Q_OBJECT

public:
    explicit ClientModel(OptionsModel *optionsModel, QObject *parent = 0);
    ~ClientModel();

    OptionsModel *getOptionsModel();
    PeerTableModel *getPeerTableModel();
    BanTableModel *getBanTableModel();

    //! Return number of connections, default is in- and outbound (total)
    int getNumConnections(unsigned int flags = CONNECTIONS_ALL) const;
    int getNumBlocks() const;
    int getHeaderTipHeight() const;
    int64_t getHeaderTipTime() const;
    //! Return number of transactions in the mempool
    long getMempoolSize() const;
    //! Return the dynamic memory usage of the mempool
    size_t getMempoolDynamicUsage() const;
    //! Return number of ISLOCKs
    size_t getInstantSentLockCount() const;

    void setMasternodeList(const CDeterministicMNList& mnList);
    CDeterministicMNList getMasternodeList() const;
    void refreshMasternodeList();

    quint64 getTotalBytesRecv() const;
    quint64 getTotalBytesSent() const;

    double getVerificationProgress(const CBlockIndex *tip) const;
    QDateTime getLastBlockDate() const;
    QString getLastBlockHash() const;

    //! Return true if core is doing initial block download
    bool inInitialBlockDownload() const;
    //! Returns enum BlockSource of the current importing/syncing state
    enum BlockSource getBlockSource() const;
    //! Return true if network activity in core is enabled
    bool getNetworkActive() const;
    //! Toggle network activity state in core
    void setNetworkActive(bool active);
    //! Return warnings to be displayed in status bar
    QString getStatusBarWarnings() const;

    QString formatFullVersion() const;
    QString formatSubVersion() const;
    bool isReleaseVersion() const;
    QString formatClientStartupTime() const;
    QString dataDir() const;

    // caches for the best header
    mutable std::atomic<int> cachedBestHeaderHeight;
    mutable std::atomic<int64_t> cachedBestHeaderTime;

private:
    OptionsModel *optionsModel;
    PeerTableModel *peerTableModel;
    BanTableModel *banTableModel;

    QTimer *pollTimer;

    // The cache for mn list is not technically needed because CDeterministicMNManager
    // caches it internally for recent blocks but it's not enough to get consistent
    // representation of the list in UI during initial sync/reindex, so we cache it here too.
    mutable CCriticalSection cs_mnlinst; // protects mnListCached
    CDeterministicMNList mnListCached;

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

Q_SIGNALS:
    void numConnectionsChanged(int count);
    void masternodeListChanged() const;
    void numBlocksChanged(int count, const QDateTime& blockDate, const QString& blockHash, double nVerificationProgress, bool header);
    void additionalDataSyncProgressChanged(double nSyncProgress);
    void mempoolSizeChanged(long count, size_t mempoolSizeInBytes);
    void islockCountChanged(size_t count);
    void networkActiveChanged(bool networkActive);
    void alertsChanged(const QString &warnings);
    void bytesChanged(quint64 totalBytesIn, quint64 totalBytesOut);

    //! Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);

    // Show progress dialog e.g. for verifychain
    void showProgress(const QString &title, int nProgress);

public Q_SLOTS:
    void updateTimer();
    void updateNumConnections(int numConnections);
    void updateNetworkActive(bool networkActive);
    void updateAlert();
    void updateBanlist();
};

#endif // BITCOIN_QT_CLIENTMODEL_H
