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

//! # HMAC support

use core::{borrow, fmt, ops, str};
#[cfg(feature="serde")]
use serde::{Serialize, Serializer, Deserialize, Deserializer};

use HashEngine as EngineTrait;
use Hash as HashTrait;
use Error;

/// A hash computed from a RFC 2104 HMAC. Parameterized by the underlying hash function.
#[derive(Copy, Clone, PartialEq, Eq, Default, PartialOrd, Ord, Hash)]
pub struct Hmac<T: HashTrait>(T);

impl<T: HashTrait + str::FromStr> str::FromStr for Hmac<T> {
    type Err = <T as str::FromStr>::Err;
    fn from_str(s: &str) -> Result<Self, Self::Err> {
        Ok(Hmac(str::FromStr::from_str(s)?))
    }
}

/// Pair of underlying hash midstates which represent the current state
/// of an `HmacEngine`
pub struct HmacMidState<T: HashTrait> {
    /// Midstate of the inner hash engine
    pub inner: <T::Engine as EngineTrait>::MidState,
    /// Midstate of the outer hash engine
    pub outer: <T::Engine as EngineTrait>::MidState,
}

/// Pair of underyling hash engines, used for the inner and outer hash of HMAC
#[derive(Clone)]
pub struct HmacEngine<T: HashTrait> {
    iengine: T::Engine,
    oengine: T::Engine,
}

impl<T: HashTrait> Default for HmacEngine<T> {
    fn default() -> Self {
        HmacEngine::new(&[])
    }
}

impl<T: HashTrait> HmacEngine<T> {
    /// Construct a new keyed HMAC with the given key. We only support underlying hashes
    /// whose block sizes are ≤ 128 bytes; larger hashes will result in panics.
    pub fn new(key: &[u8]) -> HmacEngine<T> {
        debug_assert!(T::Engine::BLOCK_SIZE <= 128);

        let mut ipad = [0x36u8; 128];
        let mut opad = [0x5cu8; 128];
        let mut ret = HmacEngine {
            iengine: <T as HashTrait>::engine(),
            oengine: <T as HashTrait>::engine(),
        };

        if key.len() > T::Engine::BLOCK_SIZE {
            let hash = <T as HashTrait>::hash(key);
            for (b_i, b_h) in ipad.iter_mut().zip(&hash[..]) {
                *b_i ^= *b_h;
            }
            for (b_o, b_h) in opad.iter_mut().zip(&hash[..]) {
                *b_o ^= *b_h;
            }
        } else {
            for (b_i, b_h) in ipad.iter_mut().zip(&key[..]) {
                *b_i ^= *b_h;
            }
            for (b_o, b_h) in opad.iter_mut().zip(&key[..]) {
                *b_o ^= *b_h;
            }
        };

        EngineTrait::input(&mut ret.iengine, &ipad[..T::Engine::BLOCK_SIZE]);
        EngineTrait::input(&mut ret.oengine, &opad[..T::Engine::BLOCK_SIZE]);
        ret
    }
}

impl<T: HashTrait> EngineTrait for HmacEngine<T> {
    type MidState = HmacMidState<T>;

    fn midstate(&self) -> Self::MidState {
        HmacMidState {
            inner: self.iengine.midstate(),
            outer: self.oengine.midstate(),
        }
    }

    const BLOCK_SIZE: usize = T::Engine::BLOCK_SIZE;
    
    fn input(&mut self, buf: &[u8]) {
        self.iengine.input(buf)
    }
}

impl<T: HashTrait> fmt::Debug for Hmac<T> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        fmt::Debug::fmt(&self.0, f)
    }
}

impl<T: HashTrait> fmt::Display for Hmac<T> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        fmt::Display::fmt(&self.0, f)
    }
}

impl<T: HashTrait> fmt::LowerHex for Hmac<T> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        fmt::LowerHex::fmt(&self.0, f)
    }
}

impl<T: HashTrait> ops::Index<usize> for Hmac<T> {
    type Output = u8;
    fn index(&self, index: usize) -> &u8 {
        &self.0[index]
    }
}

