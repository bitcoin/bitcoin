//! Definitions for uclibc on 64bit systems
pub type blkcnt_t = i64;
pub type blksize_t = i64;
pub type clock_t = i64;
pub type c_char = u8;
pub type c_long = i64;
pub type c_ulong = u64;
pub type fsblkcnt_t = ::c_ulong;
pub type fsfilcnt_t = ::c_ulong;
pub type fsword_t = ::c_long;
pub type ino_t = ::c_ulong;
pub type nlink_t = ::c_uint;
pub type off_t = ::c_long;
pub type rlim_t = c_ulong;
pub type rlim64_t = u64;
// [uClibc docs] Note stat64 has the same shape as stat for x86-64.
pub type stat64 = stat;
pub type suseconds_t = ::c_long;
pub type time_t = ::c_int;
pub type wchar_t = ::c_int;

s! {
    pub struct ipc_perm {
        pub __key: ::key_t,
        pub uid: ::uid_t,
        pub gid: ::gid_t,
        pub cuid: ::uid_t,
        pub cgid: ::gid_t,
        pub mode: ::c_ushort, // read / write
        __pad1: ::c_ushort,
        pub __seq: ::c_ushort,
        __pad2: ::c_ushort,
        __unused1: ::c_ulong,
        __unused2: ::c_ulong
    }

    #[cfg(not(target_os = "l4re"))]
    pub struct pthread_attr_t {
        __detachstate: ::c_int,
        __schedpolicy: ::c_int,
        __schedparam: __sched_param,
        __inheritsched: ::c_int,
        __scope: ::c_int,
        __guardsize: ::size_t,
        __stackaddr_set: ::c_int,
        __stackaddr: *mut ::c_void, // better don't use it
        __stacksize: ::size_t,
    }

    pub struct __sched_param {
        __sched_priority: ::c_int,
    }

    pub struct siginfo_t {
        si_signo: ::c_int, // signal number
        si_errno: ::c_int, // if not zero: error value of signal, see errno.h
        si_code: ::c_int,  // signal code
        pub _pad: [::c_int; 28], // unported union
        _align: [usize; 0],
    }

    pub struct shmid_ds {
        pub shm_perm: ::ipc_perm,
        pub shm_segsz: ::size_t, // segment size in bytes
        pub shm_atime: ::time_t, // time of last shmat()
        pub shm_dtime: ::time_t,
        pub shm_ctime: ::time_t,
        pub shm_cpid: ::pid_t,
        pub shm_lpid: ::pid_t,
        pub shm_nattch: ::shmatt_t,
        __unused1: ::c_ulong,
        __unused2: ::c_ulong
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
        __ignored1: ::c_ulong,
        __ignored2: ::c_ulong,
    }

    pub struct sockaddr {
        pub sa_family: ::sa_family_t,
        pub sa_data: [::c_char; 14],
    }

    pub struct sockaddr_in {
        pub sin_family: ::sa_family_t,
        pub sin_port: ::in_port_t,
        pub sin_addr: ::in_addr,
        pub sin_zero: [u8; 8],
    }

    pub struct sockaddr_in6 {
        pub sin6_family: ::sa_family_t,
        pub sin6_port: ::in_port_t,
        pub sin6_flowinfo: u32,
        pub sin6_addr: ::in6_addr,
        pub sin6_scope_id: u32,
    }

    // ------------------------------------------------------------
    // definitions below are *unverified* and might **break** the software
//    pub struct in_addr {
//        pub s_addr: in_addr_t,
//    }
//
//    pub struct in6_addr {
//        pub s6_addr: [u8; 16],
//        #[cfg(not(libc_align))]
//        __align: [u32; 0],
//    }

    pub struct stat {
        pub st_dev: ::c_ulong,
        pub st_ino: ::ino_t,
        // According to uclibc/libc/sysdeps/linux/x86_64/bits/stat.h, order of
        // nlink and mode are swapped on 64 bit systems.
        pub st_nlink: ::nlink_t,
        pub st_mode: ::mode_t,
        pub st_uid: ::uid_t,
        pub st_gid: ::gid_t,
        pub st_rdev: ::c_ulong, // dev_t
        pub st_size: off_t, // file size
        pub st_blksize: ::blksize_t,
        pub st_blocks: ::blkcnt_t,
        pub st_atime: ::time_t,
        pub st_atime_nsec: ::c_ulong,
        pub st_mtime: ::time_t,
        pub st_mtime_nsec: ::c_ulong,
        pub st_ctime: ::time_t,
        pub st_ctime_nsec: ::c_ulong,
        st_pad4: [::c_long; 3]
    }

    pub struct sigaction {
        pub sa_handler: ::sighandler_t,
        pub sa_flags: ::c_ulong,
        pub sa_restorer: *mut ::c_void,
        pub sa_mask: ::sigset_t,
    }

    pub struct stack_t { // ToDo
        pub ss_sp: *mut ::c_void,
        pub ss_flags: ::c_int,
        pub ss_size: ::size_t
    }

    pub struct statfs { // ToDo
        pub f_type: fsword_t,
        pub f_bsize: fsword_t,
        pub f_blocks: ::fsblkcnt_t,
        pub f_bfree: ::fsblkcnt_t,
        pub f_bavail: ::fsblkcnt_t,
        pub f_files: ::fsfilcnt_t,
        pub f_ffree: ::fsfilcnt_t,
        pub f_fsid: ::fsid_t,
        pub f_namelen: fsword_t,
        pub f_frsize: fsword_t,
        f_spare: [fsword_t; 5],
    }

    pub struct msghdr { // ToDo
        pub msg_name: *mut ::c_void,
        pub msg_namelen: ::socklen_t,
        pub msg_iov: *mut ::iovec,
        pub msg_iovlen: ::size_t,
        pub msg_control: *mut ::c_void,
        pub msg_controllen: ::size_t,
        pub msg_flags: ::c_int,
    }

    pub struct termios { // ToDo
        pub c_iflag: ::tcflag_t,
        pub c_oflag: ::tcflag_t,
        pub c_cflag: ::tcflag_t,
        pub c_lflag: ::tcflag_t,
        pub c_line: ::cc_t,
        pub c_cc: [::cc_t; ::NCCS],
    }

    pub struct sigset_t { // ToDo
        __val: [::c_ulong; 16],
    }

    pub struct sysinfo { // ToDo
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

    pub struct glob_t { // ToDo
        pub gl_pathc: ::size_t,
        pub gl_pathv: *mut *mut c_char,
        pub gl_offs: ::size_t,
        pub gl_flags: ::c_int,
        __unused1: *mut ::c_void,
        __unused2: *mut ::c_void,
        __unused3: *mut ::c_void,
        __unused4: *mut ::c_void,
        __unused5: *mut ::c_void,
    }

    pub struct rlimit64 { // ToDo
        pub rlim_cur: rlim64_t,
        pub rlim_max: rlim64_t,
    }

    pub struct cpu_set_t { // ToDo
        #[cfg(target_pointer_width = "32")]
        bits: [u32; 32],
        #[cfg(target_pointer_width = "64")]
        bits: [u64; 16],
    }

    pub struct fsid_t { // ToDo
        __val: [::c_int; 2],
    }
}

s_no_extra_traits! {
    #[allow(missing_debug_implementations)]
    pub struct dirent {
        pub d_ino: ::ino64_t,
        pub d_off: ::off64_t,
        pub d_reclen: u16,
        pub d_type: u8,
        pub d_name: [::c_char; 256],
    }
    #[allow(missing_debug_implementations)]
    pub struct dirent64 {
        pub d_ino: ::ino64_t,
        pub d_off: ::off64_t,
        pub d_reclen: u16,
        pub d_type: u8,
        pub d_name: [::c_char; 256],
    }
}

// constants
pub const EADDRINUSE: ::c_int = 98; // Address already in use
pub const EADDRNOTAVAIL: ::c_int = 99; // Cannot assign requested address
pub const ECONNABORTED: ::c_int = 103; // Software caused connection abort
pub const ECONNREFUSED: ::c_int = 111; // Connection refused
pub const ECONNRESET: ::c_int = 104; // Connection reset by peer
pub const EDEADLK: ::c_int = 35; // Resource deadlock would occur
pub const ENOSYS: ::c_int = 38; // Function not implemented
pub const ENOTCONN: ::c_int = 107; // Transport endpoint is not connected
pub const ETIMEDOUT: ::c_int = 110; // connection timed out
pub const O_APPEND: ::c_int = 02000;
pub const O_ACCMODE: ::c_int = 0003;
pub const O_CLOEXEC: ::c_int = 0x80000;
pub const O_CREAT: ::c_int = 0100;
pub const O_DIRECTORY: ::c_int = 0200000;
pub const O_EXCL: ::c_int = 0200;
pub const O_NONBLOCK: ::c_int = 04000;
pub const O_TRUNC: ::c_int = 01000;
pub const NCCS: usize = 32;
pub const SIG_SETMASK: ::c_int = 2; // Set the set of blocked signals
pub const __SIZEOF_PTHREAD_MUTEX_T: usize = 40;
pub const __SIZEOF_PTHREAD_MUTEXATTR_T: usize = 4;
pub const SO_BROADCAST: ::c_int = 6;
pub const SOCK_DGRAM: ::c_int = 2; // connectionless, unreliable datagrams
pub const SOCK_STREAM: ::c_int = 1; // …/common/bits/socket_type.h
pub const SO_ERROR: ::c_int = 4;
pub const SOL_SOCKET: ::c_int = 1;
pub const SO_RCVTIMEO: ::c_int = 20;
pub const SO_REUSEADDR: ::c_int = 2;
pub const SO_SNDTIMEO: ::c_int = 21;
pub const RLIM_INFINITY: u64 = 0xffffffffffffffff;
pub const __SIZEOF_PTHREAD_COND_T: usize = 48;
pub const __SIZEOF_PTHREAD_CONDATTR_T: usize = 4;
pub const __SIZEOF_PTHREAD_RWLOCK_T: usize = 56;

cfg_if! {
    if #[cfg(target_os = "l4re")] {
        mod l4re;
        pub use self::l4re::*;
    } else {
        mod other;
        pub use other::*;
    }
}

cfg_if! {
    if #[cfg(libc_align)] {
        #[macro_use]
        mod align;
    } else {
        #[macro_use]
        mod no_align;
    }
}
expand_align!();
