#[cfg(not(feature = "with_serde"))]
use alloc::vec::Vec;
use binary_sv2::Str0255;
#[cfg(not(feature = "with_serde"))]
use binary_sv2::{
    binary_codec_sv2, binary_codec_sv2::CVec, decodable::DecodableField, decodable::FieldMarker,
    free_vec, Error, GetSize,
};
use binary_sv2::{Deserialize, Serialize};
use const_sv2::{
    SV2_JOB_DISTR_PROTOCOL_DISCRIMINANT, SV2_JOB_NEG_PROTOCOL_DISCRIMINANT,
    SV2_MINING_PROTOCOL_DISCRIMINANT, SV2_TEMPLATE_DISTR_PROTOCOL_DISCRIMINANT,
};
use core::convert::TryFrom;
#[cfg(not(feature = "with_serde"))]
use core::convert::TryInto;

/// ## SetupConnection (Client -> Server)
/// Initiates the connection. This MUST be the first message sent by the client on the newly
/// opened connection. Server MUST respond with either a [`SetupConnectionSuccess`] or
/// [`SetupConnectionError`] message. Clients that are not configured to provide telemetry data to
/// the upstream node SHOULD set device_id to 0-length strings. However, they MUST always set
/// vendor to a string describing the manufacturer/developer and firmware version and SHOULD
/// always set hardware_version to a string describing, at least, the particular hardware/software
/// package in use.
///
#[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Eq)]
pub struct SetupConnection<'decoder> {
    /// [`Protocol`]
    pub protocol: Protocol,
    /// The minimum protocol version the client supports (currently must be 2).
    pub min_version: u16,
    /// The maximum protocol version the client supports (currently must be 2).
    pub max_version: u16,
    /// Flags indicating optional protocol features the client supports. Each
    /// protocol from [`SetupConnection.protocol`] field has its own values/flags.
    pub flags: u32,
    /// ASCII text indicating the hostname or IP address.
    #[cfg_attr(feature = "with_serde", serde(borrow))]
    pub endpoint_host: Str0255<'decoder>,
    /// Connecting port value
    pub endpoint_port: u16,
    //-- DEVICE INFORMATION --//
    #[cfg_attr(feature = "with_serde", serde(borrow))]
    pub vendor: Str0255<'decoder>,
    #[cfg_attr(feature = "with_serde", serde(borrow))]
    pub hardware_version: Str0255<'decoder>,
    #[cfg_attr(feature = "with_serde", serde(borrow))]
    pub firmware: Str0255<'decoder>,
    #[cfg_attr(feature = "with_serde", serde(borrow))]
    pub device_id: Str0255<'decoder>,
}

#[repr(C)]
#[cfg(not(feature = "with_serde"))]
#[derive(Debug, Clone)]
pub struct CSetupConnection {
    pub protocol: Protocol,
    pub min_version: u16,
    pub max_version: u16,
    pub flags: u32,
    pub endpoint_host: CVec,
    pub endpoint_port: u16,
    pub vendor: CVec,
    pub hardware_version: CVec,
    pub firmware: CVec,
    pub device_id: CVec,
}

#[cfg(not(feature = "with_serde"))]
impl<'a> CSetupConnection {
    #[cfg(not(feature = "with_serde"))]
    pub fn to_rust_rep_mut(&'a mut self) -> Result<SetupConnection<'a>, Error> {
        let endpoint_host: Str0255 = self.endpoint_host.as_mut_slice().try_into()?;
        let vendor: Str0255 = self.vendor.as_mut_slice().try_into()?;
        let hardware_version: Str0255 = self.hardware_version.as_mut_slice().try_into()?;
        let firmware: Str0255 = self.firmware.as_mut_slice().try_into()?;
        let device_id: Str0255 = self.device_id.as_mut_slice().try_into()?;

        Ok(SetupConnection {
            protocol: self.protocol,
            min_version: self.min_version,
            max_version: self.max_version,
            flags: self.flags,
            endpoint_host,
            endpoint_port: self.endpoint_port,
            vendor,
            hardware_version,
            firmware,
            device_id,
        })
    }
}

#[no_mangle]
#[cfg(not(feature = "with_serde"))]
pub extern "C" fn free_setup_connection(s: CSetupConnection) {
    drop(s)
}

#[cfg(not(feature = "with_serde"))]
impl Drop for CSetupConnection {
    fn drop(&mut self) {
        free_vec(&mut self.endpoint_host);
        free_vec(&mut self.vendor);
        free_vec(&mut self.hardware_version);
        free_vec(&mut self.firmware);
        free_vec(&mut self.device_id);
    }
}

#[cfg(not(feature = "with_serde"))]
impl<'a> From<SetupConnection<'a>> for CSetupConnection {
    fn from(v: SetupConnection) -> Self {
        Self {
            protocol: v.protocol,
            min_version: v.min_version,
            max_version: v.max_version,
            flags: v.flags,
            endpoint_host: v.endpoint_host.into(),
            endpoint_port: v.endpoint_port,
            vendor: v.vendor.into(),
            hardware_version: v.hardware_version.into(),
            firmware: v.firmware.into(),
            device_id: v.device_id.into(),
        }
    }
}

/// ## SetupConnection.Success (Server -> Client)
/// Response to [`SetupConnection`] message if the server accepts the connection. The client is
/// required to verify the set of feature flags that the server supports and act accordingly.
#[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct SetupConnectionSuccess {
    /// Selected version proposed by the connecting node that the upstream
    /// node supports. This version will be used on the connection for the rest
    /// of its life.
    pub used_version: u16,
    /// Flags indicating optional protocol features the server supports. Each
    /// protocol from [`Protocol`] field has its own values/flags.
    pub flags: u32,
}

