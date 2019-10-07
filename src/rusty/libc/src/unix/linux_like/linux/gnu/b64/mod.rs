//! 64-bit specific definitions for linux-like values

pub type clock_t = i64;
pub type time_t = i64;
pub type ino_t = u64;
pub type off_t = i64;
pub type blkcnt_t = i64;
pub type shmatt_t = u64;
pub type msgqnum_t = u64;
pub type msglen_t = u64;
pub type fsblkcnt_t = u64;
pub type fsfilcnt_t = u64;
pub type rlim_t = u64;
pub type __fsword_t = i64;

s! {
    pub struct sigset_t {
        #[cfg(target_pointer_width = "32")]
        __val: [u32; 32],
        #[cfg(target_pointer_width = "64")]
        __val: [u64; 16],
    }

    pub struct sysinfo {
        pub uptime: i64,
        pub loads: [u64; 3],
        pub totalram: u64,
        pub freeram: u64,
        pub sharedram: u64,
        pub bufferram: u64,
        pub totalswap: u64,
        pub freeswap: u64,
        pub procs: ::c_ushort,
        pub pad: ::c_ushort,
        pub totalhigh: u64,
        pub freehigh: u64,
        pub mem_unit: ::c_uint,
        pub _f: [::c_char; 0],
    }

    pub struct msqid_ds {
        pub msg_perm: ::ipc_perm,
        pub msg_stime: ::time_t,
        pub msg_rtime: ::time_t,
        pub msg_ctime: ::time_t,
        __msg_cbytes: u64,
        pub msg_qnum: ::msgqnum_t,
        pub msg_qbytes: ::msglen_t,
        pub msg_lspid: ::pid_t,
        pub msg_lrpid: ::pid_t,
        __glibc_reserved4: u64,
        __glibc_reserved5: u64,
    }

}

pub const RLIM_INFINITY: ::rlim_t = !0;
pub const __SIZEOF_PTHREAD_RWLOCKATTR_T: usize = 8;

pub const O_LARGEFILE: ::c_int = 0;

cfg_if! {
    if #[cfg(target_arch = "aarch64")] {
        mod aarch64;
        pub use self::aarch64::*;
    } else if #[cfg(any(target_arch = "powerpc64"))] {
        mod powerpc64;
        pub use self::powerpc64::*;
    } else if #[cfg(any(target_arch = "sparc64"))] {
        mod sparc64;
        pub use self::sparc64::*;
    } else if #[cfg(any(target_arch = "mips64"))] {
        mod mips64;
        pub use self::mips64::*;
    } else if #[cfg(any(target_arch = "s390x"))] {
        mod s390x;
        pub use self::s390x::*;
    } else if #[cfg(any(target_arch = "x86_64"))] {
        mod x86_64;
        pub use self::x86_64::*;
    } else {
        // Unknown target_arch
    }
}
