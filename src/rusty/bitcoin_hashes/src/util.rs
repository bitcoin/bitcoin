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

/// Circular left-shift a 32-bit word
macro_rules! circular_lshift32 (
    ($shift:expr, $w:expr) => (($w << $shift) | ($w >> (32 - $shift)))
);

macro_rules! circular_lshift64 (
    ($shift:expr, $w:expr) => (($w << $shift) | ($w >> (64 - $shift)))
);

macro_rules! hex_fmt_impl(
    ($imp:ident, $ty:ident) => (
        impl ::core::fmt::$imp for $ty {
            fn fmt(&self, f: &mut ::core::fmt::Formatter) -> ::core::fmt::Result {
                use hex::{format_hex, format_hex_reverse};
                if $ty::DISPLAY_BACKWARD {
                    format_hex_reverse(&self.0, f)
                } else {
                    format_hex(&self.0, f)
                }
            }
        }
    )
);

macro_rules! index_impl(
    ($ty:ty) => (
        impl ::core::ops::Index<usize> for $ty {
            type Output = u8;
            fn index(&self, index: usize) -> &u8 {
                &self.0[index]
            }
        }

        impl ::core::ops::Index<::core::ops::Range<usize>> for $ty {
            type Output = [u8];
            fn index(&self, index: ::core::ops::Range<usize>) -> &[u8] {
                &self.0[index]
            }
        }

        impl ::core::ops::Index<::core::ops::RangeFrom<usize>> for $ty {
            type Output = [u8];
            fn index(&self, index: ::core::ops::RangeFrom<usize>) -> &[u8] {
                &self.0[index]
            }
        }

        impl ::core::ops::Index<::core::ops::RangeTo<usize>> for $ty {
            type Output = [u8];
            fn index(&self, index: ::core::ops::RangeTo<usize>) -> &[u8] {
                &self.0[index]
            }
        }

        impl ::core::ops::Index<::core::ops::RangeFull> for $ty {
            type Output = [u8];
            fn index(&self, index: ::core::ops::RangeFull) -> &[u8] {
                &self.0[index]
            }
        }
    )
);

macro_rules! borrow_slice_impl(
    ($ty:ty) => (
        impl ::core::borrow::Borrow<[u8]> for $ty {
            fn borrow(&self) -> &[u8] {
                &self[..]
            }
        }

        impl ::core::convert::AsRef<[u8]> for $ty {
            fn as_ref(&self) -> &[u8] {
                &self[..]
            }
        }

        impl ::core::ops::Deref for $ty {
            type Target = [u8];

            fn deref(&self) -> &Self::Target {
                &self.0
            }
        }
    )
);

macro_rules! engine_input_impl(
    () => (
        #[cfg(not(feature = "fuzztarget"))]
        fn input(&mut self, mut inp: &[u8]) {
            while !inp.is_empty() {
                let buf_idx = self.length % <Self as EngineTrait>::BLOCK_SIZE;
                let rem_len = <Self as EngineTrait>::BLOCK_SIZE - buf_idx;
                let write_len = cmp::min(rem_len, inp.len());

                self.buffer[buf_idx..buf_idx + write_len]
                    .copy_from_slice(&inp[..write_len]);
                self.length += write_len;
                if self.length % <Self as EngineTrait>::BLOCK_SIZE == 0 {
                    self.process_block();
                }
                inp = &inp[write_len..];
            }
        }

        #[cfg(feature = "fuzztarget")]
        fn input(&mut self, inp: &[u8]) {
            for c in inp {
                self.buffer[0] ^= *c;
            }
            self.length += inp.len();
        }
    )
);

#[inline]
pub fn slice_to_u32_be(slice: &[u8]) -> u32 {
    assert_eq!(slice.len(), 4);
    (slice[0] as u32) << 3*8 |
    (slice[1] as u32) << 2*8 |
    (slice[2] as u32) << 1*8 |
    (slice[3] as u32) << 0*8
}

#[inline]
pub fn u32_to_array_be(val: u32) -> [u8; 4] {
    [
        ((val >> 3*8) & 0xff) as u8,
        ((val >> 2*8) & 0xff) as u8,
        ((val >> 1*8) & 0xff) as u8,
        ((val >> 0*8) & 0xff) as u8,
    ]
}

#[inline]
pub fn slice_to_u64_be(slice: &[u8]) -> u64 {
    assert_eq!(slice.len(), 8);
    (slice[0] as u64) << 7*8 |
    (slice[1] as u64) << 6*8 |
    (slice[2] as u64) << 5*8 |
    (slice[3] as u64) << 4*8 |
    (slice[4] as u64) << 3*8 |
    (slice[5] as u64) << 2*8 |
    (slice[6] as u64) << 1*8 |
    (slice[7] as u64) << 0*8
}

#[inline]
pub fn u64_to_array_be(val: u64) -> [u8; 8] {
    [
        ((val >> 7*8) & 0xff) as u8,
        ((val >> 6*8) & 0xff) as u8,
        ((val >> 5*8) & 0xff) as u8,
        ((val >> 4*8) & 0xff) as u8,
        ((val >> 3*8) & 0xff) as u8,
        ((val >> 2*8) & 0xff) as u8,
        ((val >> 1*8) & 0xff) as u8,
        ((val >> 0*8) & 0xff) as u8,
    ]
}

#[inline]
pub fn slice_to_u32_le(slice: &[u8]) -> u32 {
    assert_eq!(slice.len(), 4);
    (slice[0] as u32) << 0*8 |
    (slice[1] as u32) << 1*8 |
    (slice[2] as u32) << 2*8 |
    (slice[3] as u32) << 3*8
}

#[inline]
pub fn u32_to_array_le(val: u32) -> [u8; 4] {
    [
        ((val >> 0*8) & 0xff) as u8,
        ((val >> 1*8) & 0xff) as u8,
        ((val >> 2*8) & 0xff) as u8,
        ((val >> 3*8) & 0xff) as u8,
    ]
}

#[inline]
pub fn slice_to_u64_le(slice: &[u8]) -> u64 {
    assert_eq!(slice.len(), 8);
    (slice[0] as u64) << 0*8 |
    (slice[1] as u64) << 1*8 |
    (slice[2] as u64) << 2*8 |
    (slice[3] as u64) << 3*8 |
    (slice[4] as u64) << 4*8 |
    (slice[5] as u64) << 5*8 |
    (slice[6] as u64) << 6*8 |
    (slice[7] as u64) << 7*8
}

#[inline]
pub fn u64_to_array_le(val: u64) -> [u8; 8] {
    [
        ((val >> 0*8) & 0xff) as u8,
        ((val >> 1*8) & 0xff) as u8,
        ((val >> 2*8) & 0xff) as u8,
        ((val >> 3*8) & 0xff) as u8,
        ((val >> 4*8) & 0xff) as u8,
        ((val >> 5*8) & 0xff) as u8,
        ((val >> 6*8) & 0xff) as u8,
        ((val >> 7*8) & 0xff) as u8,
    ]
}

#[cfg(test)]
mod test {
    use Hash;
    use sha256;

    #[test]
    fn borrow_slice_impl_to_vec() {
        // Test that the borrow_slice_impl macro gives to_vec.
        let hash = sha256::Hash::hash(&[3, 50]);
        assert_eq!(hash.to_vec().len(), sha256::Hash::LEN);
    }
}
