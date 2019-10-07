pub type blkcnt_t = i64;
pub type blksize_t = i64;
pub type c_char = i8;
pub type c_long = i64;
pub type c_ulong = u64;
pub type fsblkcnt_t = ::c_ulong;
pub type fsfilcnt_t = ::c_ulong;
pub type ino_t = u64;
pub type nlink_t = u64;
pub type off_t = i64;
pub type rlim_t = ::c_ulong;
pub type suseconds_t = i64;
pub type time_t = i64;
pub type wchar_t = i32;

s! {
    pub struct stat {
        pub st_dev: ::c_ulong,
        st_pad1: [::c_long; 2],
        pub st_ino: ::ino_t,
        pub st_mode: ::mode_t,
        pub st_nlink: ::nlink_t,
        pub st_uid: ::uid_t,
        pub st_gid: ::gid_t,
        pub st_rdev: ::c_ulong,
        st_pad2: [::c_ulong; 1],
        pub st_size: ::off_t,
        st_pad3: ::c_long,
        pub st_atime: ::time_t,
        pub st_atime_nsec: ::c_long,
        pub st_mtime: ::time_t,
        pub st_mtime_nsec: ::c_long,
        pub st_ctime: ::time_t,
        pub st_ctime_nsec: ::c_long,
        pub st_blksize: ::blksize_t,
        st_pad4: ::c_long,
        pub st_blocks: ::blkcnt_t,
        st_pad5: [::c_long; 7],
    }

    pub struct stat64 {
        pub st_dev: ::c_ulong,
        st_pad1: [::c_long; 2],
        pub st_ino: ::ino64_t,
        pub st_mode: ::mode_t,
        pub st_nlink: ::nlink_t,
        pub st_uid: ::uid_t,
        pub st_gid: ::gid_t,
        pub st_rdev: ::c_ulong,
        st_pad2: [::c_long; 2],
        pub st_size: ::off64_t,
        pub st_atime: ::time_t,
        pub st_atime_nsec: ::c_long,
        pub st_mtime: ::time_t,
        pub st_mtime_nsec: ::c_long,
        pub st_ctime: ::time_t,
        pub st_ctime_nsec: ::c_long,
        pub st_blksize: ::blksize_t,
        st_pad3: ::c_long,
        pub st_blocks: ::blkcnt64_t,
        st_pad5: [::c_long; 7],
    }

    pub struct pthread_attr_t {
        __size: [::c_ulong; 7]
    }

    pub struct sigaction {
        pub sa_flags: ::c_int,
        pub sa_sigaction: ::sighandler_t,
        pub sa_mask: sigset_t,
        _restorer: *mut ::c_void,
    }

    pub struct stack_t {
        pub ss_sp: *mut ::c_void,
        pub ss_size: ::size_t,
        pub ss_flags: ::c_int,
    }

    pub struct sigset_t {
        __size: [::c_ulong; 16],
    }

    pub struct siginfo_t {
        pub si_signo: ::c_int,
        pub si_code: ::c_int,
        pub si_errno: ::c_int,
        _pad: ::c_int,
        _pad2: [::c_long; 14],
    }

    pub struct ipc_perm {
        pub __key: ::key_t,
        pub uid: ::uid_t,
        pub gid: ::gid_t,
        pub cuid: ::uid_t,
        pub cgid: ::gid_t,
        pub mode: ::c_uint,
        pub __seq: ::c_ushort,
        __pad1: ::c_ushort,
        __unused1: ::c_ulong,
        __unused2: ::c_ulong
    }

    pub struct shmid_ds {
        pub shm_perm: ::ipc_perm,
        pub shm_segsz: ::size_t,
        pub shm_atime: ::time_t,
        pub shm_dtime: ::time_t,
        pub shm_ctime: ::time_t,
        pub shm_cpid: ::pid_t,
        pub shm_lpid: ::pid_t,
        pub shm_nattch: ::shmatt_t,
        __unused4: ::c_ulong,
        __unused5: ::c_ulong
    }

    pub struct msqid_ds {
        pub msg_perm: ::ipc_perm,
        pub msg_stime: ::time_t,
        pub msg_rtime: ::time_t,
        pub msg_ctime: ::time_t,
        __msg_cbytes: ::c_ulong,
        pub msg_qnum: ::msgqnum_t,
        pub msg_qbytes: ::msglen_t,
        pub msg_lspid: ::pid_t,
        pub msg_lrpid: ::pid_t,
        __glibc_reserved4: ::c_ulong,
        __glibc_reserved5: ::c_ulong,
    }

    pub struct statfs {
        pub f_type: ::c_long,
        pub f_bsize: ::c_long,
        pub f_frsize: ::c_long,
        pub f_blocks: ::fsblkcnt_t,
        pub f_bfree: ::fsblkcnt_t,
        pub f_files: ::fsblkcnt_t,
        pub f_ffree: ::fsblkcnt_t,
        pub f_bavail: ::fsblkcnt_t,
        pub f_fsid: ::fsid_t,

        pub f_namelen: ::c_long,
        f_spare: [::c_long; 6],
    }

    pub struct msghdr {
        pub msg_name: *mut ::c_void,
        pub msg_namelen: ::socklen_t,
        pub msg_iov: *mut ::iovec,
        pub msg_iovlen: ::size_t,
        pub msg_control: *mut ::c_void,
        pub msg_controllen: ::size_t,
        pub msg_flags: ::c_int,
    }

    pub struct cmsghdr {
        pub cmsg_len: ::size_t,
        pub cmsg_level: ::c_int,
        pub cmsg_type: ::c_int,
    }

    pub struct termios {
        pub c_iflag: ::tcflag_t,
        pub c_oflag: ::tcflag_t,
        pub c_cflag: ::tcflag_t,
        pub c_lflag: ::tcflag_t,
        pub c_line: ::cc_t,
        pub c_cc: [::cc_t; ::NCCS],
    }

    pub struct sysinfo {
        pub uptime: ::c_long,
        pub loads: [::c_ulong; 3],
        pub totalram: ::c_ulong,
        pub freeram: ::c_ulong,
        pub sharedram: ::c_ulong,
        pub bufferram: ::c_ulong,
        pub totalswap: ::c_ulong,
        pub freeswap: ::c_ulong,
        pub procs: ::c_ushort,
        pub pad: ::c_ushort,
        pub totalhigh: ::c_ulong,
        pub freehigh: ::c_ulong,
        pub mem_unit: ::c_uint,
        pub _f: [::c_char; 0],
    }
}

pub const __SIZEOF_PTHREAD_CONDATTR_T: usize = 4;
pub const __SIZEOF_PTHREAD_MUTEXATTR_T: usize = 4;
pub const __SIZEOF_PTHREAD_MUTEX_T: usize = 40;
pub const __SIZEOF_PTHREAD_RWLOCK_T: usize = 56;

pub const RLIM_INFINITY: ::rlim_t = 0xffff_ffff_ffff_ffff;

pub const SYS_gettid: ::c_long = 5178; // Valid for n64

#[link(name = "util")]
extern "C" {
    pub fn ioctl(fd: ::c_int, request: ::c_ulong, ...) -> ::c_int;
}

cfg_if! {
    if #[cfg(libc_align)] {
        mod align;
        pub use self::align::*;
    } else {
        mod no_align;
        pub use self::no_align::*;
    }
}
