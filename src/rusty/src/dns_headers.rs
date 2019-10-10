use std::net::{IpAddr, Ipv6Addr, ToSocketAddrs};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::time::Duration;
use std::panic::catch_unwind;

use std::ffi::CStr;
use std::os::raw::c_char;

use crate::bridge::*;
use crate::await_ibd_complete_or_stalled;

/// Maps a set of six IPv6 addresses to an 80-byte Bitcoin header.
/// The first two bytes of each address are ignored.
/// The next 4 bits in each address indicate the ordering of the addresses
/// (as DNS resolvers/servers often shuffle the addresses)
/// The first 8 bits (ie the second half of the 3rd byte and first half of the 4th)
/// of the first address are interpreted as a version and must currently be 0.
/// The remaining bits are placed into the 80 byte result in order.
fn map_addrs_to_header(ips: &mut [Ipv6Addr]) -> [u8; 80] {
    let mut header = [0u8; 80];
    if ips.len() != 6 { return header; }
    ips.sort_unstable_by(|a, b| {
        // Sort based on the first 4 bits in the 3rd byte...
        (&(a.octets()[2] & 0xf0)).cmp(&(b.octets()[2] & 0xf0))
    });
    if ips.len() != 6 { unreachable!(); }
    let version = (ips[0].octets()[2] & 0x0f) | (ips[0].octets()[3] & 0xf0);
    if version != 0 { return header; }

    let mut offs = 0; // in bytes * 2
    for (idx, ip) in ips.iter().enumerate() {
        for i in if idx == 0 { 3..14*2 } else { 1..14*2 } {
            if i % 2 == 1 {
                header[offs/2] |= (ip.octets()[i/2 + 2] & 0x0f) >> 0;
            } else {
                header[offs/2] |= (ip.octets()[i/2 + 2] & 0xf0) >> 4;
            }
            if offs % 2 == 0 {
                header[offs/2] <<= 4;
            }
            offs += 1;
        }
    }
    header
}

#[cfg(test)]
#[test]
fn test_map_addrs() {
    use std::str::FromStr;

    let mut ips = Vec::new();
    // The genesis header:
    ips.push(Ipv6Addr::from_str("2001:0000:1000:0000:0000:0000:0000:0000").unwrap());
    ips.push(Ipv6Addr::from_str("2001:1000:0000:0000:0000:0000:0000:0000").unwrap());
    ips.push(Ipv6Addr::from_str("2001:2000:0000:0000:0000:0000:03ba:3edf").unwrap());
    ips.push(Ipv6Addr::from_str("2001:3d7a:7b12:b27a:c72c:3e67:768f:617f").unwrap());
    ips.push(Ipv6Addr::from_str("2001:4c81:bc38:88a5:1323:a9fb:8aa4:b1e5").unwrap());
    ips.push(Ipv6Addr::from_str("2001:5e4a:29ab:5f49:ffff:001d:1dac:2b7c").unwrap());

    assert_eq!(&map_addrs_to_header(&mut ips)[..],
        &[0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3b, 0xa3, 0xed, 0xfd, 0x7a, 0x7b, 0x12, 0xb2, 0x7a, 0xc7, 0x2c, 0x3e, 0x67, 0x76, 0x8f, 0x61, 0x7f, 0xc8, 0x1b, 0xc3, 0x88, 0x8a, 0x51, 0x32, 0x3a, 0x9f, 0xb8, 0xaa, 0x4b, 0x1e, 0x5e, 0x4a, 0x29, 0xab, 0x5f, 0x49, 0xff, 0xff, 0x0, 0x1d, 0x1d, 0xac, 0x2b, 0x7c][..]);
}

static THREAD_COUNT: AtomicUsize = AtomicUsize::new(0);

#[no_mangle]
pub extern "C" fn init_fetch_dns_headers(domain: *const c_char) -> bool {
    if let Err(_) = catch_unwind(move || {
        let domain_str: String = match unsafe { CStr::from_ptr(domain) }.to_str() {
            Ok(r) => r.to_string(),
            Err(_) => return false,
        };
        std::thread::spawn(move || {
            // Always catch panics so that even if we have some bug in our parser we don't take the
            // rest of Bitcoin Core down with us:
            THREAD_COUNT.fetch_add(1, Ordering::AcqRel);
            let _ = catch_unwind(move || {
                await_ibd_complete_or_stalled();
                let mut height = BlockIndex::tip().height();
                'dns_lookup: while unsafe { !rusty_ShutdownRequested() } {
                    let mut ips: Vec<_> = match (format!("{}.{}.{}", height, height / 10000, domain_str).as_str(), 0u16).to_socket_addrs() {
                        Ok(ips) => ips,
                        Err(_) => {
                            std::thread::sleep(Duration::from_secs(5));
                            continue 'dns_lookup;
                        },
                    }.filter_map(|a| match a.ip() {
                        IpAddr::V6(a) => Some(a),
                        _ => None,
                    }).collect();
                    if ips.len() != 6 {
                        std::thread::sleep(Duration::from_secs(5));
                        continue 'dns_lookup;
                    }

                    if unsafe { !rusty_ShutdownRequested() } {
                        match connect_headers_flat_bytes(&map_addrs_to_header(&mut ips)) {
                            Some(_) => {
                                height += 1;
                            },
                            None => {
                                // We couldn't connect the header, step back and try again
                                if height > 0 {
                                    height -= 1;
                                } else {
                                    std::thread::sleep(Duration::from_secs(5));
                                }
                            },
                        }
                    }
                }
            });
            THREAD_COUNT.fetch_sub(1, Ordering::AcqRel);
        });
        true
    }) {
        false
    } else {
        true
    }
}

#[no_mangle]
pub extern "C" fn stop_fetch_dns_headers() {
    while THREAD_COUNT.load(Ordering::Acquire) != 0 {
        std::thread::sleep(Duration::from_millis(10));
    }
}
