extern crate bech32;

use std::str::FromStr;

fn do_test(data: &[u8]) {
    if data.len() < 1 {
        return;
    }

    let hrp_end = (data[0] as usize) + 1;

    if data.len() < hrp_end {
        return;
    }

    let hrp = String::from_utf8_lossy(&data[1..hrp_end]).to_lowercase().to_string();

    let dp = data[hrp_end..]
        .iter()
        .map(|b| bech32::u5::try_from_u8(b % 32).unwrap())
        .collect::<Vec<_>>();

    if let Ok(data_str) = bech32::encode(&hrp, &dp).map(|b32| b32.to_string()) {
        let decoded = bech32::decode(&data_str);
        let b32 = decoded.expect("should be able to decode own encoding");

        assert_eq!(bech32::encode(&b32.0, &b32.1).unwrap(), data_str);
    }
}

#[cfg(feature = "afl")]
extern crate afl;
#[cfg(feature = "afl")]
fn main() {
    afl::read_stdio_bytes(|data| {
        do_test(&data);
    });
}

#[cfg(feature = "honggfuzz")]
#[macro_use] extern crate honggfuzz;
#[cfg(feature = "honggfuzz")]
fn main() {
    loop {
        fuzz!(|data| {
            do_test(data);
        });
    }
}

#[cfg(test)]
mod tests {
    fn extend_vec_from_hex(hex: &str, out: &mut Vec<u8>) {
        let mut b = 0;
        for (idx, c) in hex.as_bytes().iter().filter(|&&c| c != b'\n').enumerate() {
            b <<= 4;
            match *c {
                b'A'...b'F' => b |= c - b'A' + 10,
                b'a'...b'f' => b |= c - b'a' + 10,
                b'0'...b'9' => b |= c - b'0',
                _ => panic!("Bad hex"),
            }
            if (idx & 1) == 1 {
                out.push(b);
                b = 0;
            }
        }
    }

    #[test]
    fn duplicate_crash() {
        let mut a = Vec::new();
        extend_vec_from_hex("ff6c2d", &mut a);
        super::do_test(&a);
    }
}
