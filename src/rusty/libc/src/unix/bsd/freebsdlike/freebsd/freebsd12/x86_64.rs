#[repr(C)]
#[cfg_attr(feature = "extra_traits", derive(Debug, Eq, Hash, PartialEq))]
pub struct stat {
    pub st_dev: ::dev_t,
    pub st_ino: ::ino_t,
    pub st_nlink: ::nlink_t,
    pub st_mode: ::mode_t,
    st_padding0: i16,
    pub st_uid: ::uid_t,
    pub st_gid: ::gid_t,
    st_padding1: i32,
    pub st_rdev: ::dev_t,
    pub st_atime: ::time_t,
    pub st_atime_nsec: ::c_long,
    pub st_mtime: ::time_t,
    pub st_mtime_nsec: ::c_long,
    pub st_ctime: ::time_t,
    pub st_ctime_nsec: ::c_long,
    pub st_birthtime: ::time_t,
    pub st_birthtime_nsec: ::c_long,
    pub st_size: ::off_t,
    pub st_blocks: ::blkcnt_t,
    pub st_blksize: ::blksize_t,
    pub st_flags: ::fflags_t,
    pub st_gen: u64,
    pub st_spare: [u64; 10],
}

impl ::Copy for ::stat {}
impl ::Clone for ::stat {
    fn clone(&self) -> ::stat {
        *self
    }
}
