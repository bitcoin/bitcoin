use crate::{
    bip32::{ExtendedPublicKey, BIP32_EXTENDED_PUBLIC_KEY_SIZE},
    BlsError,
};

impl ExtendedPublicKey {
    pub fn from_bytes_legacy(bytes: &[u8]) -> Result<Self, BlsError> {
        Self::from_bytes_with_legacy_flag(bytes, true)
    }

    pub fn public_child_legacy(&self, index: u32) -> Self {
        self.public_child_with_legacy_flag(index, true)
    }

    pub fn serialize_legacy(&self) -> Box<[u8; BIP32_EXTENDED_PUBLIC_KEY_SIZE]> {
        self.serialize_with_legacy_flag(true)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::bip32::ExtendedPrivateKey;

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
}
