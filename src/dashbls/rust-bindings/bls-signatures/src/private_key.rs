use std::{ffi::c_void, ops::Mul};

use bls_dash_sys::{
    CoreMPLDeriveChildSk, CoreMPLDeriveChildSkUnhardened, CoreMPLKeyGen, G1ElementMul,
    PrivateKeyFree, PrivateKeyFromBytes, PrivateKeyFromSeedBIP32, PrivateKeyGetG1Element,
    PrivateKeyIsEqual, PrivateKeySerialize, ThresholdPrivateKeyRecover,
};
use rand::{prelude::StdRng, Rng};
#[cfg(feature = "use_serde")]
use serde::{Deserialize, Deserializer, Serialize, Serializer};

use crate::{schemes::Scheme, utils::{c_err_to_result, SecureBox}, BasicSchemeMPL, BlsError, G1Element, G2Element};

pub const PRIVATE_KEY_SIZE: usize = 32; // TODO somehow extract it from bls library

#[derive(Debug)]
pub struct PrivateKey {
    pub(crate) c_private_key: *mut c_void,
}

impl PartialEq for PrivateKey {
    fn eq(&self, other: &Self) -> bool {
        unsafe { PrivateKeyIsEqual(self.c_private_key, other.c_private_key) }
    }
}

impl Eq for PrivateKey {}

impl Mul<G1Element> for PrivateKey {
    type Output = Result<G1Element, BlsError>;

    fn mul(self, rhs: G1Element) -> Self::Output {
        Ok(G1Element {
            c_element: c_err_to_result(|_| unsafe {
                G1ElementMul(rhs.c_element, self.c_private_key)
            })?,
        })
    }
}

impl Mul<PrivateKey> for G1Element {
    type Output = Result<G1Element, BlsError>;

    fn mul(self, rhs: PrivateKey) -> Self::Output {
        rhs * self
    }
}

impl PrivateKey {
    pub(crate) fn as_mut_ptr(&self) -> *mut c_void {
        self.c_private_key
    }

    // TODO Rename to from_seed
    pub fn key_gen(scheme: &impl Scheme, seed: &[u8]) -> Result<Self, BlsError> {
        Ok(PrivateKey {
            c_private_key: c_err_to_result(|did_err| unsafe {
                CoreMPLKeyGen(
                    scheme.as_mut_ptr(),
                    seed.as_ptr() as *const _,
                    seed.len(),
                    did_err,
                )
            })?,
        })
    }

    #[cfg(feature = "dash_helpers")]
    pub fn generate_dash(rng: &mut StdRng) -> Result<Self, BlsError> {
        let seed = rng.gen::<[u8; 32]>();
        let scheme = BasicSchemeMPL::new();
        Ok(PrivateKey {
            c_private_key: c_err_to_result(|did_err| unsafe {
                CoreMPLKeyGen(
                    scheme.as_mut_ptr(),
                    seed.as_ptr() as *const _,
                    seed.len(),
                    did_err,
                )
            })?,
        })
    }

    #[cfg(feature = "dash_helpers")]
    pub fn sign(&self, message: &[u8]) -> G2Element {
        self.sign_basic(message)
    }

    pub fn sign_basic(&self, message: &[u8]) -> G2Element {
        let scheme = BasicSchemeMPL::new();
        scheme.sign(self, message)
    }

    #[cfg(feature = "dash_helpers")]
    pub fn generate_dash_many(count: usize, rng: &mut StdRng) -> Result<Vec<Self>, BlsError> {
        (0..count)
            .into_iter()
            .map(|_| Self::generate_dash(rng))
            .collect()
    }

    pub fn g1_element(&self) -> Result<G1Element, BlsError> {
        Ok(G1Element {
            c_element: c_err_to_result(|did_err| unsafe {
                PrivateKeyGetG1Element(self.c_private_key, did_err)
            })?,
        })
    }

