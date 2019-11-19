use bitcoin::secp256k1::key::PublicKey;
use bitcoin::secp256k1::Secp256k1;

use bitcoin::network::constants;
use std::ffi::CStr;
use std::os::raw::{c_char, c_ulong};
use std::ffi::CString;
use std::sync::{Arc, Mutex};

use crate::bridge::*;
use bitcoin::consensus::{encode, serialize};
use bitcoin::util::bip32;
use bitcoin::Block;
use lightning::chain::chaininterface;
use lightning::chain::chaininterface::BlockNotifier;
use lightning::chain::keysinterface::{
    InMemoryChannelKeys, KeysInterface, KeysManager,
};
use lightning::util::logger::{Logger, Record, Level};
use std::collections::HashMap;
use std::io::{Write, Read};
use std::time::{Duration, SystemTime, UNIX_EPOCH};
use std::{fs, thread, time};

use lightning::ln::peer_handler::PeerManager;
use lightning::ln::{channelmanager, channelmonitor, peer_handler, router};
use lightning::util::config;
use lightning::util::ser::{ReadableArgs, Writeable};

use lightning::ln::channelmanager::{PaymentHash, PaymentPreimage};
use self::socket::SocketDescriptor;
use std::mem::transmute;
use std::net::{Ipv4Addr, SocketAddr, SocketAddrV4, TcpListener};
use std::str::FromStr;
use std::borrow::Borrow;
use std::fs::OpenOptions;
use bitcoin::hashes::Hash;

mod socket;
mod utils;
mod chain_interface;
mod event_handler;
use core_lightning::chain_interface::*;
use core_lightning::utils::*;
use core_lightning::event_handler::EventHandler;

const FEE_PROPORTIONAL_MILLIONTHS: u32 = 10;
const ANNOUNCE_CHANNELS: bool = false;

const SUBDIR: &str = "/lightning";
const MONITORS_SUBDIR: &str = "/monitors";
const SEED_KEY_FILE: &str = "seed.key";
const CHANNEL_MANAGER_FILE: &str = "manager.dat";
const PEERS_FILE: &str = "peers.dat";

#[repr(C)]
pub struct PeerList {
    ptr: *mut Peer,
    len: usize,
}

#[repr(C)]
pub struct Peer {
    id: CString,
}

#[repr(C)]
pub struct ChannelList {
    ptr: *mut Channel,
    len: usize,
}

#[repr(C)]
pub struct Channel {
    short_id: u64,
    capacity: u64,
    id: CString,
}

#[no_mangle]
pub extern "C" fn init_node(datadir: *const c_char) -> *mut Node {
    let data_path: String = match unsafe { CStr::from_ptr(datadir) }.to_str() {
        Ok(d) => d.to_string() + SUBDIR,
        Err(_) => panic!(),
    };

    let lightning_node = Node::new(data_path);
    let lightning_ptr = unsafe { transmute(Box::new(lightning_node)) };
    lightning_ptr
}

#[no_mangle]
pub extern "C" fn listen_incoming(ptr: *mut Node, bind_port: u16) -> bool {
    let node = unsafe { &mut *ptr };
    node.listen_incoming(bind_port)
}

#[no_mangle]
pub extern "C" fn connect_peer(ptr: *mut Node, addr: *const c_char) -> bool {
    let node = unsafe { &mut *ptr };
    let addr_string: String = match unsafe { CStr::from_ptr(addr) }.to_str() {
        Ok(d) => d.to_string(),
        Err(_) => return false,
    };
    node.connect_peer(addr_string)
}

#[no_mangle]
pub extern "C" fn get_peers(ptr: *mut Node) -> PeerList {
    let node = unsafe { &mut *ptr };
    let boxed_slice = node.get_peers().into_boxed_slice();
    let len = boxed_slice.len();
    let fat_ptr: *mut [Peer] = Box::into_raw(boxed_slice);
    let slim_ptr: *mut Peer = fat_ptr as _;
    PeerList { ptr: slim_ptr, len }
}

