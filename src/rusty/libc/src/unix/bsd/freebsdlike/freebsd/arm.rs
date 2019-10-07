pub type c_char = u8;
pub type c_long = i32;
pub type c_ulong = u32;
pub type time_t = i64;
pub type suseconds_t = i32;

s! {
    pub struct stat {
        pub st_dev: ::dev_t,
        pub st_ino: ::ino_t,
        pub st_mode: ::mode_t,
        pub st_nlink: ::nlink_t,
        pub st_uid: ::uid_t,
        pub st_gid: ::gid_t,
        pub st_rdev: ::dev_t,
        pub st_atime: ::time_t,
        pub st_atime_nsec: ::c_long,
        pub st_atime_pad: ::c_long,
        pub st_mtime: ::time_t,
        pub st_mtime_nsec: ::c_long,
        pub st_mtime_pad: ::c_long,
        pub st_ctime: ::time_t,
        pub st_ctime_nsec: ::c_long,
        pub st_ctime_pad: ::c_long,
        pub st_size: ::off_t,
        pub st_blocks: ::blkcnt_t,
        pub st_blksize: ::blksize_t,
        pub st_flags: ::fflags_t,
        pub st_gen: u32,
        pub st_lspare: i32,
        pub st_birthtime: ::time_t,
        pub st_birthtime_nsec: ::c_long,
        pub st_birthtime_pad: ::c_long,
    }
}

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
pub const MAP_32BIT: ::c_int = 0x00080000;
