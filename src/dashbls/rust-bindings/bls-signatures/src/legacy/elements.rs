use crate::{BlsError, G1Element, G2Element, G1_ELEMENT_SIZE, G2_ELEMENT_SIZE};

impl G1Element {
    pub fn serialize_legacy(&self) -> Box<[u8; G1_ELEMENT_SIZE]> {
        self.to_bytes_with_legacy_flag(true)
    }

    pub fn from_bytes_legacy(bytes: &[u8]) -> Result<Self, BlsError> {
        Self::from_bytes_with_legacy_flag(bytes, true)
    }

    pub fn fingerprint_legacy(&self) -> u32 {
        self.fingerprint_with_legacy_flag(true)
    }
}

impl G2Element {
    pub fn from_bytes_legacy(bytes: &[u8]) -> Result<Self, BlsError> {
        Self::from_bytes_with_legacy_flag(bytes, true)
    }

    pub fn serialize_legacy(&self) -> Box<[u8; G2_ELEMENT_SIZE]> {
        self.to_bytes_with_legacy_flag(true)
    }
}
