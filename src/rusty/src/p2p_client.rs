use std::collections::LinkedList;
use std::panic::catch_unwind;
use std::sync::{Arc, Condvar, Mutex};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::time::{Duration, Instant, SystemTime};

use std::ffi::{CStr, c_void};
use std::os::raw::c_char;

use crate::bridge::*;
use crate::await_ibd_complete_or_stalled;
use crate::p2p_addrs::*;
use crate::p2p_socket_handler::*;

use bitcoin::BitcoinHash;
use bitcoin::blockdata::block::BlockHeader;
use bitcoin::consensus::Decodable;
use bitcoin::consensus::encode::deserialize;
use bitcoin::network::address;
use bitcoin::network::message::NetworkMessage;
use bitcoin::network::message_network::VersionMessage;
use bitcoin::network::message_blockdata::{GetHeadersMessage, Inventory, InvType};

use bitcoin_hashes::Hash as HashTrait;
use bitcoin_hashes::sha256d::Hash;

///! A (relatively) simple P2P blocks-only implementation. The goal here is explicitly *not* to be
///! efficient (modulo a few obvious fixes), but instead to be simple, and thus robust and easy to
///! understand. That said, we can't, in the common case, blow up bandwidth usage. An easy cheat,
///! however, to avoid it, is to simply wait some number of seconds (BLOCK_REQ_DELAY, below) before
///! downloading any blocks once we've decided they should be requested. This gives the,
///! potentially more effecient, C++ client a chance to fetch the block before we do it.
///! Additionally, we wait until the C++ client has gotten us out of IBD (or it seems to have
///! stalled and isn't making progress) before we do anything at all. Further, we, with the
///! exception of any common state held in the C++ codebase, don't keep any per-peer state.

static THREAD_COUNT: AtomicUsize = AtomicUsize::new(0);

/// If a headers message contains this many headers, it means we should ask for more headers
/// starting from the last one in the message. This is only an optimization as we'll still
/// regularly poll all our peers with getheaders messages.
const HEADERS_LEN_CONTINUE: usize = 2000;
const PROTOCOL_VERSION: u32 = 70015;

/// Check for new blocks to download regularly at this interval, and also delay requesting blocks
/// for this amount of time after we decide we should, in the hopes someone else fetches it in a
/// more efficient manner.
const BLOCK_REQ_DELAY: Duration = Duration::from_secs(29);

/// Interval at which we poll our peers for their latest header.
const HEADER_POLL_INTERVAL: Duration = Duration::from_secs(31);

/// Interval at which we send regular pings to peers (and at which point they must have responded
/// to the previous ping with a pong).
const PING_INTERVAL: Duration = Duration::from_secs(23);

/// state that gets wrapped in an Arc to pass incoming and outgoing messages into/out of the socket
/// handling thread.
struct PeerState {
    queues: Arc<MessageQueues>,
    remote_addr: NetAddress,
    is_outbound: bool,
    /// The random nonce we send in our version message to detect connecting-back-to-ourselves.
    /// Only set for outbound peers and only kept around until version handshake completes.
    outbound_nonce: Option<OutboundP2PNonce>,
    recvd_ver: bool,
    recvd_verack: bool,
    recvd_sendheaders: bool,
    /// We use the C++ block-fetching logic to get its reorg-robustness. However, we *also* try to
    /// be at least partially robust against bugs therein, if only for non-reorg paths.
    block_state: BlockProviderState,
    /// The next time we should poll this peer for their current header tip
    next_header_poll: Instant,
    /// The next block we (may) request if we still don't have it at next_block_request_time.
    next_block_request: Option<BlockIndex>,
    /// We wait a bit after we hear about a block before requesting it, in the hopes that the C++
    /// code is a bit more effecient than we are, but if we reach this time, we go ahead with it.
    next_block_request_time: Instant,
    last_ping_nonce: Option<u64>,
    /// The next time we should send a ping. If last_ping_nonce is not-None by the time we get
    /// there, we should disconnect the peer.
    next_ping_time: Instant,
}

