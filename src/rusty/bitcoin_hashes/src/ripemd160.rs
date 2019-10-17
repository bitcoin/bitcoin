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

//! # RIPEMD160

use core::{cmp, str};

use HashEngine as EngineTrait;
use Hash as HashTrait;
use Error;
use util;

const BLOCK_SIZE: usize = 64;

/// Engine to compute RIPEMD160 hash function
#[derive(Clone)]
pub struct HashEngine {
    buffer: [u8; BLOCK_SIZE],
    h: [u32; 5],
    length: usize,
}

impl Default for HashEngine {
    fn default() -> Self {
        HashEngine {
            h: [0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0],
            length: 0,
            buffer: [0; BLOCK_SIZE],
        }
    }
}

impl EngineTrait for HashEngine {
    type MidState = [u8; 20];

    #[cfg(not(feature = "fuzztarget"))]
    fn midstate(&self) -> [u8; 20] {
        let mut ret = [0; 20];
        for (val, ret_bytes) in self.h.iter().zip(ret.chunks_mut(4)) {
            ret_bytes.copy_from_slice(&util::u32_to_array_le(*val));
        }
        ret
    }

    #[cfg(feature = "fuzztarget")]
    fn midstate(&self) -> [u8; 20] {
        let mut ret = [0; 20];
        ret.copy_from_slice(&self.buffer[..20]);
        ret
    }

    const BLOCK_SIZE: usize = 64;

    engine_input_impl!();
}

