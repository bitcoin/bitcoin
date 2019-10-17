// Rust Bitcoin Library
// Written in 2018 by
//     Andrew Poelstra <apoelstra@wpsoftware.net>
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to
// the public domain worldwide. This software is distributed without
// any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication
// along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
//

//! BIP143 Implementation
//!
//! Implementation of BIP143 Segwit-style signatures. Should be sufficient
//! to create signatures for Segwit transactions (which should be pushed into
//! the appropriate place in the `Transaction::witness` array) or bcash
//! signatures, which are placed in the scriptSig.
//!

use hashes::{sha256d, Hash};

use blockdata::script::Script;
use blockdata::transaction::{Transaction, TxIn};
use consensus::encode::Encodable;

/// Parts of a sighash which are common across inputs or signatures, and which are
/// sufficient (in conjunction with a private key) to sign the transaction
#[derive(Clone, PartialEq, Eq, Debug)]
pub struct SighashComponents {
    tx_version: u32,
    tx_locktime: u32,
    /// Hash of all the previous outputs
    pub hash_prevouts: sha256d::Hash,
    /// Hash of all the input sequence nos
    pub hash_sequence: sha256d::Hash,
    /// Hash of all the outputs in this transaction
    pub hash_outputs: sha256d::Hash,
}

impl SighashComponents {
    /// Compute the sighash components from an unsigned transaction and auxiliary
    /// information about its inputs.
    /// For the generated sighashes to be valid, no fields in the transaction may change except for
    /// script_sig and witnesses.
    pub fn new(tx: &Transaction) -> SighashComponents {
        let hash_prevouts = {
            let mut enc = sha256d::Hash::engine();
            for txin in &tx.input {
                txin.previous_output.consensus_encode(&mut enc).unwrap();
            }
            sha256d::Hash::from_engine(enc)
        };

        let hash_sequence = {
            let mut enc = sha256d::Hash::engine();
            for txin in &tx.input {
                txin.sequence.consensus_encode(&mut enc).unwrap();
            }
            sha256d::Hash::from_engine(enc)
        };

        let hash_outputs = {
            let mut enc = sha256d::Hash::engine();
            for txout in &tx.output {
                txout.consensus_encode(&mut enc).unwrap();
            }
            sha256d::Hash::from_engine(enc)
        };

        SighashComponents {
            tx_version: tx.version,
            tx_locktime: tx.lock_time,
            hash_prevouts: hash_prevouts,
            hash_sequence: hash_sequence,
            hash_outputs: hash_outputs,
        }
    }

    /// Compute the BIP143 sighash for a `SIGHASH_ALL` signature for the given
    /// input.
    pub fn sighash_all(&self, txin: &TxIn, script_code: &Script, value: u64) -> sha256d::Hash {
        let mut enc = sha256d::Hash::engine();
        self.tx_version.consensus_encode(&mut enc).unwrap();
        self.hash_prevouts.consensus_encode(&mut enc).unwrap();
        self.hash_sequence.consensus_encode(&mut enc).unwrap();
        txin
            .previous_output
            .consensus_encode(&mut enc)
            .unwrap();
        script_code.consensus_encode(&mut enc).unwrap();
        value.consensus_encode(&mut enc).unwrap();
        txin.sequence.consensus_encode(&mut enc).unwrap();
        self.hash_outputs.consensus_encode(&mut enc).unwrap();
        self.tx_locktime.consensus_encode(&mut enc).unwrap();
        1u32.consensus_encode(&mut enc).unwrap(); // hashtype
        sha256d::Hash::from_engine(enc)
    }
}

#[cfg(test)]
mod tests {
    use blockdata::script::Script;
    use blockdata::transaction::Transaction;
    use consensus::encode::deserialize;
    use network::constants::Network;
    use util::misc::hex_bytes;
    use util::address::Address;
    use util::key::PublicKey;
    use hex;

    use super::*;

    fn p2pkh_hex(pk: &str) -> Script {
        let pk = hex::decode(pk).unwrap();
        let pk = PublicKey::from_slice(pk.as_slice()).unwrap();
        let witness_script = Address::p2pkh(&pk, Network::Bitcoin).script_pubkey();
        witness_script
    }

