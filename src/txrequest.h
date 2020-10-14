// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXREQUEST_H
#define BITCOIN_TXREQUEST_H

#include <primitives/transaction.h>
#include <net.h> // For NodeId
#include <uint256.h>

#include <chrono>
#include <vector>

#include <stdint.h>

/** Data structure to keep track of, and schedule, transaction downloads from peers.
 *
 * === Specification ===
 *
 * We keep track of which peers have announced which transactions, and use that to determine which requests
 * should go to which peer, when, and in what order.
 *
 * The following information is tracked per peer/tx combination ("announcement"):
 * - Which peer announced it (through their NodeId)
 * - The txid or wtxid of the transaction (collectively called "txhash" in what follows)
 * - Whether it was a tx or wtx announcement (see BIP339).
 * - What the earliest permitted time is that that transaction can be requested from that peer (called "reqtime").
 * - Whether it's from a "preferred" peer or not. Which announcements get this flag is determined by the caller, but
 *   this is designed for outbound peers, or other peers that we have a higher level of trust in. Even when the
 *   peers' preferredness changes, the preferred flag of existing announcements from that peer won't change.
 * - Whether or not the transaction was requested already, and if so, when it times out (called "expiry").
 * - Whether or not the transaction request failed already (timed out, or invalid transaction or NOTFOUND was
 *   received).
 *
 * Transaction requests are then assigned to peers, following these rules:
 *
 * - No transaction is requested as long as another request for the same txhash is outstanding (it needs to fail
 *   first by passing expiry, or a NOTFOUND or invalid transaction has to be received for it).
 *
 *   Rationale: to avoid wasting bandwidth on multiple copies of the same transaction. Note that this only works
 *              per txhash, so if the same transaction is announced both through txid and wtxid, we have no means
 *              to prevent fetching both (the caller can however mitigate this by delaying one, see further).
 *
 * - The same transaction is never requested twice from the same peer, unless the announcement was forgotten in
 *   between, and re-announced. Announcements are forgotten only:
 *   - If a peer goes offline, all its announcements are forgotten.
 *   - If a transaction has been successfully received, or is otherwise no longer needed, the caller can call
 *     ForgetTxHash, which removes all announcements across all peers with the specified txhash.
 *   - If for a given txhash only already-failed announcements remain, they are all forgotten.
 *
 *   Rationale: giving a peer multiple chances to announce a transaction would allow them to bias requests in their
 *              favor, worsening transaction censoring attacks. The flip side is that as long as an attacker manages
 *              to prevent us from receiving a transaction, failed announcements (including those from honest peers)
 *              will linger longer, increasing memory usage somewhat. The impact of this is limited by imposing a
 *              cap on the number of tracked announcements per peer. As failed requests in response to announcements
 *              from honest peers should be rare, this almost solely hinders attackers.
 *              Transaction censoring attacks can be done by announcing transactions quickly while not answering
 *              requests for them. See https://allquantor.at/blockchainbib/pdf/miller2015topology.pdf for more
 *              information.
 *
 * - Transactions are not requested from a peer until its reqtime has passed.
 *
 *   Rationale: enable the calling code to define a delay for less-than-ideal peers, so that (presumed) better
 *              peers have a chance to give their announcement first.
 *
 * - If multiple viable candidate peers exist according to the above rules, pick a peer as follows:
 *
 *   - If any preferred peers are available, non-preferred peers are not considered for what follows.
 *
 *     Rationale: preferred peers are more trusted by us, so are less likely to be under attacker control.
 *
 *   - Pick a uniformly random peer among the candidates.
 *
 *     Rationale: random assignments are hard to influence for attackers.
 *
 * Together these rules strike a balance between being fast in non-adverserial conditions and minimizing
 * susceptibility to censorship attacks. An attacker that races the network:
 * - Will be unsuccessful if all preferred connections are honest (and there is at least one preferred connection).
 * - If there are P preferred connections of which Ph>=1 are honest, the attacker can delay us from learning
 *   about a transaction by k expiration periods, where k ~ 1 + NHG(N=P-1,K=P-Ph-1,r=1), which has mean
 *   P/(Ph+1) (where NHG stands for Negative Hypergeometric distribution). The "1 +" is due to the fact that the
 *   attacker can be the first to announce through a preferred connection in this scenario, which very likely means
 *   they get the first request.
 * - If all P preferred connections are to the attacker, and there are NP non-preferred connections of which NPh>=1
 *   are honest, where we assume that the attacker can disconnect and reconnect those connections, the distribution
 *   becomes k ~ P + NB(p=1-NPh/NP,r=1) (where NB stands for Negative Binomial distribution), which has mean
 *   P-1+NP/NPh.
 *
 * Complexity:
 * - Memory usage is proportional to the total number of tracked announcements (Size()) plus the number of
 *   peers with a nonzero number of tracked announcements.
 * - CPU usage is generally logarithmic in the total number of tracked announcements, plus the number of
 *   announcements affected by an operation (amortized O(1) per announcement).
 */
class TxRequestTracker {
    // Avoid littering this header file with implementation details.
    class Impl;
    const std::unique_ptr<Impl> m_impl;

public:
    //! Construct a TxRequestTracker.
    explicit TxRequestTracker(bool deterministic = false);
    ~TxRequestTracker();

