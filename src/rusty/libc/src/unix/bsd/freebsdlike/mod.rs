pub type mode_t = u16;
pub type pthread_attr_t = *mut ::c_void;
pub type rlim_t = i64;
pub type pthread_mutex_t = *mut ::c_void;
pub type pthread_mutexattr_t = *mut ::c_void;
pub type pthread_cond_t = *mut ::c_void;
pub type pthread_condattr_t = *mut ::c_void;
pub type pthread_rwlock_t = *mut ::c_void;
pub type pthread_rwlockattr_t = *mut ::c_void;
pub type pthread_key_t = ::c_int;
pub type tcflag_t = ::c_uint;
pub type speed_t = ::c_uint;
pub type nl_item = ::c_int;
pub type id_t = i64;
pub type vm_size_t = ::uintptr_t;

#[cfg_attr(feature = "extra_traits", derive(Debug))]
pub enum timezone {}
impl ::Copy for timezone {}
impl ::Clone for timezone {
    fn clone(&self) -> timezone {
        *self
    }
}

impl siginfo_t {
    pub unsafe fn si_addr(&self) -> *mut ::c_void {
        self.si_addr
    }

    pub unsafe fn si_value(&self) -> ::sigval {
        self.si_value
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

    pub struct glob_t {
        pub gl_pathc:  ::size_t,
        pub gl_matchc: ::size_t,
        pub gl_offs:   ::size_t,
        pub gl_flags:  ::c_int,
        pub gl_pathv:  *mut *mut ::c_char,
        __unused3: *mut ::c_void,
        __unused4: *mut ::c_void,
        __unused5: *mut ::c_void,
        __unused6: *mut ::c_void,
        __unused7: *mut ::c_void,
        __unused8: *mut ::c_void,
    }

    pub struct addrinfo {
        pub ai_flags: ::c_int,
        pub ai_family: ::c_int,
        pub ai_socktype: ::c_int,
        pub ai_protocol: ::c_int,
        pub ai_addrlen: ::socklen_t,
        pub ai_canonname: *mut ::c_char,
        pub ai_addr: *mut ::sockaddr,
        pub ai_next: *mut addrinfo,
    }

    pub struct sigset_t {
        bits: [u32; 4],
    }

    pub struct siginfo_t {
        pub si_signo: ::c_int,
        pub si_errno: ::c_int,
        pub si_code: ::c_int,
        pub si_pid: ::pid_t,
        pub si_uid: ::uid_t,
        pub si_status: ::c_int,
        pub si_addr: *mut ::c_void,
        pub si_value: ::sigval,
        _pad1: ::c_long,
        _pad2: [::c_int; 7],
    }

    pub struct sigaction {
        pub sa_sigaction: ::sighandler_t,
        pub sa_flags: ::c_int,
        pub sa_mask: sigset_t,
    }

    pub struct sched_param {
        pub sched_priority: ::c_int,
    }

    pub struct Dl_info {
        pub dli_fname: *const ::c_char,
        pub dli_fbase: *mut ::c_void,
        pub dli_sname: *const ::c_char,
        pub dli_saddr: *mut ::c_void,
    }

    pub struct sockaddr_in {
        pub sin_len: u8,
        pub sin_family: ::sa_family_t,
        pub sin_port: ::in_port_t,
        pub sin_addr: ::in_addr,
        pub sin_zero: [::c_char; 8],
    }

    pub struct termios {
        pub c_iflag: ::tcflag_t,
        pub c_oflag: ::tcflag_t,
        pub c_cflag: ::tcflag_t,
        pub c_lflag: ::tcflag_t,
        pub c_cc: [::cc_t; ::NCCS],
        pub c_ispeed: ::speed_t,
        pub c_ospeed: ::speed_t,
    }

    pub struct flock {
        pub l_start: ::off_t,
        pub l_len: ::off_t,
        pub l_pid: ::pid_t,
        pub l_type: ::c_short,
        pub l_whence: ::c_short,
        #[cfg(not(target_os = "dragonfly"))]
        pub l_sysid: ::c_int,
    }

    pub struct sf_hdtr {
        pub headers: *mut ::iovec,
        pub hdr_cnt: ::c_int,
        pub trailers: *mut ::iovec,
        pub trl_cnt: ::c_int,
    }

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
        pub int_n_cs_precedes: ::c_char,
        pub int_p_sep_by_space: ::c_char,
        pub int_n_sep_by_space: ::c_char,
        pub int_p_sign_posn: ::c_char,
        pub int_n_sign_posn: ::c_char,
    }

    pub struct cmsgcred {
        pub cmcred_pid: ::pid_t,
        pub cmcred_uid: ::uid_t,
        pub cmcred_euid: ::uid_t,
        pub cmcred_gid: ::gid_t,
        pub cmcred_ngroups: ::c_short,
        pub cmcred_groups: [::gid_t; CMGROUP_MAX],
    }

    pub struct rtprio {
        pub type_: ::c_ushort,
        pub prio: ::c_ushort,
    }

    pub struct in6_pktinfo {
        pub ipi6_addr: ::in6_addr,
        pub ipi6_ifindex: ::c_uint,
    }

    pub struct arphdr {
        pub ar_hrd: u16,
        pub ar_pro: u16,
        pub ar_hln: u8,
        pub ar_pln: u8,
        pub ar_op: u16,
    }
}

s_no_extra_traits! {
    pub struct sockaddr_storage {
        pub ss_len: u8,
        pub ss_family: ::sa_family_t,
        __ss_pad1: [u8; 6],
        __ss_align: i64,
        __ss_pad2: [u8; 112],
    }
}

cfg_if! {
    if #[cfg(feature = "extra_traits")] {
        impl PartialEq for sockaddr_storage {
            fn eq(&self, other: &sockaddr_storage) -> bool {
                self.ss_len == other.ss_len
                    && self.ss_family == other.ss_family
                    && self.__ss_pad1 == other.__ss_pad1
                    && self.__ss_align == other.__ss_align
                    && self
                    .__ss_pad2
                    .iter()
                    .zip(other.__ss_pad2.iter())
                    .all(|(a, b)| a == b)
            }
        }
        impl Eq for sockaddr_storage {}
        impl ::fmt::Debug for sockaddr_storage {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("sockaddr_storage")
                    .field("ss_len", &self.ss_len)
                    .field("ss_family", &self.ss_family)
                    .field("__ss_pad1", &self.__ss_pad1)
                    .field("__ss_align", &self.__ss_align)
                    // FIXME: .field("__ss_pad2", &self.__ss_pad2)
                    .finish()
            }
        }
        impl ::hash::Hash for sockaddr_storage {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.ss_len.hash(state);
                self.ss_family.hash(state);
                self.__ss_pad1.hash(state);
                self.__ss_align.hash(state);
                self.__ss_pad2.hash(state);
            }
        }
    }
}

#[deprecated(
    since = "0.2.64",
    note = "Can vary at runtime.  Use sysconf(3) instead"
)]
pub const AIO_LISTIO_MAX: ::c_int = 16;
pub const AIO_CANCELED: ::c_int = 1;
pub const AIO_NOTCANCELED: ::c_int = 2;
pub const AIO_ALLDONE: ::c_int = 3;
pub const LIO_NOP: ::c_int = 0;
pub const LIO_WRITE: ::c_int = 1;
pub const LIO_READ: ::c_int = 2;
pub const LIO_WAIT: ::c_int = 1;
pub const LIO_NOWAIT: ::c_int = 0;

pub const SIGEV_NONE: ::c_int = 0;
pub const SIGEV_SIGNAL: ::c_int = 1;
pub const SIGEV_THREAD: ::c_int = 2;
pub const SIGEV_KEVENT: ::c_int = 3;

pub const CODESET: ::nl_item = 0;
pub const D_T_FMT: ::nl_item = 1;
pub const D_FMT: ::nl_item = 2;
pub const T_FMT: ::nl_item = 3;
pub const T_FMT_AMPM: ::nl_item = 4;
pub const AM_STR: ::nl_item = 5;
pub const PM_STR: ::nl_item = 6;

pub const DAY_1: ::nl_item = 7;
pub const DAY_2: ::nl_item = 8;
pub const DAY_3: ::nl_item = 9;
pub const DAY_4: ::nl_item = 10;
pub const DAY_5: ::nl_item = 11;
pub const DAY_6: ::nl_item = 12;
pub const DAY_7: ::nl_item = 13;