/// ## SetupConnection.Error (Server -> Client)
/// When protocol version negotiation fails (or there is another reason why the upstream node
/// cannot setup the connection) the server sends this message with a particular error code prior
/// to closing the connection.
/// In order to allow a client to determine the set of available features for a given server (e.g. for
/// proxies which dynamically switch between different pools and need to be aware of supported
/// options), clients SHOULD send a SetupConnection message with all flags set and examine the
/// (potentially) resulting [`SetupConnectionError`] message’s flags field. The Server MUST provide
/// the full set of flags which it does not support in each [`SetupConnectionError`] message and
/// MUST consistently support the same set of flags across all servers on the same hostname and
/// port number. If flags is 0, the error is a result of some condition aside from unsupported flags.
#[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Eq)]
pub struct SetupConnectionError<'decoder> {
    /// Flags indicating features causing an error.
    pub flags: u32,
    /// Human-readable error code(s). See Error Codes section, [link](TODO).
    /// ### Possible error codes:
    /// * ‘unsupported-feature-flags’
    /// * ‘unsupported-protocol’
    /// * ‘protocol-version-mismatch’
    #[cfg_attr(feature = "with_serde", serde(borrow))]
    pub error_code: Str0255<'decoder>,
}

#[repr(C)]
#[cfg(not(feature = "with_serde"))]
#[derive(Debug, Clone)]
pub struct CSetupConnectionError {
    flags: u32,
    error_code: CVec,
}

#[cfg(not(feature = "with_serde"))]
impl<'a> CSetupConnectionError {
    #[cfg(not(feature = "with_serde"))]
    pub fn to_rust_rep_mut(&'a mut self) -> Result<SetupConnectionError<'a>, Error> {
        let error_code: Str0255 = self.error_code.as_mut_slice().try_into()?;

        Ok(SetupConnectionError {
            flags: self.flags,
            error_code,
        })
    }
}

#[no_mangle]
#[cfg(not(feature = "with_serde"))]
pub extern "C" fn free_setup_connection_error(s: CSetupConnectionError) {
    drop(s)
}

#[cfg(not(feature = "with_serde"))]
impl Drop for CSetupConnectionError {
    fn drop(&mut self) {
        free_vec(&mut self.error_code);
    }
}

#[cfg(not(feature = "with_serde"))]
impl<'a> From<SetupConnectionError<'a>> for CSetupConnectionError {
    fn from(v: SetupConnectionError<'a>) -> Self {
        Self {
            flags: v.flags,
            error_code: v.error_code.into(),
        }
    }
}

/// MiningProtocol = [`SV2_MINING_PROTOCOL_DISCRIMINANT`],
/// JobNegotiationProtocol = [`SV2_JOB_NEG_PROTOCOL_DISCRIMINANT`],
/// TemplateDistributionProtocol = [`SV2_TEMPLATE_DISTR_PROTOCOL_DISCRIMINANT`],
/// JobDistributionProtocol = [`SV2_JOB_DISTR_PROTOCOL_DISCRIMINANT`],
#[cfg_attr(feature = "with_serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
#[allow(clippy::enum_variant_names)]
pub enum Protocol {
    MiningProtocol = SV2_MINING_PROTOCOL_DISCRIMINANT,
    JobNegotiationProtocol = SV2_JOB_NEG_PROTOCOL_DISCRIMINANT,
    TemplateDistributionProtocol = SV2_TEMPLATE_DISTR_PROTOCOL_DISCRIMINANT,
    JobDistributionProtocol = SV2_JOB_DISTR_PROTOCOL_DISCRIMINANT,
}

#[cfg(not(feature = "with_serde"))]
impl<'a> From<Protocol> for binary_sv2::encodable::EncodableField<'a> {
    fn from(v: Protocol) -> Self {
        let val = v as u8;
        val.into()
    }
}

#[cfg(not(feature = "with_serde"))]
impl<'decoder> binary_sv2::Decodable<'decoder> for Protocol {
    fn get_structure(
        _: &[u8],
    ) -> core::result::Result<alloc::vec::Vec<FieldMarker>, binary_sv2::Error> {
        let field: FieldMarker = 0_u8.into();
        Ok(alloc::vec![field])
    }
    fn from_decoded_fields(
        mut v: alloc::vec::Vec<DecodableField<'decoder>>,
    ) -> core::result::Result<Self, binary_sv2::Error> {
        let val = v.pop().unwrap();
        let val: u8 = val.try_into().unwrap();
        Ok(val.try_into().unwrap())
    }
}

impl TryFrom<u8> for Protocol {
    type Error = ();

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            SV2_MINING_PROTOCOL_DISCRIMINANT => Ok(Protocol::MiningProtocol),
            SV2_JOB_NEG_PROTOCOL_DISCRIMINANT => Ok(Protocol::JobNegotiationProtocol),
            SV2_TEMPLATE_DISTR_PROTOCOL_DISCRIMINANT => Ok(Protocol::TemplateDistributionProtocol),
            SV2_JOB_DISTR_PROTOCOL_DISCRIMINANT => Ok(Protocol::JobDistributionProtocol),
            _ => Err(()),
        }
    }
}

#[cfg(not(feature = "with_serde"))]
impl GetSize for Protocol {
    fn get_size(&self) -> usize {
        1
    }
}
