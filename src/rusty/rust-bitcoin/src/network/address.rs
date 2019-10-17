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

//! Bitcoin network addresses
//!
//! This module defines the structures and functions needed to encode
//! network addresses in Bitcoin messages.
//!

use std::io;
use std::fmt;
use std::net::{SocketAddr, Ipv6Addr, SocketAddrV4, SocketAddrV6};

use consensus::encode::{self, Decodable, Encodable};

/// A message which can be sent on the Bitcoin network
pub struct Address {
    /// Services provided by the peer whose address this is
    pub services: u64,
    /// Network byte-order ipv6 address, or ipv4-mapped ipv6 address
    pub address: [u16; 8],
    /// Network port
    pub port: u16
}

const ONION : [u16; 3] = [0xFD87, 0xD87E, 0xEB43];

impl Address {
    /// Create an address message for a socket
    pub fn new (socket :&SocketAddr, services: u64) -> Address {
        let (address, port) = match socket {
            &SocketAddr::V4(ref addr) => (addr.ip().to_ipv6_mapped().segments(), addr.port()),
            &SocketAddr::V6(ref addr) => (addr.ip().segments(), addr.port())
        };
        Address { address: address, port: port, services: services }
    }

    /// extract socket address from an address message
    /// This will return io::Error ErrorKind::AddrNotAvailable if the message contains a Tor address.
    pub fn socket_addr (&self) -> Result<SocketAddr, io::Error> {
        let addr = &self.address;
        if addr[0..3] == ONION {
            return Err(io::Error::from(io::ErrorKind::AddrNotAvailable));
        }
        let ipv6 = Ipv6Addr::new(
            addr[0],addr[1],addr[2],addr[3],
            addr[4],addr[5],addr[6],addr[7]
        );
        if let Some(ipv4) = ipv6.to_ipv4() {
            Ok(SocketAddr::V4(SocketAddrV4::new(ipv4, self.port)))
        }
        else {
            Ok(SocketAddr::V6(SocketAddrV6::new(ipv6, self.port, 0, 0)))
        }
    }
}

fn addr_to_be(addr: [u16; 8]) -> [u16; 8] {
    [addr[0].to_be(), addr[1].to_be(), addr[2].to_be(), addr[3].to_be(),
     addr[4].to_be(), addr[5].to_be(), addr[6].to_be(), addr[7].to_be()]
}

impl Encodable for Address {
    #[inline]
    fn consensus_encode<S: io::Write>(
        &self,
        mut s: S,
    ) -> Result<usize, encode::Error> {
        let len = self.services.consensus_encode(&mut s)?
            + addr_to_be(self.address).consensus_encode(&mut s)?
            + self.port.to_be().consensus_encode(s)?;
        Ok(len)
    }
}

impl Decodable for Address {
    #[inline]
    fn consensus_decode<D: io::Read>(mut d: D) -> Result<Self, encode::Error> {
        Ok(Address {
            services: Decodable::consensus_decode(&mut d)?,
            address: addr_to_be(Decodable::consensus_decode(&mut d)?),
            port: u16::from_be(Decodable::consensus_decode(d)?)
        })
    }
}

impl fmt::Debug for Address {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        // TODO: render services and hex-ize address
        write!(f, "Address {{services: {:?}, address: {:?}, port: {:?}}}",
               self.services, &self.address[..], self.port)
    }
}

impl Clone for Address {
    fn clone(&self) -> Address {
        Address {
            services: self.services,
            address: self.address,
            port: self.port,
        }
    }
}

impl PartialEq for Address {
    fn eq(&self, other: &Address) -> bool {
        self.services == other.services &&
        &self.address[..] == &other.address[..] &&
        self.port == other.port
    }
}

impl Eq for Address {}

#[cfg(test)]
mod test {
    use std::str::FromStr;
    use super::Address;
    use std::net::{SocketAddr, IpAddr, Ipv4Addr, Ipv6Addr};

    use consensus::encode::{deserialize, serialize};

    #[test]
    fn serialize_address_test() {
        assert_eq!(serialize(&Address {
            services: 1,
            address: [0, 0, 0, 0, 0, 0xffff, 0x0a00, 0x0001],
            port: 8333
        }),
        vec![1u8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0xff, 0xff, 0x0a, 0, 0, 1, 0x20, 0x8d]);
    }

    #[test]
    fn deserialize_address_test() {
        let mut addr: Result<Address, _> = deserialize(&[1u8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                       0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x0a, 0,
                                                       0, 1, 0x20, 0x8d]);
        assert!(addr.is_ok());
        let full = addr.unwrap();
        assert!(match full.socket_addr().unwrap() {
                    SocketAddr::V4(_) => true,
                    _ => false
                }
            );
        assert_eq!(full.services, 1);
        assert_eq!(full.address, [0, 0, 0, 0, 0, 0xffff, 0x0a00, 0x0001]);
        assert_eq!(full.port, 8333);

        addr = deserialize(&[1u8, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x0a, 0, 0, 1]);
        assert!(addr.is_err());
    }

    #[test]
    fn test_socket_addr () {
        let s4 = SocketAddr::new(IpAddr::V4(Ipv4Addr::new(111,222,123,4)), 5555);
        let a4 = Address::new(&s4, 9);
        assert_eq!(a4.socket_addr().unwrap(), s4);
        let s6 = SocketAddr::new(IpAddr::V6(Ipv6Addr::new(0x1111, 0x2222, 0x3333, 0x4444,
        0x5555, 0x6666, 0x7777, 0x8888)), 9999);
        let a6 = Address::new(&s6, 9);
        assert_eq!(a6.socket_addr().unwrap(), s6);
    }

    #[test]
    fn onion_test () {
        let onionaddr = SocketAddr::new(
            IpAddr::V6(
            Ipv6Addr::from_str("FD87:D87E:EB43:edb1:8e4:3588:e546:35ca").unwrap()), 1111);
        let addr = Address::new(&onionaddr, 0);
        assert!(addr.socket_addr().is_err());
    }
}