impl<T: HashTrait> ops::Index<ops::Range<usize>> for Hmac<T> {
    type Output = [u8];
    fn index(&self, index: ops::Range<usize>) -> &[u8] {
        &self.0[index]
    }
}

impl<T: HashTrait> ops::Index<ops::RangeFrom<usize>> for Hmac<T> {
    type Output = [u8];
    fn index(&self, index: ops::RangeFrom<usize>) -> &[u8] {
        &self.0[index]
    }
}

impl<T: HashTrait> ops::Index<ops::RangeTo<usize>> for Hmac<T> {
    type Output = [u8];
    fn index(&self, index: ops::RangeTo<usize>) -> &[u8] {
        &self.0[index]
    }
}

impl<T: HashTrait> ops::Index<ops::RangeFull> for Hmac<T> {
    type Output = [u8];
    fn index(&self, index: ops::RangeFull) -> &[u8] {
        &self.0[index]
    }
}

impl<T: HashTrait> borrow::Borrow<[u8]> for Hmac<T> {
    fn borrow(&self) -> &[u8] {
        &self[..]
    }
}

impl<T: HashTrait> HashTrait for Hmac<T> {
    type Engine = HmacEngine<T>;
    type Inner = T::Inner;

    fn from_engine(mut e: HmacEngine<T>) -> Hmac<T> {
        let ihash = T::from_engine(e.iengine);
        e.oengine.input(&ihash[..]);
        let ohash = T::from_engine(e.oengine);
        Hmac(ohash)
    }

    const LEN: usize = T::LEN;

    fn from_slice(sl: &[u8]) -> Result<Hmac<T>, Error> {
        T::from_slice(sl).map(Hmac)
    }

    fn into_inner(self) -> Self::Inner {
        self.0.into_inner()
    }

    fn from_inner(inner: T::Inner) -> Self {
        Hmac(T::from_inner(inner))
    }
}

#[cfg(feature="serde")]
impl<T: HashTrait + Serialize> Serialize for Hmac<T> {
    fn serialize<S: Serializer>(&self, s: S) -> Result<S::Ok, S::Error> {
        Serialize::serialize(&self.0, s)
    }
}

#[cfg(feature="serde")]
impl<'de, T: HashTrait + Deserialize<'de>> Deserialize<'de> for Hmac<T> {
    fn deserialize<D: Deserializer<'de>>(d: D) -> Result<Hmac<T>, D::Error> {
        let inner = Deserialize::deserialize(d)?;
        Ok(Hmac(inner))
    }
}

#[cfg(test)]
mod tests {
    use sha256;
    #[cfg(feature="serde")] use sha512;
    use {Hash, HashEngine, Hmac, HmacEngine};

    #[derive(Clone)]
    struct Test {
        key: Vec<u8>,
        input: Vec<u8>,
        output: Vec<u8>,
    }

