pub type c_char = i8;
pub type c_long = i64;
pub type c_ulong = u64;
pub type wchar_t = i32;

pub type blkcnt_t = ::c_ulong;
pub type blksize_t = ::c_long;
pub type clock_t = ::c_long;
pub type clockid_t = ::c_int;
pub type dev_t = ::c_long;
pub type fsblkcnt_t = ::c_ulong;
pub type fsfilcnt_t = ::c_ulong;
pub type ino_t = ::c_ulong;
pub type mode_t = ::c_int;
pub type nfds_t = ::c_ulong;
pub type nlink_t = ::c_ulong;
pub type off_t = ::c_long;
pub type pthread_t = *mut ::c_void;
pub type pthread_attr_t = *mut ::c_void;
pub type pthread_cond_t = *mut ::c_void;
pub type pthread_condattr_t = *mut ::c_void;
// Must be usize due to libstd/sys_common/thread_local.rs,
// should technically be *mut ::c_void
pub type pthread_key_t = usize;
pub type pthread_mutex_t = *mut ::c_void;
pub type pthread_mutexattr_t = *mut ::c_void;
pub type pthread_rwlock_t = *mut ::c_void;
pub type pthread_rwlockattr_t = *mut ::c_void;
pub type rlim_t = ::c_ulonglong;
pub type sa_family_t = u16;
pub type sem_t = *mut ::c_void;
pub type sigset_t = ::c_ulong;
pub type socklen_t = u32;
pub type speed_t = u32;
pub type suseconds_t = ::c_int;
pub type tcflag_t = u32;
pub type time_t = ::c_long;

#[cfg_attr(feature = "extra_traits", derive(Debug))]
pub enum timezone {}
impl ::Copy for timezone {}
impl ::Clone for timezone {
    fn clone(&self) -> timezone {
        *self
    }
}

s_no_extra_traits! {
    #[repr(C)]
    pub struct utsname {
        pub sysname: [::c_char; UTSLENGTH],
        pub nodename: [::c_char; UTSLENGTH],
        pub release: [::c_char; UTSLENGTH],
        pub version: [::c_char; UTSLENGTH],
        pub machine: [::c_char; UTSLENGTH],
        pub domainname: [::c_char; UTSLENGTH],
    }

    pub struct dirent {
        pub d_ino: ::ino_t,
        pub d_off: ::off_t,
        pub d_reclen: ::c_ushort,
        pub d_type: ::c_uchar,
        pub d_name: [::c_char; 256],
    }

    pub struct sockaddr_un {
        pub sun_family: ::sa_family_t,
        pub sun_path: [::c_char; 108]
    }

    pub struct sockaddr_storage {
        pub ss_family: ::sa_family_t,
        __ss_padding: [
            u8;
            128 -
            ::core::mem::size_of::<sa_family_t>() -
            ::core::mem::size_of::<c_ulong>()
        ],
        __ss_align: ::c_ulong,
    }
}

