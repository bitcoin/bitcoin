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

//! Miscellaneous functions
//!
//! Various utility functions

use hashes::{sha256d, Hash};
use blockdata::opcodes;
use consensus::encode;

static MSG_SIGN_PREFIX: &'static [u8] = b"\x18Bitcoin Signed Message:\n";

/// Helper function to convert hex nibble characters to their respective value
#[inline]
fn hex_val(c: u8) -> Result<u8, encode::Error> {
    let res = match c {
        b'0' ... b'9' => c - '0' as u8,
        b'a' ... b'f' => c - 'a' as u8 + 10,
        b'A' ... b'F' => c - 'A' as u8 + 10,
        _ => return Err(encode::Error::UnexpectedHexDigit(c as char)),
    };
    Ok(res)
}

/// Convert a hexadecimal-encoded string to its corresponding bytes
pub fn hex_bytes(data: &str) -> Result<Vec<u8>, encode::Error> {
    // This code is optimized to be as fast as possible without using unsafe or platform specific
    // features. If you want to refactor it please make sure you don't introduce performance
    // regressions (run the benchmark with `cargo bench --features unstable`).

    // If the hex string has an uneven length fail early
    if data.len() % 2 != 0 {
        return Err(encode::Error::ParseFailed("hexstring of odd length"));
    }

    // Preallocate the uninitialized memory for the byte array
    let mut res = Vec::with_capacity(data.len() / 2);

    let mut hex_it = data.bytes();
    loop {
        // Get most significant nibble of current byte or end iteration
        let msn = match hex_it.next() {
            None => break,
            Some(x) => x,
        };

        // Get least significant nibble of current byte
        let lsn = match hex_it.next() {
            None => unreachable!("len % 2 == 0"),
            Some(x) => x,
        };

        // Convert bytes representing characters to their represented value and combine lsn and msn.
        // The and_then and map are crucial for performance, in comparison to using ? and then
        // using the results of that for the calculation it's nearly twice as fast. Using bit
        // shifting and or instead of multiply and add on the other hand doesn't show a significant
        // increase in performance.
        match hex_val(msn).and_then(|msn_val| hex_val(lsn).map(|lsn_val| msn_val * 16 + lsn_val)) {
            Ok(x) => res.push(x),
            Err(e) => return Err(e),
        }
    }
    Ok(res)
}

/// Search for `needle` in the vector `haystack` and remove every
/// instance of it, returning the number of instances removed.
/// Loops through the vector opcode by opcode, skipping pushed data.
pub fn script_find_and_remove(haystack: &mut Vec<u8>, needle: &[u8]) -> usize {
    if needle.len() > haystack.len() { return 0; }
    if needle.len() == 0 { return 0; }

    let mut top = haystack.len() - needle.len();
    let mut n_deleted = 0;

    let mut i = 0;
    while i <= top {
        if &haystack[i..(i + needle.len())] == needle {
            for j in i..top {
                haystack.swap(j + needle.len(), j);
            }
            n_deleted += 1;
            // This is ugly but prevents infinite loop in case of overflow
            let overflow = top < needle.len();
            top = top.wrapping_sub(needle.len());
            if overflow { break; }
        } else {
            i += match opcodes::All::from((*haystack)[i]).classify() {
                opcodes::Class::PushBytes(n) => n as usize + 1,
                opcodes::Class::Ordinary(opcodes::Ordinary::OP_PUSHDATA1) => 2,
                opcodes::Class::Ordinary(opcodes::Ordinary::OP_PUSHDATA2) => 3,
                opcodes::Class::Ordinary(opcodes::Ordinary::OP_PUSHDATA4) => 5,
                _ => 1
            };
        }
    }
    haystack.truncate(top.wrapping_add(needle.len()));
    n_deleted
}

/// Hash message for signature using Bitcoin's message signing format
pub fn signed_msg_hash(msg: &str) -> sha256d::Hash {
    sha256d::Hash::hash(
        &[
            MSG_SIGN_PREFIX,
            &encode::serialize(&encode::VarInt(msg.len() as u64)),
            msg.as_bytes(),
        ]
        .concat(),
    )
}

#[cfg(all(test, feature="unstable"))]
mod benches {
    use secp256k1::rand::{Rng, thread_rng};
    use secp256k1::rand::distributions::Standard;
    use super::hex_bytes;
    use test::Bencher;

    fn join<I: Iterator<Item=IT>, IT: AsRef<str>>(iter: I, expected_len: usize) -> String {
        let mut res = String::with_capacity(expected_len);
        for s in iter {
            res.push_str(s.as_ref());
        }
        res
    }

