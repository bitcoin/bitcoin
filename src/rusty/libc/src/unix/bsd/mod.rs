pub type wchar_t = i32;
pub type off_t = i64;
pub type useconds_t = u32;
pub type blkcnt_t = i64;
pub type socklen_t = u32;
pub type sa_family_t = u8;
pub type pthread_t = ::uintptr_t;
pub type nfds_t = ::c_uint;

s! {
    pub struct sockaddr {
        pub sa_len: u8,
        pub sa_family: sa_family_t,
        pub sa_data: [::c_char; 14],
    }

    pub struct sockaddr_in6 {
        pub sin6_len: u8,
        pub sin6_family: sa_family_t,
        pub sin6_port: ::in_port_t,
        pub sin6_flowinfo: u32,
        pub sin6_addr: ::in6_addr,
        pub sin6_scope_id: u32,
    }

    pub struct passwd {
        pub pw_name: *mut ::c_char,
        pub pw_passwd: *mut ::c_char,
        pub pw_uid: ::uid_t,
        pub pw_gid: ::gid_t,
        pub pw_change: ::time_t,
        pub pw_class: *mut ::c_char,
        pub pw_gecos: *mut ::c_char,
        pub pw_dir: *mut ::c_char,
        pub pw_shell: *mut ::c_char,
        pub pw_expire: ::time_t,

        #[cfg(not(any(target_os = "macos",
                      target_os = "ios",
                      target_os = "netbsd",
                      target_os = "openbsd")))]
        pub pw_fields: ::c_int,
    }

    pub struct ifaddrs {
        pub ifa_next: *mut ifaddrs,
        pub ifa_name: *mut ::c_char,
        pub ifa_flags: ::c_uint,
        pub ifa_addr: *mut ::sockaddr,
        pub ifa_netmask: *mut ::sockaddr,
        pub ifa_dstaddr: *mut ::sockaddr,
        pub ifa_data: *mut ::c_void,
        #[cfg(target_os = "netbsd")]
        pub ifa_addrflags: ::c_uint
    }

    pub struct fd_set {
        #[cfg(all(target_pointer_width = "64",
                  any(target_os = "freebsd", target_os = "dragonfly")))]
        fds_bits: [i64; FD_SETSIZE / 64],
        #[cfg(not(all(target_pointer_width = "64",
                      any(target_os = "freebsd", target_os = "dragonfly"))))]
        fds_bits: [i32; FD_SETSIZE / 32],
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
        pub tm_zone: *mut ::c_char,
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

    pub struct fsid_t {
        __fsid_val: [i32; 2],
    }

    pub struct if_nameindex {
        pub if_index: ::c_uint,
        pub if_name: *mut ::c_char,
    }
}

s_no_extra_traits! {
    pub struct sockaddr_un {
        pub sun_len: u8,
        pub sun_family: sa_family_t,
        pub sun_path: [c_char; 104]
    }

    pub struct utsname {
        #[cfg(not(target_os = "dragonfly"))]
        pub sysname: [::c_char; 256],
        #[cfg(target_os = "dragonfly")]
        pub sysname: [::c_char; 32],
        #[cfg(not(target_os = "dragonfly"))]
        pub nodename: [::c_char; 256],
        #[cfg(target_os = "dragonfly")]
        pub nodename: [::c_char; 32],
        #[cfg(not(target_os = "dragonfly"))]
        pub release: [::c_char; 256],
        #[cfg(target_os = "dragonfly")]
        pub release: [::c_char; 32],
        #[cfg(not(target_os = "dragonfly"))]
        pub version: [::c_char; 256],
        #[cfg(target_os = "dragonfly")]
        pub version: [::c_char; 32],
        #[cfg(not(target_os = "dragonfly"))]
        pub machine: [::c_char; 256],
        #[cfg(target_os = "dragonfly")]
        pub machine: [::c_char; 32],
    }

}

