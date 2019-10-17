// Rust Bitcoin Library
// Written in 2014 by
//     Andrew Poelstra <apoelstra@wpsoftware.net>
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to
// the public domain worldwide. This software is distributed without
// any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication
// along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
//

//! Network-related network messages
//!
//! This module defines network messages which describe peers and their
//! capabilities
//!

use network::address::Address;
use network::constants;
use consensus::{Encodable, Decodable, ReadExt};
use consensus::encode;
use std::io;
use network::message_network::RejectReason::{MALFORMED, INVALID, OBSOLETE, DUPLICATE, NONSTANDARD, DUST, CHECKPOINT, FEE};
use hashes::sha256d;

/// Some simple messages

/// The `version` message
#[derive(PartialEq, Eq, Clone, Debug)]
pub struct VersionMessage {
    /// The P2P network protocol version
    pub version: u32,
    /// A bitmask describing the services supported by this node
    pub services: u64,
    /// The time at which the `version` message was sent
    pub timestamp: i64,
    /// The network address of the peer receiving the message
    pub receiver: Address,
    /// The network address of the peer sending the message
    pub sender: Address,
    /// A random nonce used to detect loops in the network
    pub nonce: u64,
    /// A string describing the peer's software
    pub user_agent: String,
    /// The height of the maximum-work blockchain that the peer is aware of
    pub start_height: i32,
    /// Whether the receiving peer should relay messages to the sender; used
    /// if the sender is bandwidth-limited and would like to support bloom
    /// filtering. Defaults to true.
    pub relay: bool
}

impl VersionMessage {
    // TODO: we have fixed services and relay to 0
    /// Constructs a new `version` message
    pub fn new(
        services: u64,
        timestamp: i64,
        receiver: Address,
        sender: Address,
        nonce: u64,
        user_agent: String,
        start_height: i32,
    ) -> VersionMessage {
        VersionMessage {
            version: constants::PROTOCOL_VERSION,
            services: services,
            timestamp: timestamp,
            receiver: receiver,
            sender: sender,
            nonce: nonce,
            user_agent: user_agent,
            start_height: start_height,
            relay: false,
        }
    }
}

impl_consensus_encoding!(VersionMessage, version, services, timestamp,
                         receiver, sender, nonce,
                         user_agent, start_height, relay);

#[derive(PartialEq, Eq, Clone, Copy, Debug)]
/// message rejection reason as a code
pub enum RejectReason {
    /// malformed message
    MALFORMED = 0x01,
    /// invalid message
    INVALID = 0x10,
    /// obsolete message
    OBSOLETE = 0x11,
    /// duplicate message
    DUPLICATE = 0x12,
    /// nonstandard transaction
    NONSTANDARD = 0x40,
    /// an output is below dust limit
    DUST = 0x41,
    /// insufficient fee
    FEE = 0x42,
    /// checkpoint
    CHECKPOINT = 0x43
}

impl Encodable for RejectReason {
    fn consensus_encode<W: io::Write>(&self, mut e: W) -> Result<usize, encode::Error> {
        e.write_all(&[*self as u8])?;
        Ok(1)
    }
}

impl Decodable for RejectReason {
    fn consensus_decode<D: io::Read>(mut d: D) -> Result<Self, encode::Error> {
        Ok(match d.read_u8()? {
            0x01 => MALFORMED,
            0x10 => INVALID,
            0x11 => OBSOLETE,
            0x12 => DUPLICATE,
            0x40 => NONSTANDARD,
            0x41 => DUST,
            0x42 => FEE,
            0x43 => CHECKPOINT,
            _ => return Err(encode::Error::ParseFailed("unknown reject code"))
        })
    }
}

/// Reject message might be sent by peers rejecting one of our messages
#[derive(PartialEq, Eq, Clone, Debug)]
pub struct Reject {
    /// message type rejected
    pub message: String,
    /// reason of rejection as code
    pub ccode: RejectReason,
    /// reason of rejectection
    pub reason: String,
    /// reference to rejected item
    pub hash: sha256d::Hash
}

impl_consensus_encoding!(Reject, message, ccode, reason, hash);

#[cfg(test)]
mod tests {
    use super::VersionMessage;

    use hex::decode as hex_decode;

    use consensus::encode::{deserialize, serialize};

    #[test]
    fn version_message_test() {
        // This message is from my satoshi node, morning of May 27 2014
        let from_sat = hex_decode("721101000100000000000000e6e0845300000000010000000000000000000000000000000000ffff0000000000000100000000000000fd87d87eeb4364f22cf54dca59412db7208d47d920cffce83ee8102f5361746f7368693a302e392e39392f2c9f040001").unwrap();

        let decode: Result<VersionMessage, _> = deserialize(&from_sat);
        assert!(decode.is_ok());
        let real_decode = decode.unwrap();
        assert_eq!(real_decode.version, 70002);
        assert_eq!(real_decode.services, 1);
        assert_eq!(real_decode.timestamp, 1401217254);
        // address decodes should be covered by Address tests
        assert_eq!(real_decode.nonce, 16735069437859780935);
        assert_eq!(real_decode.user_agent, "/Satoshi:0.9.99/".to_string());
        assert_eq!(real_decode.start_height, 302892);
        assert_eq!(real_decode.relay, true);

        assert_eq!(serialize(&real_decode), from_sat);
    }
}