/// Finds the fork point between a and b and the block that is (up to) 2000 blocks
/// after the fork point, headed towards a.
fn find_fork_point_plus_2k(mut a: BlockIndex, mut b: BlockIndex) -> (BlockIndex, BlockIndex) {
    let mut a2k = a;
    let mut a2k_height = a2k.height();
    let mut a_height = a2k_height;
    let initial_b_height = b.height();
    if a_height > initial_b_height {
        if a_height > initial_b_height + 2000 {
            a2k = a.get_ancestor(initial_b_height + 2000).expect("prev() has to at least get us back to genesis");
        }
        a = a2k.get_ancestor(initial_b_height).expect("prev() has to at least get us back to genesis");
        a_height = a.height();
    }
    if initial_b_height > a_height {
        b = b.get_ancestor(a_height).expect("prev() has to at least get us back to genesis");
    }
    assert_eq!(a.height(), b.height());
    while a != b {
        a = a.get_prev().expect("prev() has to at least get us back to genesis");
        b = b.get_prev().expect("prev() has to at least get us back to genesis");
        a_height -= 1;
        if a2k_height > a_height + 2000 {
            a2k = a2k.get_prev().expect("prev() has to at least get us back to genesis");
            a2k_height -= 1;
        }
    }
    (a, a2k)
}

fn generate_locator_response<T: Clone, Conv: Fn(BlockIndex) -> T, Dummy: Fn() -> T>(our_best: BlockIndex, locators: &Vec<Hash>, stop_hash: &Hash, c: Conv, d: Dummy) -> Vec<T> {
    let mut res = Vec::new();
    if locators.is_empty() {
        if let Some(index) = BlockIndex::get_from_hash(&stop_hash.into_inner(), Some(our_best)) {
            res.push(c(index));
        }
    } else {
        let mut range = None;
        for h in locators {
            if let Some(index) = BlockIndex::get_from_hash(&h.into_inner(), Some(our_best)) {
                if index == our_best {
                    // They asked for anything newer than our tip, don't send them anything!
                    break;
                }
                let (fork, fork2k) = find_fork_point_plus_2k(our_best, index);
                range = Some((fork, fork2k));
                break;
            }
        }
        if range.is_none() {
            range = Some((BlockIndex::genesis(),
                         our_best.get_ancestor(std::cmp::min(2000, our_best.height())).expect("prev() has to at least get us back to genesis")));
        }
        if let Some((fork_point, mut stop_index)) = range {
            res.resize(2000, d());
            let mut idx: isize = 1999;
            while stop_index != fork_point {
                if stop_hash.into_inner() != [0; 32] && stop_hash.into_inner() == stop_index.hash() {
                    break;
                }
                res[idx as usize] = c(stop_index);
                idx -= 1;
                stop_index = stop_index.get_prev().expect("prev() has to at least get us back to genesis");
                if idx == -1 {
                    assert!(fork_point == stop_index);
                }
            }
            if idx >= 0 {
                res.drain(0..(idx as usize) + 1);
            }
        }
    }
    res
}

/// Finds the first ancestor from candidate_tip going back to the fork block from our_tip that
/// doesn't have data. This is gratuitously expensive, so we should prefer to avoid it, in general.
fn find_missing_data_fork_point(mut our_tip: BlockIndex, mut candidate_tip: BlockIndex) -> Option<BlockIndex> {
    let mut tip_walk_height = our_tip.height();
    let mut candidate_height = candidate_tip.height();
    // Walk back candidate_tip until it doesn't have_data(), but return None if we hit a common
    // ancestor with our_tip (instead of walking back further).
    while !candidate_tip.have_block() {
        if candidate_height < 1 {
            // Why would we download genesis?
            return None;
        }
        if candidate_height - 1 < tip_walk_height {
            our_tip = our_tip.get_prev().expect("prev() can't return nothing if height > 0");
            tip_walk_height -= 1;
        }
        let prev = candidate_tip.get_prev().expect("prev() can't return nothing if height > 0");
        if prev == our_tip {
            // We got to the fork point and every block is missing data, just request the block
            // immediately after the fork point.
            break;
        }
        candidate_height -= 1;
        candidate_tip = prev;
    }
    Some(candidate_tip)
}

