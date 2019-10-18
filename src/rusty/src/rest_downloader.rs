use std::cmp;
use std::net::{TcpStream, ToSocketAddrs};
use std::io::{Read, Write};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::time::Duration;
use std::panic::catch_unwind;

use crate::bridge::*;
use crate::await_ibd_complete_or_stalled;

use std::ffi::CStr;
use std::os::raw::c_char;

/// Splits an HTTP URI into its component part - (is_ssl, hostname, port number, and HTTP path)
fn split_uri<'a>(uri: &'a str) -> Option<(bool, &'a str, u16, &'a str)> {
    let mut uri_iter = uri.splitn(2, ":");
    let ssl = match uri_iter.next() {
        Some("http") => false,
        Some("https") => true,
        _ => return None,
    };
    let mut host_path = match uri_iter.next() {
        Some(r) => r,
        None => return None,
    };
    host_path = host_path.trim_left_matches("/");
    let mut host_path_iter = host_path.splitn(2, "/");
    let (host_port_len, host, port) = match host_path_iter.next() {
        Some(r) if !r.is_empty() => {
            let is_v6_explicit = r.starts_with("[");
            let mut iter = if is_v6_explicit {
                r[1..].splitn(2, "]")
            } else {
                r.splitn(2, ":")
            };
            (r.len(), match iter.next() {
                Some(host) => host,
                None => return None,
            }, match iter.next() {
                Some(port) if !is_v6_explicit || !port.is_empty() => match if is_v6_explicit {
                    if port.as_bytes()[0] != ':' as u8 { return None; }
                    &port[1..]
                } else { port }
                .parse::<u16>() {
                    Ok(p) => p,
                    Err(_) => return None,
                },
                _ => if ssl { 443 } else { 80 },
            })
        },
        _ => return None,
    };
    let path = &host_path[host_port_len..];

    Some((ssl, host, port, path))
}

#[cfg(test)]
#[test]
fn test_split_uri() {
    assert_eq!(split_uri("http://example.com:8080/path"), Some((false, "example.com", 8080, "/path")));
    assert_eq!(split_uri("http:example.com:8080/path/b"), Some((false, "example.com", 8080, "/path/b")));
    assert_eq!(split_uri("https://0.0.0.0/"), Some((true, "0.0.0.0", 443, "/")));
    assert_eq!(split_uri("http:[0:bad::43]:80/"), Some((false, "0:bad::43", 80, "/")));
    assert_eq!(split_uri("http:[::]"), Some((false, "::", 80, "")));
    assert_eq!(split_uri("http://"), None);
    assert_eq!(split_uri("http://example.com:70000/"), None);
    assert_eq!(split_uri("ftp://example.com:80/"), None);
    assert_eq!(split_uri("http://example.com"), Some((false, "example.com", 80, "")));
}

