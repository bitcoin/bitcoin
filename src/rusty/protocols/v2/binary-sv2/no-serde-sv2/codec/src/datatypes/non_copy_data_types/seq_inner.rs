use crate::codec::decodable::{Decodable, DecodableField, FieldMarker, GetMarker, PrimitiveMarker};
use crate::codec::encodable::{EncodableField, EncodablePrimitive};
use crate::codec::GetSize;
use crate::datatypes::Sv2DataType;
use crate::datatypes::*;
use crate::Error;
use core::marker::PhantomData;

#[cfg(not(feature = "no_std"))]
use std::io::Read;

/// The liftime is here only for type compatibility with serde-sv2
#[repr(C)]
#[derive(Debug, Clone, Eq, PartialEq)]
pub struct Seq0255<'a, T>(pub(crate) Vec<T>, PhantomData<&'a T>);

impl<'a, T: 'a> Seq0255<'a, T> {
    const HEADERSIZE: usize = 1;

    /// Return the len of the inner vector
    fn expected_len(data: &[u8]) -> Result<usize, Error> {
        if data.len() >= Self::HEADERSIZE {
            Ok(data[0] as usize)
        } else {
            Err(Error::ReadError(data.len(), Self::HEADERSIZE))
        }
    }

    pub fn new(inner: Vec<T>) -> Result<Self, Error> {
        if inner.len() <= 255 {
            Ok(Self(inner, PhantomData))
        } else {
            Err(Error::Todo)
        }
    }

    //pub fn try_from_slice(inner: &'a mut [T]) -> Result<Self, Error> {
    //    if inner.len() <= 255 {
    //        let inner_: Vec<T> = vec![];
    //        for v in inner {
    //            inner_.push(v);
    //        }
    //        Ok(Self(inner_, PhantomData))
    //    } else {
    //        Err(Error::Todo)
    //    }
    //}
}

impl<'a, T: GetSize> GetSize for Seq0255<'a, T> {
    fn get_size(&self) -> usize {
        let mut size = Self::HEADERSIZE;
        for with_size in &self.0 {
            size += with_size.get_size()
        }
        size
    }
}

/// The liftime is here only for type compatibility with serde-sv2
#[derive(Debug, Clone, Eq, PartialEq)]
pub struct Seq064K<'a, T>(pub(crate) Vec<T>, PhantomData<&'a T>);

impl<'a, T: 'a> Seq064K<'a, T> {
    const HEADERSIZE: usize = 2;

    /// Return the len of the inner vector
    fn expected_len(data: &[u8]) -> Result<usize, Error> {
        if data.len() >= Self::HEADERSIZE {
            Ok(u16::from_le_bytes([data[0], data[1]]) as usize)
        } else {
            Err(Error::ReadError(data.len(), Self::HEADERSIZE))
        }
    }

    pub fn new(inner: Vec<T>) -> Result<Self, Error> {
        if inner.len() <= 65535 {
            Ok(Self(inner, PhantomData))
        } else {
            Err(Error::Todo)
        }
    }
}

impl<'a, T: GetSize> GetSize for Seq064K<'a, T> {
    fn get_size(&self) -> usize {
        let mut size = Self::HEADERSIZE;
        for with_size in &self.0 {
            size += with_size.get_size()
        }
        size
    }
}

macro_rules! impl_codec_for_sequence {
    ($a:ty) => {
        impl<'a, T: 'a + Sv2DataType<'a> + GetMarker + GetSize + Decodable<'a>> Decodable<'a>
            for $a
        {
            fn get_structure(
                data: &[u8],
            ) -> Result<Vec<crate::codec::decodable::FieldMarker>, Error> {
                let len = Self::expected_len(data)?;
                let mut inner = Vec::with_capacity(len + Self::HEADERSIZE);
                for _ in 0..Self::HEADERSIZE {
                    inner.push(FieldMarker::Primitive(PrimitiveMarker::U8));
                }
                let inner_type = T::get_marker();
                inner.resize(len + Self::HEADERSIZE, inner_type);
                Ok(inner)
            }

            fn from_decoded_fields(
                data: Vec<crate::codec::decodable::DecodableField<'a>>,
            ) -> Result<Self, Error> {
                let mut inner: Vec<T> = Vec::with_capacity(data.len());
                let mut i = 0;
                for element in data {
                    if i >= Self::HEADERSIZE {
                        match element {
                            DecodableField::Primitive(p) => {
                                let element =
                                    T::from_decoded_fields(vec![DecodableField::Primitive(p)]);
                                inner.push(element?)
                            }
                            DecodableField::Struct(_) => unimplemented!(),
                        }
                    }
                    i += 1;
                }
                Ok(Self(inner, PhantomData))
            }

            fn from_bytes(data: &'a mut [u8]) -> Result<Self, Error> {
                let len = Self::expected_len(data)?;

                let mut inner = Vec::new();
                let mut tail = &mut data[Self::HEADERSIZE..];

                for _ in 0..len {
                    let element_size = T::size_hint(tail, 0)?;
                    let (head, t) = tail.split_at_mut(element_size);
                    tail = t;
                    inner.push(T::from_bytes_unchecked(head));
                }
                Ok(Self(inner, PhantomData))
            }

            #[cfg(not(feature = "no_std"))]
            fn from_reader(reader: &mut impl Read) -> Result<Self, Error> {
                let mut header = vec![0; Self::HEADERSIZE];
                reader.read_exact(&mut header)?;

                let len = Self::expected_len(&header)?;

                let mut inner = Vec::new();

                for _ in 0..len {
                    inner.push(T::from_reader_(reader)?);
                }
                Ok(Self(inner, PhantomData))
            }
        }
    };
}

