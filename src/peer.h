#ifndef BITCOIN_PEER_H
#define BITCOIN_PEER_H

#include <boost/signals2/signal.hpp>

#include <atomic>
#include <list>
#include <map>
#include <mutex>
#include <string>

class CBlockIndex;
typedef int NodeId;
typedef int64_t CAmount;

class Peer;

namespace BlockAccountability {
    namespace {
        std::mutex csBlockRejects;
    }
    struct CBlockReject {
        const CBlockIndex* pindex;
        uint8_t chRejectCode;
        std::string strRejectReason;
        inline CBlockReject(const CBlockIndex* pindexIn, uint8_t chRejectCodeIn, std::string&& strRejectReasonIn)
            : pindex(pindexIn), chRejectCode(chRejectCodeIn), strRejectReason(std::move(strRejectReasonIn)) {}
    };
    class PeerInfo {
        typedef std::lock_guard<std::mutex> lock_guard;
    public:
        struct Params {};
        inline PeerInfo(const Params& in) : fHaveRejects(false), _vRejects() {}
        inline std::list<CBlockReject> GetBlockRejections() {
            std::list<CBlockReject> ret;
            if (fHaveRejects.load(std::memory_order_relaxed)) {
                lock_guard guard{csBlockRejects};
                ret.swap(_vRejects);
                fHaveRejects.store(false, std::memory_order_relaxed);
            }
            return ret;
        }
        template <class... Args> void RejectBlock(Args&&... args) {
            lock_guard guard{csBlockRejects};
            _vRejects.emplace_back(std::forward<Args>(args)...);
            fHaveRejects.store(true, std::memory_order_relaxed);
        }
    private:
        // flag to skip the mutex for the common case
        std::atomic<bool> fHaveRejects;
        std::list<CBlockReject> _vRejects;
    };
} // namespace BlockAccountability

namespace Misbehavior {
    namespace {
        int _banscore = 0;
    }
    // only set during initial setup
    inline void _SetBanscore(int score) { _banscore = score; }
    inline int GetBanscore() { return _banscore; }
    inline bool IsExcessive(int score) { return score >= GetBanscore(); }

    /// Hook for logging etc. when a peer misbehaves.
    struct Signals {
        boost::signals2::signal<void(const Peer&, int, int)> OnMisbehavior;
    };
    inline Signals& GetSignals() { static Signals s{}; return s; }

    struct PeerInfo {
        struct Params {
            inline Params() : fNoBan(false), fNoDisconnect(false) {}
            bool fNoBan;
            bool fNoDisconnect;
        };
        inline PeerInfo(const Params& in)
            : noDisconnect(in.fNoDisconnect)
            , noBan(in.fNoBan)
            , badness(0)
            , fExceeded(false)
        {}

        inline int GetMisbehavior() const { return badness.load(std::memory_order_relaxed); }
        int Misbehaving(int delta);

        /// If a call to Misbehaving causes a peer's misbehavior to exceed GetBanscore(),
        /// this function will return true exactly once. Otherwise returns false.
        inline bool ExceededMisbehaviorThreshold() {
            // Quick check for the usual case.
            if (!fExceeded.load(std::memory_order_relaxed))
                return false;
            // Reread in case multiple concurrent callers.
            // Release-acquire sync with Misbehaving() so if we return true,
            // subsequent GetMisbehavior() will return at least GetBanscore().
            return fExceeded.exchange(false, std::memory_order_acquire);
        }

        inline bool CanDisconnect() const { return !noDisconnect; }
        inline bool CanBan() const { return !noBan; }
    private:
        const bool noDisconnect;
        const bool noBan;
        std::atomic<int> badness;
        std::atomic<bool> fExceeded;
    };
} // namespace Misbehavior

struct HeaderSync {
    struct PeerInfo {
        friend class HeaderSync;
        struct Params {};
        inline PeerInfo(const Params& in)
            : fPreferHeaders(false)
            , fSyncStarted(false)
        {}

        inline bool PrefersHeaders() const { return fPreferHeaders.load(std::memory_order_relaxed); }
        inline void SetPrefersHeaders() { fPreferHeaders.store(true, std::memory_order_relaxed); }
        inline bool IsSyncStarted() const { return fSyncStarted.load(std::memory_order_relaxed); }
    private:
        // sets fSyncStarted to true exactly once
        // returns whether successfully changed
        inline bool SetStarted() { return fSyncStarted.exchange(true, std::memory_order_relaxed); }

        //! Whether the peer prefers headers to block invs
        std::atomic<bool> fPreferHeaders;
        //! Whether we've started headers synchronization with this peer.
        std::atomic<bool> fSyncStarted;
    };
    inline HeaderSync() : fHaveSyncStarted(false) {}
    /// Sets sync started, unless sync is already started.
    /// Returns whether SyncStarted has been changed.
    inline bool Start(PeerInfo& peer) {
        // unconditional store - SetStarted only fails when the peer is already started
        fHaveSyncStarted.store(true, std::memory_order_relaxed);
        return peer.SetStarted();
    }
    /// Sets sync started, if no peers are already syncing.
    /// Returns whether the peer has been set to started.
    inline bool StartFirst(PeerInfo& peer) {
        // quick check for the common case
        if (fHaveSyncStarted.load(std::memory_order_relaxed) == true)
            return false;
        // if concurrent StartFirsts, only 1 may pass
        if (fHaveSyncStarted.exchange(true, std::memory_order_relaxed) == true)
            return false;
        // will succeed unless concurrent unconditional Start for the same peer
        return peer.SetStarted();
    }
private:
    std::atomic<bool> fHaveSyncStarted;
};

