pub type sa_family_t = u16;
pub type pthread_key_t = ::c_uint;
pub type speed_t = ::c_uint;
pub type tcflag_t = ::c_uint;
pub type loff_t = ::c_longlong;
pub type clockid_t = ::c_int;
pub type key_t = ::c_int;
pub type id_t = ::c_uint;
pub type useconds_t = u32;
pub type dev_t = u64;
pub type socklen_t = u32;
pub type mode_t = u32;
pub type ino64_t = u64;
pub type off64_t = i64;
pub type blkcnt64_t = i64;
pub type rlim64_t = u64;
pub type shmatt_t = ::c_ulong;
pub type mqd_t = ::c_int;
pub type msgqnum_t = ::c_ulong;
pub type msglen_t = ::c_ulong;
pub type nfds_t = ::c_ulong;
pub type nl_item = ::c_int;
pub type idtype_t = ::c_uint;

#[cfg_attr(feature = "extra_traits", derive(Debug))]
pub enum fpos64_t {} // TODO: fill this out with a struct
impl ::Copy for fpos64_t {}
impl ::Clone for fpos64_t {
    fn clone(&self) -> fpos64_t {
        *self
    }
}

#[cfg_attr(feature = "extra_traits", derive(Debug))]
pub enum timezone {}
impl ::Copy for timezone {}
impl ::Clone for timezone {
    fn clone(&self) -> timezone {
        *self
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

    pub struct sockaddr {
        pub sa_family: sa_family_t,
        pub sa_data: [::c_char; 14],
    }

    pub struct sockaddr_in {
        pub sin_family: sa_family_t,
        pub sin_port: ::in_port_t,
        pub sin_addr: ::in_addr,
        pub sin_zero: [u8; 8],
    }

    pub struct sockaddr_in6 {
        pub sin6_family: sa_family_t,
        pub sin6_port: ::in_port_t,
        pub sin6_flowinfo: u32,
        pub sin6_addr: ::in6_addr,
        pub sin6_scope_id: u32,
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

    pub struct sockaddr_ll {
        pub sll_family: ::c_ushort,
        pub sll_protocol: ::c_ushort,
        pub sll_ifindex: ::c_int,
        pub sll_hatype: ::c_ushort,
        pub sll_pkttype: ::c_uchar,
        pub sll_halen: ::c_uchar,
        pub sll_addr: [::c_uchar; 8]
    }

    pub struct fd_set {
        fds_bits: [::c_ulong; FD_SETSIZE / ULONG_SIZE],
    }

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
        pub tm_gmtoff: ::c_long,
        pub tm_zone: *const ::c_char,
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

    pub struct rlimit64 {
        pub rlim_cur: rlim64_t,
        pub rlim_max: rlim64_t,
    }

    pub struct glob_t {
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

    pub struct ifaddrs {
        pub ifa_next: *mut ifaddrs,
        pub ifa_name: *mut c_char,
        pub ifa_flags: ::c_uint,
        pub ifa_addr: *mut ::sockaddr,
        pub ifa_netmask: *mut ::sockaddr,
        pub ifa_ifu: *mut ::sockaddr, // FIXME This should be a union
        pub ifa_data: *mut ::c_void
    }

    pub struct pthread_rwlockattr_t {
        __lockkind: ::c_int,
        __pshared: ::c_int,
    }

    pub struct passwd {
        pub pw_name: *mut ::c_char,
        pub pw_passwd: *mut ::c_char,
        pub pw_uid: ::uid_t,
        pub pw_gid: ::gid_t,
        pub pw_gecos: *mut ::c_char,
        pub pw_dir: *mut ::c_char,
        pub pw_shell: *mut ::c_char,
    }

    pub struct spwd {
        pub sp_namp: *mut ::c_char,
        pub sp_pwdp: *mut ::c_char,
        pub sp_lstchg: ::c_long,
        pub sp_min: ::c_long,
        pub sp_max: ::c_long,
        pub sp_warn: ::c_long,
        pub sp_inact: ::c_long,
        pub sp_expire: ::c_long,
        pub sp_flag: ::c_ulong,
    }

    pub struct statvfs {
        pub f_bsize: ::c_ulong,
        pub f_frsize: ::c_ulong,
        pub f_blocks: ::fsblkcnt_t,
        pub f_bfree: ::fsblkcnt_t,
        pub f_bavail: ::fsblkcnt_t,
        pub f_files: ::fsfilcnt_t,
        pub f_ffree: ::fsfilcnt_t,
        pub f_favail: ::fsfilcnt_t,
        #[cfg(target_endian = "little")]
        pub f_fsid: ::c_ulong,
        #[cfg(target_pointer_width = "32")]
        __f_unused: ::c_int,
        #[cfg(target_endian = "big")]
        pub f_fsid: ::c_ulong,
        pub f_flag: ::c_ulong,
        pub f_namemax: ::c_ulong,
        __f_spare: [::c_int; 6],
    }

    pub struct dqblk {
        pub dqb_bhardlimit: u32,
        pub dqb_bsoftlimit: u32,
        pub dqb_curblocks: u32,
        pub dqb_ihardlimit: u32,
        pub dqb_isoftlimit: u32,
        pub dqb_curinodes: u32,
        pub dqb_btime: ::time_t,
        pub dqb_itime: ::time_t,
    }

    pub struct signalfd_siginfo {
        pub ssi_signo: u32,
        pub ssi_errno: i32,
        pub ssi_code: i32,
        pub ssi_pid: u32,
        pub ssi_uid: u32,
        pub ssi_fd: i32,
        pub ssi_tid: u32,
        pub ssi_band: u32,
        pub ssi_overrun: u32,
        pub ssi_trapno: u32,
        pub ssi_status: i32,
        pub ssi_int: i32,
        pub ssi_ptr: u64,
        pub ssi_utime: u64,
        pub ssi_stime: u64,
        pub ssi_addr: u64,
        pub ssi_addr_lsb: u16,
        _pad2: u16,
        pub ssi_syscall: i32,
        pub ssi_call_addr: u64,
        pub ssi_arch: u32,
        _pad: [u8; 28],
    }

    pub struct fsid_t {
        __val: [::c_int; 2],
    }

    pub struct cpu_set_t {
        #[cfg(target_pointer_width = "32")]
        bits: [u32; 32],
        #[cfg(target_pointer_width = "64")]
        bits: [u64; 16],
    }

    pub struct if_nameindex {
        pub if_index: ::c_uint,
        pub if_name: *mut ::c_char,
    }

    // System V IPC
    pub struct msginfo {
        pub msgpool: ::c_int,
        pub msgmap: ::c_int,
        pub msgmax: ::c_int,
        pub msgmnb: ::c_int,
        pub msgmni: ::c_int,
        pub msgssz: ::c_int,
        pub msgtql: ::c_int,
        pub msgseg: ::c_ushort,
    }
}

s_no_extra_traits! {
    #[cfg_attr(
        any(target_arch = "x86", target_arch = "x86_64"),
        repr(packed)
    )]
    #[allow(missing_debug_implementations)]
    pub struct epoll_event {
        pub events: u32,
        pub u64: u64,
    }

    #[allow(missing_debug_implementations)]
    pub struct sockaddr_un {
        pub sun_family: sa_family_t,
        pub sun_path: [::c_char; 108]
    }

    #[allow(missing_debug_implementations)]
    pub struct sockaddr_storage {
        pub ss_family: sa_family_t,
        __ss_align: ::size_t,
        #[cfg(target_pointer_width = "32")]
        __ss_pad2: [u8; 128 - 2 * 4],
        #[cfg(target_pointer_width = "64")]
        __ss_pad2: [u8; 128 - 2 * 8],
    }

    #[allow(missing_debug_implementations)]
    pub struct utsname {
        pub sysname: [::c_char; 65],
        pub nodename: [::c_char; 65],
        pub release: [::c_char; 65],
        pub version: [::c_char; 65],
        pub machine: [::c_char; 65],
        pub domainname: [::c_char; 65]
    }

    #[allow(missing_debug_implementations)]
    pub struct dirent {
        pub d_ino: ::ino_t,
        pub d_off: ::off_t,
        pub d_reclen: ::c_ushort,
        pub d_type: ::c_uchar,
        pub d_name: [::c_char; 256],
    }

    #[allow(missing_debug_implementations)]
    pub struct dirent64 {
        pub d_ino: ::ino64_t,
        pub d_off: ::off64_t,
        pub d_reclen: ::c_ushort,
        pub d_type: ::c_uchar,
        pub d_name: [::c_char; 256],
    }

    pub struct mq_attr {
        pub mq_flags: ::c_long,
        pub mq_maxmsg: ::c_long,
        pub mq_msgsize: ::c_long,
        pub mq_curmsgs: ::c_long,
        pad: [::c_long; 4]
    }

    pub struct sockaddr_nl {
        pub nl_family: ::sa_family_t,
        nl_pad: ::c_ushort,
        pub nl_pid: u32,
        pub nl_groups: u32
    }

    pub struct sigevent {
        pub sigev_value: ::sigval,
        pub sigev_signo: ::c_int,
        pub sigev_notify: ::c_int,
        // Actually a union.  We only expose sigev_notify_thread_id because it's
        // the most useful member
        pub sigev_notify_thread_id: ::c_int,
        #[cfg(target_pointer_width = "64")]
        __unused1: [::c_int; 11],
        #[cfg(target_pointer_width = "32")]
        __unused1: [::c_int; 12]
    }
}

cfg_if! {
    if #[cfg(feature = "extra_traits")] {
        impl PartialEq for mq_attr {
            fn eq(&self, other: &mq_attr) -> bool {
                self.mq_flags == other.mq_flags &&
                self.mq_maxmsg == other.mq_maxmsg &&
                self.mq_msgsize == other.mq_msgsize &&
                self.mq_curmsgs == other.mq_curmsgs
            }
        }
        impl Eq for mq_attr {}
        impl ::fmt::Debug for mq_attr {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("mq_attr")
                    .field("mq_flags", &self.mq_flags)
                    .field("mq_maxmsg", &self.mq_maxmsg)
                    .field("mq_msgsize", &self.mq_msgsize)
                    .field("mq_curmsgs", &self.mq_curmsgs)
                    .finish()
            }
        }
        impl ::hash::Hash for mq_attr {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.mq_flags.hash(state);
                self.mq_maxmsg.hash(state);
                self.mq_msgsize.hash(state);
                self.mq_curmsgs.hash(state);
            }
        }

        impl PartialEq for sockaddr_nl {
            fn eq(&self, other: &sockaddr_nl) -> bool {
                self.nl_family == other.nl_family &&
                self.nl_pid == other.nl_pid &&
                self.nl_groups == other.nl_groups
            }
        }
        impl Eq for sockaddr_nl {}
        impl ::fmt::Debug for sockaddr_nl {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("sockaddr_nl")
                    .field("nl_family", &self.nl_family)
                    .field("nl_pid", &self.nl_pid)
                    .field("nl_groups", &self.nl_groups)
                    .finish()
            }
        }
        impl ::hash::Hash for sockaddr_nl {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.nl_family.hash(state);
                self.nl_pid.hash(state);
                self.nl_groups.hash(state);
            }
        }

        impl PartialEq for sigevent {
            fn eq(&self, other: &sigevent) -> bool {
                self.sigev_value == other.sigev_value
                    && self.sigev_signo == other.sigev_signo
                    && self.sigev_notify == other.sigev_notify
                    && self.sigev_notify_thread_id
                        == other.sigev_notify_thread_id
            }
        }
        impl Eq for sigevent {}
        impl ::fmt::Debug for sigevent {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("sigevent")
                    .field("sigev_value", &self.sigev_value)
                    .field("sigev_signo", &self.sigev_signo)
                    .field("sigev_notify", &self.sigev_notify)
                    .field("sigev_notify_thread_id",
                           &self.sigev_notify_thread_id)
                    .finish()
            }
        }
        impl ::hash::Hash for sigevent {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.sigev_value.hash(state);
                self.sigev_signo.hash(state);
                self.sigev_notify.hash(state);
                self.sigev_notify_thread_id.hash(state);
            }
        }
    }
}

