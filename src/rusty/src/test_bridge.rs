///! "bridge" to C++ code without any backing C++ code. Used for shimming in dummies for testing.

use std::collections::HashMap;
use std::ffi::c_void;
use std::sync::atomic::{AtomicPtr, Ordering};
use std::sync::Mutex;

use bitcoin::blockdata::block::BlockHeader;
use bitcoin::util::uint::Uint256;

#[no_mangle]
pub unsafe extern "C" fn rusty_IsInitialBlockDownload() -> bool { unimplemented!(); }

#[no_mangle]
pub unsafe extern "C" fn rusty_ShutdownRequested() -> bool { unimplemented!(); }

pub fn connect_headers_flat_bytes(_headers: &[u8]) -> Option<BlockIndex> {
    unimplemented!();
}

pub fn connect_headers(_headers: &[BlockHeader]) -> Option<BlockIndex> {
    unimplemented!();
}

pub fn connect_block(_blockdata: &[u8], _blockindex_requested_by_state: Option<BlockIndex>) {
    unimplemented!();
}

struct BlockTree {
    blocks: HashMap<[u8; 32], BlockIndex>,
    best_block: [u8; 32],
    best_header: [u8; 32],
}
static BLOCKS: AtomicPtr<Mutex<BlockTree>> = AtomicPtr::new(std::ptr::null_mut());

fn check_blocks_avail() -> &'static Mutex<BlockTree> {
    if BLOCKS.load(Ordering::Relaxed).is_null() {
        let mut new = BlockTree {
            blocks: HashMap::new(),
            best_block: [1; 32],
            best_header: [1; 32],
        };
        new.blocks.insert([1; 32], BlockIndex {
            height: 0,
            hash: [1; 32],
            prev: [0; 32],
        });
        let mut mutex = Box::new(Mutex::new(new));
        let new_ptr = &mut *mutex as *mut Mutex<BlockTree>;
        if let Ok(_) = BLOCKS.compare_exchange(std::ptr::null_mut(), new_ptr, Ordering::AcqRel, Ordering::Acquire) {
            assert!(std::ptr::eq(Box::leak(mutex), new_ptr));
        }
    }
    unsafe { &*BLOCKS.load(Ordering::Relaxed) }
}

