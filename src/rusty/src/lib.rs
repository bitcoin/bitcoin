#[cfg(not(test))] mod bridge;
#[cfg(test)] pub mod test_bridge;
#[cfg(test)] pub use test_bridge as bridge;
use bridge::*;

mod dns_headers;

use std::time::{Duration, Instant};

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
