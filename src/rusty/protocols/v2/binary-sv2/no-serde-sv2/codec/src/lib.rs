//! ```txt
//! SERDE    <-> Sv2
//! bool     <-> BOOL
//! u8       <-> U8
//! u16      <-> U16
//! U24      <-> U24
//! u32      <-> u32
//! f32      <-> f32 // todo not in the spec but used
//! u64      <-> u64 // todo not in the spec but used
//! U256     <-> U256
//! Str0255  <-> STRO_255
//! Str032   <-> STRO_32 // todo not in the spec but used
//! Signature<-> SIGNATURE
//! B032     <-> B0_32 // todo not in the spec but used
//! B0255    <-> B0_255
//! B064K    <-> B0_64K
//! B016M    <-> B0_16M
//! [u8]     <-> BYTES
//! Pubkey   <-> PUBKEY
//! Seq0255  <-> SEQ0_255[T]
//! Seq064K  <-> SEQ0_64K[T]
//! ```
#![cfg_attr(feature = "no_std", no_std)]
use core::convert::TryInto;

#[cfg(not(feature = "no_std"))]
use std::io::{Error as E, ErrorKind};

mod codec;
mod datatypes;
pub use datatypes::{
    Bytes, PubKey, Seq0255, Seq064K, Signature, Str0255, Str032, B016M, B0255, B032, B064K, U24,
    U256,
};

pub use crate::codec::decodable::Decodable;
pub use crate::codec::encodable::{Encodable, EncodableField};
pub use crate::codec::GetSize;
pub use crate::codec::SizeHint;

#[allow(clippy::wrong_self_convention)]
pub fn to_bytes<T: Encodable + GetSize>(src: T) -> Result<Vec<u8>, Error> {
    let mut result = vec![0_u8; src.get_size()];
    src.to_bytes(&mut result)?;
    Ok(result)
}

#[allow(clippy::wrong_self_convention)]
pub fn to_writer<T: Encodable>(src: T, dst: &mut [u8]) -> Result<(), Error> {
    src.to_bytes(dst)?;
    Ok(())
}

pub fn from_bytes<'a, T: Decodable<'a>>(data: &'a mut [u8]) -> Result<T, Error> {
    T::from_bytes(data)
}

pub mod decodable {
    pub use crate::codec::decodable::Decodable;
    pub use crate::codec::decodable::DecodableField;
    pub use crate::codec::decodable::FieldMarker;
    //pub use crate::codec::decodable::PrimitiveMarker;
}

pub mod encodable {
    pub use crate::codec::encodable::Encodable;
    pub use crate::codec::encodable::EncodableField;
}

#[macro_use]
extern crate alloc;

#[derive(Debug)]
pub enum Error {
    OutOfBound,
    NotABool(u8),
    /// -> (expected size, actual size)
    WriteError(usize, usize),
    U24TooBig(u32),
    InvalidSignatureSize(usize),
    InvalidU256(usize),
    InvalidU24(u32),
    InvalidB0255Size(usize),
    InvalidB064KSize(usize),
    InvalidB016MSize(usize),
    InvalidSeq0255Size(usize),
    PrimitiveConversionError,
    DecodableConversionError,
    UnInitializedDecoder,
    #[cfg(not(feature = "no_std"))]
    IoError(E),
    ReadError(usize, usize),
    Todo,
}

#[cfg(not(feature = "no_std"))]
impl From<E> for Error {
    fn from(v: E) -> Self {
        match v.kind() {
            ErrorKind::UnexpectedEof => Error::OutOfBound,
            _ => Error::IoError(v),
        }
    }
}

/// Vec<u8> is used as the Sv2 type Bytes
impl GetSize for Vec<u8> {
    fn get_size(&self) -> usize {
        self.len()
    }
}

impl<'a> From<Vec<u8>> for EncodableField<'a> {
    fn from(v: Vec<u8>) -> Self {
        let bytes: Bytes = v.try_into().unwrap();
        crate::encodable::EncodableField::Primitive(
            crate::codec::encodable::EncodablePrimitive::Bytes(bytes),
        )
    }
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct CVec {
    data: *mut u8,
    len: usize,
    capacity: usize,
}

impl CVec {
    pub fn as_mut_slice(&mut self) -> &mut [u8] {
        unsafe { core::slice::from_raw_parts_mut(self.data, self.len) }
    }

