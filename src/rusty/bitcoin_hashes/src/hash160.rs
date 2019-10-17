// Bitcoin Hashes Library
// Written in 2018 by
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

// This module is largely copied from the rust-crypto ripemd.rs file;
// while rust-crypto is licensed under Apache, that file specifically
// was written entirely by Andrew Poelstra, who is re-licensing its
// contents here as CC0.

//! # HASH160 (SHA256 then RIPEMD160)

use core::str;

use sha256;
use ripemd160;
use Hash as HashTrait;
use Error;

/// Output of the Bitcoin HASH160 hash function
#[derive(Copy, Clone, PartialEq, Eq, Default, PartialOrd, Ord, Hash)]
pub struct Hash([u8; 20]);

hex_fmt_impl!(Debug, Hash);
hex_fmt_impl!(Display, Hash);
hex_fmt_impl!(LowerHex, Hash);
index_impl!(Hash);
serde_impl!(Hash, 20);
borrow_slice_impl!(Hash);

impl str::FromStr for Hash {
    type Err = ::hex::Error;
    fn from_str(s: &str) -> Result<Self, Self::Err> {
        ::hex::FromHex::from_hex(s)
    }
}

impl HashTrait for Hash {
    type Engine = sha256::HashEngine;
    type Inner = [u8; 20];

    fn engine() -> sha256::HashEngine {
        sha256::Hash::engine()
    }

    fn from_engine(e: sha256::HashEngine) -> Hash {
        let sha2 = sha256::Hash::from_engine(e);
        let rmd = ripemd160::Hash::hash(&sha2[..]);

        let mut ret = [0; 20];
        ret.copy_from_slice(&rmd[..]);
        Hash(ret)
    }

    const LEN: usize = 20;

    fn from_slice(sl: &[u8]) -> Result<Hash, Error> {
        if sl.len() != 20 {
            Err(Error::InvalidLength(Self::LEN, sl.len()))
        } else {
            let mut ret = [0; 20];
            ret.copy_from_slice(sl);
            Ok(Hash(ret))
        }
    }

    fn into_inner(self) -> Self::Inner {
        self.0
    }

    fn from_inner(inner: Self::Inner) -> Self {
        Hash(inner)
    }
}

#[cfg(test)]
mod tests {
    use hash160;
    use hex::{FromHex, ToHex};
    use Hash;
    use HashEngine;

    #[derive(Clone)]
    struct Test {
        input: Vec<u8>,
        output: Vec<u8>,
        output_str: &'static str,
    }

    #[test]
    fn test() {
        let tests = vec![
            // Uncompressed pubkey obtained from Bitcoin key; data from validateaddress
            Test {
                input: vec![
                    0x04, 0xa1, 0x49, 0xd7, 0x6c, 0x5d, 0xe2, 0x7a, 0x2d,
                    0xdb, 0xfa, 0xa1, 0x24, 0x6c, 0x4a, 0xdc, 0xd2, 0xb6,
                    0xf7, 0xaa, 0x29, 0x54, 0xc2, 0xe2, 0x53, 0x03, 0xf5,
                    0x51, 0x54, 0xca, 0xad, 0x91, 0x52, 0xe4, 0xf7, 0xe4,
                    0xb8, 0x5d, 0xf1, 0x69, 0xc1, 0x8a, 0x3c, 0x69, 0x7f,
                    0xbb, 0x2d, 0xc4, 0xec, 0xef, 0x94, 0xac, 0x55, 0xfe,
                    0x81, 0x64, 0xcc, 0xf9, 0x82, 0xa1, 0x38, 0x69, 0x1a,
                    0x55, 0x19,
                ],
                output: vec![
                    0xda, 0x0b, 0x34, 0x52, 0xb0, 0x6f, 0xe3, 0x41,
                    0x62, 0x6a, 0xd0, 0x94, 0x9c, 0x18, 0x3f, 0xbd,
                    0xa5, 0x67, 0x68, 0x26,
                ],
                output_str: "da0b3452b06fe341626ad0949c183fbda5676826",
            },
        ];

        for test in tests {
            // Hash through high-level API, check hex encoding/decoding
            let hash = hash160::Hash::hash(&test.input[..]);
            assert_eq!(hash, hash160::Hash::from_hex(test.output_str).expect("parse hex"));
            assert_eq!(&hash[..], &test.output[..]);
            assert_eq!(&hash.to_hex(), &test.output_str);

            // Hash through engine, checking that we can input byte by byte
            let mut engine = hash160::Hash::engine();
            for ch in test.input {
                engine.input(&[ch]);
            }
            let manual_hash = Hash::from_engine(engine);
            assert_eq!(hash, manual_hash);
            assert_eq!(hash.into_inner()[..].as_ref(), test.output.as_slice());
        }
    }

    #[cfg(feature="serde")]
    #[test]
    fn ripemd_serde() {
        use serde_test::{Configure, Token, assert_tokens};

        static HASH_BYTES: [u8; 20] = [
            0x13, 0x20, 0x72, 0xdf,
            0x69, 0x09, 0x33, 0x83,
            0x5e, 0xb8, 0xb6, 0xad,
            0x0b, 0x77, 0xe7, 0xb6,
            0xf1, 0x4a, 0xca, 0xd7,
        ];

        let hash = hash160::Hash::from_slice(&HASH_BYTES).expect("right number of bytes");
        assert_tokens(&hash.compact(), &[Token::BorrowedBytes(&HASH_BYTES[..])]);
        assert_tokens(&hash.readable(), &[Token::Str("132072df690933835eb8b6ad0b77e7b6f14acad7")]);
    }
}

#[cfg(all(test, feature="unstable"))]
mod benches {
    use test::Bencher;

    use hash160;
    use Hash;
    use HashEngine;

    #[bench]
    pub fn hash160_10(bh: & mut Bencher) {
        let mut engine = hash160::Hash::engine();
        let bytes = [1u8; 10];
        bh.iter( || {
            engine.input(&bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

    #[bench]
    pub fn hash160_1k(bh: & mut Bencher) {
        let mut engine = hash160::Hash::engine();
        let bytes = [1u8; 1024];
        bh.iter( || {
            engine.input(&bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

    #[bench]
    pub fn hash160_64k(bh: & mut Bencher) {
        let mut engine = hash160::Hash::engine();
        let bytes = [1u8; 65536];
        bh.iter( || {
            engine.input(&bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

}