#[no_mangle]
pub extern "C" fn fund_channel(ptr: *mut Node, node_id: *const c_char, amount: c_ulong) -> bool {
    let node = unsafe { &mut *ptr };
    let pk = unsafe { CStr::from_ptr(node_id) }.to_str().unwrap();
    node.fund_channel(pk.parse().unwrap(), amount)
}

#[no_mangle]
pub extern "C" fn get_channels(ptr: *mut Node) -> ChannelList {
    let node = unsafe { &mut *ptr };
    let boxed_slice = node.get_channels().into_boxed_slice();
    let len = boxed_slice.len();
    let fat_ptr: *mut [Channel] = Box::into_raw(boxed_slice);
    let slim_ptr: *mut Channel = fat_ptr as _;
    ChannelList { ptr: slim_ptr, len }
}

#[no_mangle]
pub extern "C" fn pay_invoice(ptr: *mut Node, invoice: *const c_char) -> CString {
    let node = unsafe { &mut *ptr };
    let invoice_str = unsafe { CStr::from_ptr(invoice) }.to_str().unwrap();
    CString::new(node.pay_invoice(invoice_str)).unwrap()
}

#[no_mangle]
pub extern "C" fn create_invoice(
    ptr: *mut Node,
    desc: *const c_char,
    amount: c_ulong,
) -> CString {
    let node = unsafe { &mut *ptr };
    CString::new(node.create_invoice(desc, amount)).unwrap()
}

#[no_mangle]
pub extern "C" fn close_channel(ptr: *mut Node, channel_id: *const c_char) -> CString {
    let node = unsafe { &mut *ptr };
    let id = unsafe { CStr::from_ptr(channel_id) }.to_str().unwrap();
    CString::new(node.close_channel(id)).unwrap()
}

#[no_mangle]
pub extern "C" fn notify_block(ptr: *mut Node) {
    let node = unsafe { &mut *ptr };
    node.notify_block()
}

pub struct Node {
    handler: Arc<EventHandler>,
    peers: Vec<String>,
    fee_estimator: Arc<FeeEstimator>,
}

