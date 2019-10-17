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

//! # SHA256

use core::{cmp, str};

use hex;
use HashEngine as EngineTrait;
use Hash as HashTrait;
use Error;
use util;

const BLOCK_SIZE: usize = 64;

/// Engine to compute SHA256 hash function
#[derive(Clone)]
pub struct HashEngine {
    buffer: [u8; BLOCK_SIZE],
    h: [u32; 8],
    length: usize,
}

impl Default for HashEngine {
    fn default() -> Self {
        HashEngine {
            h: [0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19],
            length: 0,
            buffer: [0; BLOCK_SIZE],
        }
    }
}

impl EngineTrait for HashEngine {
    type MidState = Midstate;

    #[cfg(not(feature = "fuzztarget"))]
    fn midstate(&self) -> Midstate {
        let mut ret = [0; 32];
        for (val, ret_bytes) in self.h.iter().zip(ret.chunks_mut(4)) {
            ret_bytes.copy_from_slice(&util::u32_to_array_be(*val));
        }
        Midstate(ret)
    }

    #[cfg(feature = "fuzztarget")]
    fn midstate(&self) -> Midstate {
        let mut ret = [0; 32];
        ret.copy_from_slice(&self.buffer[..32]);
        Midstate(ret)
    }

    const BLOCK_SIZE: usize = 64;

    engine_input_impl!();
}

/// Output of the SHA256 hash function
#[derive(Copy, Clone, PartialEq, Eq, Default, PartialOrd, Ord, Hash)]
pub struct Hash([u8; 32]);

impl str::FromStr for Hash {
    type Err = ::hex::Error;
    fn from_str(s: &str) -> Result<Self, Self::Err> {
        ::hex::FromHex::from_hex(s)
    }
}

hex_fmt_impl!(Debug, Hash);
hex_fmt_impl!(Display, Hash);
hex_fmt_impl!(LowerHex, Hash);
index_impl!(Hash);
serde_impl!(Hash, 32);
borrow_slice_impl!(Hash);

impl HashTrait for Hash {
    type Engine = HashEngine;
    type Inner = [u8; 32];

    #[cfg(not(feature = "fuzztarget"))]
    fn from_engine(mut e: HashEngine) -> Hash {
        // pad buffer with a single 1-bit then all 0s, until there are exactly 8 bytes remaining
        let data_len = e.length as u64;

        let zeroes = [0; BLOCK_SIZE - 8];
        e.input(&[0x80]);
        if e.length % BLOCK_SIZE > zeroes.len() {
            e.input(&zeroes);
        }
        let pad_length = zeroes.len() - (e.length % BLOCK_SIZE);
        e.input(&zeroes[..pad_length]);
        debug_assert_eq!(e.length % BLOCK_SIZE, zeroes.len());

        e.input(&util::u64_to_array_be(8 * data_len));
        debug_assert_eq!(e.length % BLOCK_SIZE, 0);

        Hash(e.midstate().into_inner())
    }

    #[cfg(feature = "fuzztarget")]
    fn from_engine(e: HashEngine) -> Hash {
        Hash(e.midstate().into_inner())
    }

    const LEN: usize = 32;