/// Output of the RIPEMD160 hash function
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
    type Engine = HashEngine;
    type Inner = [u8; 20];

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

        e.input(&util::u64_to_array_le(8 * data_len));
        debug_assert_eq!(e.length % BLOCK_SIZE, 0);

        Hash(e.midstate())
    }

    #[cfg(feature = "fuzztarget")]
    fn from_engine(e: HashEngine) -> Hash {
        let mut res = e.midstate();
        res[0] ^= (e.length & 0xff) as u8;
        Hash(res)
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

macro_rules! round(
    ($a:expr, $b:expr, $c:expr, $d:expr, $e:expr,
     $x:expr, $bits:expr, $add:expr, $round:expr) => ({
        $a = $a.wrapping_add($round).wrapping_add($x).wrapping_add($add);
        $a = circular_lshift32!($bits, $a).wrapping_add($e);
        $c = circular_lshift32!(10, $c);
    });
);

macro_rules! process_block(
    ($h:expr, $data:expr,
     $( round1: h_ordering $f0:expr, $f1:expr, $f2:expr, $f3:expr, $f4:expr;
                data_index $data_index1:expr; roll_shift $bits1:expr; )*
     $( round2: h_ordering $g0:expr, $g1:expr, $g2:expr, $g3:expr, $g4:expr;
                data_index $data_index2:expr; roll_shift $bits2:expr; )*
     $( round3: h_ordering $h0:expr, $h1:expr, $h2:expr, $h3:expr, $h4:expr;
                data_index $data_index3:expr; roll_shift $bits3:expr; )*
     $( round4: h_ordering $i0:expr, $i1:expr, $i2:expr, $i3:expr, $i4:expr;
                data_index $data_index4:expr; roll_shift $bits4:expr; )*
     $( round5: h_ordering $j0:expr, $j1:expr, $j2:expr, $j3:expr, $j4:expr;
                data_index $data_index5:expr; roll_shift $bits5:expr; )*
     $( par_round1: h_ordering $pj0:expr, $pj1:expr, $pj2:expr, $pj3:expr, $pj4:expr;
                    data_index $pdata_index1:expr; roll_shift $pbits1:expr; )*
     $( par_round2: h_ordering $pi0:expr, $pi1:expr, $pi2:expr, $pi3:expr, $pi4:expr;
                    data_index $pdata_index2:expr; roll_shift $pbits2:expr; )*
     $( par_round3: h_ordering $ph0:expr, $ph1:expr, $ph2:expr, $ph3:expr, $ph4:expr;
                    data_index $pdata_index3:expr; roll_shift $pbits3:expr; )*
     $( par_round4: h_ordering $pg0:expr, $pg1:expr, $pg2:expr, $pg3:expr, $pg4:expr;
                    data_index $pdata_index4:expr; roll_shift $pbits4:expr; )*
     $( par_round5: h_ordering $pf0:expr, $pf1:expr, $pf2:expr, $pf3:expr, $pf4:expr;
                    data_index $pdata_index5:expr; roll_shift $pbits5:expr; )*
    ) => ({
        let mut bb = $h;
        let mut bbb = $h;

        // Round 1
        $( round!(bb[$f0], bb[$f1], bb[$f2], bb[$f3], bb[$f4],
                  $data[$data_index1], $bits1, 0x00000000,
                  bb[$f1] ^ bb[$f2] ^ bb[$f3]); )*

        // Round 2
        $( round!(bb[$g0], bb[$g1], bb[$g2], bb[$g3], bb[$g4],
                  $data[$data_index2], $bits2, 0x5a827999,
                  (bb[$g1] & bb[$g2]) | (!bb[$g1] & bb[$g3])); )*

        // Round 3
        $( round!(bb[$h0], bb[$h1], bb[$h2], bb[$h3], bb[$h4],
                  $data[$data_index3], $bits3, 0x6ed9eba1,
                  (bb[$h1] | !bb[$h2]) ^ bb[$h3]); )*

        // Round 4
        $( round!(bb[$i0], bb[$i1], bb[$i2], bb[$i3], bb[$i4],
                  $data[$data_index4], $bits4, 0x8f1bbcdc,
                  (bb[$i1] & bb[$i3]) | (bb[$i2] & !bb[$i3])); )*

        // Round 5
        $( round!(bb[$j0], bb[$j1], bb[$j2], bb[$j3], bb[$j4],
                  $data[$data_index5], $bits5, 0xa953fd4e,
                  bb[$j1] ^ (bb[$j2] | !bb[$j3])); )*

        // Parallel rounds: these are the same as the previous five
        // rounds except that the constants have changed, we work
        // with the other buffer, and they are applied in reverse
        // order.

        // Parallel Round 1
        $( round!(bbb[$pj0], bbb[$pj1], bbb[$pj2], bbb[$pj3], bbb[$pj4],
                  $data[$pdata_index1], $pbits1, 0x50a28be6,
                  bbb[$pj1] ^ (bbb[$pj2] | !bbb[$pj3])); )*

        // Porallel Round 2
        $( round!(bbb[$pi0], bbb[$pi1], bbb[$pi2], bbb[$pi3], bbb[$pi4],
                  $data[$pdata_index2], $pbits2, 0x5c4dd124,
                  (bbb[$pi1] & bbb[$pi3]) | (bbb[$pi2] & !bbb[$pi3])); )*

        // Parallel Round 3
        $( round!(bbb[$ph0], bbb[$ph1], bbb[$ph2], bbb[$ph3], bbb[$ph4],
                  $data[$pdata_index3], $pbits3, 0x6d703ef3,
                  (bbb[$ph1] | !bbb[$ph2]) ^ bbb[$ph3]); )*

        // Parallel Round 4
        $( round!(bbb[$pg0], bbb[$pg1], bbb[$pg2], bbb[$pg3], bbb[$pg4],
                  $data[$pdata_index4], $pbits4, 0x7a6d76e9,
                  (bbb[$pg1] & bbb[$pg2]) | (!bbb[$pg1] & bbb[$pg3])); )*

        // Parallel Round 5
        $( round!(bbb[$pf0], bbb[$pf1], bbb[$pf2], bbb[$pf3], bbb[$pf4],
                  $data[$pdata_index5], $pbits5, 0x00000000,
                  bbb[$pf1] ^ bbb[$pf2] ^ bbb[$pf3]); )*

        // Combine results
        bbb[3] = bbb[3].wrapping_add($h[1]).wrapping_add(bb[2]);
        $h[1]  =  $h[2].wrapping_add(bb[3]).wrapping_add(bbb[4]);
        $h[2]  =  $h[3].wrapping_add(bb[4]).wrapping_add(bbb[0]);
        $h[3]  =  $h[4].wrapping_add(bb[0]).wrapping_add(bbb[1]);
        $h[4]  =  $h[0].wrapping_add(bb[1]).wrapping_add(bbb[2]);
        $h[0]  =                                         bbb[3];
    });
);

impl HashEngine {
    fn process_block(&mut self) {
        debug_assert_eq!(self.buffer.len(), BLOCK_SIZE);

        let mut w = [0u32; 16];
        for (w_val, buff_bytes) in w.iter_mut().zip(self.buffer.chunks(4)) {
            *w_val = util::slice_to_u32_le(buff_bytes);
        }

        process_block!(self.h, w,
            // Round 1
            round1: h_ordering 0, 1, 2, 3, 4; data_index  0; roll_shift 11;
            round1: h_ordering 4, 0, 1, 2, 3; data_index  1; roll_shift 14;
            round1: h_ordering 3, 4, 0, 1, 2; data_index  2; roll_shift 15;
            round1: h_ordering 2, 3, 4, 0, 1; data_index  3; roll_shift 12;
            round1: h_ordering 1, 2, 3, 4, 0; data_index  4; roll_shift  5;
            round1: h_ordering 0, 1, 2, 3, 4; data_index  5; roll_shift  8;
            round1: h_ordering 4, 0, 1, 2, 3; data_index  6; roll_shift  7;
            round1: h_ordering 3, 4, 0, 1, 2; data_index  7; roll_shift  9;
            round1: h_ordering 2, 3, 4, 0, 1; data_index  8; roll_shift 11;
            round1: h_ordering 1, 2, 3, 4, 0; data_index  9; roll_shift 13;
            round1: h_ordering 0, 1, 2, 3, 4; data_index 10; roll_shift 14;
            round1: h_ordering 4, 0, 1, 2, 3; data_index 11; roll_shift 15;
            round1: h_ordering 3, 4, 0, 1, 2; data_index 12; roll_shift  6;
            round1: h_ordering 2, 3, 4, 0, 1; data_index 13; roll_shift  7;
            round1: h_ordering 1, 2, 3, 4, 0; data_index 14; roll_shift  9;
            round1: h_ordering 0, 1, 2, 3, 4; data_index 15; roll_shift  8;

            // Round 2
            round2: h_ordering 4, 0, 1, 2, 3; data_index  7; roll_shift  7;
            round2: h_ordering 3, 4, 0, 1, 2; data_index  4; roll_shift  6;
            round2: h_ordering 2, 3, 4, 0, 1; data_index 13; roll_shift  8;
            round2: h_ordering 1, 2, 3, 4, 0; data_index  1; roll_shift 13;
            round2: h_ordering 0, 1, 2, 3, 4; data_index 10; roll_shift 11;
            round2: h_ordering 4, 0, 1, 2, 3; data_index  6; roll_shift  9;
            round2: h_ordering 3, 4, 0, 1, 2; data_index 15; roll_shift  7;
            round2: h_ordering 2, 3, 4, 0, 1; data_index  3; roll_shift 15;
            round2: h_ordering 1, 2, 3, 4, 0; data_index 12; roll_shift  7;
            round2: h_ordering 0, 1, 2, 3, 4; data_index  0; roll_shift 12;
            round2: h_ordering 4, 0, 1, 2, 3; data_index  9; roll_shift 15;
            round2: h_ordering 3, 4, 0, 1, 2; data_index  5; roll_shift  9;
            round2: h_ordering 2, 3, 4, 0, 1; data_index  2; roll_shift 11;
            round2: h_ordering 1, 2, 3, 4, 0; data_index 14; roll_shift  7;
            round2: h_ordering 0, 1, 2, 3, 4; data_index 11; roll_shift 13;
            round2: h_ordering 4, 0, 1, 2, 3; data_index  8; roll_shift 12;

            // Round 3
            round3: h_ordering 3, 4, 0, 1, 2; data_index  3; roll_shift 11;
            round3: h_ordering 2, 3, 4, 0, 1; data_index 10; roll_shift 13;
            round3: h_ordering 1, 2, 3, 4, 0; data_index 14; roll_shift  6;
            round3: h_ordering 0, 1, 2, 3, 4; data_index  4; roll_shift  7;
            round3: h_ordering 4, 0, 1, 2, 3; data_index  9; roll_shift 14;
            round3: h_ordering 3, 4, 0, 1, 2; data_index 15; roll_shift  9;
            round3: h_ordering 2, 3, 4, 0, 1; data_index  8; roll_shift 13;
            round3: h_ordering 1, 2, 3, 4, 0; data_index  1; roll_shift 15;
            round3: h_ordering 0, 1, 2, 3, 4; data_index  2; roll_shift 14;
            round3: h_ordering 4, 0, 1, 2, 3; data_index  7; roll_shift  8;
            round3: h_ordering 3, 4, 0, 1, 2; data_index  0; roll_shift 13;
            round3: h_ordering 2, 3, 4, 0, 1; data_index  6; roll_shift  6;
            round3: h_ordering 1, 2, 3, 4, 0; data_index 13; roll_shift  5;
            round3: h_ordering 0, 1, 2, 3, 4; data_index 11; roll_shift 12;
            round3: h_ordering 4, 0, 1, 2, 3; data_index  5; roll_shift  7;
            round3: h_ordering 3, 4, 0, 1, 2; data_index 12; roll_shift  5;

            // Round 4
            round4: h_ordering 2, 3, 4, 0, 1; data_index  1; roll_shift 11;
            round4: h_ordering 1, 2, 3, 4, 0; data_index  9; roll_shift 12;
            round4: h_ordering 0, 1, 2, 3, 4; data_index 11; roll_shift 14;
            round4: h_ordering 4, 0, 1, 2, 3; data_index 10; roll_shift 15;
            round4: h_ordering 3, 4, 0, 1, 2; data_index  0; roll_shift 14;
            round4: h_ordering 2, 3, 4, 0, 1; data_index  8; roll_shift 15;
            round4: h_ordering 1, 2, 3, 4, 0; data_index 12; roll_shift  9;
            round4: h_ordering 0, 1, 2, 3, 4; data_index  4; roll_shift  8;
            round4: h_ordering 4, 0, 1, 2, 3; data_index 13; roll_shift  9;
            round4: h_ordering 3, 4, 0, 1, 2; data_index  3; roll_shift 14;
            round4: h_ordering 2, 3, 4, 0, 1; data_index  7; roll_shift  5;
            round4: h_ordering 1, 2, 3, 4, 0; data_index 15; roll_shift  6;
            round4: h_ordering 0, 1, 2, 3, 4; data_index 14; roll_shift  8;
            round4: h_ordering 4, 0, 1, 2, 3; data_index  5; roll_shift  6;
            round4: h_ordering 3, 4, 0, 1, 2; data_index  6; roll_shift  5;
            round4: h_ordering 2, 3, 4, 0, 1; data_index  2; roll_shift 12;

            // Round 5
            round5: h_ordering 1, 2, 3, 4, 0; data_index  4; roll_shift  9;
            round5: h_ordering 0, 1, 2, 3, 4; data_index  0; roll_shift 15;
            round5: h_ordering 4, 0, 1, 2, 3; data_index  5; roll_shift  5;
            round5: h_ordering 3, 4, 0, 1, 2; data_index  9; roll_shift 11;
            round5: h_ordering 2, 3, 4, 0, 1; data_index  7; roll_shift  6;
            round5: h_ordering 1, 2, 3, 4, 0; data_index 12; roll_shift  8;
            round5: h_ordering 0, 1, 2, 3, 4; data_index  2; roll_shift 13;
            round5: h_ordering 4, 0, 1, 2, 3; data_index 10; roll_shift 12;
            round5: h_ordering 3, 4, 0, 1, 2; data_index 14; roll_shift  5;
            round5: h_ordering 2, 3, 4, 0, 1; data_index  1; roll_shift 12;
            round5: h_ordering 1, 2, 3, 4, 0; data_index  3; roll_shift 13;
            round5: h_ordering 0, 1, 2, 3, 4; data_index  8; roll_shift 14;
            round5: h_ordering 4, 0, 1, 2, 3; data_index 11; roll_shift 11;
            round5: h_ordering 3, 4, 0, 1, 2; data_index  6; roll_shift  8;
            round5: h_ordering 2, 3, 4, 0, 1; data_index 15; roll_shift  5;
            round5: h_ordering 1, 2, 3, 4, 0; data_index 13; roll_shift  6;

            // Porallel Round 1;
            par_round1: h_ordering 0, 1, 2, 3, 4; data_index  5; roll_shift  8;
            par_round1: h_ordering 4, 0, 1, 2, 3; data_index 14; roll_shift  9;
            par_round1: h_ordering 3, 4, 0, 1, 2; data_index  7; roll_shift  9;
            par_round1: h_ordering 2, 3, 4, 0, 1; data_index  0; roll_shift 11;
            par_round1: h_ordering 1, 2, 3, 4, 0; data_index  9; roll_shift 13;
            par_round1: h_ordering 0, 1, 2, 3, 4; data_index  2; roll_shift 15;
            par_round1: h_ordering 4, 0, 1, 2, 3; data_index 11; roll_shift 15;
            par_round1: h_ordering 3, 4, 0, 1, 2; data_index  4; roll_shift  5;
            par_round1: h_ordering 2, 3, 4, 0, 1; data_index 13; roll_shift  7;
            par_round1: h_ordering 1, 2, 3, 4, 0; data_index  6; roll_shift  7;
            par_round1: h_ordering 0, 1, 2, 3, 4; data_index 15; roll_shift  8;
            par_round1: h_ordering 4, 0, 1, 2, 3; data_index  8; roll_shift 11;
            par_round1: h_ordering 3, 4, 0, 1, 2; data_index  1; roll_shift 14;
            par_round1: h_ordering 2, 3, 4, 0, 1; data_index 10; roll_shift 14;
            par_round1: h_ordering 1, 2, 3, 4, 0; data_index  3; roll_shift 12;
            par_round1: h_ordering 0, 1, 2, 3, 4; data_index 12; roll_shift  6;

            // Parallel Round 2
            par_round2: h_ordering 4, 0, 1, 2, 3; data_index  6; roll_shift  9;
            par_round2: h_ordering 3, 4, 0, 1, 2; data_index 11; roll_shift 13;
            par_round2: h_ordering 2, 3, 4, 0, 1; data_index  3; roll_shift 15;
            par_round2: h_ordering 1, 2, 3, 4, 0; data_index  7; roll_shift  7;
            par_round2: h_ordering 0, 1, 2, 3, 4; data_index  0; roll_shift 12;
            par_round2: h_ordering 4, 0, 1, 2, 3; data_index 13; roll_shift  8;
            par_round2: h_ordering 3, 4, 0, 1, 2; data_index  5; roll_shift  9;
            par_round2: h_ordering 2, 3, 4, 0, 1; data_index 10; roll_shift 11;
            par_round2: h_ordering 1, 2, 3, 4, 0; data_index 14; roll_shift  7;
            par_round2: h_ordering 0, 1, 2, 3, 4; data_index 15; roll_shift  7;
            par_round2: h_ordering 4, 0, 1, 2, 3; data_index  8; roll_shift 12;
            par_round2: h_ordering 3, 4, 0, 1, 2; data_index 12; roll_shift  7;
            par_round2: h_ordering 2, 3, 4, 0, 1; data_index  4; roll_shift  6;
            par_round2: h_ordering 1, 2, 3, 4, 0; data_index  9; roll_shift 15;
            par_round2: h_ordering 0, 1, 2, 3, 4; data_index  1; roll_shift 13;
            par_round2: h_ordering 4, 0, 1, 2, 3; data_index  2; roll_shift 11;

            // Parallel Round 3
            par_round3: h_ordering 3, 4, 0, 1, 2; data_index 15; roll_shift  9;
            par_round3: h_ordering 2, 3, 4, 0, 1; data_index  5; roll_shift  7;
            par_round3: h_ordering 1, 2, 3, 4, 0; data_index  1; roll_shift 15;
            par_round3: h_ordering 0, 1, 2, 3, 4; data_index  3; roll_shift 11;
            par_round3: h_ordering 4, 0, 1, 2, 3; data_index  7; roll_shift  8;
            par_round3: h_ordering 3, 4, 0, 1, 2; data_index 14; roll_shift  6;
            par_round3: h_ordering 2, 3, 4, 0, 1; data_index  6; roll_shift  6;
            par_round3: h_ordering 1, 2, 3, 4, 0; data_index  9; roll_shift 14;
            par_round3: h_ordering 0, 1, 2, 3, 4; data_index 11; roll_shift 12;
            par_round3: h_ordering 4, 0, 1, 2, 3; data_index  8; roll_shift 13;
            par_round3: h_ordering 3, 4, 0, 1, 2; data_index 12; roll_shift  5;
            par_round3: h_ordering 2, 3, 4, 0, 1; data_index  2; roll_shift 14;
            par_round3: h_ordering 1, 2, 3, 4, 0; data_index 10; roll_shift 13;
            par_round3: h_ordering 0, 1, 2, 3, 4; data_index  0; roll_shift 13;
            par_round3: h_ordering 4, 0, 1, 2, 3; data_index  4; roll_shift  7;
            par_round3: h_ordering 3, 4, 0, 1, 2; data_index 13; roll_shift  5;

            // Parallel Round 4
            par_round4: h_ordering 2, 3, 4, 0, 1; data_index  8; roll_shift 15;
            par_round4: h_ordering 1, 2, 3, 4, 0; data_index  6; roll_shift  5;
            par_round4: h_ordering 0, 1, 2, 3, 4; data_index  4; roll_shift  8;
            par_round4: h_ordering 4, 0, 1, 2, 3; data_index  1; roll_shift 11;
            par_round4: h_ordering 3, 4, 0, 1, 2; data_index  3; roll_shift 14;
            par_round4: h_ordering 2, 3, 4, 0, 1; data_index 11; roll_shift 14;
            par_round4: h_ordering 1, 2, 3, 4, 0; data_index 15; roll_shift  6;
            par_round4: h_ordering 0, 1, 2, 3, 4; data_index  0; roll_shift 14;
            par_round4: h_ordering 4, 0, 1, 2, 3; data_index  5; roll_shift  6;
            par_round4: h_ordering 3, 4, 0, 1, 2; data_index 12; roll_shift  9;
            par_round4: h_ordering 2, 3, 4, 0, 1; data_index  2; roll_shift 12;
            par_round4: h_ordering 1, 2, 3, 4, 0; data_index 13; roll_shift  9;
            par_round4: h_ordering 0, 1, 2, 3, 4; data_index  9; roll_shift 12;
            par_round4: h_ordering 4, 0, 1, 2, 3; data_index  7; roll_shift  5;
            par_round4: h_ordering 3, 4, 0, 1, 2; data_index 10; roll_shift 15;
            par_round4: h_ordering 2, 3, 4, 0, 1; data_index 14; roll_shift  8;

            // Parallel Round 5
            par_round5: h_ordering 1, 2, 3, 4, 0; data_index 12; roll_shift  8;
            par_round5: h_ordering 0, 1, 2, 3, 4; data_index 15; roll_shift  5;
            par_round5: h_ordering 4, 0, 1, 2, 3; data_index 10; roll_shift 12;
            par_round5: h_ordering 3, 4, 0, 1, 2; data_index  4; roll_shift  9;
            par_round5: h_ordering 2, 3, 4, 0, 1; data_index  1; roll_shift 12;
            par_round5: h_ordering 1, 2, 3, 4, 0; data_index  5; roll_shift  5;
            par_round5: h_ordering 0, 1, 2, 3, 4; data_index  8; roll_shift 14;
            par_round5: h_ordering 4, 0, 1, 2, 3; data_index  7; roll_shift  6;
            par_round5: h_ordering 3, 4, 0, 1, 2; data_index  6; roll_shift  8;
            par_round5: h_ordering 2, 3, 4, 0, 1; data_index  2; roll_shift 13;
            par_round5: h_ordering 1, 2, 3, 4, 0; data_index 13; roll_shift  6;
            par_round5: h_ordering 0, 1, 2, 3, 4; data_index 14; roll_shift  5;
            par_round5: h_ordering 4, 0, 1, 2, 3; data_index  0; roll_shift 15;
            par_round5: h_ordering 3, 4, 0, 1, 2; data_index  3; roll_shift 13;
            par_round5: h_ordering 2, 3, 4, 0, 1; data_index  9; roll_shift 11;
            par_round5: h_ordering 1, 2, 3, 4, 0; data_index 11; roll_shift 11;
        );
    }
}

#[cfg(test)]
mod tests {
    use ripemd160;
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
            // Test messages from FIPS 180-1
            Test {
                input: "abc",
                output: vec![
                    0x8e, 0xb2, 0x08, 0xf7,
                    0xe0, 0x5d, 0x98, 0x7a,
                    0x9b, 0x04, 0x4a, 0x8e,
                    0x98, 0xc6, 0xb0, 0x87,
                    0xf1, 0x5a, 0x0b, 0xfc,
                ],
                output_str: "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc"
            },
            Test {
                input:
                     "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                output: vec![
                    0x12, 0xa0, 0x53, 0x38,
                    0x4a, 0x9c, 0x0c, 0x88,
                    0xe4, 0x05, 0xa0, 0x6c,
                    0x27, 0xdc, 0xf4, 0x9a,
                    0xda, 0x62, 0xeb, 0x2b,
                ],
                output_str: "12a053384a9c0c88e405a06c27dcf49ada62eb2b"
            },
            // Examples from wikipedia
            Test {
                input: "The quick brown fox jumps over the lazy dog",
                output: vec![
                    0x37, 0xf3, 0x32, 0xf6,
                    0x8d, 0xb7, 0x7b, 0xd9,
                    0xd7, 0xed, 0xd4, 0x96,
                    0x95, 0x71, 0xad, 0x67,
                    0x1c, 0xf9, 0xdd, 0x3b,
                ],
                output_str: "37f332f68db77bd9d7edd4969571ad671cf9dd3b",
            },
            Test {
                input: "The quick brown fox jumps over the lazy cog",
                output: vec![
                    0x13, 0x20, 0x72, 0xdf,
                    0x69, 0x09, 0x33, 0x83,
                    0x5e, 0xb8, 0xb6, 0xad,
                    0x0b, 0x77, 0xe7, 0xb6,
                    0xf1, 0x4a, 0xca, 0xd7,
                ],
                output_str: "132072df690933835eb8b6ad0b77e7b6f14acad7",
            },
        ];

        for test in tests {
            // Hash through high-level API, check hex encoding/decoding
            let hash = ripemd160::Hash::hash(&test.input.as_bytes());
            assert_eq!(hash, ripemd160::Hash::from_hex(test.output_str).expect("parse hex"));
            assert_eq!(&hash[..], &test.output[..]);
            assert_eq!(&hash.to_hex(), &test.output_str);

            // Hash through engine, checking that we can input byte by byte
            let mut engine = ripemd160::Hash::engine();
            for ch in test.input.as_bytes() {
                engine.input(&[*ch]);
            }
            let manual_hash = ripemd160::Hash::from_engine(engine);
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

        let hash = ripemd160::Hash::from_slice(&HASH_BYTES).expect("right number of bytes");
        assert_tokens(&hash.compact(), &[Token::BorrowedBytes(&HASH_BYTES[..])]);
        assert_tokens(&hash.readable(), &[Token::Str("132072df690933835eb8b6ad0b77e7b6f14acad7")]);
    }
}

#[cfg(all(test, feature="unstable"))]
mod benches {
    use test::Bencher;

    use ripemd160;
    use Hash;
    use HashEngine;

    #[bench]
    pub fn ripemd160_10(bh: & mut Bencher) {
        let mut engine = ripemd160::Hash::engine();
        let bytes = [1u8; 10];
        bh.iter( || {
            engine.input(&bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

    #[bench]
    pub fn ripemd160_1k(bh: & mut Bencher) {
        let mut engine = ripemd160::Hash::engine();
        let bytes = [1u8; 1024];
        bh.iter( || {
            engine.input(&bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

    #[bench]
    pub fn ripemd160_64k(bh: & mut Bencher) {
        let mut engine = ripemd160::Hash::engine();
        let bytes = [1u8; 65536];
        bh.iter( || {
            engine.input(&bytes);
        });
        bh.bytes = bytes.len() as u64;
    }

}
