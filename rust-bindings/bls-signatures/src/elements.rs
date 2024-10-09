use std::ffi::c_void;
use std::fmt::Debug;
use std::fmt::Formatter;

use bls_dash_sys::{CoreMPLDeriveChildPkUnhardened, G1ElementFree, G1ElementFromBytes, G1ElementGenerator, G1ElementGetFingerprint, G1ElementIsEqual, G1ElementSerialize, G1ElementCopy, G2ElementCopy, G2ElementFree, G2ElementFromBytes, G2ElementIsEqual, G2ElementSerialize, ThresholdPublicKeyRecover, ThresholdSignatureRecover};
#[cfg(feature = "use_serde")]
use serde::{Deserialize, Deserializer, Serialize, Serializer};

use crate::{schemes::Scheme, utils::c_err_to_result, BlsError, BasicSchemeMPL};

// TODO Split into modules

pub const G1_ELEMENT_SIZE: usize = 48; // TODO somehow extract it from bls library
pub const G2_ELEMENT_SIZE: usize = 96; // TODO somehow extract it from bls library

#[cfg(feature = "dash_helpers")]
pub type PublicKey = G1Element;

#[cfg(feature = "dash_helpers")]
pub type Signature = G2Element;

pub struct G1Element {
    pub(crate) c_element: *mut c_void,
}

// G1Element is immutable and thread safe
unsafe impl Send for G1Element {}
unsafe impl Sync for G1Element {}

impl PartialEq for G1Element {
    fn eq(&self, other: &Self) -> bool {
        unsafe { G1ElementIsEqual(self.c_element, other.c_element) }
    }
}

impl Debug for G1Element {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        let g1_hex = hex::encode(self.to_bytes().as_slice());

        write!(f, "G1Element({:?})", g1_hex)
    }
}

impl Eq for G1Element {}

impl G1Element {
    pub fn generate() -> Self {
        let c_element = unsafe { G1ElementGenerator() };

        G1Element { c_element }
    }

    #[cfg(feature = "dash_helpers")]
    pub fn verify(&self, signature: &G2Element, message: &[u8]) -> bool {
        self.verify_basic(signature, message)
    }

    pub fn verify_basic(&self, signature: &G2Element, message: &[u8]) -> bool {
        let basic_scheme = BasicSchemeMPL::new();
        basic_scheme.verify(self, message, signature)
    }

    pub(crate) fn from_bytes_with_legacy_flag(
        bytes: &[u8],
        legacy: bool,
    ) -> Result<Self, BlsError> {
        if bytes.len() != G1_ELEMENT_SIZE {
            return Err(BlsError {
                msg: format!(
                    "G1 Element size must be {}, got {}",
                    G1_ELEMENT_SIZE,
                    bytes.len()
                ),
            });
        }
        Ok(G1Element {
            c_element: c_err_to_result(|did_err| unsafe {
                G1ElementFromBytes(bytes.as_ptr() as *const _, legacy, did_err)
            })?,
        })
    }

    pub fn from_bytes(bytes: &[u8]) -> Result<Self, BlsError> {
        Self::from_bytes_with_legacy_flag(bytes, false)
    }

    pub(crate) fn to_bytes_with_legacy_flag(&self, legacy: bool) -> Box<[u8; G1_ELEMENT_SIZE]> {
        unsafe {
            let malloc_ptr = G1ElementSerialize(self.c_element, legacy);
            Box::from_raw(malloc_ptr as *mut _)
        }
    }

    pub fn to_bytes(&self) -> Box<[u8; G1_ELEMENT_SIZE]> {
        self.to_bytes_with_legacy_flag(false)
    }

    pub fn derive_child_public_key_unhardened(
        &self,
        scheme: &impl Scheme,
        index: u32,
    ) -> G1Element {
        G1Element {
            c_element: unsafe {
                CoreMPLDeriveChildPkUnhardened(scheme.as_mut_ptr(), self.c_element, index)
            },
        }
    }

    pub(crate) fn fingerprint_with_legacy_flag(&self, legacy: bool) -> u32 {
        unsafe { G1ElementGetFingerprint(self.c_element, legacy) }
    }

    pub fn fingerprint(&self) -> u32 {
        self.fingerprint_with_legacy_flag(false)
    }

