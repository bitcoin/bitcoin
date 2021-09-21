#![no_std]

//! Common messages for [stratum v2][Sv2]
//! The following protocol messages are common across all of the sv2 (sub)protocols.
extern crate alloc;
mod channel_endpoint_changed;
mod setup_connection;

#[cfg(feature = "prop_test")]
use alloc::vec;
#[cfg(feature = "prop_test")]
use core::convert::TryInto;
#[cfg(feature = "prop_test")]
use quickcheck::{Arbitrary, Gen};

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

#[cfg(feature = "prop_test")]
impl ChannelEndpointChanged {
    pub fn from_gen(g: &mut Gen) -> Self {
        ChannelEndpointChanged {
            channel_id: u32::arbitrary(g).try_into().unwrap(),
        }
    }
}

#[cfg(feature = "prop_test")]
impl SetupConnection<'static> {
    pub fn from_gen(g: &mut Gen) -> Self {
        let protocol = setup_connection::Protocol::MiningProtocol;
        // TODO: test each Protocol variant
        // let protocol = setup_connection::Protocol::JobDistributionProtocol;
        // let protocol = setup_connection::Protocol::TemplateDistributionProtocol;
        // let protocol = setup_connection::Protocol::JobNegotiationProtocol;

        let mut endpoint_host_gen = Gen::new(255);
        let mut endpoint_host: vec::Vec<u8> = vec::Vec::new();
        endpoint_host.resize_with(255, || u8::arbitrary(&mut endpoint_host_gen));
        let endpoint_host: binary_sv2::Str0255 = endpoint_host.try_into().unwrap();

        let mut vendor_gen = Gen::new(255);
        let mut vendor: vec::Vec<u8> = vec::Vec::new();
        vendor.resize_with(255, || u8::arbitrary(&mut vendor_gen));
        let vendor: binary_sv2::Str0255 = vendor.try_into().unwrap();

        let mut hardware_version_gen = Gen::new(255);
        let mut hardware_version: vec::Vec<u8> = vec::Vec::new();
        hardware_version.resize_with(255, || u8::arbitrary(&mut hardware_version_gen));
        let hardware_version: binary_sv2::Str0255 = hardware_version.try_into().unwrap();

        let mut firmware_gen = Gen::new(255);
        let mut firmware: vec::Vec<u8> = vec::Vec::new();
        firmware.resize_with(255, || u8::arbitrary(&mut firmware_gen));
        let firmware: binary_sv2::Str0255 = firmware.try_into().unwrap();

        let mut device_id_gen = Gen::new(255);
        let mut device_id: vec::Vec<u8> = vec::Vec::new();
        device_id.resize_with(255, || u8::arbitrary(&mut device_id_gen));
        let device_id: binary_sv2::Str0255 = device_id.try_into().unwrap();

        SetupConnection {
            protocol,
            min_version: u16::arbitrary(g).try_into().unwrap(),
            max_version: u16::arbitrary(g).try_into().unwrap(),
            flags: u32::arbitrary(g).try_into().unwrap(),
            endpoint_host,
            endpoint_port: u16::arbitrary(g).try_into().unwrap(),
            vendor,
            hardware_version,
            firmware,
            device_id,
        }
    }
}

#[cfg(feature = "prop_test")]
impl SetupConnectionError<'static> {
    pub fn from_gen(g: &mut Gen) -> Self {
        let mut error_code_gen = Gen::new(255);
        let mut error_code: vec::Vec<u8> = vec::Vec::new();
        error_code.resize_with(255, || u8::arbitrary(&mut error_code_gen));
        let error_code: binary_sv2::Str0255 = error_code.try_into().unwrap();

        SetupConnectionError {
            flags: u32::arbitrary(g).try_into().unwrap(),
            error_code,
        }
    }
}

#[cfg(feature = "prop_test")]
impl SetupConnectionSuccess {
    pub fn from_gen(g: &mut Gen) -> Self {
        SetupConnectionSuccess {
            used_version: u16::arbitrary(g).try_into().unwrap(),
            flags: u32::arbitrary(g).try_into().unwrap(),
        }
    }
}
