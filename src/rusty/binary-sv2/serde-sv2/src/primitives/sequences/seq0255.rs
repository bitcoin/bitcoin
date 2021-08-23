use super::super::{Signature, U24, U256};
use super::{Seq, SeqMaxLen, SeqVisitor, TryFromBSlice};
use crate::primitives::FixedSize;
use crate::primitives::GetSize;
use crate::Error;
use alloc::vec::Vec;
use serde::{ser, ser::SerializeTuple, Deserialize, Deserializer, Serialize};

#[derive(Debug, Clone)]
pub struct Seq0255<'s, T: Serialize + TryFromBSlice<'s> + Clone> {
    seq: Option<Seq<'s, T>>,
    data: Option<Vec<T>>,
}

impl<'s, T: Clone + Serialize + TryFromBSlice<'s> + core::cmp::PartialEq> PartialEq
    for Seq0255<'s, T>
{
    fn eq(&self, other: &Self) -> bool {
        match (&self.seq, &self.data, &other.seq, &other.data) {
            (Some(seq1), _, Some(seq2), _) => seq1 == seq2,
            (_, Some(data1), _, Some(data2)) => data1 == data2,
            _ => crate::ser::to_bytes(&self) == crate::ser::to_bytes(&other),
        }
    }
}

impl<'s, T: Clone + Serialize + TryFromBSlice<'s>> Seq0255<'s, T> {
    #[inline]
    pub fn new(data: Vec<T>) -> Result<Self, Error> {
        if data.len() > 255 {
            Err(Error::LenBiggerThan255)
        } else {
            Ok(Seq0255 {
                seq: None,
                data: Some(data),
            })
        }
    }
}

impl<'s, T: Clone + Serialize + TryFromBSlice<'s>> From<Seq<'s, T>> for Seq0255<'s, T> {
    #[inline]
    fn from(val: Seq<'s, T>) -> Self {
        Self {
            seq: Some(val),
            data: None,
        }
    }
}

impl<'s, T: Clone + Serialize + TryFromBSlice<'s>> Serialize for Seq0255<'s, T> {
    #[inline]
    fn serialize<S>(&self, serializer: S) -> core::result::Result<S::Ok, S::Error>
    where
        S: ser::Serializer,
    {
        match (&self.seq, &self.data) {
            (Some(seq), None) => {
                let len = seq.data.len() / seq.size as usize;
                let tuple = (len as u8, seq.data);
                let mut seq = serializer.serialize_tuple(2)?;
                seq.serialize_element(&tuple.0)?;
                seq.serialize_element(tuple.1)?;
                seq.end()
            }
            (None, Some(data)) => {
                let tuple = (data.len() as u8, &data[..]);
                let mut seq = serializer.serialize_tuple(2)?;
                seq.serialize_element(&tuple.0)?;
                seq.serialize_element(tuple.1)?;
                seq.end()
            }
            _ => panic!(),
        }
    }
}

impl<'de: 'a, 'a> Deserialize<'de> for Seq0255<'a, U256<'a>> {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        deserializer
            .deserialize_newtype_struct(
                "Seq_0255_U256",
                SeqVisitor {
                    inner_type_size: 32,
                    max_len: SeqMaxLen::_1B,
                    _a: core::marker::PhantomData,
                },
            )
            .map(|x| x.into())
    }
}

impl<'de: 'a, 'a> Deserialize<'de> for Seq0255<'a, bool> {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        deserializer
            .deserialize_newtype_struct(
                "Seq_0255_Bool",
                SeqVisitor {
                    inner_type_size: 1,
                    max_len: SeqMaxLen::_1B,
                    _a: core::marker::PhantomData,
                },
            )
            .map(|x| x.into())
    }
}

impl<'de: 'a, 'a> Deserialize<'de> for Seq0255<'a, u16> {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        deserializer
            .deserialize_newtype_struct(
                "Seq_0255_U16",
                SeqVisitor {
                    inner_type_size: 2,
                    max_len: SeqMaxLen::_1B,
                    _a: core::marker::PhantomData,
                },
            )
            .map(|x| x.into())
    }
}

impl<'de: 'a, 'a> Deserialize<'de> for Seq0255<'a, U24> {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        deserializer
            .deserialize_newtype_struct(
                "Seq_0255_U24",
                SeqVisitor {
                    inner_type_size: 3,
                    max_len: SeqMaxLen::_1B,
                    _a: core::marker::PhantomData,
                },
            )
            .map(|x| x.into())
    }
}

impl<'de: 'a, 'a> Deserialize<'de> for Seq0255<'a, u32> {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        deserializer
            .deserialize_newtype_struct(
                "Seq_0255_U32",
                SeqVisitor {
                    inner_type_size: 4,
                    max_len: SeqMaxLen::_1B,
                    _a: core::marker::PhantomData,
                },
            )
            .map(|x| x.into())
    }
}

impl<'de: 'a, 'a> Deserialize<'de> for Seq0255<'a, Signature<'a>> {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        deserializer
            .deserialize_newtype_struct(
                "Seq_0255_Signature",
                SeqVisitor {
                    inner_type_size: 64,
                    max_len: SeqMaxLen::_1B,
                    _a: core::marker::PhantomData,
                },
            )
            .map(|x| x.into())
    }
}

impl<'a, T: Clone + FixedSize + Serialize + TryFromBSlice<'a>> GetSize for Seq0255<'a, T> {
    fn get_size(&self) -> usize {
        if self.data.is_some() {
            (self.data.as_ref().unwrap().len() * T::FIXED_SIZE) + 1
        } else {
            self.seq.as_ref().unwrap().data.len() + 1
        }
    }
}