namespace TxRelay {
    struct PeerInfo {
        struct Params {
            inline Params() : fAlwaysRelayTx(false) {}
            bool fAlwaysRelayTx;
        };
        inline PeerInfo(const Params& in) : fRelayTx(false), amtMinFeeFilter(0), fAlwaysRelayTx(in.fAlwaysRelayTx) {}

        inline void SetRelayTx(bool value) { fRelayTx.store(value, std::memory_order_relaxed); }
        inline bool CanRelayTx() { return fRelayTx.load(std::memory_order_relaxed); }

        inline bool AlwaysRelayTx() { return fAlwaysRelayTx; }

        inline void SetMinFeeFilter(CAmount value) { return amtMinFeeFilter.store(value, std::memory_order_relaxed); }
        inline CAmount GetMinFeeFilter() const { return amtMinFeeFilter.load(std::memory_order_relaxed); }
    private:
        std::atomic<bool> fRelayTx;
        std::atomic<CAmount> amtMinFeeFilter;
        const bool fAlwaysRelayTx;
    };
} // namespace TxRelay

enum class DownloadPreferredness { BELOW_NORMAL, NORMAL, ABOVE_NORMAL };

/// Peer holds high-level information about a peer.
class Peer
    : public BlockAccountability::PeerInfo
    , public Misbehavior::PeerInfo
    , public HeaderSync::PeerInfo
    , public TxRelay::PeerInfo
{
    friend class PeerManager;
public:
    struct Params
        : public BlockAccountability::PeerInfo::Params
        , public Misbehavior::PeerInfo::Params
        , public HeaderSync::PeerInfo::Params
        , public TxRelay::PeerInfo::Params
    {
        friend class Peer;
        inline Params(NodeId idIn, const std::string& strNameIn)
            : preferedness(DownloadPreferredness::NORMAL)
            , id(idIn)
            , strName(strNameIn)
        {}
        DownloadPreferredness preferedness;
    private:
        const NodeId id;
        std::string strName;
    };

    // Applicable to a peer that has finished connecting (by sending its version
    // message, though the handshake may not yet have been completed with a verack).
    struct ConnectedInfo {
        inline ConnectedInfo() : fEnableDownload(false), fPingSupported(false) {}

        inline bool CanDownload() const { return fEnableDownload; }
        inline bool IsPingSupported() const { return fPingSupported; }

        inline void SetCanDownload(bool value) { fEnableDownload = value; }
        inline void SetPingSupported(bool value) { fPingSupported = value; }
    private:
        // These properties can only be written before giving the ConnectedInfo
        // to SetConnected; once they're accessible through GetConnectedInfo,
        // only read access is allowed.
        bool fEnableDownload;
        bool fPingSupported;
    };

    inline Peer(Params&& in)
        : BlockAccountability::PeerInfo(in)
        , Misbehavior::PeerInfo(in)
        , HeaderSync::PeerInfo(in)
        , TxRelay::PeerInfo(in)
        , id(in.id)
        , strName(std::move(in.strName))
        , preferedness(in.preferedness)
        , pconnected(nullptr)
    {}
    inline ~Peer() { delete pconnected.load(std::memory_order_consume); }

    inline NodeId GetId() const { return id; }
    inline const std::string& GetName() const { return strName; }
    inline bool operator!=(const Peer& other) const { return GetId() != other.GetId(); }
    inline bool operator==(const Peer& other) const { return GetId() == other.GetId(); }

    inline void SetHandshakeCompleted() { fHandshook.store(true, std::memory_order_relaxed); }
    inline bool IsHandshakeCompleted() { return fHandshook.load(std::memory_order_relaxed); }

    [[carries_dependency]] inline const ConnectedInfo* GetConnectedInfo() const {
        // Release-consume because consumer must not observe pointer value that
        // is more recent than the value written to it.
        return pconnected.load(std::memory_order_consume);
    }
private:
    inline void SetConnectedInfo(std::unique_ptr<ConnectedInfo>&& pnew) {
        return pconnected.store(pnew.release(), std::memory_order_release);
    }

    inline DownloadPreferredness GetPreferredness() const { return preferedness; }

    const NodeId id;
    const std::string strName;
    //! Whether we have a fully-established connection.
    std::atomic<bool> fHandshook;
    //! Whether we consider this a preferred download peer.
    const DownloadPreferredness preferedness;
    std::atomic<ConnectedInfo*> pconnected;
};

class PeerManager {
    typedef std::lock_guard<std::mutex> lock_guard;
public:
    inline void SetConnected(Peer& peer, std::unique_ptr<Peer::ConnectedInfo>&& pconnected) {
        if (pconnected->CanDownload()) {
            if (peer.GetPreferredness() == DownloadPreferredness::ABOVE_NORMAL)
                nPreferredDownload.fetch_add(1, std::memory_order_relaxed);
        }
        peer.SetConnectedInfo(std::move(pconnected));
    }

    void Register(NodeId id, std::shared_ptr<Peer>&& peer);
    void Unregister(NodeId id);
    std::shared_ptr<Peer> Get(NodeId id);

    inline void Clear() {
        lock_guard guard{csById};
        _byId.clear();
    }

    bool IsPreferred(Peer& peer) const;
private:
    std::atomic<uint32_t> nPreferredDownload;

    std::mutex csById;
    std::map<NodeId, std::shared_ptr<Peer>> _byId;
};

#endif // BITCOIN_PEER_H
