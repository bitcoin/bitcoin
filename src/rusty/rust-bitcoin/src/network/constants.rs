// Rust Bitcoin Library
// Written in 2014 by
//   Andrew Poelstra <apoelstra@wpsoftware.net>
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

//! Network constants
//!
//! This module provides various constants relating to the Bitcoin network
//! protocol, such as protocol versioning and magic header bytes.
//!
//! The [`Network`][1] type implements the [`Decodable`][2] and
//! [`Encodable`][3] traits and encodes the magic bytes of the given
//! network
//!
//! [1]: enum.Network.html
//! [2]: ../../consensus/encode/trait.Decodable.html
//! [3]: ../../consensus/encode/trait.Encodable.html
//!
//! # Example: encoding a network's magic bytes
//!
//! ```rust
//! use bitcoin::network::constants::Network;
//! use bitcoin::consensus::encode::serialize;
//!
//! let network = Network::Bitcoin;
//! let bytes = serialize(&network.magic());
//!
//! assert_eq!(&bytes[..], &[0xF9, 0xBE, 0xB4, 0xD9]);
//! ```

/// Version of the protocol as appearing in network message headers
pub const PROTOCOL_VERSION: u32 = 70001;
/// Bitfield of services provided by this node
pub const SERVICES: u64 = 0;
/// User agent as it appears in the version message
pub const USER_AGENT: &'static str = "bitcoin-rust v0.1";

user_enum! {
    /// The cryptocurrency to act on
    #[derive(Copy, PartialEq, Eq, PartialOrd, Ord, Clone, Hash)]
    pub enum Network {
        /// Classic Bitcoin
        Bitcoin <-> "bitcoin",
        /// Bitcoin's testnet
        Testnet <-> "testnet",
        /// Bitcoin's regtest
        Regtest <-> "regtest"
    }
}

impl Network {
    /// Creates a `Network` from the magic bytes.
    ///
    /// # Examples
    ///
    /// ```rust
    /// use bitcoin::network::constants::Network;
    ///
    /// assert_eq!(Some(Network::Bitcoin), Network::from_magic(0xD9B4BEF9));
    /// assert_eq!(None, Network::from_magic(0xFFFFFFFF));
    /// ```
    pub fn from_magic(magic: u32) -> Option<Network> {
        // Note: any new entries here must be added to `magic` below
        match magic {
            0xD9B4BEF9 => Some(Network::Bitcoin),
            0x0709110B => Some(Network::Testnet),
            0xDAB5BFFA => Some(Network::Regtest),
            _ => None
        }
    }

    /// Return the network magic bytes, which should be encoded little-endian
    /// at the start of every message
    ///
    /// # Examples
    ///
    /// ```rust
    /// use bitcoin::network::constants::Network;
    ///
    /// let network = Network::Bitcoin;
    /// assert_eq!(network.magic(), 0xD9B4BEF9);
    /// ```
    pub fn magic(&self) -> u32 {
        // Note: any new entries here must be added to `from_magic` above
        match *self {
            Network::Bitcoin => 0xD9B4BEF9,
            Network::Testnet => 0x0709110B,
            Network::Regtest => 0xDAB5BFFA,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::Network;
    use consensus::encode::{deserialize, serialize};

    #[test]
    fn serialize_test() {
        assert_eq!(
            serialize(&Network::Bitcoin.magic()),
            &[0xf9, 0xbe, 0xb4, 0xd9]
        );
        assert_eq!(
            serialize(&Network::Testnet.magic()),
            &[0x0b, 0x11, 0x09, 0x07]
        );
        assert_eq!(
            serialize(&Network::Regtest.magic()),
            &[0xfa, 0xbf, 0xb5, 0xda]
        );

        assert_eq!(
            deserialize(&[0xf9, 0xbe, 0xb4, 0xd9]).ok(),
            Some(Network::Bitcoin.magic())
        );
        assert_eq!(
            deserialize(&[0x0b, 0x11, 0x09, 0x07]).ok(),
            Some(Network::Testnet.magic())
        );
        assert_eq!(
            deserialize(&[0xfa, 0xbf, 0xb5, 0xda]).ok(),
            Some(Network::Regtest.magic())
        );
    }

  #[test]
  fn string_test() {
      assert_eq!(Network::Bitcoin.to_string(), "bitcoin");
      assert_eq!(Network::Testnet.to_string(), "testnet");
      assert_eq!(Network::Regtest.to_string(), "regtest");

      assert_eq!("bitcoin".parse::<Network>().unwrap(), Network::Bitcoin);
      assert_eq!("testnet".parse::<Network>().unwrap(), Network::Testnet);
      assert_eq!("regtest".parse::<Network>().unwrap(), Network::Regtest);
      assert!("fakenet".parse::<Network>().is_err());
  }
}

