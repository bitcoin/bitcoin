use std::collections::HashSet;
use std::net::{SocketAddr, SocketAddrV4, SocketAddrV6, ToSocketAddrs, Ipv4Addr, Ipv6Addr};
use std::{fs, io};
use std::io::Seek;

use bitcoin::network::address::Address as BitcoinAddr;

use bridge::*;

/// An address which can be used to connect to a remote peer
#[derive(Clone, PartialEq, Eq, Debug, Hash)]
pub enum NetAddress {
    /// An IPv4 address/port on which the peer is listening.
    IPv4(SocketAddrV4),
    /// An IPv6 address/port on which the peer is listening.
    IPv6(SocketAddrV6),
    /// An old-style Tor onion address/port on which the peer is listening.
    OnionV2 {
        /// The bytes (usually encoded in base32 with ".onion" appended)
        addr: [u8; 10],
        /// The port on which the node is listening
        port: u16,
    },
    /// A new-style Tor onion address/port on which the peer is listening.
    /// To create the human-readable "hostname", concatenate ed25519_pubkey, checksum, and version,
    /// wrap as base32 and append ".onion".
    OnionV3 {
        /// The ed25519 long-term public key of the peer
        ed25519_pubkey: [u8; 32],
        /// The checksum of the pubkey and version, as included in the onion address
        checksum: u16,
        /// The version byte, as defined by the Tor Onion v3 spec.
        version: u8,
        /// The port on which the node is listening
        port: u16,
    },
}

impl NetAddress {
    pub fn write<W : io::Write>(&self, write: &mut W) -> Result<(), io::Error> {
        match self {
            NetAddress::IPv4(sockaddr) => {
                write.write_all(&[0, 8])?;
                write.write_all(&sockaddr.ip().octets())?;
                write.write_all(&[(sockaddr.port() >> 8) as u8, sockaddr.port() as u8])?;
            },
            NetAddress::IPv6(sockaddr) => {
                write.write_all(&[1, 20])?;
                write.write_all(&sockaddr.ip().octets())?;
                write.write_all(&[(sockaddr.port() >> 8) as u8, sockaddr.port() as u8])?;
            },
            NetAddress::OnionV2 { addr, port } => {
                write.write_all(&[2, 14])?;
                write.write_all(addr)?;
                write.write_all(&[(*port >> 8) as u8, *port as u8])?;
            },
            NetAddress::OnionV3 { ed25519_pubkey, checksum, version, port } => {
                write.write_all(&[3, 39])?;
                write.write_all(ed25519_pubkey)?;
                write.write_all(&[(*checksum >> 8) as u8, *checksum as u8])?;
                write.write_all(&[*version])?;
                write.write_all(&[(*port >> 8) as u8, *port as u8])?;
            },
        }
        Ok(())
    }

    pub fn new(addr: SocketAddr) -> NetAddress {
        match addr {
            SocketAddr::V4(sa) => NetAddress::IPv4(sa),
            SocketAddr::V6(sa) if sa.ip().octets()[..6] == [0xFD,0x87,0xD8,0x7E,0xEB,0x43][..] => {
                let mut addr = [0; 10];
                addr.copy_from_slice(&sa.ip().octets()[6..]);
                NetAddress::OnionV2 {
                    addr,
                    port: sa.port(),
                }
            },
            SocketAddr::V6(sa) => if let Some(v4ip) = sa.ip().to_ipv4() {
                    NetAddress::IPv4(SocketAddrV4::new(v4ip, sa.port()))
                } else {
                    NetAddress::IPv6(sa)
                },
        }
    }
}

