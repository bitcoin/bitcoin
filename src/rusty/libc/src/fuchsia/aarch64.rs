pub type c_char = u8;
pub type __u64 = ::c_ulonglong;
pub type wchar_t = u32;
pub type nlink_t = ::c_ulong;
pub type blksize_t = ::c_long;

s! {
    pub struct stat {
        pub st_dev: ::dev_t,
        pub st_ino: ::ino_t,
        pub st_mode: ::mode_t,
        pub st_nlink: ::nlink_t,
        pub st_uid: ::uid_t,
        pub st_gid: ::gid_t,
        pub st_rdev: ::dev_t,
        __pad0: ::c_ulong,
        pub st_size: ::off_t,
        pub st_blksize: ::blksize_t,
        __pad1: ::c_int,
        pub st_blocks: ::blkcnt_t,
        pub st_atime: ::time_t,
        pub st_atime_nsec: ::c_long,
        pub st_mtime: ::time_t,
        pub st_mtime_nsec: ::c_long,
        pub st_ctime: ::time_t,
        pub st_ctime_nsec: ::c_long,
        __unused: [::c_uint; 2],
    }

    pub struct stat64 {
        pub st_dev: ::dev_t,
        pub st_ino: ::ino_t,
        pub st_mode: ::mode_t,
        pub st_nlink: ::nlink_t,
        pub st_uid: ::uid_t,
        pub st_gid: ::gid_t,
        pub st_rdev: ::dev_t,
        __pad0: ::c_ulong,
        pub st_size: ::off_t,
        pub st_blksize: ::blksize_t,
        __pad1: ::c_int,
        pub st_blocks: ::blkcnt_t,
        pub st_atime: ::time_t,
        pub st_atime_nsec: ::c_long,
        pub st_mtime: ::time_t,
        pub st_mtime_nsec: ::c_long,
        pub st_ctime: ::time_t,
        pub st_ctime_nsec: ::c_long,
        __unused: [::c_uint; 2],
    }

    pub struct ipc_perm {
        pub __ipc_perm_key: ::key_t,
        pub uid: ::uid_t,
        pub gid: ::gid_t,
        pub cuid: ::uid_t,
        pub cgid: ::gid_t,
        pub mode: ::mode_t,
        pub __seq: ::c_ushort,
        __unused1: ::c_ulong,
        __unused2: ::c_ulong,
    }
}

pub const MINSIGSTKSZ: ::size_t = 6144;
pub const SIGSTKSZ: ::size_t = 12288;