/// Extend the block tree by one on top of index
pub fn build_on(index: BlockIndex) -> BlockIndex {
    let mut blocks = check_blocks_avail().lock().unwrap();
    let newblock = BlockIndex {
        height: index.height + 1,
        hash: [
            ((blocks.blocks.len() as u64) >> 7*8) as u8,
            ((blocks.blocks.len() as u64) >> 6*8) as u8,
            ((blocks.blocks.len() as u64) >> 5*8) as u8,
            ((blocks.blocks.len() as u64) >> 4*8) as u8,
            ((blocks.blocks.len() as u64) >> 3*8) as u8,
            ((blocks.blocks.len() as u64) >> 2*8) as u8,
            ((blocks.blocks.len() as u64) >> 1*8) as u8,
            ((blocks.blocks.len() as u64) >> 0*8) as u8,
            (((index.height + 1) as u64) >> 7*8) as u8,
            (((index.height + 1) as u64) >> 6*8) as u8,
            (((index.height + 1) as u64) >> 5*8) as u8,
            (((index.height + 1) as u64) >> 4*8) as u8,
            (((index.height + 1) as u64) >> 3*8) as u8,
            (((index.height + 1) as u64) >> 2*8) as u8,
            (((index.height + 1) as u64) >> 1*8) as u8,
            (((index.height + 1) as u64) >> 0*8) as u8,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        prev: index.hash,
    };
    blocks.blocks.insert(newblock.hash, newblock);
    newblock
}

pub fn set_bests(best_header: BlockIndex, best_block: BlockIndex) {
    let mut blocks = check_blocks_avail().lock().unwrap();
    blocks.best_block = best_block.hash;
    blocks.best_header = best_header.hash;
}

#[derive(Debug, PartialEq, Clone, Copy)]
pub struct BlockIndex {
    height: i32,
    hash: [u8; 32],
    prev: [u8; 32],
}

impl BlockIndex {
    pub fn tip() -> Self {
        unimplemented!();
    }

    pub fn best_header() -> Self {
        unimplemented!();
    }

    pub fn get_from_hash(hash: &[u8; 32], _best_tip: Option<Self>) -> Option<Self> {
        let res = check_blocks_avail().lock().unwrap().blocks.get(hash).cloned();
        if let Some(index) = res {
            if index.is_knowledge_public() { return res; }
        }
        None
    }

    pub fn genesis() -> Self {
        let blocks = check_blocks_avail().lock().unwrap();
        blocks.blocks.get(&[1; 32]).unwrap().clone()
    }

    pub fn get_prev(&self) -> Option<Self> {
        if self.height == 0 { return None; }
        let blocks = check_blocks_avail().lock().unwrap();
        Some(blocks.blocks.get(&self.prev).unwrap().clone())
    }

    pub fn get_ancestor(&self, height: i32) -> Option<Self> {
        if height > self.height || height < 0 { return None; }
        let blocks = check_blocks_avail().lock().unwrap();
        let mut res = *self;
        while res.height != height {
            res = blocks.blocks.get(&res.prev).unwrap().clone();
        }
        Some(res)
    }

    pub fn height(&self) -> i32 {
        self.height
    }

    pub fn hash(&self) -> [u8; 32] {
        self.hash
    }

    pub fn hash_hex(&self) -> String {
        unimplemented!();
    }

    pub fn total_work(&self) -> Uint256 {
        unimplemented!();
    }

    pub fn have_block(&self) -> bool {
        unimplemented!();
    }

    pub fn not_invalid(&self, _invalid_parent_ok: bool) -> bool {
        unimplemented!();
    }

    pub fn header_bytes(&self) -> [u8; 80] {
        unimplemented!();
    }

    pub fn block_bytes(&self) -> Vec<u8> {
        unimplemented!();
    }

    /// Hacky version of the real logic - just reject anything on a fork older than 2500 blocks
    pub fn is_knowledge_public(&self) -> bool {
        let tip = {
            let blocks = check_blocks_avail().lock().unwrap();
            blocks.blocks.get(&blocks.best_block).unwrap().clone()
        };
        self.height > tip.height - 2500 || tip.get_ancestor(self.height).unwrap() == *self
    }
}

pub fn get_min_chainwork() -> Uint256 {
    unimplemented!();
}

pub struct BlockProviderState { }

impl BlockProviderState {
    pub fn new_with_current_best(_blockindex: BlockIndex) -> Self {
        unimplemented!();
    }

    pub fn set_current_best(&mut self, _blockindex: BlockIndex) {
        unimplemented!();
    }

    pub fn get_current_best(&self) -> BlockIndex {
        unimplemented!();
    }

    pub fn get_next_block_to_download(&mut self, _has_witness: bool) -> Option<BlockIndex> {
        unimplemented!();
    }
}

pub fn log_line(_line: &str, _debug: bool) {
    unimplemented!();
}

pub struct RandomContext { }
impl RandomContext {
    pub fn new() -> Self {
        unimplemented!();
    }

    pub fn get_rand_u64(&mut self) -> u64 {
        unimplemented!();
    }

    pub fn randrange(&mut self, _range: u64) -> u64 {
        unimplemented!();
    }
}

#[derive(Copy, Clone)]
pub struct Connman(pub *mut c_void);
// As long as we exit when required to, sending around pointers to Connman is fine:
unsafe impl Send for Connman {}

pub struct OutboundP2PNonce { }
impl OutboundP2PNonce {
    pub fn new(_connman: Connman, _rand_ctx: &mut RandomContext) -> Self {
        unimplemented!();
    }
    pub fn nonce(&self) -> u64 {
        unimplemented!();
    }
}

pub fn should_disconnect_by_inbound_nonce(_connman: Connman, _nonce: u64) -> bool {
    unimplemented!();
}