/// Returns true if the block pointed to by the given index is a pretty good candidate for being on
/// the best chain. Checks for some basics like the candidate tip not being obviously-bogus,
/// not known-to-be-invalid, and having more work than our tip.
/// Does *not* check whether we already have the data for this block
/// Takes a hint of a best chain that this may be building towards (which may be == block)
fn is_probably_on_best_chain(our_tip: BlockIndex, block: BlockIndex, candidate_chain_tip: BlockIndex) -> bool {
    let min_chainwork = get_min_chainwork();
    let tip_work = our_tip.total_work();

    let block_work = block.total_work();
    if block_work >= min_chainwork && block_work > tip_work && block.not_invalid(false) {
        return true;
    }
    if block == candidate_chain_tip { return false; }

    let candidate_work = candidate_chain_tip.total_work();
    candidate_work >= min_chainwork && candidate_work > tip_work &&
    candidate_chain_tip.get_ancestor(block.height()) == Some(block) &&
    candidate_chain_tip.not_invalid(false)
}

#[no_mangle]
pub extern "C" fn init_p2p_client(connman_ptr: *mut c_void, datadir_path: *const c_char, subver_c: *const c_char, bind_port: u16, dnsseed_names: *const *const c_char, dnsseed_count: usize) {
    let addr_path: String = match unsafe { CStr::from_ptr(datadir_path) }.to_str() {
        Ok(d) => d.to_string() + "/rust_p2p_addrs.dat",
        Err(_) => return,
    };
    let subver: String = match unsafe { CStr::from_ptr(subver_c) }.to_str() {
        Ok(d) => d.to_string(),
        Err(_) => return,
    };
    let mut dnsseeds: Vec<String> = Vec::with_capacity(dnsseed_count);
    for i in 0..dnsseed_count {
        dnsseeds.push(match unsafe { CStr::from_ptr(*dnsseed_names.offset(i as isize)) }.to_str() {
            Ok(d) => d.to_string(),
            Err(_) => return,
        });
    }
    let connman = Connman(connman_ptr);
    std::thread::spawn(move || {
        // Always catch panics so that even if we have some bug in our parser we don't take the
        // rest of Bitcoin Core down with us:
        THREAD_COUNT.fetch_add(1, Ordering::AcqRel);
        let _ = catch_unwind(move || {
            let mut addr_tracker = AddrTracker::new(addr_path, dnsseeds);

            await_ibd_complete_or_stalled();

            let mut rand_ctx = RandomContext::new();

            let sleep_condvar = Arc::new(Condvar::new());
            let sleep_mutex = Arc::new(Mutex::new(()));

            let pending_outbounds = Arc::new(Mutex::new(Vec::new()));
            let pending_inbounds = Arc::new(Mutex::new(Vec::new()));

            let mut peers: Vec<PeerState> = Vec::new();
            let waker = spawn_socket_handler(&THREAD_COUNT, bind_port, Arc::clone(&sleep_condvar), Arc::clone(&sleep_mutex), Arc::clone(&pending_outbounds), Arc::clone(&pending_inbounds));

            let mut prev_tip = BlockIndex::tip();
            let mut last_tip_update = Instant::now();
            let mut last_header_tip_update = Instant::now();
            let mut prev_header_tip = BlockIndex::best_header();
            'aggressiveness_check: while unsafe { !rusty_ShutdownRequested() } {
                {
                    let sleep_lock = sleep_mutex.lock().unwrap();
                    let mut skip_sleep = false;
                    for peer in peers.iter() {
                        if !peer.queues.inbound.lock().unwrap().is_empty() {
                            skip_sleep = true;
                            break;
                        }
                    }
                    if !skip_sleep {
                        // We're ok with spurious wakes making us run the main loop again, its not
                        // too expensive and its worth checking again if there's any work to do.
                        let _ = sleep_condvar.wait_timeout(sleep_lock, Duration::from_millis(100)).unwrap();
                    }
                }

                // If we haven't made progress, and our tip doesn't match our best header,
                // aggressively connect to new peers to find the block data.
                let new_tip = BlockIndex::tip();
                let new_header_tip = BlockIndex::best_header();
                if prev_tip == new_tip && new_tip != new_header_tip &&
                        Instant::now() - last_header_tip_update > Duration::from_secs(60) &&
                        Instant::now() - last_tip_update > Duration::from_secs(60) {
                    log_line("Progress not being made, seeking better peers!", false);
                    if let Some(addr) = addr_tracker.get_rand_addr(&mut rand_ctx) {
                        let queues = Arc::new(MessageQueues {
                            inbound: Mutex::new(LinkedList::new()),
                            outbound: Mutex::new(LinkedList::new()),
                        });
                        let outbound_nonce = OutboundP2PNonce::new(connman, &mut rand_ctx);
                        queues.outbound.lock().unwrap().push_back(NetMsg::Msg(NetworkMessage::Version(VersionMessage {
                            version: PROTOCOL_VERSION,
                            services: (1 << 3) | (1 << 10), // We don't bother to track pruning status
                            timestamp: SystemTime::now().duration_since(SystemTime::UNIX_EPOCH).unwrap().as_secs() as i64,
                            receiver: address::Address {
                                services: 0,
                                address: [0; 8], //TODO?
                                port: 0, //TODO?
                            },
                            sender: address::Address {
                                services: 0,
                                address: [0; 8],
                                port: 0,
                            },
                            nonce: outbound_nonce.nonce(),
                            user_agent: subver.clone(),
                            start_height: new_tip.height(),
                            relay: false, // Blocks only
                        })));
                        pending_outbounds.lock().unwrap().push((Arc::clone(&queues), addr.clone()));
                        peers.push(PeerState {
                            queues,
                            remote_addr: addr,
                            is_outbound: true,
                            outbound_nonce: Some(outbound_nonce),
                            recvd_ver: false,
                            recvd_verack: false,
                            recvd_sendheaders: false,
                            block_state: BlockProviderState::new_with_current_best(BlockIndex::genesis()),
                            next_header_poll: Instant::now() + HEADER_POLL_INTERVAL,
                            next_block_request: None,
                            next_block_request_time: Instant::now() + BLOCK_REQ_DELAY,
                            last_ping_nonce: None,
                            next_ping_time: Instant::now() + PING_INTERVAL,
                        });
                    }
                }
                if new_header_tip != prev_header_tip {
                    prev_header_tip = new_header_tip;
                    last_header_tip_update = Instant::now();
                }
                if new_tip != prev_tip {
                    prev_tip = new_tip;
                    last_tip_update = Instant::now();
                }

                {
                    let mut inbounds = pending_inbounds.lock().unwrap();
                    for (queues, addr) in inbounds.drain(..) {
                        peers.push(PeerState {
                            queues,
                            remote_addr: addr,
                            is_outbound: false,
                            outbound_nonce: None,
                            recvd_ver: false,
                            recvd_verack: false,
                            recvd_sendheaders: false,
                            block_state: BlockProviderState::new_with_current_best(BlockIndex::genesis()),
                            next_header_poll: Instant::now() + HEADER_POLL_INTERVAL,
                            next_block_request: None,
                            next_block_request_time: Instant::now() + BLOCK_REQ_DELAY,
                            last_ping_nonce: None,
                            next_ping_time: Instant::now() + PING_INTERVAL,
                        });
                    }
                }

                // Process new inbound messages
                let mut i = 0;
                'peer_iter: while i != peers.len() {
                    let mut drop_peer = false;
                    {
                        let peer = &mut peers[i];

                        macro_rules! write_msg { ($msg: expr) => { {
                            peer.queues.outbound.lock().unwrap().push_back($msg);
                            waker.wake();
                        } } }

                        // Request headers, starting at $from_target and continuing for up to 2000
                        // blocks.
                        macro_rules! request_headers { ($from_target: expr) => { {
                            let mut locator_hashes = vec![Hash::from_inner($from_target.hash())];
                            if let Some(ancestor) = $from_target.get_ancestor($from_target.height() - 1008) {
                                locator_hashes.push(Hash::from_inner(ancestor.hash()));
                            }
                            write_msg!(NetMsg::Msg(NetworkMessage::GetHeaders(GetHeadersMessage {
                                version: PROTOCOL_VERSION,
                                locator_hashes,
                                stop_hash: Hash::from_inner([0; 32]),
                            })));
                        } } }

                        while let Some(msg) = {
                            // If, eg, we're sending blocks, we have to wait for them to receive them
                            // before we want to go push another one into the send buffer...
                            if peer.queues.outbound.lock().unwrap().len() >= MAX_QUEUE_LEN {
                                None
                            } else {
                                peer.queues.inbound.lock().unwrap().pop_front()
                            }
                        } {
                            match msg {
                                NetMsg::EOF => { drop_peer = true; break; },
                                NetMsg::Msg(NetworkMessage::Version(ver)) => {
                                    if peer.recvd_ver { drop_peer = true; break; }
                                    // Check that we've connected to a full node that supports SegWit
                                    if ver.services & ((1 << 10) | (1 << 0)) == 0 || ver.services & (1 << 3) == 0 {
                                        drop_peer = true;
                                        break;
                                    }
                                    if !peer.is_outbound {
                                        if should_disconnect_by_inbound_nonce(connman, ver.nonce) {
                                            drop_peer = true;
                                            break;
                                        }
                                        write_msg!(NetMsg::Msg(NetworkMessage::Version(VersionMessage {
                                            version: PROTOCOL_VERSION,
                                            services: (1 << 3) | (1 << 10), // We don't bother to track pruning status
                                            timestamp: SystemTime::now().duration_since(SystemTime::UNIX_EPOCH).unwrap().as_secs() as i64,
                                            receiver: address::Address {
                                                services: 0,
                                                address: [0; 8], //TODO?
                                                port: 0, //TODO?
                                            },
                                            sender: address::Address {
                                                services: 0,
                                                address: [0; 8], //TODO?
                                                port: 0, //TODO?
                                            },
                                            nonce: 0,
                                            user_agent: subver.clone(),
                                            start_height: new_tip.height(),
                                            relay: false, // Blocks only
                                        })));
                                    }
                                    write_msg!(NetMsg::Msg(NetworkMessage::Verack));
                                    peer.recvd_ver = true;
                                },
                                NetMsg::Msg(NetworkMessage::Verack) => {
                                    if peer.recvd_verack { drop_peer = true; break; }
                                    peer.outbound_nonce = None;
                                    peer.recvd_verack = true;
                                    write_msg!(NetMsg::Msg(NetworkMessage::GetAddr));
                                },
                                _ if !peer.recvd_ver || !peer.recvd_verack => { drop_peer = true; break; },
                                NetMsg::Msg(NetworkMessage::SendHeaders) => {
                                    peer.recvd_sendheaders = true;
                                },
                                NetMsg::Msg(NetworkMessage::GetAddr) => {
                                    // We refuse to respond to getaddr messages to protect our own
                                    // privacy.
                                },
                                NetMsg::Msg(NetworkMessage::Addr(addrs)) => {
                                    addr_tracker.addrs_recvd(&peer.remote_addr, peer.is_outbound, &addrs);
                                },
                                NetMsg::Msg(NetworkMessage::GetBlocks(getblocks)) => {
                                    let resp = generate_locator_response(new_tip, &getblocks.locator_hashes, &getblocks.stop_hash,
                                                                         |index| Inventory { inv_type: InvType::WitnessBlock, hash: Hash::from_inner(index.hash()) },
                                                                         || Inventory { inv_type: InvType::WitnessBlock, hash: Default::default() });
                                    write_msg!(NetMsg::Msg(NetworkMessage::Inv(resp)));
                                },
                                NetMsg::Msg(NetworkMessage::GetHeaders(getheaders)) => {
                                    let resp = generate_locator_response(new_tip, &getheaders.locator_hashes, &getheaders.stop_hash,
                                                                         |index| deserialize::<BlockHeader>(&index.header_bytes()).expect("Headers are valid"),
                                                                         || BlockHeader { version: 0, prev_blockhash: Default::default(), merkle_root: Default::default(), time: 0, bits: 0, nonce: 0 });
                                    write_msg!(NetMsg::Msg(NetworkMessage::Headers(resp)));
                                },
                                NetMsg::Msg(NetworkMessage::GetData(mut invs)) => {
                                    let mut cut_idx = invs.len();
                                    for (idx, inv) in invs.iter().enumerate() {
                                        if inv.inv_type == InvType::WitnessBlock {
                                            if let Some(index) = BlockIndex::get_from_hash(&inv.hash.into_inner(), Some(new_tip)) {
                                                write_msg!(NetMsg::SerializedBlock(index.block_bytes()));
                                                cut_idx = idx;
                                                break;
                                            }
                                        }
                                    }
                                    if cut_idx != invs.len() {
                                        // Push the remaining blocks requested back into the message
                                        // stack for processing - we'll check if there's buffer space
                                        // when we go around again.
                                        let new_invs = invs.split_off(cut_idx + 1);
                                        peer.queues.inbound.lock().unwrap().push_front(NetMsg::Msg(NetworkMessage::GetData(new_invs)));
                                    }
                                },
                                NetMsg::Msg(NetworkMessage::Inv(invs)) => {
                                    for inv in invs.iter() {
                                        if inv.inv_type == InvType::WitnessBlock || inv.inv_type == InvType::Block {
                                            if let Some(index) = BlockIndex::get_from_hash(&inv.hash.into_inner(), Some(new_tip)) {
                                                peer.block_state.set_current_best(index);
                                            } else {
                                                // Request the header itself, using a null locator. If
                                                // we are somehow missing a block in between, thats ok,
                                                // we'll fall back to the header-fetch polling soon.
                                                write_msg!(NetMsg::Msg(NetworkMessage::GetHeaders(GetHeadersMessage {
                                                    version: PROTOCOL_VERSION,
                                                    locator_hashes: Vec::new(),
                                                    stop_hash: inv.hash,
                                                })));
                                            }
                                        }
                                    }
                                },
                                NetMsg::Msg(NetworkMessage::Headers(ref headers)) if !headers.is_empty() => {
                                    if let Some(best_connected) = connect_headers(&headers) {
                                        if headers.len() == HEADERS_LEN_CONTINUE {
                                            if best_connected.total_work() > peer.block_state.get_current_best().total_work() {
                                                // We've succeeded at fetching some new headers from
                                                // this peer, and should request more. If we hit a
                                                // regular header poll we'll probably end up just
                                                // sending roughly the same request again...instead,
                                                // delay fetching and give them the chance to get us up
                                                // to sync with them via this response-request flow.
                                                request_headers!(best_connected);
                                                peer.next_header_poll = Instant::now() + HEADER_POLL_INTERVAL;
                                            }
                                        }
                                        // Always blindly assume the last header we receive is the
                                        // peer's current best. This should be fine as long as we never
                                        // request headers when we don't believe the requested header
                                        // is their best block.
                                        // One important violation to keep in mind is that we may send
                                        // a headers request without enough locator hashes for them to
                                        // find our chain, resulting in them responding with headers
                                        // starting with genesis. To avoid this happening needlessly
                                        // when they reorg a block or two, we always try to send a
                                        // locator with a second block a week back.
                                        // While this should not result in a lack of robustness, it may
                                        // result in us redownloading the header chain from them.
                                        peer.block_state.set_current_best(best_connected);
                                    } else {
                                        // The peer sent us a non-empty headers message, indicating
                                        // they're trying to speak to us, I know it! I just don't know
                                        // what they're saying...we'll request again within 30 seconds.
                                        // Note that we don't want to make a request again here as it's
                                        // likely they're just sending is a block at their tip which we
                                        // haven't sync'ed to yet, and we'll end up making redundant
                                        // requests.
                                    }
                                },
                                NetMsg::SerializedBlock(blockdata) => {
                                    // If the block is a descendant of their best header, their best
                                    // header is not known to be invalid, their best header has more
                                    // work than ours, and their best header meets min chain work,
                                    // force processing (aka storing) the block.
                                    // Otherwise, just process normally.
                                    if let Ok(header) = BlockHeader::consensus_decode(&blockdata[..]) {
                                        if let Some(index) = BlockIndex::get_from_hash(&header.bitcoin_hash().into_inner(), Some(new_tip)) {
                                            if is_probably_on_best_chain(new_tip, index, peer.block_state.get_current_best()) {
                                                connect_block(&blockdata, Some(index));
                                            } else {
                                                connect_block(&blockdata, None);
                                            }
                                        } else {
                                            // Don't have the header connected anywhere. This block is
                                            // almost certainly useless and we probably didn't request
                                            // it, but hand it off to C++ just in case.
                                            connect_block(&blockdata, None);
                                        }
                                    } else {
                                        // If we couldn't even decode the header, just disconnect them.
                                        drop_peer = true; break;
                                    }
                                },
                                NetMsg::Msg(NetworkMessage::Ping(nonce)) => {
                                    write_msg!(NetMsg::Msg(NetworkMessage::Pong(nonce)));
                                },
                                NetMsg::Msg(NetworkMessage::Pong(nonce)) => {
                                    if Some(nonce) == peer.last_ping_nonce {
                                        peer.last_ping_nonce = None;
                                    }
                                },
                                _ => {},
                            }
                        }

                        // Poll for their latest header once every 30 seconds by sending a GetHeaders.
                        if !drop_peer && peer.next_header_poll < Instant::now() {
                            // If we haven't yet received their version/verack within 30 seconds after
                            // connecting, just drop the peer...
                            if !peer.recvd_ver || !peer.recvd_verack {
                                drop_peer = true;
                            } else {
                                // Always request one-header-ago so that we get a eader in response and
                                // update the best header we know they have.
                                request_headers!(new_header_tip.get_prev().unwrap_or(new_header_tip));
                                peer.next_header_poll = Instant::now() + HEADER_POLL_INTERVAL;
                            }
                        }

                        // Regularly poll the BlockProviderState for a block to download and download
                        // any blocks that we've been waiting on for a while.
                        if !drop_peer && peer.next_block_request_time < Instant::now() {
                            let their_best = peer.block_state.get_current_best();
                            let cpp_requested = peer.block_state.get_next_block_to_download(true);

                            if let Some(index) = peer.next_block_request {
                                // If we wanted to download the block in the past, and we still don't
                                // have data for it, and it has same or more work than our current tip,
                                // or builds towards a potential future tip, go ahead and download it.
                                if cpp_requested == Some(index) ||
                                   (index != new_tip && !index.have_block() && is_probably_on_best_chain(new_tip, index, their_best)) {
                                    write_msg!(NetMsg::Msg(NetworkMessage::GetData(vec![Inventory {
                                        inv_type: InvType::WitnessBlock,
                                        hash: Hash::from_inner(index.hash()),
                                    }])));
                                }
                                peer.next_block_request = None;
                            }

                            let mut may_request = true;
                            if let Some(index) = cpp_requested {
                                // We go a new potential request block from the C++ block requester.
                                // Great! But lets assume its returning garbage on each call, and make
                                // sure the block it wants us to request meets our fetch requirements.
                                if is_probably_on_best_chain(new_tip, index, their_best) {
                                    peer.next_block_request = Some(index);
                                    may_request = false;
                                }
                            }
                            if may_request && is_probably_on_best_chain(new_tip, their_best, their_best) {
                                // If the standard block requester didn't find anything we wanted to
                                // do, and their chain meets the requirements for fetching,
                                // (gratuitously inefficiently) find the fork point that hasn't been
                                // downloaded and start fetching.
                                peer.next_block_request = find_missing_data_fork_point(new_tip, their_best);
                            }
                            peer.next_block_request_time = Instant::now() + BLOCK_REQ_DELAY;
                        }

                        if !drop_peer && peer.next_ping_time < Instant::now() {
                            if peer.last_ping_nonce.is_some() {
                                drop_peer = true;
                            } else {
                                let nonce = rand_ctx.get_rand_u64();
                                peer.last_ping_nonce = Some(nonce);
                                peer.next_ping_time = Instant::now() + PING_INTERVAL;
                                write_msg!(NetMsg::Msg(NetworkMessage::Ping(nonce)));
                            }
                        }
                    }

                    if drop_peer {
                        peers[i].queues.outbound.lock().unwrap().push_back(NetMsg::EOF);
                        waker.wake();
                        peers.remove(i);
                    } else {
                        i += 1;
                    }
                }
            }
        });
        THREAD_COUNT.fetch_sub(1, Ordering::AcqRel);
    });
}

#[no_mangle]
pub extern "C" fn stop_p2p_client() {
    while THREAD_COUNT.load(Ordering::Acquire) != 0 {
        std::thread::sleep(Duration::from_millis(10));
    }
}