impl_codec_for_sequence!(Seq0255<'a, T>);
impl_codec_for_sequence!(Seq064K<'a, T>);

macro_rules! impl_into_encodable_field_for_seq {
    ($a:ty) => {
        impl<'a> From<Seq064K<'a, $a>> for EncodableField<'a> {
            fn from(v: Seq064K<'a, $a>) -> Self {
                let inner_len = v.0.len() as u16;
                let mut as_encodable: Vec<EncodableField> =
                    Vec::with_capacity((inner_len + 2) as usize);
                as_encodable.push(EncodableField::Primitive(EncodablePrimitive::OwnedU8(
                    inner_len.to_le_bytes()[0],
                )));
                as_encodable.push(EncodableField::Primitive(EncodablePrimitive::OwnedU8(
                    inner_len.to_le_bytes()[1],
                )));
                for element in v.0 {
                    as_encodable.push(element.into());
                }
                EncodableField::Struct(as_encodable)
            }
        }

        impl<'a> From<Seq0255<'a, $a>> for EncodableField<'a> {
            fn from(v: Seq0255<$a>) -> Self {
                let inner_len = v.0.len() as u8;
                let mut as_encodable: Vec<EncodableField> =
                    Vec::with_capacity((inner_len + 1) as usize);
                as_encodable.push(EncodableField::Primitive(EncodablePrimitive::OwnedU8(
                    inner_len,
                )));
                for element in v.0 {
                    as_encodable.push(element.into());
                }
                EncodableField::Struct(as_encodable)
            }
        }
    };
}

impl_into_encodable_field_for_seq!(bool);
impl_into_encodable_field_for_seq!(u8);
impl_into_encodable_field_for_seq!(u16);
impl_into_encodable_field_for_seq!(U24);
impl_into_encodable_field_for_seq!(u32);
impl_into_encodable_field_for_seq!(u64);
impl_into_encodable_field_for_seq!(U256<'a>);
impl_into_encodable_field_for_seq!(Signature<'a>);
impl_into_encodable_field_for_seq!(B0255<'a>);
impl_into_encodable_field_for_seq!(B064K<'a>);
impl_into_encodable_field_for_seq!(B016M<'a>);

#[cfg(feature = "prop_test")]
impl<'a, T> std::convert::TryFrom<Seq0255<'a, T>> for Vec<T> {
    type Error = &'static str;
    fn try_from(v: Seq0255<'a, T>) -> Result<Self, Self::Error> {
        if v.0.len() > 255 {
            Ok(v.0)
        } else {
            Err("Incorrect length, expected 225")
        }
    }
}

#[cfg(feature = "prop_test")]
impl<'a, T> std::convert::TryFrom<Seq064K<'a, T>> for Vec<T> {
    type Error = &'static str;
    fn try_from(v: Seq064K<'a, T>) -> Result<Self, Self::Error> {
        if v.0.len() > 64 {
            Ok(v.0)
        } else {
            Err("Incorrect length, expected 64")
        }
    }
}

impl<'a, T> From<Vec<T>> for Seq0255<'a, T> {
    fn from(v: Vec<T>) -> Self {
        Seq0255(v, PhantomData)
    }
}

impl<'a, T> From<Vec<T>> for Seq064K<'a, T> {
    fn from(v: Vec<T>) -> Self {
        Seq064K(v, PhantomData)
    }
}
