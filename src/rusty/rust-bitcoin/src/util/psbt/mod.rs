// Rust Bitcoin Library
// Written by
//   The Rust Bitcoin developers
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

//! # Partially Signed Transactions
//!
//! Implementation of BIP174 Partially Signed Bitcoin Transaction Format as
//! defined at https://github.com/bitcoin/bips/blob/master/bip-0174.mediawiki
//! except we define PSBTs containing non-standard SigHash types as invalid.

use blockdata::script::Script;
use blockdata::transaction::Transaction;
use consensus::{encode, Encodable, Decodable};

use std::io;

mod error;
pub use self::error::Error;

pub mod raw;

#[macro_use]
mod macros;

pub mod serialize;

mod map;
pub use self::map::{Map, Global, Input, Output};

/// A Partially Signed Transaction.
#[derive(Debug, Clone, PartialEq)]
pub struct PartiallySignedTransaction {
    /// The key-value pairs for all global data.
    pub global: Global,
    /// The corresponding key-value map for each input in the unsigned
    /// transaction.
    pub inputs: Vec<Input>,
    /// The corresponding key-value map for each output in the unsigned
    /// transaction.
    pub outputs: Vec<Output>,
}

impl PartiallySignedTransaction {
    /// Create a PartiallySignedTransaction from an unsigned transaction, error
    /// if not unsigned
    pub fn from_unsigned_tx(tx: Transaction) -> Result<Self, encode::Error> {
        Ok(PartiallySignedTransaction {
            inputs: vec![Default::default(); tx.input.len()],
            outputs: vec![Default::default(); tx.output.len()],
            global: Global::from_unsigned_tx(tx)?,
        })
    }

    /// Extract the Transaction from a PartiallySignedTransaction by filling in
    /// the available signature information in place.
    pub fn extract_tx(self) -> Transaction {
        let mut tx: Transaction = self.global.unsigned_tx;

        for (vin, psbtin) in tx.input.iter_mut().zip(self.inputs.into_iter()) {
            vin.script_sig = psbtin.final_script_sig.unwrap_or_else(|| Script::new());
            vin.witness = psbtin.final_script_witness.unwrap_or_else(|| Vec::new());
        }

        tx
    }

    /// Attempt to merge with another `PartiallySignedTransaction`.
    pub fn merge(&mut self, other: Self) -> Result<(), self::Error> {
        self.global.merge(other.global)?;

        for (self_input, other_input) in self.inputs.iter_mut().zip(other.inputs.into_iter()) {
            self_input.merge(other_input)?;
        }

        for (self_output, other_output) in self.outputs.iter_mut().zip(other.outputs.into_iter()) {
            self_output.merge(other_output)?;
        }

        Ok(())
    }
}

impl Encodable for PartiallySignedTransaction {
    fn consensus_encode<S: io::Write>(
        &self,
        mut s: S,
    ) -> Result<usize, encode::Error> {
        let mut len = 0;
        len += b"psbt".consensus_encode(&mut s)?;

        len += 0xff_u8.consensus_encode(&mut s)?;

        len += self.global.consensus_encode(&mut s)?;

        for i in &self.inputs {
            len += i.consensus_encode(&mut s)?;
        }

        for i in &self.outputs {
            len += i.consensus_encode(&mut s)?;
        }

        Ok(len)
    }
}

impl Decodable for PartiallySignedTransaction {
    fn consensus_decode<D: io::Read>(mut d: D) -> Result<Self, encode::Error> {
        let magic: [u8; 4] = Decodable::consensus_decode(&mut d)?;

        if *b"psbt" != magic {
            return Err(Error::InvalidMagic.into());
        }

        if 0xff_u8 != u8::consensus_decode(&mut d)? {
            return Err(Error::InvalidSeparator.into());
        }

        let global: Global = Decodable::consensus_decode(&mut d)?;

        let inputs: Vec<Input> = {
            let inputs_len: usize = (&global.unsigned_tx.input).len();

            let mut inputs: Vec<Input> = Vec::with_capacity(inputs_len);

            for _ in 0..inputs_len {
                inputs.push(Decodable::consensus_decode(&mut d)?);
            }

            inputs
        };

        let outputs: Vec<Output> = {
            let outputs_len: usize = (&global.unsigned_tx.output).len();

            let mut outputs: Vec<Output> = Vec::with_capacity(outputs_len);

            for _ in 0..outputs_len {
                outputs.push(Decodable::consensus_decode(&mut d)?);
            }

            outputs
        };

        Ok(PartiallySignedTransaction {
            global: global,
            inputs: inputs,
            outputs: outputs,
        })
    }
}

