#![no_std]
extern crate alloc;

///
/// Sv2 messages are framed as
/// ```txt
/// extension type: u16
/// msg type: u8
/// msg length: u24
/// payload: [u8; msg length]
/// ```
///
/// Sv2 messages can be encapsulated in noise messages, noise messages are framed as:
///
/// ```txt
/// msg length: u16
/// payload: [u8; msg length]
/// ```
///
///
pub mod framing2;

pub mod header;