cfg_if! {
    if #[cfg(feature = "extra_traits")] {
        impl PartialEq for sockaddr_un {
            fn eq(&self, other: &sockaddr_un) -> bool {
                self.sun_len == other.sun_len
                    && self.sun_family == other.sun_family
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
                    .field("sun_len", &self.sun_len)
                    .field("sun_family", &self.sun_family)
                // FIXME: .field("sun_path", &self.sun_path)
                    .finish()
            }
        }

        impl ::hash::Hash for sockaddr_un {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.sun_len.hash(state);
                self.sun_family.hash(state);
                self.sun_path.hash(state);
            }
        }

        impl PartialEq for utsname {
            fn eq(&self, other: &utsname) -> bool {
                self.sysname
                    .iter()
                    .zip(other.sysname.iter())
                    .all(|(a,b)| a == b)
                    && self
                    .nodename
                    .iter()
                    .zip(other.nodename.iter())
                    .all(|(a,b)| a == b)
                    && self
                    .release
                    .iter()
                    .zip(other.release.iter())
                    .all(|(a,b)| a == b)
                    && self
                    .version
                    .iter()
                    .zip(other.version.iter())
                    .all(|(a,b)| a == b)
                    && self
                    .machine
                    .iter()
                    .zip(other.machine.iter())
                    .all(|(a,b)| a == b)
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
            }
        }
    }
}

pub const LC_ALL: ::c_int = 0;
pub const LC_COLLATE: ::c_int = 1;
pub const LC_CTYPE: ::c_int = 2;
pub const LC_MONETARY: ::c_int = 3;
pub const LC_NUMERIC: ::c_int = 4;
pub const LC_TIME: ::c_int = 5;
pub const LC_MESSAGES: ::c_int = 6;

pub const FIOCLEX: ::c_ulong = 0x20006601;
pub const FIONCLEX: ::c_ulong = 0x20006602;
pub const FIONREAD: ::c_ulong = 0x4004667f;
pub const FIONBIO: ::c_ulong = 0x8004667e;
pub const FIOASYNC: ::c_ulong = 0x8004667d;
pub const FIOSETOWN: ::c_ulong = 0x8004667c;
pub const FIOGETOWN: ::c_ulong = 0x4004667b;

pub const PATH_MAX: ::c_int = 1024;

pub const SA_ONSTACK: ::c_int = 0x0001;
pub const SA_SIGINFO: ::c_int = 0x0040;
pub const SA_RESTART: ::c_int = 0x0002;
pub const SA_RESETHAND: ::c_int = 0x0004;
pub const SA_NOCLDSTOP: ::c_int = 0x0008;
pub const SA_NODEFER: ::c_int = 0x0010;
pub const SA_NOCLDWAIT: ::c_int = 0x0020;

pub const SS_ONSTACK: ::c_int = 1;
pub const SS_DISABLE: ::c_int = 4;

pub const SIGCHLD: ::c_int = 20;
pub const SIGBUS: ::c_int = 10;
pub const SIGUSR1: ::c_int = 30;
pub const SIGUSR2: ::c_int = 31;
pub const SIGCONT: ::c_int = 19;
pub const SIGSTOP: ::c_int = 17;
pub const SIGTSTP: ::c_int = 18;
pub const SIGURG: ::c_int = 16;
pub const SIGIO: ::c_int = 23;
pub const SIGSYS: ::c_int = 12;
pub const SIGTTIN: ::c_int = 21;
pub const SIGTTOU: ::c_int = 22;
pub const SIGXCPU: ::c_int = 24;
pub const SIGXFSZ: ::c_int = 25;
pub const SIGVTALRM: ::c_int = 26;
pub const SIGPROF: ::c_int = 27;
pub const SIGWINCH: ::c_int = 28;
pub const SIGINFO: ::c_int = 29;

pub const SIG_SETMASK: ::c_int = 3;
pub const SIG_BLOCK: ::c_int = 0x1;
pub const SIG_UNBLOCK: ::c_int = 0x2;

pub const IP_TOS: ::c_int = 3;
pub const IP_MULTICAST_IF: ::c_int = 9;
pub const IP_MULTICAST_TTL: ::c_int = 10;
pub const IP_MULTICAST_LOOP: ::c_int = 11;

pub const IPV6_UNICAST_HOPS: ::c_int = 4;
pub const IPV6_MULTICAST_IF: ::c_int = 9;
pub const IPV6_MULTICAST_HOPS: ::c_int = 10;
pub const IPV6_MULTICAST_LOOP: ::c_int = 11;
pub const IPV6_V6ONLY: ::c_int = 27;

pub const IPTOS_ECN_NOTECT: u8 = 0x00;
pub const IPTOS_ECN_MASK: u8 = 0x03;
pub const IPTOS_ECN_ECT1: u8 = 0x01;
pub const IPTOS_ECN_ECT0: u8 = 0x02;
pub const IPTOS_ECN_CE: u8 = 0x03;

pub const ST_RDONLY: ::c_ulong = 1;

