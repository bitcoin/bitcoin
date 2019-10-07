pub type c_char = i8;
pub type c_long = i64;
pub type c_ulong = u64;
pub type time_t = i64;
pub type suseconds_t = i64;

// should be pub(crate), but that requires Rust 1.18.0
cfg_if! {
    if #[cfg(libc_const_size_of)] {
        #[doc(hidden)]
        pub const _ALIGNBYTES: usize = ::mem::size_of::<::c_long>() - 1;
    } else {
        #[doc(hidden)]
        pub const _ALIGNBYTES: usize = 8 - 1;
    }
}
pub const MAP_32BIT: ::c_int = 0x00080000;

cfg_if! {
    if #[cfg(libc_align)] {
        mod align;
        pub use self::align::*;
    }
}
