// Copyright 2018 The Rust Project Developers. See the COPYRIGHT
// file at the top-level directory of this distribution and at
// http://rust-lang.org/COPYRIGHT.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

// liblibc port for HermitCore (https://hermitcore.org)
// HermitCore is a unikernel based on lwIP, newlib, and
// pthread-embedded.
// Consider these definitions when porting liblibc to another
// lwIP/newlib/pte-based target.
//
// Ported by Colin Finck <colin.finck@rwth-aachen.de>

pub type c_long = i64;
pub type c_ulong = u64;

pub type speed_t = ::c_uint;
pub type mode_t = u32;
pub type dev_t = i16;
pub type nfds_t = ::c_ulong;
pub type socklen_t = u32;
pub type sa_family_t = u8;
pub type clock_t = c_ulong;
pub type time_t = c_long;
pub type suseconds_t = c_long;
pub type off_t = i64;
pub type rlim_t = ::c_ulonglong;
pub type sigset_t = ::c_ulong;
pub type ino_t = u16;
pub type nlink_t = u16;
pub type blksize_t = c_long;
pub type blkcnt_t = c_long;
pub type stat64 = stat;
pub type clockid_t = c_ulong;
pub type pthread_t = pte_handle_t;
pub type pthread_attr_t = usize;
pub type pthread_cond_t = usize;
pub type pthread_condattr_t = usize;
pub type pthread_key_t = usize;
pub type pthread_mutex_t = usize;
pub type pthread_mutexattr_t = usize;
pub type pthread_rwlock_t = usize;
pub type pthread_rwlockattr_t = usize;

s_no_extra_traits! {
    pub struct dirent {
        pub d_ino: ::c_long,
        pub d_off: off_t,
        pub d_reclen: u16,
        pub d_name: [::c_char; 256],
    }

    // Dummy
    pub struct sockaddr_un {
        pub sun_family: sa_family_t,
        pub sun_path: [::c_char; 108],
    }

    pub struct sockaddr {
        pub sa_len: u8,
        pub sa_family: sa_family_t,
        pub sa_data: [::c_char; 14],
    }

    pub struct sockaddr_in {
        pub sin_len: u8,
        pub sin_family: sa_family_t,
        pub sin_port: ::in_port_t,
        pub sin_addr: ::in_addr,
        pub sin_zero: [::c_char; 8],
    }

    pub struct fd_set {
        fds_bits: [::c_ulong; FD_SETSIZE / ULONG_SIZE],
    }

    pub struct sockaddr_storage {
        pub s2_len: u8,
        pub ss_family: sa_family_t,
        pub s2_data1: [::c_char; 2],
        pub s2_data2: [u32; 3],
        pub s2_data3: [u32; 3],
    }

    pub struct stat {
        pub st_dev: ::dev_t,
        pub st_ino: ::ino_t,
        pub st_mode: ::mode_t,
        pub st_nlink: ::nlink_t,
        pub st_uid: ::uid_t,
        pub st_gid: ::gid_t,
        pub st_rdev: dev_t,
        pub st_size: off_t,
        pub st_atime: time_t,
        pub st_atime_nsec: ::c_long,
        pub st_mtime: time_t,
        pub st_mtime_nsec: ::c_long,
        pub st_ctime: time_t,
        pub st_ctime_nsec: ::c_long,
        pub st_blksize: blksize_t,
        pub st_blocks: blkcnt_t,
        pub st_spare4: [::c_long; 2],
    }
}

