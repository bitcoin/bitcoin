use std::sync::atomic::{AtomicUsize, Ordering};
use bridge::{estimate_fee, broadcast_tx};
use std::sync::{Arc, Mutex};
use lightning::chain::chaininterface;
use std::{cmp, fs};
use std::collections::HashMap;
use lightning::util::logger::Logger;
use bitcoin::{Network, Block, Transaction};
use bitcoin::consensus::encode;
use lightning::chain::chaininterface::ChainError;
use lightning::ln::channelmonitor;
use lightning::chain;
use bitcoin_hashes::hex::FromHex;
use std::io::Cursor;
use core_lightning::LogPrinter;
use lightning::ln::channelmonitor::ManyChannelMonitor;
use bitcoin::hashes::hex::ToHex;
use lightning::util::ser::ReadableArgs;

pub struct FeeEstimator {
    background_est: AtomicUsize,
    normal_est: AtomicUsize,
    high_prio_est: AtomicUsize,
}

impl FeeEstimator {
    pub fn new() -> Self {
        FeeEstimator {
            background_est: AtomicUsize::new(0),
            normal_est: AtomicUsize::new(0),
            high_prio_est: AtomicUsize::new(0),
        }
    }
    pub fn update_values(&self) {
        self.high_prio_est
            .store(estimate_fee(6, true) / 250 + 3, Ordering::Release);
        self.normal_est
            .store(estimate_fee(18, false) / 250 + 3, Ordering::Release);
        self.background_est
            .store(estimate_fee(144, false) / 250 + 3, Ordering::Release);
    }
}

impl chaininterface::FeeEstimator for FeeEstimator {
    fn get_est_sat_per_1000_weight(&self, conf_target: chaininterface::ConfirmationTarget) -> u64 {
        cmp::max(
            match conf_target {
                chaininterface::ConfirmationTarget::Background => {
                    self.background_est.load(Ordering::Acquire) as u64
                }
                chaininterface::ConfirmationTarget::Normal => {
                    self.normal_est.load(Ordering::Acquire) as u64
                }
                chaininterface::ConfirmationTarget::HighPriority => {
                    self.high_prio_est.load(Ordering::Acquire) as u64
                }
            },
            253,
        )
    }
}

pub struct ChainInterface {
    util: chaininterface::ChainWatchInterfaceUtil,
    txn_to_broadcast:
    Mutex<HashMap<bitcoin_hashes::sha256d::Hash, bitcoin::blockdata::transaction::Transaction>>,
}

impl ChainInterface {
    pub fn new(network: Network, logger: Arc<Logger>) -> Self {
        ChainInterface {
            util: chaininterface::ChainWatchInterfaceUtil::new(network, logger),
            txn_to_broadcast: Mutex::new(HashMap::new()),
        }
    }

    fn rebroadcast_txn(&self) {
        let txn = self.txn_to_broadcast.lock().unwrap();
        for (_, tx) in txn.iter() {
            let raw_tx = &encode::serialize(tx);
            broadcast_tx(raw_tx.clone());
        }
    }
}

impl chaininterface::BroadcasterInterface for ChainInterface {
    fn broadcast_transaction(&self, tx: &bitcoin::blockdata::transaction::Transaction) {
        self.txn_to_broadcast
            .lock()
            .unwrap()
            .insert(tx.txid(), tx.clone());
        let raw_tx = &encode::serialize(tx);
        broadcast_tx(raw_tx.clone());
    }
}

impl chaininterface::ChainWatchInterface for ChainInterface {
    fn install_watch_tx(
        &self,
        txid: &bitcoin_hashes::sha256d::Hash,
        script: &bitcoin::blockdata::script::Script,
    ) {
        self.util.install_watch_tx(txid, script);
    }

    fn install_watch_outpoint(
        &self,
        outpoint: (bitcoin_hashes::sha256d::Hash, u32),
        script_pubkey: &bitcoin::blockdata::script::Script,
    ) {
        self.util.install_watch_outpoint(outpoint, script_pubkey);
    }

    fn watch_all_txn(&self) {
        self.util.watch_all_txn();
    }

    fn get_chain_utxo(
        &self,
        genesis_hash: bitcoin_hashes::sha256d::Hash,
        unspent_tx_output_identifier: u64,
    ) -> Result<(bitcoin::blockdata::script::Script, u64), ChainError> {
        self.util
            .get_chain_utxo(genesis_hash, unspent_tx_output_identifier)
    }

    fn filter_block<'a>(&self, block: &'a Block) -> (Vec<&'a Transaction>, Vec<u32>) {
        self.util.filter_block(block)
    }

    fn reentered(&self) -> usize {
        self.util.reentered()
    }
}

