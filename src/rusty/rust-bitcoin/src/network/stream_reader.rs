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

//! Stream reader
//!
//! This module defines `StreamReader` struct and its implementation which is used
//! for parsing incoming stream into separate `RawNetworkMessage`s, handling assembling
//! messages from multiple packets or dealing with partial or multiple messages in the stream
//! (like can happen with reading from TCP socket)
//!

use std::fmt;
use std::io::{self, Read};

use consensus::{encode, Decodable};

/// Struct used to configure stream reader function
pub struct StreamReader<R: Read> {
    /// Stream to read from
    pub stream: R,
    /// I/O buffer
    data: Vec<u8>,
    /// Buffer containing unparsed message part
    unparsed: Vec<u8>
}

impl<R: Read> fmt::Debug for StreamReader<R> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "StreamReader with buffer_size={} and unparsed content {:?}",
               self.data.capacity(), self.unparsed)
    }
}

impl<R: Read> StreamReader<R> {
    /// Constructs new stream reader for a given input stream `stream` with
    /// optional parameter `buffer_size` determining reading buffer size
    pub fn new(stream: R, buffer_size: Option<usize>) -> StreamReader<R> {
        StreamReader {
            stream,
            data: vec![0u8; buffer_size.unwrap_or(64 * 1024)],
            unparsed: vec![]
        }
    }

    /// Reads stream and parses next message from its current input,
    /// also taking into account previously unparsed partial message (if there was such).
    pub fn read_next<D: Decodable>(&mut self) -> Result<D, encode::Error> {
        loop {
            match encode::deserialize_partial::<D>(&self.unparsed) {
                // In this case we just have an incomplete data, so we need to read more
                Err(encode::Error::Io(ref err)) if err.kind () == io::ErrorKind::UnexpectedEof => {
                    let count = self.stream.read(&mut self.data)?;
                    if count > 0 {
                        self.unparsed.extend(self.data[0..count].iter());
                    }
                    else {
                        return Err(encode::Error::Io(io::Error::from(io::ErrorKind::UnexpectedEof)));
                    }
                },
                Err(err) => return Err(err),
                // We have successfully read from the buffer
                Ok((message, index)) => {
                    self.unparsed.drain(..index);
                    return Ok(message)
                },
            }
        }
    }
}

#[cfg(test)]
mod test {
    use std::thread;
    use std::time::Duration;
    use std::io::{self, BufReader, Write};
    use std::net::{TcpListener, TcpStream, Shutdown};
    use std::thread::JoinHandle;

    use super::StreamReader;
    use network::message::{NetworkMessage, RawNetworkMessage};

    // First, let's define some byte arrays for sample messages - dumps are taken from live
    // Bitcoin Core node v0.17.1 with Wireshark
    const MSG_VERSION: [u8; 126] = [
        0xf9, 0xbe, 0xb4, 0xd9, 0x76, 0x65, 0x72, 0x73,
        0x69, 0x6f, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x66, 0x00, 0x00, 0x00, 0xbe, 0x61, 0xb8, 0x27,
        0x7f, 0x11, 0x01, 0x00, 0x0d, 0x04, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x4d, 0x5c,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
        0x5b, 0xf0, 0x8c, 0x80, 0xb4, 0xbd, 0x0d, 0x04,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xfa, 0xa9, 0x95, 0x59, 0xcc, 0x68, 0xa1, 0xc1,
        0x10, 0x2f, 0x53, 0x61, 0x74, 0x6f, 0x73, 0x68,
        0x69, 0x3a, 0x30, 0x2e, 0x31, 0x37, 0x2e, 0x31,
        0x2f, 0x93, 0x8c, 0x08, 0x00, 0x01
    ];

    const MSG_VERACK: [u8; 24] = [
        0xf9, 0xbe, 0xb4, 0xd9, 0x76, 0x65, 0x72, 0x61,
        0x63, 0x6b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x5d, 0xf6, 0xe0, 0xe2
    ];

    const MSG_PING: [u8; 32] = [
        0xf9, 0xbe, 0xb4, 0xd9, 0x70, 0x69, 0x6e, 0x67,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x08, 0x00, 0x00, 0x00, 0x24, 0x67, 0xf1, 0x1d,
        0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    ];