cfg_if! {
    if #[cfg(feature = "extra_traits")] {
        impl PartialEq for dirent {
            fn eq(&self, other: &dirent) -> bool {
                self.d_ino == other.d_ino
                    && self.d_off == other.d_off
                    && self.d_reclen == other.d_reclen
                    && self
                    .d_name
                    .iter()
                    .zip(other.d_name.iter())
                    .all(|(a,b)| a == b)
            }
        }
        impl Eq for dirent {}
        impl ::fmt::Debug for dirent {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("dirent")
                    .field("d_ino", &self.d_ino)
                    .field("d_off", &self.d_off)
                    .field("d_reclen", &self.d_reclen)
                    // FIXME: .field("d_name", &self.d_name)
                    .finish()
            }
        }
        impl ::hash::Hash for dirent {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.d_ino.hash(state);
                self.d_off.hash(state);
                self.d_reclen.hash(state);
                self.d_name.hash(state);
            }
        }

        impl PartialEq for sockaddr_un {
            fn eq(&self, other: &sockaddr_un) -> bool {
                self.sun_family == other.sun_family
                    && self
                    .sun_path
                    .iter()
                    .zip(other.sun_path.iter())
                    .all(|(a,b)| a == b)
            }
        }
        impl Eq for sockaddr_un {}
        impl ::fmt::Debug for sockaddr_un {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("sockaddr_un")
                    .field("sun_family", &self.sun_family)
                    // FIXME: .field("sun_path", &self.sun_path)
                    .finish()
            }
        }
        impl ::hash::Hash for sockaddr_un {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.sun_family.hash(state);
                self.sun_path.hash(state);
            }
        }

        impl PartialEq for sockaddr {
            fn eq(&self, other: &sockaddr) -> bool {
                self.sa_len == other.sa_len
                    && self.sa_family == other.sa_family
                    && self
                    .sa_data
                    .iter()
                    .zip(other.sa_data.iter())
                    .all(|(a,b)| a == b)
            }
        }
        impl Eq for sockaddr {}
        impl ::fmt::Debug for sockaddr {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("sockaddr")
                    .field("sa_len", &self.sa_len)
                    .field("sa_family", &self.sa_family)
                    // FIXME: .field("sa_data", &self.sa_data)
                    .finish()
            }
        }
        impl ::hash::Hash for sockaddr {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.sa_len.hash(state);
                self.sa_family.hash(state);
                self.sa_data.hash(state);
            }
        }

        impl PartialEq for sockaddr_in {
            fn eq(&self, other: &sockaddr_in) -> bool {
                self.sin_len == other.sin_len
                    && self.sin_family == other.sin_family
                    && self.sin_port == other.sin_port
                    && self.sin_addr == other.sin_addr
                    && self
                    .sin_zero
                    .iter()
                    .zip(other.sin_zero.iter())
                    .all(|(a,b)| a == b)
            }
        }
        impl Eq for sockaddr_in {}
        impl ::fmt::Debug for sockaddr_in {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("sockaddr_in")
                    .field("sin_len", &self.sin_len)
                    .field("sin_family", &self.sin_family)
                    .field("sin_port", &self.sin_port)
                    .field("sin_addr", &self.sin_addr)
                    // FIXME: .field("sin_zero", &self.sin_zero)
                    .finish()
            }
        }
        impl ::hash::Hash for sockaddr_in {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.sin_len.hash(state);
                self.sin_family.hash(state);
                self.sin_port.hash(state);
                self.sin_addr.hash(state);
                self.sin_zero.hash(state);
            }
        }

        impl PartialEq for fd_set {
            fn eq(&self, other: &fd_set) -> bool {
                self.fds_bits
                    .iter()
                    .zip(other.fds_bits.iter())
                    .all(|(a,b)| a == b)
            }
        }
        impl Eq for fd_set {}
        impl ::fmt::Debug for fd_set {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("fd_set")
                    // FIXME: .field("fds_bits", &self.fds_bits)
                    .finish()
            }
        }
        impl ::hash::Hash for fd_set {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.fds_bits.hash(state);
            }
        }

        impl PartialEq for sockaddr_storage {
            fn eq(&self, other: &sockaddr_storage) -> bool {
                self.s2_len == other.s2_len
                    && self.ss_family == other.ss_family
                    && self.s2_data1
                    .iter()
                    .zip(other.s2_data1.iter())
                    .all(|(a,b)| a == b)
                    && self.s2_data2
                    .iter()
                    .zip(other.s2_data2.iter())
                    .all(|(a,b)| a == b)
                    && self.s2_data3
                    .iter()
                    .zip(other.s2_data3.iter())
                    .all(|(a,b)| a == b)
            }
        }
        impl Eq for sockaddr_storage {}
        impl ::fmt::Debug for sockaddr_storage {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("sockaddr_storage")
                    .field("s2_len", &self.s2_len)
                    .field("ss_family", &self.ss_family)
                    // FIXME: .field("s2_data1", &self.s2_data1)
                    // FIXME: .field("s2_data2", &self.s2_data2)
                    // FIXME: .field("s2_data3", &self.s2_data3)
                    .finish()
            }
        }
        impl ::hash::Hash for sockaddr_storage {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.s2_len.hash(state);
                self.ss_family.hash(state);
                self.s2_data1.hash(state);
                self.s2_data2.hash(state);
                self.s2_data3.hash(state);
            }
        }

        impl PartialEq for stat {
            fn eq(&self, other: &stat) -> bool {
                self.st_dev == other.st_dev
                    && self.st_ino == other.st_ino
                    && self.st_mode == other.st_mode
                    && self.st_nlink == other.st_nlink
                    && self.st_uid == other.st_uid
                    && self.st_gid == other.st_gid
                    && self.st_rdev == other.st_rdev
                    && self.st_size == other.st_size
                    && self.st_atime == other.st_atime
                    && self.st_atime_nsec == other.st_atime_nsec
                    && self.st_mtime == other.st_mtime
                    && self.st_mtime_nsec == other.st_mtime_nsec
                    && self.st_ctime == other.st_ctime
                    && self.st_ctime_nsec == other.st_ctime_nsec
                    && self.st_blksize == other.st_blksize
                    && self.st_blocks == other.st_blocks
                    && self
                    .st_spare4
                    .iter()
                    .zip(other.st_spare4.iter())
                    .all(|(a,b)| a == b)
            }
        }
        impl Eq for stat {}
        impl ::fmt::Debug for stat {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("stat")
                    .field("st_dev", &self.st_dev)
                    .field("st_ino", &self.st_ino)
                    .field("st_mode", &self.st_mode)
                    .field("st_nlink", &self.st_nlink)
                    .field("st_uid", &self.st_uid)
                    .field("st_gid", &self.st_gid)
                    .field("st_rdev", &self.st_rdev)
                    .field("st_size", &self.st_size)
                    .field("st_atime", &self.st_atime)
                    .field("st_atime_nsec", &self.st_atime_nsec)
                    .field("st_mtime", &self.st_mtime)
                    .field("st_mtime_nsec", &self.st_mtime_nsec)
                    .field("st_ctime", &self.st_ctime)
                    .field("st_ctime_nsec", &self.st_ctime_nsec)
                    .field("st_blksize", &self.st_blksize)
                    .field("st_blocks", &self.st_blocks)
                    // FIXME: .field("st_spare4", &self.st_spare4)
                    .finish()
            }
        }
        impl ::hash::Hash for stat {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.st_dev.hash(state);
                self.st_ino.hash(state);
                self.st_mode.hash(state);
                self.st_nlink.hash(state);
                self.st_uid.hash(state);
                self.st_gid.hash(state);
                self.st_rdev.hash(state);
                self.st_size.hash(state);
                self.st_atime.hash(state);
                self.st_atime_nsec.hash(state);
                self.st_mtime.hash(state);
                self.st_mtime_nsec.hash(state);
                self.st_ctime.hash(state);
                self.st_ctime_nsec.hash(state);
                self.st_blksize.hash(state);
                self.st_blocks.hash(state);
                self.st_spare4.hash(state);
            }
        }
    }
}