pub const ABDAY_1: ::nl_item = 14;
pub const ABDAY_2: ::nl_item = 15;
pub const ABDAY_3: ::nl_item = 16;
pub const ABDAY_4: ::nl_item = 17;
pub const ABDAY_5: ::nl_item = 18;
pub const ABDAY_6: ::nl_item = 19;
pub const ABDAY_7: ::nl_item = 20;

pub const MON_1: ::nl_item = 21;
pub const MON_2: ::nl_item = 22;
pub const MON_3: ::nl_item = 23;
pub const MON_4: ::nl_item = 24;
pub const MON_5: ::nl_item = 25;
pub const MON_6: ::nl_item = 26;
pub const MON_7: ::nl_item = 27;
pub const MON_8: ::nl_item = 28;
pub const MON_9: ::nl_item = 29;
pub const MON_10: ::nl_item = 30;
pub const MON_11: ::nl_item = 31;
pub const MON_12: ::nl_item = 32;

pub const ABMON_1: ::nl_item = 33;
pub const ABMON_2: ::nl_item = 34;
pub const ABMON_3: ::nl_item = 35;
pub const ABMON_4: ::nl_item = 36;
pub const ABMON_5: ::nl_item = 37;
pub const ABMON_6: ::nl_item = 38;
pub const ABMON_7: ::nl_item = 39;
pub const ABMON_8: ::nl_item = 40;
pub const ABMON_9: ::nl_item = 41;
pub const ABMON_10: ::nl_item = 42;
pub const ABMON_11: ::nl_item = 43;
pub const ABMON_12: ::nl_item = 44;

pub const ERA: ::nl_item = 45;
pub const ERA_D_FMT: ::nl_item = 46;
pub const ERA_D_T_FMT: ::nl_item = 47;
pub const ERA_T_FMT: ::nl_item = 48;
pub const ALT_DIGITS: ::nl_item = 49;

pub const RADIXCHAR: ::nl_item = 50;
pub const THOUSEP: ::nl_item = 51;

pub const YESEXPR: ::nl_item = 52;
pub const NOEXPR: ::nl_item = 53;

pub const YESSTR: ::nl_item = 54;
pub const NOSTR: ::nl_item = 55;

pub const CRNCYSTR: ::nl_item = 56;

pub const D_MD_ORDER: ::nl_item = 57;

pub const ALTMON_1: ::nl_item = 58;
pub const ALTMON_2: ::nl_item = 59;
pub const ALTMON_3: ::nl_item = 60;
pub const ALTMON_4: ::nl_item = 61;
pub const ALTMON_5: ::nl_item = 62;
pub const ALTMON_6: ::nl_item = 63;
pub const ALTMON_7: ::nl_item = 64;
pub const ALTMON_8: ::nl_item = 65;
pub const ALTMON_9: ::nl_item = 66;
pub const ALTMON_10: ::nl_item = 67;
pub const ALTMON_11: ::nl_item = 68;
pub const ALTMON_12: ::nl_item = 69;

pub const EXIT_FAILURE: ::c_int = 1;
pub const EXIT_SUCCESS: ::c_int = 0;
pub const EOF: ::c_int = -1;
pub const SEEK_SET: ::c_int = 0;
pub const SEEK_CUR: ::c_int = 1;
pub const SEEK_END: ::c_int = 2;
pub const SEEK_DATA: ::c_int = 3;
pub const SEEK_HOLE: ::c_int = 4;
pub const _IOFBF: ::c_int = 0;
pub const _IONBF: ::c_int = 2;
pub const _IOLBF: ::c_int = 1;
pub const BUFSIZ: ::c_uint = 1024;
pub const FOPEN_MAX: ::c_uint = 20;
pub const FILENAME_MAX: ::c_uint = 1024;
pub const L_tmpnam: ::c_uint = 1024;
pub const TMP_MAX: ::c_uint = 308915776;

pub const O_NOCTTY: ::c_int = 32768;
pub const O_DIRECT: ::c_int = 0x00010000;

pub const S_IFIFO: mode_t = 4096;
pub const S_IFCHR: mode_t = 8192;
pub const S_IFBLK: mode_t = 24576;
pub const S_IFDIR: mode_t = 16384;
pub const S_IFREG: mode_t = 32768;
pub const S_IFLNK: mode_t = 40960;
pub const S_IFSOCK: mode_t = 49152;
pub const S_IFMT: mode_t = 61440;
pub const S_IEXEC: mode_t = 64;
pub const S_IWRITE: mode_t = 128;
pub const S_IREAD: mode_t = 256;
pub const S_IRWXU: mode_t = 448;
pub const S_IXUSR: mode_t = 64;
pub const S_IWUSR: mode_t = 128;
pub const S_IRUSR: mode_t = 256;
pub const S_IRWXG: mode_t = 56;
pub const S_IXGRP: mode_t = 8;
pub const S_IWGRP: mode_t = 16;
pub const S_IRGRP: mode_t = 32;
pub const S_IRWXO: mode_t = 7;
pub const S_IXOTH: mode_t = 1;
pub const S_IWOTH: mode_t = 2;
pub const S_IROTH: mode_t = 4;
pub const F_OK: ::c_int = 0;
pub const R_OK: ::c_int = 4;
pub const W_OK: ::c_int = 2;
pub const X_OK: ::c_int = 1;
pub const STDIN_FILENO: ::c_int = 0;
pub const STDOUT_FILENO: ::c_int = 1;
pub const STDERR_FILENO: ::c_int = 2;
pub const F_LOCK: ::c_int = 1;
pub const F_TEST: ::c_int = 3;
pub const F_TLOCK: ::c_int = 2;
pub const F_ULOCK: ::c_int = 0;
pub const F_DUPFD_CLOEXEC: ::c_int = 17;
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

pub const PROT_NONE: ::c_int = 0;
pub const PROT_READ: ::c_int = 1;
pub const PROT_WRITE: ::c_int = 2;
pub const PROT_EXEC: ::c_int = 4;

pub const MAP_FILE: ::c_int = 0x0000;
pub const MAP_SHARED: ::c_int = 0x0001;
pub const MAP_PRIVATE: ::c_int = 0x0002;
pub const MAP_FIXED: ::c_int = 0x0010;
pub const MAP_ANON: ::c_int = 0x1000;
pub const MAP_ANONYMOUS: ::c_int = MAP_ANON;

pub const MAP_FAILED: *mut ::c_void = !0 as *mut ::c_void;

pub const MCL_CURRENT: ::c_int = 0x0001;
pub const MCL_FUTURE: ::c_int = 0x0002;

