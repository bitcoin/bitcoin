///! "bridge" to C++ code without any backing C++ code. Used for shimming in dummies for testing.

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

#[derive(PartialEq, Clone, Copy)]
pub struct BlockIndex { }

impl BlockIndex {
    pub fn tip() -> Self {
        unimplemented!();
    }

    pub fn best_header() -> Self {
        unimplemented!();
    }

    pub fn get_from_hash(_hash: &[u8; 32], _best_tip: Option<Self>) -> Option<Self> {
        unimplemented!();
    }

    pub fn genesis() -> Self {
        unimplemented!();
    }

    pub fn get_prev(&self) -> Option<Self> {
        unimplemented!();
    }

    pub fn get_ancestor(&self, _height: i32) -> Option<Self> {
        unimplemented!();
    }

    pub fn height(&self) -> i32 {
        unimplemented!();
    }

    pub fn hash(&self) -> [u8; 32] {
        unimplemented!();
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

    pub fn is_knowledge_public(&self) -> bool {
        unimplemented!();
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