#[cfg(test)]
mod tests {
    use hashes::hex::FromHex;
    use hashes::sha256d;

    use std::collections::BTreeMap;

    use hex::decode as hex_decode;

    use secp256k1::Secp256k1;

    use blockdata::script::Script;
    use blockdata::transaction::{Transaction, TxIn, TxOut, OutPoint};
    use network::constants::Network::Bitcoin;
    use consensus::encode::{deserialize, serialize, serialize_hex};
    use util::bip32::{ChildNumber, DerivationPath, ExtendedPrivKey, ExtendedPubKey, Fingerprint};
    use util::key::PublicKey;
    use util::psbt::map::{Global, Output};
    use util::psbt::raw;

    use super::PartiallySignedTransaction;

    #[test]
    fn trivial_psbt() {
        let psbt = PartiallySignedTransaction {
            global: Global {
                unsigned_tx: Transaction {
                    version: 2,
                    lock_time: 0,
                    input: vec![],
                    output: vec![],
                },
                unknown: BTreeMap::new(),
            },
            inputs: vec![],
            outputs: vec![],
        };
        assert_eq!(
            serialize_hex(&psbt),
            "70736274ff01000a0200000000000000000000"
        );
    }

    #[test]
    fn serialize_then_deserialize_output() {
        let secp = &Secp256k1::new();
        let seed = hex_decode("000102030405060708090a0b0c0d0e0f").unwrap();

        let mut hd_keypaths: BTreeMap<PublicKey, (Fingerprint, DerivationPath)> = Default::default();

        let mut sk: ExtendedPrivKey = ExtendedPrivKey::new_master(Bitcoin, &seed).unwrap();

        let fprint: Fingerprint = sk.fingerprint(&secp);

        let dpath: Vec<ChildNumber> = vec![
            ChildNumber::from_normal_idx(0).unwrap(),
            ChildNumber::from_normal_idx(1).unwrap(),
            ChildNumber::from_normal_idx(2).unwrap(),
            ChildNumber::from_normal_idx(4).unwrap(),
            ChildNumber::from_normal_idx(42).unwrap(),
            ChildNumber::from_hardened_idx(69).unwrap(),
            ChildNumber::from_normal_idx(420).unwrap(),
            ChildNumber::from_normal_idx(31337).unwrap(),
        ];

        sk = sk.derive_priv(secp, &dpath).unwrap();

        let pk: ExtendedPubKey = ExtendedPubKey::from_private(&secp, &sk);

        hd_keypaths.insert(pk.public_key, (fprint, dpath.into()));

        let expected: Output = Output {
            redeem_script: Some(hex_script!(
                "76a914d0c59903c5bac2868760e90fd521a4665aa7652088ac"
            )),
            witness_script: Some(hex_script!(
                "a9143545e6e33b832c47050f24d3eeb93c9c03948bc787"
            )),
            hd_keypaths: hd_keypaths,
            ..Default::default()
        };

        let actual: Output = deserialize(&serialize(&expected)).unwrap();

        assert_eq!(expected, actual);
    }

    #[test]
    fn serialize_then_deserialize_global() {
        let expected = Global {
            unsigned_tx: Transaction {
                version: 2,
                lock_time: 1257139,
                input: vec![TxIn {
                    previous_output: OutPoint {
                        txid: sha256d::Hash::from_hex(
                            "f61b1742ca13176464adb3cb66050c00787bb3a4eead37e985f2df1e37718126",
                        ).unwrap(),
                        vout: 0,
                    },
                    script_sig: Script::new(),
                    sequence: 4294967294,
                    witness: vec![],
                }],
                output: vec![
                    TxOut {
                        value: 99999699,
                        script_pubkey: hex_script!(
                            "76a914d0c59903c5bac2868760e90fd521a4665aa7652088ac"
                        ),
                    },
                    TxOut {
                        value: 100000000,
                        script_pubkey: hex_script!(
                            "a9143545e6e33b832c47050f24d3eeb93c9c03948bc787"
                        ),
                    },
                ],
            },
            unknown: Default::default(),
        };

        let actual: Global = deserialize(&serialize(&expected)).unwrap();

        assert_eq!(expected, actual);
    }

