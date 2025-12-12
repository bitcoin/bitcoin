// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_NET_SIGNING_H
#define BITCOIN_LLMQ_NET_SIGNING_H

#include <net_processing.h>

#include <util/threadinterrupt.h>
#include <util/time.h>

#include <thread>

namespace llmq {
class CSigningManager;
} // namespace llmq

class NetSigning final : public NetHandler
{
public:
    NetSigning(PeerManagerInternal* peer_manager, llmq::CSigningManager& sig_manager) :
        NetHandler(peer_manager),
        m_sig_manager(sig_manager)
    {
        workInterrupt.reset();
    }
    void ProcessMessage(CNode& pfrom, const std::string& msg_type, CDataStream& vRecv) override;

    [[nodiscard]] bool ProcessPendingRecoveredSigs();
    void ProcessRecoveredSig(std::shared_ptr<const llmq::CRecoveredSig> recoveredSig, bool consider_proactive_relay);

    void Start() override;
    void Stop() override;
    void Interrupt() override { workInterrupt(); };

    void WorkThreadMain();

private:
    llmq::CSigningManager& m_sig_manager;

    CleanupThrottler<NodeClock> cleanupThrottler;
    std::thread workThread;
    CThreadInterrupt workInterrupt;
};

#endif // BITCOIN_LLMQ_NET_SIGNING_H
