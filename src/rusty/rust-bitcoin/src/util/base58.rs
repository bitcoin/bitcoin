// Rust Bitcoin Library
// Written in 2014 by
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

//! Base58 encoder and decoder

use std::{error, fmt, str, slice, iter};

use hashes::{sha256d, Hash};

use util::endian;

/// An error that might occur during base58 decoding
#[derive(Debug, PartialEq, Eq, Clone)]
pub enum Error {
    /// Invalid character encountered
    BadByte(u8),
    /// Checksum was not correct (expected, actual)
    BadChecksum(u32, u32),
    /// The length (in bytes) of the object was not correct
    /// Note that if the length is excessively long the provided length may be
    /// an estimate (and the checksum step may be skipped).
    InvalidLength(usize),
    /// Version byte(s) were not recognized
    InvalidVersion(Vec<u8>),
    /// Checked data was less than 4 bytes
    TooShort(usize),
    /// Any other error
    Other(String)
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            Error::BadByte(b) => write!(f, "invalid base58 character 0x{:x}", b),
            Error::BadChecksum(exp, actual) => write!(f, "base58ck checksum 0x{:x} does not match expected 0x{:x}", actual, exp),
            Error::InvalidLength(ell) => write!(f, "length {} invalid for this base58 type", ell),
            Error::InvalidVersion(ref v) => write!(f, "version {:?} invalid for this base58 type", v),
            Error::TooShort(_) => write!(f, "base58ck data not even long enough for a checksum"),
            Error::Other(ref s) => f.write_str(s)
        }
    }
}

impl error::Error for Error {
    fn cause(&self) -> Option<&error::Error> { None }
    fn description(&self) -> &'static str {
        match *self {
            Error::BadByte(_) => "invalid b58 character",
            Error::BadChecksum(_, _) => "invalid b58ck checksum",
            Error::InvalidLength(_) => "invalid length for b58 type",
            Error::InvalidVersion(_) => "invalid version for b58 type",
            Error::TooShort(_) => "b58ck data less than 4 bytes",
            Error::Other(_) => "unknown b58 error"
        }
    }
}

/// Vector-like object that holds the first 100 elements on the stack. If more space is needed it
/// will be allocated on the heap.
struct SmallVec<T> {
    len: usize,
    stack: [T; 100],
    heap: Vec<T>,
}

impl<T: Default + Copy> SmallVec<T> {
    pub fn new() -> SmallVec<T> {
        SmallVec {
            len: 0,
            stack: [T::default(); 100],
            heap: Vec::new(),
        }
    }

    pub fn push(&mut self, val: T) {
        if self.len < 100 {
            self.stack[self.len] = val;
            self.len += 1;
        } else {
            self.heap.push(val);
        }
    }

    pub fn iter(&self) -> iter::Chain<slice::Iter<T>, slice::Iter<T>> {
        // If len<100 then we just append an empty vec
        self.stack[0..self.len].iter().chain(self.heap.iter())
    }

    pub fn iter_mut(&mut self) -> iter::Chain<slice::IterMut<T>, slice::IterMut<T>> {
        // If len<100 then we just append an empty vec
        self.stack[0..self.len].iter_mut().chain(self.heap.iter_mut())
    }
}

static BASE58_CHARS: &'static [u8] = b"123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

static BASE58_DIGITS: [Option<u8>; 128] = [
    None,     None,     None,     None,     None,     None,     None,     None,     // 0-7
    None,     None,     None,     None,     None,     None,     None,     None,     // 8-15
    None,     None,     None,     None,     None,     None,     None,     None,     // 16-23
    None,     None,     None,     None,     None,     None,     None,     None,     // 24-31
    None,     None,     None,     None,     None,     None,     None,     None,     // 32-39
    None,     None,     None,     None,     None,     None,     None,     None,     // 40-47
    None,     Some(0),  Some(1),  Some(2),  Some(3),  Some(4),  Some(5),  Some(6),  // 48-55
    Some(7),  Some(8),  None,     None,     None,     None,     None,     None,     // 56-63
    None,     Some(9),  Some(10), Some(11), Some(12), Some(13), Some(14), Some(15), // 64-71
    Some(16), None,     Some(17), Some(18), Some(19), Some(20), Some(21), None,     // 72-79
    Some(22), Some(23), Some(24), Some(25), Some(26), Some(27), Some(28), Some(29), // 80-87
    Some(30), Some(31), Some(32), None,     None,     None,     None,     None,     // 88-95
    None,     Some(33), Some(34), Some(35), Some(36), Some(37), Some(38), Some(39), // 96-103
    Some(40), Some(41), Some(42), Some(43), None,     Some(44), Some(45), Some(46), // 104-111
    Some(47), Some(48), Some(49), Some(50), Some(51), Some(52), Some(53), Some(54), // 112-119
    Some(55), Some(56), Some(57), None,     None,     None,     None,     None,     // 120-127
];