    #[test]
    fn serialize_then_deserialize_psbtkvpair() {
        let expected = raw::Pair {
            key: raw::Key {
                type_value: 0u8,
                key: vec![42u8, 69u8],
            },
            value: vec![69u8, 42u8, 4u8],
        };

        let actual: raw::Pair = deserialize(&serialize(&expected)).unwrap();

        assert_eq!(expected, actual);
    }

    #[test]
    fn deserialize_and_serialize_psbt_with_two_partial_sigs() {
        let hex = "70736274ff0100890200000001207ae985d787dfe6143d5c58fad79cc7105e0e799fcf033b7f2ba17e62d7b3200000000000ffffffff02563d03000000000022002019899534b9a011043c0dd57c3ff9a381c3522c5f27c6a42319085b56ca543a1d6adc020000000000220020618b47a07ebecca4e156edb1b9ea7c24bdee0139fc049237965ffdaf56d5ee73000000000001012b801a0600000000002200201148e93e9315e37dbed2121be5239257af35adc03ffdfc5d914b083afa44dab82202025fe7371376d53cf8a2783917c28bf30bd690b0a4d4a207690093ca2b920ee076473044022007e06b362e89912abd4661f47945430739b006a85d1b2a16c01dc1a4bd07acab022061576d7aa834988b7ab94ef21d8eebd996ea59ea20529a19b15f0c9cebe3d8ac01220202b3fe93530020a8294f0e527e33fbdff184f047eb6b5a1558a352f62c29972f8a473044022002787f926d6817504431ee281183b8119b6845bfaa6befae45e13b6d430c9d2f02202859f149a6cd26ae2f03a107e7f33c7d91730dade305fe077bae677b5d44952a01010547522102b3fe93530020a8294f0e527e33fbdff184f047eb6b5a1558a352f62c29972f8a21025fe7371376d53cf8a2783917c28bf30bd690b0a4d4a207690093ca2b920ee07652ae0001014752210283ef76537f2d58ae3aa3a4bd8ae41c3f230ccadffb1a0bd3ca504d871cff05e7210353d79cc0cb1396f4ce278d005f16d948e02a6aec9ed1109f13747ecb1507b37b52ae00010147522102b3937241777b6665e0d694e52f9c1b188433641df852da6fc42187b5d8a368a321034cdd474f01cc5aa7ff834ad8bcc882a87e854affc775486bc2a9f62e8f49bd7852ae00";
        let psbt: PartiallySignedTransaction = hex_psbt!(hex).unwrap();
        assert_eq!(hex, serialize_hex(&psbt));
    }

    mod bip_vectors {
        use std::collections::BTreeMap;

        use hex::decode as hex_decode;

        use hashes::hex::FromHex;
        use hashes::sha256d;

        use blockdata::script::Script;
        use blockdata::transaction::{SigHashType, Transaction, TxIn, TxOut, OutPoint};
        use consensus::encode::serialize_hex;
        use util::psbt::map::{Map, Global, Input, Output};
        use util::psbt::raw;
        use util::psbt::PartiallySignedTransaction;

        #[test]
        fn invalid_vector_1() {
            let psbt: Result<PartiallySignedTransaction, _> = hex_psbt!("0200000001268171371edff285e937adeea4b37b78000c0566cbb3ad64641713ca42171bf6000000006a473044022070b2245123e6bf474d60c5b50c043d4c691a5d2435f09a34a7662a9dc251790a022001329ca9dacf280bdf30740ec0390422422c81cb45839457aeb76fc12edd95b3012102657d118d3357b8e0f4c2cd46db7b39f6d9c38d9a70abcb9b2de5dc8dbfe4ce31feffffff02d3dff505000000001976a914d0c59903c5bac2868760e90fd521a4665aa7652088ac00e1f5050000000017a9143545e6e33b832c47050f24d3eeb93c9c03948bc787b32e1300");
            assert!(psbt.is_err());
        }

