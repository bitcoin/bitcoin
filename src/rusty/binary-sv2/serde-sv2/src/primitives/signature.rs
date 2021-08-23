use crate::error::Error;
use crate::primitives::FixedSize;
use alloc::boxed::Box;
use core::convert::TryFrom;
use serde::{de::Visitor, ser, Deserialize, Deserializer, Serialize};

#[derive(Debug, PartialEq, Clone)]
enum Inner<'a> {
    Ref(&'a [u8]),
    Owned(Box<[u8; 64]>),
}

#[derive(Debug, PartialEq, Clone)]
pub struct Signature<'u>(Inner<'u>);

impl<'u> TryFrom<&'u [u8]> for Signature<'u> {
    type Error = Error;

    #[inline]
    fn try_from(v: &'u [u8]) -> core::result::Result<Self, Error> {
        if v.len() == 64 {
            Ok(Self(Inner::Ref(v)))
        } else {
            Err(Error::InvalidSignatureSize(v.len()))
        }
    }
}
impl<'u> TryFrom<&'u mut [u8]> for Signature<'u> {
    type Error = Error;

    #[inline]
    fn try_from(v: &'u mut [u8]) -> core::result::Result<Self, Error> {
        if v.len() == 64 {
            Ok(Self(Inner::Ref(v)))
        } else {
            Err(Error::InvalidSignatureSize(v.len()))
        }
    }
}

impl<'u> From<[u8; 64]> for Signature<'u> {
    fn from(v: [u8; 64]) -> Self {
        Signature(Inner::Owned(Box::new(v)))
    }
}

impl<'u> From<&'u Signature<'u>> for &'u [u8] {
    #[inline]
    fn from(v: &'u Signature<'u>) -> Self {
        match &v.0 {
            Inner::Ref(v) => v,
            Inner::Owned(v) => &v[..],
        }
    }
}

impl<'u> Serialize for Signature<'u> {
    #[inline]
    fn serialize<S>(&self, serializer: S) -> core::result::Result<S::Ok, S::Error>
    where
        S: ser::Serializer,
    {
        serializer.serialize_bytes(self.into())
    }
}

struct SignatureVisitor;

impl<'a> Visitor<'a> for SignatureVisitor {
    type Value = Signature<'a>;

    fn expecting(&self, formatter: &mut core::fmt::Formatter) -> core::fmt::Result {
        formatter.write_str("a 64 bytes unsigned le int")
    }

    #[inline]
    fn visit_borrowed_bytes<E>(self, value: &'a [u8]) -> Result<Self::Value, E> {
        Ok(Signature(Inner::Ref(value)))
    }
}

impl<'de: 'a, 'a> Deserialize<'de> for Signature<'a> {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        deserializer.deserialize_newtype_struct("Signature", SignatureVisitor)
    }
}

impl<'a> FixedSize for Signature<'a> {
    const FIXED_SIZE: usize = 64;
}