    #[test]
    fn bip143_p2wpkh() {
        let tx = deserialize::<Transaction>(
            &hex_bytes(
                "0100000002fff7f7881a8099afa6940d42d1e7f6362bec38171ea3edf433541db4e4ad969f000000\
                0000eeffffffef51e1b804cc89d182d279655c3aa89e815b1b309fe287d9b2b55d57b90ec68a01000000\
                00ffffffff02202cb206000000001976a9148280b37df378db99f66f85c95a783a76ac7a6d5988ac9093\
                510d000000001976a9143bde42dbee7e4dbe6a21b2d50ce2f0167faa815988ac11000000",
            ).unwrap()[..],
        ).unwrap();

        let witness_script = p2pkh_hex("025476c2e83188368da1ff3e292e7acafcdb3566bb0ad253f62fc70f07aeee6357");
        let value = 600_000_000;

        let comp = SighashComponents::new(&tx);
        assert_eq!(
            comp,
            SighashComponents {
                tx_version: 1,
                tx_locktime: 17,
                hash_prevouts: hex_hash!(
                    "96b827c8483d4e9b96712b6713a7b68d6e8003a781feba36c31143470b4efd37"
                ),
                hash_sequence: hex_hash!(
                    "52b0a642eea2fb7ae638c36f6252b6750293dbe574a806984b8e4d8548339a3b"
                ),
                hash_outputs: hex_hash!(
                    "863ef3e1a92afbfdb97f31ad0fc7683ee943e9abcf2501590ff8f6551f47e5e5"
                ),
            }
        );

        assert_eq!(
            comp.sighash_all(&tx.input[1], &witness_script, value),
            hex_hash!("c37af31116d1b27caf68aae9e3ac82f1477929014d5b917657d0eb49478cb670")
        );
    }

    #[test]
    fn bip143_p2wpkh_nested_in_p2sh() {
        let tx = deserialize::<Transaction>(
            &hex_bytes(
                "0100000001db6b1b20aa0fd7b23880be2ecbd4a98130974cf4748fb66092ac4d3ceb1a5477010000\
                0000feffffff02b8b4eb0b000000001976a914a457b684d7f0d539a46a45bbc043f35b59d0d96388ac00\
                08af2f000000001976a914fd270b1ee6abcaea97fea7ad0402e8bd8ad6d77c88ac92040000",
            ).unwrap()[..],
        ).unwrap();

        let witness_script = p2pkh_hex("03ad1d8e89212f0b92c74d23bb710c00662ad1470198ac48c43f7d6f93a2a26873");
        let value = 1_000_000_000;

        let comp = SighashComponents::new(&tx);
        assert_eq!(
            comp,
            SighashComponents {
                tx_version: 1,
                tx_locktime: 1170,
                hash_prevouts: hex_hash!(
                    "b0287b4a252ac05af83d2dcef00ba313af78a3e9c329afa216eb3aa2a7b4613a"
                ),
                hash_sequence: hex_hash!(
                    "18606b350cd8bf565266bc352f0caddcf01e8fa789dd8a15386327cf8cabe198"
                ),
                hash_outputs: hex_hash!(
                    "de984f44532e2173ca0d64314fcefe6d30da6f8cf27bafa706da61df8a226c83"
                ),
            }
        );

        assert_eq!(
            comp.sighash_all(&tx.input[0], &witness_script, value),
            hex_hash!("64f3b0f4dd2bb3aa1ce8566d220cc74dda9df97d8490cc81d89d735c92e59fb6")
        );
    }

    #[test]
    fn bip143_p2wsh_nested_in_p2sh() {
        let tx = deserialize::<Transaction>(
            &hex_bytes(
            "010000000136641869ca081e70f394c6948e8af409e18b619df2ed74aa106c1ca29787b96e0100000000\
             ffffffff0200e9a435000000001976a914389ffce9cd9ae88dcc0631e88a821ffdbe9bfe2688acc0832f\
             05000000001976a9147480a33f950689af511e6e84c138dbbd3c3ee41588ac00000000").unwrap()[..],
        ).unwrap();

        let witness_script = hex_script!(
            "56210307b8ae49ac90a048e9b53357a2354b3334e9c8bee813ecb98e99a7e07e8c3ba32103b28f0c28\
             bfab54554ae8c658ac5c3e0ce6e79ad336331f78c428dd43eea8449b21034b8113d703413d57761b8b\
             9781957b8c0ac1dfe69f492580ca4195f50376ba4a21033400f6afecb833092a9a21cfdf1ed1376e58\
             c5d1f47de74683123987e967a8f42103a6d48b1131e94ba04d9737d61acdaa1322008af9602b3b1486\
             2c07a1789aac162102d8b661b0b3302ee2f162b09e07a55ad5dfbe673a9f01d9f0c19617681024306b\
             56ae"
        );
        let value = 987654321;

        let comp = SighashComponents::new(&tx);
        assert_eq!(
            comp,
            SighashComponents {
                tx_version: 1,
                tx_locktime: 0,
                hash_prevouts: hex_hash!(
                    "74afdc312af5183c4198a40ca3c1a275b485496dd3929bca388c4b5e31f7aaa0"
                ),
                hash_sequence: hex_hash!(
                    "3bb13029ce7b1f559ef5e747fcac439f1455a2ec7c5f09b72290795e70665044"
                ),
                hash_outputs: hex_hash!(
                    "bc4d309071414bed932f98832b27b4d76dad7e6c1346f487a8fdbb8eb90307cc"
                ),
            }
        );

        assert_eq!(
            comp.sighash_all(&tx.input[0], &witness_script, value),
            hex_hash!("185c0be5263dce5b4bb50a047973c1b6272bfbd0103a89444597dc40b248ee7c")
        );
    }
}