// intentionally not public, only used for fd_set
cfg_if! {
    if #[cfg(target_pointer_width = "32")] {
        const ULONG_SIZE: usize = 32;
    } else if #[cfg(target_pointer_width = "64")] {
        const ULONG_SIZE: usize = 64;
    } else {
        // Unknown target_pointer_width
    }
}

pub const EXIT_FAILURE: ::c_int = 1;
pub const EXIT_SUCCESS: ::c_int = 0;
pub const RAND_MAX: ::c_int = 2147483647;
pub const EOF: ::c_int = -1;
pub const SEEK_SET: ::c_int = 0;
pub const SEEK_CUR: ::c_int = 1;
pub const SEEK_END: ::c_int = 2;
pub const _IOFBF: ::c_int = 0;
pub const _IONBF: ::c_int = 2;
pub const _IOLBF: ::c_int = 1;

pub const F_DUPFD: ::c_int = 0;
pub const F_GETFD: ::c_int = 1;
pub const F_SETFD: ::c_int = 2;
pub const F_GETFL: ::c_int = 3;
pub const F_SETFL: ::c_int = 4;

// Linux-specific fcntls
pub const F_SETLEASE: ::c_int = 1024;
pub const F_GETLEASE: ::c_int = 1025;
pub const F_NOTIFY: ::c_int = 1026;
pub const F_DUPFD_CLOEXEC: ::c_int = 1030;

// TODO(#235): Include file sealing fcntls once we have a way to verify them.

pub const SIGTRAP: ::c_int = 5;

pub const PTHREAD_CREATE_JOINABLE: ::c_int = 0;
pub const PTHREAD_CREATE_DETACHED: ::c_int = 1;

pub const CLOCK_REALTIME: ::clockid_t = 0;
pub const CLOCK_MONOTONIC: ::clockid_t = 1;
pub const CLOCK_PROCESS_CPUTIME_ID: ::clockid_t = 2;
pub const CLOCK_THREAD_CPUTIME_ID: ::clockid_t = 3;
// TODO(#247) Someday our Travis shall have glibc 2.21 (released in Sep
// 2014.) See also musl/mod.rs
// pub const CLOCK_SGI_CYCLE: ::clockid_t = 10;
// pub const CLOCK_TAI: ::clockid_t = 11;
pub const TIMER_ABSTIME: ::c_int = 1;

pub const RLIMIT_CPU: ::c_int = 0;
pub const RLIMIT_FSIZE: ::c_int = 1;
pub const RLIMIT_DATA: ::c_int = 2;
pub const RLIMIT_STACK: ::c_int = 3;
pub const RLIMIT_CORE: ::c_int = 4;
pub const RLIMIT_LOCKS: ::c_int = 10;
pub const RLIMIT_SIGPENDING: ::c_int = 11;
pub const RLIMIT_MSGQUEUE: ::c_int = 12;
pub const RLIMIT_NICE: ::c_int = 13;
pub const RLIMIT_RTPRIO: ::c_int = 14;

pub const RUSAGE_SELF: ::c_int = 0;

pub const O_RDONLY: ::c_int = 0;
pub const O_WRONLY: ::c_int = 1;
pub const O_RDWR: ::c_int = 2;

pub const SOCK_CLOEXEC: ::c_int = O_CLOEXEC;

pub const S_IFIFO: ::mode_t = 4096;
pub const S_IFCHR: ::mode_t = 8192;
pub const S_IFBLK: ::mode_t = 24576;
pub const S_IFDIR: ::mode_t = 16384;
pub const S_IFREG: ::mode_t = 32768;
pub const S_IFLNK: ::mode_t = 40960;
pub const S_IFSOCK: ::mode_t = 49152;
pub const S_IFMT: ::mode_t = 61440;
pub const S_IRWXU: ::mode_t = 448;
pub const S_IXUSR: ::mode_t = 64;
pub const S_IWUSR: ::mode_t = 128;
pub const S_IRUSR: ::mode_t = 256;
pub const S_IRWXG: ::mode_t = 56;
pub const S_IXGRP: ::mode_t = 8;
pub const S_IWGRP: ::mode_t = 16;
pub const S_IRGRP: ::mode_t = 32;
pub const S_IRWXO: ::mode_t = 7;
pub const S_IXOTH: ::mode_t = 1;
pub const S_IWOTH: ::mode_t = 2;
pub const S_IROTH: ::mode_t = 4;
pub const F_OK: ::c_int = 0;
pub const R_OK: ::c_int = 4;
pub const W_OK: ::c_int = 2;
pub const X_OK: ::c_int = 1;
pub const STDIN_FILENO: ::c_int = 0;
pub const STDOUT_FILENO: ::c_int = 1;
pub const STDERR_FILENO: ::c_int = 2;
pub const SIGHUP: ::c_int = 1;
pub const SIGINT: ::c_int = 2;
pub const SIGQUIT: ::c_int = 3;
pub const SIGILL: ::c_int = 4;
pub const SIGABRT: ::c_int = 6;
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

pub const LC_CTYPE: ::c_int = 0;
pub const LC_NUMERIC: ::c_int = 1;
pub const LC_MONETARY: ::c_int = 2;
pub const LC_TIME: ::c_int = 3;
pub const LC_COLLATE: ::c_int = 4;
pub const LC_MESSAGES: ::c_int = 5;
pub const LC_ALL: ::c_int = 6;
pub const LC_CTYPE_MASK: ::c_int = (1 << LC_CTYPE);
pub const LC_NUMERIC_MASK: ::c_int = (1 << LC_NUMERIC);
pub const LC_TIME_MASK: ::c_int = (1 << LC_TIME);
pub const LC_COLLATE_MASK: ::c_int = (1 << LC_COLLATE);
pub const LC_MONETARY_MASK: ::c_int = (1 << LC_MONETARY);
pub const LC_MESSAGES_MASK: ::c_int = (1 << LC_MESSAGES);
// LC_ALL_MASK defined per platform

pub const MAP_FILE: ::c_int = 0x0000;
pub const MAP_SHARED: ::c_int = 0x0001;
pub const MAP_PRIVATE: ::c_int = 0x0002;
pub const MAP_FIXED: ::c_int = 0x0010;

pub const MAP_FAILED: *mut ::c_void = !0 as *mut ::c_void;

// MS_ flags for msync(2)
pub const MS_ASYNC: ::c_int = 0x0001;
pub const MS_INVALIDATE: ::c_int = 0x0002;
pub const MS_SYNC: ::c_int = 0x0004;

// MS_ flags for mount(2)
pub const MS_RDONLY: ::c_ulong = 0x01;
pub const MS_NOSUID: ::c_ulong = 0x02;
pub const MS_NODEV: ::c_ulong = 0x04;
pub const MS_NOEXEC: ::c_ulong = 0x08;
pub const MS_SYNCHRONOUS: ::c_ulong = 0x10;
pub const MS_REMOUNT: ::c_ulong = 0x20;
pub const MS_MANDLOCK: ::c_ulong = 0x40;
pub const MS_NOATIME: ::c_ulong = 0x0400;
pub const MS_NODIRATIME: ::c_ulong = 0x0800;
pub const MS_BIND: ::c_ulong = 0x1000;
pub const MS_NOUSER: ::c_ulong = 0x80000000;
pub const MS_MGC_VAL: ::c_ulong = 0xc0ed0000;
pub const MS_MGC_MSK: ::c_ulong = 0xffff0000;
pub const MS_RMT_MASK: ::c_ulong = 0x800051;

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
pub const EWOULDBLOCK: ::c_int = EAGAIN;

pub const SCM_RIGHTS: ::c_int = 0x01;
pub const SCM_CREDENTIALS: ::c_int = 0x02;

// netinet/in.h
// NOTE: These are in addition to the constants defined in src/unix/mod.rs

// IPPROTO_IP defined in src/unix/mod.rs
/// Hop-by-hop option header
pub const IPPROTO_HOPOPTS: ::c_int = 0;
// IPPROTO_ICMP defined in src/unix/mod.rs
/// group mgmt protocol
pub const IPPROTO_IGMP: ::c_int = 2;
/// for compatibility
pub const IPPROTO_IPIP: ::c_int = 4;
// IPPROTO_TCP defined in src/unix/mod.rs
/// exterior gateway protocol
pub const IPPROTO_EGP: ::c_int = 8;
/// pup
pub const IPPROTO_PUP: ::c_int = 12;
// IPPROTO_UDP defined in src/unix/mod.rs
/// xns idp
pub const IPPROTO_IDP: ::c_int = 22;
/// tp-4 w/ class negotiation
pub const IPPROTO_TP: ::c_int = 29;
/// DCCP
pub const IPPROTO_DCCP: ::c_int = 33;
// IPPROTO_IPV6 defined in src/unix/mod.rs
/// IP6 routing header
pub const IPPROTO_ROUTING: ::c_int = 43;
/// IP6 fragmentation header
pub const IPPROTO_FRAGMENT: ::c_int = 44;
/// resource reservation
pub const IPPROTO_RSVP: ::c_int = 46;
/// General Routing Encap.
pub const IPPROTO_GRE: ::c_int = 47;
/// IP6 Encap Sec. Payload
pub const IPPROTO_ESP: ::c_int = 50;
/// IP6 Auth Header
pub const IPPROTO_AH: ::c_int = 51;
// IPPROTO_ICMPV6 defined in src/unix/mod.rs
/// IP6 no next header
pub const IPPROTO_NONE: ::c_int = 59;
/// IP6 destination option
pub const IPPROTO_DSTOPTS: ::c_int = 60;
pub const IPPROTO_MTP: ::c_int = 92;
pub const IPPROTO_BEETPH: ::c_int = 94;
/// encapsulation header
pub const IPPROTO_ENCAP: ::c_int = 98;
/// Protocol indep. multicast
pub const IPPROTO_PIM: ::c_int = 103;
/// IP Payload Comp. Protocol
pub const IPPROTO_COMP: ::c_int = 108;
/// SCTP
pub const IPPROTO_SCTP: ::c_int = 132;
pub const IPPROTO_MH: ::c_int = 135;
pub const IPPROTO_UDPLITE: ::c_int = 136;
pub const IPPROTO_MPLS: ::c_int = 137;
/// raw IP packet
pub const IPPROTO_RAW: ::c_int = 255;
pub const IPPROTO_MAX: ::c_int = 256;

pub const PROT_GROWSDOWN: ::c_int = 0x1000000;
pub const PROT_GROWSUP: ::c_int = 0x2000000;

pub const MAP_TYPE: ::c_int = 0x000f;

pub const MADV_NORMAL: ::c_int = 0;
pub const MADV_RANDOM: ::c_int = 1;
pub const MADV_SEQUENTIAL: ::c_int = 2;
pub const MADV_WILLNEED: ::c_int = 3;
pub const MADV_DONTNEED: ::c_int = 4;
pub const MADV_REMOVE: ::c_int = 9;
pub const MADV_DONTFORK: ::c_int = 10;
pub const MADV_DOFORK: ::c_int = 11;
pub const MADV_MERGEABLE: ::c_int = 12;
pub const MADV_UNMERGEABLE: ::c_int = 13;
pub const MADV_HWPOISON: ::c_int = 100;

