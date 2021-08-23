use crate::primitives::GetSize;
use alloc::vec::Vec;
use serde::{de::Visitor, ser, Deserialize, Deserializer, Serialize};

#[derive(Debug, PartialEq, Clone)]
enum Inner<'a> {
    Ref(&'a [u8]),
    #[allow(dead_code)]
    Owned(Vec<u8>),
}

impl<'a> Inner<'a> {
    #[inline]
    pub fn as_ref(&'a self) -> &'a [u8] {
        match self {
            Self::Ref(v) => v,
            Self::Owned(v) => &v[..],
        }
    }
}

#[derive(Debug, PartialEq, Clone)]
pub struct Bytes<'b>(Inner<'b>);

impl<'b> From<&'b [u8]> for Bytes<'b> {
    #[inline]
    fn from(v: &'b [u8]) -> Self {
        Self(Inner::Ref(v))
    }
}
impl<'b> From<&'b mut [u8]> for Bytes<'b> {
    #[inline]
    fn from(v: &'b mut [u8]) -> Self {
        Self(Inner::Ref(v))
    }
}

impl<'b> Serialize for Bytes<'b> {
    #[inline]
    fn serialize<S>(&self, serializer: S) -> core::result::Result<S::Ok, S::Error>
    where
        S: ser::Serializer,
    {
        serializer.serialize_bytes(self.0.as_ref())
    }
}

struct BytesVisitor;

impl<'a> Visitor<'a> for BytesVisitor {
    type Value = Bytes<'a>;

    fn expecting(&self, formatter: &mut core::fmt::Formatter) -> core::fmt::Result {
        formatter.write_str("a byte array shorter than 64K")
    }

    #[inline]
    fn visit_borrowed_bytes<E>(self, value: &'a [u8]) -> Result<Self::Value, E> {
        Ok(Bytes(Inner::Ref(value)))
    }
}

impl<'de: 'a, 'a> Deserialize<'de> for Bytes<'a> {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        deserializer.deserialize_newtype_struct("Bytes", BytesVisitor)
    }
}

impl<'a> GetSize for Bytes<'a> {
    fn get_size(&self) -> usize {
        match &self.0 {
            Inner::Ref(v) => v.len(),
            Inner::Owned(v) => v.len(),
        }
    }
}