s! {
    pub struct in_addr {
        pub s_addr: ::in_addr_t,
    }

    pub struct ip_mreq {
        pub imr_multiaddr: in_addr,
        pub imr_interface: in_addr,
    }

    pub struct addrinfo {
        pub ai_flags: ::c_int,
        pub ai_family: ::c_int,
        pub ai_socktype: ::c_int,
        pub ai_protocol: ::c_int,
        pub ai_addrlen: socklen_t,
        pub ai_addr: *mut ::sockaddr,
        pub ai_canonname: *mut c_char,
        pub ai_next: *mut addrinfo,
    }

    pub struct Dl_info {}

    pub struct lconv {
        pub decimal_point: *mut ::c_char,
        pub thousands_sep: *mut ::c_char,
        pub grouping: *mut ::c_char,
        pub int_curr_symbol: *mut ::c_char,
        pub currency_symbol: *mut ::c_char,
        pub mon_decimal_point: *mut ::c_char,
        pub mon_thousands_sep: *mut ::c_char,
        pub mon_grouping: *mut ::c_char,
        pub positive_sign: *mut ::c_char,
        pub negative_sign: *mut ::c_char,
        pub int_frac_digits: ::c_char,
        pub frac_digits: ::c_char,
        pub p_cs_precedes: ::c_char,
        pub p_sep_by_space: ::c_char,
        pub n_cs_precedes: ::c_char,
        pub n_sep_by_space: ::c_char,
        pub p_sign_posn: ::c_char,
        pub n_sign_posn: ::c_char,
        pub int_p_cs_precedes: ::c_char,
        pub int_p_sep_by_space: ::c_char,
        pub int_n_cs_precedes: ::c_char,
        pub int_n_sep_by_space: ::c_char,
        pub int_p_sign_posn: ::c_char,
        pub int_n_sign_posn: ::c_char,
    }

    pub struct passwd { // Unverified
        pub pw_name: *mut ::c_char,
        pub pw_passwd: *mut ::c_char,
        pub pw_uid: ::uid_t,
        pub pw_gid: ::gid_t,
        pub pw_gecos: *mut ::c_char,
        pub pw_dir: *mut ::c_char,
        pub pw_shell: *mut ::c_char,
    }

    pub struct pte_handle_t {
        pub p: usize,
        pub x: ::c_uint,
    }

    pub struct sched_param {
        pub sched_priority: ::c_int,
    }

    pub struct sem_t {
        pub value: i32,
        pub lock: usize,
        pub sem: usize,
    }

    pub struct sigaction {
        pub sa_flags: ::c_int,
        pub sa_mask: sigset_t,
        pub sa_handler: usize,
    }

    pub struct sockaddr_in6 {
        pub sin6_len: u8,
        pub sin6_family: sa_family_t,
        pub sin6_port: ::in_port_t,
        pub sin6_flowinfo: u32,
        pub sin6_addr: ::in6_addr,
        pub sin6_scope_id: u32,
    }

    pub struct statvfs {}

    pub struct tm {
        pub tm_sec: ::c_int,
        pub tm_min: ::c_int,
        pub tm_hour: ::c_int,
        pub tm_mday: ::c_int,
        pub tm_mon: ::c_int,
        pub tm_year: ::c_int,
        pub tm_wday: ::c_int,
        pub tm_yday: ::c_int,
        pub tm_isdst: ::c_int,
    }

    pub struct tms {
        pub tms_utime: ::clock_t,
        pub tms_stime: ::clock_t,
        pub tms_cutime: ::clock_t,
        pub tms_cstime: ::clock_t,
    }

    pub struct termios {}

    pub struct utsname {}
}

pub const AF_UNSPEC: ::c_int = 0;
pub const AF_INET: ::c_int = 2;
pub const AF_INET6: ::c_int = 10;

// Dummy
pub const AF_UNIX: ::c_int = 1;

pub const CLOCK_REALTIME: ::clockid_t = 1;
pub const CLOCK_MONOTONIC: ::clockid_t = 4;

// Dummy
pub const EAI_SYSTEM: ::c_int = -11;