s! {
    pub struct addrinfo {
        pub ai_flags: ::c_int,
        pub ai_family: ::c_int,
        pub ai_socktype: ::c_int,
        pub ai_protocol: ::c_int,
        pub ai_addrlen: ::size_t,
        pub ai_canonname: *mut ::c_char,
        pub ai_addr: *mut ::sockaddr,
        pub ai_next: *mut ::addrinfo,
    }

    pub struct Dl_info {
        pub dli_fname: *const ::c_char,
        pub dli_fbase: *mut ::c_void,
        pub dli_sname: *const ::c_char,
        pub dli_saddr: *mut ::c_void,
    }

    pub struct epoll_event {
        pub events: u32,
        pub u64: u64,
        pub _pad: u64,
    }

    pub struct fd_set {
        fds_bits: [::c_ulong; ::FD_SETSIZE / ULONG_SIZE],
    }

    pub struct in_addr {
        pub s_addr: ::in_addr_t,
    }

    pub struct ip_mreq {
        pub imr_multiaddr: ::in_addr,
        pub imr_interface: ::in_addr,
    }

    pub struct lconv {
        pub currency_symbol: *const ::c_char,
        pub decimal_point: *const ::c_char,
        pub frac_digits: ::c_char,
        pub grouping: *const ::c_char,
        pub int_curr_symbol: *const ::c_char,
        pub int_frac_digits: ::c_char,
        pub mon_decimal_point: *const ::c_char,
        pub mon_grouping: *const ::c_char,
        pub mon_thousands_sep: *const ::c_char,
        pub negative_sign: *const ::c_char,
        pub n_cs_precedes: ::c_char,
        pub n_sep_by_space: ::c_char,
        pub n_sign_posn: ::c_char,
        pub positive_sign: *const ::c_char,
        pub p_cs_precedes: ::c_char,
        pub p_sep_by_space: ::c_char,
        pub p_sign_posn: ::c_char,
        pub thousands_sep: *const ::c_char,
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

    pub struct sigaction {
        pub sa_handler: ::sighandler_t,
        pub sa_flags: ::c_ulong,
        pub sa_restorer: ::Option<extern fn()>,
        pub sa_mask: ::sigset_t,
    }

    pub struct sockaddr {
        pub sa_family: ::sa_family_t,
        pub sa_data: [::c_char; 14],
    }

    pub struct sockaddr_in {
        pub sin_family: ::sa_family_t,
        pub sin_port: ::in_port_t,
        pub sin_addr: ::in_addr,
        pub sin_zero: [::c_char; 8],
    }

    pub struct sockaddr_in6 {
        pub sin6_family: ::sa_family_t,
        pub sin6_port: ::in_port_t,
        pub sin6_flowinfo: u32,
        pub sin6_addr: ::in6_addr,
        pub sin6_scope_id: u32,
    }

    pub struct stat {
        pub st_dev: ::dev_t,
        pub st_ino: ::ino_t,
        pub st_nlink: ::nlink_t,
        pub st_mode: ::mode_t,
        pub st_uid: ::uid_t,
        pub st_gid: ::gid_t,
        pub st_rdev: ::dev_t,
        pub st_size: ::off_t,
        pub st_blksize: ::blksize_t,
        pub st_blocks: ::blkcnt_t,
        pub st_atime: ::time_t,
        pub st_atime_nsec: ::c_long,
        pub st_mtime: ::time_t,
        pub st_mtime_nsec: ::c_long,
        pub st_ctime: ::time_t,
        pub st_ctime_nsec: ::c_long,
        _pad: [::c_char; 24],
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
        pub f_fsid: ::c_ulong,
        pub f_flag: ::c_ulong,
        pub f_namemax: ::c_ulong,
    }

    pub struct termios {
        pub c_iflag: ::tcflag_t,
        pub c_oflag: ::tcflag_t,
        pub c_cflag: ::tcflag_t,
        pub c_lflag: ::tcflag_t,
        pub c_line: ::cc_t,
        pub c_cc: [::cc_t; ::NCCS],
        pub c_ispeed: ::speed_t,
        pub c_ospeed: ::speed_t,
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
}

pub const UTSLENGTH: usize = 65;

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

// limits.h
pub const PATH_MAX: ::c_int = 4096;

// fcntl.h
pub const F_GETLK: ::c_int = 5;
pub const F_SETLK: ::c_int = 6;
pub const F_SETLKW: ::c_int = 7;

// TODO: relibc {
pub const RTLD_DEFAULT: *mut ::c_void = 0i64 as *mut ::c_void;
// }

// dlfcn.h
pub const RTLD_LAZY: ::c_int = 0x0001;
pub const RTLD_NOW: ::c_int = 0x0002;
pub const RTLD_GLOBAL: ::c_int = 0x0100;
pub const RTLD_LOCAL: ::c_int = 0x0000;

// errno.h
pub const EPERM: ::c_int = 1; /* Operation not permitted */
pub const ENOENT: ::c_int = 2; /* No such file or directory */
pub const ESRCH: ::c_int = 3; /* No such process */
pub const EINTR: ::c_int = 4; /* Interrupted system call */
pub const EIO: ::c_int = 5; /* I/O error */
pub const ENXIO: ::c_int = 6; /* No such device or address */
pub const E2BIG: ::c_int = 7; /* Argument list too long */
pub const ENOEXEC: ::c_int = 8; /* Exec format error */
pub const EBADF: ::c_int = 9; /* Bad file number */
pub const ECHILD: ::c_int = 10; /* No child processes */
pub const EAGAIN: ::c_int = 11; /* Try again */
pub const ENOMEM: ::c_int = 12; /* Out of memory */
pub const EACCES: ::c_int = 13; /* Permission denied */
pub const EFAULT: ::c_int = 14; /* Bad address */
pub const ENOTBLK: ::c_int = 15; /* Block device required */
pub const EBUSY: ::c_int = 16; /* Device or resource busy */
pub const EEXIST: ::c_int = 17; /* File exists */
pub const EXDEV: ::c_int = 18; /* Cross-device link */
pub const ENODEV: ::c_int = 19; /* No such device */
pub const ENOTDIR: ::c_int = 20; /* Not a directory */
pub const EISDIR: ::c_int = 21; /* Is a directory */
pub const EINVAL: ::c_int = 22; /* Invalid argument */
pub const ENFILE: ::c_int = 23; /* File table overflow */
pub const EMFILE: ::c_int = 24; /* Too many open files */
pub const ENOTTY: ::c_int = 25; /* Not a typewriter */
pub const ETXTBSY: ::c_int = 26; /* Text file busy */
pub const EFBIG: ::c_int = 27; /* File too large */
pub const ENOSPC: ::c_int = 28; /* No space left on device */
pub const ESPIPE: ::c_int = 29; /* Illegal seek */
pub const EROFS: ::c_int = 30; /* Read-only file system */
pub const EMLINK: ::c_int = 31; /* Too many links */
pub const EPIPE: ::c_int = 32; /* Broken pipe */
pub const EDOM: ::c_int = 33; /* Math argument out of domain of func */
pub const ERANGE: ::c_int = 34; /* Math result not representable */
pub const EDEADLK: ::c_int = 35; /* Resource deadlock would occur */
pub const ENAMETOOLONG: ::c_int = 36; /* File name too long */
pub const ENOLCK: ::c_int = 37; /* No record locks available */
pub const ENOSYS: ::c_int = 38; /* Function not implemented */
pub const ENOTEMPTY: ::c_int = 39; /* Directory not empty */
pub const ELOOP: ::c_int = 40; /* Too many symbolic links encountered */
pub const EWOULDBLOCK: ::c_int = 41; /* Operation would block */
pub const ENOMSG: ::c_int = 42; /* No message of desired type */
pub const EIDRM: ::c_int = 43; /* Identifier removed */
pub const ECHRNG: ::c_int = 44; /* Channel number out of range */
pub const EL2NSYNC: ::c_int = 45; /* Level 2 not synchronized */
pub const EL3HLT: ::c_int = 46; /* Level 3 halted */
pub const EL3RST: ::c_int = 47; /* Level 3 reset */
pub const ELNRNG: ::c_int = 48; /* Link number out of range */
pub const EUNATCH: ::c_int = 49; /* Protocol driver not attached */
pub const ENOCSI: ::c_int = 50; /* No CSI structure available */
pub const EL2HLT: ::c_int = 51; /* Level 2 halted */
pub const EBADE: ::c_int = 52; /* Invalid exchange */
pub const EBADR: ::c_int = 53; /* Invalid request descriptor */
pub const EXFULL: ::c_int = 54; /* Exchange full */
pub const ENOANO: ::c_int = 55; /* No anode */
pub const EBADRQC: ::c_int = 56; /* Invalid request code */
pub const EBADSLT: ::c_int = 57; /* Invalid slot */
pub const EDEADLOCK: ::c_int = 58; /* Resource deadlock would occur */
pub const EBFONT: ::c_int = 59; /* Bad font file format */
pub const ENOSTR: ::c_int = 60; /* Device not a stream */
pub const ENODATA: ::c_int = 61; /* No data available */
pub const ETIME: ::c_int = 62; /* Timer expired */
pub const ENOSR: ::c_int = 63; /* Out of streams resources */
pub const ENONET: ::c_int = 64; /* Machine is not on the network */
pub const ENOPKG: ::c_int = 65; /* Package not installed */
pub const EREMOTE: ::c_int = 66; /* Object is remote */
pub const ENOLINK: ::c_int = 67; /* Link has been severed */
pub const EADV: ::c_int = 68; /* Advertise error */
pub const ESRMNT: ::c_int = 69; /* Srmount error */
pub const ECOMM: ::c_int = 70; /* Communication error on send */
pub const EPROTO: ::c_int = 71; /* Protocol error */
pub const EMULTIHOP: ::c_int = 72; /* Multihop attempted */
pub const EDOTDOT: ::c_int = 73; /* RFS specific error */
pub const EBADMSG: ::c_int = 74; /* Not a data message */
pub const EOVERFLOW: ::c_int = 75; /* Value too large for defined data type */
pub const ENOTUNIQ: ::c_int = 76; /* Name not unique on network */
pub const EBADFD: ::c_int = 77; /* File descriptor in bad state */
pub const EREMCHG: ::c_int = 78; /* Remote address changed */
pub const ELIBACC: ::c_int = 79; /* Can not access a needed shared library */
pub const ELIBBAD: ::c_int = 80; /* Accessing a corrupted shared library */
pub const ELIBSCN: ::c_int = 81; /* .lib section in a.out corrupted */
/* Attempting to link in too many shared libraries */
pub const ELIBMAX: ::c_int = 82;
pub const ELIBEXEC: ::c_int = 83; /* Cannot exec a shared library directly */
pub const EILSEQ: ::c_int = 84; /* Illegal byte sequence */
/* Interrupted system call should be restarted */
pub const ERESTART: ::c_int = 85;
pub const ESTRPIPE: ::c_int = 86; /* Streams pipe error */
pub const EUSERS: ::c_int = 87; /* Too many users */
pub const ENOTSOCK: ::c_int = 88; /* Socket operation on non-socket */
pub const EDESTADDRREQ: ::c_int = 89; /* Destination address required */
pub const EMSGSIZE: ::c_int = 90; /* Message too long */
pub const EPROTOTYPE: ::c_int = 91; /* Protocol wrong type for socket */
pub const ENOPROTOOPT: ::c_int = 92; /* Protocol not available */
pub const EPROTONOSUPPORT: ::c_int = 93; /* Protocol not supported */
pub const ESOCKTNOSUPPORT: ::c_int = 94; /* Socket type not supported */
/* Operation not supported on transport endpoint */
pub const EOPNOTSUPP: ::c_int = 95;
pub const EPFNOSUPPORT: ::c_int = 96; /* Protocol family not supported */
/* Address family not supported by protocol */
pub const EAFNOSUPPORT: ::c_int = 97;
pub const EADDRINUSE: ::c_int = 98; /* Address already in use */
pub const EADDRNOTAVAIL: ::c_int = 99; /* Cannot assign requested address */
pub const ENETDOWN: ::c_int = 100; /* Network is down */
pub const ENETUNREACH: ::c_int = 101; /* Network is unreachable */
/* Network dropped connection because of reset */
pub const ENETRESET: ::c_int = 102;
pub const ECONNABORTED: ::c_int = 103; /* Software caused connection abort */
pub const ECONNRESET: ::c_int = 104; /* Connection reset by peer */
pub const ENOBUFS: ::c_int = 105; /* No buffer space available */
pub const EISCONN: ::c_int = 106; /* Transport endpoint is already connected */
pub const ENOTCONN: ::c_int = 107; /* Transport endpoint is not connected */
/* Cannot send after transport endpoint shutdown */
pub const ESHUTDOWN: ::c_int = 108;
pub const ETOOMANYREFS: ::c_int = 109; /* Too many references: cannot splice */
pub const ETIMEDOUT: ::c_int = 110; /* Connection timed out */
pub const ECONNREFUSED: ::c_int = 111; /* Connection refused */
pub const EHOSTDOWN: ::c_int = 112; /* Host is down */
pub const EHOSTUNREACH: ::c_int = 113; /* No route to host */
pub const EALREADY: ::c_int = 114; /* Operation already in progress */
pub const EINPROGRESS: ::c_int = 115; /* Operation now in progress */
pub const ESTALE: ::c_int = 116; /* Stale NFS file handle */
pub const EUCLEAN: ::c_int = 117; /* Structure needs cleaning */
pub const ENOTNAM: ::c_int = 118; /* Not a XENIX named type file */
pub const ENAVAIL: ::c_int = 119; /* No XENIX semaphores available */
pub const EISNAM: ::c_int = 120; /* Is a named type file */
pub const EREMOTEIO: ::c_int = 121; /* Remote I/O error */
pub const EDQUOT: ::c_int = 122; /* Quota exceeded */
pub const ENOMEDIUM: ::c_int = 123; /* No medium found */
pub const EMEDIUMTYPE: ::c_int = 124; /* Wrong medium type */
pub const ECANCELED: ::c_int = 125; /* Operation Canceled */
pub const ENOKEY: ::c_int = 126; /* Required key not available */
pub const EKEYEXPIRED: ::c_int = 127; /* Key has expired */
pub const EKEYREVOKED: ::c_int = 128; /* Key has been revoked */
pub const EKEYREJECTED: ::c_int = 129; /* Key was rejected by service */
pub const EOWNERDEAD: ::c_int = 130; /* Owner died */
pub const ENOTRECOVERABLE: ::c_int = 131; /* State not recoverable */

// fcntl.h
pub const F_DUPFD: ::c_int = 0;
pub const F_GETFD: ::c_int = 1;
pub const F_SETFD: ::c_int = 2;
pub const F_GETFL: ::c_int = 3;
pub const F_SETFL: ::c_int = 4;
// TODO: relibc {
pub const F_DUPFD_CLOEXEC: ::c_int = ::F_DUPFD;
// }
pub const FD_CLOEXEC: ::c_int = 0x0100_0000;
pub const O_RDONLY: ::c_int = 0x0001_0000;
pub const O_WRONLY: ::c_int = 0x0002_0000;
pub const O_RDWR: ::c_int = 0x0003_0000;
pub const O_ACCMODE: ::c_int = 0x0003_0000;
pub const O_NONBLOCK: ::c_int = 0x0004_0000;
pub const O_APPEND: ::c_int = 0x0008_0000;
pub const O_SHLOCK: ::c_int = 0x0010_0000;
pub const O_EXLOCK: ::c_int = 0x0020_0000;
pub const O_ASYNC: ::c_int = 0x0040_0000;
pub const O_FSYNC: ::c_int = 0x0080_0000;
pub const O_CLOEXEC: ::c_int = 0x0100_0000;
pub const O_CREAT: ::c_int = 0x0200_0000;
pub const O_TRUNC: ::c_int = 0x0400_0000;
pub const O_EXCL: ::c_int = 0x0800_0000;
pub const O_DIRECTORY: ::c_int = 0x1000_0000;
pub const O_PATH: ::c_int = 0x2000_0000;
pub const O_SYMLINK: ::c_int = 0x4000_0000;
// Negative to allow it to be used as int
// TODO: Fix negative values missing from includes
pub const O_NOFOLLOW: ::c_int = -0x8000_0000;

// netdb.h
pub const EAI_SYSTEM: ::c_int = -11;

// netinet/in.h
// TODO: relibc {
pub const IP_TTL: ::c_int = 2;
pub const IPV6_UNICAST_HOPS: ::c_int = 16;
pub const IPV6_MULTICAST_IF: ::c_int = 17;
pub const IPV6_MULTICAST_HOPS: ::c_int = 18;
pub const IPV6_MULTICAST_LOOP: ::c_int = 19;
pub const IPV6_ADD_MEMBERSHIP: ::c_int = 20;
pub const IPV6_DROP_MEMBERSHIP: ::c_int = 21;
pub const IPV6_V6ONLY: ::c_int = 26;
pub const IP_MULTICAST_IF: ::c_int = 32;
pub const IP_MULTICAST_TTL: ::c_int = 33;
pub const IP_MULTICAST_LOOP: ::c_int = 34;
pub const IP_ADD_MEMBERSHIP: ::c_int = 35;
pub const IP_DROP_MEMBERSHIP: ::c_int = 36;
// }

// netinet/tcp.h
pub const TCP_NODELAY: ::c_int = 1;
// TODO: relibc {
pub const TCP_KEEPIDLE: ::c_int = 1;
// }

// poll.h
pub const POLLIN: ::c_short = 0x001;
pub const POLLPRI: ::c_short = 0x002;
pub const POLLOUT: ::c_short = 0x004;
pub const POLLERR: ::c_short = 0x008;
pub const POLLHUP: ::c_short = 0x010;
pub const POLLNVAL: ::c_short = 0x020;

// pthread.h
pub const PTHREAD_MUTEX_NORMAL: ::c_int = 0;
pub const PTHREAD_MUTEX_RECURSIVE: ::c_int = 1;
pub const PTHREAD_MUTEX_INITIALIZER: ::pthread_mutex_t = -1isize as *mut _;
pub const PTHREAD_COND_INITIALIZER: ::pthread_cond_t = -1isize as *mut _;
pub const PTHREAD_RWLOCK_INITIALIZER: ::pthread_rwlock_t = -1isize as *mut _;
pub const PTHREAD_STACK_MIN: ::size_t = 4096;

// signal.h
pub const SIG_BLOCK: ::c_int = 0;
pub const SIG_UNBLOCK: ::c_int = 1;
pub const SIG_SETMASK: ::c_int = 2;
pub const SIGHUP: ::c_int = 1;
pub const SIGINT: ::c_int = 2;
pub const SIGQUIT: ::c_int = 3;
pub const SIGILL: ::c_int = 4;
pub const SIGTRAP: ::c_int = 5;
pub const SIGABRT: ::c_int = 6;
pub const SIGBUS: ::c_int = 7;
pub const SIGFPE: ::c_int = 8;
pub const SIGKILL: ::c_int = 9;
pub const SIGUSR1: ::c_int = 10;
pub const SIGSEGV: ::c_int = 11;
pub const SIGUSR2: ::c_int = 12;
pub const SIGPIPE: ::c_int = 13;
pub const SIGALRM: ::c_int = 14;
pub const SIGTERM: ::c_int = 15;
pub const SIGSTKFLT: ::c_int = 16;
pub const SIGCHLD: ::c_int = 17;
pub const SIGCONT: ::c_int = 18;
pub const SIGSTOP: ::c_int = 19;
pub const SIGTSTP: ::c_int = 20;
pub const SIGTTIN: ::c_int = 21;
pub const SIGTTOU: ::c_int = 22;
pub const SIGURG: ::c_int = 23;
pub const SIGXCPU: ::c_int = 24;
pub const SIGXFSZ: ::c_int = 25;
pub const SIGVTALRM: ::c_int = 26;
pub const SIGPROF: ::c_int = 27;
pub const SIGWINCH: ::c_int = 28;
pub const SIGIO: ::c_int = 29;
pub const SIGPWR: ::c_int = 30;
pub const SIGSYS: ::c_int = 31;
pub const NSIG: ::c_int = 32;

pub const SA_NOCLDSTOP: ::c_ulong = 0x00000001;
pub const SA_NOCLDWAIT: ::c_ulong = 0x00000002;
pub const SA_SIGINFO: ::c_ulong = 0x00000004;
pub const SA_RESTORER: ::c_ulong = 0x04000000;
pub const SA_ONSTACK: ::c_ulong = 0x08000000;
pub const SA_RESTART: ::c_ulong = 0x10000000;
pub const SA_NODEFER: ::c_ulong = 0x40000000;
pub const SA_RESETHAND: ::c_ulong = 0x80000000;

// sys/epoll.h
pub const EPOLL_CLOEXEC: ::c_int = 0x0100_0000;
pub const EPOLL_CTL_ADD: ::c_int = 1;
pub const EPOLL_CTL_DEL: ::c_int = 2;
pub const EPOLL_CTL_MOD: ::c_int = 3;
pub const EPOLLIN: ::c_int = 1;
pub const EPOLLPRI: ::c_int = 0;
pub const EPOLLOUT: ::c_int = 2;
pub const EPOLLRDNORM: ::c_int = 0;
pub const EPOLLNVAL: ::c_int = 0;
pub const EPOLLRDBAND: ::c_int = 0;
pub const EPOLLWRNORM: ::c_int = 0;
pub const EPOLLWRBAND: ::c_int = 0;
pub const EPOLLMSG: ::c_int = 0;
pub const EPOLLERR: ::c_int = 0;
pub const EPOLLHUP: ::c_int = 0;
pub const EPOLLRDHUP: ::c_int = 0;
pub const EPOLLEXCLUSIVE: ::c_int = 0;
pub const EPOLLWAKEUP: ::c_int = 0;
pub const EPOLLONESHOT: ::c_int = 0;
pub const EPOLLET: ::c_int = 0;

// sys/stat.h
pub const S_IFMT: ::c_int = 0o0_170_000;
pub const S_IFDIR: ::c_int = 0o040_000;
pub const S_IFCHR: ::c_int = 0o020_000;
pub const S_IFBLK: ::c_int = 0o060_000;
pub const S_IFREG: ::c_int = 0o100_000;
pub const S_IFIFO: ::c_int = 0o010_000;
pub const S_IFLNK: ::c_int = 0o120_000;
pub const S_IFSOCK: ::c_int = 0o140_000;
pub const S_IRWXU: ::c_int = 0o0_700;
pub const S_IRUSR: ::c_int = 0o0_400;
pub const S_IWUSR: ::c_int = 0o0_200;
pub const S_IXUSR: ::c_int = 0o0_100;
pub const S_IRWXG: ::c_int = 0o0_070;
pub const S_IRGRP: ::c_int = 0o0_040;
pub const S_IWGRP: ::c_int = 0o0_020;
pub const S_IXGRP: ::c_int = 0o0_010;
pub const S_IRWXO: ::c_int = 0o0_007;
pub const S_IROTH: ::c_int = 0o0_004;
pub const S_IWOTH: ::c_int = 0o0_002;
pub const S_IXOTH: ::c_int = 0o0_001;

// stdlib.h
pub const EXIT_SUCCESS: ::c_int = 0;
pub const EXIT_FAILURE: ::c_int = 1;

// sys/ioctl.h
// TODO: relibc {
pub const FIONBIO: ::c_ulong = 0x5421;
pub const FIOCLEX: ::c_ulong = 0x5451;
// }
pub const TCGETS: ::c_ulong = 0x5401;
pub const TCSETS: ::c_ulong = 0x5402;
pub const TCFLSH: ::c_ulong = 0x540B;
pub const TIOCGPGRP: ::c_ulong = 0x540F;
pub const TIOCSPGRP: ::c_ulong = 0x5410;
pub const TIOCGWINSZ: ::c_ulong = 0x5413;
pub const TIOCSWINSZ: ::c_ulong = 0x5414;

// sys/select.h
pub const FD_SETSIZE: usize = 1024;

// sys/socket.h
pub const AF_UNIX: ::c_int = 1;
pub const AF_INET: ::c_int = 2;
pub const AF_INET6: ::c_int = 10;
pub const MSG_PEEK: ::c_int = 2;
pub const SHUT_RD: ::c_int = 0;
pub const SHUT_WR: ::c_int = 1;
pub const SHUT_RDWR: ::c_int = 2;
pub const SO_REUSEADDR: ::c_int = 2;
pub const SO_ERROR: ::c_int = 4;
pub const SO_BROADCAST: ::c_int = 6;
pub const SO_SNDBUF: ::c_int = 7;
pub const SO_RCVBUF: ::c_int = 8;
pub const SO_KEEPALIVE: ::c_int = 9;
pub const SO_LINGER: ::c_int = 13;
pub const SO_REUSEPORT: ::c_int = 15;
pub const SO_RCVTIMEO: ::c_int = 20;
pub const SO_SNDTIMEO: ::c_int = 21;
pub const SOCK_STREAM: ::c_int = 1;
pub const SOCK_DGRAM: ::c_int = 2;
pub const SOL_SOCKET: ::c_int = 1;

// sys/termios.h
pub const NCCS: usize = 32;

pub const VINTR: usize = 0;
pub const VQUIT: usize = 1;
pub const VERASE: usize = 2;
pub const VKILL: usize = 3;
pub const VEOF: usize = 4;
pub const VTIME: usize = 5;
pub const VMIN: usize = 6;
pub const VSWTC: usize = 7;
pub const VSTART: usize = 8;
pub const VSTOP: usize = 9;
pub const VSUSP: usize = 10;
pub const VEOL: usize = 11;
pub const VREPRINT: usize = 12;
pub const VDISCARD: usize = 13;
pub const VWERASE: usize = 14;
pub const VLNEXT: usize = 15;
pub const VEOL2: usize = 16;

pub const IGNBRK: ::tcflag_t = 0o000_001;
pub const BRKINT: ::tcflag_t = 0o000_002;
pub const IGNPAR: ::tcflag_t = 0o000_004;
pub const PARMRK: ::tcflag_t = 0o000_010;
pub const INPCK: ::tcflag_t = 0o000_020;
pub const ISTRIP: ::tcflag_t = 0o000_040;
pub const INLCR: ::tcflag_t = 0o000_100;
pub const IGNCR: ::tcflag_t = 0o000_200;
pub const ICRNL: ::tcflag_t = 0o000_400;
pub const IUCLC: ::tcflag_t = 0o001_000;
pub const IXON: ::tcflag_t = 0o002_000;
pub const IXANY: ::tcflag_t = 0o004_000;
pub const IXOFF: ::tcflag_t = 0o010_000;
pub const IMAXBEL: ::tcflag_t = 0o020_000;
pub const IUTF8: ::tcflag_t = 0o040_000;

pub const OPOST: ::tcflag_t = 0o000_001;
pub const OLCUC: ::tcflag_t = 0o000_002;
pub const ONLCR: ::tcflag_t = 0o000_004;
pub const OCRNL: ::tcflag_t = 0o000_010;
pub const ONOCR: ::tcflag_t = 0o000_020;
pub const ONLRET: ::tcflag_t = 0o00_0040;
pub const OFILL: ::tcflag_t = 0o000_100;
pub const OFDEL: ::tcflag_t = 0o000_200;

pub const VTDLY: usize = 0o040_000;
pub const VT0: usize = 0o000_000;
pub const VT1: usize = 0o040_000;

pub const B0: speed_t = 0o000_000;
pub const B50: speed_t = 0o000_001;
pub const B75: speed_t = 0o000_002;
pub const B110: speed_t = 0o000_003;
pub const B134: speed_t = 0o000_004;
pub const B150: speed_t = 0o000_005;
pub const B200: speed_t = 0o000_006;
pub const B300: speed_t = 0o000_007;
pub const B600: speed_t = 0o000_010;
pub const B1200: speed_t = 0o000_011;
pub const B1800: speed_t = 0o000_012;
pub const B2400: speed_t = 0o000_013;
pub const B4800: speed_t = 0o000_014;
pub const B9600: speed_t = 0o000_015;
pub const B19200: speed_t = 0o000_016;
pub const B38400: speed_t = 0o000_017;

pub const B57600: speed_t = 0o010_001;
pub const B115200: speed_t = 0o010_002;
pub const B230400: speed_t = 0o010_003;
pub const B460800: speed_t = 0o010_004;
pub const B500000: speed_t = 0o010_005;
pub const B576000: speed_t = 0o010_006;
pub const B921600: speed_t = 0o010_007;
pub const B1000000: speed_t = 0o010_010;
pub const B1152000: speed_t = 0o010_011;
pub const B1500000: speed_t = 0o010_012;
pub const B2000000: speed_t = 0o010_013;
pub const B2500000: speed_t = 0o010_014;
pub const B3000000: speed_t = 0o010_015;
pub const B3500000: speed_t = 0o010_016;
pub const B4000000: speed_t = 0o010_017;

pub const CSIZE: ::tcflag_t = 0o000_060;
pub const CS5: ::tcflag_t = 0o000_000;
pub const CS6: ::tcflag_t = 0o000_020;
pub const CS7: ::tcflag_t = 0o000_040;
pub const CS8: ::tcflag_t = 0o000_060;
pub const CSTOPB: ::tcflag_t = 0o000_100;
pub const CREAD: ::tcflag_t = 0o000_200;
pub const PARENB: ::tcflag_t = 0o000_400;
pub const PARODD: ::tcflag_t = 0o001_000;
pub const HUPCL: ::tcflag_t = 0o002_000;
pub const CLOCAL: ::tcflag_t = 0o004_000;

pub const ISIG: ::tcflag_t = 0o000_001;
pub const ICANON: ::tcflag_t = 0o000_002;
pub const ECHO: ::tcflag_t = 0o000_010;
pub const ECHOE: ::tcflag_t = 0o000_020;
pub const ECHOK: ::tcflag_t = 0o000_040;
pub const ECHONL: ::tcflag_t = 0o000_100;
pub const NOFLSH: ::tcflag_t = 0o000_200;
pub const TOSTOP: ::tcflag_t = 0o000_400;
pub const IEXTEN: ::tcflag_t = 0o100_000;

pub const TCOOFF: ::c_int = 0;
pub const TCOON: ::c_int = 1;
pub const TCIOFF: ::c_int = 2;
pub const TCION: ::c_int = 3;

pub const TCIFLUSH: ::c_int = 0;
pub const TCOFLUSH: ::c_int = 1;
pub const TCIOFLUSH: ::c_int = 2;

pub const TCSANOW: ::c_int = 0;
pub const TCSADRAIN: ::c_int = 1;
pub const TCSAFLUSH: ::c_int = 2;

// sys/wait.h
pub const WNOHANG: ::c_int = 1;
pub const WUNTRACED: ::c_int = 2;

pub const WSTOPPED: ::c_int = 2;
pub const WEXITED: ::c_int = 4;
pub const WCONTINUED: ::c_int = 8;
pub const WNOWAIT: ::c_int = 0x0100_0000;

pub const __WNOTHREAD: ::c_int = 0x2000_0000;
pub const __WALL: ::c_int = 0x4000_0000;
#[allow(overflowing_literals)]
pub const __WCLONE: ::c_int = 0x8000_0000;

// time.h
pub const CLOCK_REALTIME: ::c_int = 1;
pub const CLOCK_MONOTONIC: ::c_int = 4;

// unistd.h
// POSIX.1 {
pub const _SC_ARG_MAX: ::c_int = 0;
pub const _SC_CHILD_MAX: ::c_int = 1;
pub const _SC_CLK_TCK: ::c_int = 2;
pub const _SC_NGROUPS_MAX: ::c_int = 3;
pub const _SC_OPEN_MAX: ::c_int = 4;
pub const _SC_STREAM_MAX: ::c_int = 5;
pub const _SC_TZNAME_MAX: ::c_int = 6;
// ...
pub const _SC_VERSION: ::c_int = 29;
pub const _SC_PAGESIZE: ::c_int = 30;
pub const _SC_PAGE_SIZE: ::c_int = 30;
// ...
pub const _SC_RE_DUP_MAX: ::c_int = 44;
// ...
pub const _SC_LOGIN_NAME_MAX: ::c_int = 71;
pub const _SC_TTY_NAME_MAX: ::c_int = 72;
// ...
pub const _SC_SYMLOOP_MAX: ::c_int = 173;
// ...
pub const _SC_HOST_NAME_MAX: ::c_int = 180;
// } POSIX.1

pub const F_OK: ::c_int = 0;
pub const R_OK: ::c_int = 4;
pub const W_OK: ::c_int = 2;
pub const X_OK: ::c_int = 1;

pub const SEEK_SET: ::c_int = 0;
pub const SEEK_CUR: ::c_int = 1;
pub const SEEK_END: ::c_int = 2;
pub const STDIN_FILENO: ::c_int = 0;
pub const STDOUT_FILENO: ::c_int = 1;
pub const STDERR_FILENO: ::c_int = 2;

pub const _PC_LINK_MAX: ::c_int = 0;
pub const _PC_MAX_CANON: ::c_int = 1;
pub const _PC_MAX_INPUT: ::c_int = 2;
pub const _PC_NAME_MAX: ::c_int = 3;
pub const _PC_PATH_MAX: ::c_int = 4;
pub const _PC_PIPE_BUF: ::c_int = 5;
pub const _PC_CHOWN_RESTRICTED: ::c_int = 6;
pub const _PC_NO_TRUNC: ::c_int = 7;
pub const _PC_VDISABLE: ::c_int = 8;
pub const _PC_SYNC_IO: ::c_int = 9;
pub const _PC_ASYNC_IO: ::c_int = 10;
pub const _PC_PRIO_IO: ::c_int = 11;
pub const _PC_SOCK_MAXBUF: ::c_int = 12;
pub const _PC_FILESIZEBITS: ::c_int = 13;
pub const _PC_REC_INCR_XFER_SIZE: ::c_int = 14;
pub const _PC_REC_MAX_XFER_SIZE: ::c_int = 15;
pub const _PC_REC_MIN_XFER_SIZE: ::c_int = 16;
pub const _PC_REC_XFER_ALIGN: ::c_int = 17;
pub const _PC_ALLOC_SIZE_MIN: ::c_int = 18;
pub const _PC_SYMLINK_MAX: ::c_int = 19;
pub const _PC_2_SYMLINKS: ::c_int = 20;

// wait.h
f! {
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
}

extern "C" {
    // errno.h
    pub fn __errno_location() -> *mut ::c_int;
    pub fn strerror_r(
        errnum: ::c_int,
        buf: *mut c_char,
        buflen: ::size_t,
    ) -> ::c_int;

    // unistd.h
    pub fn pipe2(fds: *mut ::c_int, flags: ::c_int) -> ::c_int;

    // malloc.h
    pub fn memalign(align: ::size_t, size: ::size_t) -> *mut ::c_void;

    // pthread.h
    pub fn pthread_atfork(
        prepare: ::Option<unsafe extern "C" fn()>,
        parent: ::Option<unsafe extern "C" fn()>,
        child: ::Option<unsafe extern "C" fn()>,
    ) -> ::c_int;
    pub fn pthread_create(
        tid: *mut ::pthread_t,
        attr: *const ::pthread_attr_t,
        start: extern "C" fn(*mut ::c_void) -> *mut ::c_void,
        arg: *mut ::c_void,
    ) -> ::c_int;
    pub fn pthread_condattr_setclock(
        attr: *mut pthread_condattr_t,
        clock_id: ::clockid_t,
    ) -> ::c_int;

    // pwd.h
    pub fn getpwuid_r(
        uid: ::uid_t,
        pwd: *mut passwd,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut passwd,
    ) -> ::c_int;

    // signal.h
    pub fn pthread_sigmask(
        how: ::c_int,
        set: *const ::sigset_t,
        oldset: *mut ::sigset_t,
    ) -> ::c_int;

    // sys/epoll.h
    pub fn epoll_create(size: ::c_int) -> ::c_int;
    pub fn epoll_create1(flags: ::c_int) -> ::c_int;
    pub fn epoll_wait(
        epfd: ::c_int,
        events: *mut ::epoll_event,
        maxevents: ::c_int,
        timeout: ::c_int,
    ) -> ::c_int;
    pub fn epoll_ctl(
        epfd: ::c_int,
        op: ::c_int,
        fd: ::c_int,
        event: *mut ::epoll_event,
    ) -> ::c_int;

    // sys/ioctl.h
    pub fn ioctl(fd: ::c_int, request: ::c_ulong, ...) -> ::c_int;

    // sys/resource.h
    pub fn getrlimit(resource: ::c_int, rlim: *mut ::rlimit) -> ::c_int;
    pub fn setrlimit(resource: ::c_int, rlim: *const ::rlimit) -> ::c_int;

    // sys/socket.h
    pub fn bind(
        socket: ::c_int,
        address: *const ::sockaddr,
        address_len: ::socklen_t,
    ) -> ::c_int;
    pub fn recvfrom(
        socket: ::c_int,
        buf: *mut ::c_void,
        len: ::size_t,
        flags: ::c_int,
        addr: *mut ::sockaddr,
        addrlen: *mut ::socklen_t,
    ) -> ::ssize_t;

    // sys/stat.h
    pub fn futimens(fd: ::c_int, times: *const ::timespec) -> ::c_int;

    // sys/uio.h
    pub fn readv(
        fd: ::c_int,
        iov: *const ::iovec,
        iovcnt: ::c_int,
    ) -> ::ssize_t;
    pub fn writev(
        fd: ::c_int,
        iov: *const ::iovec,
        iovcnt: ::c_int,
    ) -> ::ssize_t;

    // sys/utsname.h
    pub fn uname(utsname: *mut utsname) -> ::c_int;

    // time.h
    pub fn gettimeofday(tp: *mut ::timeval, tz: *mut ::timezone) -> ::c_int;
    pub fn clock_gettime(clk_id: ::clockid_t, tp: *mut ::timespec) -> ::c_int;
}

cfg_if! {
    if #[cfg(feature = "extra_traits")] {
        impl PartialEq for dirent {
            fn eq(&self, other: &dirent) -> bool {
                self.d_ino == other.d_ino
                    && self.d_off == other.d_off
                    && self.d_reclen == other.d_reclen
                    && self.d_type == other.d_type
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
                    .field("d_type", &self.d_type)
                // FIXME: .field("d_name", &self.d_name)
                    .finish()
            }
        }

        impl ::hash::Hash for dirent {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.d_ino.hash(state);
                self.d_off.hash(state);
                self.d_reclen.hash(state);
                self.d_type.hash(state);
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

        impl PartialEq for sockaddr_storage {
            fn eq(&self, other: &sockaddr_storage) -> bool {
                self.ss_family == other.ss_family
                    && self.__ss_align == self.__ss_align
                    && self
                    .__ss_padding
                    .iter()
                    .zip(other.__ss_padding.iter())
                    .all(|(a,b)| a == b)
            }
        }

        impl Eq for sockaddr_storage {}

        impl ::fmt::Debug for sockaddr_storage {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("sockaddr_storage")
                    .field("ss_family", &self.ss_family)
                    .field("__ss_align", &self.__ss_align)
                // FIXME: .field("__ss_padding", &self.__ss_padding)
                    .finish()
            }
        }

        impl ::hash::Hash for sockaddr_storage {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.ss_family.hash(state);
                self.__ss_padding.hash(state);
                self.__ss_align.hash(state);
            }
        }

        impl PartialEq for utsname {
            fn eq(&self, other: &utsname) -> bool {
                self.sysname
                    .iter()
                    .zip(other.sysname.iter())
                    .all(|(a, b)| a == b)
                    && self
                    .nodename
                    .iter()
                    .zip(other.nodename.iter())
                    .all(|(a, b)| a == b)
                    && self
                    .release
                    .iter()
                    .zip(other.release.iter())
                    .all(|(a, b)| a == b)
                    && self
                    .version
                    .iter()
                    .zip(other.version.iter())
                    .all(|(a, b)| a == b)
                    && self
                    .machine
                    .iter()
                    .zip(other.machine.iter())
                    .all(|(a, b)| a == b)
                    && self
                    .domainname
                    .iter()
                    .zip(other.domainname.iter())
                    .all(|(a, b)| a == b)
            }
        }

        impl Eq for utsname {}

        impl ::fmt::Debug for utsname {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("utsname")
                // FIXME: .field("sysname", &self.sysname)
                // FIXME: .field("nodename", &self.nodename)
                // FIXME: .field("release", &self.release)
                // FIXME: .field("version", &self.version)
                // FIXME: .field("machine", &self.machine)
                // FIXME: .field("domainname", &self.domainname)
                    .finish()
            }
        }

        impl ::hash::Hash for utsname {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.sysname.hash(state);
                self.nodename.hash(state);
                self.release.hash(state);
                self.version.hash(state);
                self.machine.hash(state);
                self.domainname.hash(state);
            }
        }
    }
}