    // Conceptually, the data structure consists of a collection of "announcements", one for each peer/txhash
    // combination:
    //
    // - CANDIDATE announcements represent transactions that were announced by a peer, and that become available for
    //   download after their reqtime has passed.
    //
    // - REQUESTED announcements represent transactions that have been requested, and which we're awaiting a
    //   response for from that peer. Their expiry value determines when the request times out.
    //
    // - COMPLETED announcements represent transactions that have been requested from a peer, and a NOTFOUND or a
    //   transaction was received in response (valid or not), or they timed out. They're only kept around to
    //   prevent requesting them again. If only COMPLETED announcements for a given txhash remain (so no CANDIDATE
    //   or REQUESTED ones), all of them are deleted (this is an invariant, and maintained by all operations below).
    //
    // The operations below manipulate the data structure.

    /** Adds a new CANDIDATE announcement.
     *
     * Does nothing if one already exists for that (txhash, peer) combination (whether it's CANDIDATE, REQUESTED, or
     * COMPLETED). Note that the txid/wtxid property is ignored for determining uniqueness, so if an announcement
     * is added for a wtxid H, while one for txid H from the same peer already exists, it will be ignored. This is
     * harmless as the txhashes being equal implies it is a non-segwit transaction, so it doesn't matter how it is
     * fetched. The new announcement is given the specified preferred and reqtime values, and takes its is_wtxid
     * from the specified gtxid.
     */
    void ReceivedInv(NodeId peer, const GenTxid& gtxid, bool preferred,
        std::chrono::microseconds reqtime);

    /** Deletes all announcements for a given peer.
     *
     * It should be called when a peer goes offline.
     */
    void DisconnectedPeer(NodeId peer);

    /** Deletes all announcements for a given txhash (both txid and wtxid ones).
     *
     * This should be called when a transaction is no longer needed. The caller should ensure that new announcements
     * for the same txhash will not trigger new ReceivedInv calls, at least in the short term after this call.
     */
    void ForgetTxHash(const uint256& txhash);

    /** Find the txids to request now from peer.
     *
     * It does the following:
     *  - Convert all REQUESTED announcements (for all txhashes/peers) with (expiry <= now) to COMPLETED ones.
     *    These are returned in expired, if non-nullptr.
     *  - Requestable announcements are selected: CANDIDATE announcements from the specified peer with
     *    (reqtime <= now) for which no existing REQUESTED announcement with the same txhash from a different peer
     *    exists, and for which the specified peer is the best choice among all (reqtime <= now) CANDIDATE
     *    announcements with the same txhash (subject to preferredness rules, and tiebreaking using a deterministic
     *    salted hash of peer and txhash).
     *  - The selected announcements are converted to GenTxids using their is_wtxid flag, and returned in
     *    announcement order (even if multiple were added at the same time, or when the clock went backwards while
     *    they were being added). This is done to minimize disruption from dependent transactions being requested
     *    out of order: if multiple dependent transactions are announced simultaneously by one peer, and end up
     *    being requested from them, the requests will happen in announcement order.
     */
    std::vector<GenTxid> GetRequestable(NodeId peer, std::chrono::microseconds now,
        std::vector<std::pair<NodeId, GenTxid>>* expired = nullptr);

    /** Marks a transaction as requested, with a specified expiry.
     *
     * If no CANDIDATE announcement for the provided peer and txhash exists, this call has no effect. Otherwise:
     *  - That announcement is converted to REQUESTED.
     *  - If any other REQUESTED announcement for the same txhash already existed, it means an unexpected request
     *    was made (GetRequestable will never advise doing so). In this case it is converted to COMPLETED, as we're
     *    no longer waiting for a response to it.
     */
    void RequestedTx(NodeId peer, const uint256& txhash, std::chrono::microseconds expiry);

    /** Converts a CANDIDATE or REQUESTED announcement to a COMPLETED one. If no such announcement exists for the
     *  provided peer and txhash, nothing happens.
     *
     * It should be called whenever a transaction or NOTFOUND was received from a peer. When the transaction is
     * not needed entirely anymore, ForgetTxhash should be called instead of, or in addition to, this call.
     */
    void ReceivedResponse(NodeId peer, const uint256& txhash);

    // The operations below inspect the data structure.

    /** Count how many REQUESTED announcements a peer has. */
    size_t CountInFlight(NodeId peer) const;

    /** Count how many CANDIDATE announcements a peer has. */
    size_t CountCandidates(NodeId peer) const;

    /** Count how many announcements a peer has (REQUESTED, CANDIDATE, and COMPLETED combined). */
    size_t Count(NodeId peer) const;

    /** Count how many announcements are being tracked in total across all peers and transaction hashes. */
    size_t Size() const;

    /** Access to the internal priority computation (testing only) */
    uint64_t ComputePriority(const uint256& txhash, NodeId peer, bool preferred) const;

    /** Run internal consistency check (testing only). */
    void SanityCheck() const;

    /** Run a time-dependent internal consistency check (testing only).
     *
     * This can only be called immediately after GetRequestable, with the same 'now' parameter.
     */
    void PostGetRequestableSanityCheck(std::chrono::microseconds now) const;
};

#endif // BITCOIN_TXREQUEST_H
