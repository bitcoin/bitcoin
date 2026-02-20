// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_CLIENTFEEDS_H
#define BITCOIN_QT_CLIENTFEEDS_H

#include <interfaces/node.h>
#include <saltedhasher.h>
#include <sync.h>
#include <threadsafety.h>

#include <QObject>
#include <QTimer>

#include <atomic>
#include <cassert>
#include <memory>
#include <vector>

QT_BEGIN_NAMESPACE
class QThread;
QT_END_NAMESPACE

class ClientModel;
class MasternodeEntry;
class Proposal;

class FeedBase : public QObject
{
    Q_OBJECT

    friend class ClientFeeds;

public:
    struct Config {
        std::chrono::milliseconds m_baseline;
        std::chrono::milliseconds m_throttle;
    };

    explicit FeedBase(QObject* parent, const Config& config);
    virtual ~FeedBase();

    void setSyncing(bool syncing) { m_syncing.store(syncing); }
    const Config& config() const { return m_config; }

    virtual void fetch() = 0;
    void requestForceRefresh();
    void requestRefresh();

Q_SIGNALS:
    void dataReady();

protected:
    Config m_config;
    QTimer* m_timer{nullptr};
    std::atomic<bool> m_in_progress{false};
    std::atomic<bool> m_retry_pending{false};
    std::atomic<bool> m_syncing{true};
};

template<typename T>
class Feed : public FeedBase
{
public:
    using Data = T;

    explicit Feed(QObject* parent, const Config& config) :
        FeedBase{parent, config}
    {
    }
    ~Feed() override = default;

    std::shared_ptr<const Data> data() const EXCLUSIVE_LOCKS_REQUIRED(!m_cs)
    {
        LOCK(m_cs);
        return m_data;
    }

    // FeedBase
    void fetch() override EXCLUSIVE_LOCKS_REQUIRED(!m_cs) = 0;

protected:
    void setData(std::shared_ptr<const Data> data) EXCLUSIVE_LOCKS_REQUIRED(!m_cs)
    {
        LOCK(m_cs);
        m_data = std::move(data);
    }

private:
    mutable Mutex m_cs;
    std::shared_ptr<const Data> m_data GUARDED_BY(m_cs);
};

using MasternodeEntryList = std::vector<std::shared_ptr<MasternodeEntry>>;

struct MasternodeData {
    bool m_valid{false};
    int m_list_height{0};
    MasternodeEntryList m_entries;
};

class MasternodeFeed : public Feed<MasternodeData> {
    Q_OBJECT

public:
    explicit MasternodeFeed(QObject* parent, ClientModel& client_model);
    ~MasternodeFeed();

    void fetch() override;

private:
    ClientModel& m_client_model;
};

using Proposals = std::vector<std::shared_ptr<Proposal>>;

struct ProposalData {
    int m_abs_vote_req{0};
    interfaces::GOV::GovernanceInfo m_gov_info;
    Proposals m_proposals;
    Uint256HashSet m_fundable_hashes;
};

class ProposalFeed : public Feed<ProposalData> {
    Q_OBJECT

public:
    explicit ProposalFeed(QObject* parent, ClientModel& client_model);
    ~ProposalFeed();

    void fetch() override;

private:
    ClientModel& m_client_model;
};

class ClientFeeds : public QObject
{
    Q_OBJECT

public:
    explicit ClientFeeds(QObject* parent);
    ~ClientFeeds();

    template<typename Source, typename... Args>
    Source* add(Args&&... args) {
        assert(!m_stopped);
        auto source = std::make_unique<Source>(std::forward<Args>(args)...);
        Source* ptr = source.get();
        registerFeed(ptr);
        m_sources.push_back(std::move(source));
        return ptr;
    }

    void setSyncing(bool syncing);
    void start();
    void stop();

private:
    void registerFeed(FeedBase* source);

private:
    bool m_stopped{false};
    QObject* m_worker{nullptr};
    QThread* m_thread{nullptr};
    std::vector<std::unique_ptr<FeedBase>> m_sources{};
};

#endif // BITCOIN_QT_CLIENTFEEDS_H
