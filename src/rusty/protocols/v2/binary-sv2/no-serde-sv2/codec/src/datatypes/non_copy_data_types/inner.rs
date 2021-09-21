use super::IntoOwned;
use crate::codec::{GetSize, SizeHint};
use crate::datatypes::Sv2DataType;
use crate::Error;
use core::convert::TryFrom;

#[cfg(not(feature = "no_std"))]
use std::io::{Error as E, Read, Write};

#[repr(C)]
#[derive(Debug)]
pub enum Inner<
    'a,
    const ISFIXED: bool,
    const SIZE: usize,
    const HEADERSIZE: usize,
    const MAXSIZE: usize,
> {
    Ref(&'a mut [u8]),
    Owned(Vec<u8>),
}

impl<'a, const ISFIXED: bool, const SIZE: usize, const HEADERSIZE: usize, const MAXSIZE: usize>
    PartialEq for Inner<'a, ISFIXED, SIZE, HEADERSIZE, MAXSIZE>
{
    fn eq(&self, other: &Self) -> bool {
        match (self, other) {
            (Inner::Ref(b), Inner::Owned(a)) => *b == &a[..],
            (Inner::Owned(b), Inner::Ref(a)) => *a == &b[..],
            (Inner::Owned(b), Inner::Owned(a)) => b == a,
            (Inner::Ref(b), Inner::Ref(a)) => b == a,
        }
    }
}

impl<'a, const ISFIXED: bool, const SIZE: usize, const HEADERSIZE: usize, const MAXSIZE: usize> Eq
    for Inner<'a, ISFIXED, SIZE, HEADERSIZE, MAXSIZE>
{
}

impl<'a, const ISFIXED: bool, const SIZE: usize, const HEADERSIZE: usize, const MAXSIZE: usize>
    Inner<'a, ISFIXED, SIZE, HEADERSIZE, MAXSIZE>
{
    fn expected_length(data: &[u8]) -> Result<usize, Error> {
        let expected_length = match ISFIXED {
            true => Self::expected_length_fixed(),
            false => Self::expected_length_variable(data)?,
        };
        if ISFIXED || expected_length <= MAXSIZE {
            Ok(expected_length)
        } else {
            Err(Error::ReadError(data.len(), MAXSIZE))
        }
    }

    fn expected_length_fixed() -> usize {
        SIZE
    }

    fn expected_length_variable(data: &[u8]) -> Result<usize, Error> {
        if data.len() >= HEADERSIZE {
            let size = match HEADERSIZE {
                0 => Ok(data.len()),
                1 => Ok(data[0] as usize),
                2 => Ok(u16::from_le_bytes([data[0], data[1]]) as usize),
                3 => Ok(u32::from_le_bytes([data[0], data[1], data[2], 0]) as usize),
                _ => unimplemented!(),
            };
            size.map(|x| x + HEADERSIZE)
        } else {
            Err(Error::ReadError(data.len(), HEADERSIZE))
        }
    }

    #[cfg(not(feature = "no_std"))]
    fn expected_length_for_reader(mut reader: impl Read) -> Result<usize, Error> {
        if ISFIXED {
            Ok(SIZE)
        } else {
            let mut header = [0_u8; HEADERSIZE];
            reader.read_exact(&mut header)?;
            let expected_length = match HEADERSIZE {
                1 => header[0] as usize,
                2 => u16::from_le_bytes([header[0], header[1]]) as usize,
                3 => u32::from_le_bytes([header[0], header[1], header[2], 0]) as usize,
                _ => unimplemented!(),
            };
            if expected_length <= MAXSIZE {
                Ok(expected_length)
            } else {
                Err(Error::ReadError(expected_length, MAXSIZE))
            }
        }
    }

    pub fn len(&self) -> usize {
        match (self, ISFIXED) {
            (Inner::Ref(data), false) => data.len(),
            (Inner::Owned(data), false) => data.len(),
            (_, true) => 1,
        }
    }

    fn get_header(&self) -> Vec<u8> {
        if HEADERSIZE == 0 {
            Vec::new()
        } else {
            let len = self.len();
            len.to_le_bytes().into()
        }
    }
}

impl<'a, const ISFIXED: bool, const SIZE: usize, const HEADERSIZE: usize, const MAXSIZE: usize>
    TryFrom<&'a mut [u8]> for Inner<'a, ISFIXED, SIZE, HEADERSIZE, MAXSIZE>
{
    type Error = Error;

    fn try_from(value: &'a mut [u8]) -> Result<Self, Self::Error> {
        if ISFIXED && value.len() == SIZE {
            Ok(Self::Ref(value))
        } else if ISFIXED {
            Err(Error::Todo)
        } else if value.len() <= MAXSIZE {
            Ok(Self::Ref(value))
        } else {
            Err(Error::Todo)
        }
    }
}

impl<'a, const ISFIXED: bool, const SIZE: usize, const HEADERSIZE: usize, const MAXSIZE: usize>
    TryFrom<Vec<u8>> for Inner<'a, ISFIXED, SIZE, HEADERSIZE, MAXSIZE>
{
    type Error = Error;

    fn try_from(value: Vec<u8>) -> Result<Self, Self::Error> {
        if ISFIXED && value.len() == SIZE {
            Ok(Self::Owned(value))
        } else if ISFIXED {
            Err(Error::Todo)
        } else if value.len() <= MAXSIZE {
            Ok(Self::Owned(value))
        } else {
            Err(Error::Todo)
        }
    }
}

impl<'a, const ISFIXED: bool, const SIZE: usize, const HEADERSIZE: usize, const MAXSIZE: usize>
    GetSize for Inner<'a, ISFIXED, SIZE, HEADERSIZE, MAXSIZE>
{
    fn get_size(&self) -> usize {
        match self {
            Inner::Ref(data) => data.len() + HEADERSIZE,
            Inner::Owned(data) => data.len() + HEADERSIZE,
        }
    }
}

impl<'a, const ISFIXED: bool, const HEADERSIZE: usize, const SIZE: usize, const MAXSIZE: usize>
    SizeHint for Inner<'a, ISFIXED, HEADERSIZE, SIZE, MAXSIZE>
{
    fn size_hint(data: &[u8], offset: usize) -> Result<usize, Error> {
        Self::expected_length(&data[offset..])
    }

    fn size_hint_(&self, data: &[u8], offset: usize) -> Result<usize, Error> {
        Self::expected_length(&data[offset..])
    }
}
use crate::codec::decodable::FieldMarker;

impl<'a, const ISFIXED: bool, const SIZE: usize, const HEADERSIZE: usize, const MAXSIZE: usize>
    Sv2DataType<'a> for Inner<'a, ISFIXED, SIZE, HEADERSIZE, MAXSIZE>
where
    Self: Into<FieldMarker>,
{
    fn from_bytes_unchecked(data: &'a mut [u8]) -> Self {
        if ISFIXED {
            Self::Ref(data)
        } else {
            Self::Ref(&mut data[HEADERSIZE..])
        }
    }

    fn from_vec_(data: Vec<u8>) -> Result<Self, Error> {
        Self::size_hint(&data, 0)?;
        Ok(Self::Owned(data))
    }

    fn from_vec_unchecked(data: Vec<u8>) -> Self {
        Self::Owned(data)
    }

    #[cfg(not(feature = "no_std"))]
    fn from_reader_(mut reader: &mut impl Read) -> Result<Self, Error> {
        let size = Self::expected_length_for_reader(&mut reader)?;

        let mut dst = vec![0; size];

        reader.read_exact(&mut dst)?;
        Ok(Self::from_vec_unchecked(dst))
    }

    fn to_slice_unchecked(&'a self, dst: &mut [u8]) {
        let size = self.get_size();
        let header = self.get_header();
        dst[0..HEADERSIZE].copy_from_slice(&header[..HEADERSIZE]);
        match self {
            Inner::Ref(data) => {
                let dst = &mut dst[0..size];
                dst[HEADERSIZE..].copy_from_slice(data);
            }
            Inner::Owned(data) => {
                let dst = &mut dst[0..size];
                dst[HEADERSIZE..].copy_from_slice(data);
            }
        }
    }

    #[cfg(not(feature = "no_std"))]
    fn to_writer_(&self, writer: &mut impl Write) -> Result<(), E> {
        match self {
            Inner::Ref(data) => {
                writer.write_all(data)?;
            }
            Inner::Owned(data) => {
                writer.write_all(data)?;
            }
        };
        Ok(())
    }
}

impl<'a, const ISFIXED: bool, const SIZE: usize, const HEADERSIZE: usize, const MAXSIZE: usize>
    IntoOwned for Inner<'a, ISFIXED, SIZE, HEADERSIZE, MAXSIZE>
{
    fn into_owned(self) -> Self {
        match self {
            Inner::Ref(data) => {
                let v: Vec<u8> = data.into();
                Self::Owned(v)
            }
            Inner::Owned(_) => self,
        }
    }
}

impl<'a, const ISFIXED: bool, const SIZE: usize, const HEADERSIZE: usize, const MAXSIZE: usize>
    Clone for Inner<'a, ISFIXED, SIZE, HEADERSIZE, MAXSIZE>
{
    fn clone(&self) -> Self {
        match self {
            Inner::Ref(data) => {
                let mut v = Vec::with_capacity(data.len());
                v.extend_from_slice(data);
                Self::Owned(v)
            }
            Inner::Owned(data) => Inner::Owned(data.clone()),
        }
    }
}

impl<'a, const ISFIXED: bool, const SIZE: usize, const HEADERSIZE: usize, const MAXSIZE: usize>
    AsRef<[u8]> for Inner<'a, ISFIXED, SIZE, HEADERSIZE, MAXSIZE>
{
    fn as_ref(&self) -> &[u8] {
        match self {
            Inner::Ref(r) => &r[..],
            Inner::Owned(r) => &r[..],
        }
    }
}
