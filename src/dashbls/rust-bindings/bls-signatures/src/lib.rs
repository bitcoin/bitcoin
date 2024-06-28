mod elements;
mod private_key;
mod schemes;
mod utils;

#[cfg(feature = "legacy")]
mod legacy;

#[cfg(feature = "bip32")]
pub mod bip32;

use std::{error::Error, fmt::Display};

pub use elements::{G1Element, G2Element, G1_ELEMENT_SIZE, G2_ELEMENT_SIZE};
#[cfg(feature = "dash_helpers")]
pub use elements::{PublicKey, Signature};
pub use private_key::{PrivateKey, PRIVATE_KEY_SIZE};
pub use schemes::{AugSchemeMPL, BasicSchemeMPL, LegacySchemeMPL, Scheme};

#[derive(Debug, PartialEq)]
pub struct BlsError {
    // Need to use owned version as each time BLS has an error its binding glue overwrites error
    // message variable.
    msg: String,
}

impl Display for BlsError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.msg)
    }
}

impl Error for BlsError {}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::schemes::{AugSchemeMPL, Scheme};

    #[test]
    fn basic_sign() {
        let seed = b"seedweedseedweedseedweedseedweed";
        let bad_seed = b"weedseedweedseedweedseedweedseed";

        let scheme = AugSchemeMPL::new();

        let private_key_before =
            PrivateKey::key_gen(&scheme, seed).expect("unable to generate private key");

        // Also test private key serialization
        let private_key_bytes = private_key_before.to_bytes();
        let private_key = PrivateKey::from_bytes(private_key_bytes.as_slice(), false)
            .expect("cannot build private key from bytes");
        drop(private_key_bytes);

        let public_key = private_key.g1_element().expect("unable to get public key");

        let private_key_bad =
            PrivateKey::key_gen(&scheme, bad_seed).expect("unable to generate private key");
        let public_key_bad = private_key_bad
            .g1_element()
            .expect("unable to get public key");

        let message = b"Evgeny owns 1337 dash no cap";

        let signature = scheme.sign(&private_key, message);
        let verify = scheme.verify(&public_key, message, &signature);
        assert!(verify);
        let verify_bad = scheme.verify(&public_key_bad, message, &signature);
        assert!(!verify_bad);
    }

    #[test]
    fn bad_seed() {
        let seed = b"lol";
        let scheme = AugSchemeMPL::new();
        let private_key = PrivateKey::key_gen(&scheme, seed);

        assert!(matches!(
            private_key,
            Err(BlsError { msg }) if msg == "Seed size must be at least 32 bytes"
        ));
    }

    #[test]
    fn hd_keys_deterministic() {
        let seed = b"seedweedseedweedseedweedseedweed";
        let scheme = AugSchemeMPL::new();

        let master_sk = PrivateKey::key_gen(&scheme, seed).expect("unable to generate private key");
        let master_pk = master_sk.g1_element().expect("unable to get public key");

        let child_sk_u = master_sk.derive_child_private_key_unhardened(&scheme, 22);
        let grandchild_sk_u = child_sk_u.derive_child_private_key_unhardened(&scheme, 0);

        let child_pk_u = master_pk.derive_child_public_key_unhardened(&scheme, 22);
        let grandchild_pk_u = child_pk_u.derive_child_public_key_unhardened(&scheme, 0);

        assert_eq!(
            grandchild_pk_u,
            grandchild_sk_u.g1_element().expect("cannot get public key")
        );
    }
}
