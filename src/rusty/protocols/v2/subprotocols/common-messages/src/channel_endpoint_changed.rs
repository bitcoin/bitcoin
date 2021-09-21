#[cfg(not(feature = "with_serde"))]
use alloc::vec::Vec;
#[cfg(not(feature = "with_serde"))]
use binary_sv2::binary_codec_sv2;
use binary_sv2::{Deserialize, Serialize};

/// ## ChannelEndpointChanged (Server -> Client)
/// When a channel’s upstream or downstream endpoint changes and that channel had previously
/// sent messages with [channel_msg](TODO) bitset of unknown extension_type, the intermediate proxy
/// MUST send a [`ChannelEndpointChanged`] message. Upon receipt thereof, any extension state
/// (including version negotiation and the presence of support for a given extension) MUST be
/// reset and version/presence negotiation must begin again.
///
#[repr(C)]
#[derive(Serialize, Deserialize, Debug, Copy, Clone, PartialEq, Eq)]
pub struct ChannelEndpointChanged {
    /// The channel which has changed endpoint.
    pub channel_id: u32,
}