pub const MS_SYNC: ::c_int = 0x0000;
pub const MS_ASYNC: ::c_int = 0x0001;
pub const MS_INVALIDATE: ::c_int = 0x0002;

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
pub const EDEADLK: ::c_int = 11;
pub const ENOMEM: ::c_int = 12;
pub const EACCES: ::c_int = 13;
pub const EFAULT: ::c_int = 14;
pub const ENOTBLK: ::c_int = 15;
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
pub const EAGAIN: ::c_int = 35;
pub const EWOULDBLOCK: ::c_int = 35;
pub const EINPROGRESS: ::c_int = 36;
pub const EALREADY: ::c_int = 37;
pub const ENOTSOCK: ::c_int = 38;
pub const EDESTADDRREQ: ::c_int = 39;
pub const EMSGSIZE: ::c_int = 40;
pub const EPROTOTYPE: ::c_int = 41;
pub const ENOPROTOOPT: ::c_int = 42;
pub const EPROTONOSUPPORT: ::c_int = 43;
pub const ESOCKTNOSUPPORT: ::c_int = 44;
pub const EOPNOTSUPP: ::c_int = 45;
pub const ENOTSUP: ::c_int = EOPNOTSUPP;
pub const EPFNOSUPPORT: ::c_int = 46;
pub const EAFNOSUPPORT: ::c_int = 47;
pub const EADDRINUSE: ::c_int = 48;
pub const EADDRNOTAVAIL: ::c_int = 49;
pub const ENETDOWN: ::c_int = 50;
pub const ENETUNREACH: ::c_int = 51;
pub const ENETRESET: ::c_int = 52;
pub const ECONNABORTED: ::c_int = 53;
pub const ECONNRESET: ::c_int = 54;
pub const ENOBUFS: ::c_int = 55;
pub const EISCONN: ::c_int = 56;
pub const ENOTCONN: ::c_int = 57;
pub const ESHUTDOWN: ::c_int = 58;
pub const ETOOMANYREFS: ::c_int = 59;
pub const ETIMEDOUT: ::c_int = 60;
pub const ECONNREFUSED: ::c_int = 61;
pub const ELOOP: ::c_int = 62;
pub const ENAMETOOLONG: ::c_int = 63;
pub const EHOSTDOWN: ::c_int = 64;
pub const EHOSTUNREACH: ::c_int = 65;
pub const ENOTEMPTY: ::c_int = 66;
pub const EPROCLIM: ::c_int = 67;
pub const EUSERS: ::c_int = 68;
pub const EDQUOT: ::c_int = 69;
pub const ESTALE: ::c_int = 70;
pub const EREMOTE: ::c_int = 71;
pub const EBADRPC: ::c_int = 72;
pub const ERPCMISMATCH: ::c_int = 73;
pub const EPROGUNAVAIL: ::c_int = 74;
pub const EPROGMISMATCH: ::c_int = 75;
pub const EPROCUNAVAIL: ::c_int = 76;
pub const ENOLCK: ::c_int = 77;
pub const ENOSYS: ::c_int = 78;
pub const EFTYPE: ::c_int = 79;
pub const EAUTH: ::c_int = 80;
pub const ENEEDAUTH: ::c_int = 81;
pub const EIDRM: ::c_int = 82;
pub const ENOMSG: ::c_int = 83;
pub const EOVERFLOW: ::c_int = 84;
pub const ECANCELED: ::c_int = 85;
pub const EILSEQ: ::c_int = 86;
pub const ENOATTR: ::c_int = 87;
pub const EDOOFUS: ::c_int = 88;
pub const EBADMSG: ::c_int = 89;
pub const EMULTIHOP: ::c_int = 90;
pub const ENOLINK: ::c_int = 91;
pub const EPROTO: ::c_int = 92;

pub const POLLSTANDARD: ::c_short = ::POLLIN
    | ::POLLPRI
    | ::POLLOUT
    | ::POLLRDNORM
    | ::POLLRDBAND
    | ::POLLWRBAND
    | ::POLLERR
    | ::POLLHUP
    | ::POLLNVAL;

pub const EAI_AGAIN: ::c_int = 2;
pub const EAI_BADFLAGS: ::c_int = 3;
pub const EAI_FAIL: ::c_int = 4;
pub const EAI_FAMILY: ::c_int = 5;
pub const EAI_MEMORY: ::c_int = 6;
pub const EAI_NONAME: ::c_int = 8;
pub const EAI_SERVICE: ::c_int = 9;
pub const EAI_SOCKTYPE: ::c_int = 10;
pub const EAI_SYSTEM: ::c_int = 11;
pub const EAI_OVERFLOW: ::c_int = 14;

pub const F_DUPFD: ::c_int = 0;
pub const F_GETFD: ::c_int = 1;
pub const F_SETFD: ::c_int = 2;
pub const F_GETFL: ::c_int = 3;
pub const F_SETFL: ::c_int = 4;

pub const SIGTRAP: ::c_int = 5;

pub const GLOB_APPEND: ::c_int = 0x0001;
pub const GLOB_DOOFFS: ::c_int = 0x0002;
pub const GLOB_ERR: ::c_int = 0x0004;
pub const GLOB_MARK: ::c_int = 0x0008;
pub const GLOB_NOCHECK: ::c_int = 0x0010;
pub const GLOB_NOSORT: ::c_int = 0x0020;
pub const GLOB_NOESCAPE: ::c_int = 0x2000;

pub const GLOB_NOSPACE: ::c_int = -1;
pub const GLOB_ABORTED: ::c_int = -2;
pub const GLOB_NOMATCH: ::c_int = -3;

pub const POSIX_MADV_NORMAL: ::c_int = 0;
pub const POSIX_MADV_RANDOM: ::c_int = 1;
pub const POSIX_MADV_SEQUENTIAL: ::c_int = 2;
pub const POSIX_MADV_WILLNEED: ::c_int = 3;
pub const POSIX_MADV_DONTNEED: ::c_int = 4;

pub const PTHREAD_PROCESS_PRIVATE: ::c_int = 0;
pub const PTHREAD_PROCESS_SHARED: ::c_int = 1;
pub const PTHREAD_CREATE_JOINABLE: ::c_int = 0;
pub const PTHREAD_CREATE_DETACHED: ::c_int = 1;

pub const RLIMIT_CPU: ::c_int = 0;
pub const RLIMIT_FSIZE: ::c_int = 1;
pub const RLIMIT_DATA: ::c_int = 2;
pub const RLIMIT_STACK: ::c_int = 3;
pub const RLIMIT_CORE: ::c_int = 4;
pub const RLIMIT_RSS: ::c_int = 5;
pub const RLIMIT_MEMLOCK: ::c_int = 6;
pub const RLIMIT_NPROC: ::c_int = 7;
pub const RLIMIT_NOFILE: ::c_int = 8;
pub const RLIMIT_SBSIZE: ::c_int = 9;
pub const RLIMIT_VMEM: ::c_int = 10;
pub const RLIMIT_AS: ::c_int = RLIMIT_VMEM;
pub const RLIM_INFINITY: rlim_t = 0x7fff_ffff_ffff_ffff;

pub const RUSAGE_SELF: ::c_int = 0;
pub const RUSAGE_CHILDREN: ::c_int = -1;

pub const MADV_NORMAL: ::c_int = 0;
pub const MADV_RANDOM: ::c_int = 1;
pub const MADV_SEQUENTIAL: ::c_int = 2;
pub const MADV_WILLNEED: ::c_int = 3;
pub const MADV_DONTNEED: ::c_int = 4;
pub const MADV_FREE: ::c_int = 5;
pub const MADV_NOSYNC: ::c_int = 6;
pub const MADV_AUTOSYNC: ::c_int = 7;
pub const MADV_NOCORE: ::c_int = 8;
pub const MADV_CORE: ::c_int = 9;

pub const MINCORE_INCORE: ::c_int = 0x1;
pub const MINCORE_REFERENCED: ::c_int = 0x2;
pub const MINCORE_MODIFIED: ::c_int = 0x4;
pub const MINCORE_REFERENCED_OTHER: ::c_int = 0x8;
pub const MINCORE_MODIFIED_OTHER: ::c_int = 0x10;
pub const MINCORE_SUPER: ::c_int = 0x20;

pub const AF_UNSPEC: ::c_int = 0;
pub const AF_LOCAL: ::c_int = 1;
pub const AF_UNIX: ::c_int = AF_LOCAL;
pub const AF_INET: ::c_int = 2;
pub const AF_IMPLINK: ::c_int = 3;
pub const AF_PUP: ::c_int = 4;
pub const AF_CHAOS: ::c_int = 5;
pub const AF_NETBIOS: ::c_int = 6;
pub const AF_ISO: ::c_int = 7;
pub const AF_OSI: ::c_int = AF_ISO;
pub const AF_ECMA: ::c_int = 8;
pub const AF_DATAKIT: ::c_int = 9;
pub const AF_CCITT: ::c_int = 10;
pub const AF_SNA: ::c_int = 11;
pub const AF_DECnet: ::c_int = 12;
pub const AF_DLI: ::c_int = 13;
pub const AF_LAT: ::c_int = 14;
pub const AF_HYLINK: ::c_int = 15;
pub const AF_APPLETALK: ::c_int = 16;
pub const AF_ROUTE: ::c_int = 17;
pub const AF_LINK: ::c_int = 18;
pub const pseudo_AF_XTP: ::c_int = 19;
pub const AF_COIP: ::c_int = 20;
pub const AF_CNT: ::c_int = 21;
pub const pseudo_AF_RTIP: ::c_int = 22;
pub const AF_IPX: ::c_int = 23;
pub const AF_SIP: ::c_int = 24;
pub const pseudo_AF_PIP: ::c_int = 25;
pub const AF_ISDN: ::c_int = 26;
pub const AF_E164: ::c_int = AF_ISDN;
pub const pseudo_AF_KEY: ::c_int = 27;
pub const AF_INET6: ::c_int = 28;
pub const AF_NATM: ::c_int = 29;
pub const AF_ATM: ::c_int = 30;
pub const pseudo_AF_HDRCMPLT: ::c_int = 31;
pub const AF_NETGRAPH: ::c_int = 32;

