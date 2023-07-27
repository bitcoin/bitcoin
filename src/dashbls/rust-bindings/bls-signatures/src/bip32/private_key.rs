use std::ffi::c_void;

use bls_dash_sys::{
    BIP32ExtendedPrivateKeyFree, BIP32ExtendedPrivateKeyFromBytes, BIP32ExtendedPrivateKeyFromSeed,
    BIP32ExtendedPrivateKeyGetChainCode, BIP32ExtendedPrivateKeyGetExtendedPublicKey,
    BIP32ExtendedPrivateKeyGetPrivateKey, BIP32ExtendedPrivateKeyGetPublicKey,
    BIP32ExtendedPrivateKeyIsEqual, BIP32ExtendedPrivateKeyPrivateChild,
    BIP32ExtendedPrivateKeyPublicChild, BIP32ExtendedPrivateKeySerialize,
};

use crate::{
    bip32::{chain_code::ChainCode, ExtendedPublicKey},
    utils::{c_err_to_result, SecureBox},
    BlsError, G1Element, PrivateKey,
};

pub const BIP32_EXTENDED_PRIVATE_KEY_SIZE: usize = 77;

#[derive(Debug)]
pub struct ExtendedPrivateKey {
    c_extended_private_key: *mut c_void,
}

impl PartialEq for ExtendedPrivateKey {
    fn eq(&self, other: &Self) -> bool {
        unsafe {
            BIP32ExtendedPrivateKeyIsEqual(
                self.c_extended_private_key,
                other.c_extended_private_key,
            )
        }
    }
}

impl Eq for ExtendedPrivateKey {}

impl ExtendedPrivateKey {
    pub fn from_bytes(bytes: &[u8]) -> Result<Self, BlsError> {
        if bytes.len() != BIP32_EXTENDED_PRIVATE_KEY_SIZE {
            return Err(BlsError {
                msg: format!(
                    "Extended Private Key size must be {}, got {}",
                    BIP32_EXTENDED_PRIVATE_KEY_SIZE,
                    bytes.len()
                ),
            });
        }
        Ok(ExtendedPrivateKey {
            c_extended_private_key: c_err_to_result(|did_err| unsafe {
                BIP32ExtendedPrivateKeyFromBytes(bytes.as_ptr() as *const _, did_err)
            })?,
        })
    }

    pub fn from_seed(bytes: &[u8]) -> Result<Self, BlsError> {
        Ok(ExtendedPrivateKey {
            c_extended_private_key: c_err_to_result(|did_err| unsafe {
                BIP32ExtendedPrivateKeyFromSeed(bytes.as_ptr() as *const _, bytes.len(), did_err)
            })?,
        })
    }

    pub(crate) fn private_child_with_legacy_flag(&self, index: u32, legacy: bool) -> Self {
        ExtendedPrivateKey {
            c_extended_private_key: unsafe {
                BIP32ExtendedPrivateKeyPrivateChild(self.c_extended_private_key, index, legacy)
            },
        }
    }

    pub fn private_child(&self, index: u32) -> Self {
        self.private_child_with_legacy_flag(index, false)
    }

    pub fn public_child(&self, index: u32) -> ExtendedPublicKey {
        ExtendedPublicKey {
            c_extended_public_key: unsafe {
                BIP32ExtendedPrivateKeyPublicChild(self.c_extended_private_key, index)
            },
        }
    }

    pub(crate) fn extended_public_key_with_legacy_flag(
        &self,
        legacy: bool,
    ) -> Result<ExtendedPublicKey, BlsError> {
        Ok(ExtendedPublicKey {
            c_extended_public_key: c_err_to_result(|did_err| unsafe {
                BIP32ExtendedPrivateKeyGetExtendedPublicKey(
                    self.c_extended_private_key,
                    legacy,
                    did_err,
                )
            })?,
        })
    }

    pub fn extended_public_key(&self) -> Result<ExtendedPublicKey, BlsError> {
        self.extended_public_key_with_legacy_flag(false)
    }

    pub fn public_key(&self) -> Result<G1Element, BlsError> {
        Ok(G1Element {
            c_element: c_err_to_result(|did_err| unsafe {
                BIP32ExtendedPrivateKeyGetPublicKey(self.c_extended_private_key, did_err)
            })?,
        })
    }

    pub fn private_key(&self) -> PrivateKey {
        PrivateKey {
            c_private_key: unsafe {
                BIP32ExtendedPrivateKeyGetPrivateKey(self.c_extended_private_key)
            },
        }
    }

    pub fn serialize(&self) -> SecureBox {
        unsafe {
            let secalloc_ptr = BIP32ExtendedPrivateKeySerialize(self.c_extended_private_key);
            SecureBox::from_ptr(secalloc_ptr as *mut u8, BIP32_EXTENDED_PRIVATE_KEY_SIZE)
        }
    }

    pub fn chain_code(&self) -> ChainCode {
        ChainCode {
            c_chain_code: unsafe {
                BIP32ExtendedPrivateKeyGetChainCode(self.c_extended_private_key)
            },
        }
    }
}

impl Drop for ExtendedPrivateKey {
    fn drop(&mut self) {
        unsafe { BIP32ExtendedPrivateKeyFree(self.c_extended_private_key) }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn serialize_deserialize() {
        let seed = b"seedweedseedweedseedweedseedweed";
        let private_key =
            ExtendedPrivateKey::from_seed(seed).expect("cannot generate extended private key");

        let private_key_bytes = private_key.serialize();
        let private_key_2 = ExtendedPrivateKey::from_bytes(private_key_bytes.as_ref())
            .expect("cannot deserialize extended private key");

        assert_eq!(private_key, private_key_2);
        assert_eq!(private_key.private_key(), private_key_2.private_key());
        assert_eq!(private_key.public_key(), private_key_2.public_key());
    }

    #[test]
    fn hierarchical_deterministic_keys() {
        let seed = b"seedweedseedweedseedweedseedweed";
        let private_key =
            ExtendedPrivateKey::from_seed(seed).expect("cannot generate extended private key");
        let public_key = private_key
            .extended_public_key()
            .expect("cannot get extended public key");

        let private_child = private_key.private_child(1337);
        let private_grandchild = private_child.private_child(420);

        let public_child = public_key.public_child(1337);
        let public_grandchild = public_child.public_child(420);

        assert_eq!(
            public_grandchild,
            private_grandchild
                .extended_public_key()
                .expect("cannot get extended public key")
        );
    }

    #[test]
    fn public_keys_match() {
        let seed = b"seedweedseedweedseedweedseedweed";
        let private_key =
            ExtendedPrivateKey::from_seed(seed).expect("cannot generate extended private key");
        let public_key = private_key
            .extended_public_key()
            .expect("cannot get extended public key");

        assert_eq!(private_key.public_key(), Ok(public_key.public_key()));
    }

    #[test]
    fn fingerprint_for_short_bip32_seed() {
        assert_eq!(
            ExtendedPrivateKey::from_seed(&[1u8, 50, 6, 244, 24, 199, 1, 25])
                .expect("cannot generate extended private key")
                .public_key()
                .expect("cannot get public key from extended private key")
                .fingerprint_legacy(),
            0xa4700b27
        );
    }
}