/// Decode base58-encoded string into a byte vector
pub fn from(data: &str) -> Result<Vec<u8>, Error> {
    // 11/15 is just over log_256(58)
    let mut scratch = vec![0u8; 1 + data.len() * 11 / 15];
    // Build in base 256
    for d58 in data.bytes() {
        // Compute "X = X * 58 + next_digit" in base 256
        if d58 as usize > BASE58_DIGITS.len() {
            return Err(Error::BadByte(d58));
        }
        let mut carry = match BASE58_DIGITS[d58 as usize] {
            Some(d58) => d58 as u32,
            None => { return Err(Error::BadByte(d58)); }
        };
        for d256 in scratch.iter_mut().rev() {
            carry += *d256 as u32 * 58;
            *d256 = carry as u8;
            carry /= 256;
        }
        assert_eq!(carry, 0);
    }

    // Copy leading zeroes directly
    let mut ret: Vec<u8> = data.bytes().take_while(|&x| x == BASE58_CHARS[0])
                                       .map(|_| 0)
                                       .collect();
    // Copy rest of string
    ret.extend(scratch.into_iter().skip_while(|&x| x == 0));
    Ok(ret)
}

/// Decode a base58check-encoded string
pub fn from_check(data: &str) -> Result<Vec<u8>, Error> {
    let mut ret: Vec<u8> = from(data)?;
    if ret.len() < 4 {
        return Err(Error::TooShort(ret.len()));
    }
    let ck_start = ret.len() - 4;
    let expected = endian::slice_to_u32_le(&sha256d::Hash::hash(&ret[..ck_start])[..4]);
    let actual = endian::slice_to_u32_le(&ret[ck_start..(ck_start + 4)]);
    if expected != actual {
        return Err(Error::BadChecksum(expected, actual));
    }

    ret.truncate(ck_start);
    Ok(ret)
}

fn format_iter<I, W>(writer: &mut W, data: I) -> Result<(), fmt::Error>
where
    I: Iterator<Item = u8> + Clone,
    W: fmt::Write
{
    let mut ret = SmallVec::new();

    let mut leading_zero_count = 0;
    let mut leading_zeroes = true;
    // Build string in little endian with 0-58 in place of characters...
    for d256 in data {
        let mut carry = d256 as usize;
        if leading_zeroes && carry == 0 {
            leading_zero_count += 1;
        } else {
            leading_zeroes = false;
        }

        for ch in ret.iter_mut() {
            let new_ch = *ch as usize * 256 + carry;
            *ch = (new_ch % 58) as u8;
            carry = new_ch / 58;
        }
        while carry > 0 {
            ret.push((carry % 58) as u8);
            carry /= 58;
        }
    }

    // ... then reverse it and convert to chars
    for _ in 0..leading_zero_count {
        ret.push(0);
    }

    for ch in ret.iter().rev() {
        writer.write_char(BASE58_CHARS[*ch as usize] as char)?;
    }

    Ok(())
}

fn encode_iter<I>(data: I) -> String
where
    I: Iterator<Item = u8> + Clone,
{
    let mut ret = String::new();
    format_iter(&mut ret, data).expect("writing into string shouldn't fail");
    ret
}


/// Directly encode a slice as base58
pub fn encode_slice(data: &[u8]) -> String {
    encode_iter(data.iter().cloned())
}