pub const PF_UNSPEC: ::c_int = AF_UNSPEC;
pub const PF_LOCAL: ::c_int = AF_LOCAL;
pub const PF_UNIX: ::c_int = PF_LOCAL;
pub const PF_INET: ::c_int = AF_INET;
pub const PF_IMPLINK: ::c_int = AF_IMPLINK;
pub const PF_PUP: ::c_int = AF_PUP;
pub const PF_CHAOS: ::c_int = AF_CHAOS;
pub const PF_NETBIOS: ::c_int = AF_NETBIOS;
pub const PF_ISO: ::c_int = AF_ISO;
pub const PF_OSI: ::c_int = AF_ISO;
pub const PF_ECMA: ::c_int = AF_ECMA;
pub const PF_DATAKIT: ::c_int = AF_DATAKIT;
pub const PF_CCITT: ::c_int = AF_CCITT;
pub const PF_SNA: ::c_int = AF_SNA;
pub const PF_DECnet: ::c_int = AF_DECnet;
pub const PF_DLI: ::c_int = AF_DLI;
pub const PF_LAT: ::c_int = AF_LAT;
pub const PF_HYLINK: ::c_int = AF_HYLINK;
pub const PF_APPLETALK: ::c_int = AF_APPLETALK;
pub const PF_ROUTE: ::c_int = AF_ROUTE;
pub const PF_LINK: ::c_int = AF_LINK;
pub const PF_XTP: ::c_int = pseudo_AF_XTP;
pub const PF_COIP: ::c_int = AF_COIP;
pub const PF_CNT: ::c_int = AF_CNT;
pub const PF_SIP: ::c_int = AF_SIP;
pub const PF_IPX: ::c_int = AF_IPX;
pub const PF_RTIP: ::c_int = pseudo_AF_RTIP;
pub const PF_PIP: ::c_int = pseudo_AF_PIP;
pub const PF_ISDN: ::c_int = AF_ISDN;
pub const PF_KEY: ::c_int = pseudo_AF_KEY;
pub const PF_INET6: ::c_int = AF_INET6;
pub const PF_NATM: ::c_int = AF_NATM;
pub const PF_ATM: ::c_int = AF_ATM;
pub const PF_NETGRAPH: ::c_int = AF_NETGRAPH;

pub const PT_TRACE_ME: ::c_int = 0;
pub const PT_READ_I: ::c_int = 1;
pub const PT_READ_D: ::c_int = 2;
pub const PT_WRITE_I: ::c_int = 4;
pub const PT_WRITE_D: ::c_int = 5;
pub const PT_CONTINUE: ::c_int = 7;
pub const PT_KILL: ::c_int = 8;
pub const PT_STEP: ::c_int = 9;
pub const PT_ATTACH: ::c_int = 10;
pub const PT_DETACH: ::c_int = 11;
pub const PT_IO: ::c_int = 12;

pub const SOMAXCONN: ::c_int = 128;

pub const MSG_OOB: ::c_int = 0x00000001;
pub const MSG_PEEK: ::c_int = 0x00000002;
pub const MSG_DONTROUTE: ::c_int = 0x00000004;
pub const MSG_EOR: ::c_int = 0x00000008;
pub const MSG_TRUNC: ::c_int = 0x00000010;
pub const MSG_CTRUNC: ::c_int = 0x00000020;
pub const MSG_WAITALL: ::c_int = 0x00000040;
pub const MSG_DONTWAIT: ::c_int = 0x00000080;
pub const MSG_EOF: ::c_int = 0x00000100;

pub const SCM_TIMESTAMP: ::c_int = 0x02;

pub const SOCK_STREAM: ::c_int = 1;
pub const SOCK_DGRAM: ::c_int = 2;
pub const SOCK_RAW: ::c_int = 3;
pub const SOCK_RDM: ::c_int = 4;
pub const SOCK_SEQPACKET: ::c_int = 5;
pub const SOCK_CLOEXEC: ::c_int = 0x10000000;
pub const SOCK_NONBLOCK: ::c_int = 0x20000000;
pub const SOCK_MAXADDRLEN: ::c_int = 255;
pub const IP_TTL: ::c_int = 4;
pub const IP_HDRINCL: ::c_int = 2;
pub const IP_RECVDSTADDR: ::c_int = 7;
pub const IP_SENDSRCADDR: ::c_int = IP_RECVDSTADDR;
pub const IP_ADD_MEMBERSHIP: ::c_int = 12;
pub const IP_DROP_MEMBERSHIP: ::c_int = 13;
pub const IP_RECVIF: ::c_int = 20;
pub const IPV6_JOIN_GROUP: ::c_int = 12;
pub const IPV6_LEAVE_GROUP: ::c_int = 13;
pub const IPV6_RECVPKTINFO: ::c_int = 36;
pub const IPV6_PKTINFO: ::c_int = 46;
pub const IPV6_RECVTCLASS: ::c_int = 57;
pub const IPV6_TCLASS: ::c_int = 61;

pub const TCP_NOPUSH: ::c_int = 4;
pub const TCP_NOOPT: ::c_int = 8;
pub const TCP_KEEPIDLE: ::c_int = 256;
pub const TCP_KEEPINTVL: ::c_int = 512;
pub const TCP_KEEPCNT: ::c_int = 1024;

pub const SOL_SOCKET: ::c_int = 0xffff;
pub const SO_DEBUG: ::c_int = 0x01;
pub const SO_ACCEPTCONN: ::c_int = 0x0002;
pub const SO_REUSEADDR: ::c_int = 0x0004;
pub const SO_KEEPALIVE: ::c_int = 0x0008;
pub const SO_DONTROUTE: ::c_int = 0x0010;
pub const SO_BROADCAST: ::c_int = 0x0020;
pub const SO_USELOOPBACK: ::c_int = 0x0040;
pub const SO_LINGER: ::c_int = 0x0080;
pub const SO_OOBINLINE: ::c_int = 0x0100;
pub const SO_REUSEPORT: ::c_int = 0x0200;
pub const SO_TIMESTAMP: ::c_int = 0x0400;
pub const SO_NOSIGPIPE: ::c_int = 0x0800;
pub const SO_ACCEPTFILTER: ::c_int = 0x1000;
pub const SO_SNDBUF: ::c_int = 0x1001;
pub const SO_RCVBUF: ::c_int = 0x1002;
pub const SO_SNDLOWAT: ::c_int = 0x1003;
pub const SO_RCVLOWAT: ::c_int = 0x1004;
pub const SO_SNDTIMEO: ::c_int = 0x1005;
pub const SO_RCVTIMEO: ::c_int = 0x1006;
pub const SO_ERROR: ::c_int = 0x1007;
pub const SO_TYPE: ::c_int = 0x1008;

pub const SHUT_RD: ::c_int = 0;
pub const SHUT_WR: ::c_int = 1;
pub const SHUT_RDWR: ::c_int = 2;

pub const LOCK_SH: ::c_int = 1;
pub const LOCK_EX: ::c_int = 2;
pub const LOCK_NB: ::c_int = 4;
pub const LOCK_UN: ::c_int = 8;

pub const MAP_COPY: ::c_int = 0x0002;
#[doc(hidden)]
#[deprecated(since = "0.2.54", note = "Removed in FreeBSD 11")]
pub const MAP_RENAME: ::c_int = 0x0020;
#[doc(hidden)]
#[deprecated(since = "0.2.54", note = "Removed in FreeBSD 11")]
pub const MAP_NORESERVE: ::c_int = 0x0040;
pub const MAP_HASSEMAPHORE: ::c_int = 0x0200;
pub const MAP_STACK: ::c_int = 0x0400;
pub const MAP_NOSYNC: ::c_int = 0x0800;
pub const MAP_NOCORE: ::c_int = 0x020000;

pub const IPPROTO_RAW: ::c_int = 255;

