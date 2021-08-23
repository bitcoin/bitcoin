use crate::primitives::FixedSize;
use core::convert::TryFrom;
use serde::{de::Visitor, ser, Deserialize, Deserializer, Serialize};

#[derive(Debug, PartialEq, Copy, Clone)]
pub struct U24(pub(crate) u32);

impl U24 {
    const MAX: u32 = 16777215;
}

impl From<U24> for u32 {
    #[inline]
    fn from(v: U24) -> Self {
        v.0
    }
}

impl From<&U24> for u32 {
    #[inline]
    fn from(v: &U24) -> Self {
        v.0
    }
}

impl TryFrom<u32> for U24 {
    type Error = crate::Error;

    fn try_from(v: u32) -> Result<Self, Self::Error> {
        match v {
            0..=Self::MAX => Ok(Self(v)),
            _ => Err(crate::Error::U24TooBig(v)),
        }
    }
}

use core::convert::TryInto;

impl TryFrom<usize> for U24 {
    type Error = crate::Error;

    fn try_from(v: usize) -> Result<Self, Self::Error> {
        let v: u32 = v
            .try_into()
            .map_err(|_| crate::Error::U24TooBig(u32::MAX))?;
        match v {
            0..=Self::MAX => Ok(Self(v)),
            _ => Err(crate::Error::U24TooBig(v)),
        }
    }
}

impl From<U24> for usize {
    fn from(v: U24) -> Self {
        v.0 as usize
    }
}

impl Serialize for U24 {
    #[inline]
    fn serialize<S>(&self, serializer: S) -> core::result::Result<S::Ok, S::Error>
    where
        S: ser::Serializer,
    {
        serializer.serialize_bytes(&self.0.to_le_bytes()[0..=2])
    }
}

struct U24Visitor;

impl<'de> Visitor<'de> for U24Visitor {
    type Value = U24;

    fn expecting(&self, formatter: &mut core::fmt::Formatter) -> core::fmt::Result {
        formatter.write_str("an integer between 0 and 2^24 3 bytes le")
    }

    #[inline]
    fn visit_u32<E>(self, value: u32) -> Result<Self::Value, E> {
        // This is safe as this struct is deserialized using parse_u24 that can never return a
        // value bigger than U24::MAX
        Ok(U24(value))
    }
}

impl<'de> Deserialize<'de> for U24 {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        deserializer.deserialize_newtype_struct("U24", U24Visitor)
    }
}

impl FixedSize for U24 {
    const FIXED_SIZE: usize = 3;
}
