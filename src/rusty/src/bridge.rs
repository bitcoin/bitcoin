use std::ffi::{c_void, CString};
use std::ptr;
use std::io::Cursor;

use bitcoin::blockdata::block::BlockHeader;
use bitcoin::consensus::Encodable;
use bitcoin::util::uint::Uint256;

#[inline]
pub fn slice_to_u64_le(slice: &[u8]) -> u64 {
    assert_eq!(slice.len(), 8);
    (slice[0] as u64) << 0*8 |
    (slice[1] as u64) << 1*8 |
    (slice[2] as u64) << 2*8 |
    (slice[3] as u64) << 3*8 |
    (slice[4] as u64) << 4*8 |
    (slice[5] as u64) << 5*8 |
    (slice[6] as u64) << 6*8 |
    (slice[7] as u64) << 7*8
}

#[repr(C)]
struct ThirtyTwoBytes {
    pub val: [u8; 32],
}
impl ThirtyTwoBytes {
    pub fn to_uint_le(&self) -> Uint256 {
        Uint256([
            slice_to_u64_le(&self.val[0*8..1*8]),
            slice_to_u64_le(&self.val[1*8..2*8]),
            slice_to_u64_le(&self.val[2*8..3*8]),
            slice_to_u64_le(&self.val[3*8..4*8])
        ])
    }
}

extern "C" {
    pub fn rusty_IsInitialBlockDownload() -> bool;
    pub fn rusty_ShutdownRequested() -> bool;

    fn rusty_ProcessNewBlock(blockdata: *const u8, blockdatalen: usize, blockindex_requested: *const c_void);

    /// Connects count headers serialized in a block of memory, each stride bytes from each other.
    /// Returns the last header which was connected, if any (or NULL).
    fn rusty_ConnectHeaders(headers: *const u8, stride: usize, count: usize) -> *const c_void;

    // Utilities to work with CBlockIndex pointers. Wrapped in a safe wrapper below.

    /// Gets a CBlockIndex* pointer (casted to a c_void) representing the current tip.
    /// Guaranteed to never be NULL (but may be genesis)
    fn rusty_GetChainTip() -> *const c_void;

    /// Gets a CBlockIndex* pointer (casted to a c_void) representing the current best
    /// (not known to be invalid) header.
    /// Guaranteed to never be NULL (but may be genesis)
    fn rusty_GetBestHeader() -> *const c_void;

    /// Gets a CBlockIndex* pointer (casted to a c_void) representing the genesis block.
    /// Guaranteed to never be NULL
    fn rusty_GetGenesisIndex() -> *const c_void;

    /// Finds a CBlockIndex* for a given block hash, or NULL if none is found
    fn rusty_HashToIndex(hash: *const u8) -> *const c_void;

    /// Gets the height of a given CBlockIndex* pointer
    fn rusty_IndexToHeight(index: *const c_void) -> i32;

    /// Gets the hash of a given CBlockIndex* pointer
    fn rusty_IndexToHash(index: *const c_void) -> *const u8;

    /// Gets the immediate ancestor of the given index
    fn rusty_IndexToPrev(index: *const c_void) -> *const c_void;

    /// Gets the ancestor of the given index at the given height
    fn rusty_IndexToAncestor(index: *const c_void, height: i32) -> *const c_void;

    /// Gets the total (estimated) work that went into creating index, as a little endian u256
    fn rusty_IndexToWork(index: *const c_void) -> ThirtyTwoBytes;

    /// Checks whether we currently have a copy of the block this index points to on disk.
    fn rusty_IndexToHaveData(index: *const c_void) -> bool;

    /// Checks whether the block the given index points to is not known to be invalid.
    fn rusty_IndexToNotInvalid(index: *const c_void, invalid_parent_ok: bool) -> bool;

    /// Gets nMinimumChainWork
    fn rusty_MinimumChainWork() -> ThirtyTwoBytes;

    /// Serializes the header pointed to by the CBlockIndex* into eighty_bytes_dest.
    fn rusty_SerializeIndex(index: *const c_void, eighty_bytes_dest: *mut u8);

    /// Given a CBlockIndex* pointer, gets a pointer/length pair for the serialized block,
    /// returning an opaque resource pointer, which must be deallocated (invalidating the
    /// returned serialized data pointer) via rusty_FreeBlockData().
    fn rusty_GetBlockData(index: *const c_void, resdata: *mut *const u8, reslen: *mut u64) -> *const c_void;

    /// Frees the data allocated by rusty_GetBlockData().
    fn rusty_FreeBlockData(dataresource: *const c_void);

    /// Returns true if we're allowed to expose knowledge of the given CBlockIndex* to peers.
    fn rusty_BlockRequestAllowed(pindexvoid: *const c_void) -> bool;
}

