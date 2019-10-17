// Bitcoin Hashes Library
// Written in 2019 by
//   The rust-bitcoin developers
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

// This module is largely copied from the rust-siphash sip.rs file;
// while rust-siphash is licensed under Apache, that file specifically
// was written entirely by Steven Roose, who is re-licensing its
// contents here as CC0.

//! # SipHash 2-4

use core::{cmp, mem, ptr, str};

use Error;
use Hash as HashTrait;
use HashEngine as EngineTrait;
use util;

macro_rules! compress {
    ($state:expr) => {{
        compress!($state.v0, $state.v1, $state.v2, $state.v3)
    }};
    ($v0:expr, $v1:expr, $v2:expr, $v3:expr) => {{
        $v0 = $v0.wrapping_add($v1);
        $v1 = $v1.rotate_left(13);
        $v1 ^= $v0;
        $v0 = $v0.rotate_left(32);
        $v2 = $v2.wrapping_add($v3);
        $v3 = $v3.rotate_left(16);
        $v3 ^= $v2;
        $v0 = $v0.wrapping_add($v3);
        $v3 = $v3.rotate_left(21);
        $v3 ^= $v0;
        $v2 = $v2.wrapping_add($v1);
        $v1 = $v1.rotate_left(17);
        $v1 ^= $v2;
        $v2 = $v2.rotate_left(32);
    }};
}

/// Load an integer of the desired type from a byte stream, in LE order. Uses
/// `copy_nonoverlapping` to let the compiler generate the most efficient way
/// to load it from a possibly unaligned address.
///
/// Unsafe because: unchecked indexing at `i..i+size_of(int_ty)`
macro_rules! load_int_le {
    ($buf:expr, $i:expr, $int_ty:ident) => {{
        debug_assert!($i + mem::size_of::<$int_ty>() <= $buf.len());
        let mut data = 0 as $int_ty;
        ptr::copy_nonoverlapping(
            $buf.get_unchecked($i),
            &mut data as *mut _ as *mut u8,
            mem::size_of::<$int_ty>(),
        );
        data.to_le()
    }};
}

/// Internal state of the [HashEngine].
#[derive(Debug, Clone)]
pub struct State {
    // v0, v2 and v1, v3 show up in pairs in the algorithm,
    // and simd implementations of SipHash will use vectors
    // of v02 and v13. By placing them in this order in the struct,
    // the compiler can pick up on just a few simd optimizations by itself.
    v0: u64,
    v2: u64,
    v1: u64,
    v3: u64,
}

/// Engine to compute SipHash24 hash function.
#[derive(Debug, Clone)]
pub struct HashEngine {
    k0: u64,
    k1: u64,
    length: usize, // how many bytes we've processed
    state: State,  // hash State
    tail: u64,     // unprocessed bytes le
    ntail: usize,  // how many bytes in tail are valid
}

impl HashEngine {
    /// Create a new SipHash24 engine with keys.
    pub fn with_keys(k0: u64, k1: u64) -> HashEngine {
        HashEngine {
            k0: k0,
            k1: k1,
            length: 0,
            state: State {
                v0: k0 ^ 0x736f6d6570736575,
                v1: k1 ^ 0x646f72616e646f6d,
                v2: k0 ^ 0x6c7967656e657261,
                v3: k1 ^ 0x7465646279746573,
            },
            tail: 0,
            ntail: 0,
        }
    }

    /// Create a new SipHash24 engine.
    pub fn new() -> HashEngine {
        HashEngine::with_keys(0, 0)
    }

    /// Retrieve the keys of this engine.
    pub fn keys(&self) -> (u64, u64) {
        (self.k0, self.k1)
    }

    #[inline]
    fn c_rounds(state: &mut State) {
        compress!(state);
        compress!(state);
    }

    #[inline]
    fn d_rounds(state: &mut State) {
        compress!(state);
        compress!(state);
        compress!(state);
        compress!(state);
    }
}

impl Default for HashEngine {
    fn default() -> Self {
        HashEngine::new()
    }
}

impl EngineTrait for HashEngine {
    type MidState = State;

    fn midstate(&self) -> State {
        self.state.clone()
    }

    const BLOCK_SIZE: usize = 8;

    #[inline]
    fn input(&mut self, msg: &[u8]) {
        let length = msg.len();
        self.length += length;

        let mut needed = 0;

        if self.ntail != 0 {
            needed = 8 - self.ntail;
            self.tail |= unsafe { u8to64_le(msg, 0, cmp::min(length, needed)) } << (8 * self.ntail);
            if length < needed {
                self.ntail += length;
                return;
            } else {
                self.state.v3 ^= self.tail;
                HashEngine::c_rounds(&mut self.state);
                self.state.v0 ^= self.tail;
                self.ntail = 0;
            }
        }

        // Buffered tail is now flushed, process new input.
        let len = length - needed;
        let left = len & 0x7;

        let mut i = needed;
        while i < len - left {
            let mi = unsafe { load_int_le!(msg, i, u64) };

            self.state.v3 ^= mi;
            HashEngine::c_rounds(&mut self.state);
            self.state.v0 ^= mi;

            i += 8;
        }

        self.tail = unsafe { u8to64_le(msg, i, left) };
        self.ntail = left;
    }
}