    fn from_slice(sl: &[u8]) -> Result<Hash, Error> {
        if sl.len() != 32 {
            Err(Error::InvalidLength(Self::LEN, sl.len()))
        } else {
            let mut ret = [0; 32];
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

/// Output of the SHA256 hash function
#[derive(Copy, Clone, PartialEq, Eq, Default, PartialOrd, Ord, Hash)]
pub struct Midstate([u8; 32]);

hex_fmt_impl!(Debug, Midstate);
hex_fmt_impl!(Display, Midstate);
hex_fmt_impl!(LowerHex, Midstate);
index_impl!(Midstate);
serde_impl!(Midstate, 32);
borrow_slice_impl!(Midstate);

impl Midstate {
    /// Length of the midstate, in bytes.
    const LEN: usize = 32;

    /// Flag indicating whether user-visible serializations of this hash
    /// should be backward. For some reason Satoshi decided this should be
    /// true for `Sha256dHash`, so here we are.
    const DISPLAY_BACKWARD: bool = true;

    /// Construct a new midstate from the inner value.
    pub fn from_inner(inner: [u8; 32]) -> Self {
        Midstate(inner)
    }

    /// Copies a byte slice into the [Midstate] object.
    pub fn from_slice(sl: &[u8]) -> Result<Midstate, Error> {
        if sl.len() != Self::LEN {
            Err(Error::InvalidLength(Self::LEN, sl.len()))
        } else {
            let mut ret = [0; 32];
            ret.copy_from_slice(sl);
            Ok(Midstate(ret))
        }
    }

    /// Unwraps the [Midstate] and returns the underlying byte array.
    pub fn into_inner(self) -> [u8; 32] {
        self.0
    }
}

impl hex::FromHex for Midstate {
    fn from_byte_iter<I>(iter: I) -> Result<Self, hex::Error>
        where I: Iterator<Item=Result<u8, hex::Error>> +
            ExactSizeIterator +
            DoubleEndedIterator,
    {
        // DISPLAY_BACKWARD is true
        Ok(Midstate::from_inner(hex::FromHex::from_byte_iter(iter.rev())?))
    }
}

macro_rules! Ch( ($x:expr, $y:expr, $z:expr) => ($z ^ ($x & ($y ^ $z))) );
macro_rules! Maj( ($x:expr, $y:expr, $z:expr) => (($x & $y) | ($z & ($x | $y))) );
macro_rules! Sigma0( ($x:expr) => (circular_lshift32!(30, $x) ^ circular_lshift32!(19, $x) ^ circular_lshift32!(10, $x)) ); macro_rules! Sigma1( ($x:expr) => (circular_lshift32!(26, $x) ^ circular_lshift32!(21, $x) ^ circular_lshift32!(7, $x)) );
macro_rules! sigma0( ($x:expr) => (circular_lshift32!(25, $x) ^ circular_lshift32!(14, $x) ^ ($x >> 3)) );
macro_rules! sigma1( ($x:expr) => (circular_lshift32!(15, $x) ^ circular_lshift32!(13, $x) ^ ($x >> 10)) );

macro_rules! round(
    // first round
    ($a:expr, $b:expr, $c:expr, $d:expr, $e:expr, $f:expr, $g:expr, $h:expr, $k:expr, $w:expr) => (
        let t1 = $h.wrapping_add(Sigma1!($e)).wrapping_add(Ch!($e, $f, $g)).wrapping_add($k).wrapping_add($w);
        let t2 = Sigma0!($a).wrapping_add(Maj!($a, $b, $c));
        $d = $d.wrapping_add(t1);
        $h = t1.wrapping_add(t2);
    );
    // later rounds we reassign $w before doing the first-round computation
    ($a:expr, $b:expr, $c:expr, $d:expr, $e:expr, $f:expr, $g:expr, $h:expr, $k:expr, $w:expr, $w1:expr, $w2:expr, $w3:expr) => (
        $w = $w.wrapping_add(sigma1!($w1)).wrapping_add($w2).wrapping_add(sigma0!($w3));
        round!($a, $b, $c, $d, $e, $f, $g, $h, $k, $w);
    )
);

impl HashEngine {
    /// Create a new [HashEngine] from a midstate.
    ///
    /// Be aware that this method panics when [length] is
    /// not a multiple of the block size.
    pub fn from_midstate(midstate: Midstate, length: usize) -> HashEngine {
        assert!(length % BLOCK_SIZE == 0, "length is no multiple of the block size");

        let mut ret = [0; 8];
        for (ret_val, midstate_bytes) in ret.iter_mut().zip(midstate[..].chunks(4)) {
            *ret_val = util::slice_to_u32_be(midstate_bytes);
        }

        HashEngine {
            buffer: [0; BLOCK_SIZE],
            h: ret,
            length: length,
        }
    }

    // Algorithm copied from libsecp256k1
    fn process_block(&mut self) {
        debug_assert_eq!(self.buffer.len(), BLOCK_SIZE);

        let mut w = [0u32; 16];
        for (w_val, buff_bytes) in w.iter_mut().zip(self.buffer.chunks(4)) {
            *w_val = util::slice_to_u32_be(buff_bytes);
        }

        let mut a = self.h[0];
        let mut b = self.h[1];
        let mut c = self.h[2];
        let mut d = self.h[3];
        let mut e = self.h[4];
        let mut f = self.h[5];
        let mut g = self.h[6];
        let mut h = self.h[7];

        round!(a, b, c, d, e, f, g, h, 0x428a2f98, w[0]);
        round!(h, a, b, c, d, e, f, g, 0x71374491, w[1]);
        round!(g, h, a, b, c, d, e, f, 0xb5c0fbcf, w[2]);
        round!(f, g, h, a, b, c, d, e, 0xe9b5dba5, w[3]);
        round!(e, f, g, h, a, b, c, d, 0x3956c25b, w[4]);
        round!(d, e, f, g, h, a, b, c, 0x59f111f1, w[5]);
        round!(c, d, e, f, g, h, a, b, 0x923f82a4, w[6]);
        round!(b, c, d, e, f, g, h, a, 0xab1c5ed5, w[7]);
        round!(a, b, c, d, e, f, g, h, 0xd807aa98, w[8]);
        round!(h, a, b, c, d, e, f, g, 0x12835b01, w[9]);
        round!(g, h, a, b, c, d, e, f, 0x243185be, w[10]);
        round!(f, g, h, a, b, c, d, e, 0x550c7dc3, w[11]);
        round!(e, f, g, h, a, b, c, d, 0x72be5d74, w[12]);
        round!(d, e, f, g, h, a, b, c, 0x80deb1fe, w[13]);
        round!(c, d, e, f, g, h, a, b, 0x9bdc06a7, w[14]);
        round!(b, c, d, e, f, g, h, a, 0xc19bf174, w[15]);

        round!(a, b, c, d, e, f, g, h, 0xe49b69c1, w[0], w[14], w[9], w[1]);
        round!(h, a, b, c, d, e, f, g, 0xefbe4786, w[1], w[15], w[10], w[2]);
        round!(g, h, a, b, c, d, e, f, 0x0fc19dc6, w[2], w[0], w[11], w[3]);
        round!(f, g, h, a, b, c, d, e, 0x240ca1cc, w[3], w[1], w[12], w[4]);
        round!(e, f, g, h, a, b, c, d, 0x2de92c6f, w[4], w[2], w[13], w[5]);
        round!(d, e, f, g, h, a, b, c, 0x4a7484aa, w[5], w[3], w[14], w[6]);
        round!(c, d, e, f, g, h, a, b, 0x5cb0a9dc, w[6], w[4], w[15], w[7]);
        round!(b, c, d, e, f, g, h, a, 0x76f988da, w[7], w[5], w[0], w[8]);
        round!(a, b, c, d, e, f, g, h, 0x983e5152, w[8], w[6], w[1], w[9]);
        round!(h, a, b, c, d, e, f, g, 0xa831c66d, w[9], w[7], w[2], w[10]);
        round!(g, h, a, b, c, d, e, f, 0xb00327c8, w[10], w[8], w[3], w[11]);
        round!(f, g, h, a, b, c, d, e, 0xbf597fc7, w[11], w[9], w[4], w[12]);
        round!(e, f, g, h, a, b, c, d, 0xc6e00bf3, w[12], w[10], w[5], w[13]);
        round!(d, e, f, g, h, a, b, c, 0xd5a79147, w[13], w[11], w[6], w[14]);
        round!(c, d, e, f, g, h, a, b, 0x06ca6351, w[14], w[12], w[7], w[15]);
        round!(b, c, d, e, f, g, h, a, 0x14292967, w[15], w[13], w[8], w[0]);

        round!(a, b, c, d, e, f, g, h, 0x27b70a85, w[0], w[14], w[9], w[1]);
        round!(h, a, b, c, d, e, f, g, 0x2e1b2138, w[1], w[15], w[10], w[2]);
        round!(g, h, a, b, c, d, e, f, 0x4d2c6dfc, w[2], w[0], w[11], w[3]);
        round!(f, g, h, a, b, c, d, e, 0x53380d13, w[3], w[1], w[12], w[4]);
        round!(e, f, g, h, a, b, c, d, 0x650a7354, w[4], w[2], w[13], w[5]);
        round!(d, e, f, g, h, a, b, c, 0x766a0abb, w[5], w[3], w[14], w[6]);
        round!(c, d, e, f, g, h, a, b, 0x81c2c92e, w[6], w[4], w[15], w[7]);
        round!(b, c, d, e, f, g, h, a, 0x92722c85, w[7], w[5], w[0], w[8]);
        round!(a, b, c, d, e, f, g, h, 0xa2bfe8a1, w[8], w[6], w[1], w[9]);
        round!(h, a, b, c, d, e, f, g, 0xa81a664b, w[9], w[7], w[2], w[10]);
        round!(g, h, a, b, c, d, e, f, 0xc24b8b70, w[10], w[8], w[3], w[11]);
        round!(f, g, h, a, b, c, d, e, 0xc76c51a3, w[11], w[9], w[4], w[12]);
        round!(e, f, g, h, a, b, c, d, 0xd192e819, w[12], w[10], w[5], w[13]);
        round!(d, e, f, g, h, a, b, c, 0xd6990624, w[13], w[11], w[6], w[14]);
        round!(c, d, e, f, g, h, a, b, 0xf40e3585, w[14], w[12], w[7], w[15]);
        round!(b, c, d, e, f, g, h, a, 0x106aa070, w[15], w[13], w[8], w[0]);

        round!(a, b, c, d, e, f, g, h, 0x19a4c116, w[0], w[14], w[9], w[1]);
        round!(h, a, b, c, d, e, f, g, 0x1e376c08, w[1], w[15], w[10], w[2]);
        round!(g, h, a, b, c, d, e, f, 0x2748774c, w[2], w[0], w[11], w[3]);
        round!(f, g, h, a, b, c, d, e, 0x34b0bcb5, w[3], w[1], w[12], w[4]);
        round!(e, f, g, h, a, b, c, d, 0x391c0cb3, w[4], w[2], w[13], w[5]);
        round!(d, e, f, g, h, a, b, c, 0x4ed8aa4a, w[5], w[3], w[14], w[6]);
        round!(c, d, e, f, g, h, a, b, 0x5b9cca4f, w[6], w[4], w[15], w[7]);
        round!(b, c, d, e, f, g, h, a, 0x682e6ff3, w[7], w[5], w[0], w[8]);
        round!(a, b, c, d, e, f, g, h, 0x748f82ee, w[8], w[6], w[1], w[9]);
        round!(h, a, b, c, d, e, f, g, 0x78a5636f, w[9], w[7], w[2], w[10]);
        round!(g, h, a, b, c, d, e, f, 0x84c87814, w[10], w[8], w[3], w[11]);
        round!(f, g, h, a, b, c, d, e, 0x8cc70208, w[11], w[9], w[4], w[12]);
        round!(e, f, g, h, a, b, c, d, 0x90befffa, w[12], w[10], w[5], w[13]);
        round!(d, e, f, g, h, a, b, c, 0xa4506ceb, w[13], w[11], w[6], w[14]);
        round!(c, d, e, f, g, h, a, b, 0xbef9a3f7, w[14], w[12], w[7], w[15]);
        round!(b, c, d, e, f, g, h, a, 0xc67178f2, w[15], w[13], w[8], w[0]);

        self.h[0] = self.h[0].wrapping_add(a);
        self.h[1] = self.h[1].wrapping_add(b);
        self.h[2] = self.h[2].wrapping_add(c);
        self.h[3] = self.h[3].wrapping_add(d);
        self.h[4] = self.h[4].wrapping_add(e);
        self.h[5] = self.h[5].wrapping_add(f);
        self.h[6] = self.h[6].wrapping_add(g);
        self.h[7] = self.h[7].wrapping_add(h);
    }
}

#[cfg(test)]
mod tests {
    use sha256;
    use hex::{FromHex, ToHex};
    use {Hash, HashEngine};

    #[derive(Clone)]
    struct Test {
        input: &'static str,
        output: Vec<u8>,
        output_str: &'static str,
    }

    #[test]
    fn test() {
        let tests = vec![
            // Examples from wikipedia
            Test {
                input: "",
                output: vec![
                    0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
                    0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
                    0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
                    0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55,
                ],
                output_str: "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
            },
            Test {
                input: "The quick brown fox jumps over the lazy dog",
                output: vec![
                    0xd7, 0xa8, 0xfb, 0xb3, 0x07, 0xd7, 0x80, 0x94,
                    0x69, 0xca, 0x9a, 0xbc, 0xb0, 0x08, 0x2e, 0x4f,
                    0x8d, 0x56, 0x51, 0xe4, 0x6d, 0x3c, 0xdb, 0x76,
                    0x2d, 0x02, 0xd0, 0xbf, 0x37, 0xc9, 0xe5, 0x92,
                ],
                output_str: "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592",
            },
            Test {
                input: "The quick brown fox jumps over the lazy dog.",
                output: vec![
                    0xef, 0x53, 0x7f, 0x25, 0xc8, 0x95, 0xbf, 0xa7,
                    0x82, 0x52, 0x65, 0x29, 0xa9, 0xb6, 0x3d, 0x97,
                    0xaa, 0x63, 0x15, 0x64, 0xd5, 0xd7, 0x89, 0xc2,
                    0xb7, 0x65, 0x44, 0x8c, 0x86, 0x35, 0xfb, 0x6c,
                ],
                output_str: "ef537f25c895bfa782526529a9b63d97aa631564d5d789c2b765448c8635fb6c",
            },
        ];

        for test in tests {
            // Hash through high-level API, check hex encoding/decoding
            let hash = sha256::Hash::hash(&test.input.as_bytes());
            assert_eq!(hash, sha256::Hash::from_hex(test.output_str).expect("parse hex"));
            assert_eq!(&hash[..], &test.output[..]);
            assert_eq!(&hash.to_hex(), &test.output_str);

            // Hash through engine, checking that we can input byte by byte
            let mut engine = sha256::Hash::engine();
            for ch in test.input.as_bytes() {
                engine.input(&[*ch]);
            }
            let manual_hash = sha256::Hash::from_engine(engine);
            assert_eq!(hash, manual_hash);
            assert_eq!(hash.into_inner()[..].as_ref(), test.output.as_slice());
        }
    }

    #[test]
    fn midstate() {
        // Test vector obtained by doing an asset issuance on Elements
        let mut engine = sha256::Hash::engine();
        // sha256dhash of outpoint
        // 73828cbc65fd68ab78dc86992b76ae50ae2bf8ceedbe8de0483172f0886219f7:0
        engine.input(&[
            0x9d, 0xd0, 0x1b, 0x56, 0xb1, 0x56, 0x45, 0x14,
            0x3e, 0xad, 0x15, 0x8d, 0xec, 0x19, 0xf8, 0xce,
            0xa9, 0x0b, 0xd0, 0xa9, 0xb2, 0xf8, 0x1d, 0x21,
            0xff, 0xa3, 0xa4, 0xc6, 0x44, 0x81, 0xd4, 0x1c,
        ]);
        // 32 bytes of zeroes representing "new asset"
        engine.input(&[0; 32]);
        assert_eq!(
            engine.midstate(),
            // RPC output
            sha256::Midstate::from_inner([
                0x0b, 0xcf, 0xe0, 0xe5, 0x4e, 0x6c, 0xc7, 0xd3,
                0x4f, 0x4f, 0x7c, 0x1d, 0xf0, 0xb0, 0xf5, 0x03,
                0xf2, 0xf7, 0x12, 0x91, 0x2a, 0x06, 0x05, 0xb4,
                0x14, 0xed, 0x33, 0x7f, 0x7f, 0x03, 0x2e, 0x03,
            ])
        );
    }

    #[test]
    fn engine_with_state() {
        let mut engine = sha256::Hash::engine();
        let midstate_engine = sha256::HashEngine::from_midstate(engine.midstate(), 0);
        // Fresh engine and engine initialized with fresh state should have same state
        assert_eq!(engine.h, midstate_engine.h);

        // Midstate changes after writing 64 bytes
        engine.input(&[1; 63]);
        assert_eq!(engine.h, midstate_engine.h);
        engine.input(&[2; 1]);
        assert_ne!(engine.h, midstate_engine.h);

        // Initializing an engine with midstate from another engine should result in
        // both engines producing the same hashes
        let data_vec = vec![vec![3; 1], vec![4; 63], vec![5; 65], vec![6; 66]];
        for data in data_vec {
            let mut engine = engine.clone();
            let mut midstate_engine =
                sha256::HashEngine::from_midstate(engine.midstate(), engine.length);
            assert_eq!(engine.h, midstate_engine.h);
            assert_eq!(engine.length, midstate_engine.length);
            engine.input(&data);
            midstate_engine.input(&data);
            assert_eq!(engine.h, midstate_engine.h);
            let hash1 = sha256::Hash::from_engine(engine);
            let hash2 = sha256::Hash::from_engine(midstate_engine);
            assert_eq!(hash1, hash2);
        }

        // Test that a specific midstate results in a specific hash. Midstate was
        // obtained by applying sha256 to sha256("MuSig coefficient")||sha256("MuSig
        // coefficient").
        static MIDSTATE: [u8; 32] = [
            0x0f, 0xd0, 0x69, 0x0c, 0xfe, 0xfe, 0xae, 0x97, 0x99, 0x6e, 0xac, 0x7f, 0x5c, 0x30,
            0xd8, 0x64, 0x8c, 0x4a, 0x05, 0x73, 0xac, 0xa1, 0xa2, 0x2f, 0x6f, 0x43, 0xb8, 0x01,
            0x85, 0xce, 0x27, 0xcd,
        ];
        static HASH_EXPECTED: [u8; 32] = [
            0x18, 0x84, 0xe4, 0x72, 0x40, 0x4e, 0xf4, 0x5a, 0xb4, 0x9c, 0x4e, 0xa4, 0x9a, 0xe6,
            0x23, 0xa8, 0x88, 0x52, 0x7f, 0x7d, 0x8a, 0x06, 0x94, 0x20, 0x8f, 0xf1, 0xf7, 0xa9,
            0xd5, 0x69, 0x09, 0x59,
        ];
        let midstate_engine =
            sha256::HashEngine::from_midstate(sha256::Midstate::from_inner(MIDSTATE), 64);
        let hash = sha256::Hash::from_engine(midstate_engine);
        assert_eq!(hash, sha256::Hash(HASH_EXPECTED));
    }

    #[cfg(feature="serde")]
    #[test]
    fn sha256_serde() {
        use serde_test::{Configure, Token, assert_tokens};

        static HASH_BYTES: [u8; 32] = [
            0xef, 0x53, 0x7f, 0x25, 0xc8, 0x95, 0xbf, 0xa7,
            0x82, 0x52, 0x65, 0x29, 0xa9, 0xb6, 0x3d, 0x97,
            0xaa, 0x63, 0x15, 0x64, 0xd5, 0xd7, 0x89, 0xc2,
            0xb7, 0x65, 0x44, 0x8c, 0x86, 0x35, 0xfb, 0x6c,
        ];

        let hash = sha256::Hash::from_slice(&HASH_BYTES).expect("right number of bytes");
        assert_tokens(&hash.compact(), &[Token::BorrowedBytes(&HASH_BYTES[..])]);
        assert_tokens(&hash.readable(), &[Token::Str("ef537f25c895bfa782526529a9b63d97aa631564d5d789c2b765448c8635fb6c")]);
    }
}

#[cfg(all(test, feature="unstable"))]
mod benches {
    use test::Bencher;

    use sha256;
    use Hash;
    use HashEngine;

    #[bench]
    pub fn sha256_10(bh: & mut Bencher) {
        let mut engine = sha256::Hash::engine();
        let bytes = [1u8; 10];
        bh.iter( || {
            engine.input(&bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

    #[bench]
    pub fn sha256_1k(bh: & mut Bencher) {
        let mut engine = sha256::Hash::engine();
        let bytes = [1u8; 1024];
        bh.iter( || {
            engine.input(&bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

    #[bench]
    pub fn sha256_64k(bh: & mut Bencher) {
        let mut engine = sha256::Hash::engine();
        let bytes = [1u8; 65536];
        bh.iter( || {
            engine.input(&bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

}