impl Node {
    pub fn new(data_path: String) -> Node {
        fs::create_dir_all(data_path.clone()).expect("Couldn't create lightning data directory");

        let network = match BlockIndex::network_id().as_ref() {
            "main" => constants::Network::Bitcoin,
            "test" => constants::Network::Testnet,
            "regtest" => constants::Network::Regtest,
            _ => panic!("Unknown network"),
        };

        let node_seed = if let Ok(seed) = fs::read(format!("{}/{}", data_path.clone(), SEED_KEY_FILE)) {
            assert_eq!(seed.len(), 32);
            let mut key = [0; 32];
            key.copy_from_slice(&seed);
            key
        } else {
            let mut rand_ctx = RandomContext::new();
            let seed = rand_ctx.randthirtytwobytes();
            let mut key = [0; 32];
            key.copy_from_slice(serialize(&seed).as_slice());
            let mut f = fs::File::create(format!("{}/{}", data_path.clone(), SEED_KEY_FILE)).unwrap();
            f.write_all(&key).expect("Failed to write seed to disk");
            f.sync_all().expect("Failed to sync seed to disk");
            key
        };

        let logger = Arc::new(LogPrinter {});

        let starting_time = SystemTime::now();
        let since_the_epoch = starting_time
            .duration_since(UNIX_EPOCH)
            .expect("Time went backwards");

        let keys = Arc::new(KeysManager::new(
            &node_seed,
            network,
            logger.clone(),
            since_the_epoch.as_secs(),
            since_the_epoch.subsec_micros(),
        ));

        let secp_ctx = Secp256k1::new();

        let (import_key_1, import_key_2) =
            bip32::ExtendedPrivKey::new_master(network, &node_seed)
                .map(|extpriv| {
                    (
                        extpriv
                            .ckd_priv(&secp_ctx, bip32::ChildNumber::from_hardened_idx(1).unwrap())
                            .unwrap()
                            .private_key
                            .key,
                        extpriv
                            .ckd_priv(&secp_ctx, bip32::ChildNumber::from_hardened_idx(2).unwrap())
                            .unwrap()
                            .private_key
                            .key,
                    )
                })
                .unwrap();

        if !import_key(import_key_1.encode()) || !import_key(import_key_2.encode()) {
            panic!("Couldn't import private keys")
        }

        let chain_monitor = Arc::new(ChainInterface::new(network, logger.clone()));

        let fee_estimator = Arc::new(FeeEstimator::new());

        let monitors_dir = data_path.clone() + MONITORS_SUBDIR;

        fs::create_dir_all(monitors_dir.clone()).expect("Couldn't create monitors subdir");

        let mut monitors_loaded =
            ChannelMonitor::load_from_disk(&monitors_dir);

        let channel_monitor = Arc::new(ChannelMonitor {
            monitor: channelmonitor::SimpleManyChannelMonitor::new(
                chain_monitor.clone(),
                chain_monitor.clone(),
                logger.clone(),
                fee_estimator.clone(),
            ),
            file_prefix: monitors_dir.clone(),
        });

        let mut config = config::UserConfig::default();
        config.channel_options.fee_proportional_millionths = FEE_PROPORTIONAL_MILLIONTHS;
        config.channel_options.announced_channel = ANNOUNCE_CHANNELS;

        let channel_manager = if let Ok(mut f) = fs::File::open(format!("{}/{}", data_path.clone(), CHANNEL_MANAGER_FILE))
        {
            let (last_block_hash, manager) = {
                let mut monitors_refs = HashMap::new();
                for (outpoint, monitor) in monitors_loaded.iter_mut() {
                    monitors_refs.insert(*outpoint, monitor);
                }
                <(
                    bitcoin_hashes::sha256d::Hash,
                    channelmanager::ChannelManager<InMemoryChannelKeys>,
                )>::read(
                    &mut f,
                    channelmanager::ChannelManagerReadArgs {
                        keys_manager: keys.clone(),
                        fee_estimator: fee_estimator.clone(),
                        monitor: channel_monitor.clone(),
                        tx_broadcaster: chain_monitor.clone(),
                        logger: logger.clone(),
                        default_config: config,
                        channel_monitors: &mut monitors_refs,
                    },
                )
                .expect("Failed to deserialize channel manager")
            };
            channel_monitor.load_from_vec(monitors_loaded);
            //TODO: Rescan
            let manager = Arc::new(manager);
            manager
        } else {
            if !monitors_loaded.is_empty() {
                panic!("Found some channel monitors but no channel state!");
            }
            channelmanager::ChannelManager::new(
                network,
                fee_estimator.clone(),
                channel_monitor.clone(),
                chain_monitor.clone(),
                logger.clone(),
                keys.clone(),
                config,
                get_block_count() as usize,
            )
            .unwrap()
        };

        let router = Arc::new(router::Router::new(
            PublicKey::from_secret_key(&secp_ctx, &keys.get_node_secret()),
            chain_monitor.clone(),
            logger.clone(),
        ));

        let mut rand_ctx = RandomContext::new();
        let rand256 = rand_ctx.randthirtytwobytes();
        let mut rand = [0; 32];
        rand.copy_from_slice(serialize(&rand256).as_slice());

        let peer_manager: Arc<PeerManager<socket::SocketDescriptor>> =
            Arc::new(peer_handler::PeerManager::new(
                peer_handler::MessageHandler {
                    chan_handler: channel_manager.clone(),
                    route_handler: router.clone(),
                },
                keys.get_node_secret(),
                &rand,
                logger.clone(),
            ));

        let payment_preimages: Arc<Mutex<HashMap<PaymentHash, PaymentPreimage>>> =
            Arc::new(Mutex::new(HashMap::new()));

        let block_notifier = Arc::new(BlockNotifier::new(chain_monitor.clone()));

        let channel_manager_listener: Arc<chaininterface::ChainListener> = channel_manager.clone();
        block_notifier.register_listener(Arc::downgrade(&channel_manager_listener));

        let event_handler = Arc::new(EventHandler::new(
            network,
            data_path,
            peer_manager.clone(),
            channel_monitor.monitor.clone(),
            channel_manager.clone(),
            chain_monitor.clone(),
            block_notifier.clone(),
            router.clone(),
            payment_preimages.clone(),
        ));

        let event_handler_clone = event_handler.clone();

        thread::spawn(move || {
            loop {
                thread::sleep(time::Duration::from_millis(500));
                event_handler_clone.process_events();
            }
        });

        let mut res = Node {
            handler: event_handler,
            peers: Vec::new(),
            fee_estimator: fee_estimator.clone(),
        };

        res.reconnect_peers();
        res
    }