        #[test]
        fn invalid_vector_2() {
            let psbt: Result<PartiallySignedTransaction, _> = hex_psbt!("70736274ff0100750200000001268171371edff285e937adeea4b37b78000c0566cbb3ad64641713ca42171bf60000000000feffffff02d3dff505000000001976a914d0c59903c5bac2868760e90fd521a4665aa7652088ac00e1f5050000000017a9143545e6e33b832c47050f24d3eeb93c9c03948bc787b32e1300000100fda5010100000000010289a3c71eab4d20e0371bbba4cc698fa295c9463afa2e397f8533ccb62f9567e50100000017160014be18d152a9b012039daf3da7de4f53349eecb985ffffffff86f8aa43a71dff1448893a530a7237ef6b4608bbb2dd2d0171e63aec6a4890b40100000017160014fe3e9ef1a745e974d902c4355943abcb34bd5353ffffffff0200c2eb0b000000001976a91485cff1097fd9e008bb34af709c62197b38978a4888ac72fef84e2c00000017a914339725ba21efd62ac753a9bcd067d6c7a6a39d05870247304402202712be22e0270f394f568311dc7ca9a68970b8025fdd3b240229f07f8a5f3a240220018b38d7dcd314e734c9276bd6fb40f673325bc4baa144c800d2f2f02db2765c012103d2e15674941bad4a996372cb87e1856d3652606d98562fe39c5e9e7e413f210502483045022100d12b852d85dcd961d2f5f4ab660654df6eedcc794c0c33ce5cc309ffb5fce58d022067338a8e0e1725c197fb1a88af59f51e44e4255b20167c8684031c05d1f2592a01210223b72beef0965d10be0778efecd61fcac6f79a4ea169393380734464f84f2ab30000000000");
            assert!(psbt.is_err());
        }

        #[test]
        fn invalid_vector_3() {
            let psbt: Result<PartiallySignedTransaction, _> = hex_psbt!("70736274ff0100fd0a010200000002ab0949a08c5af7c49b8212f417e2f15ab3f5c33dcf153821a8139f877a5b7be4000000006a47304402204759661797c01b036b25928948686218347d89864b719e1f7fcf57d1e511658702205309eabf56aa4d8891ffd111fdf1336f3a29da866d7f8486d75546ceedaf93190121035cdc61fc7ba971c0b501a646a2a83b102cb43881217ca682dc86e2d73fa88292feffffffab0949a08c5af7c49b8212f417e2f15ab3f5c33dcf153821a8139f877a5b7be40100000000feffffff02603bea0b000000001976a914768a40bbd740cbe81d988e71de2a4d5c71396b1d88ac8e240000000000001976a9146f4620b553fa095e721b9ee0efe9fa039cca459788ac00000000000001012000e1f5050000000017a9143545e6e33b832c47050f24d3eeb93c9c03948bc787010416001485d13537f2e265405a34dbafa9e3dda01fb82308000000");
            assert!(psbt.is_err());
        }

        #[test]
        fn invalid_vector_4() {
            let psbt: Result<PartiallySignedTransaction, _> = hex_psbt!("70736274ff000100fda5010100000000010289a3c71eab4d20e0371bbba4cc698fa295c9463afa2e397f8533ccb62f9567e50100000017160014be18d152a9b012039daf3da7de4f53349eecb985ffffffff86f8aa43a71dff1448893a530a7237ef6b4608bbb2dd2d0171e63aec6a4890b40100000017160014fe3e9ef1a745e974d902c4355943abcb34bd5353ffffffff0200c2eb0b000000001976a91485cff1097fd9e008bb34af709c62197b38978a4888ac72fef84e2c00000017a914339725ba21efd62ac753a9bcd067d6c7a6a39d05870247304402202712be22e0270f394f568311dc7ca9a68970b8025fdd3b240229f07f8a5f3a240220018b38d7dcd314e734c9276bd6fb40f673325bc4baa144c800d2f2f02db2765c012103d2e15674941bad4a996372cb87e1856d3652606d98562fe39c5e9e7e413f210502483045022100d12b852d85dcd961d2f5f4ab660654df6eedcc794c0c33ce5cc309ffb5fce58d022067338a8e0e1725c197fb1a88af59f51e44e4255b20167c8684031c05d1f2592a01210223b72beef0965d10be0778efecd61fcac6f79a4ea169393380734464f84f2ab30000000000");
            assert!(psbt.is_err());
        }

