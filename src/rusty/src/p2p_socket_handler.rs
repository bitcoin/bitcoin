use std::collections::LinkedList;
use std::convert::TryInto;
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::sync::{Arc, Condvar, Mutex};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::net::{SocketAddr, SocketAddrV6, Ipv6Addr};
use std::net::{TcpStream, TcpListener};
use std::io::{Cursor, Read, Write};
use std::os::raw::c_int;
use std::os::unix::io::AsRawFd;

use crate::bridge::*;
use crate::p2p_addrs::*;

use bitcoin::consensus::{Decodable, Encodable};
use bitcoin::consensus::encode::CheckedData;
use bitcoin::network::message::{CommandString, RawNetworkMessage, NetworkMessage};

#[inline]
pub fn slice_to_u32_le(slice: &[u8]) -> u32 {
    assert_eq!(slice.len(), 4);
    (slice[0] as u32) << 0*8 |
    (slice[1] as u32) << 1*8 |
    (slice[2] as u32) << 2*8 |
    (slice[3] as u32) << 3*8
}

pub enum NetMsg {
    Msg(NetworkMessage),
    /// Since we just hand blocks over the wall to C++ for deserialization anyway, we don't bother
    /// to deserialize-reserialize-deserialize blocks and just put them, in full form, in a Vec.
    SerializedBlock(Vec<u8>),
    /// Indicates either the socket should be closed (if sent outbound) or that the socket has been
    /// closed (if received inbound)
    EOF,
}

/// state that gets wrapped in an Arc to pass incoming and outgoing messages into/out of the socket
/// handling thread.
pub struct MessageQueues {
    pub inbound: Mutex<LinkedList<NetMsg>>,
    pub outbound: Mutex<LinkedList<NetMsg>>,
}

const MSG_HDR_LEN: usize = 4 + 12 + 4 + 4;
/// Max number of messages to hold in the message queue, minus one
pub const MAX_QUEUE_LEN: usize = 2;

#[cfg(target_family = "unix")]
mod poll {
    use super::*;
    use libc::{c_void, poll, pollfd, POLLIN, POLLOUT, pipe2, O_NONBLOCK, read};

    /// Reference to a way to wake up the socket handling thread to write data.
    /// Currently implemented as a pipe write fd.
    pub struct Waker(c_int);
    impl Waker {
        pub fn wake(&self) {
            let data = [0u8; 1];
            unsafe { libc::write(self.0, data.as_ptr() as *const libc::c_void, 1) };
        }
    }

    /// socket-handler-thread-specific data about a given peer (buffers and the socket itself).
    pub struct SocketData {
        pub sock: TcpStream,
        pub read_len: usize,
        pub read_buff: Vec<u8>,
        pub write_pos: usize,
        pub write_buff: Vec<u8>,
        pub queues: Arc<MessageQueues>,
    }

