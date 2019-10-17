extern crate bitcoin;

fn do_test(data: &[u8]) {
    let tx_result: Result<bitcoin::blockdata::transaction::Transaction, _> = bitcoin::consensus::encode::deserialize(data);
    match tx_result {
        Err(_) => {},
        Ok(mut tx) => {
            let ser = bitcoin::consensus::encode::serialize(&tx);
            assert_eq!(&ser[..], data);
            let len = ser.len();
            let calculated_weight = tx.get_weight();
            for input in &mut tx.input {
                input.witness = vec![];
            }
            let no_witness_len = bitcoin::consensus::encode::serialize(&tx).len();
            // For 0-input transactions, `no_witness_len` will be incorrect because
            // we serialize as segwit even after "stripping the witnesses". We need
            // to drop two bytes (i.e. eight weight)
            if tx.input.is_empty() {
                assert_eq!(no_witness_len * 3 + len - 8, calculated_weight);
            } else {
                assert_eq!(no_witness_len * 3 + len, calculated_weight);
            }
        },
    }
}

#[cfg(feature = "afl")]
#[macro_use] extern crate afl;
#[cfg(feature = "afl")]
fn main() {
    fuzz!(|data| {
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
        for (idx, c) in hex.as_bytes().iter().enumerate() {
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
        extend_vec_from_hex("000700000001000000010000", &mut a);
        super::do_test(&a);
    }
}