    pub fn threshold_recover(
        bls_ids_with_elements: &[(Vec<u8>, G1Element)],
    ) -> Result<Self, BlsError> {
        unsafe {
            let len = bls_ids_with_elements.len();
            let (c_hashes, c_elements): (Vec<_>, Vec<_>) = bls_ids_with_elements
                .iter()
                .map(|(hash, element)| {
                    (
                        hash.as_ptr() as *mut c_void,
                        element.c_element as *mut c_void,
                    )
                })
                .unzip();
            let c_hashes_ptr = c_hashes.as_ptr() as *mut *mut c_void;
            let c_elements_ptr = c_elements.as_ptr() as *mut *mut c_void;
            Ok(G1Element {
                c_element: c_err_to_result(|did_err| {
                    ThresholdPublicKeyRecover(c_elements_ptr, len, c_hashes_ptr, len, did_err)
                })?,
            })
        }
    }
}

impl Clone for G1Element {
    fn clone(&self) -> Self {
        unsafe {
            G1Element{c_element: G1ElementCopy(self.c_element)}
        }
    }
}

#[cfg(feature = "use_serde")]
// Implement Serialize trait for G1Element
impl Serialize for G1Element {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        let bytes = *self.to_bytes();
        serializer.serialize_bytes(&bytes)
    }
}

#[cfg(feature = "use_serde")]
// Implement Deserialize trait for G1Element
impl<'de> Deserialize<'de> for G1Element {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct G1ElementVisitor;

        impl<'de> serde::de::Visitor<'de> for G1ElementVisitor {
            type Value = G1Element;

            fn expecting(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
                formatter.write_str("a byte array representing a G1Element")
            }

            fn visit_bytes<E>(self, bytes: &[u8]) -> Result<Self::Value, E>
            where
                E: serde::de::Error,
            {
                G1Element::from_bytes(bytes).map_err(serde::de::Error::custom)
            }
        }

        deserializer.deserialize_bytes(G1ElementVisitor)
    }
}

impl Drop for G1Element {
    fn drop(&mut self) {
        unsafe { G1ElementFree(self.c_element) }
    }
}

pub struct G2Element {
    pub(crate) c_element: *mut c_void,
}

// G2Element is immutable and thread safe
unsafe impl Send for G2Element {}
unsafe impl Sync for G2Element {}

impl PartialEq for G2Element {
    fn eq(&self, other: &Self) -> bool {
        unsafe { G2ElementIsEqual(self.c_element, other.c_element) }
    }
}

impl Debug for G2Element {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        let g2_hex = hex::encode(self.to_bytes().as_slice());

        write!(f, "G2Element({:?})", g2_hex)
    }
}

impl Eq for G2Element {}

impl G2Element {
    pub(crate) fn from_bytes_with_legacy_flag(
        bytes: &[u8],
        legacy: bool,
    ) -> Result<Self, BlsError> {
        if bytes.len() != G2_ELEMENT_SIZE {
            return Err(BlsError {
                msg: format!(
                    "G2 Element size must be {}, got {}",
                    G2_ELEMENT_SIZE,
                    bytes.len()
                ),
            });
        }
        Ok(G2Element {
            c_element: c_err_to_result(|did_err| unsafe {
                G2ElementFromBytes(bytes.as_ptr() as *const _, legacy, did_err)
            })?,
        })
    }

    pub fn from_bytes(bytes: &[u8]) -> Result<Self, BlsError> {
        Self::from_bytes_with_legacy_flag(bytes, false)
    }

    pub(crate) fn to_bytes_with_legacy_flag(&self, legacy: bool) -> Box<[u8; G2_ELEMENT_SIZE]> {
        unsafe {
            let malloc_ptr = G2ElementSerialize(self.c_element, legacy);
            Box::from_raw(malloc_ptr as *mut _)
        }
    }

    pub fn to_bytes(&self) -> Box<[u8; G2_ELEMENT_SIZE]> {
        self.to_bytes_with_legacy_flag(false)
    }

    pub fn threshold_recover(
        bls_ids_with_elements: &[(Vec<u8>, G2Element)],
    ) -> Result<Self, BlsError> {
        unsafe {
            let len = bls_ids_with_elements.len();
            let (c_hashes, c_elements): (Vec<_>, Vec<_>) = bls_ids_with_elements
                .iter()
                .map(|(hash, element)| {
                    (
                        hash.as_ptr() as *mut c_void,
                        element.c_element as *mut c_void,
                    )
                })
                .unzip();
            let c_hashes_ptr = c_hashes.as_ptr() as *mut *mut c_void;
            let c_elements_ptr = c_elements.as_ptr() as *mut *mut c_void;
            Ok(G2Element {
                c_element: c_err_to_result(|did_err| {
                    ThresholdSignatureRecover(c_elements_ptr, len, c_hashes_ptr, len, did_err)
                })?,
            })
        }
    }
}