        #[test]
        fn invalid_vector_5() {
            let psbt: Result<PartiallySignedTransaction, _> = hex_psbt!("70736274ff0100750200000001268171371edff285e937adeea4b37b78000c0566cbb3ad64641713ca42171bf60000000000feffffff02d3dff505000000001976a914d0c59903c5bac2868760e90fd521a4665aa7652088ac00e1f5050000000017a9143545e6e33b832c47050f24d3eeb93c9c03948bc787b32e1300000100fda5010100000000010289a3c71eab4d20e0371bbba4cc698fa295c9463afa2e397f8533ccb62f9567e50100000017160014be18d152a9b012039daf3da7de4f53349eecb985ffffffff86f8aa43a71dff1448893a530a7237ef6b4608bbb2dd2d0171e63aec6a4890b40100000017160014fe3e9ef1a745e974d902c4355943abcb34bd5353ffffffff0200c2eb0b000000001976a91485cff1097fd9e008bb34af709c62197b38978a4888ac72fef84e2c00000017a914339725ba21efd62ac753a9bcd067d6c7a6a39d05870247304402202712be22e0270f394f568311dc7ca9a68970b8025fdd3b240229f07f8a5f3a240220018b38d7dcd314e734c9276bd6fb40f673325bc4baa144c800d2f2f02db2765c012103d2e15674941bad4a996372cb87e1856d3652606d98562fe39c5e9e7e413f210502483045022100d12b852d85dcd961d2f5f4ab660654df6eedcc794c0c33ce5cc309ffb5fce58d022067338a8e0e1725c197fb1a88af59f51e44e4255b20167c8684031c05d1f2592a01210223b72beef0965d10be0778efecd61fcac6f79a4ea169393380734464f84f2ab30000000001003f0200000001ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff0000000000ffffffff010000000000000000036a010000000000000000");
            assert!(psbt.is_err());
        }

        #[test]
        fn valid_vector_1() {
            let unserialized = PartiallySignedTransaction {
                global: Global {
                    unsigned_tx: Transaction {
                        version: 2,
                        lock_time: 1257139,
                        input: vec![TxIn {
                            previous_output: OutPoint {
                                txid: sha256d::Hash::from_hex(
                                    "f61b1742ca13176464adb3cb66050c00787bb3a4eead37e985f2df1e37718126",
                                ).unwrap(),
                                vout: 0,
                            },
                            script_sig: Script::new(),
                            sequence: 4294967294,
                            witness: vec![],
                        }],
                        output: vec![
                            TxOut {
                                value: 99999699,
                                script_pubkey: hex_script!("76a914d0c59903c5bac2868760e90fd521a4665aa7652088ac"),
                            },
                            TxOut {
                                value: 100000000,
                                script_pubkey: hex_script!("a9143545e6e33b832c47050f24d3eeb93c9c03948bc787"),
                            },
                        ],
                    },
                    unknown: BTreeMap::new(),
                },
                inputs: vec![Input {
                    non_witness_utxo: Some(Transaction {
                        version: 1,
                        lock_time: 0,
                        input: vec![TxIn {
                            previous_output: OutPoint {
                                txid: sha256d::Hash::from_hex(
                                    "e567952fb6cc33857f392efa3a46c995a28f69cca4bb1b37e0204dab1ec7a389",
                                ).unwrap(),
                                vout: 1,
                            },
                            script_sig: hex_script!("160014be18d152a9b012039daf3da7de4f53349eecb985"),
                            sequence: 4294967295,
                            witness: vec![
                                hex_decode("304402202712be22e0270f394f568311dc7ca9a68970b8025fdd3b240229f07f8a5f3a240220018b38d7dcd314e734c9276bd6fb40f673325bc4baa144c800d2f2f02db2765c01").unwrap(),
                                hex_decode("03d2e15674941bad4a996372cb87e1856d3652606d98562fe39c5e9e7e413f2105").unwrap(),
                            ],
                        },
                        TxIn {
                            previous_output: OutPoint {
                                txid: sha256d::Hash::from_hex(
                                    "b490486aec3ae671012dddb2bb08466bef37720a533a894814ff1da743aaf886",
                                ).unwrap(),
                                vout: 1,
                            },
                            script_sig: hex_script!("160014fe3e9ef1a745e974d902c4355943abcb34bd5353"),
                            sequence: 4294967295,
                            witness: vec![
                                hex_decode("3045022100d12b852d85dcd961d2f5f4ab660654df6eedcc794c0c33ce5cc309ffb5fce58d022067338a8e0e1725c197fb1a88af59f51e44e4255b20167c8684031c05d1f2592a01").unwrap(),
                                hex_decode("0223b72beef0965d10be0778efecd61fcac6f79a4ea169393380734464f84f2ab3").unwrap(),
                            ],
                        }],
                        output: vec![
                            TxOut {
                                value: 200000000,
                                script_pubkey: hex_script!("76a91485cff1097fd9e008bb34af709c62197b38978a4888ac"),
                            },
                            TxOut {
                                value: 190303501938,
                                script_pubkey: hex_script!("a914339725ba21efd62ac753a9bcd067d6c7a6a39d0587"),
                            },
                        ],
                    }),
                    ..Default::default()
                },],
                outputs: vec![
                    Output {
                        ..Default::default()
                    },
                    Output {
                        ..Default::default()
                    },
                ],
            };

            let serialized = "70736274ff0100750200000001268171371edff285e937adeea4b37b78000c0566cbb3ad64641713ca42171bf60000000000feffffff02d3dff505000000001976a914d0c59903c5bac2868760e90fd521a4665aa7652088ac00e1f5050000000017a9143545e6e33b832c47050f24d3eeb93c9c03948bc787b32e1300000100fda5010100000000010289a3c71eab4d20e0371bbba4cc698fa295c9463afa2e397f8533ccb62f9567e50100000017160014be18d152a9b012039daf3da7de4f53349eecb985ffffffff86f8aa43a71dff1448893a530a7237ef6b4608bbb2dd2d0171e63aec6a4890b40100000017160014fe3e9ef1a745e974d902c4355943abcb34bd5353ffffffff0200c2eb0b000000001976a91485cff1097fd9e008bb34af709c62197b38978a4888ac72fef84e2c00000017a914339725ba21efd62ac753a9bcd067d6c7a6a39d05870247304402202712be22e0270f394f568311dc7ca9a68970b8025fdd3b240229f07f8a5f3a240220018b38d7dcd314e734c9276bd6fb40f673325bc4baa144c800d2f2f02db2765c012103d2e15674941bad4a996372cb87e1856d3652606d98562fe39c5e9e7e413f210502483045022100d12b852d85dcd961d2f5f4ab660654df6eedcc794c0c33ce5cc309ffb5fce58d022067338a8e0e1725c197fb1a88af59f51e44e4255b20167c8684031c05d1f2592a01210223b72beef0965d10be0778efecd61fcac6f79a4ea169393380734464f84f2ab300000000000000";

            assert_eq!(serialize_hex(&unserialized), serialized);
            assert_eq!(unserialized, hex_psbt!(serialized).unwrap());
        }