fn read_http_resp(socket: &mut TcpStream, max_resp: usize) -> Option<Vec<u8>> {
    let mut resp = Vec::new();
    let mut bytes_read = 0;
    macro_rules! read_socket { () => { {
        if unsafe { rusty_ShutdownRequested() } { return None; }
        match socket.read(&mut resp[bytes_read..]) {
            Ok(0) => return None,
            Ok(b) => b,
            Err(_) => return None,
        }
    } } }

    let mut actual_len = 0;
    let mut ok_found = false;
    let mut chunked = false;
    // We expect the HTTP headers to fit in 8KB, and use resp as a temporary buffer for headers
    // until we know our real length.
    resp.extend_from_slice(&[0; 8192]);
    'read_headers: loop {
        if bytes_read >= 8192 { return None; }
        bytes_read += read_socket!();
        for line in resp[..bytes_read].split(|c| *c == '\n' as u8 || *c == '\r' as u8) {
            let content_header = b"Content-Length: ";
            if line.len() > content_header.len() && line[..content_header.len()].eq_ignore_ascii_case(content_header) {
                actual_len = match match std::str::from_utf8(&line[content_header.len()..]){
                    Ok(s) => s, Err(_) => return None,
                }.parse() {
                    Ok(len) => len, Err(_) => return None,
                };
            }
            let http_resp_1 = b"HTTP/1.1 200 ";
            let http_resp_0 = b"HTTP/1.0 200 ";
            if line.len() > http_resp_1.len() && (line[..http_resp_1.len()].eq_ignore_ascii_case(http_resp_1) ||
                                                  line[..http_resp_0.len()].eq_ignore_ascii_case(http_resp_0)) {
                ok_found = true;
            }
            let transfer_encoding = b"Transfer-Encoding: ";
            if line.len() > transfer_encoding.len() && line[..transfer_encoding.len()].eq_ignore_ascii_case(transfer_encoding) {
                match &*String::from_utf8_lossy(&line[transfer_encoding.len()..]).to_ascii_lowercase() {
                    "chunked" => chunked = true,
                    _ => return None, // Unsupported
                }
            }
        }
        let mut end_headers_pos = None;
        for (idx, window) in resp[..bytes_read].windows(4).enumerate() {
            if window[0..2] == *b"\n\n" || window[0..2] == *b"\r\r" {
                end_headers_pos = Some(idx + 2);
                break;
            } else if window[0..4] == *b"\r\n\r\n" {
                end_headers_pos = Some(idx + 4);
                break;
            }
        }
        if let Some(pos) = end_headers_pos {
            resp = resp.split_off(pos);
            resp.resize(bytes_read - pos, 0);
            break 'read_headers;
        }
    }
    if !ok_found || (!chunked && (actual_len == 0 || actual_len > max_resp)) { return None; } // Sorry, not implemented
    bytes_read = resp.len();
    if !chunked {
        resp.resize(actual_len, 0);
        while bytes_read < actual_len {
            bytes_read += read_socket!();
        }
        Some(resp)
    } else {
        actual_len = 0;
        let mut chunk_remaining = 0;
        'read_bytes: loop {
            if chunk_remaining == 0 {
                let mut bytes_skipped = 0;
                let mut finished_read = false;
                {
                    let mut lineiter = resp[actual_len..bytes_read].split(|c| *c == '\n' as u8 || *c == '\r' as u8).peekable();
                    loop {
                        let line = match lineiter.next() { Some(line) => line, None => break };
                        if lineiter.peek().is_none() { // We haven't yet read to the end of this line
                            if line.len() > 8 {
                                // No reason to ever have a chunk length line longer than 4 chars
                                return None;
                            }
                            break;
                        }
                        bytes_skipped += line.len() + 1;
                        if line.len() == 0 { continue; } // Probably between the \r and \n
                        match usize::from_str_radix(&match std::str::from_utf8(line) {
                            Ok(s) => s, Err(_) => return None,
                        }, 16) {
                            Ok(chunklen) => {
                                if chunklen == 0 { finished_read = true; }
                                chunk_remaining = chunklen;
                                match lineiter.next() {
                                    Some(l) if l.is_empty() => {
                                        // Drop \r after \n
                                        bytes_skipped += 1;
                                        if actual_len + bytes_skipped > bytes_read {
                                            // Go back and get more bytes so we can skip trailing \n
                                            chunk_remaining = 0;
                                        }
                                    },
                                    Some(_) => {},
                                    None => {
                                        // Go back and get more bytes so we can skip trailing \n
                                        chunk_remaining = 0;
                                    },
                                }
                                break;
                            },
                            Err(_) => return None,
                        }
                    }
                }
                if chunk_remaining != 0 {
                    bytes_read -= bytes_skipped;
                    resp.drain(actual_len..actual_len + bytes_skipped);
                    if actual_len + chunk_remaining > max_resp { return None; }
                    let already_in_chunk = cmp::min(bytes_read - actual_len, chunk_remaining);
                    actual_len += already_in_chunk;
                    chunk_remaining -= already_in_chunk;
                    continue 'read_bytes;
                } else {
                    if finished_read {
                        // Note that we may leave some extra \r\ns to be read, but that's OK,
                        // we'll ignore then when parsing headers for the next request.
                        resp.resize(actual_len, 0);
                        return Some(resp);
                    } else {
                        // Need to read more bytes to figure out chunk length
                    }
                }
            }
            resp.resize(bytes_read + cmp::max(10, chunk_remaining), 0);
            let avail = read_socket!();
            bytes_read += avail;
            if chunk_remaining != 0 {
                let chunk_read = cmp::min(chunk_remaining, avail);
                chunk_remaining -= chunk_read;
                actual_len += chunk_read;
            }
        }
    }
}