/// Output of the SipHash24 hash function.
#[derive(Copy, Clone, PartialEq, Eq, Default, PartialOrd, Ord, Hash)]
pub struct Hash([u8; 8]);

hex_fmt_impl!(Debug, Hash);
hex_fmt_impl!(Display, Hash);
hex_fmt_impl!(LowerHex, Hash);
index_impl!(Hash);
serde_impl!(Hash, 8);
borrow_slice_impl!(Hash);

impl str::FromStr for Hash {
    type Err = ::hex::Error;
    fn from_str(s: &str) -> Result<Self, Self::Err> {
        ::hex::FromHex::from_hex(s)
    }
}

impl Hash {
    /// Hash the given data with an engine with the provided keys.
    pub fn hash_with_keys(k0: u64, k1: u64, data: &[u8]) -> Hash {
        let mut engine = HashEngine::with_keys(k0, k1);
        engine.input(data);
        Hash::from_engine(engine)
    }

    /// Hash the given data directly to u64 with an engine with the provided keys.
    pub fn hash_to_u64_with_keys(k0: u64, k1: u64, data: &[u8]) -> u64 {
        let mut engine = HashEngine::with_keys(k0, k1);
        engine.input(data);
        Hash::from_engine_to_u64(engine)
    }

    /// Produce a hash as u64 from the current state of a given engine
    #[inline]
    pub fn from_engine_to_u64(e: HashEngine) -> u64 {
        let mut state = e.state;

        let b: u64 = ((e.length as u64 & 0xff) << 56) | e.tail;

        state.v3 ^= b;
        HashEngine::c_rounds(&mut state);
        state.v0 ^= b;

        state.v2 ^= 0xff;
        HashEngine::d_rounds(&mut state);

        state.v0 ^ state.v1 ^ state.v2 ^ state.v3
    }

    /// Returns the (little endian) 64-bit integer representation of the hash value.
    pub fn as_u64(&self) -> u64 {
        util::slice_to_u64_le(&self.0[..])
    }

    /// Create a hash from its (little endian) 64-bit integer representation.
    pub fn from_u64(hash: u64) -> Hash {
        Hash(util::u64_to_array_le(hash))
    }
}

impl HashTrait for Hash {
    type Engine = HashEngine;
    type Inner = [u8; 8];

    #[cfg(not(feature = "fuzztarget"))]
    fn from_engine(e: HashEngine) -> Hash {
        Hash::from_u64(Hash::from_engine_to_u64(e))
    }

    #[cfg(feature = "fuzztarget")]
    fn from_engine(e: HashEngine) -> Hash {
        let state = e.midstate();
        Hash::from_u64(state.v0 ^ state.v1 ^ state.v2 ^ state.v3)
    }

    const LEN: usize = 8;