/// Connects the given array of (sorted, in chain order) headers (in serialized, 80-byte form).
/// Returns the last header which was connected, if any.
pub fn connect_headers_flat_bytes(headers: &[u8]) -> Option<BlockIndex> {
    if headers.len() % 80 != 0 { return None; }
    if headers.is_empty() { return None; }
    let index = unsafe { rusty_ConnectHeaders(headers.as_ptr(), 80, headers.len() / 80) };
    if index.is_null() { None } else { Some(BlockIndex { index }) }
}

/// Connects the given array of (sorted, in chain order) headers. Note that this is slightly less
/// effecient than the connect_headers_flat_bytes version as the headers are reserialized before
/// handing them to C++.
/// Returns the last header which was connected, if any.
pub fn connect_headers(headers: &[BlockHeader]) -> Option<BlockIndex> {
    let mut encoder = Cursor::new(Vec::with_capacity(headers.len() * 80));
    for header in headers {
        header.consensus_encode(&mut encoder).unwrap();
    }
    connect_headers_flat_bytes(&encoder.into_inner())
}

/// Processes a new block, in serialized form.
/// blockindex_requested_by_state should be set *only* if the given BlockIndex was provided by
/// BlockProviderState::get_next_block_to_download(), and may be set to None always.
pub fn connect_block(blockdata: &[u8], blockindex_requested_by_state: Option<BlockIndex>) {
    let blockindex = match blockindex_requested_by_state { Some(index) => index.index, None => std::ptr::null(), };
    unsafe {
        rusty_ProcessNewBlock(blockdata.as_ptr(), blockdata.len(), blockindex);
    }
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

    pub fn best_header() -> Self {
        Self {
            index: unsafe { rusty_GetBestHeader() },
        }
    }

    /// Gets the a block index with the given hash. Will *not* return old, stale blocks for which
    /// we shouldn't reveal knowledge of a block.
    /// best_tip optimizes this check somewhat, as it can otherwise be expensive (read: take
    /// cs_main).
    pub fn get_from_hash(hash: &[u8; 32], best_tip: Option<BlockIndex>) -> Option<Self> {
        let index = unsafe { rusty_HashToIndex(hash.as_ptr()) };
        if index.is_null() {
            None
        } else {
            let res = Self { index };
            if let Some(tip) = best_tip {
                if tip.get_ancestor(res.height()) == Some(res) {
                    return Some(res);
                }
            }
            if res.is_knowledge_public() {
                Some(Self { index })
            } else { None }
        }
    }

    pub fn genesis() -> Self {
        Self {
            index: unsafe { rusty_GetGenesisIndex() },
        }
    }

    /// Gets the ancestor immediately prior to this block.
    /// Returns Some for any block except genesis
    pub fn get_prev(&self) -> Option<Self> {
        let index = unsafe { rusty_IndexToPrev(self.index) };
        if index.is_null() {
            None
        } else {
            Some(Self { index })
        }
    }

    /// Gets the ancestor 1008 blocks prior to this one, if any
    pub fn get_ancestor(&self, height: i32) -> Option<Self> {
        if height < 0 { return None; }
        let index = unsafe { rusty_IndexToAncestor(self.index, height) };
        if index.is_null() {
            None
        } else {
            Some(Self { index })
        }
    }

    pub fn height(&self) -> i32 {
        unsafe { rusty_IndexToHeight(self.index) }
    }

    pub fn hash(&self) -> [u8; 32] {
        let hashptr = unsafe { rusty_IndexToHash(self.index) };
        if hashptr.is_null() { unreachable!(); }
        let mut res = [0u8; 32];
        unsafe { std::ptr::copy(hashptr, res.as_mut_ptr(), 32) };
        res
    }

    /// Gets the hex formatted hash of this block, in byte-revered order (ie starting with the PoW
    /// 0s, as is commonly used in Bitcoin APIs).
    pub fn hash_hex(&self) -> String {
        let hash_bytes = self.hash();
        let mut res = String::with_capacity(64);
        for b in hash_bytes.iter().rev() {
            res.push(std::char::from_digit((b >> 4) as u32, 16).unwrap());
            res.push(std::char::from_digit((b & 0x0f) as u32, 16).unwrap());
        }
        res
    }

    /// Gets the total number of expected double-SHA256 operations required to build a chain from
    /// genesis to this block.
    pub fn total_work(&self) -> Uint256 {
        unsafe { rusty_IndexToWork(self.index) }.to_uint_le()
    }

    /// Returns true if we have data for this block
    pub fn have_block(&self) -> bool {
        unsafe { rusty_IndexToHaveData(self.index) }
    }

    /// Returns true if this block is not known to be invalid.
    /// Note that this is distinct from being valid, as the block may not have been verified yet.
    /// This *may* include blocks who's parent is invalid (and thus cannot be considered "valid"),
    /// but this is not guaranteed. If you want a guarantee that a block has no invalid parents,
    /// set the boolean "invalid_parent_ok" arg to false and the C++ side will walk the tree up
    /// until it finds a valid parent (or genesis).
    pub fn not_invalid(&self, invalid_parent_ok: bool) -> bool {
        unsafe { rusty_IndexToNotInvalid(self.index, invalid_parent_ok) }
    }

    /// Gets the full, serialized, header
    pub fn header_bytes(&self) -> [u8; 80] {
        let mut ser = [0u8; 80];
        unsafe { rusty_SerializeIndex(self.index, (&mut ser).as_mut_ptr()); }
        ser
    }

    /// Gets the full, serialized, block, in witness form
    pub fn block_bytes(&self) -> Vec<u8> {
        let mut len: u64 = 0;
        let mut data: *const u8 = ptr::null();
        let resource = unsafe { rusty_GetBlockData(self.index, &mut data, &mut len) };
        assert!(!data.is_null());
        let mut res = Vec::new();
        res.resize(len as usize, 0);
        unsafe { ptr::copy_nonoverlapping(data, res.as_mut_ptr(), len as usize); }
        unsafe { rusty_FreeBlockData(resource) };
        res
    }

    /// Returns true if we're allowed to leak to our peers the fact that we know about this header.
    /// In general, once a header is old, if its not on the best chain, we want to avoid letting
    /// anyone know we heard about it, to prevent long-term node fingerprinting attacks.
    /// It shouldn't practically be possible to get a !is_knowledge_public BlockIndex via the
    /// current API, modulo if our best header forks off from our best chain long ago, and you walk
    /// backwards from our best header.
    pub fn is_knowledge_public(&self) -> bool {
        unsafe { rusty_BlockRequestAllowed(self.index) }
    }
}