pub const SCM_RIGHTS: ::c_int = 0x01;

pub const NCCS: usize = 20;

pub const O_ACCMODE: ::c_int = 0x3;
pub const O_RDONLY: ::c_int = 0;
pub const O_WRONLY: ::c_int = 1;
pub const O_RDWR: ::c_int = 2;
pub const O_APPEND: ::c_int = 8;
pub const O_CREAT: ::c_int = 512;
pub const O_TRUNC: ::c_int = 1024;
pub const O_EXCL: ::c_int = 2048;
pub const O_ASYNC: ::c_int = 0x40;
pub const O_SYNC: ::c_int = 0x80;
pub const O_NONBLOCK: ::c_int = 0x4;
pub const O_NOFOLLOW: ::c_int = 0x100;
pub const O_SHLOCK: ::c_int = 0x10;
pub const O_EXLOCK: ::c_int = 0x20;
pub const O_FSYNC: ::c_int = O_SYNC;
pub const O_NDELAY: ::c_int = O_NONBLOCK;

pub const F_GETOWN: ::c_int = 5;
pub const F_SETOWN: ::c_int = 6;

pub const F_RDLCK: ::c_short = 1;
pub const F_UNLCK: ::c_short = 2;
pub const F_WRLCK: ::c_short = 3;

pub const MNT_FORCE: ::c_int = 0x80000;

pub const Q_SYNC: ::c_int = 0x600;
pub const Q_QUOTAON: ::c_int = 0x100;
pub const Q_QUOTAOFF: ::c_int = 0x200;

pub const TCIOFF: ::c_int = 3;
pub const TCION: ::c_int = 4;
pub const TCOOFF: ::c_int = 1;
pub const TCOON: ::c_int = 2;
pub const TCIFLUSH: ::c_int = 1;
pub const TCOFLUSH: ::c_int = 2;
pub const TCIOFLUSH: ::c_int = 3;
pub const TCSANOW: ::c_int = 0;
pub const TCSADRAIN: ::c_int = 1;
pub const TCSAFLUSH: ::c_int = 2;
pub const VEOF: usize = 0;
pub const VEOL: usize = 1;
pub const VEOL2: usize = 2;
pub const VERASE: usize = 3;
pub const VWERASE: usize = 4;
pub const VKILL: usize = 5;
pub const VREPRINT: usize = 6;
pub const VINTR: usize = 8;
pub const VQUIT: usize = 9;
pub const VSUSP: usize = 10;
pub const VDSUSP: usize = 11;
pub const VSTART: usize = 12;
pub const VSTOP: usize = 13;
pub const VLNEXT: usize = 14;
pub const VDISCARD: usize = 15;
pub const VMIN: usize = 16;
pub const VTIME: usize = 17;
pub const VSTATUS: usize = 18;
pub const _POSIX_VDISABLE: ::cc_t = 0xff;
pub const IGNBRK: ::tcflag_t = 0x00000001;
pub const BRKINT: ::tcflag_t = 0x00000002;
pub const IGNPAR: ::tcflag_t = 0x00000004;
pub const PARMRK: ::tcflag_t = 0x00000008;
pub const INPCK: ::tcflag_t = 0x00000010;
pub const ISTRIP: ::tcflag_t = 0x00000020;
pub const INLCR: ::tcflag_t = 0x00000040;
pub const IGNCR: ::tcflag_t = 0x00000080;
pub const ICRNL: ::tcflag_t = 0x00000100;
pub const IXON: ::tcflag_t = 0x00000200;
pub const IXOFF: ::tcflag_t = 0x00000400;
pub const IXANY: ::tcflag_t = 0x00000800;
pub const IMAXBEL: ::tcflag_t = 0x00002000;
pub const OPOST: ::tcflag_t = 0x1;
pub const ONLCR: ::tcflag_t = 0x2;
pub const OXTABS: ::tcflag_t = 0x4;
pub const ONOEOT: ::tcflag_t = 0x8;
pub const CIGNORE: ::tcflag_t = 0x00000001;
pub const CSIZE: ::tcflag_t = 0x00000300;
pub const CS5: ::tcflag_t = 0x00000000;
pub const CS6: ::tcflag_t = 0x00000100;
pub const CS7: ::tcflag_t = 0x00000200;
pub const CS8: ::tcflag_t = 0x00000300;
pub const CSTOPB: ::tcflag_t = 0x00000400;
pub const CREAD: ::tcflag_t = 0x00000800;
pub const PARENB: ::tcflag_t = 0x00001000;
pub const PARODD: ::tcflag_t = 0x00002000;
pub const HUPCL: ::tcflag_t = 0x00004000;
pub const CLOCAL: ::tcflag_t = 0x00008000;
pub const ECHOKE: ::tcflag_t = 0x00000001;
pub const ECHOE: ::tcflag_t = 0x00000002;
pub const ECHOK: ::tcflag_t = 0x00000004;
pub const ECHO: ::tcflag_t = 0x00000008;
pub const ECHONL: ::tcflag_t = 0x00000010;
pub const ECHOPRT: ::tcflag_t = 0x00000020;
pub const ECHOCTL: ::tcflag_t = 0x00000040;
pub const ISIG: ::tcflag_t = 0x00000080;
pub const ICANON: ::tcflag_t = 0x00000100;
pub const ALTWERASE: ::tcflag_t = 0x00000200;
pub const IEXTEN: ::tcflag_t = 0x00000400;
pub const EXTPROC: ::tcflag_t = 0x00000800;
pub const TOSTOP: ::tcflag_t = 0x00400000;
pub const FLUSHO: ::tcflag_t = 0x00800000;
pub const NOKERNINFO: ::tcflag_t = 0x02000000;
pub const PENDIN: ::tcflag_t = 0x20000000;
pub const NOFLSH: ::tcflag_t = 0x80000000;
pub const MDMBUF: ::tcflag_t = 0x00100000;