    pub struct Sock<'a> {
        pfd: &'a mut pollfd,
        sock_data: &'a mut Option<SocketData>,
    }
    impl<'a> Sock<'a> {
        /// Only valid if using an iterator retruend from poll()!
        pub fn is_readable(&self) -> bool {
            self.pfd.revents & POLLIN == POLLIN
        }
        /// Only valid if using an iterator retruend from poll()!
        pub fn is_writable(&self) -> bool {
            self.pfd.revents & POLLOUT == POLLOUT
        }
        /// Always Some() unless drop_sock has been called on this Sock
        pub fn state<'b>(&'b mut self) -> Option<&'b mut SocketData> {
            self.sock_data.as_mut()
        }
        pub fn drop_sock(&mut self) {
            self.sock_data.as_ref().unwrap().queues.inbound.lock().unwrap().push_back(NetMsg::EOF);
            self.pfd.fd = -1;
            self.pfd.events = 0;
            *self.sock_data = None;
        }
        pub fn pause_read(&mut self) {
            self.pfd.events &= !POLLIN;
        }
        pub fn resume_read(&mut self) {
            self.pfd.events |= POLLIN;
        }
        /// Pause writing for now, note that if any outbound messages appear in queues, we
        /// automatically un-pause writing.
        pub fn pause_write(&mut self) {
            self.pfd.events &= !POLLOUT;
        }
    }

    pub struct SockIter<'a>(std::iter::Zip<std::slice::IterMut<'a, pollfd>, std::slice::IterMut<'a, Option<SocketData>>>);
    impl<'a> Iterator for SockIter<'a> {
        type Item = Sock<'a>;
        fn next(&mut self) -> Option<Sock<'a>> {
            while let Some((pfd, sock_data)) = self.0.next() {
                if sock_data.is_some() {
                    return Some(Sock {
                        pfd,
                        sock_data,
                    });
                }
            }
            None
        }
    }

    pub struct Sockets {
        /// File descriptors we are watching - first is always the wake pipe listen end.
        /// Second is "reserved" for a listen socket.
        /// Thereafter, each fds entry corresponds with a sock_data entry (which may be None,
        /// resulting in a fd of -1).
        fds: Vec<pollfd>,
        sock_data: Vec<Option<SocketData>>,
    }
    impl Sockets {
        pub fn new(listener: &Option<TcpListener>) -> (Self, Waker) {
            let mut wakefds: [c_int; 2] = [0; 2];
            assert_eq!(unsafe { pipe2(wakefds.as_mut_ptr(), O_NONBLOCK) }, 0);
            let waker = Waker(wakefds[1]);
            let fds = vec![pollfd {
                fd: wakefds[0],
                events: POLLIN,
                revents: 0,
            }, match listener {
                Some(sock) => {
                    sock.set_nonblocking(true).unwrap();
                    pollfd {
                        fd: sock.as_raw_fd(),
                        events: POLLIN,
                        revents: 0,
                    }
                },
                None => {
                    pollfd {
                        fd: -1,
                        events: 0,
                        revents: 0,
                    }
                },
            }];
            (Self {
                fds,
                sock_data: Vec::new(),
            }, waker)
        }

        pub fn register_sockdata(&mut self, data: SocketData) {
            data.sock.set_nonblocking(true).unwrap();
            let pfd = pollfd {
                fd: data.sock.as_raw_fd(),
                events: POLLIN | POLLOUT,
                revents: 0,
            };
            for (idx, data_iter) in self.sock_data.iter_mut().enumerate() {
                if data_iter.is_none() {
                    *data_iter = Some(data);
                    self.fds[idx + 2] = pfd;
                    return;
                }
            }
            self.sock_data.push(Some(data));
            self.fds.push(pfd);
        }

        /// Waits for socket events.
        /// Returns (a bool indicating there are incoming sockets available, iterator over
        /// existing-socket events).
        pub fn poll<'a>(&'a mut self, timeout_ms: c_int) -> (bool, SockIter<'a>) {
            // Check if any peers need their writing re-enabled, trying to avoid locking a bit
            // by only checking those with writing already paused.
            for mut sock in self.sock_iter() {
                if sock.pfd.events & POLLOUT == 0 {
                    if !sock.state().unwrap().queues.outbound.lock().unwrap().is_empty() {
                        sock.pfd.events |= POLLOUT;
                    }
                }
            }
            unsafe { poll(self.fds.as_mut_ptr(), self.fds.len().try_into().unwrap(), timeout_ms) };
            if self.fds[0].revents != 0 {
                // Always drain the read side of our wakeup pipe:
                let mut readbuff = [0u8; 8192];
                assert!(unsafe { read(self.fds[0].fd, readbuff.as_mut_ptr() as *mut c_void, 8192) } > 0);
            }
            (self.fds[1].revents & POLLIN == POLLIN, self.sock_iter())
        }

        pub fn sock_iter<'a>(&'a mut self) -> SockIter<'a> {
            let mut fds_iter = self.fds.iter_mut();
            fds_iter.next();
            fds_iter.next();
            SockIter(fds_iter.zip(self.sock_data.iter_mut()))
        }
    }
}
pub use self::poll::Waker;
use self::poll::*;

enum ReadResult {
    /// We maybe read some stuff, we should keep trying to read some stuff.
    Good,
    /// We should disconnect (or already have, and should clean up resources)
    Disconnect,
    /// We've read lots of stuff, we should process it before reading more
    PauseRead,
}

