use std::ffi::c_void;
extern "C" {
    pub fn rusty_IsInitialBlockDownload() -> bool;
    pub fn rusty_ShutdownRequested() -> bool;

    /// Connects count headers serialized in a block of memory, each stride bytes from each other.
    /// Returns the last header which was connected, if any (or NULL).
    fn rusty_ConnectHeaders(headers: *const u8, stride: usize, count: usize) -> *const c_void;

    // Utilities to work with CBlockIndex pointers. Wrapped in a safe wrapper below.

    /// Gets a CBlockIndex* pointer (casted to a c_void) representing the current tip.
    /// Guaranteed to never be NULL (but may be genesis)
    fn rusty_GetChainTip() -> *const c_void;

    /// Gets the height of a given CBlockIndex* pointer
    fn rusty_IndexToHeight(index: *const c_void) -> i32;
}

/// Connects the given array of (sorted, in chain order) headers (in serialized, 80-byte form).
/// Returns the last header which was connected, if any.
pub fn connect_headers_flat_bytes(headers: &[u8]) -> Option<BlockIndex> {
    if headers.len() % 80 != 0 { return None; }
    if headers.is_empty() { return None; }
    let index = unsafe { rusty_ConnectHeaders(headers.as_ptr(), 80, headers.len() / 80) };
    if index.is_null() { None } else { Some(BlockIndex { index }) }
}

#[derive(PartialEq, Clone, Copy)]
pub struct BlockIndex {
    index: *const c_void,
}

impl BlockIndex {
    pub fn tip() -> Self {
        Self {
            index: unsafe { rusty_GetChainTip() },
        }
    }

    pub fn height(&self) -> i32 {
        unsafe { rusty_IndexToHeight(self.index) }
    }
}
