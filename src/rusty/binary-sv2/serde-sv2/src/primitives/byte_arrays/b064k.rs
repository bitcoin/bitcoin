use crate::error::Error;
use crate::primitives::GetSize;
use alloc::vec::Vec;
use core::convert::TryFrom;
use serde::{de::Visitor, ser, ser::SerializeTuple, Deserialize, Deserializer, Serialize};

#[derive(Debug, PartialEq, Clone)]
enum Inner<'a> {
    Ref(&'a [u8]),
    Owned(Vec<u8>),
}

impl<'a> Inner<'a> {
    #[inline]
    pub fn len(&self) -> [u8; 2] {
        let l = match self {
            Self::Ref(v) => v.len().to_le_bytes(),
            Self::Owned(v) => v.len().to_le_bytes(),
        };
        [l[0], l[1]]
    }

    #[inline]
    pub fn as_ref(&'a self) -> &'a [u8] {
        match self {
            Self::Ref(v) => v,
            Self::Owned(v) => &v[..],
        }
    }
}

#[derive(Debug, PartialEq, Clone)]
pub struct B064K<'b>(Inner<'b>);

impl<'b> TryFrom<&'b [u8]> for B064K<'b> {
    type Error = Error;

    #[inline]
    fn try_from(v: &'b [u8]) -> core::result::Result<Self, Self::Error> {
        match v.len() {
            0..=65535 => Ok(Self(Inner::Ref(v))),
            _ => Err(Error::LenBiggerThan16M),
        }
    }
}

impl<'b> TryFrom<&'b mut [u8]> for B064K<'b> {
    type Error = Error;

    #[inline]
    fn try_from(v: &'b mut [u8]) -> core::result::Result<Self, Self::Error> {
        match v.len() {
            0..=65535 => Ok(Self(Inner::Ref(v))),
            _ => Err(Error::LenBiggerThan16M),
        }
    }
}

impl<'b> TryFrom<Vec<u8>> for B064K<'b> {
    type Error = Error;

    fn try_from(v: Vec<u8>) -> core::result::Result<Self, Self::Error> {
        match v.len() {
            0..=65535 => Ok(Self(Inner::Owned(v))),
            _ => Err(Error::LenBiggerThan16M),
        }
    }
}

impl<'b> Serialize for B064K<'b> {
    #[inline]
    fn serialize<S>(&self, serializer: S) -> core::result::Result<S::Ok, S::Error>
    where
        S: ser::Serializer,
    {
        let len = self.0.len();
        let inner = self.0.as_ref();

        // tuple is: (byte array len, byte array)
        let tuple = (len, &inner);

        let tuple_len = 2;
        let mut seq = serializer.serialize_tuple(tuple_len)?;

        seq.serialize_element(&tuple.0)?;
        seq.serialize_element(tuple.1)?;
        seq.end()
    }
}

struct B064KVisitor;

impl<'a> Visitor<'a> for B064KVisitor {
    type Value = B064K<'a>;

    fn expecting(&self, formatter: &mut core::fmt::Formatter) -> core::fmt::Result {
        formatter.write_str("a byte array shorter than 64K")
    }

    #[inline]
    fn visit_borrowed_bytes<E>(self, value: &'a [u8]) -> Result<Self::Value, E> {
        Ok(B064K(Inner::Ref(value)))
    }
}

impl<'de: 'a, 'a> Deserialize<'de> for B064K<'a> {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        deserializer.deserialize_newtype_struct("B064K", B064KVisitor)
    }
}

impl<'a> GetSize for B064K<'a> {
    fn get_size(&self) -> usize {
        match &self.0 {
            Inner::Ref(v) => v.len() + 2,
            Inner::Owned(v) => v.len() + 2,
        }
    }
}