    pub fn to_bytes(&self) -> SecureBox {
        // `PrivateKeySerialize` internally securely allocates memory which we have to
        // wrap safely
        unsafe {
            SecureBox::from_ptr(
                PrivateKeySerialize(self.c_private_key) as *mut u8,
                PRIVATE_KEY_SIZE,
            )
        }
    }

    pub fn from_bytes(bytes: &[u8], mod_order: bool) -> Result<Self, BlsError> {
        if bytes.len() != PRIVATE_KEY_SIZE {
            return Err(BlsError {
                msg: format!(
                    "Private key size must be {}, got {}",
                    PRIVATE_KEY_SIZE,
                    bytes.len()
                ),
            });
        }

        let c_private_key = c_err_to_result(|did_err| unsafe {
            PrivateKeyFromBytes(bytes.as_ptr() as *const c_void, mod_order, did_err)
        })?;

        Ok(PrivateKey { c_private_key })
    }

    pub fn from_bip32_seed(bytes: &[u8]) -> Self {
        let c_private_key =
            unsafe { PrivateKeyFromSeedBIP32(bytes.as_ptr() as *const c_void, bytes.len()) };

        PrivateKey { c_private_key }
    }

    pub fn derive_child_private_key(&self, scheme: &impl Scheme, index: u32) -> PrivateKey {
        PrivateKey {
            c_private_key: unsafe {
                CoreMPLDeriveChildSk(scheme.as_mut_ptr(), self.c_private_key, index)
            },
        }
    }

    pub fn derive_child_private_key_unhardened(
        &self,
        scheme: &impl Scheme,
        index: u32,
    ) -> PrivateKey {
        PrivateKey {
            c_private_key: unsafe {
                CoreMPLDeriveChildSkUnhardened(scheme.as_mut_ptr(), self.c_private_key, index)
            },
        }
    }

    pub fn threshold_recover(
        bls_ids_with_private_keys: &[(Vec<u8>, PrivateKey)],
    ) -> Result<Self, BlsError> {
        unsafe {
            let len = bls_ids_with_private_keys.len();
            let (c_hashes, c_elements): (Vec<_>, Vec<_>) = bls_ids_with_private_keys
                .iter()
                .map(|(hash, element)| (hash.as_ptr() as *mut c_void, element.c_private_key))
                .unzip();
            let c_hashes_ptr = c_hashes.as_ptr() as *mut *mut c_void;
            let c_elements_ptr = c_elements.as_ptr() as *mut *mut c_void;
            Ok(PrivateKey {
                c_private_key: c_err_to_result(|did_err| {
                    ThresholdPrivateKeyRecover(c_elements_ptr, len, c_hashes_ptr, len, did_err)
                })?,
            })
        }
    }
}

impl Clone for PrivateKey {
    fn clone(&self) -> Self {
        // Serialize the element
        let bytes = self.to_bytes();
        // We can panic
        PrivateKey::from_bytes(bytes.as_slice(), false).expect("expected bytes to be valid")
    }
}

#[cfg(feature = "use_serde")]
// Implement Serialize trait for G1Element
impl Serialize for PrivateKey {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_bytes(self.to_bytes().as_slice())
    }
}

#[cfg(feature = "use_serde")]
// Implement Deserialize trait for G1Element
impl<'de> Deserialize<'de> for PrivateKey {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct PrivateKeyElementVisitor;

        impl<'de> serde::de::Visitor<'de> for PrivateKeyElementVisitor {
            type Value = PrivateKey;

            fn expecting(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
                formatter.write_str("a byte array representing a Private Key")
            }

            fn visit_bytes<E>(self, bytes: &[u8]) -> Result<Self::Value, E>
            where
                E: serde::de::Error,
            {
                PrivateKey::from_bytes(bytes, false).map_err(serde::de::Error::custom)
            }
        }

        deserializer.deserialize_bytes(PrivateKeyElementVisitor)
    }
}