    #[test]
    fn test() {
        let tests = vec![
            // Test vectors copied from libsecp256k1
            // Sadly the RFC2104 test vectors all use MD5 as their underlying hash function,
            // which of course this library does not support.
            Test {
                key: vec![ 0x0b; 20],
                input: vec![0x48, 0x69, 0x20, 0x54, 0x68, 0x65, 0x72, 0x65],
                output: vec![
                    0xb0, 0x34, 0x4c, 0x61, 0xd8, 0xdb, 0x38, 0x53,
                    0x5c, 0xa8, 0xaf, 0xce, 0xaf, 0x0b, 0xf1, 0x2b,
                    0x88, 0x1d, 0xc2, 0x00, 0xc9, 0x83, 0x3d, 0xa7,
                    0x26, 0xe9, 0x37, 0x6c, 0x2e, 0x32, 0xcf, 0xf7,
                ],
            },
            Test {
                key: vec![ 0x4a, 0x65, 0x66, 0x65 ],
                input: vec![
                    0x77, 0x68, 0x61, 0x74, 0x20, 0x64, 0x6f, 0x20,
                    0x79, 0x61, 0x20, 0x77, 0x61, 0x6e, 0x74, 0x20,
                    0x66, 0x6f, 0x72, 0x20, 0x6e, 0x6f, 0x74, 0x68,
                    0x69, 0x6e, 0x67, 0x3f,
                ],
                output: vec![
                    0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e,
                    0x6a, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xc7,
                    0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27, 0x39, 0x83,
                    0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec, 0x38, 0x43,
                ],
            },
            Test {
                key: vec![ 0xaa; 20 ],
                input: vec![ 0xdd; 50 ],
                output: vec![
                    0x77, 0x3e, 0xa9, 0x1e, 0x36, 0x80, 0x0e, 0x46,
                    0x85, 0x4d, 0xb8, 0xeb, 0xd0, 0x91, 0x81, 0xa7,
                    0x29, 0x59, 0x09, 0x8b, 0x3e, 0xf8, 0xc1, 0x22,
                    0xd9, 0x63, 0x55, 0x14, 0xce, 0xd5, 0x65, 0xfe,
                ],
            },
            Test {
                key: vec![
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
                    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                    0x19
                ],
                input: vec![ 0xcd; 50 ],
                output: vec![
                    0x82, 0x55, 0x8a, 0x38, 0x9a, 0x44, 0x3c, 0x0e,
                    0xa4, 0xcc, 0x81, 0x98, 0x99, 0xf2, 0x08, 0x3a,
                    0x85, 0xf0, 0xfa, 0xa3, 0xe5, 0x78, 0xf8, 0x07,
                    0x7a, 0x2e, 0x3f, 0xf4, 0x67, 0x29, 0x66, 0x5b,
                ],
            },
            Test {
                key: vec! [ 0xaa; 131 ],
                input: vec![
                    0x54, 0x65, 0x73, 0x74, 0x20, 0x55, 0x73, 0x69,
                    0x6e, 0x67, 0x20, 0x4c, 0x61, 0x72, 0x67, 0x65,
                    0x72, 0x20, 0x54, 0x68, 0x61, 0x6e, 0x20, 0x42,
                    0x6c, 0x6f, 0x63, 0x6b, 0x2d, 0x53, 0x69, 0x7a,
                    0x65, 0x20, 0x4b, 0x65, 0x79, 0x20, 0x2d, 0x20,
                    0x48, 0x61, 0x73, 0x68, 0x20, 0x4b, 0x65, 0x79,
                    0x20, 0x46, 0x69, 0x72, 0x73, 0x74,
                ],
                output: vec![
                    0x60, 0xe4, 0x31, 0x59, 0x1e, 0xe0, 0xb6, 0x7f,
                    0x0d, 0x8a, 0x26, 0xaa, 0xcb, 0xf5, 0xb7, 0x7f,
                    0x8e, 0x0b, 0xc6, 0x21, 0x37, 0x28, 0xc5, 0x14,
                    0x05, 0x46, 0x04, 0x0f, 0x0e, 0xe3, 0x7f, 0x54,
                ],
            },
            Test {
                key: vec! [ 0xaa; 131 ],
                input: vec![
                    0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20,
                    0x61, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x75,
                    0x73, 0x69, 0x6e, 0x67, 0x20, 0x61, 0x20, 0x6c,
                    0x61, 0x72, 0x67, 0x65, 0x72, 0x20, 0x74, 0x68,
                    0x61, 0x6e, 0x20, 0x62, 0x6c, 0x6f, 0x63, 0x6b,
                    0x2d, 0x73, 0x69, 0x7a, 0x65, 0x20, 0x6b, 0x65,
                    0x79, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x61, 0x20,
                    0x6c, 0x61, 0x72, 0x67, 0x65, 0x72, 0x20, 0x74,
                    0x68, 0x61, 0x6e, 0x20, 0x62, 0x6c, 0x6f, 0x63,
                    0x6b, 0x2d, 0x73, 0x69, 0x7a, 0x65, 0x20, 0x64,
                    0x61, 0x74, 0x61, 0x2e, 0x20, 0x54, 0x68, 0x65,
                    0x20, 0x6b, 0x65, 0x79, 0x20, 0x6e, 0x65, 0x65,
                    0x64, 0x73, 0x20, 0x74, 0x6f, 0x20, 0x62, 0x65,
                    0x20, 0x68, 0x61, 0x73, 0x68, 0x65, 0x64, 0x20,
                    0x62, 0x65, 0x66, 0x6f, 0x72, 0x65, 0x20, 0x62,
                    0x65, 0x69, 0x6e, 0x67, 0x20, 0x75, 0x73, 0x65,
                    0x64, 0x20, 0x62, 0x79, 0x20, 0x74, 0x68, 0x65,
                    0x20, 0x48, 0x4d, 0x41, 0x43, 0x20, 0x61, 0x6c,
                    0x67, 0x6f, 0x72, 0x69, 0x74, 0x68, 0x6d, 0x2e,
                ],
                output: vec![
                    0x9b, 0x09, 0xff, 0xa7, 0x1b, 0x94, 0x2f, 0xcb,
                    0x27, 0x63, 0x5f, 0xbc, 0xd5, 0xb0, 0xe9, 0x44,
                    0xbf, 0xdc, 0x63, 0x64, 0x4f, 0x07, 0x13, 0x93,
                    0x8a, 0x7f, 0x51, 0x53, 0x5c, 0x3a, 0x35, 0xe2,
                ],
            },
        ];

        for test in tests {
            let mut engine = HmacEngine::<sha256::Hash>::new(&test.key);
            engine.input(&test.input);
            let hash = Hmac::<sha256::Hash>::from_engine(engine);
            assert_eq!(&hash[..], &test.output[..]);
            assert_eq!(hash.into_inner()[..].as_ref(), test.output.as_slice());
        }
    }