pub const _PC_LINK_MAX: ::c_int = 1;
pub const _PC_MAX_CANON: ::c_int = 2;
pub const _PC_MAX_INPUT: ::c_int = 3;
pub const _PC_NAME_MAX: ::c_int = 4;
pub const _PC_PATH_MAX: ::c_int = 5;
pub const _PC_PIPE_BUF: ::c_int = 6;
pub const _PC_CHOWN_RESTRICTED: ::c_int = 7;
pub const _PC_NO_TRUNC: ::c_int = 8;
pub const _PC_VDISABLE: ::c_int = 9;
pub const _PC_ALLOC_SIZE_MIN: ::c_int = 10;
pub const _PC_FILESIZEBITS: ::c_int = 12;
pub const _PC_REC_INCR_XFER_SIZE: ::c_int = 14;
pub const _PC_REC_MAX_XFER_SIZE: ::c_int = 15;
pub const _PC_REC_MIN_XFER_SIZE: ::c_int = 16;
pub const _PC_REC_XFER_ALIGN: ::c_int = 17;
pub const _PC_SYMLINK_MAX: ::c_int = 18;
pub const _PC_MIN_HOLE_SIZE: ::c_int = 21;
pub const _PC_ASYNC_IO: ::c_int = 53;
pub const _PC_PRIO_IO: ::c_int = 54;
pub const _PC_SYNC_IO: ::c_int = 55;
pub const _PC_ACL_EXTENDED: ::c_int = 59;
pub const _PC_ACL_PATH_MAX: ::c_int = 60;
pub const _PC_CAP_PRESENT: ::c_int = 61;
pub const _PC_INF_PRESENT: ::c_int = 62;
pub const _PC_MAC_PRESENT: ::c_int = 63;

pub const _SC_ARG_MAX: ::c_int = 1;
pub const _SC_CHILD_MAX: ::c_int = 2;
pub const _SC_CLK_TCK: ::c_int = 3;
pub const _SC_NGROUPS_MAX: ::c_int = 4;
pub const _SC_OPEN_MAX: ::c_int = 5;
pub const _SC_JOB_CONTROL: ::c_int = 6;
pub const _SC_SAVED_IDS: ::c_int = 7;
pub const _SC_VERSION: ::c_int = 8;
pub const _SC_BC_BASE_MAX: ::c_int = 9;
pub const _SC_BC_DIM_MAX: ::c_int = 10;
pub const _SC_BC_SCALE_MAX: ::c_int = 11;
pub const _SC_BC_STRING_MAX: ::c_int = 12;
pub const _SC_COLL_WEIGHTS_MAX: ::c_int = 13;
pub const _SC_EXPR_NEST_MAX: ::c_int = 14;
pub const _SC_LINE_MAX: ::c_int = 15;
pub const _SC_RE_DUP_MAX: ::c_int = 16;
pub const _SC_2_VERSION: ::c_int = 17;
pub const _SC_2_C_BIND: ::c_int = 18;
pub const _SC_2_C_DEV: ::c_int = 19;
pub const _SC_2_CHAR_TERM: ::c_int = 20;
pub const _SC_2_FORT_DEV: ::c_int = 21;
pub const _SC_2_FORT_RUN: ::c_int = 22;
pub const _SC_2_LOCALEDEF: ::c_int = 23;
pub const _SC_2_SW_DEV: ::c_int = 24;
pub const _SC_2_UPE: ::c_int = 25;
pub const _SC_STREAM_MAX: ::c_int = 26;
pub const _SC_TZNAME_MAX: ::c_int = 27;
pub const _SC_ASYNCHRONOUS_IO: ::c_int = 28;
pub const _SC_MAPPED_FILES: ::c_int = 29;
pub const _SC_MEMLOCK: ::c_int = 30;
pub const _SC_MEMLOCK_RANGE: ::c_int = 31;
pub const _SC_MEMORY_PROTECTION: ::c_int = 32;
pub const _SC_MESSAGE_PASSING: ::c_int = 33;
pub const _SC_PRIORITIZED_IO: ::c_int = 34;
pub const _SC_PRIORITY_SCHEDULING: ::c_int = 35;
pub const _SC_REALTIME_SIGNALS: ::c_int = 36;
pub const _SC_SEMAPHORES: ::c_int = 37;
pub const _SC_FSYNC: ::c_int = 38;
pub const _SC_SHARED_MEMORY_OBJECTS: ::c_int = 39;
pub const _SC_SYNCHRONIZED_IO: ::c_int = 40;
pub const _SC_TIMERS: ::c_int = 41;
pub const _SC_AIO_LISTIO_MAX: ::c_int = 42;
pub const _SC_AIO_MAX: ::c_int = 43;
pub const _SC_AIO_PRIO_DELTA_MAX: ::c_int = 44;
pub const _SC_DELAYTIMER_MAX: ::c_int = 45;
pub const _SC_MQ_OPEN_MAX: ::c_int = 46;
pub const _SC_PAGESIZE: ::c_int = 47;
pub const _SC_PAGE_SIZE: ::c_int = _SC_PAGESIZE;
pub const _SC_RTSIG_MAX: ::c_int = 48;
pub const _SC_SEM_NSEMS_MAX: ::c_int = 49;
pub const _SC_SEM_VALUE_MAX: ::c_int = 50;
pub const _SC_SIGQUEUE_MAX: ::c_int = 51;
pub const _SC_TIMER_MAX: ::c_int = 52;
pub const _SC_IOV_MAX: ::c_int = 56;
pub const _SC_NPROCESSORS_CONF: ::c_int = 57;
pub const _SC_2_PBS: ::c_int = 59;
pub const _SC_2_PBS_ACCOUNTING: ::c_int = 60;
pub const _SC_2_PBS_CHECKPOINT: ::c_int = 61;
pub const _SC_2_PBS_LOCATE: ::c_int = 62;
pub const _SC_2_PBS_MESSAGE: ::c_int = 63;
pub const _SC_2_PBS_TRACK: ::c_int = 64;
pub const _SC_ADVISORY_INFO: ::c_int = 65;
pub const _SC_BARRIERS: ::c_int = 66;
pub const _SC_CLOCK_SELECTION: ::c_int = 67;
pub const _SC_CPUTIME: ::c_int = 68;
pub const _SC_FILE_LOCKING: ::c_int = 69;
pub const _SC_NPROCESSORS_ONLN: ::c_int = 58;
pub const _SC_GETGR_R_SIZE_MAX: ::c_int = 70;
pub const _SC_GETPW_R_SIZE_MAX: ::c_int = 71;
pub const _SC_HOST_NAME_MAX: ::c_int = 72;
pub const _SC_LOGIN_NAME_MAX: ::c_int = 73;
pub const _SC_MONOTONIC_CLOCK: ::c_int = 74;
pub const _SC_MQ_PRIO_MAX: ::c_int = 75;
pub const _SC_READER_WRITER_LOCKS: ::c_int = 76;
pub const _SC_REGEXP: ::c_int = 77;
pub const _SC_SHELL: ::c_int = 78;
pub const _SC_SPAWN: ::c_int = 79;
pub const _SC_SPIN_LOCKS: ::c_int = 80;
pub const _SC_SPORADIC_SERVER: ::c_int = 81;
pub const _SC_THREAD_ATTR_STACKADDR: ::c_int = 82;
pub const _SC_THREAD_ATTR_STACKSIZE: ::c_int = 83;
pub const _SC_THREAD_DESTRUCTOR_ITERATIONS: ::c_int = 85;
pub const _SC_THREAD_KEYS_MAX: ::c_int = 86;
pub const _SC_THREAD_PRIO_INHERIT: ::c_int = 87;
pub const _SC_THREAD_PRIO_PROTECT: ::c_int = 88;
pub const _SC_THREAD_PRIORITY_SCHEDULING: ::c_int = 89;
pub const _SC_THREAD_PROCESS_SHARED: ::c_int = 90;
pub const _SC_THREAD_SAFE_FUNCTIONS: ::c_int = 91;
pub const _SC_THREAD_SPORADIC_SERVER: ::c_int = 92;
pub const _SC_THREAD_STACK_MIN: ::c_int = 93;
pub const _SC_THREAD_THREADS_MAX: ::c_int = 94;
pub const _SC_TIMEOUTS: ::c_int = 95;
pub const _SC_THREADS: ::c_int = 96;
pub const _SC_TRACE: ::c_int = 97;
pub const _SC_TRACE_EVENT_FILTER: ::c_int = 98;
pub const _SC_TRACE_INHERIT: ::c_int = 99;
pub const _SC_TRACE_LOG: ::c_int = 100;
pub const _SC_TTY_NAME_MAX: ::c_int = 101;
pub const _SC_TYPED_MEMORY_OBJECTS: ::c_int = 102;
pub const _SC_V6_ILP32_OFF32: ::c_int = 103;
pub const _SC_V6_ILP32_OFFBIG: ::c_int = 104;
pub const _SC_V6_LP64_OFF64: ::c_int = 105;
pub const _SC_V6_LPBIG_OFFBIG: ::c_int = 106;
pub const _SC_ATEXIT_MAX: ::c_int = 107;
pub const _SC_XOPEN_CRYPT: ::c_int = 108;
pub const _SC_XOPEN_ENH_I18N: ::c_int = 109;
pub const _SC_XOPEN_LEGACY: ::c_int = 110;
pub const _SC_XOPEN_REALTIME: ::c_int = 111;
pub const _SC_XOPEN_REALTIME_THREADS: ::c_int = 112;
pub const _SC_XOPEN_SHM: ::c_int = 113;
pub const _SC_XOPEN_STREAMS: ::c_int = 114;
pub const _SC_XOPEN_UNIX: ::c_int = 115;
pub const _SC_XOPEN_VERSION: ::c_int = 116;
pub const _SC_XOPEN_XCU_VERSION: ::c_int = 117;
pub const _SC_IPV6: ::c_int = 118;
pub const _SC_RAW_SOCKETS: ::c_int = 119;
pub const _SC_SYMLOOP_MAX: ::c_int = 120;
pub const _SC_PHYS_PAGES: ::c_int = 121;

