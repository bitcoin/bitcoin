#[cfg(not(test))] mod bridge;
#[cfg(test)] pub mod test_bridge;
#[cfg(test)] pub use test_bridge as bridge;
use bridge::*;

mod dns_headers;

use std::alloc::{GlobalAlloc, Layout, System};
use std::ptr;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::time::{Duration, Instant};

// We keep track of all memory allocated by Rust code, refusing new allocations if it exceeds
// 128MB.
//
// Note that while Rust's std, in general, should panic in response to a null allocation, it
// is totally conceivable that some code will instead dereference this null pointer, which
// would violate our guarantees that Rust modules should never crash the entire application.
//
// In the future, as upstream Rust explores a safer allocation API (eg the Alloc API which
// returns Results instead of raw pointers, or redefining the GlobalAlloc API to allow
// panic!()s inside of alloc calls), we should switch to those, however these APIs are
// currently unstable.
const TOTAL_MEM_LIMIT_BYTES: usize = 128 * 1024 * 1024;
static TOTAL_MEM_ALLOCD: AtomicUsize = AtomicUsize::new(0);
struct MemoryLimitingAllocator;
unsafe impl GlobalAlloc for MemoryLimitingAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let len = layout.size();
        if len > TOTAL_MEM_LIMIT_BYTES {
            return ptr::null_mut();
        }
        if TOTAL_MEM_ALLOCD.fetch_add(len, Ordering::AcqRel) + len > TOTAL_MEM_LIMIT_BYTES {
            TOTAL_MEM_ALLOCD.fetch_sub(len, Ordering::AcqRel);
            return ptr::null_mut();
        }
        System.alloc(layout)
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        System.dealloc(ptr, layout);
        TOTAL_MEM_ALLOCD.fetch_sub(layout.size(), Ordering::AcqRel);
    }
}

#[global_allocator]
static ALLOC: MemoryLimitingAllocator = MemoryLimitingAllocator;

/// Waits for IBD to complete, to get stuck, or shutdown to be initiated. This should be called
/// prior to any background block fetchers initiating connections.
pub fn await_ibd_complete_or_stalled() {
    // Wait until we have finished IBD or aren't making any progress before kicking off
    // redundant sync.
    let mut last_tip = BlockIndex::tip();
    let mut last_tip_change = Instant::now();
    while unsafe { !rusty_ShutdownRequested() } {
        std::thread::sleep(Duration::from_millis(500));
        if unsafe { !rusty_IsInitialBlockDownload() } { break; }
        let new_tip = BlockIndex::tip();
        if new_tip != last_tip {
            last_tip = new_tip;
            last_tip_change = Instant::now();
        } else if (Instant::now() - last_tip_change) > Duration::from_secs(600) {
            break;
        }
    }
}
