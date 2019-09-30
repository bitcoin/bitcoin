///! "bridge" to C++ code without any backing C++ code. Used for shimming in dummies for testing.

#[no_mangle]
pub unsafe extern "C" fn rusty_IsInitialBlockDownload() -> bool { unimplemented!(); }

#[no_mangle]
pub unsafe extern "C" fn rusty_ShutdownRequested() -> bool { unimplemented!(); }

pub fn connect_headers_flat_bytes(_headers: &[u8]) -> Option<BlockIndex> {
    unimplemented!();
}

#[derive(PartialEq, Clone, Copy)]
pub struct BlockIndex { }

impl BlockIndex {
    pub fn tip() -> Self {
        unimplemented!();
    }

    pub fn height(&self) -> i32 {
        unimplemented!();
    }
}
