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

//! # SHA512

use core::{cmp, hash, str};

use HashEngine as EngineTrait;
use Hash as HashTrait;
use Error;
use util;

const BLOCK_SIZE: usize = 128;

/// Engine to compute SHA512 hash function
#[derive(Clone)]
pub struct HashEngine {
    h: [u64; 8],
    length: usize,
    buffer: [u8; BLOCK_SIZE],
}

impl Default for HashEngine {
    fn default() -> Self {
        HashEngine {
            h: [
                0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
                0x510e527fade682d1, 0x9b05688c2b3e6c1f, 0x1f83d9abfb41bd6b, 0x5be0cd19137e2179,
            ],
            length: 0,
            buffer: [0; BLOCK_SIZE],
        }
    }
}

impl EngineTrait for HashEngine {
    type MidState = [u8; 64];

    #[cfg(not(feature = "fuzztarget"))]
    fn midstate(&self) -> [u8; 64] {
        let mut ret = [0; 64];
        for (val, ret_bytes) in self.h.iter().zip(ret.chunks_mut(8)) {
            ret_bytes.copy_from_slice(&util::u64_to_array_be(*val));
        }
        ret
    }

    #[cfg(feature = "fuzztarget")]
    fn midstate(&self) -> [u8; 64] {
        let mut ret = [0; 64];
        ret.copy_from_slice(&self.buffer[..64]);
        ret
    }

    const BLOCK_SIZE: usize = 128;

    engine_input_impl!();
}

/// Output of the SHA256 hash function
pub struct Hash([u8; 64]);

impl Copy for Hash {}

impl Clone for Hash {
    fn clone(&self) -> Hash {
        let mut ret = [0; 64];
        ret.copy_from_slice(&self.0);
        Hash(ret)
    }
}

impl PartialEq for Hash {
    fn eq(&self, other: &Hash) -> bool {
        self.0[..] == other.0[..]
    }
}

impl Eq for Hash {}

impl Default for Hash {
    fn default() -> Hash {
        Hash([0; 64])
    }
}

impl PartialOrd for Hash {
    fn partial_cmp(&self, other: &Hash) -> Option<cmp::Ordering> {
        (&self.0).partial_cmp(&other.0)
    }
}

impl Ord for Hash {
    fn cmp(&self, other: &Hash) -> cmp::Ordering {
        (&self.0).cmp(&other.0)
    }
}

impl hash::Hash for Hash {
    fn hash<H: hash::Hasher>(&self, state: &mut H) {
        (&self.0).hash(state)
    }
}

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
serde_impl!(Hash, 64);
borrow_slice_impl!(Hash);

impl HashTrait for Hash {
    type Engine = HashEngine;
    type Inner = [u8; 64];

    #[cfg(not(feature = "fuzztarget"))]
    fn from_engine(mut e: HashEngine) -> Hash {
        // pad buffer with a single 1-bit then all 0s, until there are exactly 16 bytes remaining
        let data_len = e.length as u64;

        let zeroes = [0; BLOCK_SIZE - 16];
        e.input(&[0x80]);
        if e.length % BLOCK_SIZE > zeroes.len() {
            e.input(&zeroes);
        }
        let pad_length = zeroes.len() - (e.length % BLOCK_SIZE);
        e.input(&zeroes[..pad_length]);
        debug_assert_eq!(e.length % BLOCK_SIZE, zeroes.len());

        e.input(&[0; 8]);
        e.input(&util::u64_to_array_be(8 * data_len));
        debug_assert_eq!(e.length % BLOCK_SIZE, 0);

        Hash(e.midstate())
    }

    #[cfg(feature = "fuzztarget")]
    fn from_engine(e: HashEngine) -> Hash {
        let mut hash = e.midstate();
        hash[0] ^= 0xff; // Make this distinct from SHA-256
        Hash(hash)
    }

    const LEN: usize = 64;