pub const PTHREAD_MUTEX_INITIALIZER: pthread_mutex_t = 0 as *mut _;
pub const PTHREAD_COND_INITIALIZER: pthread_cond_t = 0 as *mut _;
pub const PTHREAD_RWLOCK_INITIALIZER: pthread_rwlock_t = 0 as *mut _;
pub const PTHREAD_MUTEX_ERRORCHECK: ::c_int = 1;
pub const PTHREAD_MUTEX_RECURSIVE: ::c_int = 2;
pub const PTHREAD_MUTEX_NORMAL: ::c_int = 3;
pub const PTHREAD_MUTEX_DEFAULT: ::c_int = PTHREAD_MUTEX_ERRORCHECK;

pub const SCHED_FIFO: ::c_int = 1;
pub const SCHED_OTHER: ::c_int = 2;
pub const SCHED_RR: ::c_int = 3;

pub const FD_SETSIZE: usize = 1024;

pub const ST_NOSUID: ::c_ulong = 2;

pub const NI_MAXHOST: ::size_t = 1025;

pub const RTLD_LOCAL: ::c_int = 0;
pub const RTLD_NODELETE: ::c_int = 0x1000;
pub const RTLD_NOLOAD: ::c_int = 0x2000;
pub const RTLD_GLOBAL: ::c_int = 0x100;

pub const LOG_NTP: ::c_int = 12 << 3;
pub const LOG_SECURITY: ::c_int = 13 << 3;
pub const LOG_CONSOLE: ::c_int = 14 << 3;
pub const LOG_NFACILITIES: ::c_int = 24;

pub const TIOCEXCL: ::c_uint = 0x2000740d;
pub const TIOCNXCL: ::c_uint = 0x2000740e;
pub const TIOCFLUSH: ::c_ulong = 0x80047410;
pub const TIOCGETA: ::c_uint = 0x402c7413;
pub const TIOCSETA: ::c_ulong = 0x802c7414;
pub const TIOCSETAW: ::c_ulong = 0x802c7415;
pub const TIOCSETAF: ::c_ulong = 0x802c7416;
pub const TIOCGETD: ::c_uint = 0x4004741a;
pub const TIOCSETD: ::c_ulong = 0x8004741b;
pub const TIOCGDRAINWAIT: ::c_uint = 0x40047456;
pub const TIOCSDRAINWAIT: ::c_ulong = 0x80047457;
pub const TIOCTIMESTAMP: ::c_uint = 0x40107459;
pub const TIOCMGDTRWAIT: ::c_uint = 0x4004745a;
pub const TIOCMSDTRWAIT: ::c_ulong = 0x8004745b;
pub const TIOCDRAIN: ::c_uint = 0x2000745e;
pub const TIOCEXT: ::c_ulong = 0x80047460;
pub const TIOCSCTTY: ::c_uint = 0x20007461;
pub const TIOCCONS: ::c_ulong = 0x80047462;
pub const TIOCGSID: ::c_uint = 0x40047463;
pub const TIOCSTAT: ::c_uint = 0x20007465;
pub const TIOCUCNTL: ::c_ulong = 0x80047466;
pub const TIOCSWINSZ: ::c_ulong = 0x80087467;
pub const TIOCGWINSZ: ::c_uint = 0x40087468;
pub const TIOCMGET: ::c_uint = 0x4004746a;
pub const TIOCM_LE: ::c_int = 0x1;
pub const TIOCM_DTR: ::c_int = 0x2;
pub const TIOCM_RTS: ::c_int = 0x4;
pub const TIOCM_ST: ::c_int = 0x8;
pub const TIOCM_SR: ::c_int = 0x10;
pub const TIOCM_CTS: ::c_int = 0x20;
pub const TIOCM_RI: ::c_int = 0x80;
pub const TIOCM_DSR: ::c_int = 0x100;
pub const TIOCM_CD: ::c_int = 0x40;
pub const TIOCM_CAR: ::c_int = 0x40;
pub const TIOCM_RNG: ::c_int = 0x80;
pub const TIOCMBIC: ::c_ulong = 0x8004746b;
pub const TIOCMBIS: ::c_ulong = 0x8004746c;
pub const TIOCMSET: ::c_ulong = 0x8004746d;
pub const TIOCSTART: ::c_uint = 0x2000746e;
pub const TIOCSTOP: ::c_uint = 0x2000746f;
pub const TIOCPKT: ::c_ulong = 0x80047470;
pub const TIOCPKT_DATA: ::c_int = 0x0;
pub const TIOCPKT_FLUSHREAD: ::c_int = 0x1;
pub const TIOCPKT_FLUSHWRITE: ::c_int = 0x2;
pub const TIOCPKT_STOP: ::c_int = 0x4;
pub const TIOCPKT_START: ::c_int = 0x8;
pub const TIOCPKT_NOSTOP: ::c_int = 0x10;
pub const TIOCPKT_DOSTOP: ::c_int = 0x20;
pub const TIOCPKT_IOCTL: ::c_int = 0x40;
pub const TIOCNOTTY: ::c_uint = 0x20007471;
pub const TIOCSTI: ::c_ulong = 0x80017472;
pub const TIOCOUTQ: ::c_uint = 0x40047473;
pub const TIOCSPGRP: ::c_ulong = 0x80047476;
pub const TIOCGPGRP: ::c_uint = 0x40047477;
pub const TIOCCDTR: ::c_uint = 0x20007478;
pub const TIOCSDTR: ::c_uint = 0x20007479;
pub const TIOCCBRK: ::c_uint = 0x2000747a;
pub const TIOCSBRK: ::c_uint = 0x2000747b;
pub const TTYDISC: ::c_int = 0x0;
pub const SLIPDISC: ::c_int = 0x4;
pub const PPPDISC: ::c_int = 0x5;
pub const NETGRAPHDISC: ::c_int = 0x6;

pub const BIOCGRSIG: ::c_ulong = 0x40044272;
pub const BIOCSRSIG: ::c_ulong = 0x80044273;
pub const BIOCSDLT: ::c_ulong = 0x80044278;
pub const BIOCGSEESENT: ::c_ulong = 0x40044276;
pub const BIOCSSEESENT: ::c_ulong = 0x80044277;
pub const BIOCSETF: ::c_ulong = 0x80104267;
pub const BIOCGDLTLIST: ::c_ulong = 0xc0104279;
pub const BIOCSRTIMEOUT: ::c_ulong = 0x8010426d;
pub const BIOCGRTIMEOUT: ::c_ulong = 0x4010426e;

pub const FIODTYPE: ::c_ulong = 0x4004667a;
pub const FIOGETLBA: ::c_ulong = 0x40046679;
pub const FIODGNAME: ::c_ulong = 0x80106678;