    fn from_slice(sl: &[u8]) -> Result<Hash, Error> {
        if sl.len() != 8 {
            Err(Error::InvalidLength(Self::LEN, sl.len()))
        } else {
            let mut ret = [0; 8];
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

/// Load an u64 using up to 7 bytes of a byte slice.
///
/// Unsafe because: unchecked indexing at start..start+len
#[inline]
unsafe fn u8to64_le(buf: &[u8], start: usize, len: usize) -> u64 {
    debug_assert!(len < 8);
    let mut i = 0; // current byte index (from LSB) in the output u64
    let mut out = 0;
    if i + 3 < len {
        out = u64::from(load_int_le!(buf, start + i, u32));
        i += 4;
    }
    if i + 1 < len {
        out |= u64::from(load_int_le!(buf, start + i, u16)) << (i * 8);
        i += 2
    }
    if i < len {
        out |= u64::from(*buf.get_unchecked(start + i)) << (i * 8);
        i += 1;
    }
    debug_assert_eq!(i, len);
    out
}

#[cfg(test)]
mod tests {
    use super::*;
    use Hash as HashTrait;

    #[test]
    fn test_siphash_2_4() {
        let vecs: [[u8; 8]; 64] = [
            [0x31, 0x0e, 0x0e, 0xdd, 0x47, 0xdb, 0x6f, 0x72],
            [0xfd, 0x67, 0xdc, 0x93, 0xc5, 0x39, 0xf8, 0x74],
            [0x5a, 0x4f, 0xa9, 0xd9, 0x09, 0x80, 0x6c, 0x0d],
            [0x2d, 0x7e, 0xfb, 0xd7, 0x96, 0x66, 0x67, 0x85],
            [0xb7, 0x87, 0x71, 0x27, 0xe0, 0x94, 0x27, 0xcf],
            [0x8d, 0xa6, 0x99, 0xcd, 0x64, 0x55, 0x76, 0x18],
            [0xce, 0xe3, 0xfe, 0x58, 0x6e, 0x46, 0xc9, 0xcb],
            [0x37, 0xd1, 0x01, 0x8b, 0xf5, 0x00, 0x02, 0xab],
            [0x62, 0x24, 0x93, 0x9a, 0x79, 0xf5, 0xf5, 0x93],
            [0xb0, 0xe4, 0xa9, 0x0b, 0xdf, 0x82, 0x00, 0x9e],
            [0xf3, 0xb9, 0xdd, 0x94, 0xc5, 0xbb, 0x5d, 0x7a],
            [0xa7, 0xad, 0x6b, 0x22, 0x46, 0x2f, 0xb3, 0xf4],
            [0xfb, 0xe5, 0x0e, 0x86, 0xbc, 0x8f, 0x1e, 0x75],
            [0x90, 0x3d, 0x84, 0xc0, 0x27, 0x56, 0xea, 0x14],
            [0xee, 0xf2, 0x7a, 0x8e, 0x90, 0xca, 0x23, 0xf7],
            [0xe5, 0x45, 0xbe, 0x49, 0x61, 0xca, 0x29, 0xa1],
            [0xdb, 0x9b, 0xc2, 0x57, 0x7f, 0xcc, 0x2a, 0x3f],
            [0x94, 0x47, 0xbe, 0x2c, 0xf5, 0xe9, 0x9a, 0x69],
            [0x9c, 0xd3, 0x8d, 0x96, 0xf0, 0xb3, 0xc1, 0x4b],
            [0xbd, 0x61, 0x79, 0xa7, 0x1d, 0xc9, 0x6d, 0xbb],
            [0x98, 0xee, 0xa2, 0x1a, 0xf2, 0x5c, 0xd6, 0xbe],
            [0xc7, 0x67, 0x3b, 0x2e, 0xb0, 0xcb, 0xf2, 0xd0],
            [0x88, 0x3e, 0xa3, 0xe3, 0x95, 0x67, 0x53, 0x93],
            [0xc8, 0xce, 0x5c, 0xcd, 0x8c, 0x03, 0x0c, 0xa8],
            [0x94, 0xaf, 0x49, 0xf6, 0xc6, 0x50, 0xad, 0xb8],
            [0xea, 0xb8, 0x85, 0x8a, 0xde, 0x92, 0xe1, 0xbc],
            [0xf3, 0x15, 0xbb, 0x5b, 0xb8, 0x35, 0xd8, 0x17],
            [0xad, 0xcf, 0x6b, 0x07, 0x63, 0x61, 0x2e, 0x2f],
            [0xa5, 0xc9, 0x1d, 0xa7, 0xac, 0xaa, 0x4d, 0xde],
            [0x71, 0x65, 0x95, 0x87, 0x66, 0x50, 0xa2, 0xa6],
            [0x28, 0xef, 0x49, 0x5c, 0x53, 0xa3, 0x87, 0xad],
            [0x42, 0xc3, 0x41, 0xd8, 0xfa, 0x92, 0xd8, 0x32],
            [0xce, 0x7c, 0xf2, 0x72, 0x2f, 0x51, 0x27, 0x71],
            [0xe3, 0x78, 0x59, 0xf9, 0x46, 0x23, 0xf3, 0xa7],
            [0x38, 0x12, 0x05, 0xbb, 0x1a, 0xb0, 0xe0, 0x12],
            [0xae, 0x97, 0xa1, 0x0f, 0xd4, 0x34, 0xe0, 0x15],
            [0xb4, 0xa3, 0x15, 0x08, 0xbe, 0xff, 0x4d, 0x31],
            [0x81, 0x39, 0x62, 0x29, 0xf0, 0x90, 0x79, 0x02],
            [0x4d, 0x0c, 0xf4, 0x9e, 0xe5, 0xd4, 0xdc, 0xca],
            [0x5c, 0x73, 0x33, 0x6a, 0x76, 0xd8, 0xbf, 0x9a],
            [0xd0, 0xa7, 0x04, 0x53, 0x6b, 0xa9, 0x3e, 0x0e],
            [0x92, 0x59, 0x58, 0xfc, 0xd6, 0x42, 0x0c, 0xad],
            [0xa9, 0x15, 0xc2, 0x9b, 0xc8, 0x06, 0x73, 0x18],
            [0x95, 0x2b, 0x79, 0xf3, 0xbc, 0x0a, 0xa6, 0xd4],
            [0xf2, 0x1d, 0xf2, 0xe4, 0x1d, 0x45, 0x35, 0xf9],
            [0x87, 0x57, 0x75, 0x19, 0x04, 0x8f, 0x53, 0xa9],
            [0x10, 0xa5, 0x6c, 0xf5, 0xdf, 0xcd, 0x9a, 0xdb],
            [0xeb, 0x75, 0x09, 0x5c, 0xcd, 0x98, 0x6c, 0xd0],
            [0x51, 0xa9, 0xcb, 0x9e, 0xcb, 0xa3, 0x12, 0xe6],
            [0x96, 0xaf, 0xad, 0xfc, 0x2c, 0xe6, 0x66, 0xc7],
            [0x72, 0xfe, 0x52, 0x97, 0x5a, 0x43, 0x64, 0xee],
            [0x5a, 0x16, 0x45, 0xb2, 0x76, 0xd5, 0x92, 0xa1],
            [0xb2, 0x74, 0xcb, 0x8e, 0xbf, 0x87, 0x87, 0x0a],
            [0x6f, 0x9b, 0xb4, 0x20, 0x3d, 0xe7, 0xb3, 0x81],
            [0xea, 0xec, 0xb2, 0xa3, 0x0b, 0x22, 0xa8, 0x7f],
            [0x99, 0x24, 0xa4, 0x3c, 0xc1, 0x31, 0x57, 0x24],
            [0xbd, 0x83, 0x8d, 0x3a, 0xaf, 0xbf, 0x8d, 0xb7],
            [0x0b, 0x1a, 0x2a, 0x32, 0x65, 0xd5, 0x1a, 0xea],
            [0x13, 0x50, 0x79, 0xa3, 0x23, 0x1c, 0xe6, 0x60],
            [0x93, 0x2b, 0x28, 0x46, 0xe4, 0xd7, 0x06, 0x66],
            [0xe1, 0x91, 0x5f, 0x5c, 0xb1, 0xec, 0xa4, 0x6c],
            [0xf3, 0x25, 0x96, 0x5c, 0xa1, 0x6d, 0x62, 0x9f],
            [0x57, 0x5f, 0xf2, 0x8e, 0x60, 0x38, 0x1b, 0xe5],
            [0x72, 0x45, 0x06, 0xeb, 0x4c, 0x32, 0x8a, 0x95],
        ];

        let k0 = 0x_07_06_05_04_03_02_01_00;
        let k1 = 0x_0f_0e_0d_0c_0b_0a_09_08;
        let mut vin = [0u8; 64];
        let mut state_inc = HashEngine::with_keys(k0, k1);

        for i in 0..64 {
            vin[i] = i as u8;
            let vec = Hash::from_slice(&vecs[i][..]).unwrap();
            let out = Hash::hash_with_keys(k0, k1, &vin[0..i]);
            assert_eq!(vec, out, "vec #{}", i);

            let inc = Hash::from_engine(state_inc.clone());
            assert_eq!(vec, inc, "vec #{}", i);
            state_inc.input(&[i as u8]);
        }
    }
}

#[cfg(all(test, feature = "unstable"))]
mod benches {
    use test::Bencher;

    use siphash24;
    use Hash;
    use HashEngine;

    #[bench]
    pub fn siphash24_1ki(bh: &mut Bencher) {
        let mut engine = siphash24::Hash::engine();
        let bytes = [1u8; 1024];
        bh.iter(|| {
            engine.input(&bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

    #[bench]
    pub fn siphash24_64ki(bh: &mut Bencher) {
        let mut engine = siphash24::Hash::engine();
        let bytes = [1u8; 65536];
        bh.iter(|| {
            engine.input(&bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

    #[bench]
    pub fn siphash24_1ki_hash(bh: &mut Bencher) {
        let k0 = 0x_07_06_05_04_03_02_01_00;
        let k1 = 0x_0f_0e_0d_0c_0b_0a_09_08;
        let bytes = [1u8; 1024];
        bh.iter(|| {
            let _ = siphash24::Hash::hash_with_keys(k0, k1, &bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

    #[bench]
    pub fn siphash24_1ki_hash_u64(bh: &mut Bencher) {
        let k0 = 0x_07_06_05_04_03_02_01_00;
        let k1 = 0x_0f_0e_0d_0c_0b_0a_09_08;
        let bytes = [1u8; 1024];
        bh.iter(|| {
            let _ = siphash24::Hash::hash_to_u64_with_keys(k0, k1, &bytes);
        });
        bh.bytes = bytes.len() as u64;
    }
}