pub const EPERM: ::c_int = 1;
pub const ENOENT: ::c_int = 2;
pub const ESRCH: ::c_int = 3;
pub const EINTR: ::c_int = 4;
pub const EIO: ::c_int = 5;
pub const ENXIO: ::c_int = 6;
pub const E2BIG: ::c_int = 7;
pub const ENOEXEC: ::c_int = 8;
pub const EBADF: ::c_int = 9;
pub const ECHILD: ::c_int = 10;
pub const EAGAIN: ::c_int = 11;
pub const ENOMEM: ::c_int = 12;
pub const EACCES: ::c_int = 13;
pub const EFAULT: ::c_int = 14;
pub const EBUSY: ::c_int = 16;
pub const EEXIST: ::c_int = 17;
pub const EXDEV: ::c_int = 18;
pub const ENODEV: ::c_int = 19;
pub const ENOTDIR: ::c_int = 20;
pub const EISDIR: ::c_int = 21;
pub const EINVAL: ::c_int = 22;
pub const ENFILE: ::c_int = 23;
pub const EMFILE: ::c_int = 24;
pub const ENOTTY: ::c_int = 25;
pub const ETXTBSY: ::c_int = 26;
pub const EFBIG: ::c_int = 27;
pub const ENOSPC: ::c_int = 28;
pub const ESPIPE: ::c_int = 29;
pub const EROFS: ::c_int = 30;
pub const EMLINK: ::c_int = 31;
pub const EPIPE: ::c_int = 32;
pub const EDOM: ::c_int = 33;
pub const ERANGE: ::c_int = 34;
pub const EDEADLK: ::c_int = 35;
pub const ENAMETOOLONG: ::c_int = 36;
pub const ENOLCK: ::c_int = 37;
pub const ENOSYS: ::c_int = 38;
pub const ENOTEMPTY: ::c_int = 39;
pub const ELOOP: ::c_int = 40;
pub const EWOULDBLOCK: ::c_int = EAGAIN;
pub const ENOMSG: ::c_int = 42;
pub const EIDRM: ::c_int = 43;
pub const ECHRNG: ::c_int = 44;
pub const EL2NSYNC: ::c_int = 45;
pub const EL3HLT: ::c_int = 46;
pub const EL3RST: ::c_int = 47;
pub const ELNRNG: ::c_int = 48;
pub const EUNATCH: ::c_int = 49;
pub const ENOCSI: ::c_int = 50;
pub const EL2HLT: ::c_int = 51;
pub const EBADE: ::c_int = 52;
pub const EBADR: ::c_int = 53;
pub const EXFULL: ::c_int = 54;
pub const ENOANO: ::c_int = 55;
pub const EBADRQC: ::c_int = 56;
pub const EBADSLT: ::c_int = 57;
pub const EDEADLOCK: ::c_int = EDEADLK;
pub const EBFONT: ::c_int = 59;
pub const ENOSTR: ::c_int = 60;
pub const ENODATA: ::c_int = 61;
pub const ETIME: ::c_int = 62;
pub const ENOSR: ::c_int = 63;
pub const ENONET: ::c_int = 64;
pub const ENOPKG: ::c_int = 65;
pub const EREMOTE: ::c_int = 66;
pub const ENOLINK: ::c_int = 67;
pub const EADV: ::c_int = 68;
pub const ESRMNT: ::c_int = 69;
pub const ECOMM: ::c_int = 70;
pub const EPROTO: ::c_int = 71;
pub const EMULTIHOP: ::c_int = 72;
pub const EDOTDOT: ::c_int = 73;
pub const EBADMSG: ::c_int = 74;
pub const EOVERFLOW: ::c_int = 75;
pub const ENOTUNIQ: ::c_int = 76;
pub const EBADFD: ::c_int = 77;
pub const EREMCHG: ::c_int = 78;
pub const ELIBACC: ::c_int = 79;
pub const ELIBBAD: ::c_int = 80;
pub const ELIBSCN: ::c_int = 81;
pub const ELIBMAX: ::c_int = 82;
pub const ELIBEXEC: ::c_int = 83;
pub const EILSEQ: ::c_int = 84;
pub const ERESTART: ::c_int = 85;
pub const ESTRPIPE: ::c_int = 86;
pub const EUSERS: ::c_int = 87;
pub const ENOTSOCK: ::c_int = 88;
pub const EDESTADDRREQ: ::c_int = 89;
pub const EMSGSIZE: ::c_int = 90;
pub const EPROTOTYPE: ::c_int = 91;
pub const ENOPROTOOPT: ::c_int = 92;
pub const EPROTONOSUPPORT: ::c_int = 93;
pub const ESOCKTNOSUPPORT: ::c_int = 94;
pub const EOPNOTSUPP: ::c_int = 95;
pub const EPFNOSUPPORT: ::c_int = 96;
pub const EAFNOSUPPORT: ::c_int = 97;
pub const EADDRINUSE: ::c_int = 98;
pub const EADDRNOTAVAIL: ::c_int = 99;
pub const ENETDOWN: ::c_int = 100;
pub const ENETUNREACH: ::c_int = 101;
pub const ENETRESET: ::c_int = 102;
pub const ECONNABORTED: ::c_int = 103;
pub const ECONNRESET: ::c_int = 104;
pub const ENOBUFS: ::c_int = 105;
pub const EISCONN: ::c_int = 106;
pub const ENOTCONN: ::c_int = 107;
pub const ESHUTDOWN: ::c_int = 108;
pub const ETOOMANYREFS: ::c_int = 109;
pub const ETIMEDOUT: ::c_int = 110;
pub const ECONNREFUSED: ::c_int = 111;
pub const EHOSTDOWN: ::c_int = 112;
pub const EHOSTUNREACH: ::c_int = 113;
pub const EALREADY: ::c_int = 114;
pub const EINPROGRESS: ::c_int = 115;
pub const ESTALE: ::c_int = 116;
pub const EUCLEAN: ::c_int = 117;
pub const ENOTNAM: ::c_int = 118;
pub const ENAVAIL: ::c_int = 119;
pub const EISNAM: ::c_int = 120;
pub const EREMOTEIO: ::c_int = 121;
pub const EDQUOT: ::c_int = 122;
pub const ENOMEDIUM: ::c_int = 123;
pub const EMEDIUMTYPE: ::c_int = 124;
pub const ECANCELED: ::c_int = 125;
pub const ENOKEY: ::c_int = 126;
pub const EKEYEXPIRED: ::c_int = 127;
pub const EKEYREVOKED: ::c_int = 128;
pub const EKEYREJECTED: ::c_int = 129;
pub const EOWNERDEAD: ::c_int = 130;
pub const ENOTRECOVERABLE: ::c_int = 131;
pub const ERFKILL: ::c_int = 132;
pub const EHWPOISON: ::c_int = 133;

pub const EXIT_FAILURE: ::c_int = 1;
pub const EXIT_SUCCESS: ::c_int = 0;