    const MSG_ALERT: [u8; 192] = [
        0xf9, 0xbe, 0xb4, 0xd9, 0x61, 0x6c, 0x65, 0x72,
        0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xa8, 0x00, 0x00, 0x00, 0x1b, 0xf9, 0xaa, 0xea,
        0x60, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
        0x7f, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
        0x7f, 0xfe, 0xff, 0xff, 0x7f, 0x01, 0xff, 0xff,
        0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
        0xff, 0x7f, 0x00, 0xff, 0xff, 0xff, 0x7f, 0x00,
        0x2f, 0x55, 0x52, 0x47, 0x45, 0x4e, 0x54, 0x3a,
        0x20, 0x41, 0x6c, 0x65, 0x72, 0x74, 0x20, 0x6b,
        0x65, 0x79, 0x20, 0x63, 0x6f, 0x6d, 0x70, 0x72,
        0x6f, 0x6d, 0x69, 0x73, 0x65, 0x64, 0x2c, 0x20,
        0x75, 0x70, 0x67, 0x72, 0x61, 0x64, 0x65, 0x20,
        0x72, 0x65, 0x71, 0x75, 0x69, 0x72, 0x65, 0x64,
        0x00, 0x46, 0x30, 0x44, 0x02, 0x20, 0x65, 0x3f,
        0xeb, 0xd6, 0x41, 0x0f, 0x47, 0x0f, 0x6b, 0xae,
        0x11, 0xca, 0xd1, 0x9c, 0x48, 0x41, 0x3b, 0xec,
        0xb1, 0xac, 0x2c, 0x17, 0xf9, 0x08, 0xfd, 0x0f,
        0xd5, 0x3b, 0xdc, 0x3a, 0xbd, 0x52, 0x02, 0x20,
        0x6d, 0x0e, 0x9c, 0x96, 0xfe, 0x88, 0xd4, 0xa0,
        0xf0, 0x1e, 0xd9, 0xde, 0xda, 0xe2, 0xb6, 0xf9,
        0xe0, 0x0d, 0xa9, 0x4c, 0xad, 0x0f, 0xec, 0xaa,
        0xe6, 0x6e, 0xcf, 0x68, 0x9b, 0xf7, 0x1b, 0x50
    ];

    // Helper functions that checks parsed versions of the messages from the byte arrays above
    fn check_version_msg(msg: &RawNetworkMessage) {
        assert_eq!(msg.magic, 0xd9b4bef9);
        if let NetworkMessage::Version(ref version_msg) = msg.payload {
            assert_eq!(version_msg.version, 70015);
            assert_eq!(version_msg.services, 1037);
            assert_eq!(version_msg.timestamp, 1548554224);
            assert_eq!(version_msg.nonce, 13952548347456104954);
            assert_eq!(version_msg.user_agent, "/Satoshi:0.17.1/");
            assert_eq!(version_msg.start_height, 560275);
            assert_eq!(version_msg.relay, true);
        } else {
            panic!("Wrong message type: expected VersionMessage");
        }
    }

    fn check_alert_msg(msg: &RawNetworkMessage) {
        assert_eq!(msg.magic, 0xd9b4bef9);
        if let NetworkMessage::Alert(ref alert) = msg.payload {
            assert_eq!(alert.clone(), [
                0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
                0x7f, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
                0x7f, 0xfe, 0xff, 0xff, 0x7f, 0x01, 0xff, 0xff,
                0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
                0xff, 0x7f, 0x00, 0xff, 0xff, 0xff, 0x7f, 0x00,
                0x2f, 0x55, 0x52, 0x47, 0x45, 0x4e, 0x54, 0x3a,
                0x20, 0x41, 0x6c, 0x65, 0x72, 0x74, 0x20, 0x6b,
                0x65, 0x79, 0x20, 0x63, 0x6f, 0x6d, 0x70, 0x72,
                0x6f, 0x6d, 0x69, 0x73, 0x65, 0x64, 0x2c, 0x20,
                0x75, 0x70, 0x67, 0x72, 0x61, 0x64, 0x65, 0x20,
                0x72, 0x65, 0x71, 0x75, 0x69, 0x72, 0x65, 0x64,
                0x00,
            ].to_vec());
        } else {
            panic!("Wrong message type: expected AlertMessage");
        }
    }

    #[test]
    fn parse_multipartmsg_test() {
        let stream = io::empty();
        let mut reader = StreamReader::new(stream, None);
        reader.unparsed = MSG_ALERT[..24].to_vec();
        let message: Result<RawNetworkMessage, _> = reader.read_next();
        assert!(message.is_err());
        assert_eq!(reader.unparsed.len(), 24);

        reader.unparsed = MSG_ALERT.to_vec();
        let message = reader.read_next().unwrap();
        assert_eq!(reader.unparsed.len(), 0);

        check_alert_msg(&message);
    }