// https://github.com/kraj/uClibc/blob/master/include/net/if.h#L44
pub const IFF_UP: ::c_int = 0x1; // Interface is up.
pub const IFF_BROADCAST: ::c_int = 0x2; // Broadcast address valid.
pub const IFF_DEBUG: ::c_int = 0x4; // Turn on debugging.
pub const IFF_LOOPBACK: ::c_int = 0x8; // Is a loopback net.
pub const IFF_POINTOPOINT: ::c_int = 0x10; // Interface is point-to-point link.
pub const IFF_NOTRAILERS: ::c_int = 0x20; // Avoid use of trailers.
pub const IFF_RUNNING: ::c_int = 0x40; // Resources allocated.
pub const IFF_NOARP: ::c_int = 0x80; // No address resolution protocol.
pub const IFF_PROMISC: ::c_int = 0x100; // Receive all packets.

// Not supported
pub const IFF_ALLMULTI: ::c_int = 0x200; // Receive all multicast packets.
pub const IFF_MASTER: ::c_int = 0x400; // Master of a load balancer.
pub const IFF_SLAVE: ::c_int = 0x800; // Slave of a load balancer.
pub const IFF_MULTICAST: ::c_int = 0x1000; // Supports multicast.
pub const IFF_PORTSEL: ::c_int = 0x2000; // Can set media type.
pub const IFF_AUTOMEDIA: ::c_int = 0x4000; // Auto media select active.

// Dialup device with changing addresses.
pub const IFF_DYNAMIC: ::c_int = 0x8000;

pub const SOL_IP: ::c_int = 0;
pub const SOL_TCP: ::c_int = 6;
pub const SOL_IPV6: ::c_int = 41;
pub const SOL_ICMPV6: ::c_int = 58;
pub const SOL_RAW: ::c_int = 255;
pub const SOL_DECNET: ::c_int = 261;
pub const SOL_X25: ::c_int = 262;
pub const SOL_PACKET: ::c_int = 263;
pub const SOL_ATM: ::c_int = 264;
pub const SOL_AAL: ::c_int = 265;
pub const SOL_IRDA: ::c_int = 266;

pub const AF_UNSPEC: ::c_int = 0;
pub const AF_UNIX: ::c_int = 1;
pub const AF_LOCAL: ::c_int = 1;
pub const AF_INET: ::c_int = 2;
pub const AF_AX25: ::c_int = 3;
pub const AF_IPX: ::c_int = 4;
pub const AF_APPLETALK: ::c_int = 5;
pub const AF_NETROM: ::c_int = 6;
pub const AF_BRIDGE: ::c_int = 7;
pub const AF_ATMPVC: ::c_int = 8;
pub const AF_X25: ::c_int = 9;
pub const AF_INET6: ::c_int = 10;
pub const AF_ROSE: ::c_int = 11;
pub const AF_DECnet: ::c_int = 12;
pub const AF_NETBEUI: ::c_int = 13;
pub const AF_SECURITY: ::c_int = 14;
pub const AF_KEY: ::c_int = 15;
pub const AF_NETLINK: ::c_int = 16;
pub const AF_ROUTE: ::c_int = AF_NETLINK;
pub const AF_PACKET: ::c_int = 17;
pub const AF_ASH: ::c_int = 18;
pub const AF_ECONET: ::c_int = 19;
pub const AF_ATMSVC: ::c_int = 20;
pub const AF_SNA: ::c_int = 22;
pub const AF_IRDA: ::c_int = 23;
pub const AF_PPPOX: ::c_int = 24;
pub const AF_WANPIPE: ::c_int = 25;
pub const AF_LLC: ::c_int = 26;
pub const AF_CAN: ::c_int = 29;
pub const AF_TIPC: ::c_int = 30;
pub const AF_BLUETOOTH: ::c_int = 31;
pub const AF_IUCV: ::c_int = 32;
pub const AF_RXRPC: ::c_int = 33;
pub const AF_ISDN: ::c_int = 34;
pub const AF_PHONET: ::c_int = 35;
pub const AF_IEEE802154: ::c_int = 36;
pub const AF_CAIF: ::c_int = 37;
pub const AF_ALG: ::c_int = 38;

pub const PF_UNSPEC: ::c_int = AF_UNSPEC;
pub const PF_UNIX: ::c_int = AF_UNIX;
pub const PF_LOCAL: ::c_int = AF_LOCAL;
pub const PF_INET: ::c_int = AF_INET;
pub const PF_AX25: ::c_int = AF_AX25;
pub const PF_IPX: ::c_int = AF_IPX;
pub const PF_APPLETALK: ::c_int = AF_APPLETALK;
pub const PF_NETROM: ::c_int = AF_NETROM;
pub const PF_BRIDGE: ::c_int = AF_BRIDGE;
pub const PF_ATMPVC: ::c_int = AF_ATMPVC;
pub const PF_X25: ::c_int = AF_X25;
pub const PF_INET6: ::c_int = AF_INET6;
pub const PF_ROSE: ::c_int = AF_ROSE;
pub const PF_DECnet: ::c_int = AF_DECnet;
pub const PF_NETBEUI: ::c_int = AF_NETBEUI;
pub const PF_SECURITY: ::c_int = AF_SECURITY;
pub const PF_KEY: ::c_int = AF_KEY;
pub const PF_NETLINK: ::c_int = AF_NETLINK;
pub const PF_ROUTE: ::c_int = AF_ROUTE;
pub const PF_PACKET: ::c_int = AF_PACKET;
pub const PF_ASH: ::c_int = AF_ASH;
pub const PF_ECONET: ::c_int = AF_ECONET;
pub const PF_ATMSVC: ::c_int = AF_ATMSVC;
pub const PF_SNA: ::c_int = AF_SNA;
pub const PF_IRDA: ::c_int = AF_IRDA;
pub const PF_PPPOX: ::c_int = AF_PPPOX;
pub const PF_WANPIPE: ::c_int = AF_WANPIPE;
pub const PF_LLC: ::c_int = AF_LLC;
pub const PF_CAN: ::c_int = AF_CAN;
pub const PF_TIPC: ::c_int = AF_TIPC;
pub const PF_BLUETOOTH: ::c_int = AF_BLUETOOTH;
pub const PF_IUCV: ::c_int = AF_IUCV;
pub const PF_RXRPC: ::c_int = AF_RXRPC;
pub const PF_ISDN: ::c_int = AF_ISDN;
pub const PF_PHONET: ::c_int = AF_PHONET;
pub const PF_IEEE802154: ::c_int = AF_IEEE802154;
pub const PF_CAIF: ::c_int = AF_CAIF;
pub const PF_ALG: ::c_int = AF_ALG;

pub const SOMAXCONN: ::c_int = 128;

pub const MSG_OOB: ::c_int = 1;
pub const MSG_PEEK: ::c_int = 2;
pub const MSG_DONTROUTE: ::c_int = 4;
pub const MSG_CTRUNC: ::c_int = 8;
pub const MSG_TRUNC: ::c_int = 0x20;
pub const MSG_DONTWAIT: ::c_int = 0x40;
pub const MSG_EOR: ::c_int = 0x80;
pub const MSG_WAITALL: ::c_int = 0x100;
pub const MSG_FIN: ::c_int = 0x200;
pub const MSG_SYN: ::c_int = 0x400;
pub const MSG_CONFIRM: ::c_int = 0x800;
pub const MSG_RST: ::c_int = 0x1000;
pub const MSG_ERRQUEUE: ::c_int = 0x2000;
pub const MSG_NOSIGNAL: ::c_int = 0x4000;
pub const MSG_MORE: ::c_int = 0x8000;
pub const MSG_WAITFORONE: ::c_int = 0x10000;
pub const MSG_CMSG_CLOEXEC: ::c_int = 0x40000000;

pub const SOCK_RAW: ::c_int = 3;
pub const IP_MULTICAST_TTL: ::c_int = 33;
pub const IP_MULTICAST_LOOP: ::c_int = 34;
pub const IP_TTL: ::c_int = 2;
pub const IP_HDRINCL: ::c_int = 3;
pub const IP_ADD_MEMBERSHIP: ::c_int = 35;
pub const IP_DROP_MEMBERSHIP: ::c_int = 36;
pub const IPV6_ADD_MEMBERSHIP: ::c_int = 20;
pub const IPV6_DROP_MEMBERSHIP: ::c_int = 21;

pub const IPV6_JOIN_GROUP: ::c_int = 20;
pub const IPV6_LEAVE_GROUP: ::c_int = 21;

pub const TCP_NODELAY: ::c_int = 1;
pub const TCP_MAXSEG: ::c_int = 2;
pub const TCP_CORK: ::c_int = 3;
pub const TCP_KEEPIDLE: ::c_int = 4;
pub const TCP_KEEPINTVL: ::c_int = 5;
pub const TCP_KEEPCNT: ::c_int = 6;
pub const TCP_SYNCNT: ::c_int = 7;
pub const TCP_LINGER2: ::c_int = 8;
pub const TCP_DEFER_ACCEPT: ::c_int = 9;
pub const TCP_WINDOW_CLAMP: ::c_int = 10;
pub const TCP_INFO: ::c_int = 11;
pub const TCP_QUICKACK: ::c_int = 12;
pub const TCP_CONGESTION: ::c_int = 13;

pub const IPV6_MULTICAST_LOOP: ::c_int = 19;
pub const IPV6_V6ONLY: ::c_int = 26;

pub const SO_DEBUG: ::c_int = 1;

pub const SHUT_RD: ::c_int = 0;
pub const SHUT_WR: ::c_int = 1;
pub const SHUT_RDWR: ::c_int = 2;

pub const LOCK_SH: ::c_int = 1;
pub const LOCK_EX: ::c_int = 2;
pub const LOCK_NB: ::c_int = 4;
pub const LOCK_UN: ::c_int = 8;

pub const SS_ONSTACK: ::c_int = 1;
pub const SS_DISABLE: ::c_int = 2;

pub const PATH_MAX: ::c_int = 4096;

pub const FD_SETSIZE: usize = 1024;

pub const EPOLLIN: ::c_int = 0x1;
pub const EPOLLPRI: ::c_int = 0x2;
pub const EPOLLOUT: ::c_int = 0x4;
pub const EPOLLRDNORM: ::c_int = 0x40;
pub const EPOLLRDBAND: ::c_int = 0x80;
pub const EPOLLWRNORM: ::c_int = 0x100;
pub const EPOLLWRBAND: ::c_int = 0x200;
pub const EPOLLMSG: ::c_int = 0x400;
pub const EPOLLERR: ::c_int = 0x8;
pub const EPOLLHUP: ::c_int = 0x10;
pub const EPOLLET: ::c_int = 0x80000000;

pub const EPOLL_CTL_ADD: ::c_int = 1;
pub const EPOLL_CTL_MOD: ::c_int = 3;
pub const EPOLL_CTL_DEL: ::c_int = 2;

pub const MNT_DETACH: ::c_int = 0x2;
pub const MNT_EXPIRE: ::c_int = 0x4;

pub const MNT_FORCE: ::c_int = 0x1;