pub const F_DUPFD: ::c_int = 0;
pub const F_GETFD: ::c_int = 1;
pub const F_SETFD: ::c_int = 2;
pub const F_GETFL: ::c_int = 3;
pub const F_SETFL: ::c_int = 4;
pub const F_GETOWN: ::c_int = 5;
pub const F_SETOWN: ::c_int = 6;
pub const F_GETLK: ::c_int = 7;
pub const F_SETLK: ::c_int = 8;
pub const F_SETLKW: ::c_int = 9;
pub const F_RGETLK: ::c_int = 10;
pub const F_RSETLK: ::c_int = 11;
pub const F_CNVT: ::c_int = 12;
pub const F_RSETLKW: ::c_int = 13;
pub const F_DUPFD_CLOEXEC: ::c_int = 14;

pub const FD_SETSIZE: usize = 1024;

// Dummy
pub const FIOCLEX: ::c_int = 0x5451;

pub const FIONBIO: ::c_int = 0x8004667e;
pub const FIONREAD: ::c_int = 0x4004667f;

pub const IP_ADD_MEMBERSHIP: ::c_int = 3;
pub const IP_DROP_MEMBERSHIP: ::c_int = 4;

pub const IP_TOS: ::c_int = 1;
pub const IP_TTL: ::c_int = 2;

pub const IP_MULTICAST_TTL: ::c_int = 5;
pub const IP_MULTICAST_IF: ::c_int = 6;
pub const IP_MULTICAST_LOOP: ::c_int = 7;

pub const IPV6_JOIN_GROUP: ::c_int = 12;
pub const IPV6_ADD_MEMBERSHIP: ::c_int = 12;
pub const IPV6_LEAVE_GROUP: ::c_int = 13;
pub const IPV6_DROP_MEMBERSHIP: ::c_int = 13;
pub const IPV6_V6ONLY: ::c_int = 27;

// Dummy
pub const IPV6_MULTICAST_LOOP: ::c_int = 7;

pub const MSG_PEEK: ::c_int = 0x01;
pub const MSG_WAITALL: ::c_int = 0x02;
pub const MSG_OOB: ::c_int = 0x04;
pub const MSG_DONTWAIT: ::c_int = 0x08;
pub const MSG_MORE: ::c_int = 0x10;

pub const O_ACCMODE: ::c_int = 3;
pub const O_RDONLY: ::c_int = 0;
pub const O_WRONLY: ::c_int = 1;
pub const O_RDWR: ::c_int = 2;
pub const O_APPEND: ::c_int = 1024;
pub const O_CREAT: ::c_int = 64;
pub const O_EXCL: ::c_int = 128;
pub const O_NOCTTY: ::c_int = 256;
pub const O_NONBLOCK: ::c_int = 2048;
pub const O_TRUNC: ::c_int = 512;
pub const O_CLOEXEC: ::c_int = 524288;

pub const POLLIN: ::c_short = 0x1;
pub const POLLPRI: ::c_short = 0x2;
pub const POLLOUT: ::c_short = 0x4;
pub const POLLERR: ::c_short = 0x8;
pub const POLLHUP: ::c_short = 0x10;
pub const POLLNVAL: ::c_short = 0x20;

pub const PTHREAD_COND_INITIALIZER: pthread_cond_t = usize::max_value();
pub const PTHREAD_MUTEX_INITIALIZER: pthread_mutex_t = usize::max_value();
pub const PTHREAD_RWLOCK_INITIALIZER: pthread_rwlock_t = usize::max_value();

pub const PTHREAD_MUTEX_NORMAL: ::c_int = 0;
pub const PTHREAD_MUTEX_RECURSIVE: ::c_int = 1;
pub const PTHREAD_STACK_MIN: ::size_t = 0;

// Dummy
pub const RTLD_DEFAULT: *mut ::c_void = 0i64 as *mut ::c_void;

