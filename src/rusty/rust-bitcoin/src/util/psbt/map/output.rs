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

use std::collections::BTreeMap;

use blockdata::script::Script;
use consensus::encode;
use util::bip32::{DerivationPath, Fingerprint};
use util::key::PublicKey;
use util::psbt;
use util::psbt::map::Map;
use util::psbt::raw;
use util::psbt::Error;

/// A key-value map for an output of the corresponding index in the unsigned
/// transaction.
#[derive(Clone, Default, Debug, PartialEq)]
pub struct Output {
    /// The redeem script for this output.
    pub redeem_script: Option<Script>,
    /// The witness script for this output.
    pub witness_script: Option<Script>,
    /// A map from public keys needed to spend this output to their
    /// corresponding master key fingerprints and derivation paths.
    pub hd_keypaths: BTreeMap<PublicKey, (Fingerprint, DerivationPath)>,
    /// Unknown key-value pairs for this output.
    pub unknown: BTreeMap<raw::Key, Vec<u8>>,
}

impl Map for Output {
    fn insert_pair(&mut self, pair: raw::Pair) -> Result<(), encode::Error> {
        let raw::Pair {
            key: raw_key,
            value: raw_value,
        } = pair;

        match raw_key.type_value {
            0u8 => {
                impl_psbt_insert_pair! {
                    self.redeem_script <= <raw_key: _>|<raw_value: Script>
                }
            }
            1u8 => {
                impl_psbt_insert_pair! {
                    self.witness_script <= <raw_key: _>|<raw_value: Script>
                }
            }
            2u8 => {
                impl_psbt_insert_pair! {
                    self.hd_keypaths <= <raw_key: PublicKey>|<raw_value: (Fingerprint, DerivationPath)>
                }
            }
            _ => {
                if self.unknown.contains_key(&raw_key) {
                    return Err(Error::DuplicateKey(raw_key).into());
                } else {
                    self.unknown.insert(raw_key, raw_value);
                }
            }
        }

        Ok(())
    }

    fn get_pairs(&self) -> Result<Vec<raw::Pair>, encode::Error> {
        let mut rv: Vec<raw::Pair> = Default::default();

        impl_psbt_get_pair! {
            rv.push(self.redeem_script as <0u8, _>|<Script>)
        }

        impl_psbt_get_pair! {
            rv.push(self.witness_script as <1u8, _>|<Script>)
        }

        impl_psbt_get_pair! {
            rv.push(self.hd_keypaths as <2u8, PublicKey>|<(Fingerprint, DerivationPath)>)
        }

        for (key, value) in self.unknown.iter() {
            rv.push(raw::Pair {
                key: key.clone(),
                value: value.clone(),
            });
        }

        Ok(rv)
    }

    fn merge(&mut self, other: Self) -> Result<(), psbt::Error> {
        self.hd_keypaths.extend(other.hd_keypaths);
        self.unknown.extend(other.unknown);

        merge!(redeem_script, self, other);
        merge!(witness_script, self, other);

        Ok(())
    }
}

impl_psbtmap_consensus_enc_dec_oding!(Output);
