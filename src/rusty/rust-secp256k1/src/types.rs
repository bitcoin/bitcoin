#![allow(non_camel_case_types)]
use core::fmt;

pub type c_int = i32;
pub type c_uchar = u8;
pub type c_uint = u32;

/// This might not match C's `c_char` exactly.
/// The way we use it makes it fine either way but this type shouldn't be used outside of the library.
pub type c_char = i8;

/// This is an exact copy of https://doc.rust-lang.org/core/ffi/enum.c_void.html
/// It should be Equivalent to C's void type when used as a pointer.
/// 
/// We can replace this with `core::ffi::c_void` once we update the rustc version to >=1.30.0.
#[repr(u8)]
pub enum c_void {
    #[doc(hidden)] __variant1,
    #[doc(hidden)] __variant2,
}

impl fmt::Debug for c_void {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.pad("c_void")
    }
}