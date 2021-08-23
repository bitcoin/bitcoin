use crate::codec::{GetSize, SizeHint};
use crate::datatypes::{Bytes, Signature, Sv2DataType, B016M, B0255, B032, B064K, U24, U256};
use crate::Error;
use alloc::vec::Vec;
#[cfg(not(feature = "no_std"))]
use std::io::{Cursor, Read};

/// Implmented by all the decodable structure, it can be derived for every structure composed only
/// by primitives or other Decodable.
pub trait Decodable<'a>: Sized {
    fn get_structure(data: &[u8]) -> Result<Vec<FieldMarker>, Error>;

    fn from_decoded_fields(data: Vec<DecodableField<'a>>) -> Result<Self, Error>;

    fn from_bytes(data: &'a mut [u8]) -> Result<Self, Error> {
        let structure = Self::get_structure(data)?;
        let mut fields = Vec::new();
        let mut tail = data;

        for field in structure {
            let field_size = field.size_hint_(tail, 0)?;
            let (head, t) = tail.split_at_mut(field_size);
            tail = t;
            fields.push(field.decode(head)?);
        }
        Self::from_decoded_fields(fields)
    }

    #[cfg(not(feature = "no_std"))]
    fn from_reader(reader: &mut impl Read) -> Result<Self, Error> {
        let mut data = Vec::new();
        reader.read_to_end(&mut data)?;

        let structure = Self::get_structure(&data[..])?;

        let mut fields = Vec::new();
        let mut reader = Cursor::new(data);

        for field in structure {
            fields.push(field.from_reader(&mut reader)?);
        }
        Self::from_decoded_fields(fields)
    }
}

/// Passed to a decoder to define the structure of the data to be decoded
#[derive(Debug, Clone, Copy)]
pub enum PrimitiveMarker {
    U8,
    U16,
    Bool,
    U24,
    U256,
    Signature,
    U32,
    F32,
    U64,
    B032,
    B0255,
    B064K,
    B016M,
    Bytes,
}

/// Passed to a decoder to define the structure of the data to be decoded
#[derive(Debug, Clone)]
pub enum FieldMarker {
    Primitive(PrimitiveMarker),
    Struct(Vec<FieldMarker>),
}
pub trait GetMarker {
    fn get_marker() -> FieldMarker;
}

