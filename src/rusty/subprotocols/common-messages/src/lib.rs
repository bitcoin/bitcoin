#![no_std]

//! Common messages for [stratum v2][Sv2]
//! The following protocol messages are common across all of the sv2 (sub)protocols.
extern crate alloc;
mod channel_endpoint_changed;
mod setup_connection;

pub use channel_endpoint_changed::ChannelEndpointChanged;
#[cfg(not(feature = "with_serde"))]
pub use setup_connection::{CSetupConnection, CSetupConnectionError};
pub use setup_connection::{
    Protocol, SetupConnection, SetupConnectionError, SetupConnectionSuccess,
};

#[cfg(not(feature = "with_serde"))]
#[no_mangle]
pub extern "C" fn _c_export_channel_endpoint_changed(_a: ChannelEndpointChanged) {}

#[cfg(not(feature = "with_serde"))]
#[no_mangle]
pub extern "C" fn _c_export_setup_conn_succ(_a: SetupConnectionSuccess) {}
