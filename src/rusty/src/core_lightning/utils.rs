use bitcoin::secp256k1::PublicKey;

pub fn hex_to_vec(hex: &str) -> Option<Vec<u8>> {
    let mut out = Vec::with_capacity(hex.len() / 2);

    let mut b = 0;
    for (idx, c) in hex.as_bytes().iter().enumerate() {
        b <<= 4;
        match *c {
            b'A'...b'F' => b |= c - b'A' + 10,
            b'a'...b'f' => b |= c - b'a' + 10,
            b'0'...b'9' => b |= c - b'0',
            _ => return None,
        }
        if (idx & 1) == 1 {
            out.push(b);
            b = 0;
        }
    }

    Some(out)
}

pub fn hex_to_compressed_pubkey(hex: &str) -> Option<PublicKey> {
    let data = match hex_to_vec(&hex[0..33 * 2]) {
        Some(bytes) => bytes,
        None => return None,
    };
    match PublicKey::from_slice(&data) {
        Ok(pk) => Some(pk),
        Err(_) => None,
    }
}

#[inline]
pub fn hex_str(value: &[u8]) -> String {
    let mut res = String::with_capacity(64);
    for v in value {
        res += &format!("{:02x}", v);
    }
    res
}

#[inline]
pub fn slice_to_be64(v: &[u8]) -> u64 {
    ((v[0] as u64) << 8*7) |
        ((v[1] as u64) << 8*6) |
        ((v[2] as u64) << 8*5) |
        ((v[3] as u64) << 8*4) |
        ((v[4] as u64) << 8*3) |
        ((v[5] as u64) << 8*2) |
        ((v[6] as u64) << 8*1) |
        ((v[7] as u64) << 8*0)
}