        #[test]
        fn valid_vector_2() {
            let psbt: PartiallySignedTransaction = hex_psbt!("70736274ff0100a00200000002ab0949a08c5af7c49b8212f417e2f15ab3f5c33dcf153821a8139f877a5b7be40000000000feffffffab0949a08c5af7c49b8212f417e2f15ab3f5c33dcf153821a8139f877a5b7be40100000000feffffff02603bea0b000000001976a914768a40bbd740cbe81d988e71de2a4d5c71396b1d88ac8e240000000000001976a9146f4620b553fa095e721b9ee0efe9fa039cca459788ac000000000001076a47304402204759661797c01b036b25928948686218347d89864b719e1f7fcf57d1e511658702205309eabf56aa4d8891ffd111fdf1336f3a29da866d7f8486d75546ceedaf93190121035cdc61fc7ba971c0b501a646a2a83b102cb43881217ca682dc86e2d73fa882920001012000e1f5050000000017a9143545e6e33b832c47050f24d3eeb93c9c03948bc787010416001485d13537f2e265405a34dbafa9e3dda01fb82308000000").unwrap();

            assert_eq!(psbt.inputs.len(), 2);
            assert_eq!(psbt.outputs.len(), 2);

            assert!(&psbt.inputs[0].final_script_sig.is_some());

            let redeem_script: &Script = &psbt.inputs[1].redeem_script.as_ref().unwrap();
            let expected_out = hex_script!("a9143545e6e33b832c47050f24d3eeb93c9c03948bc787");

            assert!(redeem_script.is_v0_p2wpkh());
            assert_eq!(
                redeem_script.to_p2sh(),
                psbt.inputs[1].witness_utxo.as_ref().unwrap().script_pubkey
            );
            assert_eq!(redeem_script.to_p2sh(), expected_out);

            for output in psbt.outputs {
                assert_eq!(output.get_pairs().unwrap().len(), 0)
            }
        }

