//! Copy data types
use crate::codec::Fixed;
use crate::datatypes::Sv2DataType;
use crate::Error;
use core::convert::{TryFrom, TryInto};

#[cfg(not(feature = "no_std"))]
use std::io::{Error as E, Read, Write};

// Impl bool as a primitive

impl Fixed for bool {
    const SIZE: usize = 1;
}

// Boolean value. Encoded as an unsigned 1-bit integer,
// True = 1, False = 0 with 7 additional padding bits in
// the high positions.
// x
// Recipients MUST NOT interpret bits outside of the
// least significant bit. Senders MAY set bits outside of
// the least significant bit to any value without any
// impact on meaning. This allows future use of other
// bits as flag bits.
impl<'a> Sv2DataType<'a> for bool {
    fn from_bytes_unchecked(data: &'a mut [u8]) -> Self {
        match data
            .get(0)
            .map(|x: &u8| x << 7)
            .map(|x: u8| x >> 7)
            .unwrap()
        {
            0 => false,
            1 => true,
            _ => panic!(),
        }
    }

    fn from_vec_(mut data: Vec<u8>) -> Result<Self, Error> {
        Self::from_bytes_(&mut data)
    }

    fn from_vec_unchecked(mut data: Vec<u8>) -> Self {
        Self::from_bytes_unchecked(&mut data)
    }

    #[cfg(not(feature = "no_std"))]
    fn from_reader_(reader: &mut impl Read) -> Result<Self, Error> {
        let mut dst = [0_u8; Self::SIZE];
        reader.read_exact(&mut dst)?;
        Self::from_bytes_(&mut dst)
    }

    fn to_slice_unchecked(&'a self, dst: &mut [u8]) {
        match self {
            true => dst[0] = 1,
            false => dst[0] = 0,
        };
    }

    #[cfg(not(feature = "no_std"))]
    fn to_writer_(&self, writer: &mut impl Write) -> Result<(), E> {
        match self {
            true => writer.write_all(&[1]),
            false => writer.write_all(&[0]),
        }
    }
}

// Impl unsigned as a primitives

impl Fixed for u8 {
    const SIZE: usize = 1;
}

impl Fixed for u16 {
    const SIZE: usize = 2;
}

impl Fixed for u32 {
    const SIZE: usize = 4;
}

// TODO fix not in the specs
impl Fixed for u64 {
    const SIZE: usize = 8;
}

macro_rules! impl_sv2_for_unsigned {
    ($a:ty) => {
        impl<'a> Sv2DataType<'a> for $a {
            fn from_bytes_unchecked(data: &'a mut [u8]) -> Self {
                let a: &[u8; Self::SIZE] = data[0..Self::SIZE].try_into().unwrap();
                Self::from_le_bytes(*a)
            }

            fn from_vec_(mut data: Vec<u8>) -> Result<Self, Error> {
                Self::from_bytes_(&mut data)
            }

            fn from_vec_unchecked(mut data: Vec<u8>) -> Self {
                Self::from_bytes_unchecked(&mut data)
            }

            #[cfg(not(feature = "no_std"))]
            fn from_reader_(reader: &mut impl Read) -> Result<Self, Error> {
                let mut dst = [0_u8; Self::SIZE];
                reader.read_exact(&mut dst)?;
                Ok(Self::from_bytes_unchecked(&mut dst))
            }

            fn to_slice_unchecked(&'a self, dst: &mut [u8]) {
                let dst = &mut dst[0..Self::SIZE];
                let src = self.to_le_bytes();
                dst.copy_from_slice(&src);
            }

            #[cfg(not(feature = "no_std"))]
            fn to_writer_(&self, writer: &mut impl Write) -> Result<(), E> {
                let bytes = self.to_le_bytes();
                writer.write_all(&bytes)
            }
        }
    };
}
impl_sv2_for_unsigned!(u8);
impl_sv2_for_unsigned!(u16);
impl_sv2_for_unsigned!(u32);
impl_sv2_for_unsigned!(u64);

// Impl f32 as a primitives

impl Fixed for f32 {
    const SIZE: usize = 4;
}

impl_sv2_for_unsigned!(f32);

#[repr(C)]
#[derive(Debug, Clone, Copy, Eq, PartialEq)]
pub struct U24(pub(crate) u32);

impl Fixed for U24 {
    const SIZE: usize = 3;
}

impl U24 {
    fn from_le_bytes(b: [u8; Self::SIZE]) -> Self {
        let inner = u32::from_le_bytes([b[0], b[1], b[2], 0]);
        Self(inner)
    }

    fn to_le_bytes(self) -> [u8; Self::SIZE] {
        let b = self.0.to_le_bytes();
        [b[0], b[1], b[2]]
    }
}

impl_sv2_for_unsigned!(U24);

impl TryFrom<u32> for U24 {
    type Error = Error;

    fn try_from(value: u32) -> Result<Self, Self::Error> {
        if value <= 16777215 {
            Ok(Self(value))
        } else {
            Err(Error::InvalidU24(value))
        }
    }
}

impl From<U24> for u32 {
    fn from(v: U24) -> Self {
        v.0
    }
}