struct AddrStream<'a, R : io::Read> (Option<&'a mut R>);
impl<'a, R : io::Read> AddrStream<'a, R> {
    pub fn new(read: &'a mut R) -> AddrStream<'a, R> {
        let mut ver_info = [0u8; 2]; // (min ver, max ver)
        if let Ok(_) = read.read_exact(&mut ver_info) {
            if ver_info[0] <= 1 {
                Self(Some(read))
            } else { Self(None) }
        } else { Self(None) }
    }
}
impl<'a, R : io::Read> Iterator for AddrStream<'a, R> {
    type Item = NetAddress;
    fn next(&mut self) -> Option<NetAddress> {
        'bad_file: loop {
            match &mut self.0 {
                &mut None => return None,
                &mut Some(ref mut r) => {
                    let mut addr = [0; 255];
                    // Min length is 8 (type, len, ipv4, port)
                    if let Ok(_) = r.read_exact(&mut addr[0..2]) {
                        let len = addr[1] as usize;
                        if len < 8 { break 'bad_file; }
                        else if let Ok(_) = r.read_exact(&mut addr[2..len]) {
                            match addr[0] {
                                0 if addr[1] == 8 => {
                                    return Some(NetAddress::IPv4(SocketAddrV4::new(
                                                Ipv4Addr::new(addr[2], addr[3], addr[4], addr[5]),
                                                 ((addr[6] as u16) << 8) | addr[7] as u16)));
                                },
                                1 if addr[1] == 20 => {
                                    return Some(NetAddress::IPv6(SocketAddrV6::new(
                                                 Ipv6Addr::new(((addr[ 2] as u16) << 8) | addr[ 3] as u16,
                                                               ((addr[ 4] as u16) << 8) | addr[ 5] as u16,
                                                               ((addr[ 6] as u16) << 8) | addr[ 7] as u16,
                                                               ((addr[ 8] as u16) << 8) | addr[ 9] as u16,
                                                               ((addr[10] as u16) << 8) | addr[11] as u16,
                                                               ((addr[12] as u16) << 8) | addr[13] as u16,
                                                               ((addr[14] as u16) << 8) | addr[15] as u16,
                                                               ((addr[16] as u16) << 8) | addr[17] as u16),
                                                 ((addr[18] as u16) << 8) | addr[19] as u16, 0, 0)));
                                },
                                2 if addr[1] == 14 => {
                                    let mut onion_bytes = [0; 10];
                                    onion_bytes.copy_from_slice(&mut addr[2..12]);
                                    return Some(NetAddress::OnionV2 {
                                        addr: onion_bytes,
                                        port: ((addr[12] as u16) << 8) | addr[13] as u16,
                                    });
                                },
                                3 if addr[1] == 2 + 32 + 2 + 1 + 2 => {
                                    let mut ed25519_pubkey = [0; 32];
                                    ed25519_pubkey.copy_from_slice(&mut addr[2..34]);
                                    return Some(NetAddress::OnionV3 {
                                        ed25519_pubkey,
                                        checksum: ((addr[34] as u16) << 8) | addr[35] as u16,
                                        version: addr[36],
                                        port: ((addr[37] as u16) << 8) | addr[38] as u16,
                                    });
                                },
                                _ => { },
                            }
                        } else { break 'bad_file; }
                    } else { break 'bad_file; }
                },
            };
        }
        self.0 = None;
        None
    }
}

pub struct AddrTracker {
    addr_set: HashSet<NetAddress>,
}
impl AddrTracker {
    pub fn new(addr_path: String, dnsseeds: Vec<String>) -> Self {
        // First read up to 1000 addrs off disk, and if we don't have 500, pull up to 25/seed
        // from the dnsseeds, then write them back to disk.
        let mut addr_f = fs::OpenOptions::new().read(true).write(true).create(true).open(addr_path).expect("Failed to open addr database");
        let mut addr_set: HashSet<_> = AddrStream::new(&mut addr_f).take(1000).collect();
        if addr_set.len() < 500 {
            for dnsseed in dnsseeds {
                if let Ok(addrs) = dnsseed.to_socket_addrs() {
                    for addr in addrs.take(25) {
                        addr_set.insert(NetAddress::new(addr));
                    }
                }
            }
            addr_f.seek(io::SeekFrom::Start(0)).unwrap();
            for addr in addr_set.iter() {
                addr.write(&mut addr_f).expect("Failed to write new addresses");
            }
        }
        Self { addr_set }
    }

    pub fn get_rand_addr(&self, rand_ctx: &mut RandomContext) -> Option<NetAddress> {
        if self.addr_set.is_empty() {
            None
        } else {
            self.addr_set.iter().skip(rand_ctx.randrange(self.addr_set.len() as u64) as usize).next().cloned()
        }
    }

    pub fn addrs_recvd(&mut self, _from: &NetAddress, _outbound: bool, _addrs: &Vec<(u32, BitcoinAddr)>) {}
}