    pub fn listen_incoming(&self, bind_port: u16) -> bool {
        let listener = TcpListener::bind(SocketAddr::V4(SocketAddrV4::new(
            Ipv4Addr::new(0, 0, 0, 0),
            bind_port,
        )))
        .unwrap();
        listener.set_nonblocking(true).expect("Cannot set non-blocking");

        print!("Listening on port {}...\n", bind_port);

        for stream in listener.incoming() {
            match stream {
                Ok(_) => {
                    print!("Incoming connection on {}...\n", bind_port);
                }
                Err(e) => panic!("encountered IO error: {}", e),
            }
        }

        true
    }

    fn save_peer(&self, addr: String) {
        if self.peers.contains(&addr) {
            return
        }

        let peer_file_path = format!("{}/{}", self.handler.file_prefix, PEERS_FILE);
        if let Ok(mut f) = OpenOptions::new().append(true).open(peer_file_path.as_str()) {
            write!(f, "{}\n", addr).unwrap();
        } else {
            let mut f = fs::File::create(peer_file_path.as_str()).unwrap();
            write!(f, "{}\n", addr).unwrap();
            f.sync_all().expect("Failed to sync peers to disk");
        }
    }

    fn reconnect_peers(&mut self) {
        let peer_file_path = format!("{}/{}", self.handler.file_prefix, PEERS_FILE);
        if let Ok(mut f) = fs::File::open(peer_file_path.as_str()) {
            let mut data = String::new();
            if f.read_to_string(&mut data).is_ok() {
                let file_peers = data.split("\n");
                for peer in file_peers {
                    if peer.len() > 33 {
                        self.peers.push(peer.to_string());
                        self.connect_peer(peer.to_string());
                    }
                }
            }
        }
    }

    pub fn connect_peer(&self, addr: String) -> bool {
        match hex_to_compressed_pubkey(&addr) {
            Some(pk) => {
                if addr.as_bytes()[33 * 2] == '@' as u8 {
                    let parse_res: Result<std::net::SocketAddr, _> =
                        addr.split_at(33 * 2 + 1).1.parse();
                    if let Ok(sock_addr) = parse_res {
                        print!("Attempting to connect to {}...", sock_addr);
                        match std::net::TcpStream::connect_timeout(&sock_addr, Duration::from_secs(10)) {
                            Ok(stream) => {
                                stream
                                    .set_nonblocking(false)
                                    .expect("set_nonblocking call failed");
                                println!("connected, initiating handshake!");
                                let mut descriptor = SocketDescriptor::new(
                                    self.handler.clone(),
                                    stream,
                                );
                                descriptor.setup_outbound(pk);
                                self.save_peer(addr);
                                true
                            }
                            Err(e) => {
                                println!("connection failed {:?}!", e);
                                false
                            }
                        }
                    } else {
                        println!("Couldn't parse host:port into a socket address");
                        false
                    }
                } else {
                    println!("Invalid line, should be c pubkey@host:port");
                    false
                }
            }
            None => {
                println!("Bad PubKey for remote node");
                false
            }
        };
        false
    }

    pub fn get_peers(&self) -> Vec<Peer> {
        let mut peers: Vec<Peer> = Vec::new();

        for node_id in self.handler.peer_manager.get_peer_node_ids() {
            let hex = CString::new(hex_str(&node_id.serialize())).unwrap();
            peers.push(Peer{id: hex.clone()});
        }

        peers
    }