pub const B0: speed_t = 0;
pub const B50: speed_t = 50;
pub const B75: speed_t = 75;
pub const B110: speed_t = 110;
pub const B134: speed_t = 134;
pub const B150: speed_t = 150;
pub const B200: speed_t = 200;
pub const B300: speed_t = 300;
pub const B600: speed_t = 600;
pub const B1200: speed_t = 1200;
pub const B1800: speed_t = 1800;
pub const B2400: speed_t = 2400;
pub const B4800: speed_t = 4800;
pub const B9600: speed_t = 9600;
pub const B19200: speed_t = 19200;
pub const B38400: speed_t = 38400;
pub const B7200: speed_t = 7200;
pub const B14400: speed_t = 14400;
pub const B28800: speed_t = 28800;
pub const B57600: speed_t = 57600;
pub const B76800: speed_t = 76800;
pub const B115200: speed_t = 115200;
pub const B230400: speed_t = 230400;
pub const EXTA: speed_t = 19200;
pub const EXTB: speed_t = 38400;

pub const SEM_FAILED: *mut sem_t = 0 as *mut sem_t;

pub const CRTSCTS: ::tcflag_t = 0x00030000;
pub const CCTS_OFLOW: ::tcflag_t = 0x00010000;
pub const CRTS_IFLOW: ::tcflag_t = 0x00020000;
pub const CDTR_IFLOW: ::tcflag_t = 0x00040000;
pub const CDSR_OFLOW: ::tcflag_t = 0x00080000;
pub const CCAR_OFLOW: ::tcflag_t = 0x00100000;
pub const VERASE2: usize = 7;
pub const OCRNL: ::tcflag_t = 0x10;
pub const ONOCR: ::tcflag_t = 0x20;
pub const ONLRET: ::tcflag_t = 0x40;

pub const CMGROUP_MAX: usize = 16;

// https://github.com/freebsd/freebsd/blob/master/sys/net/bpf.h
// sizeof(long)
pub const BPF_ALIGNMENT: ::c_int = 8;

// Values for rtprio struct (prio field) and syscall (function argument)
pub const RTP_PRIO_MIN: ::c_ushort = 0;
pub const RTP_PRIO_MAX: ::c_ushort = 31;
pub const RTP_LOOKUP: ::c_int = 0;
pub const RTP_SET: ::c_int = 1;

// Flags for chflags(2)
pub const UF_SETTABLE: ::c_ulong = 0x0000ffff;
pub const UF_NODUMP: ::c_ulong = 0x00000001;
pub const UF_IMMUTABLE: ::c_ulong = 0x00000002;
pub const UF_APPEND: ::c_ulong = 0x00000004;
pub const UF_OPAQUE: ::c_ulong = 0x00000008;
pub const UF_NOUNLINK: ::c_ulong = 0x00000010;
pub const SF_SETTABLE: ::c_ulong = 0xffff0000;
pub const SF_ARCHIVED: ::c_ulong = 0x00010000;
pub const SF_IMMUTABLE: ::c_ulong = 0x00020000;
pub const SF_APPEND: ::c_ulong = 0x00040000;
pub const SF_NOUNLINK: ::c_ulong = 0x00100000;

pub const TIMER_ABSTIME: ::c_int = 1;

f! {
    pub fn WIFCONTINUED(status: ::c_int) -> bool {
        status == 0x13
    }

    pub fn WSTOPSIG(status: ::c_int) -> ::c_int {
        status >> 8
    }

    pub fn WIFSIGNALED(status: ::c_int) -> bool {
        (status & 0o177) != 0o177 && (status & 0o177) != 0
    }

    pub fn WIFSTOPPED(status: ::c_int) -> bool {
        (status & 0o177) == 0o177
    }
}

