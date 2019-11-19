use bitcoin::network::constants;
use std::sync::{Arc, Mutex};
use std::collections::HashMap;
use lightning::ln::router::Router;
use lightning::chain::chaininterface::BlockNotifier;
use lightning::chain;
use lightning::ln::{channelmonitor, channelmanager, peer_handler};
use core_lightning::{socket, CHANNEL_MANAGER_FILE};
use lightning::chain::keysinterface::{InMemoryChannelKeys, SpendableOutputDescriptor};
use bitcoin::blockdata;
use lightning::ln::channelmanager::{PaymentHash, PaymentPreimage};
use lightning::ln::peer_handler::PeerManager;
use lightning::util::events::{EventsProvider, Event};
use bridge::get_signed_tx;
use bitcoin::consensus::encode;
use core_lightning::utils::hex_str;
use std::fs;
use lightning::util::ser::Writeable;
use core_lightning::socket::SocketDescriptor;

pub struct EventHandler {
    pub(crate) network: constants::Network,
    pub(crate) file_prefix: String,
    pub(crate) peer_manager: Arc<peer_handler::PeerManager<socket::SocketDescriptor>>,
    pub(crate) channel_manager: Arc<channelmanager::ChannelManager<InMemoryChannelKeys>>,
    monitor: Arc<channelmonitor::SimpleManyChannelMonitor<chain::transaction::OutPoint>>,
    broadcaster: Arc<chain::chaininterface::BroadcasterInterface>,
    pub(crate) block_notifier: Arc<BlockNotifier>,
    pub(crate) router: Arc<Router>,
    txn_to_broadcast:
    Mutex<HashMap<chain::transaction::OutPoint, blockdata::transaction::Transaction>>,
    payment_preimages: Arc<Mutex<HashMap<PaymentHash, PaymentPreimage>>>,
}

impl EventHandler {
    pub(crate) fn new(
        network: constants::Network,
        file_prefix: String,
        peer_manager: Arc<peer_handler::PeerManager<socket::SocketDescriptor>>,
        monitor: Arc<channelmonitor::SimpleManyChannelMonitor<chain::transaction::OutPoint>>,
        channel_manager: Arc<channelmanager::ChannelManager<InMemoryChannelKeys>>,
        broadcaster: Arc<chain::chaininterface::BroadcasterInterface>,
        block_notifier: Arc<BlockNotifier>,
        router: Arc<Router>,
        payment_preimages: Arc<Mutex<HashMap<PaymentHash, PaymentPreimage>>>,
    ) -> EventHandler {
        Self {
            network,
            file_prefix,
            peer_manager,
            channel_manager,
            monitor,
            broadcaster,
            block_notifier,
            router,
            txn_to_broadcast: Mutex::new(HashMap::new()),
            payment_preimages,
        }
    }

    pub fn clone_peer_manager(&self) -> Arc<PeerManager<SocketDescriptor>> {
        self.peer_manager.clone()
    }

    pub fn process_events(&self) {
        self.peer_manager.process_events();
        let mut events = self.channel_manager.get_and_clear_pending_events();
        events.append(&mut self.monitor.get_and_clear_pending_events());

        let save_state = events.is_empty();

        for event in events {
            match event {
                Event::FundingGenerationReady {
                    temporary_channel_id,
                    channel_value_satoshis,
                    output_script,
                    ..
                } => {
                    let us = self.clone();

                    let addr = bitcoin_bech32::WitnessProgram::from_scriptpubkey(&output_script[..], match us.network {
                        constants::Network::Bitcoin => bitcoin_bech32::constants::Network::Bitcoin,
                        constants::Network::Testnet => bitcoin_bech32::constants::Network::Testnet,
                        constants::Network::Regtest => bitcoin_bech32::constants::Network::Regtest,
                    }).expect("LN funding tx should always be to a SegWit output").to_address();

                    let signed_tx = get_signed_tx(addr.into_bytes(), channel_value_satoshis);

                    let tx: blockdata::transaction::Transaction =
                        encode::deserialize(signed_tx.as_slice()).unwrap();

                    let outpoint = chain::transaction::OutPoint {
                        txid: tx.txid(),
                        index: 1,
                    };

                    us.channel_manager
                        .funding_transaction_generated(&temporary_channel_id, outpoint);
                    us.txn_to_broadcast.lock().unwrap().insert(outpoint, tx);
                    println!("Generated funding tx!");
                }
                Event::FundingBroadcastSafe { funding_txo, .. } => {
                    let mut txn = self.txn_to_broadcast.lock().unwrap();
                    let tx = txn.remove(&funding_txo).unwrap();
                    self.broadcaster.broadcast_transaction(&tx);
                    println!("Broadcast funding tx {}!", tx.txid());
                }
                Event::PaymentReceived { payment_hash, amt } => {
                    let images = self.payment_preimages.lock().unwrap();
                    if let Some(payment_preimage) = images.get(&payment_hash) {
                        if self
                            .channel_manager
                            .claim_funds(payment_preimage.clone(), 10000)
                        {
                            println!("Moneymoney! {} id {}", amt, hex_str(&payment_hash.0));
                        } else {
                            println!("Failed to claim money we were told we had?");
                        }
                    } else {
                        self.channel_manager.fail_htlc_backwards(&payment_hash);
                        println!("Received payment but we didn't know the preimage :(");
                    }
                }
                Event::PaymentSent { payment_preimage } => {
                    println!("Less money :(, proof: {}", hex_str(&payment_preimage.0));
                }
                Event::PaymentFailed {
                    payment_hash,
                    rejected_by_dest,
                } => {
                    println!(
                        "{} failed id {}!",
                        if rejected_by_dest { "Send" } else { "Route" },
                        hex_str(&payment_hash.0)
                    );
                }
                Event::PendingHTLCsForwardable { time_forwardable } => {
                    let us = self.clone();
                    us.channel_manager.process_pending_htlc_forwards();
                }
                Event::SpendableOutputs { mut outputs } => {
                    for output in outputs.drain(..) {
                        match output {
                            SpendableOutputDescriptor::StaticOutput { outpoint, .. } => {
                                println!("Got on-chain output Bitcoin Core should know how to claim at {}:{}", hex_str(&outpoint.txid[..]), outpoint.vout);
                            }
                            SpendableOutputDescriptor::DynamicOutputP2WSH { .. } => {
                                println!("Got on-chain output we should claim...");
                                //TODO: Send back to Bitcoin Core!
                            }
                            SpendableOutputDescriptor::DynamicOutputP2WPKH { .. } => {
                                println!("Got on-chain output we should claim...");
                                //TODO: Send back to Bitcoin Core!
                            }
                        }
                    }
                }
            }
        }

        if save_state {
            self.save_channel_manager_state();
        }
    }

    fn save_channel_manager_state(&self) {
        let filename = format!("{}/{}", self.file_prefix, CHANNEL_MANAGER_FILE);
        let tmp_filename = filename.clone() + ".tmp";
        {
            let mut f = fs::File::create(&tmp_filename).unwrap();
            self.channel_manager.write(&mut f).unwrap();
        }
        fs::rename(&tmp_filename, &filename).unwrap();
    }
}