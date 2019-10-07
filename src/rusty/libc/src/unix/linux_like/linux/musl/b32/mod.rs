pub type c_long = i32;
pub type c_ulong = u32;
pub type nlink_t = u32;
pub type blksize_t = ::c_long;
pub type __u64 = ::c_ulonglong;

s! {
    pub struct pthread_attr_t {
        __size: [u32; 9]
    }

    pub struct sigset_t {
        __val: [::c_ulong; 32],
    }

    pub struct msghdr {
        pub msg_name: *mut ::c_void,
        pub msg_namelen: ::socklen_t,
        pub msg_iov: *mut ::iovec,
        pub msg_iovlen: ::c_int,
        pub msg_control: *mut ::c_void,
        pub msg_controllen: ::socklen_t,
        pub msg_flags: ::c_int,
    }

    pub struct cmsghdr {
        pub cmsg_len: ::socklen_t,
        pub cmsg_level: ::c_int,
        pub cmsg_type: ::c_int,
    }

    pub struct sem_t {
        __val: [::c_int; 4],
    }
}

pub const __SIZEOF_PTHREAD_RWLOCK_T: usize = 32;
pub const __SIZEOF_PTHREAD_MUTEX_T: usize = 24;

pub const TIOCINQ: ::c_int = ::FIONREAD;

extern "C" {
    pub fn ioctl(fd: ::c_int, request: ::c_int, ...) -> ::c_int;
}

cfg_if! {
    if #[cfg(any(target_arch = "x86"))] {
        mod x86;
        pub use self::x86::*;
    } else if #[cfg(any(target_arch = "mips"))] {
        mod mips;
        pub use self::mips::*;
    } else if #[cfg(any(target_arch = "arm"))] {
        mod arm;
        pub use self::arm::*;
    } else if #[cfg(any(target_arch = "powerpc"))] {
        mod powerpc;
        pub use self::powerpc::*;
    } else if #[cfg(any(target_arch = "hexagon"))] {
        mod hexagon;
        pub use self::hexagon::*;
    } else {
        // Unknown target_arch
    }
}
