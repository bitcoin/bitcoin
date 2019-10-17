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

#[allow(unused_macros)]
macro_rules! hex_psbt {
    ($s:expr) => { ::consensus::deserialize(&::hex::decode($s).unwrap()) };
}

macro_rules! merge {
    ($thing:ident, $slf:ident, $other:ident) => {
        if let (&None, Some($thing)) = (&$slf.$thing, $other.$thing) {
            $slf.$thing = Some($thing);
        }
    };
}

macro_rules! impl_psbt_de_serialize {
    ($thing:ty) => {
        impl_psbt_serialize!($thing);
        impl_psbt_deserialize!($thing);
    };
}

macro_rules! impl_psbt_deserialize {
    ($thing:ty) => {
        impl ::util::psbt::serialize::Deserialize for $thing {
            fn deserialize(bytes: &[u8]) -> Result<Self, ::consensus::encode::Error> {
                ::consensus::deserialize(&bytes[..])
            }
        }
    };
}

macro_rules! impl_psbt_serialize {
    ($thing:ty) => {
        impl ::util::psbt::serialize::Serialize for $thing {
            fn serialize(&self) -> Vec<u8> {
                ::consensus::serialize(self)
            }
        }
    };
}

macro_rules! impl_psbtmap_consensus_encoding {
    ($thing:ty) => {
        impl ::consensus::Encodable for $thing {
            fn consensus_encode<S: ::std::io::Write>(
                &self,
                mut s: S,
            ) -> Result<usize, ::consensus::encode::Error> {
                let mut len = 0;
                for pair in ::util::psbt::Map::get_pairs(self)? {
                    len += ::consensus::Encodable::consensus_encode(
                        &pair,
                        &mut s,
                    )?;
                }

                Ok(len + ::consensus::Encodable::consensus_encode(&0x00_u8, s)?)
            }
        }
    };
}

macro_rules! impl_psbtmap_consensus_decoding {
    ($thing:ty) => {
        impl ::consensus::Decodable for $thing {
            fn consensus_decode<D: ::std::io::Read>(
                mut d: D,
            ) -> Result<Self, ::consensus::encode::Error> {
                let mut rv: Self = ::std::default::Default::default();

                loop {
                    match ::consensus::Decodable::consensus_decode(&mut d) {
                        Ok(pair) => ::util::psbt::Map::insert_pair(&mut rv, pair)?,
                        Err(::consensus::encode::Error::Psbt(::util::psbt::Error::NoMorePairs)) => return Ok(rv),
                        Err(e) => return Err(e),
                    }
                }
            }
        }
    };
}

macro_rules! impl_psbtmap_consensus_enc_dec_oding {
    ($thing:ty) => {
        impl_psbtmap_consensus_decoding!($thing);
        impl_psbtmap_consensus_encoding!($thing);
    };
}

#[cfg_attr(rustfmt, rustfmt_skip)]
macro_rules! impl_psbt_insert_pair {
    ($slf:ident.$unkeyed_name:ident <= <$raw_key:ident: _>|<$raw_value:ident: $unkeyed_value_type:ty>) => {
        if $raw_key.key.is_empty() {
            if let None = $slf.$unkeyed_name {
                let val: $unkeyed_value_type = ::util::psbt::serialize::Deserialize::deserialize(&$raw_value)?;

                $slf.$unkeyed_name = Some(val)
            } else {
                return Err(::util::psbt::Error::DuplicateKey($raw_key).into());
            }
        } else {
            return Err(::util::psbt::Error::InvalidKey($raw_key).into());
        }
    };
    ($slf:ident.$keyed_name:ident <= <$raw_key:ident: $keyed_key_type:ty>|<$raw_value:ident: $keyed_value_type:ty>) => {
        if !$raw_key.key.is_empty() {
            let key_val: $keyed_key_type = ::util::psbt::serialize::Deserialize::deserialize(&$raw_key.key)?;

            if $slf.$keyed_name.contains_key(&key_val) {
                return Err(::util::psbt::Error::DuplicateKey($raw_key).into());
            } else {
                let val: $keyed_value_type = ::util::psbt::serialize::Deserialize::deserialize(&$raw_value)?;

                $slf.$keyed_name.insert(key_val, val);
            }
        } else {
            return Err(::util::psbt::Error::InvalidKey($raw_key).into());
        }
    };
}

#[cfg_attr(rustfmt, rustfmt_skip)]
macro_rules! impl_psbt_get_pair {
    ($rv:ident.push($slf:ident.$unkeyed_name:ident as <$unkeyed_typeval:expr, _>|<$unkeyed_value_type:ty>)) => {
        if let Some(ref $unkeyed_name) = $slf.$unkeyed_name {
            $rv.push(::util::psbt::raw::Pair {
                key: ::util::psbt::raw::Key {
                    type_value: $unkeyed_typeval,
                    key: vec![],
                },
                value: ::util::psbt::serialize::Serialize::serialize($unkeyed_name),
            });
        }
    };
    ($rv:ident.push($slf:ident.$keyed_name:ident as <$keyed_typeval:expr, $keyed_key_type:ty>|<$keyed_value_type:ty>)) => {
        for (key, val) in &$slf.$keyed_name {
            $rv.push(::util::psbt::raw::Pair {
                key: ::util::psbt::raw::Key {
                    type_value: $keyed_typeval,
                    key: ::util::psbt::serialize::Serialize::serialize(key),
                },
                value: ::util::psbt::serialize::Serialize::serialize(val),
            });
        }
    };
}
