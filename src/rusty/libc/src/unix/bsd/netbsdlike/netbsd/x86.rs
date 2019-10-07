pub type c_long = i32;
pub type c_ulong = u32;
pub type c_char = i8;
pub type __cpu_simple_lock_nv_t = ::c_uchar;

// should be pub(crate), but that requires Rust 1.18.0
cfg_if! {
    if #[cfg(libc_const_size_of)] {
        #[doc(hidden)]
        pub const _ALIGNBYTES: usize = ::mem::size_of::<::c_int>() - 1;
    } else {
        #[doc(hidden)]
        pub const _ALIGNBYTES: usize = 4 - 1;
    }
}