/// Reads from the given socket, deserializing messages into the inbound queue, and potentially
/// pausing read if the queue grows too large.
fn sock_read(sock_state: &mut SocketData, msg_wake_condvar: &Condvar, msg_wake_mutex: &Mutex<()>) -> ReadResult {
    loop { // Read until we have too many pending messages or we get Err(WouldBlock)
        if sock_state.read_len == 0 {
            // We've paused reading, and probably shouldn't have gotten here, but we may have hit
            // some kind of spurious wake, so just return false and move on.
            return ReadResult::PauseRead;
        }
        // We should never be asked to read if we already have a buffer of the next-read size:
        assert!(sock_state.read_buff.len() < sock_state.read_len);
        let read_pos = sock_state.read_buff.len();
        sock_state.read_buff.resize(sock_state.read_len, 0u8);
        match sock_state.sock.read(&mut sock_state.read_buff[read_pos..]) {
            Ok(0) => {
                // EOF - we've been disconnected
                return ReadResult::Disconnect;
            },
            Ok(read_len) => {
                assert!(read_pos + read_len <= sock_state.read_buff.len());
                if read_pos + read_len == sock_state.read_buff.len() {
                    macro_rules! process_msg { () => { {
                        macro_rules! push_msg { ($msg: expr) => { {
                            if {
                                // Push the new message onto the queue, passing whether to
                                // pause read into the if without holding the lock.
                                let mut inbounds = sock_state.queues.inbound.lock().unwrap();
                                inbounds.push_back($msg);
                                inbounds.len() >= MAX_QUEUE_LEN
                            } || sock_state.queues.outbound.lock().unwrap().len() >= MAX_QUEUE_LEN {
                                // Drop the buffer and pause reading...
                                sock_state.read_buff.clear();
                                sock_state.read_buff.shrink_to_fit();
                                sock_state.read_len = 0;
                                return ReadResult::PauseRead;
                            } else {
                                // Re-capacity the buffer to 8KB and resize to 0
                                sock_state.read_buff.resize(8 * 1024, 0u8);
                                sock_state.read_buff.shrink_to_fit();
                                sock_state.read_buff.clear();
                                sock_state.read_len = MSG_HDR_LEN;
                            }
                            {
                                // All we need to do is ensure that the message-handling thread
                                // will check for available messages after we've pushed a message
                                // or go to sleep before we call notify_one(), below. If we just
                                // lock the msg_wake_mutex here, we ensure that either it has
                                // slept, or it has yet to check for available messages when we
                                // exit this block. Thus, we should be good!
                                //
                                // Note that we very much deliberately do not take this lock at the
                                // same time as any of the queue locks, thus leaving any lock
                                // ordering guarantees up to the message-handling thread.
                                let _ = msg_wake_mutex.lock().unwrap();
                            }
                            msg_wake_condvar.notify_one();
                        } } }

                        match u32::consensus_decode(&sock_state.read_buff[..]) {
                            Ok(res) if res == bitcoin::Network::Bitcoin.magic() => {},
                            _ => return ReadResult::Disconnect,
                        }
                        // First deserialize the command. If it is a block, don't deserialize to a
                        // Rust-Bitcoin block (only to reserialize it and hand it to C++), but instead
                        // just check the checksum and hand it over the wall. Otherwise, call Rust-Bitcoin's
                        // deserialize routine for general network messages.
                        match CommandString::consensus_decode(&sock_state.read_buff[4..]) {
                            Ok(CommandString(ref cmd)) if cmd == "block" => {
                                match CheckedData::consensus_decode(&sock_state.read_buff[4 + 12..]) {
                                    Ok(res) => push_msg!(NetMsg::SerializedBlock(res.0)),
                                    Err(_) => return ReadResult::Disconnect,
                                }
                            },
                            Ok(_) => match RawNetworkMessage::consensus_decode(&sock_state.read_buff[..]) {
                                Ok(res) => push_msg!(NetMsg::Msg(res.payload)),
                                Err(bitcoin::consensus::encode::Error::UnrecognizedNetworkCommand(_)) => {
                                    // Re-capacity the buffer to 8KB and resize to 0
                                    sock_state.read_buff.resize(8 * 1024, 0u8);
                                    sock_state.read_buff.shrink_to_fit();
                                    sock_state.read_buff.clear();
                                    sock_state.read_len = MSG_HDR_LEN;
                                },
                                Err(_) => return ReadResult::Disconnect,
                            },
                            Err(_) => return ReadResult::Disconnect,
                        }
                    } } }

                    // If we're currently reading the header, deserialize the payload length then continue
                    // the read loop. If the payload happens to be zero-length, process the message too.
                    if sock_state.read_len == MSG_HDR_LEN {
                        let payload_len = slice_to_u32_le(&sock_state.read_buff[4 + 12..4 + 12 + 4]);
                        if payload_len as usize > bitcoin::consensus::encode::MAX_VEC_SIZE {
                            return ReadResult::Disconnect;
                        }
                        if payload_len == 0 {
                            process_msg!();
                        } else {
                            sock_state.read_len = MSG_HDR_LEN + payload_len as usize;
                        }
                    } else {
                        process_msg!();
                    }
                } else {
                    // Drop the size of the read_buff to how much we've
                    // read. Shouldn't ever result in a realloc (nor
                    // should reading later, since capacity doesn't
                    // change).
                    sock_state.read_buff.resize(read_pos + read_len, 0u8);
                }
            },
            Err(ref e) if e.kind() == std::io::ErrorKind::WouldBlock => {
                sock_state.read_buff.resize(read_pos, 0u8);
                return ReadResult::Good;
            },
            Err(_) => return ReadResult::Disconnect,
        }
    }
}