pub const _SC_ARG_MAX: ::c_int = 0;
pub const _SC_CHILD_MAX: ::c_int = 1;
pub const _SC_CLK_TCK: ::c_int = 2;
pub const _SC_NGROUPS_MAX: ::c_int = 3;
pub const _SC_OPEN_MAX: ::c_int = 4;
pub const _SC_JOB_CONTROL: ::c_int = 5;
pub const _SC_SAVED_IDS: ::c_int = 6;
pub const _SC_VERSION: ::c_int = 7;
pub const _SC_PAGESIZE: ::c_int = 8;
pub const _SC_PAGE_SIZE: ::c_int = _SC_PAGESIZE;
pub const _SC_NPROCESSORS_CONF: ::c_int = 9;
pub const _SC_NPROCESSORS_ONLN: ::c_int = 10;
pub const _SC_PHYS_PAGES: ::c_int = 11;
pub const _SC_AVPHYS_PAGES: ::c_int = 12;
pub const _SC_MQ_OPEN_MAX: ::c_int = 13;
pub const _SC_MQ_PRIO_MAX: ::c_int = 14;
pub const _SC_RTSIG_MAX: ::c_int = 15;
pub const _SC_SEM_NSEMS_MAX: ::c_int = 16;
pub const _SC_SEM_VALUE_MAX: ::c_int = 17;
pub const _SC_SIGQUEUE_MAX: ::c_int = 18;
pub const _SC_TIMER_MAX: ::c_int = 19;
pub const _SC_TZNAME_MAX: ::c_int = 20;
pub const _SC_ASYNCHRONOUS_IO: ::c_int = 21;
pub const _SC_FSYNC: ::c_int = 22;
pub const _SC_MAPPED_FILES: ::c_int = 23;
pub const _SC_MEMLOCK: ::c_int = 24;
pub const _SC_MEMLOCK_RANGE: ::c_int = 25;
pub const _SC_MEMORY_PROTECTION: ::c_int = 26;
pub const _SC_MESSAGE_PASSING: ::c_int = 27;
pub const _SC_PRIORITIZED_IO: ::c_int = 28;
pub const _SC_REALTIME_SIGNALS: ::c_int = 29;
pub const _SC_SEMAPHORES: ::c_int = 30;
pub const _SC_SHARED_MEMORY_OBJECTS: ::c_int = 31;
pub const _SC_SYNCHRONIZED_IO: ::c_int = 32;
pub const _SC_TIMERS: ::c_int = 33;
pub const _SC_AIO_LISTIO_MAX: ::c_int = 34;
pub const _SC_AIO_MAX: ::c_int = 35;
pub const _SC_AIO_PRIO_DELTA_MAX: ::c_int = 36;
pub const _SC_DELAYTIMER_MAX: ::c_int = 37;
pub const _SC_THREAD_KEYS_MAX: ::c_int = 38;
pub const _SC_THREAD_STACK_MIN: ::c_int = 39;
pub const _SC_THREAD_THREADS_MAX: ::c_int = 40;
pub const _SC_TTY_NAME_MAX: ::c_int = 41;
pub const _SC_THREADS: ::c_int = 42;
pub const _SC_THREAD_ATTR_STACKADDR: ::c_int = 43;
pub const _SC_THREAD_ATTR_STACKSIZE: ::c_int = 44;
pub const _SC_THREAD_PRIORITY_SCHEDULING: ::c_int = 45;
pub const _SC_THREAD_PRIO_INHERIT: ::c_int = 46;
pub const _SC_THREAD_PRIO_PROTECT: ::c_int = 47;
pub const _SC_THREAD_PRIO_CEILING: ::c_int = _SC_THREAD_PRIO_PROTECT;
pub const _SC_THREAD_PROCESS_SHARED: ::c_int = 48;
pub const _SC_THREAD_SAFE_FUNCTIONS: ::c_int = 49;
pub const _SC_GETGR_R_SIZE_MAX: ::c_int = 50;
pub const _SC_GETPW_R_SIZE_MAX: ::c_int = 51;
pub const _SC_LOGIN_NAME_MAX: ::c_int = 52;
pub const _SC_THREAD_DESTRUCTOR_ITERATIONS: ::c_int = 53;
pub const _SC_ADVISORY_INFO: ::c_int = 54;
pub const _SC_ATEXIT_MAX: ::c_int = 55;
pub const _SC_BARRIERS: ::c_int = 56;
pub const _SC_BC_BASE_MAX: ::c_int = 57;
pub const _SC_BC_DIM_MAX: ::c_int = 58;
pub const _SC_BC_SCALE_MAX: ::c_int = 59;
pub const _SC_BC_STRING_MAX: ::c_int = 60;
pub const _SC_CLOCK_SELECTION: ::c_int = 61;
pub const _SC_COLL_WEIGHTS_MAX: ::c_int = 62;
pub const _SC_CPUTIME: ::c_int = 63;
pub const _SC_EXPR_NEST_MAX: ::c_int = 64;
pub const _SC_HOST_NAME_MAX: ::c_int = 65;
pub const _SC_IOV_MAX: ::c_int = 66;
pub const _SC_IPV6: ::c_int = 67;
pub const _SC_LINE_MAX: ::c_int = 68;
pub const _SC_MONOTONIC_CLOCK: ::c_int = 69;
pub const _SC_RAW_SOCKETS: ::c_int = 70;
pub const _SC_READER_WRITER_LOCKS: ::c_int = 71;
pub const _SC_REGEXP: ::c_int = 72;
pub const _SC_RE_DUP_MAX: ::c_int = 73;
pub const _SC_SHELL: ::c_int = 74;
pub const _SC_SPAWN: ::c_int = 75;
pub const _SC_SPIN_LOCKS: ::c_int = 76;
pub const _SC_SPORADIC_SERVER: ::c_int = 77;
pub const _SC_SS_REPL_MAX: ::c_int = 78;
pub const _SC_SYMLOOP_MAX: ::c_int = 79;
pub const _SC_THREAD_CPUTIME: ::c_int = 80;
pub const _SC_THREAD_SPORADIC_SERVER: ::c_int = 81;
pub const _SC_TIMEOUTS: ::c_int = 82;
pub const _SC_TRACE: ::c_int = 83;
pub const _SC_TRACE_EVENT_FILTER: ::c_int = 84;
pub const _SC_TRACE_EVENT_NAME_MAX: ::c_int = 85;
pub const _SC_TRACE_INHERIT: ::c_int = 86;
pub const _SC_TRACE_LOG: ::c_int = 87;
pub const _SC_TRACE_NAME_MAX: ::c_int = 88;
pub const _SC_TRACE_SYS_MAX: ::c_int = 89;
pub const _SC_TRACE_USER_EVENT_MAX: ::c_int = 90;
pub const _SC_TYPED_MEMORY_OBJECTS: ::c_int = 91;
pub const _SC_V7_ILP32_OFF32: ::c_int = 92;
pub const _SC_V6_ILP32_OFF32: ::c_int = _SC_V7_ILP32_OFF32;
pub const _SC_XBS5_ILP32_OFF32: ::c_int = _SC_V7_ILP32_OFF32;
pub const _SC_V7_ILP32_OFFBIG: ::c_int = 93;
pub const _SC_V6_ILP32_OFFBIG: ::c_int = _SC_V7_ILP32_OFFBIG;
pub const _SC_XBS5_ILP32_OFFBIG: ::c_int = _SC_V7_ILP32_OFFBIG;
pub const _SC_V7_LP64_OFF64: ::c_int = 94;
pub const _SC_V6_LP64_OFF64: ::c_int = _SC_V7_LP64_OFF64;
pub const _SC_XBS5_LP64_OFF64: ::c_int = _SC_V7_LP64_OFF64;
pub const _SC_V7_LPBIG_OFFBIG: ::c_int = 95;
pub const _SC_V6_LPBIG_OFFBIG: ::c_int = _SC_V7_LPBIG_OFFBIG;
pub const _SC_XBS5_LPBIG_OFFBIG: ::c_int = _SC_V7_LPBIG_OFFBIG;
pub const _SC_XOPEN_CRYPT: ::c_int = 96;
pub const _SC_XOPEN_ENH_I18N: ::c_int = 97;
pub const _SC_XOPEN_LEGACY: ::c_int = 98;
pub const _SC_XOPEN_REALTIME: ::c_int = 99;
pub const _SC_STREAM_MAX: ::c_int = 100;
pub const _SC_PRIORITY_SCHEDULING: ::c_int = 101;
pub const _SC_XOPEN_REALTIME_THREADS: ::c_int = 102;
pub const _SC_XOPEN_SHM: ::c_int = 103;
pub const _SC_XOPEN_STREAMS: ::c_int = 104;
pub const _SC_XOPEN_UNIX: ::c_int = 105;
pub const _SC_XOPEN_VERSION: ::c_int = 106;
pub const _SC_2_CHAR_TERM: ::c_int = 107;
pub const _SC_2_C_BIND: ::c_int = 108;
pub const _SC_2_C_DEV: ::c_int = 109;
pub const _SC_2_FORT_DEV: ::c_int = 110;
pub const _SC_2_FORT_RUN: ::c_int = 111;
pub const _SC_2_LOCALEDEF: ::c_int = 112;
pub const _SC_2_PBS: ::c_int = 113;
pub const _SC_2_PBS_ACCOUNTING: ::c_int = 114;
pub const _SC_2_PBS_CHECKPOINT: ::c_int = 115;
pub const _SC_2_PBS_LOCATE: ::c_int = 116;
pub const _SC_2_PBS_MESSAGE: ::c_int = 117;
pub const _SC_2_PBS_TRACK: ::c_int = 118;
pub const _SC_2_SW_DEV: ::c_int = 119;
pub const _SC_2_UPE: ::c_int = 120;
pub const _SC_2_VERSION: ::c_int = 121;
pub const _SC_THREAD_ROBUST_PRIO_INHERIT: ::c_int = 122;
pub const _SC_THREAD_ROBUST_PRIO_PROTECT: ::c_int = 123;
pub const _SC_XOPEN_UUCP: ::c_int = 124;
pub const _SC_LEVEL1_ICACHE_SIZE: ::c_int = 125;
pub const _SC_LEVEL1_ICACHE_ASSOC: ::c_int = 126;
pub const _SC_LEVEL1_ICACHE_LINESIZE: ::c_int = 127;
pub const _SC_LEVEL1_DCACHE_SIZE: ::c_int = 128;
pub const _SC_LEVEL1_DCACHE_ASSOC: ::c_int = 129;
pub const _SC_LEVEL1_DCACHE_LINESIZE: ::c_int = 130;
pub const _SC_LEVEL2_CACHE_SIZE: ::c_int = 131;
pub const _SC_LEVEL2_CACHE_ASSOC: ::c_int = 132;
pub const _SC_LEVEL2_CACHE_LINESIZE: ::c_int = 133;
pub const _SC_LEVEL3_CACHE_SIZE: ::c_int = 134;
pub const _SC_LEVEL3_CACHE_ASSOC: ::c_int = 135;
pub const _SC_LEVEL3_CACHE_LINESIZE: ::c_int = 136;
pub const _SC_LEVEL4_CACHE_SIZE: ::c_int = 137;
pub const _SC_LEVEL4_CACHE_ASSOC: ::c_int = 138;
pub const _SC_LEVEL4_CACHE_LINESIZE: ::c_int = 139;