    fn bench_from_hex(b: &mut Bencher, data_size: usize) {
        let data_bytes = thread_rng()
            .sample_iter(&Standard)
            .take(data_size)
            .collect::<Vec<u8>>();
        let data = join(data_bytes.iter().map(|x| format!("{:02x}", x)), data_size * 2);

        assert_eq!(hex_bytes(&data).unwrap(), data_bytes);

        b.iter(|| {
            hex_bytes(&data).unwrap()
        })
    }

    #[bench]
    fn from_hex_16_bytes(b: &mut Bencher) {
        bench_from_hex(b, 16);
    }

    #[bench]
    fn from_hex_64_bytes(b: &mut Bencher) {
        bench_from_hex(b, 64);
    }

    #[bench]
    fn from_hex_256_bytes(b: &mut Bencher) {
        bench_from_hex(b, 256);
    }

    #[bench]
    fn from_hex_4m_bytes(b: &mut Bencher) {
        bench_from_hex(b, 1024 * 1024 * 4);
    }
}

#[cfg(test)]
mod tests {
    use hashes::hex::ToHex;
    use super::script_find_and_remove;
    use super::hex_bytes;
    use super::signed_msg_hash;

    #[test]
    fn test_script_find_and_remove() {
        let mut v = vec![101u8, 102, 103, 104, 102, 103, 104, 102, 103, 104, 105, 106, 107, 108, 109];

        assert_eq!(script_find_and_remove(&mut v, &[]), 0);
        assert_eq!(script_find_and_remove(&mut v, &[105, 105, 105]), 0);
        assert_eq!(v, vec![101, 102, 103, 104, 102, 103, 104, 102, 103, 104, 105, 106, 107, 108, 109]);

        assert_eq!(script_find_and_remove(&mut v, &[105, 106, 107]), 1);
        assert_eq!(v, vec![101, 102, 103, 104, 102, 103, 104, 102, 103, 104, 108, 109]);

        assert_eq!(script_find_and_remove(&mut v, &[104, 108, 109]), 1);
        assert_eq!(v, vec![101, 102, 103, 104, 102, 103, 104, 102, 103]);

        assert_eq!(script_find_and_remove(&mut v, &[101]), 1);
        assert_eq!(v, vec![102, 103, 104, 102, 103, 104, 102, 103]);

        assert_eq!(script_find_and_remove(&mut v, &[102]), 3);
        assert_eq!(v, vec![103, 104, 103, 104, 103]);

        assert_eq!(script_find_and_remove(&mut v, &[103, 104]), 2);
        assert_eq!(v, vec![103]);

        assert_eq!(script_find_and_remove(&mut v, &[105, 105, 5]), 0);
        assert_eq!(script_find_and_remove(&mut v, &[105]), 0);
        assert_eq!(script_find_and_remove(&mut v, &[103]), 1);
        assert_eq!(v, Vec::<u8>::new());

        assert_eq!(script_find_and_remove(&mut v, &[105, 105, 5]), 0);
        assert_eq!(script_find_and_remove(&mut v, &[105]), 0);
    }

    #[test]
    fn test_script_codesep_remove() {
        let mut s = vec![33u8, 3, 132, 121, 160, 250, 153, 140, 211, 82, 89, 162, 239, 10, 122, 92, 104, 102, 44, 20, 116, 248, 140, 203, 109, 8, 167, 103, 123, 190, 199, 242, 32, 65, 173, 171, 33, 3, 132, 121, 160, 250, 153, 140, 211, 82, 89, 162, 239, 10, 122, 92, 104, 102, 44, 20, 116, 248, 140, 203, 109, 8, 167, 103, 123, 190, 199, 242, 32, 65, 173, 171, 81];
        assert_eq!(script_find_and_remove(&mut s, &[171]), 2);
        assert_eq!(s, vec![33, 3, 132, 121, 160, 250, 153, 140, 211, 82, 89, 162, 239, 10, 122, 92, 104, 102, 44, 20, 116, 248, 140, 203, 109, 8, 167, 103, 123, 190, 199, 242, 32, 65, 173, 33, 3, 132, 121, 160, 250, 153, 140, 211, 82, 89, 162, 239, 10, 122, 92, 104, 102, 44, 20, 116, 248, 140, 203, 109, 8, 167, 103, 123, 190, 199, 242, 32, 65, 173, 81]);
    }

    #[test]
    fn test_hex_bytes() {
        assert_eq!(&hex_bytes("abcd").unwrap(), &[171u8, 205]);
        assert!(hex_bytes("abcde").is_err());
        assert!(hex_bytes("aBcDeF").is_ok());
        assert!(hex_bytes("aBcD4eFL").is_err());
    }

    #[test]
    fn test_signed_msg_hash() {
        let hash = signed_msg_hash("test");
        assert_eq!(hash.to_hex(), "a6f87fe6d58a032c320ff8d1541656f0282c2c7bfcc69d61af4c8e8ed528e49c");
    }
}