    #[test]
    fn read_singlemsg_test() {
        let stream = MSG_VERSION[..].to_vec();
        let stream = stream.as_slice();
        let message = StreamReader::new(stream, None).read_next().unwrap();
        check_version_msg(&message);
    }

    #[test]
    fn read_doublemsgs_test() {
        let mut stream = MSG_VERSION.to_vec();
        stream.extend(&MSG_PING);
        let stream = stream.as_slice();
        let mut reader = StreamReader::new(stream, None);
        let message = reader.read_next().unwrap();
        check_version_msg(&message);

        let msg: RawNetworkMessage = reader.read_next().unwrap();
        assert_eq!(msg.magic, 0xd9b4bef9);
        if let NetworkMessage::Ping(nonce) = msg.payload {
            assert_eq!(nonce, 100);
        } else {
            panic!("Wrong message type, expected PingMessage");
        }
    }

    // Helper function that set ups emulation of client-server TCP connection for
    // testing message transfer via TCP packets
    fn serve_tcp(pieces: Vec<Vec<u8>>) -> (JoinHandle<()>, BufReader<TcpStream>) {
        // 1. Creating server part (emulating Bitcoin Core node)
        let listener = TcpListener::bind(format!("127.0.0.1:{}", 0)).unwrap();
        let port = listener.local_addr().unwrap().port();
        // 2. Spawning thread that will be writing our messages to the TCP Stream at the server side
        // in async mode
        let handle = thread::spawn(move || {
            for ostream in listener.incoming() {
                let mut ostream = ostream.unwrap();

                for piece in pieces {
                    ostream.write(&piece[..]).unwrap();
                    ostream.flush().unwrap();
                    thread::sleep(Duration::from_secs(1));
                }

                ostream.shutdown(Shutdown::Both).unwrap();
                break;
            }
        });

        // 3. Creating client side of the TCP socket connection
        thread::sleep(Duration::from_secs(1));
        let istream = TcpStream::connect(format!("127.0.0.1:{}", port)).unwrap();
        let reader = BufReader::new(istream);

        return (handle, reader)
    }

    #[test]
    fn read_multipartmsg_test() {
        // Setting up TCP connection emulation
        let (handle, istream) = serve_tcp(vec![
            // single message split in two parts to emulate real network conditions
            MSG_VERSION[..24].to_vec(), MSG_VERSION[24..].to_vec()
        ]);
        let stream = istream;
        let mut reader = StreamReader::new(stream, None);

        // Reading and checking the whole message back
        let message = reader.read_next().unwrap();
        check_version_msg(&message);

        // Waiting TCP server thread to terminate
        handle.join().unwrap();
    }

    #[test]
    fn read_sequencemsg_test() {
        // Setting up TCP connection emulation
        let (handle, istream) = serve_tcp(vec![
            // Real-world Bitcoin core communication case for /Satoshi:0.17.1/
            MSG_VERSION[..23].to_vec(), MSG_VERSION[23..].to_vec(),
            MSG_VERACK.to_vec(),
            MSG_ALERT[..24].to_vec(), MSG_ALERT[24..].to_vec()
        ]);
        let stream = istream;
        let mut reader = StreamReader::new(stream, None);

        // Reading and checking the first message (Version)
        let message = reader.read_next().unwrap();
        check_version_msg(&message);

        // Reading and checking the second message (Verack)
        let msg: RawNetworkMessage = reader.read_next().unwrap();
        assert_eq!(msg.magic, 0xd9b4bef9);
        assert_eq!(msg.payload, NetworkMessage::Verack, "Wrong message type, expected VerackMessage");

        // Reading and checking the third message (Alert)
        let msg = reader.read_next().unwrap();
        check_alert_msg(&msg);

        // Waiting TCP server thread to terminate
        handle.join().unwrap();
    }