pub const S_BLKSIZE: ::mode_t = 1024;
pub const S_IREAD: ::mode_t = 256;
pub const S_IWRITE: ::mode_t = 128;
pub const S_IEXEC: ::mode_t = 64;
pub const S_ENFMT: ::mode_t = 1024;
pub const S_IFMT: ::mode_t = 61440;
pub const S_IFDIR: ::mode_t = 16384;
pub const S_IFCHR: ::mode_t = 8192;
pub const S_IFBLK: ::mode_t = 24576;
pub const S_IFREG: ::mode_t = 32768;
pub const S_IFLNK: ::mode_t = 40960;
pub const S_IFSOCK: ::mode_t = 49152;
pub const S_IFIFO: ::mode_t = 4096;
pub const S_IRUSR: ::mode_t = 256;
pub const S_IWUSR: ::mode_t = 128;
pub const S_IXUSR: ::mode_t = 64;
pub const S_IRGRP: ::mode_t = 32;
pub const S_IWGRP: ::mode_t = 16;
pub const S_IXGRP: ::mode_t = 8;
pub const S_IROTH: ::mode_t = 4;
pub const S_IWOTH: ::mode_t = 2;
pub const S_IXOTH: ::mode_t = 1;

pub const SEEK_SET: ::c_int = 0;
pub const SEEK_CUR: ::c_int = 1;
pub const SEEK_END: ::c_int = 2;

pub const SHUT_RD: ::c_int = 0;
pub const SHUT_WR: ::c_int = 1;
pub const SHUT_RDWR: ::c_int = 2;

pub const SIG_SETMASK: ::c_int = 0;