/// Obtain a string with the base58check encoding of a slice
/// (Tack the first 4 256-digits of the object's Bitcoin hash onto the end.)
pub fn check_encode_slice(data: &[u8]) -> String {
    let checksum = sha256d::Hash::hash(&data);
    encode_iter(
        data.iter()
            .cloned()
            .chain(checksum[0..4].iter().cloned())
    )
}

/// Obtain a string with the base58check encoding of a slice
/// (Tack the first 4 256-digits of the object's Bitcoin hash onto the end.)
pub fn check_encode_slice_to_fmt(fmt: &mut fmt::Formatter, data: &[u8]) -> fmt::Result {
    let checksum = sha256d::Hash::hash(&data);
    let iter = data.iter()
        .cloned()
        .chain(checksum[0..4].iter().cloned());
    format_iter(fmt, iter)
}

#[cfg(test)]
mod tests {
    use super::*;
    use hex::decode as hex_decode;

    #[test]
    fn test_base58_encode() {
        // Basics
        assert_eq!(&encode_slice(&[0][..]), "1");
        assert_eq!(&encode_slice(&[1][..]), "2");
        assert_eq!(&encode_slice(&[58][..]), "21");
        assert_eq!(&encode_slice(&[13, 36][..]), "211");

        // Leading zeroes
        assert_eq!(&encode_slice(&[0, 13, 36][..]), "1211");
        assert_eq!(&encode_slice(&[0, 0, 0, 0, 13, 36][..]), "1111211");

        // Long input (>100 bytes => has to use heap)
        let res = encode_slice(&"BitcoinBitcoinBitcoinBitcoinBitcoinBitcoinBitcoinBitcoinBitcoinBit\
        coinBitcoinBitcoinBitcoinBitcoinBitcoinBitcoinBitcoinBitcoinBitcoinBitcoin".as_bytes());
        let exp = "ZqC5ZdfpZRi7fjA8hbhX5pEE96MdH9hEaC1YouxscPtbJF16qVWksHWR4wwvx7MotFcs2ChbJqK8KJ9X\
        wZznwWn1JFDhhTmGo9v6GjAVikzCsBWZehu7bm22xL8b5zBR5AsBygYRwbFJsNwNkjpyFuDKwmsUTKvkULCvucPJrN5\
        QUdxpGakhqkZFL7RU4yT";
        assert_eq!(&res, exp);

        // Addresses
        let addr = hex_decode("00f8917303bfa8ef24f292e8fa1419b20460ba064d").unwrap();
        assert_eq!(&check_encode_slice(&addr[..]), "1PfJpZsjreyVrqeoAfabrRwwjQyoSQMmHH");
      }

      #[test]
      fn test_base58_decode() {
        // Basics
        assert_eq!(from("1").ok(), Some(vec![0u8]));
        assert_eq!(from("2").ok(), Some(vec![1u8]));
        assert_eq!(from("21").ok(), Some(vec![58u8]));
        assert_eq!(from("211").ok(), Some(vec![13u8, 36]));

        // Leading zeroes
        assert_eq!(from("1211").ok(), Some(vec![0u8, 13, 36]));
        assert_eq!(from("111211").ok(), Some(vec![0u8, 0, 0, 13, 36]));

        // Addresses
        assert_eq!(from_check("1PfJpZsjreyVrqeoAfabrRwwjQyoSQMmHH").ok(),
                   Some(hex_decode("00f8917303bfa8ef24f292e8fa1419b20460ba064d").unwrap()))
    }

    #[test]
    fn test_base58_roundtrip() {
        let s = "xprv9wTYmMFdV23N2TdNG573QoEsfRrWKQgWeibmLntzniatZvR9BmLnvSxqu53Kw1UmYPxLgboyZQaXwTCg8MSY3H2EU4pWcQDnRnrVA1xe8fs";
        let v: Vec<u8> = from_check(s).unwrap();
        assert_eq!(check_encode_slice(&v[..]), s);
        assert_eq!(from_check(&check_encode_slice(&v[..])).ok(), Some(v));
    }
}

