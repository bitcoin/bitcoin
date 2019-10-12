///! "bridge" to C++ code without any backing C++ code. Used for shimming in dummies for testing.

#[no_mangle]
pub unsafe extern "C" fn rusty_IsInitialBlockDownload() -> bool { unimplemented!(); }

#[no_mangle]
pub unsafe extern "C" fn rusty_ShutdownRequested() -> bool { unimplemented!(); }

pub fn connect_headers_flat_bytes(_headers: &[u8]) -> Option<BlockIndex> {
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

    pub fn genesis() -> Self {
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