pub(crate) struct ChannelMonitor {
    pub(crate) monitor: Arc<channelmonitor::SimpleManyChannelMonitor<chain::transaction::OutPoint>>,
    pub(crate) file_prefix: String,
}

impl ChannelMonitor {
    pub(crate) fn load_from_disk(
        file_prefix: &String,
    ) -> Vec<(chain::transaction::OutPoint, channelmonitor::ChannelMonitor)> {
        let mut res = Vec::new();
        for file_option in fs::read_dir(file_prefix).unwrap() {
            let mut loaded = false;
            let file = file_option.unwrap();
            if let Some(filename) = file.file_name().to_str() {
                if filename.is_ascii() && filename.len() > 65 {
                    if let Ok(txid) =
                    bitcoin_hashes::sha256d::Hash::from_hex(filename.split_at(64).0)
                    {
                        if let Ok(index) =
                        filename.split_at(65).1.split('.').next().unwrap().parse()
                        {
                            if let Ok(contents) = fs::read(&file.path()) {
                                if let Ok((last_block_hash, loaded_monitor)) = <(
                                    bitcoin_hashes::sha256d::Hash,
                                    channelmonitor::ChannelMonitor,
                                )>::read(
                                    &mut Cursor::new(&contents),
                                    Arc::new(LogPrinter {}),
                                ) {
                                    // TODO: Rescan from last_block_hash
                                    res.push((
                                        chain::transaction::OutPoint { txid, index },
                                        loaded_monitor,
                                    ));
                                    loaded = true;
                                }
                            }
                        }
                    }
                }
            }
            if !loaded {
                println!("WARNING: Failed to read one of the channel monitor storage files! Check perms!");
            }
        }
        res
    }

    pub(crate) fn load_from_vec(
        &self,
        mut monitors: Vec<(chain::transaction::OutPoint, channelmonitor::ChannelMonitor)>,
    ) {
        for (outpoint, monitor) in monitors.drain(..) {
            if let Err(_) = self.monitor.add_update_monitor(outpoint, monitor) {
                panic!("Failed to load monitor that deserialized");
            }
        }
    }
}

impl channelmonitor::ManyChannelMonitor for ChannelMonitor {
    fn add_update_monitor(
        &self,
        funding_txo: chain::transaction::OutPoint,
        monitor: channelmonitor::ChannelMonitor,
    ) -> Result<(), channelmonitor::ChannelMonitorUpdateErr> {
        macro_rules! try_fs {
            ($res: expr) => {
                match $res {
                    Ok(res) => res,
                    Err(_) => return Err(channelmonitor::ChannelMonitorUpdateErr::PermanentFailure),
                }
            };
        }
        // Do a crazy dance with lots of fsync()s to be overly cautious here...
        // We never want to end up in a state where we've lost the old data, or end up using the
        // old data on power loss after we've returned
        // Note that this actually *isn't* enough (at least on Linux)! We need to fsync an fd with
        // the containing dir, but Rust doesn't let us do that directly, sadly. TODO: Fix this with
        // the libc crate!
        let filename = format!(
            "{}/{}_{}",
            self.file_prefix,
            funding_txo.txid.to_hex(),
            funding_txo.index
        );
        let tmp_filename = filename.clone() + ".tmp";

        {
            let mut f = try_fs!(fs::File::create(&tmp_filename));
            try_fs!(monitor.write_for_disk(&mut f));
            try_fs!(f.sync_all());
        }
        // We don't need to create a backup if didn't already have the file, but in any other case
        // try to create the backup and expect failure on fs::copy() if eg there's a perms issue.
        let need_bk = match fs::metadata(&filename) {
            Ok(data) => {
                if !data.is_file() {
                    return Err(channelmonitor::ChannelMonitorUpdateErr::PermanentFailure);
                }
                true
            }
            Err(e) => match e.kind() {
                std::io::ErrorKind::NotFound => false,
                _ => true,
            },
        };
        let bk_filename = filename.clone() + ".bk";
        if need_bk {
            try_fs!(fs::copy(&filename, &bk_filename));
            {
                let f = try_fs!(fs::File::open(&bk_filename));
                try_fs!(f.sync_all());
            }
        }
        try_fs!(fs::rename(&tmp_filename, &filename));
        {
            let f = try_fs!(fs::File::open(&filename));
            try_fs!(f.sync_all());
        }
        if need_bk {
            try_fs!(fs::remove_file(&bk_filename));
        }
        self.monitor.add_update_monitor(funding_txo, monitor)
    }

    fn fetch_pending_htlc_updated(&self) -> Vec<channelmonitor::HTLCUpdate> {
        self.monitor.fetch_pending_htlc_updated()
    }
}