    fn from_slice(sl: &[u8]) -> Result<Hash, Error> {
        if sl.len() != 64 {
            Err(Error::InvalidLength(Self::LEN, sl.len()))
        } else {
            let mut ret = [0; 64];
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

macro_rules! Ch( ($x:expr, $y:expr, $z:expr) => ($z ^ ($x & ($y ^ $z))) );
macro_rules! Maj( ($x:expr, $y:expr, $z:expr) => (($x & $y) | ($z & ($x | $y))) );
macro_rules! Sigma0( ($x:expr) => (circular_lshift64!(36, $x) ^ circular_lshift64!(30, $x) ^ circular_lshift64!(25, $x)) );
macro_rules! Sigma1( ($x:expr) => (circular_lshift64!(50, $x) ^ circular_lshift64!(46, $x) ^ circular_lshift64!(23, $x)) );
macro_rules! sigma0( ($x:expr) => (circular_lshift64!(63, $x) ^ circular_lshift64!(56, $x) ^ ($x >> 7)) );
macro_rules! sigma1( ($x:expr) => (circular_lshift64!(45, $x) ^ circular_lshift64!(3, $x) ^ ($x >> 6)) );

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
    // Algorithm copied from libsecp256k1
    fn process_block(&mut self) {
        debug_assert_eq!(self.buffer.len(), BLOCK_SIZE);

        let mut w = [0u64; 16];
        for (w_val, buff_bytes) in w.iter_mut().zip(self.buffer.chunks(8)) {
            *w_val = util::slice_to_u64_be(buff_bytes);
        }

        let mut a = self.h[0];
        let mut b = self.h[1];
        let mut c = self.h[2];
        let mut d = self.h[3];
        let mut e = self.h[4];
        let mut f = self.h[5];
        let mut g = self.h[6];
        let mut h = self.h[7];

        round!(a, b, c, d, e, f, g, h, 0x428a2f98d728ae22, w[0]);
        round!(h, a, b, c, d, e, f, g, 0x7137449123ef65cd, w[1]);
        round!(g, h, a, b, c, d, e, f, 0xb5c0fbcfec4d3b2f, w[2]);
        round!(f, g, h, a, b, c, d, e, 0xe9b5dba58189dbbc, w[3]);
        round!(e, f, g, h, a, b, c, d, 0x3956c25bf348b538, w[4]);
        round!(d, e, f, g, h, a, b, c, 0x59f111f1b605d019, w[5]);
        round!(c, d, e, f, g, h, a, b, 0x923f82a4af194f9b, w[6]);
        round!(b, c, d, e, f, g, h, a, 0xab1c5ed5da6d8118, w[7]);
        round!(a, b, c, d, e, f, g, h, 0xd807aa98a3030242, w[8]);
        round!(h, a, b, c, d, e, f, g, 0x12835b0145706fbe, w[9]);
        round!(g, h, a, b, c, d, e, f, 0x243185be4ee4b28c, w[10]);
        round!(f, g, h, a, b, c, d, e, 0x550c7dc3d5ffb4e2, w[11]);
        round!(e, f, g, h, a, b, c, d, 0x72be5d74f27b896f, w[12]);
        round!(d, e, f, g, h, a, b, c, 0x80deb1fe3b1696b1, w[13]);
        round!(c, d, e, f, g, h, a, b, 0x9bdc06a725c71235, w[14]);
        round!(b, c, d, e, f, g, h, a, 0xc19bf174cf692694, w[15]);

        round!(a, b, c, d, e, f, g, h, 0xe49b69c19ef14ad2, w[0], w[14], w[9], w[1]);
        round!(h, a, b, c, d, e, f, g, 0xefbe4786384f25e3, w[1], w[15], w[10], w[2]);
        round!(g, h, a, b, c, d, e, f, 0x0fc19dc68b8cd5b5, w[2], w[0], w[11], w[3]);
        round!(f, g, h, a, b, c, d, e, 0x240ca1cc77ac9c65, w[3], w[1], w[12], w[4]);
        round!(e, f, g, h, a, b, c, d, 0x2de92c6f592b0275, w[4], w[2], w[13], w[5]);
        round!(d, e, f, g, h, a, b, c, 0x4a7484aa6ea6e483, w[5], w[3], w[14], w[6]);
        round!(c, d, e, f, g, h, a, b, 0x5cb0a9dcbd41fbd4, w[6], w[4], w[15], w[7]);
        round!(b, c, d, e, f, g, h, a, 0x76f988da831153b5, w[7], w[5], w[0], w[8]);
        round!(a, b, c, d, e, f, g, h, 0x983e5152ee66dfab, w[8], w[6], w[1], w[9]);
        round!(h, a, b, c, d, e, f, g, 0xa831c66d2db43210, w[9], w[7], w[2], w[10]);
        round!(g, h, a, b, c, d, e, f, 0xb00327c898fb213f, w[10], w[8], w[3], w[11]);
        round!(f, g, h, a, b, c, d, e, 0xbf597fc7beef0ee4, w[11], w[9], w[4], w[12]);
        round!(e, f, g, h, a, b, c, d, 0xc6e00bf33da88fc2, w[12], w[10], w[5], w[13]);
        round!(d, e, f, g, h, a, b, c, 0xd5a79147930aa725, w[13], w[11], w[6], w[14]);
        round!(c, d, e, f, g, h, a, b, 0x06ca6351e003826f, w[14], w[12], w[7], w[15]);
        round!(b, c, d, e, f, g, h, a, 0x142929670a0e6e70, w[15], w[13], w[8], w[0]);

        round!(a, b, c, d, e, f, g, h, 0x27b70a8546d22ffc, w[0], w[14], w[9], w[1]);
        round!(h, a, b, c, d, e, f, g, 0x2e1b21385c26c926, w[1], w[15], w[10], w[2]);
        round!(g, h, a, b, c, d, e, f, 0x4d2c6dfc5ac42aed, w[2], w[0], w[11], w[3]);
        round!(f, g, h, a, b, c, d, e, 0x53380d139d95b3df, w[3], w[1], w[12], w[4]);
        round!(e, f, g, h, a, b, c, d, 0x650a73548baf63de, w[4], w[2], w[13], w[5]);
        round!(d, e, f, g, h, a, b, c, 0x766a0abb3c77b2a8, w[5], w[3], w[14], w[6]);
        round!(c, d, e, f, g, h, a, b, 0x81c2c92e47edaee6, w[6], w[4], w[15], w[7]);
        round!(b, c, d, e, f, g, h, a, 0x92722c851482353b, w[7], w[5], w[0], w[8]);
        round!(a, b, c, d, e, f, g, h, 0xa2bfe8a14cf10364, w[8], w[6], w[1], w[9]);
        round!(h, a, b, c, d, e, f, g, 0xa81a664bbc423001, w[9], w[7], w[2], w[10]);
        round!(g, h, a, b, c, d, e, f, 0xc24b8b70d0f89791, w[10], w[8], w[3], w[11]);
        round!(f, g, h, a, b, c, d, e, 0xc76c51a30654be30, w[11], w[9], w[4], w[12]);
        round!(e, f, g, h, a, b, c, d, 0xd192e819d6ef5218, w[12], w[10], w[5], w[13]);
        round!(d, e, f, g, h, a, b, c, 0xd69906245565a910, w[13], w[11], w[6], w[14]);
        round!(c, d, e, f, g, h, a, b, 0xf40e35855771202a, w[14], w[12], w[7], w[15]);
        round!(b, c, d, e, f, g, h, a, 0x106aa07032bbd1b8, w[15], w[13], w[8], w[0]);

        round!(a, b, c, d, e, f, g, h, 0x19a4c116b8d2d0c8, w[0], w[14], w[9], w[1]);
        round!(h, a, b, c, d, e, f, g, 0x1e376c085141ab53, w[1], w[15], w[10], w[2]);
        round!(g, h, a, b, c, d, e, f, 0x2748774cdf8eeb99, w[2], w[0], w[11], w[3]);
        round!(f, g, h, a, b, c, d, e, 0x34b0bcb5e19b48a8, w[3], w[1], w[12], w[4]);
        round!(e, f, g, h, a, b, c, d, 0x391c0cb3c5c95a63, w[4], w[2], w[13], w[5]);
        round!(d, e, f, g, h, a, b, c, 0x4ed8aa4ae3418acb, w[5], w[3], w[14], w[6]);
        round!(c, d, e, f, g, h, a, b, 0x5b9cca4f7763e373, w[6], w[4], w[15], w[7]);
        round!(b, c, d, e, f, g, h, a, 0x682e6ff3d6b2b8a3, w[7], w[5], w[0], w[8]);
        round!(a, b, c, d, e, f, g, h, 0x748f82ee5defb2fc, w[8], w[6], w[1], w[9]);
        round!(h, a, b, c, d, e, f, g, 0x78a5636f43172f60, w[9], w[7], w[2], w[10]);
        round!(g, h, a, b, c, d, e, f, 0x84c87814a1f0ab72, w[10], w[8], w[3], w[11]);
        round!(f, g, h, a, b, c, d, e, 0x8cc702081a6439ec, w[11], w[9], w[4], w[12]);
        round!(e, f, g, h, a, b, c, d, 0x90befffa23631e28, w[12], w[10], w[5], w[13]);
        round!(d, e, f, g, h, a, b, c, 0xa4506cebde82bde9, w[13], w[11], w[6], w[14]);
        round!(c, d, e, f, g, h, a, b, 0xbef9a3f7b2c67915, w[14], w[12], w[7], w[15]);
        round!(b, c, d, e, f, g, h, a, 0xc67178f2e372532b, w[15], w[13], w[8], w[0]);

        round!(a, b, c, d, e, f, g, h, 0xca273eceea26619c, w[0], w[14], w[9], w[1]);
        round!(h, a, b, c, d, e, f, g, 0xd186b8c721c0c207, w[1], w[15], w[10], w[2]);
        round!(g, h, a, b, c, d, e, f, 0xeada7dd6cde0eb1e, w[2], w[0], w[11], w[3]);
        round!(f, g, h, a, b, c, d, e, 0xf57d4f7fee6ed178, w[3], w[1], w[12], w[4]);
        round!(e, f, g, h, a, b, c, d, 0x06f067aa72176fba, w[4], w[2], w[13], w[5]);
        round!(d, e, f, g, h, a, b, c, 0x0a637dc5a2c898a6, w[5], w[3], w[14], w[6]);
        round!(c, d, e, f, g, h, a, b, 0x113f9804bef90dae, w[6], w[4], w[15], w[7]);
        round!(b, c, d, e, f, g, h, a, 0x1b710b35131c471b, w[7], w[5], w[0], w[8]);
        round!(a, b, c, d, e, f, g, h, 0x28db77f523047d84, w[8], w[6], w[1], w[9]);
        round!(h, a, b, c, d, e, f, g, 0x32caab7b40c72493, w[9], w[7], w[2], w[10]);
        round!(g, h, a, b, c, d, e, f, 0x3c9ebe0a15c9bebc, w[10], w[8], w[3], w[11]);
        round!(f, g, h, a, b, c, d, e, 0x431d67c49c100d4c, w[11], w[9], w[4], w[12]);
        round!(e, f, g, h, a, b, c, d, 0x4cc5d4becb3e42b6, w[12], w[10], w[5], w[13]);
        round!(d, e, f, g, h, a, b, c, 0x597f299cfc657e2a, w[13], w[11], w[6], w[14]);
        round!(c, d, e, f, g, h, a, b, 0x5fcb6fab3ad6faec, w[14], w[12], w[7], w[15]);
        round!(b, c, d, e, f, g, h, a, 0x6c44198c4a475817, w[15], w[13], w[8], w[0]);

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
    use sha512;
    use hex::{FromHex, ToHex};
    use Hash;
    use HashEngine;

    #[derive(Clone)]
    struct Test {
        input: &'static str,
        output: Vec<u8>,
        output_str: &'static str,
    }

    #[test]
    fn test() {
        let tests = vec![
            // Test vectors computed with `sha512sum`
            Test {
                input: "",
                output: vec![
                    0xcf, 0x83, 0xe1, 0x35, 0x7e, 0xef, 0xb8, 0xbd,
                    0xf1, 0x54, 0x28, 0x50, 0xd6, 0x6d, 0x80, 0x07,
                    0xd6, 0x20, 0xe4, 0x05, 0x0b, 0x57, 0x15, 0xdc,
                    0x83, 0xf4, 0xa9, 0x21, 0xd3, 0x6c, 0xe9, 0xce,
                    0x47, 0xd0, 0xd1, 0x3c, 0x5d, 0x85, 0xf2, 0xb0,
                    0xff, 0x83, 0x18, 0xd2, 0x87, 0x7e, 0xec, 0x2f,
                    0x63, 0xb9, 0x31, 0xbd, 0x47, 0x41, 0x7a, 0x81,
                    0xa5, 0x38, 0x32, 0x7a, 0xf9, 0x27, 0xda, 0x3e,
                ],
                output_str: "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"
            },
            Test {
                input: "The quick brown fox jumps over the lazy dog",
                output: vec![
                    0x07, 0xe5, 0x47, 0xd9, 0x58, 0x6f, 0x6a, 0x73,
                    0xf7, 0x3f, 0xba, 0xc0, 0x43, 0x5e, 0xd7, 0x69,
                    0x51, 0x21, 0x8f, 0xb7, 0xd0, 0xc8, 0xd7, 0x88,
                    0xa3, 0x09, 0xd7, 0x85, 0x43, 0x6b, 0xbb, 0x64,
                    0x2e, 0x93, 0xa2, 0x52, 0xa9, 0x54, 0xf2, 0x39,
                    0x12, 0x54, 0x7d, 0x1e, 0x8a, 0x3b, 0x5e, 0xd6,
                    0xe1, 0xbf, 0xd7, 0x09, 0x78, 0x21, 0x23, 0x3f,
                    0xa0, 0x53, 0x8f, 0x3d, 0xb8, 0x54, 0xfe, 0xe6,
                ],
                output_str: "07e547d9586f6a73f73fbac0435ed76951218fb7d0c8d788a309d785436bbb642e93a252a954f23912547d1e8a3b5ed6e1bfd7097821233fa0538f3db854fee6",
            },
            Test {
                input: "The quick brown fox jumps over the lazy dog.",
                output: vec![
                    0x91, 0xea, 0x12, 0x45, 0xf2, 0x0d, 0x46, 0xae,
                    0x9a, 0x03, 0x7a, 0x98, 0x9f, 0x54, 0xf1, 0xf7,
                    0x90, 0xf0, 0xa4, 0x76, 0x07, 0xee, 0xb8, 0xa1,
                    0x4d, 0x12, 0x89, 0x0c, 0xea, 0x77, 0xa1, 0xbb,
                    0xc6, 0xc7, 0xed, 0x9c, 0xf2, 0x05, 0xe6, 0x7b,
                    0x7f, 0x2b, 0x8f, 0xd4, 0xc7, 0xdf, 0xd3, 0xa7,
                    0xa8, 0x61, 0x7e, 0x45, 0xf3, 0xc4, 0x63, 0xd4,
                    0x81, 0xc7, 0xe5, 0x86, 0xc3, 0x9a, 0xc1, 0xed,
                ],
                output_str: "91ea1245f20d46ae9a037a989f54f1f790f0a47607eeb8a14d12890cea77a1bbc6c7ed9cf205e67b7f2b8fd4c7dfd3a7a8617e45f3c463d481c7e586c39ac1ed",
            },
        ];

        for test in tests {
            // Hash through high-level API, check hex encoding/decoding
            let hash = sha512::Hash::hash(&test.input.as_bytes());
            assert_eq!(hash, sha512::Hash::from_hex(test.output_str).expect("parse hex"));
            assert_eq!(&hash[..], &test.output[..]);
            assert_eq!(&hash.to_hex(), &test.output_str);

            // Hash through engine, checking that we can input byte by byte
            let mut engine = sha512::Hash::engine();
            for ch in test.input.as_bytes() {
                engine.input(&[*ch]);
            }
            let manual_hash = sha512::Hash::from_engine(engine);
            assert_eq!(hash, manual_hash);
            assert_eq!(hash.into_inner()[..].as_ref(), test.output.as_slice());
        }
    }

    #[cfg(feature="serde")]
    #[test]
    fn sha512_serde() {
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

        let hash = sha512::Hash::from_slice(&HASH_BYTES).expect("right number of bytes");
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

    use sha512;
    use Hash;
    use HashEngine;

    #[bench]
    pub fn sha512_10(bh: & mut Bencher) {
        let mut engine = sha512::Hash::engine();
        let bytes = [1u8; 10];
        bh.iter( || {
            engine.input(&bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

    #[bench]
    pub fn sha512_1k(bh: & mut Bencher) {
        let mut engine = sha512::Hash::engine();
        let bytes = [1u8; 1024];
        bh.iter( || {
            engine.input(&bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

    #[bench]
    pub fn sha512_64k(bh: & mut Bencher) {
        let mut engine = sha512::Hash::engine();
        let bytes = [1u8; 65536];
        bh.iter( || {
            engine.input(&bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

}