pub const SIGHUP: ::c_int = 1;
pub const SIGINT: ::c_int = 2;
pub const SIGQUIT: ::c_int = 3;
pub const SIGILL: ::c_int = 4;
pub const SIGABRT: ::c_int = 6;
pub const SIGEMT: ::c_int = 7;
pub const SIGFPE: ::c_int = 8;
pub const SIGKILL: ::c_int = 9;
pub const SIGSEGV: ::c_int = 11;
pub const SIGPIPE: ::c_int = 13;
pub const SIGALRM: ::c_int = 14;
pub const SIGTERM: ::c_int = 15;

pub const SO_DEBUG: ::c_int = 0x0001;
pub const SO_ACCEPTCONN: ::c_int = 0x0002;
pub const SO_REUSEADDR: ::c_int = 0x0004;
pub const SO_KEEPALIVE: ::c_int = 0x0008;
pub const SO_DONTROUTE: ::c_int = 0x0010;
pub const SO_BROADCAST: ::c_int = 0x0020;
pub const SO_USELOOPBACK: ::c_int = 0x0040;
pub const SO_LINGER: ::c_int = 0x0080;
pub const SO_OOBINLINE: ::c_int = 0x0100;
pub const SO_REUSEPORT: ::c_int = 0x0200;
pub const SO_SNDBUF: ::c_int = 0x1001;
pub const SO_RCVBUF: ::c_int = 0x1002;
pub const SO_SNDLOWAT: ::c_int = 0x1003;
pub const SO_RCVLOWAT: ::c_int = 0x1004;
pub const SO_SNDTIMEO: ::c_int = 0x1005;
pub const SO_RCVTIMEO: ::c_int = 0x1006;
pub const SO_ERROR: ::c_int = 0x1007;
pub const SO_TYPE: ::c_int = 0x1008;
pub const SO_CONTIMEO: ::c_int = 0x1009;
pub const SO_NO_CHECK: ::c_int = 0x100a;

pub const SOCK_STREAM: ::c_int = 1;
pub const SOCK_DGRAM: ::c_int = 2;
pub const SOCK_RAW: ::c_int = 3;

pub const SOL_SOCKET: ::c_int = 0xfff;

pub const STDIN_FILENO: ::c_int = 0;
pub const STDOUT_FILENO: ::c_int = 1;
pub const STDERR_FILENO: ::c_int = 2;

pub const TCP_NODELAY: ::c_int = 0x01;
pub const TCP_KEEPALIVE: ::c_int = 0x02;
pub const TCP_KEEPIDLE: ::c_int = 0x03;
pub const TCP_KEEPINTVL: ::c_int = 0x04;
pub const TCP_KEEPCNT: ::c_int = 0x05;

const ULONG_SIZE: usize = 64;

pub const WNOHANG: ::c_int = 0x00000001;

f! {
    pub fn WEXITSTATUS(status: ::c_int) -> ::c_int {
        (status >> 8) & 0xff
    }

    pub fn WIFEXITED(status: ::c_int) -> bool {
        (status & 0xff) == 0
    }

    pub fn WTERMSIG(status: ::c_int) -> ::c_int {
        status & 0x7f
    }
}

extern "C" {
    pub fn getrlimit(resource: ::c_int, rlim: *mut ::rlimit) -> ::c_int;
    pub fn setrlimit(resource: ::c_int, rlim: *const ::rlimit) -> ::c_int;
    pub fn strerror_r(
        errnum: ::c_int,
        buf: *mut c_char,
        buflen: ::size_t,
    ) -> ::c_int;

    pub fn sem_destroy(sem: *mut sem_t) -> ::c_int;
    pub fn sem_init(
        sem: *mut sem_t,
        pshared: ::c_int,
        value: ::c_uint,
    ) -> ::c_int;

    pub fn abs(i: ::c_int) -> ::c_int;
    pub fn atof(s: *const ::c_char) -> ::c_double;
    pub fn labs(i: ::c_long) -> ::c_long;
    pub fn rand() -> ::c_int;
    pub fn srand(seed: ::c_uint);

    pub fn bind(
        s: ::c_int,
        name: *const ::sockaddr,
        namelen: ::socklen_t,
    ) -> ::c_int;

    pub fn clock_gettime(
        clock_id: ::clockid_t,
        tp: *mut ::timespec,
    ) -> ::c_int;

    pub fn gettimeofday(tp: *mut ::timeval, tz: *mut ::c_void) -> ::c_int;
    pub fn getpwuid_r(
        uid: ::uid_t,
        pwd: *mut passwd,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut passwd,
    ) -> ::c_int;

    // Dummy
    pub fn ioctl(fd: ::c_int, request: ::c_int, ...) -> ::c_int;

    pub fn memalign(align: ::size_t, nbytes: ::size_t) -> *mut ::c_void;

    pub fn pthread_create(
        tid: *mut ::pthread_t,
        attr: *const ::pthread_attr_t,
        start: extern "C" fn(*mut ::c_void) -> *mut ::c_void,
        arg: *mut ::c_void,
    ) -> ::c_int;

    pub fn pthread_sigmask(
        how: ::c_int,
        set: *const ::sigset_t,
        oset: *mut ::sigset_t,
    ) -> ::c_int;

    pub fn recvfrom(
        s: ::c_int,
        mem: *mut ::c_void,
        len: ::size_t,
        flags: ::c_int,
        from: *mut ::sockaddr,
        fromlen: *mut ::socklen_t,
    ) -> ::c_int;

    pub fn setgroups(ngroups: ::c_int, grouplist: *const ::gid_t) -> ::c_int;
    pub fn uname(buf: *mut ::utsname) -> ::c_int;
}

cfg_if! {
    if #[cfg(target_arch = "aarch64")] {
        mod aarch64;
        pub use self::aarch64::*;
    } else if #[cfg(target_arch = "x86_64")] {
        mod x86_64;
        pub use self::x86_64::*;
    } else {
        // Unknown target_arch
    }
}