pub const Q_SYNC: ::c_int = 0x600;
pub const Q_QUOTAON: ::c_int = 0x100;
pub const Q_QUOTAOFF: ::c_int = 0x200;
pub const Q_GETQUOTA: ::c_int = 0x300;
pub const Q_SETQUOTA: ::c_int = 0x400;

pub const TCIOFF: ::c_int = 2;
pub const TCION: ::c_int = 3;
pub const TCOOFF: ::c_int = 0;
pub const TCOON: ::c_int = 1;
pub const TCIFLUSH: ::c_int = 0;
pub const TCOFLUSH: ::c_int = 1;
pub const TCIOFLUSH: ::c_int = 2;
pub const NL0: ::tcflag_t = 0x00000000;
pub const NL1: ::tcflag_t = 0x00000100;
pub const TAB0: ::tcflag_t = 0x00000000;
pub const CR0: ::tcflag_t = 0x00000000;
pub const FF0: ::tcflag_t = 0x00000000;
pub const BS0: ::tcflag_t = 0x00000000;
pub const VT0: ::tcflag_t = 0x00000000;
pub const VERASE: usize = 2;
pub const VKILL: usize = 3;
pub const VINTR: usize = 0;
pub const VQUIT: usize = 1;
pub const VLNEXT: usize = 15;
pub const IGNBRK: ::tcflag_t = 0x00000001;
pub const BRKINT: ::tcflag_t = 0x00000002;
pub const IGNPAR: ::tcflag_t = 0x00000004;
pub const PARMRK: ::tcflag_t = 0x00000008;
pub const INPCK: ::tcflag_t = 0x00000010;
pub const ISTRIP: ::tcflag_t = 0x00000020;
pub const INLCR: ::tcflag_t = 0x00000040;
pub const IGNCR: ::tcflag_t = 0x00000080;
pub const ICRNL: ::tcflag_t = 0x00000100;
pub const IXANY: ::tcflag_t = 0x00000800;
pub const IMAXBEL: ::tcflag_t = 0x00002000;
pub const OPOST: ::tcflag_t = 0x1;
pub const CS5: ::tcflag_t = 0x00000000;
pub const CRTSCTS: ::tcflag_t = 0x80000000;
pub const ECHO: ::tcflag_t = 0x00000008;

pub const CLONE_VM: ::c_int = 0x100;
pub const CLONE_FS: ::c_int = 0x200;
pub const CLONE_FILES: ::c_int = 0x400;
pub const CLONE_SIGHAND: ::c_int = 0x800;
pub const CLONE_PTRACE: ::c_int = 0x2000;
pub const CLONE_VFORK: ::c_int = 0x4000;
pub const CLONE_PARENT: ::c_int = 0x8000;
pub const CLONE_THREAD: ::c_int = 0x10000;
pub const CLONE_NEWNS: ::c_int = 0x20000;
pub const CLONE_SYSVSEM: ::c_int = 0x40000;
pub const CLONE_SETTLS: ::c_int = 0x80000;
pub const CLONE_PARENT_SETTID: ::c_int = 0x100000;
pub const CLONE_CHILD_CLEARTID: ::c_int = 0x200000;
pub const CLONE_DETACHED: ::c_int = 0x400000;
pub const CLONE_UNTRACED: ::c_int = 0x800000;
pub const CLONE_CHILD_SETTID: ::c_int = 0x01000000;
pub const CLONE_NEWUTS: ::c_int = 0x04000000;
pub const CLONE_NEWIPC: ::c_int = 0x08000000;
pub const CLONE_NEWUSER: ::c_int = 0x10000000;
pub const CLONE_NEWPID: ::c_int = 0x20000000;
pub const CLONE_NEWNET: ::c_int = 0x40000000;
pub const CLONE_IO: ::c_int = 0x80000000;

pub const WNOHANG: ::c_int = 0x00000001;
pub const WUNTRACED: ::c_int = 0x00000002;
pub const WSTOPPED: ::c_int = WUNTRACED;
pub const WEXITED: ::c_int = 0x00000004;
pub const WCONTINUED: ::c_int = 0x00000008;
pub const WNOWAIT: ::c_int = 0x01000000;

pub const __WNOTHREAD: ::c_int = 0x20000000;
pub const __WALL: ::c_int = 0x40000000;
pub const __WCLONE: ::c_int = 0x80000000;

pub const SPLICE_F_MOVE: ::c_uint = 0x01;
pub const SPLICE_F_NONBLOCK: ::c_uint = 0x02;
pub const SPLICE_F_MORE: ::c_uint = 0x04;
pub const SPLICE_F_GIFT: ::c_uint = 0x08;

pub const RTLD_LOCAL: ::c_int = 0;
pub const RTLD_LAZY: ::c_int = 1;

pub const POSIX_FADV_NORMAL: ::c_int = 0;
pub const POSIX_FADV_RANDOM: ::c_int = 1;
pub const POSIX_FADV_SEQUENTIAL: ::c_int = 2;
pub const POSIX_FADV_WILLNEED: ::c_int = 3;

pub const AT_FDCWD: ::c_int = -100;
pub const AT_SYMLINK_NOFOLLOW: ::c_int = 0x100;
pub const AT_REMOVEDIR: ::c_int = 0x200;
pub const AT_SYMLINK_FOLLOW: ::c_int = 0x400;

pub const LOG_CRON: ::c_int = 9 << 3;
pub const LOG_AUTHPRIV: ::c_int = 10 << 3;
pub const LOG_FTP: ::c_int = 11 << 3;
pub const LOG_PERROR: ::c_int = 0x20;

pub const POLLIN: ::c_short = 0x1;
pub const POLLPRI: ::c_short = 0x2;
pub const POLLOUT: ::c_short = 0x4;
pub const POLLERR: ::c_short = 0x8;
pub const POLLHUP: ::c_short = 0x10;
pub const POLLNVAL: ::c_short = 0x20;

pub const PIPE_BUF: usize = 4096;

pub const SI_LOAD_SHIFT: ::c_uint = 16;

pub const SIGEV_SIGNAL: ::c_int = 0;
pub const SIGEV_NONE: ::c_int = 1;
pub const SIGEV_THREAD: ::c_int = 2;

pub const P_ALL: idtype_t = 0;
pub const P_PID: idtype_t = 1;
pub const P_PGID: idtype_t = 2;

pub const UTIME_OMIT: c_long = 1073741822;
pub const UTIME_NOW: c_long = 1073741823;

pub const L_tmpnam: ::c_uint = 20;
pub const _PC_LINK_MAX: ::c_int = 0;
pub const _PC_MAX_CANON: ::c_int = 1;
pub const _PC_MAX_INPUT: ::c_int = 2;
pub const _PC_NAME_MAX: ::c_int = 3;
pub const _PC_PATH_MAX: ::c_int = 4;
pub const _PC_PIPE_BUF: ::c_int = 5;
pub const _PC_CHOWN_RESTRICTED: ::c_int = 6;
pub const _PC_NO_TRUNC: ::c_int = 7;
pub const _PC_VDISABLE: ::c_int = 8;

pub const _SC_ARG_MAX: ::c_int = 0;
pub const _SC_CHILD_MAX: ::c_int = 1;
pub const _SC_CLK_TCK: ::c_int = 2;
pub const _SC_NGROUPS_MAX: ::c_int = 3;
pub const _SC_OPEN_MAX: ::c_int = 4;
pub const _SC_STREAM_MAX: ::c_int = 5;
pub const _SC_TZNAME_MAX: ::c_int = 6;
pub const _SC_JOB_CONTROL: ::c_int = 7;
pub const _SC_SAVED_IDS: ::c_int = 8;
pub const _SC_REALTIME_SIGNALS: ::c_int = 9;
pub const _SC_PRIORITY_SCHEDULING: ::c_int = 10;
pub const _SC_TIMERS: ::c_int = 11;
pub const _SC_ASYNCHRONOUS_IO: ::c_int = 12;
pub const _SC_PRIORITIZED_IO: ::c_int = 13;
pub const _SC_SYNCHRONIZED_IO: ::c_int = 14;
pub const _SC_FSYNC: ::c_int = 15;
pub const _SC_MAPPED_FILES: ::c_int = 16;
pub const _SC_MEMLOCK: ::c_int = 17;
pub const _SC_MEMLOCK_RANGE: ::c_int = 18;
pub const _SC_MEMORY_PROTECTION: ::c_int = 19;
pub const _SC_MESSAGE_PASSING: ::c_int = 20;
pub const _SC_SEMAPHORES: ::c_int = 21;
pub const _SC_SHARED_MEMORY_OBJECTS: ::c_int = 22;
pub const _SC_AIO_LISTIO_MAX: ::c_int = 23;
pub const _SC_AIO_MAX: ::c_int = 24;
pub const _SC_AIO_PRIO_DELTA_MAX: ::c_int = 25;
pub const _SC_DELAYTIMER_MAX: ::c_int = 26;
pub const _SC_MQ_OPEN_MAX: ::c_int = 27;
pub const _SC_MQ_PRIO_MAX: ::c_int = 28;
pub const _SC_VERSION: ::c_int = 29;
pub const _SC_PAGESIZE: ::c_int = 30;
pub const _SC_PAGE_SIZE: ::c_int = _SC_PAGESIZE;
pub const _SC_RTSIG_MAX: ::c_int = 31;
pub const _SC_SEM_NSEMS_MAX: ::c_int = 32;
pub const _SC_SEM_VALUE_MAX: ::c_int = 33;
pub const _SC_SIGQUEUE_MAX: ::c_int = 34;
pub const _SC_TIMER_MAX: ::c_int = 35;
pub const _SC_BC_BASE_MAX: ::c_int = 36;
pub const _SC_BC_DIM_MAX: ::c_int = 37;
pub const _SC_BC_SCALE_MAX: ::c_int = 38;
pub const _SC_BC_STRING_MAX: ::c_int = 39;
pub const _SC_COLL_WEIGHTS_MAX: ::c_int = 40;
pub const _SC_EXPR_NEST_MAX: ::c_int = 42;
pub const _SC_LINE_MAX: ::c_int = 43;
pub const _SC_RE_DUP_MAX: ::c_int = 44;
pub const _SC_2_VERSION: ::c_int = 46;
pub const _SC_2_C_BIND: ::c_int = 47;
pub const _SC_2_C_DEV: ::c_int = 48;
pub const _SC_2_FORT_DEV: ::c_int = 49;
pub const _SC_2_FORT_RUN: ::c_int = 50;
pub const _SC_2_SW_DEV: ::c_int = 51;
pub const _SC_2_LOCALEDEF: ::c_int = 52;
pub const _SC_IOV_MAX: ::c_int = 60;
pub const _SC_THREADS: ::c_int = 67;
pub const _SC_THREAD_SAFE_FUNCTIONS: ::c_int = 68;
pub const _SC_GETGR_R_SIZE_MAX: ::c_int = 69;
pub const _SC_GETPW_R_SIZE_MAX: ::c_int = 70;
pub const _SC_LOGIN_NAME_MAX: ::c_int = 71;
pub const _SC_TTY_NAME_MAX: ::c_int = 72;
pub const _SC_THREAD_DESTRUCTOR_ITERATIONS: ::c_int = 73;
pub const _SC_THREAD_KEYS_MAX: ::c_int = 74;
pub const _SC_THREAD_STACK_MIN: ::c_int = 75;
pub const _SC_THREAD_THREADS_MAX: ::c_int = 76;
pub const _SC_THREAD_ATTR_STACKADDR: ::c_int = 77;
pub const _SC_THREAD_ATTR_STACKSIZE: ::c_int = 78;
pub const _SC_THREAD_PRIORITY_SCHEDULING: ::c_int = 79;
pub const _SC_THREAD_PRIO_INHERIT: ::c_int = 80;
pub const _SC_THREAD_PRIO_PROTECT: ::c_int = 81;
pub const _SC_NPROCESSORS_ONLN: ::c_int = 84;
pub const _SC_ATEXIT_MAX: ::c_int = 87;
pub const _SC_XOPEN_VERSION: ::c_int = 89;
pub const _SC_XOPEN_XCU_VERSION: ::c_int = 90;
pub const _SC_XOPEN_UNIX: ::c_int = 91;
pub const _SC_XOPEN_CRYPT: ::c_int = 92;
pub const _SC_XOPEN_ENH_I18N: ::c_int = 93;
pub const _SC_XOPEN_SHM: ::c_int = 94;
pub const _SC_2_CHAR_TERM: ::c_int = 95;
pub const _SC_2_UPE: ::c_int = 97;
pub const _SC_XBS5_ILP32_OFF32: ::c_int = 125;
pub const _SC_XBS5_ILP32_OFFBIG: ::c_int = 126;
pub const _SC_XBS5_LPBIG_OFFBIG: ::c_int = 128;
pub const _SC_XOPEN_LEGACY: ::c_int = 129;
pub const _SC_XOPEN_REALTIME: ::c_int = 130;
pub const _SC_XOPEN_REALTIME_THREADS: ::c_int = 131;
pub const _SC_HOST_NAME_MAX: ::c_int = 180;

