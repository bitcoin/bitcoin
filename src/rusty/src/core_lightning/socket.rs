use bitcoin::secp256k1::PublicKey;
use lightning::ln::peer_handler;
use lightning::ln::peer_handler::SocketDescriptor as LnSocketTrait;
use std::io::{Read, Write};
use std::net::TcpStream;
use std::sync::atomic::AtomicU64;
use std::sync::atomic::Ordering;
use std::sync::Arc;
use std::{hash, thread};
use core_lightning::event_handler::EventHandler;

static ID_COUNTER: AtomicU64 = AtomicU64::new(0);

pub struct SocketDescriptor {
    event_notify: Arc<EventHandler>,
    pending_read: Vec<u8>,
    read_paused: bool,
    need_disconnect: bool,
    id: u64,
    stream: TcpStream,
    read_scheduled: bool,
}

impl SocketDescriptor {
    pub(crate) fn new(
        event_notify: Arc<EventHandler>,
        stream: TcpStream,
    ) -> Self {
        Self {
            id: ID_COUNTER.fetch_add(1, Ordering::AcqRel),
            event_notify,
            pending_read: Vec::new(),
            read_paused: false,
            need_disconnect: true,
            stream,
            read_scheduled: true,
        }
    }

    pub fn constant_read(&mut self) {
        let mut input_stream = self.stream.try_clone().unwrap();
        let peer_manager_clone = self.event_notify.clone_peer_manager();
        let mut us = self.clone();

        thread::spawn(move || {
            let mut client_buffer = [0u8; 1024];

            loop {
                match input_stream.read(&mut client_buffer) {
                    Ok(n) => {
                        if n > 0 {
                            println!("Read {}!", n);
                            match peer_manager_clone
                                .read_event(&mut us, client_buffer[..n].to_vec())
                            {
                                Ok(pause_read) => {
                                    if pause_read {
                                        us.read_paused = true;
                                    }
                                }
                                Err(e) => {
                                    us.need_disconnect = false;
                                }
                            }

                            us.event_notify.process_events();
                        }
                    }
                    Err(error) => {
                        println!("{}", error.to_string());
                    }
                }
            }
        });
    }

    pub fn setup_outbound(&mut self, their_node_id: PublicKey) {
        if let Ok(initial_send) = self
            .event_notify.clone_peer_manager().new_outbound_connection(their_node_id, self.clone())
        {
            if self.send_data(&initial_send, true) == initial_send.len() {
                self.constant_read();
            }
        }
    }
}

impl peer_handler::SocketDescriptor for SocketDescriptor {
    fn send_data(&mut self, data: &[u8], resume_read: bool) -> usize {
        println!(
            "Writing to: {}",
            // TODO: peer can be disconnected
            self.stream.peer_addr().unwrap().ip().to_string()
        );

        let bytes_written = self.stream.write(data).unwrap();

        println!("Written {}", bytes_written);

        if resume_read {
            self.read_scheduled = true;
        }

        bytes_written
    }

    fn disconnect_socket(&mut self) {
        self.need_disconnect = true;
        self.read_paused = true;
    }
}
impl Eq for SocketDescriptor {}
impl PartialEq for SocketDescriptor {
    fn eq(&self, o: &Self) -> bool {
        self.id == o.id
    }
}
impl hash::Hash for SocketDescriptor {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        self.id.hash(state);
    }
}

impl Clone for SocketDescriptor {
    fn clone(&self) -> SocketDescriptor {
        SocketDescriptor {
            event_notify: self.event_notify.clone(),
            pending_read: self.pending_read.clone(),
            read_paused: self.read_paused,
            need_disconnect: self.need_disconnect,
            id: self.id,
            stream: self.stream.try_clone().unwrap(),
            read_scheduled: self.read_scheduled,
        }
    }
}
