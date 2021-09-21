use crate::codec::GetSize;
use crate::datatypes::{Bytes, Signature, Sv2DataType, B016M, B0255, B032, B064K, U24, U256};
use crate::Error;
use alloc::vec::Vec;
#[cfg(not(feature = "no_std"))]
use std::io::{Error as E, Write};

pub trait Encodable {
    #[allow(clippy::wrong_self_convention)]
    fn to_bytes(self, dst: &mut [u8]) -> Result<usize, Error>;

    #[cfg(not(feature = "no_std"))]
    #[allow(clippy::wrong_self_convention)]
    fn to_writer(self, dst: &mut impl Write) -> Result<(), E>;
}

//
impl<'a, T: Into<EncodableField<'a>>> Encodable for T {
    #[allow(clippy::wrong_self_convention)]
    fn to_bytes(self, dst: &mut [u8]) -> Result<usize, Error> {
        let encoded_field = self.into();
        encoded_field.encode(dst, 0)
    }

    #[cfg(not(feature = "no_std"))]
    #[allow(clippy::wrong_self_convention)]
    fn to_writer(self, dst: &mut impl Write) -> Result<(), E> {
        let encoded_field = self.into();
        encoded_field.to_writer(dst)
    }
}

#[derive(Debug)]
pub enum EncodablePrimitive<'a> {
    U8(u8),
    OwnedU8(u8),
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

impl<'a> EncodablePrimitive<'a> {
    fn encode(&self, dst: &mut [u8]) -> Result<usize, Error> {
        match self {
            Self::U8(v) => v.to_slice(dst),
            Self::OwnedU8(v) => v.to_slice(dst),
            Self::U16(v) => v.to_slice(dst),
            Self::Bool(v) => v.to_slice(dst),
            Self::U24(v) => v.to_slice(dst),
            Self::U256(v) => v.to_slice(dst),
            Self::Signature(v) => v.to_slice(dst),
            Self::U32(v) => v.to_slice(dst),
            Self::F32(v) => v.to_slice(dst),
            Self::U64(v) => v.to_slice(dst),
            Self::B032(v) => v.to_slice(dst),
            Self::B0255(v) => v.to_slice(dst),
            Self::B064K(v) => v.to_slice(dst),
            Self::B016M(v) => v.to_slice(dst),
            Self::Bytes(v) => v.to_slice(dst),
        }
    }

    #[cfg(not(feature = "no_std"))]
    pub fn write(&self, writer: &mut impl Write) -> Result<(), E> {
        match self {
            Self::U8(v) => v.to_writer_(writer),
            Self::OwnedU8(v) => v.to_writer_(writer),
            Self::U16(v) => v.to_writer_(writer),
            Self::Bool(v) => v.to_writer_(writer),
            Self::U24(v) => v.to_writer_(writer),
            Self::U256(v) => v.to_writer_(writer),
            Self::Signature(v) => v.to_writer_(writer),
            Self::U32(v) => v.to_writer_(writer),
            Self::F32(v) => v.to_writer_(writer),
            Self::U64(v) => v.to_writer_(writer),
            Self::B032(v) => v.to_writer_(writer),
            Self::B0255(v) => v.to_writer_(writer),
            Self::B064K(v) => v.to_writer_(writer),
            Self::B016M(v) => v.to_writer_(writer),
            Self::Bytes(v) => v.to_writer_(writer),
        }
    }
}

impl<'a> GetSize for EncodablePrimitive<'a> {
    fn get_size(&self) -> usize {
        match self {
            Self::U8(v) => v.get_size(),
            Self::OwnedU8(v) => v.get_size(),
            Self::U16(v) => v.get_size(),
            Self::Bool(v) => v.get_size(),
            Self::U24(v) => v.get_size(),
            Self::U256(v) => v.get_size(),
            Self::Signature(v) => v.get_size(),
            Self::U32(v) => v.get_size(),
            Self::F32(v) => v.get_size(),
            Self::U64(v) => v.get_size(),
            Self::B032(v) => v.get_size(),
            Self::B0255(v) => v.get_size(),
            Self::B064K(v) => v.get_size(),
            Self::B016M(v) => v.get_size(),
            Self::Bytes(v) => v.get_size(),
        }
    }
}

#[derive(Debug)]
pub enum EncodableField<'a> {
    Primitive(EncodablePrimitive<'a>),
    Struct(Vec<EncodableField<'a>>),
}

impl<'a> EncodableField<'a> {
    pub fn encode(&self, dst: &mut [u8], mut offset: usize) -> Result<usize, Error> {
        match (self, dst.len() >= offset) {
            (Self::Primitive(p), true) => p.encode(&mut dst[offset..]),
            (Self::Struct(ps), true) => {
                for p in ps {
                    let encoded_bytes = p.encode(dst, offset)?;
                    offset += encoded_bytes;
                }
                Ok(offset)
            }
            _ => Err(Error::WriteError(offset, dst.len())),
        }
    }

    #[cfg(not(feature = "no_std"))]
    pub fn to_writer(&self, writer: &mut impl Write) -> Result<(), E> {
        match self {
            Self::Primitive(p) => p.write(writer),
            Self::Struct(ps) => {
                for p in ps {
                    p.to_writer(writer)?;
                }
                Ok(())
            }
        }
    }
}

impl<'a> GetSize for EncodableField<'a> {
    fn get_size(&self) -> usize {
        match self {
            Self::Primitive(p) => p.get_size(),
            Self::Struct(ps) => {
                let mut size = 0;
                for p in ps {
                    size += p.get_size();
                }
                size
            }
        }
    }
}