    /// Used when we need to fill a buffer allocated in rust from C.
    ///
    /// # Safety
    ///
    /// This function construct a CVec without taking ownership of the pointed buffer so if the
    /// owner drop them the CVec will point to garbage.
    #[allow(clippy::wrong_self_convention)]
    pub fn as_shared_buffer(v: &mut [u8]) -> Self {
        let (data, len) = (v.as_mut_ptr(), v.len());
        Self {
            data,
            len,
            capacity: len,
        }
    }
}

impl From<&[u8]> for CVec {
    fn from(v: &[u8]) -> Self {
        let mut buffer: Vec<u8> = vec![0; v.len()];
        buffer.copy_from_slice(v);

        // Get the length, first, then the pointer (doing it the other way around **currently** doesn't cause UB, but it may be unsound due to unclear (to me, at least) guarantees of the std lib)
        let len = buffer.len();
        let ptr = buffer.as_mut_ptr();
        std::mem::forget(buffer);

        CVec {
            data: ptr,
            len,
            capacity: len,
        }
    }
}

/// Given a C allocated buffer return a rust allocated CVec
///
/// # Safety
///
/// TODO
#[no_mangle]
pub unsafe extern "C" fn cvec_from_buffer(data: *const u8, len: usize) -> CVec {
    let input = std::slice::from_raw_parts(data, len);

    let mut buffer: Vec<u8> = vec![0; len];
    buffer.copy_from_slice(input);

    // Get the length, first, then the pointer (doing it the other way around **currently** doesn't cause UB, but it may be unsound due to unclear (to me, at least) guarantees of the std lib)
    let len = buffer.len();
    let ptr = buffer.as_mut_ptr();
    std::mem::forget(buffer);

    CVec {
        data: ptr,
        len,
        capacity: len,
    }
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct CVec2 {
    data: *mut CVec,
    len: usize,
    capacity: usize,
}

impl CVec2 {
    pub fn as_mut_slice(&mut self) -> &mut [CVec] {
        unsafe { core::slice::from_raw_parts_mut(self.data, self.len) }
    }
}
impl From<CVec2> for Vec<CVec> {
    fn from(v: CVec2) -> Self {
        unsafe { Vec::from_raw_parts(v.data, v.len, v.capacity) }
    }
}

pub fn free_vec(buf: &mut CVec) {
    let _: Vec<u8> = unsafe { Vec::from_raw_parts(buf.data, buf.len, buf.capacity) };
}

pub fn free_vec_2(buf: &mut CVec2) {
    let vs: Vec<CVec> = unsafe { Vec::from_raw_parts(buf.data, buf.len, buf.capacity) };
    for mut s in vs {
        free_vec(&mut s)
    }
}

impl<'a, const A: bool, const B: usize, const C: usize, const D: usize>
    From<datatypes::Inner<'a, A, B, C, D>> for CVec
{
    fn from(v: datatypes::Inner<'a, A, B, C, D>) -> Self {
        let (ptr, len, cap): (*mut u8, usize, usize) = match v {
            datatypes::Inner::Ref(inner) => {
                // Data is copied in a vector that then will be forgetted from the allocator,
                // cause the owner of the data is going to be dropped by rust
                let mut inner: Vec<u8> = inner.into();

                // Get the length, first, then the pointer (doing it the other way around **currently** doesn't cause UB, but it may be unsound due to unclear (to me, at least) guarantees of the std lib)
                let len = inner.len();
                let cap = inner.capacity();
                let ptr = inner.as_mut_ptr();
                std::mem::forget(inner);

                (ptr, len, cap)
            }
            datatypes::Inner::Owned(mut inner) => {
                // Get the length, first, then the pointer (doing it the other way around **currently** doesn't cause UB, but it may be unsound due to unclear (to me, at least) guarantees of the std lib)
                let len = inner.len();
                let cap = inner.capacity();
                let ptr = inner.as_mut_ptr();
                std::mem::forget(inner);

                (ptr, len, cap)
            }
        };
        Self {
            data: ptr,
            len,
            capacity: cap,
        }
    }
}

impl<'a, T: Into<CVec>> From<Seq0255<'a, T>> for CVec2 {
    fn from(v: Seq0255<'a, T>) -> Self {
        let mut v: Vec<CVec> = v.0.into_iter().map(|x| x.into()).collect();
        // Get the length, first, then the pointer (doing it the other way around **currently** doesn't cause UB, but it may be unsound due to unclear (to me, at least) guarantees of the std lib)
        let len = v.len();
        let capacity = v.capacity();
        let data = v.as_mut_ptr();
        std::mem::forget(v);
        Self {
            data,
            len,
            capacity,
        }
    }
}
impl<'a, T: Into<CVec>> From<Seq064K<'a, T>> for CVec2 {
    fn from(v: Seq064K<'a, T>) -> Self {
        let mut v: Vec<CVec> = v.0.into_iter().map(|x| x.into()).collect();
        // Get the length, first, then the pointer (doing it the other way around **currently** doesn't cause UB, but it may be unsound due to unclear (to me, at least) guarantees of the std lib)
        let len = v.len();
        let capacity = v.capacity();
        let data = v.as_mut_ptr();
        std::mem::forget(v);
        Self {
            data,
            len,
            capacity,
        }
    }
}

#[no_mangle]
pub extern "C" fn _c_export_u24(_a: U24) {}
#[no_mangle]
pub extern "C" fn _c_export_cvec(_a: CVec) {}
#[no_mangle]
pub extern "C" fn _c_export_cvec2(_a: CVec2) {}