enum WriteResult {
    /// We maybe wrote some stuff, we should keep trying to write some stuff.
    Good,
    /// We should disconnect (or already have, and should clean up resources)
    Disconnect,
    /// We've written all the stuff.
    DoneWriting,
}

/// Writes to the given socket, taking messages from the outbound queue as necessary.
/// Does *not* automatically unpause read if we've sufficiently drained the outbound queue.
fn sock_write(sock_state: &mut SocketData) -> WriteResult {
    loop { // Write until we get Err(WouldBlock)
        if sock_state.write_pos >= sock_state.write_buff.len() { // ie incl sock_state.write_buff.is_empty()
            let next_out_msg = sock_state.queues.outbound.lock().unwrap().pop_front();
            if let Some(write_msg) = next_out_msg {
                match write_msg {
                    NetMsg::Msg(msg) => {
                        let mut write_buff = Vec::new();
                        std::mem::swap(&mut write_buff, &mut sock_state.write_buff);
                        let mut cursor = Cursor::new(write_buff);
                        RawNetworkMessage {
                            magic: bitcoin::Network::Bitcoin.magic(),
                            payload: msg,
                        }.consensus_encode(&mut cursor).expect("Should only get I/O errors, which Cursor won't generate");
                        std::mem::swap(&mut cursor.into_inner(), &mut sock_state.write_buff);
                        sock_state.write_pos = 0;
                    },
                    NetMsg::SerializedBlock(block) => {
                        let mut write_buff = Vec::new();
                        std::mem::swap(&mut write_buff, &mut sock_state.write_buff);
                        let mut cursor = Cursor::new(write_buff);
                        bitcoin::Network::Bitcoin.magic().consensus_encode(&mut cursor).unwrap();
                        CommandString("block".to_string()).consensus_encode(&mut cursor).unwrap();
                        CheckedData(block).consensus_encode(&mut cursor).unwrap();
                        std::mem::swap(&mut cursor.into_inner(), &mut sock_state.write_buff);
                        sock_state.write_pos = 0;
                    },
                    NetMsg::EOF => { return WriteResult::Disconnect; },
                }
            } else {return WriteResult::DoneWriting; }
        }
        match sock_state.sock.write(&mut sock_state.write_buff[sock_state.write_pos..]) {
            Ok(0) => { panic!(); }, //XXX: No, but need to figure out if this means EOF or WouldBlock!
            Ok(writelen) => {
                assert!(writelen <= sock_state.write_buff.len() - sock_state.write_pos);
                sock_state.write_pos += writelen;
                if sock_state.write_pos == sock_state.write_buff.len() {
                    // Re-capacity the buffer to 8KB and resize to 0
                    sock_state.write_buff.resize(8 * 1024, 0u8);
                    sock_state.write_buff.shrink_to_fit();
                    sock_state.write_buff.clear();
                }
            },
            Err(ref e) if e.kind() == std::io::ErrorKind::WouldBlock => return WriteResult::Good,
            Err(_) => return WriteResult::Disconnect,
        }
    }
}