pub const WNOHANG: ::c_int = 0x00000001;
pub const WUNTRACED: ::c_int = 0x00000002;

pub const RTLD_LAZY: ::c_int = 0x1;
pub const RTLD_NOW: ::c_int = 0x2;
pub const RTLD_NEXT: *mut ::c_void = -1isize as *mut ::c_void;
pub const RTLD_DEFAULT: *mut ::c_void = -2isize as *mut ::c_void;
pub const RTLD_SELF: *mut ::c_void = -3isize as *mut ::c_void;

pub const LOG_CRON: ::c_int = 9 << 3;
pub const LOG_AUTHPRIV: ::c_int = 10 << 3;
pub const LOG_FTP: ::c_int = 11 << 3;
pub const LOG_PERROR: ::c_int = 0x20;

pub const TCP_NODELAY: ::c_int = 1;
pub const TCP_MAXSEG: ::c_int = 2;

pub const PIPE_BUF: usize = 512;

pub const POLLIN: ::c_short = 0x1;
pub const POLLPRI: ::c_short = 0x2;
pub const POLLOUT: ::c_short = 0x4;
pub const POLLERR: ::c_short = 0x8;
pub const POLLHUP: ::c_short = 0x10;
pub const POLLNVAL: ::c_short = 0x20;
pub const POLLRDNORM: ::c_short = 0x040;
pub const POLLWRNORM: ::c_short = 0x004;
pub const POLLRDBAND: ::c_short = 0x080;
pub const POLLWRBAND: ::c_short = 0x100;

pub const BIOCGBLEN: ::c_ulong = 0x40044266;
pub const BIOCSBLEN: ::c_ulong = 0xc0044266;
pub const BIOCFLUSH: ::c_uint = 0x20004268;
pub const BIOCPROMISC: ::c_uint = 0x20004269;
pub const BIOCGDLT: ::c_ulong = 0x4004426a;
pub const BIOCGETIF: ::c_ulong = 0x4020426b;
pub const BIOCSETIF: ::c_ulong = 0x8020426c;
pub const BIOCGSTATS: ::c_ulong = 0x4008426f;
pub const BIOCIMMEDIATE: ::c_ulong = 0x80044270;
pub const BIOCVERSION: ::c_ulong = 0x40044271;
pub const BIOCGHDRCMPLT: ::c_ulong = 0x40044274;
pub const BIOCSHDRCMPLT: ::c_ulong = 0x80044275;
pub const SIOCGIFADDR: ::c_ulong = 0xc0206921;