static THREAD_COUNT: AtomicUsize = AtomicUsize::new(0);
#[no_mangle]
pub extern "C" fn init_fetch_rest_blocks(uri: *const c_char) -> bool {
    let uri_str: String = match unsafe { CStr::from_ptr(uri) }.to_str() {
        Ok(r) => r.to_string(),
        Err(_) => return false,
    };
    // Sadly only non-SSL is supported for now
    if let Some((false, _, _, _)) = split_uri(&uri_str) { } else { return false; }
    std::thread::spawn(move || {
        // Always catch panics so that even if we have some bug in our parser we don't take the
        // rest of Bitcoin Core down with us:
        THREAD_COUNT.fetch_add(1, Ordering::AcqRel);
        let _ = catch_unwind(move || {
            await_ibd_complete_or_stalled();
            let (ssl, host, port, path) = split_uri(&uri_str).unwrap();
            let mut provider_state = BlockProviderState::new_with_current_best(BlockIndex::tip());
            'reconnect: while unsafe { !rusty_ShutdownRequested() } {
                std::thread::sleep(Duration::from_secs(1));
                if unsafe { rusty_ShutdownRequested() } { return; }

                let mut stream;
                macro_rules! reconnect {
                    () => { {
                        stream = match TcpStream::connect_timeout(&match (host, port).to_socket_addrs() {
                            Ok(mut sockaddrs) => match sockaddrs.next() { Some(sockaddr) => sockaddr, None => continue 'reconnect },
                            Err(_) => continue 'reconnect,
                        }, Duration::from_secs(1)) {
                            Ok(stream) => stream,
                            Err(_) => continue 'reconnect,
                        };
                        stream.set_write_timeout(Some(Duration::from_secs(1))).expect("Host kernel is uselessly old?");
                        stream.set_read_timeout(Some(Duration::from_secs(10))).expect("Host kernel is uselessly old?");
                        if ssl {
                            unimplemented!();
                        }
                    } }
                }
                reconnect!();

                'continue_sync: while unsafe { !rusty_ShutdownRequested() } {
                    macro_rules! write_req {
                        ($req_str: expr) => { {
                            match stream.write($req_str.as_bytes()) {
                                Ok(len) if len == $req_str.len() => {},
                                _ => {
                                    // Reconnect since we've probably timed out and try again.
                                    reconnect!();

                                    match stream.write($req_str.as_bytes()) {
                                        Ok(len) if len == $req_str.len() => {},
                                        _ => continue 'reconnect,
                                    }
                                }
                            }
                        } }
                    }

                    'header_sync: while unsafe { !rusty_ShutdownRequested() } {
                        let req = format!("GET {}/headers/2000/{}.bin HTTP/1.1\nHost: {}\nConnection: keep-alive\n\n", path, provider_state.get_current_best().hash_hex(), host);
                        write_req!(req);
                        let headers = match read_http_resp(&mut stream, 80*2000) {
                            Some(h) => h,
                            None => continue 'reconnect,
                        };
                        if headers.len() == 80 {
                            // We got exactly the header we requested, ie it is *also* the tip for the
                            // remote node, go on to block fetching!
                            break 'header_sync;
                        } else if headers.len() == 0 {
                            let genesis_tip = BlockIndex::genesis();
                            if genesis_tip == provider_state.get_current_best() { // Maybe they're on a different network entirely?
                                continue 'reconnect;
                            }
                            // We are on a fork, but there's nothing in REST (currently) that lets us walk
                            // back until we find the fork point, so we just start downloading again from
                            // genesis, 2000 headers at a time.
                            provider_state.set_current_best(genesis_tip);
                            continue 'header_sync;
                        }
                        match connect_headers_flat_bytes(&headers) {
                            Some(new_best) => {
                                if new_best == provider_state.get_current_best() {
                                    // We tried to connect > 1 header, but didn't move forward, reconnect
                                    // and try again
                                    continue 'reconnect;
                                }
                                provider_state.set_current_best(new_best);
                                continue 'header_sync;
                            },
                            None => {
                                // We consider their response bogus, reconnect and try again
                                continue 'reconnect;
                            },
                        }
                    }

                    // We think we're caught up with their header chain!
                    // Wait a little bit to give the regular (more efficient) P2P logic a chance to
                    // download blocks, then check if we should download anything.
                    for _ in 0..30 {
                        std::thread::sleep(Duration::from_secs(1));
                        if unsafe { rusty_ShutdownRequested() } { return; }
                    }

                    'block_fetch: while unsafe { !rusty_ShutdownRequested() } {
                        match provider_state.get_next_block_to_download(true) {
                            Some(to_fetch) => {
                                let req = format!("GET {}/block/{}.bin HTTP/1.1\nHost: {}\nConnection: keep-alive\n\n", path, to_fetch.hash_hex(), host);
                                write_req!(req);
                                let block = match read_http_resp(&mut stream, 4 * 1000 * 1000) {
                                    Some(b) => b,
                                    None => continue 'reconnect,
                                };
                                connect_block(&block, Some(to_fetch));
                            },
                            None => continue 'continue_sync,
                        }
                    }
                }
            }
        });
        THREAD_COUNT.fetch_sub(1, Ordering::AcqRel);
    });
    true
}

#[no_mangle]
pub extern "C" fn stop_fetch_rest_blocks() {
    while THREAD_COUNT.load(Ordering::Acquire) != 0 {
        std::thread::sleep(Duration::from_millis(10));
    }
}
