#include "peer.h"

int Misbehavior::PeerInfo::Misbehaving(int delta) {
    int badnessBefore = badness.fetch_add(delta, std::memory_order_relaxed);
    int badnessAfter = badnessBefore + delta;
    GetSignals().OnMisbehavior(static_cast<const Peer&>(*this), badnessBefore, badnessAfter);
    if (IsExcessive(badnessAfter) && !IsExcessive(badnessBefore))
        fExceeded.store(true, std::memory_order_release);
    return badnessAfter;
}

void PeerManager::Register(NodeId id, std::shared_ptr<Peer>&& peer) {
    lock_guard guard{csById};
    auto ret = _byId.emplace(std::piecewise_construct,
                            std::forward_as_tuple(id),
                            std::forward_as_tuple(peer));
    assert(ret.second);
}

void PeerManager::Unregister(NodeId id) {
    lock_guard guard{csById};
    auto it = _byId.find(id);
    assert(it != _byId.end());
    {
        std::shared_ptr<Peer> ppeer = it->second;
        const Peer::ConnectedInfo* pconnected = ppeer->GetConnectedInfo();
        if (pconnected && pconnected->CanDownload() && ppeer->GetPreferredness() == DownloadPreferredness::ABOVE_NORMAL)
            nPreferredDownload.fetch_sub(1, std::memory_order_relaxed);
    }
    _byId.erase(it);
}

std::shared_ptr<Peer> PeerManager::Get(NodeId id) {
    lock_guard guard{csById};
    auto it = _byId.find(id);
    if (it == _byId.end())
        return { nullptr };
    else
        return it->second;
}

bool PeerManager::IsPreferred(Peer& peer) const {
    switch (peer.GetPreferredness()) {
    case DownloadPreferredness::ABOVE_NORMAL: return true;
    case DownloadPreferredness::BELOW_NORMAL: return false;
    case DownloadPreferredness::NORMAL: return nPreferredDownload.load(std::memory_order_relaxed) == 0;
    default: assert(!"unreachable!");
    }
}