/// Used to contrustuct primitives is returned by the decoder
#[derive(Debug)]
pub enum DecodablePrimitive<'a> {
    U8(u8),
    U16(u16),
    Bool(bool),
    U24(U24),
    U256(U256<'a>),
    Signature(Signature<'a>),
    U32(u32),
    F32(f32),
    U64(u64),
    B032(B032<'a>),
    B0255(B0255<'a>),
    B064K(B064K<'a>),
    B016M(B016M<'a>),
    Bytes(Bytes<'a>),
}

/// Used to contrustuct messages is returned by the decoder
#[derive(Debug)]
pub enum DecodableField<'a> {
    Primitive(DecodablePrimitive<'a>),
    Struct(Vec<DecodableField<'a>>),
}

impl SizeHint for PrimitiveMarker {
    fn size_hint(_data: &[u8], _offset: usize) -> Result<usize, Error> {
        unimplemented!()
    }

    fn size_hint_(&self, data: &[u8], offset: usize) -> Result<usize, Error> {
        match self {
            Self::U8 => u8::size_hint(data, offset),
            Self::U16 => u16::size_hint(data, offset),
            Self::Bool => bool::size_hint(data, offset),
            Self::U24 => U24::size_hint(data, offset),
            Self::U256 => U256::size_hint(data, offset),
            Self::Signature => Signature::size_hint(data, offset),
            Self::U32 => u32::size_hint(data, offset),
            Self::F32 => f32::size_hint(data, offset),
            Self::U64 => u64::size_hint(data, offset),
            Self::B032 => B032::size_hint(data, offset),
            Self::B0255 => B0255::size_hint(data, offset),
            Self::B064K => B064K::size_hint(data, offset),
            Self::B016M => B016M::size_hint(data, offset),
            Self::Bytes => Bytes::size_hint(data, offset),
        }
    }
}

impl SizeHint for FieldMarker {
    fn size_hint(_data: &[u8], _offset: usize) -> Result<usize, Error> {
        unimplemented!()
    }

    fn size_hint_(&self, data: &[u8], offset: usize) -> Result<usize, Error> {
        match self {
            Self::Primitive(p) => p.size_hint_(data, offset),
            Self::Struct(ps) => {
                let mut size = 0;
                for p in ps {
                    size += p.size_hint_(data, offset + size)?;
                }
                Ok(size)
            }
        }
    }
}

impl SizeHint for Vec<FieldMarker> {
    fn size_hint(_data: &[u8], _offset: usize) -> Result<usize, Error> {
        unimplemented!()
    }

    fn size_hint_(&self, data: &[u8], offset: usize) -> Result<usize, Error> {
        let mut size = 0;
        for field in self {
            let field_size = field.size_hint_(data, offset + size)?;
            size += field_size;
        }
        Ok(size)
    }
}

impl From<PrimitiveMarker> for FieldMarker {
    fn from(v: PrimitiveMarker) -> Self {
        FieldMarker::Primitive(v)
    }
}

impl From<Vec<FieldMarker>> for FieldMarker {
    fn from(mut v: Vec<FieldMarker>) -> Self {
        match v.len() {
            0 => panic!("TODO"),
            1 => v.pop().unwrap(),
            _ => FieldMarker::Struct(v),
        }
    }
}

impl<'a> From<DecodableField<'a>> for Vec<DecodableField<'a>> {
    fn from(v: DecodableField<'a>) -> Self {
        match v {
            DecodableField::Primitive(p) => vec![DecodableField::Primitive(p)],
            DecodableField::Struct(ps) => ps,
        }
    }
}

impl PrimitiveMarker {
    fn decode<'a>(&self, data: &'a mut [u8], offset: usize) -> DecodablePrimitive<'a> {
        match self {
            Self::U8 => DecodablePrimitive::U8(u8::from_bytes_unchecked(&mut data[offset..])),
            Self::U16 => DecodablePrimitive::U16(u16::from_bytes_unchecked(&mut data[offset..])),
            Self::Bool => DecodablePrimitive::Bool(bool::from_bytes_unchecked(&mut data[offset..])),
            Self::U24 => DecodablePrimitive::U24(U24::from_bytes_unchecked(&mut data[offset..])),
            Self::U256 => DecodablePrimitive::U256(U256::from_bytes_unchecked(&mut data[offset..])),
            Self::Signature => {
                DecodablePrimitive::Signature(Signature::from_bytes_unchecked(&mut data[offset..]))
            }
            Self::U32 => DecodablePrimitive::U32(u32::from_bytes_unchecked(&mut data[offset..])),
            Self::F32 => DecodablePrimitive::F32(f32::from_bytes_unchecked(&mut data[offset..])),
            Self::U64 => DecodablePrimitive::U64(u64::from_bytes_unchecked(&mut data[offset..])),
            Self::B032 => DecodablePrimitive::B032(B032::from_bytes_unchecked(&mut data[offset..])),
            Self::B0255 => {
                DecodablePrimitive::B0255(B0255::from_bytes_unchecked(&mut data[offset..]))
            }
            Self::B064K => {
                DecodablePrimitive::B064K(B064K::from_bytes_unchecked(&mut data[offset..]))
            }
            Self::B016M => {
                DecodablePrimitive::B016M(B016M::from_bytes_unchecked(&mut data[offset..]))
            }
            Self::Bytes => {
                DecodablePrimitive::Bytes(Bytes::from_bytes_unchecked(&mut data[offset..]))
            }
        }
    }

    #[cfg(not(feature = "no_std"))]
    fn from_reader<'a>(&self, reader: &mut impl Read) -> Result<DecodablePrimitive<'a>, Error> {
        match self {
            Self::U8 => Ok(DecodablePrimitive::U8(u8::from_reader_(reader)?)),
            Self::U16 => Ok(DecodablePrimitive::U16(u16::from_reader_(reader)?)),
            Self::Bool => Ok(DecodablePrimitive::Bool(bool::from_reader_(reader)?)),
            Self::U24 => Ok(DecodablePrimitive::U24(U24::from_reader_(reader)?)),
            Self::U256 => Ok(DecodablePrimitive::U256(U256::from_reader_(reader)?)),
            Self::Signature => Ok(DecodablePrimitive::Signature(Signature::from_reader_(
                reader,
            )?)),
            Self::U32 => Ok(DecodablePrimitive::U32(u32::from_reader_(reader)?)),
            Self::F32 => Ok(DecodablePrimitive::F32(f32::from_reader_(reader)?)),
            Self::U64 => Ok(DecodablePrimitive::U64(u64::from_reader_(reader)?)),
            Self::B032 => Ok(DecodablePrimitive::B032(B032::from_reader_(reader)?)),
            Self::B0255 => Ok(DecodablePrimitive::B0255(B0255::from_reader_(reader)?)),
            Self::B064K => Ok(DecodablePrimitive::B064K(B064K::from_reader_(reader)?)),
            Self::B016M => Ok(DecodablePrimitive::B016M(B016M::from_reader_(reader)?)),
            Self::Bytes => Ok(DecodablePrimitive::Bytes(Bytes::from_reader_(reader)?)),
        }
    }
}

impl<'a> GetSize for DecodablePrimitive<'a> {
    fn get_size(&self) -> usize {
        match self {
            DecodablePrimitive::U8(v) => v.get_size(),
            DecodablePrimitive::U16(v) => v.get_size(),
            DecodablePrimitive::Bool(v) => v.get_size(),
            DecodablePrimitive::U24(v) => v.get_size(),
            DecodablePrimitive::U256(v) => v.get_size(),
            DecodablePrimitive::Signature(v) => v.get_size(),
            DecodablePrimitive::U32(v) => v.get_size(),
            DecodablePrimitive::F32(v) => v.get_size(),
            DecodablePrimitive::U64(v) => v.get_size(),
            DecodablePrimitive::B032(v) => v.get_size(),
            DecodablePrimitive::B0255(v) => v.get_size(),
            DecodablePrimitive::B064K(v) => v.get_size(),
            DecodablePrimitive::B016M(v) => v.get_size(),
            DecodablePrimitive::Bytes(v) => v.get_size(),
        }
    }
}

impl FieldMarker {
    pub(crate) fn decode<'a>(&self, data: &'a mut [u8]) -> Result<DecodableField<'a>, Error> {
        match self {
            Self::Primitive(p) => Ok(DecodableField::Primitive(p.decode(data, 0))),
            Self::Struct(ps) => {
                let mut decodeds = Vec::new();
                let mut tail = data;
                for p in ps {
                    let field_size = p.size_hint_(tail, 0)?;
                    let (head, t) = tail.split_at_mut(field_size);
                    tail = t;
                    decodeds.push(p.decode(head)?);
                }
                Ok(DecodableField::Struct(decodeds))
            }
        }
    }

    #[cfg(not(feature = "no_std"))]
    pub(crate) fn from_reader<'a>(
        &self,
        reader: &mut impl Read,
    ) -> Result<DecodableField<'a>, Error> {
        match self {
            Self::Primitive(p) => Ok(DecodableField::Primitive(p.from_reader(reader)?)),
            Self::Struct(ps) => {
                let mut decodeds = Vec::new();
                for p in ps {
                    decodeds.push(p.from_reader(reader)?);
                }
                Ok(DecodableField::Struct(decodeds))
            }
        }
    }
}