impl Drop for PrivateKey {
    fn drop(&mut self) {
        unsafe { PrivateKeyFree(self.c_private_key) }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::schemes::AugSchemeMPL;

    #[test]
    fn serialize_deserialize() {
        let seed = b"seedweedseedweedseedweedseedweed";
        let scheme = AugSchemeMPL::new();
        let sk1 = PrivateKey::key_gen(&scheme, seed).expect("unable to generate private key");
        let sk1_bytes = sk1.to_bytes();
        let sk2 = PrivateKey::from_bytes(sk1_bytes.as_slice(), false)
            .expect("cannot build private key from bytes");

        assert_eq!(sk1, sk2);
    }

    #[test]
    fn should_return_private_key_from_bip32_bytes() {
        let long_seed = [
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8,
            9, 10, 1, 2,
        ];
        let long_private_key_test_data = [
            50, 67, 148, 112, 207, 6, 210, 118, 137, 125, 27, 144, 105, 189, 214, 228, 68, 83, 144,
            205, 80, 105, 133, 222, 14, 26, 28, 136, 167, 111, 241, 118,
        ];
        let long_private_key = PrivateKey::from_bip32_seed(&long_seed);
        assert_eq!(*long_private_key.to_bytes(), long_private_key_test_data);

        // Previously didn't work with seed with length != 32
        let short_seed = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
        let short_private_key_test_data = [
            70, 137, 28, 44, 236, 73, 89, 60, 129, 146, 30, 71, 61, 183, 72, 0, 41, 224, 252, 30,
            185, 51, 198, 185, 61, 129, 245, 55, 14, 177, 159, 189,
        ];
        let short_private_key = PrivateKey::from_bip32_seed(&short_seed);
        assert_eq!(*short_private_key.to_bytes(), short_private_key_test_data);
    }

    #[test]
    fn test_keys_multiplication() {
        // 46891c2cec49593c81921e473db7480029e0fc1eb933c6b93d81f5370eb19fbd
        let private_key_data = [
            70, 137, 28, 44, 236, 73, 89, 60, 129, 146, 30, 71, 61, 183, 72, 0, 41, 224, 252, 30,
            185, 51, 198, 185, 61, 129, 245, 55, 14, 177, 159, 189,
        ];
        // 0e2f9055c17eb13221d8b41833468ab49f7d4e874ddf4b217f5126392a608fd48ccab3510548f1da4f397c1ad4f8e01a
        let public_key_data = [
            14, 47, 144, 85, 193, 126, 177, 50, 33, 216, 180, 24, 51, 70, 138, 180, 159, 125, 78,
            135, 77, 223, 75, 33, 127, 81, 38, 57, 42, 96, 143, 212, 140, 202, 179, 81, 5, 72, 241,
            218, 79, 57, 124, 26, 212, 248, 224, 26,
        ];
        // 03fd387c4d4c66ec9dcdb31ef0c08ad881090dcda13d4b2c9cbc5ef264ff4dc7
        let expected_data = [
            3, 253, 56, 124, 77, 76, 102, 236, 157, 205, 179, 30, 240, 192, 138, 216, 129, 9, 13,
            205, 161, 61, 75, 44, 156, 188, 94, 242, 100, 255, 77, 199,
        ];
        let private_key = PrivateKey::from_bytes(&private_key_data, false).unwrap();
        let public_key = G1Element::from_bytes_legacy(&public_key_data).unwrap();
        let result = (private_key * public_key).unwrap();
        assert_eq!(
            &result.serialize_legacy()[..32],
            &expected_data,
            "should match"
        );
        let private_key = PrivateKey::from_bytes(&private_key_data, false).unwrap();
        let public_key = G1Element::from_bytes_legacy(&public_key_data).unwrap();
        let result = (public_key * private_key).unwrap();
        assert_eq!(
            &result.serialize_legacy()[..32],
            &expected_data,
            "should match"
        );
    }
}