f! {
    pub fn CMSG_FIRSTHDR(mhdr: *const ::msghdr) -> *mut ::cmsghdr {
        if (*mhdr).msg_controllen as usize >= ::mem::size_of::<::cmsghdr>() {
            (*mhdr).msg_control as *mut ::cmsghdr
        } else {
            0 as *mut ::cmsghdr
        }
    }

    pub fn FD_CLR(fd: ::c_int, set: *mut fd_set) -> () {
        let bits = ::mem::size_of_val(&(*set).fds_bits[0]) * 8;
        let fd = fd as usize;
        (*set).fds_bits[fd / bits] &= !(1 << (fd % bits));
        return
    }

    pub fn FD_ISSET(fd: ::c_int, set: *mut fd_set) -> bool {
        let bits = ::mem::size_of_val(&(*set).fds_bits[0]) * 8;
        let fd = fd as usize;
        return ((*set).fds_bits[fd / bits] & (1 << (fd % bits))) != 0
    }

    pub fn FD_SET(fd: ::c_int, set: *mut fd_set) -> () {
        let bits = ::mem::size_of_val(&(*set).fds_bits[0]) * 8;
        let fd = fd as usize;
        (*set).fds_bits[fd / bits] |= 1 << (fd % bits);
        return
    }

    pub fn FD_ZERO(set: *mut fd_set) -> () {
        for slot in (*set).fds_bits.iter_mut() {
            *slot = 0;
        }
    }

    pub fn WTERMSIG(status: ::c_int) -> ::c_int {
        status & 0o177
    }

    pub fn WIFEXITED(status: ::c_int) -> bool {
        (status & 0o177) == 0
    }

    pub fn WEXITSTATUS(status: ::c_int) -> ::c_int {
        status >> 8
    }

    pub fn WCOREDUMP(status: ::c_int) -> bool {
        (status & 0o200) != 0
    }

    pub fn QCMD(cmd: ::c_int, type_: ::c_int) -> ::c_int {
        (cmd << 8) | (type_ & 0x00ff)
    }
}