/// Gets the minimum amount of total work (ie BlockIndex::total_work()) that a chain should have
/// before any blocks along that chain should be stored.
pub fn get_min_chainwork() -> Uint256 {
    unsafe { rusty_MinimumChainWork() }.to_uint_le()
}

extern "C" {
    // Utilities to work with BlockProviderState objects. Wrapped in a safe wrapper below.

    /// Creates a new BlockProviderState with a given current best CBlockIndex*.
    /// Don't forget to de-allocate!
    fn rusty_ProviderStateInit(blockindex: *const c_void) -> *mut c_void;
    /// De-allocates a BlockProviderState.
    fn rusty_ProviderStateFree(provider_state: *mut c_void);

    /// Sets the current best available CBlockIndex* for the given provider state.
    fn rusty_ProviderStateSetBest(provider_state: *mut c_void, blockindex: *const c_void);

    /// Gets the next CBlockIndex* a given provider should download, or NULL
    fn rusty_ProviderStateGetNextDownloads(providerindexvoid: *mut c_void, has_witness: bool) -> *const c_void;
}

/// Tracks a generic block provider's current best tip and allows querying for which blocks to
/// download given a best tip, handling reorgs appropriately. Any blocks which are downloaded from
/// the provider based on a get_next_block_to_download() call should be fed to connect_bock with
/// the blockindex_requested_by_state parameter set to the returned BlockIndex (or a clone()
/// thereof).
///
/// Note that the most-recently-set current best is tracked independently from the C++ logic,
/// making it at least slightly more robust against failure in the C++ code.
pub struct BlockProviderState {
    // TODO: We should be smarter than to keep a copy of the current best pointer twice, but
    // crossing the FFI boundary just to look it up again sucks (and we want to be a bit more
    // robust).
    current_best: BlockIndex,
    state: *mut c_void,
}
impl BlockProviderState {
    /// Initializes block provider state with a given current best header.
    /// Note that you can use a guess on the current best that moves backwards as you discover the
    /// providers' true chain state, though for efficiency you should try to avoid calling
    /// get_next_block_to_download in such a state.
    pub fn new_with_current_best(blockindex: BlockIndex) -> Self {
        Self {
            current_best: blockindex,
            state: unsafe { rusty_ProviderStateInit(blockindex.index) }
        }
    }

    /// Sets the current best available blockindex to the given one on this state.
    pub fn set_current_best(&mut self, blockindex: BlockIndex) {
        self.current_best = blockindex;
        unsafe { rusty_ProviderStateSetBest(self.state, blockindex.index) };
    }