pub fn spawn_socket_handler(thread_count_tracker: &'static AtomicUsize, bind_port: u16, msg_wake_arg: Arc<Condvar>, msg_wake_mutex: Arc<Mutex<()>>, pending_outbounds: Arc<Mutex<Vec<(Arc<MessageQueues>, NetAddress)>>>, pending_inbounds: Arc<Mutex<Vec<(Arc<MessageQueues>, NetAddress)>>>) -> Waker {
    let listener = if bind_port != 0 {
        if let Ok(listener) = TcpListener::bind(SocketAddr::V6(SocketAddrV6::new(Ipv6Addr::new(0,0,0,0,0,0,0,0), bind_port, 0, 0))) {
            Some(listener)
        } else { None }
    } else { None };
    let (mut poll, waker) = Sockets::new(&listener);

    std::thread::spawn(move || {
        // Always catch panics so that even if we have some bug in our parser we don't take the
        // rest of Bitcoin Core down with us:
        thread_count_tracker.fetch_add(1, Ordering::AcqRel);
        //XXX: WTF DOES THIS MEAN:
        let msg_wake_condvar = AssertUnwindSafe(msg_wake_arg);
        let _ = catch_unwind(move || {
            while unsafe { !rusty_ShutdownRequested() } {
                macro_rules! register_sock {
                    ($queues: expr, $sock: expr) => { {
                        poll.register_sockdata(SocketData {
                            sock: $sock,
                            queues: $queues,
                            read_len: MSG_HDR_LEN,
                            read_buff: Vec::new(),
                            write_pos: 0,
                            write_buff: Vec::new(),
                        });
                    } }
                }

                if {
                    let (inbounds, iter) = poll.poll(100);
                    for mut event in iter {
                        if event.is_readable() {
                            match sock_read(event.state().unwrap(), &msg_wake_condvar, &msg_wake_mutex) {
                                ReadResult::Disconnect => {
                                    event.drop_sock();
                                    continue;
                                },
                                ReadResult::PauseRead => {
                                    event.pause_read()
                                },
                                ReadResult::Good => {},
                            }
                        }

                        if event.is_writable() {
                            match sock_write(event.state().unwrap()) {
                                WriteResult::Disconnect => {
                                    event.drop_sock();
                                    continue;
                                },
                                WriteResult::DoneWriting => {
                                    event.pause_write()
                                },
                                WriteResult::Good => {},
                            }
                        }
                    }
                    inbounds
                } {
                    match listener.as_ref().unwrap().accept() {
                        Ok((sock, addr)) => {
                            let queues = Arc::new(MessageQueues {
                                inbound: Mutex::new(LinkedList::new()),
                                outbound: Mutex::new(LinkedList::new()),
                            });
                            pending_inbounds.lock().unwrap().push((Arc::clone(&queues), NetAddress::new(addr)));
                            register_sock!(queues, sock);
                        },
                        Err(ref e) if e.kind() == std::io::ErrorKind::WouldBlock => {},
                        Err(_) => { panic!("How did we lose a listening sock?!"); },
                    }
                }

                for mut sock in poll.sock_iter() {
                    match {
                        let sock_state = sock.state().unwrap();
                        // If we paused reading for this peer (read_len == 0) and the inbound+outbound
                        // message queues have room again, unpause reading.
                        if sock_state.read_len == 0 && sock_state.queues.inbound.lock().unwrap().len() < MAX_QUEUE_LEN &&
                            sock_state.queues.outbound.lock().unwrap().len() < MAX_QUEUE_LEN {
                            sock_state.read_len = MSG_HDR_LEN;
                            sock_read(sock_state, &msg_wake_condvar, &msg_wake_mutex)
                        } else { ReadResult::Good }
                    } {
                        ReadResult::Disconnect => {
                            sock.drop_sock();
                            continue;
                        },
                        ReadResult::PauseRead => {
                            sock.pause_read()
                        },
                        ReadResult::Good => {
                            sock.resume_read()
                        },
                    }
                }

                // Check if we've been asked to open new connections...
                let mut outbounds = pending_outbounds.lock().unwrap();
                'connect_loop: for (queues, addr) in outbounds.drain(..) {
                    match match addr {
                        NetAddress::IPv4(a) => TcpStream::connect(SocketAddr::V4(a)).ok(),
                        NetAddress::IPv6(a) => TcpStream::connect(SocketAddr::V6(a)).ok(),
                        _ => None,
                    } {
                        Some(sock) => { register_sock!(queues, sock); },
                        None => {},
                    }
                }
            }
        });
        thread_count_tracker.fetch_sub(1, Ordering::AcqRel);
    });
    waker
}