extern "C" {
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "getrlimit$UNIX2003"
    )]
    pub fn getrlimit(resource: ::c_int, rlim: *mut ::rlimit) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "setrlimit$UNIX2003"
    )]
    pub fn setrlimit(resource: ::c_int, rlim: *const ::rlimit) -> ::c_int;

    pub fn strerror_r(
        errnum: ::c_int,
        buf: *mut c_char,
        buflen: ::size_t,
    ) -> ::c_int;
    pub fn abs(i: ::c_int) -> ::c_int;
    pub fn atof(s: *const ::c_char) -> ::c_double;
    pub fn labs(i: ::c_long) -> ::c_long;
    pub fn rand() -> ::c_int;
    pub fn srand(seed: ::c_uint);

    pub fn getifaddrs(ifap: *mut *mut ::ifaddrs) -> ::c_int;
    pub fn freeifaddrs(ifa: *mut ::ifaddrs);
    pub fn setgroups(ngroups: ::c_int, ptr: *const ::gid_t) -> ::c_int;
    pub fn ioctl(fd: ::c_int, request: ::c_ulong, ...) -> ::c_int;
    pub fn kqueue() -> ::c_int;
    pub fn unmount(target: *const ::c_char, arg: ::c_int) -> ::c_int;
    pub fn syscall(num: ::c_int, ...) -> ::c_int;
    #[cfg_attr(target_os = "netbsd", link_name = "__getpwent50")]
    pub fn getpwent() -> *mut passwd;
    pub fn setpwent();
    pub fn endpwent();
    pub fn endgrent();
    pub fn getgrent() -> *mut ::group;

    pub fn getprogname() -> *const ::c_char;
    pub fn setprogname(name: *const ::c_char);
    pub fn getloadavg(loadavg: *mut ::c_double, nelem: ::c_int) -> ::c_int;
    pub fn if_nameindex() -> *mut if_nameindex;
    pub fn if_freenameindex(ptr: *mut if_nameindex);

    pub fn getpeereid(
        socket: ::c_int,
        euid: *mut ::uid_t,
        egid: *mut ::gid_t,
    ) -> ::c_int;

    #[cfg_attr(target_os = "macos", link_name = "glob$INODE64")]
    #[cfg_attr(target_os = "netbsd", link_name = "__glob30")]
    #[cfg_attr(
        all(target_os = "freebsd", any(freebsd11, freebsd10)),
        link_name = "glob@FBSD_1.0"
    )]
    pub fn glob(
        pattern: *const ::c_char,
        flags: ::c_int,
        errfunc: ::Option<
            extern "C" fn(epath: *const ::c_char, errno: ::c_int) -> ::c_int,
        >,
        pglob: *mut ::glob_t,
    ) -> ::c_int;
    #[cfg_attr(target_os = "netbsd", link_name = "__globfree30")]
    #[cfg_attr(
        all(target_os = "freebsd", any(freebsd11, freebsd10)),
        link_name = "globfree@FBSD_1.0"
    )]
    pub fn globfree(pglob: *mut ::glob_t);

    pub fn posix_madvise(
        addr: *mut ::c_void,
        len: ::size_t,
        advice: ::c_int,
    ) -> ::c_int;

    pub fn shm_unlink(name: *const ::c_char) -> ::c_int;

    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86_64"),
        link_name = "seekdir$INODE64"
    )]
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "seekdir$INODE64$UNIX2003"
    )]
    pub fn seekdir(dirp: *mut ::DIR, loc: ::c_long);

    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86_64"),
        link_name = "telldir$INODE64"
    )]
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "telldir$INODE64$UNIX2003"
    )]
    pub fn telldir(dirp: *mut ::DIR) -> ::c_long;
    pub fn madvise(
        addr: *mut ::c_void,
        len: ::size_t,
        advice: ::c_int,
    ) -> ::c_int;

    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "msync$UNIX2003"
    )]
    #[cfg_attr(target_os = "netbsd", link_name = "__msync13")]
    pub fn msync(
        addr: *mut ::c_void,
        len: ::size_t,
        flags: ::c_int,
    ) -> ::c_int;

    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "recvfrom$UNIX2003"
    )]
    pub fn recvfrom(
        socket: ::c_int,
        buf: *mut ::c_void,
        len: ::size_t,
        flags: ::c_int,
        addr: *mut ::sockaddr,
        addrlen: *mut ::socklen_t,
    ) -> ::ssize_t;
    pub fn mkstemps(template: *mut ::c_char, suffixlen: ::c_int) -> ::c_int;
    #[cfg_attr(target_os = "netbsd", link_name = "__futimes50")]
    pub fn futimes(fd: ::c_int, times: *const ::timeval) -> ::c_int;
    pub fn nl_langinfo(item: ::nl_item) -> *mut ::c_char;

    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "bind$UNIX2003"
    )]
    pub fn bind(
        socket: ::c_int,
        address: *const ::sockaddr,
        address_len: ::socklen_t,
    ) -> ::c_int;

    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "writev$UNIX2003"
    )]
    pub fn writev(
        fd: ::c_int,
        iov: *const ::iovec,
        iovcnt: ::c_int,
    ) -> ::ssize_t;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "readv$UNIX2003"
    )]
    pub fn readv(
        fd: ::c_int,
        iov: *const ::iovec,
        iovcnt: ::c_int,
    ) -> ::ssize_t;

    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "sendmsg$UNIX2003"
    )]
    pub fn sendmsg(
        fd: ::c_int,
        msg: *const ::msghdr,
        flags: ::c_int,
    ) -> ::ssize_t;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "recvmsg$UNIX2003"
    )]
    pub fn recvmsg(
        fd: ::c_int,
        msg: *mut ::msghdr,
        flags: ::c_int,
    ) -> ::ssize_t;

    pub fn sync();
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
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "pthread_cancel$UNIX2003"
    )]
    pub fn pthread_cancel(thread: ::pthread_t) -> ::c_int;
    pub fn pthread_kill(thread: ::pthread_t, sig: ::c_int) -> ::c_int;
    pub fn sem_unlink(name: *const ::c_char) -> ::c_int;
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
    pub fn getgrgid(gid: ::gid_t) -> *mut ::group;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "popen$UNIX2003"
    )]
    pub fn popen(command: *const c_char, mode: *const c_char) -> *mut ::FILE;
    pub fn faccessat(
        dirfd: ::c_int,
        pathname: *const ::c_char,
        mode: ::c_int,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn pthread_create(
        native: *mut ::pthread_t,
        attr: *const ::pthread_attr_t,
        f: extern "C" fn(*mut ::c_void) -> *mut ::c_void,
        value: *mut ::c_void,
    ) -> ::c_int;
    pub fn acct(filename: *const ::c_char) -> ::c_int;
}

cfg_if! {
    if #[cfg(any(target_os = "macos", target_os = "ios"))] {
        mod apple;
        pub use self::apple::*;
    } else if #[cfg(any(target_os = "openbsd", target_os = "netbsd"))] {
        mod netbsdlike;
        pub use self::netbsdlike::*;
    } else if #[cfg(any(target_os = "freebsd", target_os = "dragonfly"))] {
        mod freebsdlike;
        pub use self::freebsdlike::*;
    } else {
        // Unknown target_os
    }
}