        #[test]
        fn valid_vector_3() {
            let psbt: PartiallySignedTransaction = hex_psbt!("70736274ff0100750200000001268171371edff285e937adeea4b37b78000c0566cbb3ad64641713ca42171bf60000000000feffffff02d3dff505000000001976a914d0c59903c5bac2868760e90fd521a4665aa7652088ac00e1f5050000000017a9143545e6e33b832c47050f24d3eeb93c9c03948bc787b32e1300000100fda5010100000000010289a3c71eab4d20e0371bbba4cc698fa295c9463afa2e397f8533ccb62f9567e50100000017160014be18d152a9b012039daf3da7de4f53349eecb985ffffffff86f8aa43a71dff1448893a530a7237ef6b4608bbb2dd2d0171e63aec6a4890b40100000017160014fe3e9ef1a745e974d902c4355943abcb34bd5353ffffffff0200c2eb0b000000001976a91485cff1097fd9e008bb34af709c62197b38978a4888ac72fef84e2c00000017a914339725ba21efd62ac753a9bcd067d6c7a6a39d05870247304402202712be22e0270f394f568311dc7ca9a68970b8025fdd3b240229f07f8a5f3a240220018b38d7dcd314e734c9276bd6fb40f673325bc4baa144c800d2f2f02db2765c012103d2e15674941bad4a996372cb87e1856d3652606d98562fe39c5e9e7e413f210502483045022100d12b852d85dcd961d2f5f4ab660654df6eedcc794c0c33ce5cc309ffb5fce58d022067338a8e0e1725c197fb1a88af59f51e44e4255b20167c8684031c05d1f2592a01210223b72beef0965d10be0778efecd61fcac6f79a4ea169393380734464f84f2ab30000000001030401000000000000").unwrap();

            assert_eq!(psbt.inputs.len(), 1);
            assert_eq!(psbt.outputs.len(), 2);

            let tx_input = &psbt.global.unsigned_tx.input[0];
            let psbt_non_witness_utxo = (&psbt.inputs[0].non_witness_utxo).as_ref().unwrap();

            assert_eq!(tx_input.previous_output.txid, psbt_non_witness_utxo.txid());
            assert!(
                psbt_non_witness_utxo.output[tx_input.previous_output.vout as usize]
                    .script_pubkey
                    .is_p2pkh()
            );
            assert_eq!(
                (&psbt.inputs[0].sighash_type).as_ref().unwrap(),
                &SigHashType::All
            );
        }

        #[test]
        fn valid_vector_4() {
            let psbt: PartiallySignedTransaction = hex_psbt!("70736274ff0100a00200000002ab0949a08c5af7c49b8212f417e2f15ab3f5c33dcf153821a8139f877a5b7be40000000000feffffffab0949a08c5af7c49b8212f417e2f15ab3f5c33dcf153821a8139f877a5b7be40100000000feffffff02603bea0b000000001976a914768a40bbd740cbe81d988e71de2a4d5c71396b1d88ac8e240000000000001976a9146f4620b553fa095e721b9ee0efe9fa039cca459788ac00000000000100df0200000001268171371edff285e937adeea4b37b78000c0566cbb3ad64641713ca42171bf6000000006a473044022070b2245123e6bf474d60c5b50c043d4c691a5d2435f09a34a7662a9dc251790a022001329ca9dacf280bdf30740ec0390422422c81cb45839457aeb76fc12edd95b3012102657d118d3357b8e0f4c2cd46db7b39f6d9c38d9a70abcb9b2de5dc8dbfe4ce31feffffff02d3dff505000000001976a914d0c59903c5bac2868760e90fd521a4665aa7652088ac00e1f5050000000017a9143545e6e33b832c47050f24d3eeb93c9c03948bc787b32e13000001012000e1f5050000000017a9143545e6e33b832c47050f24d3eeb93c9c03948bc787010416001485d13537f2e265405a34dbafa9e3dda01fb8230800220202ead596687ca806043edc3de116cdf29d5e9257c196cd055cf698c8d02bf24e9910b4a6ba670000008000000080020000800022020394f62be9df19952c5587768aeb7698061ad2c4a25c894f47d8c162b4d7213d0510b4a6ba6700000080010000800200008000").unwrap();

            assert_eq!(psbt.inputs.len(), 2);
            assert_eq!(psbt.outputs.len(), 2);

            assert!(&psbt.inputs[0].final_script_sig.is_none());
            assert!(&psbt.inputs[1].final_script_sig.is_none());

            let redeem_script: &Script = &psbt.inputs[1].redeem_script.as_ref().unwrap();
            let expected_out = hex_script!("a9143545e6e33b832c47050f24d3eeb93c9c03948bc787");

            assert!(redeem_script.is_v0_p2wpkh());
            assert_eq!(
                redeem_script.to_p2sh(),
                psbt.inputs[1].witness_utxo.as_ref().unwrap().script_pubkey
            );
            assert_eq!(redeem_script.to_p2sh(), expected_out);

            for output in psbt.outputs {
                assert!(output.get_pairs().unwrap().len() > 0)
            }
        }

