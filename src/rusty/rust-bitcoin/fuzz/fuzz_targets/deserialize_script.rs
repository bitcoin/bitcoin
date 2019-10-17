extern crate bitcoin;

use bitcoin::util::address::Address;
use bitcoin::network::constants::Network;
use bitcoin::blockdata::script;
use bitcoin::consensus::encode;

fn do_test(data: &[u8]) {
    let s: Result<script::Script, _> = encode::deserialize(data);
    if let Ok(script) = s {
        let _: Vec<script::Instruction> = script.iter(false).collect();
        let enforce_min: Vec<script::Instruction> = script.iter(true).collect();

        let mut b = script::Builder::new();
        for ins in enforce_min {
            match ins {
                script::Instruction::Error(_) => return,
                script::Instruction::Op(op) => { b = b.push_opcode(op); }
                script::Instruction::PushBytes(bytes) => {
                    // Any one-byte pushes, except -0, which can be interpreted as numbers, should be
                    // reserialized as numbers. (For -1 through 16, this will use special ops; for
                    // others it'll just reserialize them as pushes.)
                    if bytes.len() == 1 && bytes[0] != 0x80 && bytes[0] != 0x00 {
                        if let Ok(num) = script::read_scriptint(bytes) {
                            b = b.push_int(num);
                        } else {
                            b = b.push_slice(bytes);
                        }
                    } else {
                        b = b.push_slice(bytes);
                    }
                }
            }
        }
        assert_eq!(b.into_script(), script);
        assert_eq!(data, &encode::serialize(&script)[..]);

        // Check if valid address and if that address roundtrips.
        if let Some(addr) = Address::from_script(&script, Network::Bitcoin) {
            assert_eq!(addr.script_pubkey(), script);
        }
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
        extend_vec_from_hex("00", &mut a);
        super::do_test(&a);
    }
}