    pub fn fund_channel(&self, node_id: String, amount: u64) -> bool {
        let pk = hex_to_compressed_pubkey(node_id.as_str());

        match self
            .handler
            .channel_manager
            .create_channel(pk.unwrap(), amount, 0, 0)
        {
            Ok(_) => {
                self.handler.process_events();
                true
            }
            Err(e) => {
                println!("Failed to open channel: {:?}!", e);
                false
            }
        }
    }

    pub fn get_channels(&self) -> Vec<Channel> {
        let mut channels: Vec<Channel> = Vec::new();
        for chan_info in self.handler.channel_manager.list_channels() {
            let id = CString::new(hex_str(&chan_info.channel_id[..])).unwrap();
            if let Some(short_id) = chan_info.short_channel_id {
                channels.push(Channel{id, short_id, capacity: chan_info.channel_value_satoshis});
            } else {
                channels.push(Channel{id, short_id: 0, capacity: chan_info.channel_value_satoshis});
            }
        }

        channels
    }

    pub fn pay_invoice(&self, bolt11: &str) -> &str {
        match lightning_invoice::Invoice::from_str(bolt11) {
            Ok(invoice) => {
                if match invoice.currency() {
                    lightning_invoice::Currency::Bitcoin => constants::Network::Bitcoin,
                    lightning_invoice::Currency::BitcoinTestnet => constants::Network::Testnet,
                    lightning_invoice::Currency::Regtest => constants::Network::Regtest,
                } != self.handler.network {
                    "Wrong network on invoice"
                } else {
                    let mut route_hint = Vec::with_capacity(invoice.routes().len());
                    for route in invoice.routes() {
                            route_hint.push(router::RouteHint {
                                src_node_id: route[0].pubkey,
                                short_channel_id: slice_to_be64(&route[0].short_channel_id),
                                fee_base_msat: route[0].fee_base_msat,
                                fee_proportional_millionths: route[0].fee_proportional_millionths,
                                cltv_expiry_delta: route[0].cltv_expiry_delta,
                                htlc_minimum_msat: 0,
                            });
                    }

                    let final_cltv = invoice.min_final_cltv_expiry().unwrap();

                    match self.handler.router.get_route(invoice.recover_payee_pub_key().borrow(), Some(&self.handler.channel_manager.list_usable_channels()), &route_hint, invoice.amount_pico_btc().unwrap() / 10, *final_cltv as u32) {
                        Ok(route) => {
                            match self.handler.channel_manager.send_payment(route, PaymentHash(invoice.payment_hash().into_inner())) {
                                Ok(()) => {
                                    "Sending payment"
                                },
                                Err(_) => {
                                    "Payment failed"
                                }
                            }
                        },
                        Err(e) => {
                            e.err
                        }
                    }
                }
            },
            Err(_) => {
                "Bad invoice"
            },
        }
    }

    pub fn create_invoice(&self, _desc: *const c_char, _amount: c_ulong) -> &str {
        "not implemented"
    }

    pub fn close_channel(&self, chan_id: &str) -> &str {
        if chan_id.len() == 64  {
            if let Some(chan_id_vec) = hex_to_vec(chan_id) {
                let mut channel_id = [0; 32];
                channel_id.copy_from_slice(&chan_id_vec);
                match self.handler.channel_manager.close_channel(&channel_id) {
                    Ok(()) => {
                        "Closing channel!"
                    },
                    Err(_) => "Failed to close channel."
                }
            } else { "Bad channel_id" }
        } else { "channel_id too short" }
    }

    pub fn notify_block(&self) {
        self.fee_estimator.update_values();
        let chain_tip = BlockIndex::tip();
        let block: Block = encode::deserialize(chain_tip.block_bytes().as_ref()).expect("Couldn't deserialize tip data");
        self.handler.block_notifier.block_connected(block.borrow(), chain_tip.height() as u32);
    }
}

struct LogPrinter {}

impl Logger for LogPrinter {
    fn log(&self, record: &Record) {
        log_line(
            format!(
                "CoreLightning: {:<5} [{} : {}, {}] {}",
                record.level.to_string(),
                record.module_path,
                record.file,
                record.line,
                record.args
            )
                .as_str(),
            false, //record.level == Level::Trace,
        );
    }
}