pub const RLIM_SAVED_MAX: ::rlim_t = RLIM_INFINITY;
pub const RLIM_SAVED_CUR: ::rlim_t = RLIM_INFINITY;

pub const GLOB_ERR: ::c_int = 1 << 0;
pub const GLOB_MARK: ::c_int = 1 << 1;
pub const GLOB_NOSORT: ::c_int = 1 << 2;
pub const GLOB_DOOFFS: ::c_int = 1 << 3;
pub const GLOB_NOCHECK: ::c_int = 1 << 4;
pub const GLOB_APPEND: ::c_int = 1 << 5;
pub const GLOB_NOESCAPE: ::c_int = 1 << 6;

pub const GLOB_NOSPACE: ::c_int = 1;
pub const GLOB_ABORTED: ::c_int = 2;
pub const GLOB_NOMATCH: ::c_int = 3;

pub const POSIX_MADV_NORMAL: ::c_int = 0;
pub const POSIX_MADV_RANDOM: ::c_int = 1;
pub const POSIX_MADV_SEQUENTIAL: ::c_int = 2;
pub const POSIX_MADV_WILLNEED: ::c_int = 3;

pub const S_IEXEC: mode_t = 64;
pub const S_IWRITE: mode_t = 128;
pub const S_IREAD: mode_t = 256;

pub const F_LOCK: ::c_int = 1;
pub const F_TEST: ::c_int = 3;
pub const F_TLOCK: ::c_int = 2;
pub const F_ULOCK: ::c_int = 0;

pub const ST_RDONLY: ::c_ulong = 1;
pub const ST_NOSUID: ::c_ulong = 2;
pub const ST_NODEV: ::c_ulong = 4;
pub const ST_NOEXEC: ::c_ulong = 8;
pub const ST_SYNCHRONOUS: ::c_ulong = 16;
pub const ST_MANDLOCK: ::c_ulong = 64;
pub const ST_WRITE: ::c_ulong = 128;
pub const ST_APPEND: ::c_ulong = 256;
pub const ST_IMMUTABLE: ::c_ulong = 512;
pub const ST_NOATIME: ::c_ulong = 1024;
pub const ST_NODIRATIME: ::c_ulong = 2048;

pub const RTLD_NEXT: *mut ::c_void = -1i64 as *mut ::c_void;
pub const RTLD_DEFAULT: *mut ::c_void = 0i64 as *mut ::c_void;
pub const RTLD_NODELETE: ::c_int = 0x1000;
pub const RTLD_NOW: ::c_int = 0x2;

pub const TCP_MD5SIG: ::c_int = 14;

align_const! {
    pub const PTHREAD_MUTEX_INITIALIZER: pthread_mutex_t = pthread_mutex_t {
        size: [0; __SIZEOF_PTHREAD_MUTEX_T],
    };
    pub const PTHREAD_COND_INITIALIZER: pthread_cond_t = pthread_cond_t {
        size: [0; __SIZEOF_PTHREAD_COND_T],
    };
    pub const PTHREAD_RWLOCK_INITIALIZER: pthread_rwlock_t = pthread_rwlock_t {
        size: [0; __SIZEOF_PTHREAD_RWLOCK_T],
    };
}
pub const PTHREAD_MUTEX_NORMAL: ::c_int = 0;
pub const PTHREAD_MUTEX_RECURSIVE: ::c_int = 1;
pub const PTHREAD_MUTEX_ERRORCHECK: ::c_int = 2;
pub const PTHREAD_MUTEX_DEFAULT: ::c_int = PTHREAD_MUTEX_NORMAL;
pub const __SIZEOF_PTHREAD_COND_T: usize = 48;

pub const SCHED_OTHER: ::c_int = 0;
pub const SCHED_FIFO: ::c_int = 1;
pub const SCHED_RR: ::c_int = 2;
pub const SCHED_BATCH: ::c_int = 3;
pub const SCHED_IDLE: ::c_int = 5;

// System V IPC
pub const IPC_PRIVATE: ::key_t = 0;

pub const IPC_CREAT: ::c_int = 0o1000;
pub const IPC_EXCL: ::c_int = 0o2000;
pub const IPC_NOWAIT: ::c_int = 0o4000;

pub const IPC_RMID: ::c_int = 0;
pub const IPC_SET: ::c_int = 1;
pub const IPC_STAT: ::c_int = 2;
pub const IPC_INFO: ::c_int = 3;
pub const MSG_STAT: ::c_int = 11;
pub const MSG_INFO: ::c_int = 12;

pub const MSG_NOERROR: ::c_int = 0o10000;
pub const MSG_EXCEPT: ::c_int = 0o20000;

pub const SHM_R: ::c_int = 0o400;
pub const SHM_W: ::c_int = 0o200;

pub const SHM_RDONLY: ::c_int = 0o10000;
pub const SHM_RND: ::c_int = 0o20000;
pub const SHM_REMAP: ::c_int = 0o40000;

pub const SHM_LOCK: ::c_int = 11;
pub const SHM_UNLOCK: ::c_int = 12;

pub const SHM_HUGETLB: ::c_int = 0o4000;
pub const SHM_NORESERVE: ::c_int = 0o10000;

pub const EPOLLRDHUP: ::c_int = 0x2000;
pub const EPOLLONESHOT: ::c_int = 0x40000000;

pub const QFMT_VFS_OLD: ::c_int = 1;
pub const QFMT_VFS_V0: ::c_int = 2;

pub const EFD_SEMAPHORE: ::c_int = 0x1;

pub const LOG_NFACILITIES: ::c_int = 24;

pub const SEM_FAILED: *mut ::sem_t = 0 as *mut sem_t;

pub const RB_AUTOBOOT: ::c_int = 0x01234567u32 as i32;
pub const RB_HALT_SYSTEM: ::c_int = 0xcdef0123u32 as i32;
pub const RB_ENABLE_CAD: ::c_int = 0x89abcdefu32 as i32;
pub const RB_DISABLE_CAD: ::c_int = 0x00000000u32 as i32;
pub const RB_POWER_OFF: ::c_int = 0x4321fedcu32 as i32;

pub const AI_PASSIVE: ::c_int = 0x0001;
pub const AI_CANONNAME: ::c_int = 0x0002;
pub const AI_NUMERICHOST: ::c_int = 0x0004;
pub const AI_V4MAPPED: ::c_int = 0x0008;
pub const AI_ALL: ::c_int = 0x0010;
pub const AI_ADDRCONFIG: ::c_int = 0x0020;

pub const AI_NUMERICSERV: ::c_int = 0x0400;

pub const EAI_BADFLAGS: ::c_int = -1;
pub const EAI_NONAME: ::c_int = -2;
pub const EAI_AGAIN: ::c_int = -3;
pub const EAI_FAIL: ::c_int = -4;
pub const EAI_FAMILY: ::c_int = -6;
pub const EAI_SOCKTYPE: ::c_int = -7;
pub const EAI_SERVICE: ::c_int = -8;
pub const EAI_MEMORY: ::c_int = -10;
pub const EAI_OVERFLOW: ::c_int = -12;

pub const NI_NUMERICHOST: ::c_int = 1;
pub const NI_NUMERICSERV: ::c_int = 2;
pub const NI_NOFQDN: ::c_int = 4;
pub const NI_NAMEREQD: ::c_int = 8;
pub const NI_DGRAM: ::c_int = 16;

pub const SYNC_FILE_RANGE_WAIT_BEFORE: ::c_uint = 1;
pub const SYNC_FILE_RANGE_WRITE: ::c_uint = 2;
pub const SYNC_FILE_RANGE_WAIT_AFTER: ::c_uint = 4;

pub const EAI_SYSTEM: ::c_int = -11;

pub const MREMAP_MAYMOVE: ::c_int = 1;
pub const MREMAP_FIXED: ::c_int = 2;

pub const PR_SET_PDEATHSIG: ::c_int = 1;
pub const PR_GET_PDEATHSIG: ::c_int = 2;

pub const PR_GET_DUMPABLE: ::c_int = 3;
pub const PR_SET_DUMPABLE: ::c_int = 4;

pub const PR_GET_UNALIGN: ::c_int = 5;
pub const PR_SET_UNALIGN: ::c_int = 6;
pub const PR_UNALIGN_NOPRINT: ::c_int = 1;
pub const PR_UNALIGN_SIGBUS: ::c_int = 2;

pub const PR_GET_KEEPCAPS: ::c_int = 7;
pub const PR_SET_KEEPCAPS: ::c_int = 8;

pub const PR_GET_FPEMU: ::c_int = 9;
pub const PR_SET_FPEMU: ::c_int = 10;
pub const PR_FPEMU_NOPRINT: ::c_int = 1;
pub const PR_FPEMU_SIGFPE: ::c_int = 2;

pub const PR_GET_FPEXC: ::c_int = 11;
pub const PR_SET_FPEXC: ::c_int = 12;
pub const PR_FP_EXC_SW_ENABLE: ::c_int = 0x80;
pub const PR_FP_EXC_DIV: ::c_int = 0x010000;
pub const PR_FP_EXC_OVF: ::c_int = 0x020000;
pub const PR_FP_EXC_UND: ::c_int = 0x040000;
pub const PR_FP_EXC_RES: ::c_int = 0x080000;
pub const PR_FP_EXC_INV: ::c_int = 0x100000;
pub const PR_FP_EXC_DISABLED: ::c_int = 0;
pub const PR_FP_EXC_NONRECOV: ::c_int = 1;
pub const PR_FP_EXC_ASYNC: ::c_int = 2;
pub const PR_FP_EXC_PRECISE: ::c_int = 3;

pub const PR_GET_TIMING: ::c_int = 13;
pub const PR_SET_TIMING: ::c_int = 14;
pub const PR_TIMING_STATISTICAL: ::c_int = 0;
pub const PR_TIMING_TIMESTAMP: ::c_int = 1;

pub const PR_SET_NAME: ::c_int = 15;
pub const PR_GET_NAME: ::c_int = 16;