    #[test]
    fn read_block_from_file_test() {
        use std::io;
        use consensus::serialize;
        use hex::decode as hex_decode;
        use Block;

        let normal_data = hex_decode("010000004ddccd549d28f385ab457e98d1b11ce80bfea2c5ab93015ade4973e400000000bf4473e53794beae34e64fccc471dace6ae544180816f89591894e0f417a914cd74d6e49ffff001d323b3a7b0201000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0804ffff001d026e04ffffffff0100f2052a0100000043410446ef0102d1ec5240f0d061a4246c1bdef63fc3dbab7733052fbbf0ecd8f41fc26bf049ebb4f9527f374280259e7cfa99c48b0e3f39c51347a19a5819651503a5ac00000000010000000321f75f3139a013f50f315b23b0c9a2b6eac31e2bec98e5891c924664889942260000000049483045022100cb2c6b346a978ab8c61b18b5e9397755cbd17d6eb2fe0083ef32e067fa6c785a02206ce44e613f31d9a6b0517e46f3db1576e9812cc98d159bfdaf759a5014081b5c01ffffffff79cda0945903627c3da1f85fc95d0b8ee3e76ae0cfdc9a65d09744b1f8fc85430000000049483045022047957cdd957cfd0becd642f6b84d82f49b6cb4c51a91f49246908af7c3cfdf4a022100e96b46621f1bffcf5ea5982f88cef651e9354f5791602369bf5a82a6cd61a62501fffffffffe09f5fe3ffbf5ee97a54eb5e5069e9da6b4856ee86fc52938c2f979b0f38e82000000004847304402204165be9a4cbab8049e1af9723b96199bfd3e85f44c6b4c0177e3962686b26073022028f638da23fc003760861ad481ead4099312c60030d4cb57820ce4d33812a5ce01ffffffff01009d966b01000000434104ea1feff861b51fe3f5f8a3b12d0f4712db80e919548a80839fc47c6a21e66d957e9c5d8cd108c7a2d2324bad71f9904ac0ae7336507d785b17a2c115e427a32fac00000000").unwrap();
        let cutoff_data = hex_decode("010000004ddccd549d28f385ab457e98d1b11ce80bfea2c5ab93015ade4973e400000000bf4473e53794beae34e64fccc471dace6ae544180816f89591894e0f417a914cd74d6e49ffff001d323b3a7b0201000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0804ffff001d026e04ffffffff0100f2052a0100000043410446ef0102d1ec5240f0d061a4246c1bdef63fc3dbab7733052fbbf0ecd8f41fc26bf049ebb4f9527f374280259e7cfa99c48b0e3f39c51347a19a5819651503a5ac00000000010000000321f75f3139a013f50f315b23b0c9a2b6eac31e2bec98e5891c924664889942260000000049483045022100cb2c6b346a978ab8c61b18b5e9397755cbd17d6eb2fe0083ef32e067fa6c785a02206ce44e613f31d9a6b0517e46f3db1576e9812cc98d159bfdaf759a5014081b5c01ffffffff79cda0945903627c3da1f85fc95d0b8ee3e76ae0cfdc9a65d09744b1f8fc85430000000049483045022047957cdd957cfd0becd642f6b84d82f49b6cb4c51a91f49246908af7c3cfdf4a022100e96b46621f1bffcf5ea5982f88cef651e9354f5791602369bf5a82a6cd61a62501fffffffffe09f5fe3ffbf5ee97a54eb5e5069e9da6b4856ee86fc52938c2f979b0f38e82000000004847304402204165be9a4cbab8049e1af9723b96199bfd3e85f44c6b4c0177e3962686b26073022028f638da23fc003760861ad481ead4099312c60030d4cb57820ce4d33812a5ce01ffffffff01009d966b01000000434104ea1feff861b51fe3f5f8a3b12d0f4712db80e919548a80839fc47c6a21e66d957e9c5d8cd108c7a2d2324bad71f9904ac0ae7336507d785b17a2c115e427a32fac").unwrap();
        let prevhash = hex_decode("4ddccd549d28f385ab457e98d1b11ce80bfea2c5ab93015ade4973e400000000").unwrap();
        let merkle = hex_decode("bf4473e53794beae34e64fccc471dace6ae544180816f89591894e0f417a914c").unwrap();

        let stream = io::BufReader::new(&normal_data[..]);
        let mut reader = StreamReader::new(stream, None);
        let normal_block = reader.read_next::<Block>();

        let stream = io::BufReader::new(&cutoff_data[..]);
        let mut reader = StreamReader::new(stream, None);
        let cutoff_block = reader.read_next::<Block>();

        assert!(normal_block.is_ok());
        assert!(cutoff_block.is_err());
        let block = normal_block.unwrap();
        assert_eq!(block.header.version, 1);
        assert_eq!(serialize(&block.header.prev_blockhash), prevhash);
        assert_eq!(serialize(&block.header.merkle_root), merkle);
        assert_eq!(block.header.time, 1231965655);
        assert_eq!(block.header.bits, 486604799);
        assert_eq!(block.header.nonce, 2067413810);

        // should be also ok for a non-witness block as commitment is optional in that case
        assert!(block.check_witness_commitment());
    }
}
