#include <ecai/proof.h>
#include <util/system.h>
#include <hash.h>
#include <zmq/zmqpublishnotifier.h> // already conditionally compiled in Core
#include <node/transaction.h>
#include "ecai/api.h"
#include <hash.h>
#include <serialize.h>


extern CZMQNotificationInterface* g_zmq_notification_interface;

namespace ecai {

static uint256 MsgHash(const uint256& txid, const uint256& pid, unsigned char vtag,
                       const std::array<unsigned char,33>& cpol,
                       const std::array<unsigned char,33>& ptx,
                       const Ctx& c)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << txid << pid << vtag;
    ss.write((const char*)cpol.data(), 33);
    ss.write((const char*)ptx.data(), 33);
    ss << c.height << c.mediantime;
    return ss.GetHash();
}

// For simplicity we do not wire a node key in this snippet (leave sig zeros)
bool SignAndAssemble(const uint256& txid, const uint256& pid, unsigned char vtag,
                     const std::array<unsigned char,33>& cpol,
                     const std::array<unsigned char,33>& ptx,
                     const Ctx& ctx, Proof& out)
{
    out.txid = txid; out.policy_id = pid; out.verdict_tag = vtag;
    out.c_pol = cpol; out.p_tx = ptx; out.ctx = ctx;
    // If you want real signing: plumb a Schnorr key here and fill out.sig
    memset(out.sig.data(), 0, out.sig.size());
    return true;
}
bool SignAndAssemble(const Txid& txid, Proof& out)
{
    // Convert the strong-typed Txid to a stable 256-bit digest.
    // SerializeHash works with Core's identifier types.
    const uint256 h = SerializeHash(txid);
    return SignAndAssemble(h, out);
}
	bool SignAndAssemble(const Txid& txid,
                     int /*policy_id*/,
                     unsigned char /*verdict*/,
                     int /*c_pol*/,
                     const std::array<unsigned char, 33>& /*p_tx*/,
                     const Ctx& /*ctx*/,
                     Proof& out)
{
    // For now, ignore the extra metadata and reuse your core implementation.
    // Later, you can embed these fields into `Proof` if you want.
    return SignAndAssemble(txid, out);
void Publish(const Proof& p)
{
#ifdef ENABLE_ZMQ
    if (!g_zmq_notification_interface) return;
    std::vector<unsigned char> payload;
    payload.reserve(32+32+1+33+33+4+8+64);
    payload.insert(payload.end(), p.txid.begin(), p.txid.end());
    payload.insert(payload.end(), p.policy_id.begin(), p.policy_id.end());
    payload.push_back(p.verdict_tag);
    payload.insert(payload.end(), p.c_pol.begin(), p.c_pol.end());
    payload.insert(payload.end(), p.p_tx.begin(), p.p_tx.end());
    // ctx
    for (int i=3;i>=0;--i) payload.push_back((unsigned char)((p.ctx.height>>(i*8))&0xFF));
    for (int i=7;i>=0;--i) payload.push_back((unsigned char)((p.ctx.mediantime>>(i*8))&0xFF));
    payload.insert(payload.end(), p.sig.begin(), p.sig.end());
    g_zmq_notification_interface->SendRawMessage("ecai.verdict", payload);
#endif
}

} // namespace ecai