        #[test]
        fn valid_vector_5() {
            let psbt: PartiallySignedTransaction = hex_psbt!("70736274ff0100550200000001279a2323a5dfb51fc45f220fa58b0fc13e1e3342792a85d7e36cd6333b5cbc390000000000ffffffff01a05aea0b000000001976a914ffe9c0061097cc3b636f2cb0460fa4fc427d2b4588ac0000000000010120955eea0b0000000017a9146345200f68d189e1adc0df1c4d16ea8f14c0dbeb87220203b1341ccba7683b6af4f1238cd6e97e7167d569fac47f1e48d47541844355bd4646304302200424b58effaaa694e1559ea5c93bbfd4a89064224055cdf070b6771469442d07021f5c8eb0fea6516d60b8acb33ad64ede60e8785bfb3aa94b99bdf86151db9a9a010104220020771fd18ad459666dd49f3d564e3dbc42f4c84774e360ada16816a8ed488d5681010547522103b1341ccba7683b6af4f1238cd6e97e7167d569fac47f1e48d47541844355bd462103de55d1e1dac805e3f8a58c1fbf9b94c02f3dbaafe127fefca4995f26f82083bd52ae220603b1341ccba7683b6af4f1238cd6e97e7167d569fac47f1e48d47541844355bd4610b4a6ba67000000800000008004000080220603de55d1e1dac805e3f8a58c1fbf9b94c02f3dbaafe127fefca4995f26f82083bd10b4a6ba670000008000000080050000800000").unwrap();

            assert_eq!(psbt.inputs.len(), 1);
            assert_eq!(psbt.outputs.len(), 1);

            assert!(&psbt.inputs[0].final_script_sig.is_none());

            let redeem_script: &Script = &psbt.inputs[0].redeem_script.as_ref().unwrap();
            let expected_out = hex_script!("a9146345200f68d189e1adc0df1c4d16ea8f14c0dbeb87");

            assert!(redeem_script.is_v0_p2wsh());
            assert_eq!(
                redeem_script.to_p2sh(),
                psbt.inputs[0].witness_utxo.as_ref().unwrap().script_pubkey
            );

            assert_eq!(redeem_script.to_p2sh(), expected_out);
        }

        #[test]
        fn valid_vector_6() {
            let psbt: PartiallySignedTransaction = hex_psbt!("70736274ff01003f0200000001ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff0000000000ffffffff010000000000000000036a010000000000000a0f0102030405060708090f0102030405060708090a0b0c0d0e0f0000").unwrap();

            assert_eq!(psbt.inputs.len(), 1);
            assert_eq!(psbt.outputs.len(), 1);

            let tx = &psbt.global.unsigned_tx;
            assert_eq!(
                tx.txid(),
                sha256d::Hash::from_hex(
                    "75c5c9665a570569ad77dd1279e6fd4628a093c4dcbf8d41532614044c14c115"
                ).unwrap()
            );

            let mut unknown: BTreeMap<raw::Key, Vec<u8>> = BTreeMap::new();
            let key: raw::Key = raw::Key {
                type_value: 0x0fu8,
                key: hex_decode("010203040506070809").unwrap(),
            };
            let value: Vec<u8> = hex_decode("0102030405060708090a0b0c0d0e0f").unwrap();

            unknown.insert(key, value);

            assert_eq!(psbt.inputs[0].unknown, unknown)
        }
    }
}