pub const PR_GET_ENDIAN: ::c_int = 19;
pub const PR_SET_ENDIAN: ::c_int = 20;
pub const PR_ENDIAN_BIG: ::c_int = 0;
pub const PR_ENDIAN_LITTLE: ::c_int = 1;
pub const PR_ENDIAN_PPC_LITTLE: ::c_int = 2;

pub const PR_GET_SECCOMP: ::c_int = 21;
pub const PR_SET_SECCOMP: ::c_int = 22;

pub const PR_CAPBSET_READ: ::c_int = 23;
pub const PR_CAPBSET_DROP: ::c_int = 24;

pub const PR_GET_TSC: ::c_int = 25;
pub const PR_SET_TSC: ::c_int = 26;
pub const PR_TSC_ENABLE: ::c_int = 1;
pub const PR_TSC_SIGSEGV: ::c_int = 2;

pub const PR_GET_SECUREBITS: ::c_int = 27;
pub const PR_SET_SECUREBITS: ::c_int = 28;

pub const PR_SET_TIMERSLACK: ::c_int = 29;
pub const PR_GET_TIMERSLACK: ::c_int = 30;

pub const PR_TASK_PERF_EVENTS_DISABLE: ::c_int = 31;
pub const PR_TASK_PERF_EVENTS_ENABLE: ::c_int = 32;

pub const PR_MCE_KILL: ::c_int = 33;
pub const PR_MCE_KILL_CLEAR: ::c_int = 0;
pub const PR_MCE_KILL_SET: ::c_int = 1;

pub const PR_MCE_KILL_LATE: ::c_int = 0;
pub const PR_MCE_KILL_EARLY: ::c_int = 1;
pub const PR_MCE_KILL_DEFAULT: ::c_int = 2;

pub const PR_MCE_KILL_GET: ::c_int = 34;

pub const PR_SET_MM: ::c_int = 35;
pub const PR_SET_MM_START_CODE: ::c_int = 1;
pub const PR_SET_MM_END_CODE: ::c_int = 2;
pub const PR_SET_MM_START_DATA: ::c_int = 3;
pub const PR_SET_MM_END_DATA: ::c_int = 4;
pub const PR_SET_MM_START_STACK: ::c_int = 5;
pub const PR_SET_MM_START_BRK: ::c_int = 6;
pub const PR_SET_MM_BRK: ::c_int = 7;
pub const PR_SET_MM_ARG_START: ::c_int = 8;
pub const PR_SET_MM_ARG_END: ::c_int = 9;
pub const PR_SET_MM_ENV_START: ::c_int = 10;
pub const PR_SET_MM_ENV_END: ::c_int = 11;
pub const PR_SET_MM_AUXV: ::c_int = 12;
pub const PR_SET_MM_EXE_FILE: ::c_int = 13;
pub const PR_SET_MM_MAP: ::c_int = 14;
pub const PR_SET_MM_MAP_SIZE: ::c_int = 15;

pub const PR_SET_PTRACER: ::c_int = 0x59616d61;

pub const PR_SET_CHILD_SUBREAPER: ::c_int = 36;
pub const PR_GET_CHILD_SUBREAPER: ::c_int = 37;

pub const PR_SET_NO_NEW_PRIVS: ::c_int = 38;
pub const PR_GET_NO_NEW_PRIVS: ::c_int = 39;

pub const PR_GET_TID_ADDRESS: ::c_int = 40;

pub const PR_SET_THP_DISABLE: ::c_int = 41;
pub const PR_GET_THP_DISABLE: ::c_int = 42;

pub const GRND_NONBLOCK: ::c_uint = 0x0001;
pub const GRND_RANDOM: ::c_uint = 0x0002;

pub const ABDAY_1: ::nl_item = 0x300;
pub const ABDAY_2: ::nl_item = 0x301;
pub const ABDAY_3: ::nl_item = 0x302;
pub const ABDAY_4: ::nl_item = 0x303;
pub const ABDAY_5: ::nl_item = 0x304;
pub const ABDAY_6: ::nl_item = 0x305;
pub const ABDAY_7: ::nl_item = 0x306;

pub const DAY_1: ::nl_item = 0x307;
pub const DAY_2: ::nl_item = 0x308;
pub const DAY_3: ::nl_item = 0x309;
pub const DAY_4: ::nl_item = 0x30A;
pub const DAY_5: ::nl_item = 0x30B;
pub const DAY_6: ::nl_item = 0x30C;
pub const DAY_7: ::nl_item = 0x30D;

pub const ABMON_1: ::nl_item = 0x30E;
pub const ABMON_2: ::nl_item = 0x30F;
pub const ABMON_3: ::nl_item = 0x310;
pub const ABMON_4: ::nl_item = 0x311;
pub const ABMON_5: ::nl_item = 0x312;
pub const ABMON_6: ::nl_item = 0x313;
pub const ABMON_7: ::nl_item = 0x314;
pub const ABMON_8: ::nl_item = 0x315;
pub const ABMON_9: ::nl_item = 0x316;
pub const ABMON_10: ::nl_item = 0x317;
pub const ABMON_11: ::nl_item = 0x318;
pub const ABMON_12: ::nl_item = 0x319;

pub const MON_1: ::nl_item = 0x31A;
pub const MON_2: ::nl_item = 0x31B;
pub const MON_3: ::nl_item = 0x31C;
pub const MON_4: ::nl_item = 0x31D;
pub const MON_5: ::nl_item = 0x31E;
pub const MON_6: ::nl_item = 0x31F;
pub const MON_7: ::nl_item = 0x320;
pub const MON_8: ::nl_item = 0x321;
pub const MON_9: ::nl_item = 0x322;
pub const MON_10: ::nl_item = 0x323;
pub const MON_11: ::nl_item = 0x324;
pub const MON_12: ::nl_item = 0x325;

pub const AM_STR: ::nl_item = 0x326;
pub const PM_STR: ::nl_item = 0x327;

pub const D_T_FMT: ::nl_item = 0x328;
pub const D_FMT: ::nl_item = 0x329;
pub const T_FMT: ::nl_item = 0x32A;
pub const T_FMT_AMPM: ::nl_item = 0x32B;

pub const ERA: ::nl_item = 0x32C;
pub const ERA_D_FMT: ::nl_item = 0x32E;
pub const ALT_DIGITS: ::nl_item = 0x32F;
pub const ERA_D_T_FMT: ::nl_item = 0x330;
pub const ERA_T_FMT: ::nl_item = 0x331;

pub const CODESET: ::nl_item = 10;

pub const CRNCYSTR: ::nl_item = 0x215;

pub const RADIXCHAR: ::nl_item = 0x100;
pub const THOUSEP: ::nl_item = 0x101;

pub const NOEXPR: ::nl_item = 0x501;
pub const YESSTR: ::nl_item = 0x502;
pub const NOSTR: ::nl_item = 0x503;

pub const FILENAME_MAX: ::c_uint = 4095;

f! {
    pub fn FD_CLR(fd: ::c_int, set: *mut fd_set) -> () {
        let fd = fd as usize;
        let size = ::mem::size_of_val(&(*set).fds_bits[0]) * 8;
        (*set).fds_bits[fd / size] &= !(1 << (fd % size));
        return
    }

    pub fn FD_ISSET(fd: ::c_int, set: *mut fd_set) -> bool {
        let fd = fd as usize;
        let size = ::mem::size_of_val(&(*set).fds_bits[0]) * 8;
        return ((*set).fds_bits[fd / size] & (1 << (fd % size))) != 0
    }

    pub fn FD_SET(fd: ::c_int, set: *mut fd_set) -> () {
        let fd = fd as usize;
        let size = ::mem::size_of_val(&(*set).fds_bits[0]) * 8;
        (*set).fds_bits[fd / size] |= 1 << (fd % size);
        return
    }

    pub fn FD_ZERO(set: *mut fd_set) -> () {
        for slot in (*set).fds_bits.iter_mut() {
            *slot = 0;
        }
    }

    pub fn WIFSTOPPED(status: ::c_int) -> bool {
        (status & 0xff) == 0x7f
    }

    pub fn WSTOPSIG(status: ::c_int) -> ::c_int {
        (status >> 8) & 0xff
    }

    pub fn WIFCONTINUED(status: ::c_int) -> bool {
        status == 0xffff
    }

    pub fn WIFSIGNALED(status: ::c_int) -> bool {
        ((status & 0x7f) + 1) as i8 >= 2
    }

    pub fn WTERMSIG(status: ::c_int) -> ::c_int {
        status & 0x7f
    }

    pub fn WIFEXITED(status: ::c_int) -> bool {
        (status & 0x7f) == 0
    }

    pub fn WEXITSTATUS(status: ::c_int) -> ::c_int {
        (status >> 8) & 0xff
    }

    pub fn WCOREDUMP(status: ::c_int) -> bool {
        (status & 0x80) != 0
    }

    pub fn CPU_ZERO(cpuset: &mut cpu_set_t) -> () {
        for slot in cpuset.bits.iter_mut() {
            *slot = 0;
        }
    }

    pub fn CPU_SET(cpu: usize, cpuset: &mut cpu_set_t) -> () {
        let size_in_bits
            = 8 * ::mem::size_of_val(&cpuset.bits[0]); // 32, 64 etc
        let (idx, offset) = (cpu / size_in_bits, cpu % size_in_bits);
        cpuset.bits[idx] |= 1 << offset;
        ()
    }

    pub fn CPU_CLR(cpu: usize, cpuset: &mut cpu_set_t) -> () {
        let size_in_bits
            = 8 * ::mem::size_of_val(&cpuset.bits[0]); // 32, 64 etc
        let (idx, offset) = (cpu / size_in_bits, cpu % size_in_bits);
        cpuset.bits[idx] &= !(1 << offset);
        ()
    }

    pub fn CPU_ISSET(cpu: usize, cpuset: &cpu_set_t) -> bool {
        let size_in_bits = 8 * ::mem::size_of_val(&cpuset.bits[0]);
        let (idx, offset) = (cpu / size_in_bits, cpu % size_in_bits);
        0 != (cpuset.bits[idx] & (1 << offset))
    }

    pub fn CPU_EQUAL(set1: &cpu_set_t, set2: &cpu_set_t) -> bool {
        set1.bits == set2.bits
    }

    pub fn QCMD(cmd: ::c_int, type_: ::c_int) -> ::c_int {
        (cmd << 8) | (type_ & 0x00ff)
    }
}

