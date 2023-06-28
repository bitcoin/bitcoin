use crate::{
    bip32::{ExtendedPrivateKey, ExtendedPublicKey},
    BlsError,
};

impl ExtendedPrivateKey {
    pub fn private_child_legacy(&self, index: u32) -> Self {
        self.private_child_with_legacy_flag(index, true)
    }

    pub fn extended_public_key_legacy(&self) -> Result<ExtendedPublicKey, BlsError> {
        self.extended_public_key_with_legacy_flag(true)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn serialize_deserialize_legacy() {
        let seed = b"seedweedseedweedseedweedseedweed";
        let private_key =
            ExtendedPrivateKey::from_seed(seed).expect("cannot generate extended private key");
        let public_key = private_key
            .extended_public_key_legacy()
            .expect("cannot get extended public key");

        let public_key_bytes = public_key.serialize_legacy();
        let public_key_2 = ExtendedPublicKey::from_bytes_legacy(public_key_bytes.as_ref())
            .expect("cannot deserialize extended public key");

        assert_eq!(public_key, public_key_2);
    }

    #[test]
    fn hierarchical_deterministic_keys_legacy() {
        let seed = b"seedweedseedweedseedweedseedweed";
        let private_key =
            ExtendedPrivateKey::from_seed(seed).expect("cannot generate extended private key");
        let public_key = private_key
            .extended_public_key_legacy()
            .expect("cannot get extended public key");

        let private_child = private_key.private_child_legacy(1337);
        let private_grandchild = private_child.private_child_legacy(420);

        let public_child = public_key.public_child_legacy(1337);
        let public_grandchild = public_child.public_child_legacy(420);

        assert_eq!(
            public_grandchild,
            private_grandchild
                .extended_public_key_legacy()
                .expect("cannot get extended public key")
        );
    }
}