extern "C" {
    pub fn sem_destroy(sem: *mut sem_t) -> ::c_int;
    pub fn sem_init(
        sem: *mut sem_t,
        pshared: ::c_int,
        value: ::c_uint,
    ) -> ::c_int;

    pub fn daemon(nochdir: ::c_int, noclose: ::c_int) -> ::c_int;
    pub fn gettimeofday(tp: *mut ::timeval, tz: *mut ::timezone) -> ::c_int;
    pub fn accept4(
        s: ::c_int,
        addr: *mut ::sockaddr,
        addrlen: *mut ::socklen_t,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn aio_read(aiocbp: *mut aiocb) -> ::c_int;
    pub fn aio_write(aiocbp: *mut aiocb) -> ::c_int;
    pub fn aio_fsync(op: ::c_int, aiocbp: *mut aiocb) -> ::c_int;
    pub fn aio_error(aiocbp: *const aiocb) -> ::c_int;
    pub fn aio_return(aiocbp: *mut aiocb) -> ::ssize_t;
    pub fn aio_suspend(
        aiocb_list: *const *const aiocb,
        nitems: ::c_int,
        timeout: *const ::timespec,
    ) -> ::c_int;
    pub fn aio_cancel(fd: ::c_int, aiocbp: *mut aiocb) -> ::c_int;
    pub fn chflags(path: *const ::c_char, flags: ::c_ulong) -> ::c_int;
    pub fn chflagsat(
        fd: ::c_int,
        path: *const ::c_char,
        flags: ::c_ulong,
        atflag: ::c_int,
    ) -> ::c_int;
    pub fn dirfd(dirp: *mut ::DIR) -> ::c_int;
    pub fn duplocale(base: ::locale_t) -> ::locale_t;
    pub fn endutxent();
    pub fn fchflags(fd: ::c_int, flags: ::c_ulong) -> ::c_int;
    pub fn futimens(fd: ::c_int, times: *const ::timespec) -> ::c_int;
    pub fn getdomainname(name: *mut ::c_char, len: ::c_int) -> ::c_int;
    pub fn getgrent_r(
        grp: *mut ::group,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut ::group,
    ) -> ::c_int;
    #[cfg_attr(target_os = "netbsd", link_name = "__getpwent_r50")]
    pub fn getpwent_r(
        pwd: *mut ::passwd,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut ::passwd,
    ) -> ::c_int;
    pub fn getgrouplist(
        name: *const ::c_char,
        basegid: ::gid_t,
        groups: *mut ::gid_t,
        ngroups: *mut ::c_int,
    ) -> ::c_int;
    pub fn getnameinfo(
        sa: *const ::sockaddr,
        salen: ::socklen_t,
        host: *mut ::c_char,
        hostlen: ::size_t,
        serv: *mut ::c_char,
        servlen: ::size_t,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn getpriority(which: ::c_int, who: ::c_int) -> ::c_int;
    pub fn getutxent() -> *mut utmpx;
    pub fn getutxid(ut: *const utmpx) -> *mut utmpx;
    pub fn getutxline(ut: *const utmpx) -> *mut utmpx;
    pub fn initgroups(name: *const ::c_char, basegid: ::gid_t) -> ::c_int;
    #[cfg_attr(
        all(target_os = "freebsd", any(freebsd11, freebsd10)),
        link_name = "kevent@FBSD_1.0"
    )]
    pub fn kevent(
        kq: ::c_int,
        changelist: *const ::kevent,
        nchanges: ::c_int,
        eventlist: *mut ::kevent,
        nevents: ::c_int,
        timeout: *const ::timespec,
    ) -> ::c_int;
    pub fn lchflags(path: *const ::c_char, flags: ::c_ulong) -> ::c_int;
    pub fn lio_listio(
        mode: ::c_int,
        aiocb_list: *const *mut aiocb,
        nitems: ::c_int,
        sevp: *mut sigevent,
    ) -> ::c_int;
    pub fn lutimes(file: *const ::c_char, times: *const ::timeval) -> ::c_int;
    pub fn memrchr(
        cx: *const ::c_void,
        c: ::c_int,
        n: ::size_t,
    ) -> *mut ::c_void;
    pub fn mkfifoat(
        dirfd: ::c_int,
        pathname: *const ::c_char,
        mode: ::mode_t,
    ) -> ::c_int;
    #[cfg_attr(
        all(target_os = "freebsd", any(freebsd11, freebsd10)),
        link_name = "mknodat@FBSD_1.1"
    )]
    pub fn mknodat(
        dirfd: ::c_int,
        pathname: *const ::c_char,
        mode: ::mode_t,
        dev: dev_t,
    ) -> ::c_int;
    pub fn mq_close(mqd: ::mqd_t) -> ::c_int;
    pub fn mq_getattr(mqd: ::mqd_t, attr: *mut ::mq_attr) -> ::c_int;
    pub fn mq_notify(mqd: ::mqd_t, notification: *const ::sigevent)
        -> ::c_int;
    pub fn mq_open(name: *const ::c_char, oflag: ::c_int, ...) -> ::mqd_t;
    pub fn mq_receive(
        mqd: ::mqd_t,
        msg_ptr: *mut ::c_char,
        msg_len: ::size_t,
        msg_prio: *mut ::c_uint,
    ) -> ::ssize_t;
    pub fn mq_send(
        mqd: ::mqd_t,
        msg_ptr: *const ::c_char,
        msg_len: ::size_t,
        msg_prio: ::c_uint,
    ) -> ::c_int;
    pub fn mq_setattr(
        mqd: ::mqd_t,
        newattr: *const ::mq_attr,
        oldattr: *mut ::mq_attr,
    ) -> ::c_int;
    pub fn mq_timedreceive(
        mqd: ::mqd_t,
        msg_ptr: *mut ::c_char,
        msg_len: ::size_t,
        msg_prio: *mut ::c_uint,
        abs_timeout: *const ::timespec,
    ) -> ::ssize_t;
    pub fn mq_timedsend(
        mqd: ::mqd_t,
        msg_ptr: *const ::c_char,
        msg_len: ::size_t,
        msg_prio: ::c_uint,
        abs_timeout: *const ::timespec,
    ) -> ::c_int;
    pub fn mq_unlink(name: *const ::c_char) -> ::c_int;
    pub fn mincore(
        addr: *const ::c_void,
        len: ::size_t,
        vec: *mut ::c_char,
    ) -> ::c_int;
    pub fn newlocale(
        mask: ::c_int,
        locale: *const ::c_char,
        base: ::locale_t,
    ) -> ::locale_t;
    pub fn nl_langinfo_l(item: ::nl_item, locale: ::locale_t)
        -> *mut ::c_char;
    pub fn pipe2(fds: *mut ::c_int, flags: ::c_int) -> ::c_int;
    pub fn ppoll(
        fds: *mut ::pollfd,
        nfds: ::nfds_t,
        timeout: *const ::timespec,
        sigmask: *const sigset_t,
    ) -> ::c_int;
    pub fn preadv(
        fd: ::c_int,
        iov: *const ::iovec,
        iovcnt: ::c_int,
        offset: ::off_t,
    ) -> ::ssize_t;
    pub fn pthread_attr_get_np(
        tid: ::pthread_t,
        attr: *mut ::pthread_attr_t,
    ) -> ::c_int;
    pub fn pthread_attr_getguardsize(
        attr: *const ::pthread_attr_t,
        guardsize: *mut ::size_t,
    ) -> ::c_int;
    pub fn pthread_attr_getstack(
        attr: *const ::pthread_attr_t,
        stackaddr: *mut *mut ::c_void,
        stacksize: *mut ::size_t,
    ) -> ::c_int;
    pub fn pthread_condattr_getclock(
        attr: *const pthread_condattr_t,
        clock_id: *mut clockid_t,
    ) -> ::c_int;
    pub fn pthread_condattr_getpshared(
        attr: *const pthread_condattr_t,
        pshared: *mut ::c_int,
    ) -> ::c_int;
    pub fn pthread_condattr_setclock(
        attr: *mut pthread_condattr_t,
        clock_id: ::clockid_t,
    ) -> ::c_int;
    pub fn pthread_condattr_setpshared(
        attr: *mut pthread_condattr_t,
        pshared: ::c_int,
    ) -> ::c_int;
    pub fn pthread_main_np() -> ::c_int;
    pub fn pthread_mutex_timedlock(
        lock: *mut pthread_mutex_t,
        abstime: *const ::timespec,
    ) -> ::c_int;
    pub fn pthread_mutexattr_getpshared(
        attr: *const pthread_mutexattr_t,
        pshared: *mut ::c_int,
    ) -> ::c_int;
    pub fn pthread_mutexattr_setpshared(
        attr: *mut pthread_mutexattr_t,
        pshared: ::c_int,
    ) -> ::c_int;
    pub fn pthread_rwlockattr_getpshared(
        attr: *const pthread_rwlockattr_t,
        val: *mut ::c_int,
    ) -> ::c_int;
    pub fn pthread_rwlockattr_setpshared(
        attr: *mut pthread_rwlockattr_t,
        val: ::c_int,
    ) -> ::c_int;
    pub fn pthread_set_name_np(tid: ::pthread_t, name: *const ::c_char);
    pub fn ptrace(
        request: ::c_int,
        pid: ::pid_t,
        addr: *mut ::c_char,
        data: ::c_int,
    ) -> ::c_int;
    pub fn pututxline(ut: *const utmpx) -> *mut utmpx;
    pub fn pwritev(
        fd: ::c_int,
        iov: *const ::iovec,
        iovcnt: ::c_int,
        offset: ::off_t,
    ) -> ::ssize_t;
    pub fn querylocale(mask: ::c_int, loc: ::locale_t) -> *const ::c_char;
    pub fn rtprio(
        function: ::c_int,
        pid: ::pid_t,
        rtp: *mut rtprio,
    ) -> ::c_int;
    pub fn sched_getscheduler(pid: ::pid_t) -> ::c_int;
    pub fn sched_setscheduler(
        pid: ::pid_t,
        policy: ::c_int,
        param: *const ::sched_param,
    ) -> ::c_int;
    pub fn sem_getvalue(sem: *mut sem_t, sval: *mut ::c_int) -> ::c_int;
    pub fn sem_timedwait(
        sem: *mut sem_t,
        abstime: *const ::timespec,
    ) -> ::c_int;
    pub fn sendfile(
        fd: ::c_int,
        s: ::c_int,
        offset: ::off_t,
        nbytes: ::size_t,
        hdtr: *mut ::sf_hdtr,
        sbytes: *mut ::off_t,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn setdomainname(name: *const ::c_char, len: ::c_int) -> ::c_int;
    pub fn sethostname(name: *const ::c_char, len: ::c_int) -> ::c_int;
    pub fn setpriority(which: ::c_int, who: ::c_int, prio: ::c_int)
        -> ::c_int;
    pub fn setresgid(rgid: ::gid_t, egid: ::gid_t, sgid: ::gid_t) -> ::c_int;
    pub fn setresuid(ruid: ::uid_t, euid: ::uid_t, suid: ::uid_t) -> ::c_int;
    pub fn settimeofday(
        tv: *const ::timeval,
        tz: *const ::timezone,
    ) -> ::c_int;
    pub fn setutxent();
    pub fn shm_open(
        name: *const ::c_char,
        oflag: ::c_int,
        mode: ::mode_t,
    ) -> ::c_int;
    pub fn sigtimedwait(
        set: *const sigset_t,
        info: *mut siginfo_t,
        timeout: *const ::timespec,
    ) -> ::c_int;
    pub fn sigwaitinfo(set: *const sigset_t, info: *mut siginfo_t) -> ::c_int;
    pub fn sysctl(
        name: *const ::c_int,
        namelen: ::c_uint,
        oldp: *mut ::c_void,
        oldlenp: *mut ::size_t,
        newp: *const ::c_void,
        newlen: ::size_t,
    ) -> ::c_int;
    pub fn sysctlbyname(
        name: *const ::c_char,
        oldp: *mut ::c_void,
        oldlenp: *mut ::size_t,
        newp: *const ::c_void,
        newlen: ::size_t,
    ) -> ::c_int;
    pub fn sysctlnametomib(
        name: *const ::c_char,
        mibp: *mut ::c_int,
        sizep: *mut ::size_t,
    ) -> ::c_int;
    pub fn uselocale(loc: ::locale_t) -> ::locale_t;
    pub fn utimensat(
        dirfd: ::c_int,
        path: *const ::c_char,
        times: *const ::timespec,
        flag: ::c_int,
    ) -> ::c_int;
}

#[link(name = "util")]
extern "C" {
    pub fn openpty(
        amaster: *mut ::c_int,
        aslave: *mut ::c_int,
        name: *mut ::c_char,
        termp: *mut termios,
        winp: *mut ::winsize,
    ) -> ::c_int;
    pub fn forkpty(
        amaster: *mut ::c_int,
        name: *mut ::c_char,
        termp: *mut termios,
        winp: *mut ::winsize,
    ) -> ::pid_t;
    pub fn login_tty(fd: ::c_int) -> ::c_int;
}

cfg_if! {
    if #[cfg(target_os = "freebsd")] {
        mod freebsd;
        pub use self::freebsd::*;
    } else if #[cfg(target_os = "dragonfly")] {
        mod dragonfly;
        pub use self::dragonfly::*;
    } else {
        // ...
    }
}