    /// Gets the current best available blockindex as provided previously by set_current_best or
    /// new_with_current_best.
    pub fn get_current_best(&self) -> BlockIndex {
        self.current_best
    }

    /// Gets the BlockIndex representing the next block which should be downloaded, if any.
    pub fn get_next_block_to_download(&mut self, has_witness: bool) -> Option<BlockIndex> {
        let index = unsafe { rusty_ProviderStateGetNextDownloads(self.state, has_witness) };
        if index.is_null() { None } else { Some(BlockIndex { index }) }
    }
}
impl Drop for BlockProviderState {
    fn drop(&mut self) {
        unsafe { rusty_ProviderStateFree(self.state) };
    }
}

extern "C" {
    // General utilities. Wrapped in safe wrappers below.

    /// Log some string
    fn rusty_LogLine(string: *const u8, debug: bool);
}

pub fn log_line(line: &str, debug: bool) {
    let cstr = match CString::new(line) {
        Ok(cstr) => cstr,
        Err(_) => CString::new("Attempted to log an str with nul bytes in it?!").unwrap(),
    };
    let ptr = cstr.as_bytes_with_nul();
    unsafe { rusty_LogLine(ptr.as_ptr(), debug); }
}

extern "C" {
    // Utilities to wrap the C++ FastRandomContext (essentially a ChaCha stream)

    /// Creates and returns a FastRandomContext*. Should be called after we're up so that
    /// it had reandom data to pull from.
    fn rusty_InitRandContext() -> *mut c_void;

    /// Frees a FastRandomContext* generated with rusty_InitRandContext().
    fn rusty_FreeRandContext(rand_context: *mut c_void);

    /// Gets a u64 out of a Random Context generated with rusty_InitRandContext()
    fn rusty_GetRandU64(rand_context: *mut c_void) -> u64;

    /// Gets a u64 less than the given max out of a Random Context generated with rusty_InitRandContext()
    fn rusty_GetRandRange(rand_context: *mut c_void, range: u64) -> u64;
}

pub struct RandomContext {
    index: *mut c_void,
}
impl RandomContext {
    pub fn new() -> Self {
        let index = unsafe { rusty_InitRandContext() };
        assert!(!index.is_null());
        Self { index }
    }

    pub fn get_rand_u64(&mut self) -> u64 {
        unsafe { rusty_GetRandU64(self.index) }
    }

    /// Gets a random number in the range [0..range)
    pub fn randrange(&mut self, range: u64) -> u64 {
        assert!(range > 0);
        unsafe { rusty_GetRandRange(self.index, range) }
    }
}
impl Drop for RandomContext {
    fn drop(&mut self) {
        unsafe { rusty_FreeRandContext(self.index) }
    }
}

extern "C" {
    // P2P-specific utilities.

    /// Begins tracking the given nonce to reject any incoming connections with the same
    /// in their VERSION message, preventing us from connecting to ourselves.
    /// nonce must be random, and you MUST call rusty_DropOutboundP2PNonce afterwards.
    fn rusty_AddOutboundP2PNonce(connman: *mut c_void, nonce: u64);

    /// Stops tracking the given connection nonce, provided to rusty_AddOutboundP2PNonce.
    fn rusty_DropOutboundP2PNonce(connman: *mut c_void, nonce: u64);

    /// Returns false if the given nonce has been used on any of our outbound connections
    /// (including those made by the C++ P2P client and those added via
    /// rusty_AddOutboundP2PNonce).
    fn rusty_CheckInboundP2PNonce(connman: *mut c_void, nonce: u64) -> bool;
}

#[derive(Copy, Clone)]
pub struct Connman(pub *mut c_void);
// As long as we exit when required to, sending around pointers to Connman is fine:
unsafe impl Send for Connman {}

pub struct OutboundP2PNonce {
    connman: Connman,
    nonce: u64,
}
impl OutboundP2PNonce {
    pub fn new(connman: Connman, rand_ctx: &mut RandomContext) -> Self {
        let nonce = rand_ctx.get_rand_u64();
        unsafe { rusty_AddOutboundP2PNonce(connman.0, nonce) }
        Self { connman, nonce }
    }
    pub fn nonce(&self) -> u64 { self.nonce }
}
impl Drop for OutboundP2PNonce  {
    fn drop(&mut self) {
        unsafe { rusty_DropOutboundP2PNonce(self.connman.0, self.nonce) }
    }
}

pub fn should_disconnect_by_inbound_nonce(connman: Connman, nonce: u64) -> bool {
    ! unsafe { rusty_CheckInboundP2PNonce(connman.0, nonce) }
}
