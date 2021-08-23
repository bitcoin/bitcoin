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
    pub fn len(&self) -> [u8; 3] {
        let l = match self {
            Self::Ref(v) => v.len().to_le_bytes(),
            Self::Owned(v) => v.len().to_le_bytes(),
        };
        [l[0], l[1], l[2]]
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
pub struct B016M<'b>(Inner<'b>);

impl<'b> TryFrom<&'b [u8]> for B016M<'b> {
    type Error = Error;

    #[inline]
    fn try_from(v: &'b [u8]) -> core::result::Result<Self, Self::Error> {
        match v.len() {
            0..=16777215 => Ok(Self(Inner::Ref(v))),
            _ => Err(Error::LenBiggerThan16M),
        }
    }
}
impl<'b> TryFrom<&'b mut [u8]> for B016M<'b> {
    type Error = Error;

    #[inline]
    fn try_from(v: &'b mut [u8]) -> core::result::Result<Self, Self::Error> {
        match v.len() {
            0..=16777215 => Ok(Self(Inner::Ref(v))),
            _ => Err(Error::LenBiggerThan16M),
        }
    }
}

impl<'b> TryFrom<Vec<u8>> for B016M<'b> {
    type Error = Error;

    fn try_from(v: Vec<u8>) -> core::result::Result<Self, Self::Error> {
        match v.len() {
            0..=16777215 => Ok(Self(Inner::Owned(v))),
            _ => Err(Error::LenBiggerThan16M),
        }
    }
}

impl<'b> Serialize for B016M<'b> {
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

struct B016MVisitor;

impl<'a> Visitor<'a> for B016MVisitor {
    type Value = B016M<'a>;

    fn expecting(&self, formatter: &mut core::fmt::Formatter) -> core::fmt::Result {
        formatter.write_str("a byte array shorter than 16M")
    }

    #[inline]
    fn visit_borrowed_bytes<E>(self, value: &'a [u8]) -> Result<Self::Value, E> {
        Ok(B016M(Inner::Ref(value)))
    }
}

impl<'de: 'a, 'a> Deserialize<'de> for B016M<'a> {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        deserializer.deserialize_newtype_struct("B016M", B016MVisitor)
    }
}

impl<'a> GetSize for B016M<'a> {
    fn get_size(&self) -> usize {
        match &self.0 {
            Inner::Ref(v) => v.len() + 3,
            Inner::Owned(v) => v.len() + 3,
        }
    }
}

impl<'a> B016M<'a> {
    pub fn get_elements_number_in_array(a: &[u8]) -> usize {
        let total_len = a.len();
        let mut next_element_index: usize = 0;
        let mut elements_number: usize = 0;
        while next_element_index < total_len {
            let len = &a[next_element_index..next_element_index + 3];
            let len = u32::from_le_bytes([len[0], len[1], len[2], 0]);
            next_element_index += len as usize + 3;
            elements_number += 1;
        }
        elements_number
    }
}