    #[cfg(feature="serde")]
    #[test]
    fn hmac_sha512_serde() {
        use serde_test::{Configure, Token, assert_tokens};

        static HASH_BYTES: [u8; 64] = [
            0x8b, 0x41, 0xe1, 0xb7, 0x8a, 0xd1, 0x15, 0x21,
            0x11, 0x3c, 0x52, 0xff, 0x18, 0x2a, 0x1b, 0x8e,
            0x0a, 0x19, 0x57, 0x54, 0xaa, 0x52, 0x7f, 0xcd,
            0x00, 0xa4, 0x11, 0x62, 0x0b, 0x46, 0xf2, 0x0f,
            0xff, 0xfb, 0x80, 0x88, 0xcc, 0xf8, 0x54, 0x97,
            0x12, 0x1a, 0xd4, 0x49, 0x9e, 0x08, 0x45, 0xb8,
            0x76, 0xf6, 0xdd, 0x66, 0x40, 0x08, 0x8a, 0x2f,
            0x0b, 0x2d, 0x8a, 0x60, 0x0b, 0xdf, 0x4c, 0x0c,
        ];

        let hash = Hmac::<sha512::Hash>::from_slice(&HASH_BYTES).expect("right number of bytes");
        assert_tokens(&hash.compact(), &[Token::BorrowedBytes(&HASH_BYTES[..])]);
        assert_tokens(
            &hash.readable(),
            &[Token::Str(
                "8b41e1b78ad11521113c52ff182a1b8e0a195754aa527fcd00a411620b46f20f\
                 fffb8088ccf85497121ad4499e0845b876f6dd6640088a2f0b2d8a600bdf4c0c"
            )],
        );
    }
}

#[cfg(all(test, feature="unstable"))]
mod benches {
    use test::Bencher;

    use sha256;
    use {Hmac, Hash, HashEngine};

    #[bench]
    pub fn hmac_sha256_10(bh: & mut Bencher) {
        let mut engine = Hmac::<sha256::Hash>::engine();
        let bytes = [1u8; 10];
        bh.iter( || {
            engine.input(&bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

    #[bench]
    pub fn hmac_sha256_1k(bh: & mut Bencher) {
        let mut engine = Hmac::<sha256::Hash>::engine();
        let bytes = [1u8; 1024];
        bh.iter( || {
            engine.input(&bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

    #[bench]
    pub fn hmac_sha256_64k(bh: & mut Bencher) {
        let mut engine = Hmac::<sha256::Hash>::engine();
        let bytes = [1u8; 65536];
        bh.iter( || {
            engine.input(&bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

}