extern "C" {
    #[cfg_attr(target_os = "linux", link_name = "__xpg_strerror_r")]
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

    pub fn fdatasync(fd: ::c_int) -> ::c_int;
    pub fn gettimeofday(tp: *mut ::timeval, tz: *mut ::timezone) -> ::c_int;
    pub fn mincore(
        addr: *mut ::c_void,
        len: ::size_t,
        vec: *mut ::c_uchar,
    ) -> ::c_int;
    pub fn clock_getres(clk_id: ::clockid_t, tp: *mut ::timespec) -> ::c_int;
    pub fn clock_gettime(clk_id: ::clockid_t, tp: *mut ::timespec) -> ::c_int;
    pub fn clock_nanosleep(
        clk_id: ::clockid_t,
        flags: ::c_int,
        rqtp: *const ::timespec,
        rmtp: *mut ::timespec,
    ) -> ::c_int;
    pub fn clock_settime(
        clk_id: ::clockid_t,
        tp: *const ::timespec,
    ) -> ::c_int;
    pub fn prctl(option: ::c_int, ...) -> ::c_int;
    pub fn pthread_getattr_np(
        native: ::pthread_t,
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
    pub fn memalign(align: ::size_t, size: ::size_t) -> *mut ::c_void;
    pub fn setgroups(ngroups: ::size_t, ptr: *const ::gid_t) -> ::c_int;
    pub fn initgroups(user: *const ::c_char, group: ::gid_t) -> ::c_int;
    pub fn sched_setscheduler(
        pid: ::pid_t,
        policy: ::c_int,
        param: *const ::sched_param,
    ) -> ::c_int;
    pub fn sched_getscheduler(pid: ::pid_t) -> ::c_int;
    pub fn sched_get_priority_max(policy: ::c_int) -> ::c_int;
    pub fn sched_get_priority_min(policy: ::c_int) -> ::c_int;
    pub fn epoll_create(size: ::c_int) -> ::c_int;
    pub fn epoll_create1(flags: ::c_int) -> ::c_int;
    pub fn epoll_ctl(
        epfd: ::c_int,
        op: ::c_int,
        fd: ::c_int,
        event: *mut ::epoll_event,
    ) -> ::c_int;
    pub fn epoll_wait(
        epfd: ::c_int,
        events: *mut ::epoll_event,
        maxevents: ::c_int,
        timeout: ::c_int,
    ) -> ::c_int;
    pub fn pipe2(fds: *mut ::c_int, flags: ::c_int) -> ::c_int;
    pub fn mount(
        src: *const ::c_char,
        target: *const ::c_char,
        fstype: *const ::c_char,
        flags: ::c_ulong,
        data: *const ::c_void,
    ) -> ::c_int;
    pub fn umount(target: *const ::c_char) -> ::c_int;
    pub fn umount2(target: *const ::c_char, flags: ::c_int) -> ::c_int;
    pub fn clone(
        cb: extern "C" fn(*mut ::c_void) -> ::c_int,
        child_stack: *mut ::c_void,
        flags: ::c_int,
        arg: *mut ::c_void,
        ...
    ) -> ::c_int;
    pub fn statfs(path: *const ::c_char, buf: *mut statfs) -> ::c_int;
    pub fn fstatfs(fd: ::c_int, buf: *mut statfs) -> ::c_int;
    pub fn memrchr(
        cx: *const ::c_void,
        c: ::c_int,
        n: ::size_t,
    ) -> *mut ::c_void;
    pub fn syscall(num: ::c_long, ...) -> ::c_long;
    pub fn sendfile(
        out_fd: ::c_int,
        in_fd: ::c_int,
        offset: *mut off_t,
        count: ::size_t,
    ) -> ::ssize_t;
    pub fn splice(
        fd_in: ::c_int,
        off_in: *mut ::loff_t,
        fd_out: ::c_int,
        off_out: *mut ::loff_t,
        len: ::size_t,
        flags: ::c_uint,
    ) -> ::ssize_t;
    pub fn tee(
        fd_in: ::c_int,
        fd_out: ::c_int,
        len: ::size_t,
        flags: ::c_uint,
    ) -> ::ssize_t;
    pub fn vmsplice(
        fd: ::c_int,
        iov: *const ::iovec,
        nr_segs: ::size_t,
        flags: ::c_uint,
    ) -> ::ssize_t;

    pub fn posix_fadvise(
        fd: ::c_int,
        offset: ::off_t,
        len: ::off_t,
        advise: ::c_int,
    ) -> ::c_int;
    pub fn getrlimit(resource: ::c_int, rlim: *mut ::rlimit) -> ::c_int;
    pub fn setrlimit(resource: ::c_int, rlim: *const ::rlimit) -> ::c_int;
    pub fn futimens(fd: ::c_int, times: *const ::timespec) -> ::c_int;
    pub fn utimensat(
        dirfd: ::c_int,
        path: *const ::c_char,
        times: *const ::timespec,
        flag: ::c_int,
    ) -> ::c_int;
    pub fn duplocale(base: ::locale_t) -> ::locale_t;
    pub fn freelocale(loc: ::locale_t);
    pub fn newlocale(
        mask: ::c_int,
        locale: *const ::c_char,
        base: ::locale_t,
    ) -> ::locale_t;
    pub fn uselocale(loc: ::locale_t) -> ::locale_t;
    pub fn creat64(path: *const c_char, mode: mode_t) -> ::c_int;
    pub fn fstat64(fildes: ::c_int, buf: *mut stat64) -> ::c_int;
    pub fn fstatat64(
        fildes: ::c_int,
        path: *const ::c_char,
        buf: *mut stat64,
        flag: ::c_int,
    ) -> ::c_int;
    pub fn ftruncate64(fd: ::c_int, length: off64_t) -> ::c_int;
    pub fn getrlimit64(resource: ::c_int, rlim: *mut rlimit64) -> ::c_int;
    pub fn lseek64(fd: ::c_int, offset: off64_t, whence: ::c_int) -> off64_t;
    pub fn lstat64(path: *const c_char, buf: *mut stat64) -> ::c_int;
    pub fn mmap64(
        addr: *mut ::c_void,
        len: ::size_t,
        prot: ::c_int,
        flags: ::c_int,
        fd: ::c_int,
        offset: off64_t,
    ) -> *mut ::c_void;
    pub fn open64(path: *const c_char, oflag: ::c_int, ...) -> ::c_int;
    pub fn openat64(
        fd: ::c_int,
        path: *const c_char,
        oflag: ::c_int,
        ...
    ) -> ::c_int;
    pub fn pread64(
        fd: ::c_int,
        buf: *mut ::c_void,
        count: ::size_t,
        offset: off64_t,
    ) -> ::ssize_t;
    pub fn pwrite64(
        fd: ::c_int,
        buf: *const ::c_void,
        count: ::size_t,
        offset: off64_t,
    ) -> ::ssize_t;
    pub fn readdir64(dirp: *mut ::DIR) -> *mut ::dirent64;
    pub fn readdir64_r(
        dirp: *mut ::DIR,
        entry: *mut ::dirent64,
        result: *mut *mut ::dirent64,
    ) -> ::c_int;
    pub fn setrlimit64(resource: ::c_int, rlim: *const rlimit64) -> ::c_int;
    pub fn stat64(path: *const c_char, buf: *mut stat64) -> ::c_int;
    pub fn truncate64(path: *const c_char, length: off64_t) -> ::c_int;
    pub fn eventfd(init: ::c_uint, flags: ::c_int) -> ::c_int;

    pub fn mknodat(
        dirfd: ::c_int,
        pathname: *const ::c_char,
        mode: ::mode_t,
        dev: dev_t,
    ) -> ::c_int;
    pub fn ppoll(
        fds: *mut ::pollfd,
        nfds: nfds_t,
        timeout: *const ::timespec,
        sigmask: *const sigset_t,
    ) -> ::c_int;
    pub fn pthread_condattr_getclock(
        attr: *const pthread_condattr_t,
        clock_id: *mut clockid_t,
    ) -> ::c_int;
    pub fn pthread_condattr_setclock(
        attr: *mut pthread_condattr_t,
        clock_id: ::clockid_t,
    ) -> ::c_int;
    pub fn pthread_condattr_setpshared(
        attr: *mut pthread_condattr_t,
        pshared: ::c_int,
    ) -> ::c_int;
    pub fn pthread_condattr_getpshared(
        attr: *const pthread_condattr_t,
        pshared: *mut ::c_int,
    ) -> ::c_int;
    pub fn sched_getaffinity(
        pid: ::pid_t,
        cpusetsize: ::size_t,
        cpuset: *mut cpu_set_t,
    ) -> ::c_int;
    pub fn sched_setaffinity(
        pid: ::pid_t,
        cpusetsize: ::size_t,
        cpuset: *const cpu_set_t,
    ) -> ::c_int;
    pub fn unshare(flags: ::c_int) -> ::c_int;
    pub fn sem_timedwait(
        sem: *mut sem_t,
        abstime: *const ::timespec,
    ) -> ::c_int;
    pub fn sem_getvalue(sem: *mut sem_t, sval: *mut ::c_int) -> ::c_int;
    pub fn accept4(
        fd: ::c_int,
        addr: *mut ::sockaddr,
        len: *mut ::socklen_t,
        flg: ::c_int,
    ) -> ::c_int;
    pub fn pthread_mutex_timedlock(
        lock: *mut pthread_mutex_t,
        abstime: *const ::timespec,
    ) -> ::c_int;
    pub fn pthread_mutexattr_setpshared(
        attr: *mut pthread_mutexattr_t,
        pshared: ::c_int,
    ) -> ::c_int;
    pub fn pthread_mutexattr_getpshared(
        attr: *const pthread_mutexattr_t,
        pshared: *mut ::c_int,
    ) -> ::c_int;
    pub fn pthread_rwlockattr_getkind_np(
        attr: *const pthread_rwlockattr_t,
        val: *mut ::c_int,
    ) -> ::c_int;
    pub fn pthread_rwlockattr_setkind_np(
        attr: *mut pthread_rwlockattr_t,
        val: ::c_int,
    ) -> ::c_int;
    pub fn pthread_rwlockattr_getpshared(
        attr: *const pthread_rwlockattr_t,
        val: *mut ::c_int,
    ) -> ::c_int;
    pub fn pthread_rwlockattr_setpshared(
        attr: *mut pthread_rwlockattr_t,
        val: ::c_int,
    ) -> ::c_int;
    pub fn ptsname_r(
        fd: ::c_int,
        buf: *mut ::c_char,
        buflen: ::size_t,
    ) -> ::c_int;
    pub fn clearenv() -> ::c_int;
    pub fn waitid(
        idtype: idtype_t,
        id: id_t,
        infop: *mut ::siginfo_t,
        options: ::c_int,
    ) -> ::c_int;

    pub fn lutimes(file: *const ::c_char, times: *const ::timeval) -> ::c_int;

    pub fn setpwent();
    pub fn endpwent();
    pub fn getpwent() -> *mut passwd;
    pub fn setspent();
    pub fn endspent();
    pub fn getspent() -> *mut spwd;
    pub fn getspnam(__name: *const ::c_char) -> *mut spwd;

    pub fn shm_open(
        name: *const c_char,
        oflag: ::c_int,
        mode: mode_t,
    ) -> ::c_int;

    // System V IPC
    pub fn shmget(key: ::key_t, size: ::size_t, shmflg: ::c_int) -> ::c_int;
    pub fn shmat(
        shmid: ::c_int,
        shmaddr: *const ::c_void,
        shmflg: ::c_int,
    ) -> *mut ::c_void;
    pub fn shmdt(shmaddr: *const ::c_void) -> ::c_int;
    pub fn shmctl(
        shmid: ::c_int,
        cmd: ::c_int,
        buf: *mut ::shmid_ds,
    ) -> ::c_int;
    pub fn ftok(pathname: *const ::c_char, proj_id: ::c_int) -> ::key_t;
    pub fn msgctl(msqid: ::c_int, cmd: ::c_int, buf: *mut msqid_ds)
        -> ::c_int;
    pub fn msgget(key: ::key_t, msgflg: ::c_int) -> ::c_int;
    pub fn msgrcv(
        msqid: ::c_int,
        msgp: *mut ::c_void,
        msgsz: ::size_t,
        msgtyp: ::c_long,
        msgflg: ::c_int,
    ) -> ::ssize_t;
    pub fn msgsnd(
        msqid: ::c_int,
        msgp: *const ::c_void,
        msgsz: ::size_t,
        msgflg: ::c_int,
    ) -> ::c_int;

    pub fn mprotect(
        addr: *mut ::c_void,
        len: ::size_t,
        prot: ::c_int,
    ) -> ::c_int;
    pub fn __errno_location() -> *mut ::c_int;

    pub fn fopen64(
        filename: *const c_char,
        mode: *const c_char,
    ) -> *mut ::FILE;
    pub fn freopen64(
        filename: *const c_char,
        mode: *const c_char,
        file: *mut ::FILE,
    ) -> *mut ::FILE;
    pub fn tmpfile64() -> *mut ::FILE;
    pub fn fgetpos64(stream: *mut ::FILE, ptr: *mut fpos64_t) -> ::c_int;
    pub fn fsetpos64(stream: *mut ::FILE, ptr: *const fpos64_t) -> ::c_int;
    pub fn fseeko64(
        stream: *mut ::FILE,
        offset: ::off64_t,
        whence: ::c_int,
    ) -> ::c_int;
    pub fn ftello64(stream: *mut ::FILE) -> ::off64_t;
    pub fn readahead(
        fd: ::c_int,
        offset: ::off64_t,
        count: ::size_t,
    ) -> ::ssize_t;
    pub fn getxattr(
        path: *const c_char,
        name: *const c_char,
        value: *mut ::c_void,
        size: ::size_t,
    ) -> ::ssize_t;
    pub fn lgetxattr(
        path: *const c_char,
        name: *const c_char,
        value: *mut ::c_void,
        size: ::size_t,
    ) -> ::ssize_t;
    pub fn fgetxattr(
        filedes: ::c_int,
        name: *const c_char,
        value: *mut ::c_void,
        size: ::size_t,
    ) -> ::ssize_t;
    pub fn setxattr(
        path: *const c_char,
        name: *const c_char,
        value: *const ::c_void,
        size: ::size_t,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn lsetxattr(
        path: *const c_char,
        name: *const c_char,
        value: *const ::c_void,
        size: ::size_t,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn fsetxattr(
        filedes: ::c_int,
        name: *const c_char,
        value: *const ::c_void,
        size: ::size_t,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn listxattr(
        path: *const c_char,
        list: *mut c_char,
        size: ::size_t,
    ) -> ::ssize_t;
    pub fn llistxattr(
        path: *const c_char,
        list: *mut c_char,
        size: ::size_t,
    ) -> ::ssize_t;
    pub fn flistxattr(
        filedes: ::c_int,
        list: *mut c_char,
        size: ::size_t,
    ) -> ::ssize_t;
    pub fn removexattr(path: *const c_char, name: *const c_char) -> ::c_int;
    pub fn lremovexattr(path: *const c_char, name: *const c_char) -> ::c_int;
    pub fn fremovexattr(filedes: ::c_int, name: *const c_char) -> ::c_int;
    pub fn signalfd(
        fd: ::c_int,
        mask: *const ::sigset_t,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn quotactl(
        cmd: ::c_int,
        special: *const ::c_char,
        id: ::c_int,
        data: *mut ::c_char,
    ) -> ::c_int;
    pub fn mq_open(name: *const ::c_char, oflag: ::c_int, ...) -> ::mqd_t;
    pub fn mq_close(mqd: ::mqd_t) -> ::c_int;
    pub fn mq_unlink(name: *const ::c_char) -> ::c_int;
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
    pub fn mq_getattr(mqd: ::mqd_t, attr: *mut ::mq_attr) -> ::c_int;
    pub fn mq_setattr(
        mqd: ::mqd_t,
        newattr: *const ::mq_attr,
        oldattr: *mut ::mq_attr,
    ) -> ::c_int;
    pub fn epoll_pwait(
        epfd: ::c_int,
        events: *mut ::epoll_event,
        maxevents: ::c_int,
        timeout: ::c_int,
        sigmask: *const ::sigset_t,
    ) -> ::c_int;
    pub fn sethostname(name: *const ::c_char, len: ::size_t) -> ::c_int;
    pub fn sigtimedwait(
        set: *const sigset_t,
        info: *mut siginfo_t,
        timeout: *const ::timespec,
    ) -> ::c_int;
    pub fn sigwaitinfo(set: *const sigset_t, info: *mut siginfo_t) -> ::c_int;
    pub fn nl_langinfo_l(item: ::nl_item, locale: ::locale_t)
        -> *mut ::c_char;
    pub fn prlimit(
        pid: ::pid_t,
        resource: ::c_int,
        new_limit: *const ::rlimit,
        old_limit: *mut ::rlimit,
    ) -> ::c_int;
    pub fn prlimit64(
        pid: ::pid_t,
        resource: ::c_int,
        new_limit: *const ::rlimit64,
        old_limit: *mut ::rlimit64,
    ) -> ::c_int;
    pub fn reboot(how_to: ::c_int) -> ::c_int;
    pub fn setfsgid(gid: ::gid_t) -> ::c_int;
    pub fn setfsuid(uid: ::uid_t) -> ::c_int;
    pub fn setresgid(rgid: ::gid_t, egid: ::gid_t, sgid: ::gid_t) -> ::c_int;
    pub fn setresuid(ruid: ::uid_t, euid: ::uid_t, suid: ::uid_t) -> ::c_int;

    // Not available now on Android
    pub fn mkfifoat(
        dirfd: ::c_int,
        pathname: *const ::c_char,
        mode: ::mode_t,
    ) -> ::c_int;
    pub fn if_nameindex() -> *mut if_nameindex;
    pub fn if_freenameindex(ptr: *mut if_nameindex);
    pub fn sync_file_range(
        fd: ::c_int,
        offset: ::off64_t,
        nbytes: ::off64_t,
        flags: ::c_uint,
    ) -> ::c_int;
    pub fn getifaddrs(ifap: *mut *mut ::ifaddrs) -> ::c_int;
    pub fn freeifaddrs(ifa: *mut ::ifaddrs);

    pub fn mremap(
        addr: *mut ::c_void,
        len: ::size_t,
        new_len: ::size_t,
        flags: ::c_int,
        ...
    ) -> *mut ::c_void;

    pub fn glob(
        pattern: *const c_char,
        flags: ::c_int,
        errfunc: ::Option<
            extern "C" fn(epath: *const c_char, errno: ::c_int) -> ::c_int,
        >,
        pglob: *mut ::glob_t,
    ) -> ::c_int;
    pub fn globfree(pglob: *mut ::glob_t);

    pub fn shm_unlink(name: *const ::c_char) -> ::c_int;

    pub fn seekdir(dirp: *mut ::DIR, loc: ::c_long);

    pub fn dirfd(dirp: *mut ::DIR) -> ::c_int;

    pub fn telldir(dirp: *mut ::DIR) -> ::c_long;
    pub fn madvise(
        addr: *mut ::c_void,
        len: ::size_t,
        advice: ::c_int,
    ) -> ::c_int;

    pub fn msync(
        addr: *mut ::c_void,
        len: ::size_t,
        flags: ::c_int,
    ) -> ::c_int;

    pub fn recvfrom(
        socket: ::c_int,
        buf: *mut ::c_void,
        len: ::size_t,
        flags: ::c_int,
        addr: *mut ::sockaddr,
        addrlen: *mut ::socklen_t,
    ) -> ::ssize_t;
    pub fn nl_langinfo(item: ::nl_item) -> *mut ::c_char;

    pub fn bind(
        socket: ::c_int,
        address: *const ::sockaddr,
        address_len: ::socklen_t,
    ) -> ::c_int;

    pub fn writev(
        fd: ::c_int,
        iov: *const ::iovec,
        iovcnt: ::c_int,
    ) -> ::ssize_t;
    pub fn readv(
        fd: ::c_int,
        iov: *const ::iovec,
        iovcnt: ::c_int,
    ) -> ::ssize_t;

    pub fn sendmsg(
        fd: ::c_int,
        msg: *const ::msghdr,
        flags: ::c_int,
    ) -> ::ssize_t;
    pub fn recvmsg(
        fd: ::c_int,
        msg: *mut ::msghdr,
        flags: ::c_int,
    ) -> ::ssize_t;
    #[cfg_attr(target_os = "solaris", link_name = "__posix_getgrgid_r")]
    pub fn getgrgid_r(
        gid: ::gid_t,
        grp: *mut ::group,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut ::group,
    ) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "sigaltstack$UNIX2003"
    )]
    #[cfg_attr(target_os = "netbsd", link_name = "__sigaltstack14")]
    pub fn sigaltstack(ss: *const stack_t, oss: *mut stack_t) -> ::c_int;
    pub fn sem_close(sem: *mut sem_t) -> ::c_int;
    pub fn getdtablesize() -> ::c_int;
    #[cfg_attr(target_os = "solaris", link_name = "__posix_getgrnam_r")]
    pub fn getgrnam_r(
        name: *const ::c_char,
        grp: *mut ::group,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut ::group,
    ) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "pthread_sigmask$UNIX2003"
    )]
    pub fn pthread_sigmask(
        how: ::c_int,
        set: *const sigset_t,
        oldset: *mut sigset_t,
    ) -> ::c_int;
    pub fn sem_open(name: *const ::c_char, oflag: ::c_int, ...) -> *mut sem_t;
    pub fn getgrnam(name: *const ::c_char) -> *mut ::group;
    pub fn pthread_kill(thread: ::pthread_t, sig: ::c_int) -> ::c_int;
    pub fn sem_unlink(name: *const ::c_char) -> ::c_int;
    pub fn daemon(nochdir: ::c_int, noclose: ::c_int) -> ::c_int;
    #[cfg_attr(target_os = "netbsd", link_name = "__getpwnam_r50")]
    #[cfg_attr(target_os = "solaris", link_name = "__posix_getpwnam_r")]
    pub fn getpwnam_r(
        name: *const ::c_char,
        pwd: *mut passwd,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut passwd,
    ) -> ::c_int;
    #[cfg_attr(target_os = "netbsd", link_name = "__getpwuid_r50")]
    #[cfg_attr(target_os = "solaris", link_name = "__posix_getpwuid_r")]
    pub fn getpwuid_r(
        uid: ::uid_t,
        pwd: *mut passwd,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut passwd,
    ) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "sigwait$UNIX2003"
    )]
    #[cfg_attr(target_os = "solaris", link_name = "__posix_sigwait")]
    pub fn sigwait(set: *const sigset_t, sig: *mut ::c_int) -> ::c_int;
    pub fn pthread_atfork(
        prepare: ::Option<unsafe extern "C" fn()>,
        parent: ::Option<unsafe extern "C" fn()>,
        child: ::Option<unsafe extern "C" fn()>,
    ) -> ::c_int;
    pub fn pthread_create(
        native: *mut ::pthread_t,
        attr: *const ::pthread_attr_t,
        f: extern "C" fn(*mut ::c_void) -> *mut ::c_void,
        value: *mut ::c_void,
    ) -> ::c_int;
    pub fn getgrgid(gid: ::gid_t) -> *mut ::group;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "popen$UNIX2003"
    )]
    pub fn popen(command: *const c_char, mode: *const c_char) -> *mut ::FILE;
    pub fn uname(buf: *mut ::utsname) -> ::c_int;
}

cfg_if! {
    if #[cfg(any(target_arch = "mips", target_arch = "mips64"))] {
        mod mips;
        pub use self::mips::*;
    } else if #[cfg(target_arch = "x86_64")] {
        mod x86_64;
        pub use self::x86_64::*;
    } else if #[cfg(target_arch = "arm")] {
        mod arm;
        pub use self::arm::*;
    } else {
        pub use unsupported_target;
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