impl Clone for G2Element {
    fn clone(&self) -> Self {
        unsafe {
            G2Element{c_element: G2ElementCopy(self.c_element)}
        }
    }
}

#[cfg(feature = "use_serde")]
// Implement Serialize trait for G1Element
impl Serialize for G2Element {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        let bytes = *self.to_bytes();
        serializer.serialize_bytes(&bytes)
    }
}

#[cfg(feature = "use_serde")]
// Implement Deserialize trait for G1Element
impl<'de> Deserialize<'de> for G2Element {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct G2ElementVisitor;

        impl<'de> serde::de::Visitor<'de> for G2ElementVisitor {
            type Value = G2Element;

            fn expecting(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
                formatter.write_str("a byte array representing a G2Element")
            }

            fn visit_bytes<E>(self, bytes: &[u8]) -> Result<Self::Value, E>
            where
                E: serde::de::Error,
            {
                G2Element::from_bytes(bytes).map_err(serde::de::Error::custom)
            }
        }

        deserializer.deserialize_bytes(G2ElementVisitor)
    }
}

impl Drop for G2Element {
    fn drop(&mut self) {
        unsafe { G2ElementFree(self.c_element) }
    }
}

#[cfg(test)]
mod tests {
    use std::thread;
    use super::*;
    use crate::{
        schemes::{AugSchemeMPL, Scheme},
        PrivateKey,
    };

    #[test]
    fn g1_serialize_deserialize() {
        let seed = b"seedweedseedweedseedweedseedweed";
        let scheme = AugSchemeMPL::new();
        let sk = PrivateKey::key_gen(&scheme, seed).expect("unable to generate private key");

        let g1 = sk.g1_element().expect("cannot get G1 element");
        let g1_bytes = g1.to_bytes();
        let g1_2 =
            G1Element::from_bytes(g1_bytes.as_ref()).expect("cannot build G1 element from bytes");

        assert_eq!(g1, g1_2);
    }

    #[test]
    fn g2_serialize_deserialize() {
        let seed = b"seedweedseedweedseedweedseedweed";
        let scheme = AugSchemeMPL::new();
        let sk = PrivateKey::key_gen(&scheme, seed).expect("unable to generate private key");

        let g2 = scheme.sign(&sk, b"ayy");
        let g2_bytes = g2.to_bytes();
        let g2_2 =
            G2Element::from_bytes(g2_bytes.as_ref()).expect("cannot build G2 element from bytes");

        assert_eq!(g2, g2_2);
    }

    #[test]
    fn should_generate_new_g1_element() {
        let g1_element = G1Element::generate();

        assert_eq!(g1_element.to_bytes().len(), 48);
    }

    #[test]
    fn should_return_fingerprint() {
        let bytes = [
            151, 241, 211, 167, 49, 151, 215, 148, 38, 149, 99, 140, 79, 169, 172, 15, 195, 104,
            140, 79, 151, 116, 185, 5, 161, 78, 58, 63, 23, 27, 172, 88, 108, 85, 232, 63, 249,
            122, 26, 239, 251, 58, 240, 10, 219, 34, 198, 187,
        ];

        let g1_element =
            G1Element::from_bytes(&bytes).expect("should create g1 element from bytes");

        assert_eq!(g1_element.fingerprint(), 2093959050);
    }

    #[test]
    fn should_be_thread_safe() {
        let bytes = [
            151, 241, 211, 167, 49, 151, 215, 148, 38, 149, 99, 140, 79, 169, 172, 15, 195, 104,
            140, 79, 151, 116, 185, 5, 161, 78, 58, 63, 23, 27, 172, 88, 108, 85, 232, 63, 249,
            122, 26, 239, 251, 58, 240, 10, 219, 34, 198, 187,
        ];

        let g1_element =
            G1Element::from_bytes(&bytes).expect("should create g1 element from bytes");

        let test_thread = thread::spawn(move|| {
            assert_eq!(g1_element.fingerprint(), 2093959050);
        });

        test_thread.join().unwrap();
    }
}
