use std::ffi::c_void;

use bls_dash_sys::{
    BIP32ExtendedPublicKeyFree, BIP32ExtendedPublicKeyFromBytes,
    BIP32ExtendedPublicKeyGetChainCode, BIP32ExtendedPublicKeyGetPublicKey,
    BIP32ExtendedPublicKeyIsEqual, BIP32ExtendedPublicKeyPublicChild,
    BIP32ExtendedPublicKeySerialize,
};

use crate::{bip32::chain_code::ChainCode, utils::c_err_to_result, BlsError, G1Element};

pub const BIP32_EXTENDED_PUBLIC_KEY_SIZE: usize = 93;

#[derive(Debug)]
pub struct ExtendedPublicKey {
    pub(crate) c_extended_public_key: *mut c_void,
}

impl PartialEq for ExtendedPublicKey {
    fn eq(&self, other: &Self) -> bool {
        unsafe {
            BIP32ExtendedPublicKeyIsEqual(self.c_extended_public_key, other.c_extended_public_key)
        }
    }
}

impl Eq for ExtendedPublicKey {}

impl ExtendedPublicKey {
    pub(crate) fn from_bytes_with_legacy_flag(
        bytes: &[u8],
        legacy: bool,
    ) -> Result<Self, BlsError> {
        if bytes.len() != BIP32_EXTENDED_PUBLIC_KEY_SIZE {
            return Err(BlsError {
                msg: format!(
                    "Extended Public Key size must be {}, got {}",
                    BIP32_EXTENDED_PUBLIC_KEY_SIZE,
                    bytes.len()
                ),
            });
        }
        Ok(ExtendedPublicKey {
            c_extended_public_key: c_err_to_result(|did_err| unsafe {
                BIP32ExtendedPublicKeyFromBytes(bytes.as_ptr() as *const _, legacy, did_err)
            })?,
        })
    }

    pub fn from_bytes(bytes: &[u8]) -> Result<Self, BlsError> {
        Self::from_bytes_with_legacy_flag(bytes, false)
    }

    pub(crate) fn public_child_with_legacy_flag(&self, index: u32, legacy: bool) -> Self {
        ExtendedPublicKey {
            c_extended_public_key: unsafe {
                BIP32ExtendedPublicKeyPublicChild(self.c_extended_public_key, index, legacy)
            },
        }
    }

    pub fn public_child(&self, index: u32) -> Self {
        self.public_child_with_legacy_flag(index, false)
    }

    pub(crate) fn serialize_with_legacy_flag(
        &self,
        legacy: bool,
    ) -> Box<[u8; BIP32_EXTENDED_PUBLIC_KEY_SIZE]> {
        unsafe {
            let malloc_ptr = BIP32ExtendedPublicKeySerialize(self.c_extended_public_key, legacy);
            Box::from_raw(malloc_ptr as *mut _)
        }
    }

    pub fn serialize(&self) -> Box<[u8; BIP32_EXTENDED_PUBLIC_KEY_SIZE]> {
        self.serialize_with_legacy_flag(false)
    }

    pub fn chain_code(&self) -> ChainCode {
        ChainCode {
            c_chain_code: unsafe { BIP32ExtendedPublicKeyGetChainCode(self.c_extended_public_key) },
        }
    }

    pub fn public_key(&self) -> G1Element {
        G1Element {
            c_element: unsafe { BIP32ExtendedPublicKeyGetPublicKey(self.c_extended_public_key) },
        }
    }
}

impl Drop for ExtendedPublicKey {
    fn drop(&mut self) {
        unsafe { BIP32ExtendedPublicKeyFree(self.c_extended_public_key) }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::bip32::ExtendedPrivateKey;

    #[test]
    fn serialize_deserialize() {
        let seed = b"seedweedseedweedseedweedseedweed";
        let private_key =
            ExtendedPrivateKey::from_seed(seed).expect("cannot generate extended private key");
        let public_key = private_key
            .extended_public_key()
            .expect("cannot get extended public key");

        let public_key_bytes = public_key.serialize();
        let public_key_2 = ExtendedPublicKey::from_bytes(public_key_bytes.as_ref())
            .expect("cannot deserialize extended public key");

        assert_eq!(public_key, public_key_2);
    }
}
