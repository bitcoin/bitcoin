//! Definitions found commonly among almost all Unix derivatives
//!
//! More functions and definitions can be found in the more specific modules
//! according to the platform in question.

pub type c_schar = i8;
pub type c_uchar = u8;
pub type c_short = i16;
pub type c_ushort = u16;
pub type c_int = i32;
pub type c_uint = u32;
pub type c_float = f32;
pub type c_double = f64;
pub type c_longlong = i64;
pub type c_ulonglong = u64;
pub type intmax_t = i64;
pub type uintmax_t = u64;

pub type size_t = usize;
pub type ptrdiff_t = isize;
pub type intptr_t = isize;
pub type uintptr_t = usize;
pub type ssize_t = isize;

pub type pid_t = i32;
pub type uid_t = u32;
pub type gid_t = u32;
pub type in_addr_t = u32;
pub type in_port_t = u16;
pub type sighandler_t = ::size_t;
pub type cc_t = ::c_uchar;

#[cfg_attr(feature = "extra_traits", derive(Debug))]
pub enum DIR {}
impl ::Copy for DIR {}
impl ::Clone for DIR {
    fn clone(&self) -> DIR {
        *self
    }
}
pub type locale_t = *mut ::c_void;

s! {
    pub struct group {
        pub gr_name: *mut ::c_char,
        pub gr_passwd: *mut ::c_char,
        pub gr_gid: ::gid_t,
        pub gr_mem: *mut *mut ::c_char,
    }

    pub struct utimbuf {
        pub actime: time_t,
        pub modtime: time_t,
    }

    pub struct timeval {
        pub tv_sec: time_t,
        pub tv_usec: suseconds_t,
    }

    // linux x32 compatibility
    // See https://sourceware.org/bugzilla/show_bug.cgi?id=16437
    pub struct timespec {
        pub tv_sec: time_t,
        #[cfg(all(target_arch = "x86_64", target_pointer_width = "32"))]
        pub tv_nsec: i64,
        #[cfg(not(all(target_arch = "x86_64", target_pointer_width = "32")))]
        pub tv_nsec: ::c_long,
    }

    pub struct rlimit {
        pub rlim_cur: rlim_t,
        pub rlim_max: rlim_t,
    }

    pub struct rusage {
        pub ru_utime: timeval,
        pub ru_stime: timeval,
        pub ru_maxrss: c_long,
        #[cfg(all(target_arch = "x86_64", target_pointer_width = "32"))]
        __pad1: u32,
        pub ru_ixrss: c_long,
        #[cfg(all(target_arch = "x86_64", target_pointer_width = "32"))]
        __pad2: u32,
        pub ru_idrss: c_long,
        #[cfg(all(target_arch = "x86_64", target_pointer_width = "32"))]
        __pad3: u32,
        pub ru_isrss: c_long,
        #[cfg(all(target_arch = "x86_64", target_pointer_width = "32"))]
        __pad4: u32,
        pub ru_minflt: c_long,
        #[cfg(all(target_arch = "x86_64", target_pointer_width = "32"))]
        __pad5: u32,
        pub ru_majflt: c_long,
        #[cfg(all(target_arch = "x86_64", target_pointer_width = "32"))]
        __pad6: u32,
        pub ru_nswap: c_long,
        #[cfg(all(target_arch = "x86_64", target_pointer_width = "32"))]
        __pad7: u32,
        pub ru_inblock: c_long,
        #[cfg(all(target_arch = "x86_64", target_pointer_width = "32"))]
        __pad8: u32,
        pub ru_oublock: c_long,
        #[cfg(all(target_arch = "x86_64", target_pointer_width = "32"))]
        __pad9: u32,
        pub ru_msgsnd: c_long,
        #[cfg(all(target_arch = "x86_64", target_pointer_width = "32"))]
        __pad10: u32,
        pub ru_msgrcv: c_long,
        #[cfg(all(target_arch = "x86_64", target_pointer_width = "32"))]
        __pad11: u32,
        pub ru_nsignals: c_long,
        #[cfg(all(target_arch = "x86_64", target_pointer_width = "32"))]
        __pad12: u32,
        pub ru_nvcsw: c_long,
        #[cfg(all(target_arch = "x86_64", target_pointer_width = "32"))]
        __pad13: u32,
        pub ru_nivcsw: c_long,
        #[cfg(all(target_arch = "x86_64", target_pointer_width = "32"))]
        __pad14: u32,

        #[cfg(any(target_env = "musl", target_os = "emscripten"))]
        __reserved: [c_long; 16],
    }

    pub struct ipv6_mreq {
        pub ipv6mr_multiaddr: in6_addr,
        #[cfg(target_os = "android")]
        pub ipv6mr_interface: ::c_int,
        #[cfg(not(target_os = "android"))]
        pub ipv6mr_interface: ::c_uint,
    }

    pub struct hostent {
        pub h_name: *mut ::c_char,
        pub h_aliases: *mut *mut ::c_char,
        pub h_addrtype: ::c_int,
        pub h_length: ::c_int,
        pub h_addr_list: *mut *mut ::c_char,
    }

    pub struct iovec {
        pub iov_base: *mut ::c_void,
        pub iov_len: ::size_t,
    }

    pub struct pollfd {
        pub fd: ::c_int,
        pub events: ::c_short,
        pub revents: ::c_short,
    }

    pub struct winsize {
        pub ws_row: ::c_ushort,
        pub ws_col: ::c_ushort,
        pub ws_xpixel: ::c_ushort,
        pub ws_ypixel: ::c_ushort,
    }

    pub struct linger {
        pub l_onoff: ::c_int,
        pub l_linger: ::c_int,
    }

    pub struct sigval {
        // Actually a union of an int and a void*
        pub sival_ptr: *mut ::c_void
    }

    // <sys/time.h>
    pub struct itimerval {
        pub it_interval: ::timeval,
        pub it_value: ::timeval,
    }

    // <sys/times.h>
    pub struct tms {
        pub tms_utime: ::clock_t,
        pub tms_stime: ::clock_t,
        pub tms_cutime: ::clock_t,
        pub tms_cstime: ::clock_t,
    }

    pub struct servent {
        pub s_name: *mut ::c_char,
        pub s_aliases: *mut *mut ::c_char,
        pub s_port: ::c_int,
        pub s_proto: *mut ::c_char,
    }

    pub struct protoent {
        pub p_name: *mut ::c_char,
        pub p_aliases: *mut *mut ::c_char,
        pub p_proto: ::c_int,
    }
}

pub const INT_MIN: c_int = -2147483648;
pub const INT_MAX: c_int = 2147483647;

pub const SIG_DFL: sighandler_t = 0 as sighandler_t;
pub const SIG_IGN: sighandler_t = 1 as sighandler_t;
pub const SIG_ERR: sighandler_t = !0 as sighandler_t;

pub const DT_UNKNOWN: u8 = 0;
pub const DT_FIFO: u8 = 1;
pub const DT_CHR: u8 = 2;
pub const DT_DIR: u8 = 4;
pub const DT_BLK: u8 = 6;
pub const DT_REG: u8 = 8;
pub const DT_LNK: u8 = 10;
pub const DT_SOCK: u8 = 12;

cfg_if! {
    if #[cfg(not(target_os = "redox"))] {
        pub const FD_CLOEXEC: ::c_int = 0x1;
    }
}

pub const USRQUOTA: ::c_int = 0;
pub const GRPQUOTA: ::c_int = 1;

pub const SIGIOT: ::c_int = 6;

pub const S_ISUID: ::mode_t = 0x800;
pub const S_ISGID: ::mode_t = 0x400;
pub const S_ISVTX: ::mode_t = 0x200;

pub const IF_NAMESIZE: ::size_t = 16;
pub const IFNAMSIZ: ::size_t = IF_NAMESIZE;

pub const LOG_EMERG: ::c_int = 0;
pub const LOG_ALERT: ::c_int = 1;
pub const LOG_CRIT: ::c_int = 2;
pub const LOG_ERR: ::c_int = 3;
pub const LOG_WARNING: ::c_int = 4;
pub const LOG_NOTICE: ::c_int = 5;
pub const LOG_INFO: ::c_int = 6;
pub const LOG_DEBUG: ::c_int = 7;

pub const LOG_KERN: ::c_int = 0;
pub const LOG_USER: ::c_int = 1 << 3;
pub const LOG_MAIL: ::c_int = 2 << 3;
pub const LOG_DAEMON: ::c_int = 3 << 3;
pub const LOG_AUTH: ::c_int = 4 << 3;
pub const LOG_SYSLOG: ::c_int = 5 << 3;
pub const LOG_LPR: ::c_int = 6 << 3;
pub const LOG_NEWS: ::c_int = 7 << 3;
pub const LOG_UUCP: ::c_int = 8 << 3;
pub const LOG_LOCAL0: ::c_int = 16 << 3;
pub const LOG_LOCAL1: ::c_int = 17 << 3;
pub const LOG_LOCAL2: ::c_int = 18 << 3;
pub const LOG_LOCAL3: ::c_int = 19 << 3;
pub const LOG_LOCAL4: ::c_int = 20 << 3;
pub const LOG_LOCAL5: ::c_int = 21 << 3;
pub const LOG_LOCAL6: ::c_int = 22 << 3;
pub const LOG_LOCAL7: ::c_int = 23 << 3;

pub const LOG_PID: ::c_int = 0x01;
pub const LOG_CONS: ::c_int = 0x02;
pub const LOG_ODELAY: ::c_int = 0x04;
pub const LOG_NDELAY: ::c_int = 0x08;
pub const LOG_NOWAIT: ::c_int = 0x10;

pub const LOG_PRIMASK: ::c_int = 7;
pub const LOG_FACMASK: ::c_int = 0x3f8;

pub const PRIO_PROCESS: ::c_int = 0;
pub const PRIO_PGRP: ::c_int = 1;
pub const PRIO_USER: ::c_int = 2;

pub const PRIO_MIN: ::c_int = -20;
pub const PRIO_MAX: ::c_int = 20;

pub const IPPROTO_ICMP: ::c_int = 1;
pub const IPPROTO_ICMPV6: ::c_int = 58;
pub const IPPROTO_TCP: ::c_int = 6;
pub const IPPROTO_UDP: ::c_int = 17;
pub const IPPROTO_IP: ::c_int = 0;
pub const IPPROTO_IPV6: ::c_int = 41;

pub const INADDR_LOOPBACK: in_addr_t = 2130706433;
pub const INADDR_ANY: in_addr_t = 0;
pub const INADDR_BROADCAST: in_addr_t = 4294967295;
pub const INADDR_NONE: in_addr_t = 4294967295;

pub const ARPOP_REQUEST: u16 = 1;
pub const ARPOP_REPLY: u16 = 2;

pub const ATF_COM: ::c_int = 0x02;
pub const ATF_PERM: ::c_int = 0x04;
pub const ATF_PUBL: ::c_int = 0x08;
pub const ATF_USETRAILERS: ::c_int = 0x10;

cfg_if! {
    if #[cfg(target_os = "l4re")] {
        // required libraries for L4Re are linked externally, ATM
    } else if #[cfg(feature = "std")] {
        // cargo build, don't pull in anything extra as the libstd dep
        // already pulls in all libs.
    } else if #[cfg(target_env = "musl")] {
        #[cfg_attr(feature = "rustc-dep-of-std",
                   link(name = "c", kind = "static",
                        cfg(target_feature = "crt-static")))]
        #[cfg_attr(feature = "rustc-dep-of-std",
                   link(name = "c", cfg(not(target_feature = "crt-static"))))]
        extern {}
    } else if #[cfg(target_os = "emscripten")] {
        #[link(name = "c")]
        extern {}
    } else if #[cfg(all(target_os = "netbsd",
                        feature = "rustc-dep-of-std",
                        target_vendor = "rumprun"))] {
        // Since we don't use -nodefaultlibs on Rumprun, libc is always pulled
        // in automatically by the linker. We avoid passing it explicitly, as it
        // causes some versions of binutils to crash with an assertion failure.
        #[link(name = "m")]
        extern {}
    } else if #[cfg(any(target_os = "macos",
                        target_os = "ios",
                        target_os = "android",
                        target_os = "openbsd"))] {
        #[link(name = "c")]
        #[link(name = "m")]
        extern {}
    } else if #[cfg(target_os = "haiku")] {
        #[link(name = "root")]
        #[link(name = "network")]
        extern {}
    } else if #[cfg(target_env = "newlib")] {
        #[link(name = "c")]
        #[link(name = "m")]
        extern {}
    } else if #[cfg(target_os = "hermit")] {
        // no_default_libraries is set to false for HermitCore, so only a link
        // to "pthread" needs to be added.
        #[link(name = "pthread")]
        extern {}
    } else if #[cfg(target_env = "illumos")] {
        #[link(name = "c")]
        #[link(name = "m")]
        extern {}
    } else if #[cfg(target_os = "redox")] {
        #[cfg_attr(feature = "rustc-dep-of-std",
                   link(name = "c", kind = "static-nobundle",
                        cfg(target_feature = "crt-static")))]
        #[cfg_attr(feature = "rustc-dep-of-std",
                   link(name = "c", cfg(not(target_feature = "crt-static"))))]
        extern {}
    } else {
        #[link(name = "c")]
        #[link(name = "m")]
        #[link(name = "rt")]
        #[link(name = "pthread")]
        extern {}
    }
}

#[cfg_attr(feature = "extra_traits", derive(Debug))]
pub enum FILE {}
impl ::Copy for FILE {}
impl ::Clone for FILE {
    fn clone(&self) -> FILE {
        *self
    }
}
#[cfg_attr(feature = "extra_traits", derive(Debug))]
pub enum fpos_t {} // TODO: fill this out with a struct
impl ::Copy for fpos_t {}
impl ::Clone for fpos_t {
    fn clone(&self) -> fpos_t {
        *self
    }
}

extern "C" {
    pub fn isalnum(c: c_int) -> c_int;
    pub fn isalpha(c: c_int) -> c_int;
    pub fn iscntrl(c: c_int) -> c_int;
    pub fn isdigit(c: c_int) -> c_int;
    pub fn isgraph(c: c_int) -> c_int;
    pub fn islower(c: c_int) -> c_int;
    pub fn isprint(c: c_int) -> c_int;
    pub fn ispunct(c: c_int) -> c_int;
    pub fn isspace(c: c_int) -> c_int;
    pub fn isupper(c: c_int) -> c_int;
    pub fn isxdigit(c: c_int) -> c_int;
    pub fn isblank(c: c_int) -> c_int;
    pub fn tolower(c: c_int) -> c_int;
    pub fn toupper(c: c_int) -> c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "fopen$UNIX2003"
    )]
    pub fn fopen(filename: *const c_char, mode: *const c_char) -> *mut FILE;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "freopen$UNIX2003"
    )]
    pub fn freopen(
        filename: *const c_char,
        mode: *const c_char,
        file: *mut FILE,
    ) -> *mut FILE;
    pub fn fflush(file: *mut FILE) -> c_int;
    pub fn fclose(file: *mut FILE) -> c_int;
    pub fn remove(filename: *const c_char) -> c_int;
    pub fn rename(oldname: *const c_char, newname: *const c_char) -> c_int;
    pub fn tmpfile() -> *mut FILE;
    pub fn setvbuf(
        stream: *mut FILE,
        buffer: *mut c_char,
        mode: c_int,
        size: size_t,
    ) -> c_int;
    pub fn setbuf(stream: *mut FILE, buf: *mut c_char);
    pub fn getchar() -> c_int;
    pub fn putchar(c: c_int) -> c_int;
    pub fn fgetc(stream: *mut FILE) -> c_int;
    pub fn fgets(buf: *mut c_char, n: c_int, stream: *mut FILE)
        -> *mut c_char;
    pub fn fputc(c: c_int, stream: *mut FILE) -> c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "fputs$UNIX2003"
    )]
    pub fn fputs(s: *const c_char, stream: *mut FILE) -> c_int;
    pub fn puts(s: *const c_char) -> c_int;
    pub fn ungetc(c: c_int, stream: *mut FILE) -> c_int;
    pub fn fread(
        ptr: *mut c_void,
        size: size_t,
        nobj: size_t,
        stream: *mut FILE,
    ) -> size_t;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "fwrite$UNIX2003"
    )]
    pub fn fwrite(
        ptr: *const c_void,
        size: size_t,
        nobj: size_t,
        stream: *mut FILE,
    ) -> size_t;
    pub fn fseek(stream: *mut FILE, offset: c_long, whence: c_int) -> c_int;
    pub fn ftell(stream: *mut FILE) -> c_long;
    pub fn rewind(stream: *mut FILE);
    #[cfg_attr(target_os = "netbsd", link_name = "__fgetpos50")]
    pub fn fgetpos(stream: *mut FILE, ptr: *mut fpos_t) -> c_int;
    #[cfg_attr(target_os = "netbsd", link_name = "__fsetpos50")]
    pub fn fsetpos(stream: *mut FILE, ptr: *const fpos_t) -> c_int;
    pub fn feof(stream: *mut FILE) -> c_int;
    pub fn ferror(stream: *mut FILE) -> c_int;
    pub fn perror(s: *const c_char);
    pub fn atoi(s: *const c_char) -> c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "strtod$UNIX2003"
    )]
    pub fn strtod(s: *const c_char, endp: *mut *mut c_char) -> c_double;
    pub fn strtol(
        s: *const c_char,
        endp: *mut *mut c_char,
        base: c_int,
    ) -> c_long;
    pub fn strtoul(
        s: *const c_char,
        endp: *mut *mut c_char,
        base: c_int,
    ) -> c_ulong;
    pub fn calloc(nobj: size_t, size: size_t) -> *mut c_void;
    pub fn malloc(size: size_t) -> *mut c_void;
    pub fn realloc(p: *mut c_void, size: size_t) -> *mut c_void;
    pub fn free(p: *mut c_void);
    pub fn abort() -> !;
    pub fn exit(status: c_int) -> !;
    pub fn _exit(status: c_int) -> !;
    pub fn atexit(cb: extern "C" fn()) -> c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "system$UNIX2003"
    )]
    pub fn system(s: *const c_char) -> c_int;
    pub fn getenv(s: *const c_char) -> *mut c_char;

    pub fn strcpy(dst: *mut c_char, src: *const c_char) -> *mut c_char;
    pub fn strncpy(
        dst: *mut c_char,
        src: *const c_char,
        n: size_t,
    ) -> *mut c_char;
    pub fn strcat(s: *mut c_char, ct: *const c_char) -> *mut c_char;
    pub fn strncat(
        s: *mut c_char,
        ct: *const c_char,
        n: size_t,
    ) -> *mut c_char;
    pub fn strcmp(cs: *const c_char, ct: *const c_char) -> c_int;
    pub fn strncmp(cs: *const c_char, ct: *const c_char, n: size_t) -> c_int;
    pub fn strcoll(cs: *const c_char, ct: *const c_char) -> c_int;
    pub fn strchr(cs: *const c_char, c: c_int) -> *mut c_char;
    pub fn strrchr(cs: *const c_char, c: c_int) -> *mut c_char;
    pub fn strspn(cs: *const c_char, ct: *const c_char) -> size_t;
    pub fn strcspn(cs: *const c_char, ct: *const c_char) -> size_t;
    pub fn strdup(cs: *const c_char) -> *mut c_char;
    pub fn strpbrk(cs: *const c_char, ct: *const c_char) -> *mut c_char;
    pub fn strstr(cs: *const c_char, ct: *const c_char) -> *mut c_char;
    pub fn strcasecmp(s1: *const c_char, s2: *const c_char) -> c_int;
    pub fn strncasecmp(
        s1: *const c_char,
        s2: *const c_char,
        n: size_t,
    ) -> c_int;
    pub fn strlen(cs: *const c_char) -> size_t;
    pub fn strnlen(cs: *const c_char, maxlen: size_t) -> size_t;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "strerror$UNIX2003"
    )]
    pub fn strerror(n: c_int) -> *mut c_char;
    pub fn strtok(s: *mut c_char, t: *const c_char) -> *mut c_char;
    pub fn strxfrm(s: *mut c_char, ct: *const c_char, n: size_t) -> size_t;
    pub fn wcslen(buf: *const wchar_t) -> size_t;
    pub fn wcstombs(
        dest: *mut c_char,
        src: *const wchar_t,
        n: size_t,
    ) -> ::size_t;

    pub fn memchr(cx: *const c_void, c: c_int, n: size_t) -> *mut c_void;
    pub fn memcmp(cx: *const c_void, ct: *const c_void, n: size_t) -> c_int;
    pub fn memcpy(
        dest: *mut c_void,
        src: *const c_void,
        n: size_t,
    ) -> *mut c_void;
    pub fn memmove(
        dest: *mut c_void,
        src: *const c_void,
        n: size_t,
    ) -> *mut c_void;
    pub fn memset(dest: *mut c_void, c: c_int, n: size_t) -> *mut c_void;
}

extern "C" {
    #[cfg_attr(target_os = "netbsd", link_name = "__getpwnam50")]
    pub fn getpwnam(name: *const ::c_char) -> *mut passwd;
    #[cfg_attr(target_os = "netbsd", link_name = "__getpwuid50")]
    pub fn getpwuid(uid: ::uid_t) -> *mut passwd;

    pub fn fprintf(
        stream: *mut ::FILE,
        format: *const ::c_char,
        ...
    ) -> ::c_int;
    pub fn printf(format: *const ::c_char, ...) -> ::c_int;
    pub fn snprintf(
        s: *mut ::c_char,
        n: ::size_t,
        format: *const ::c_char,
        ...
    ) -> ::c_int;
    pub fn sprintf(s: *mut ::c_char, format: *const ::c_char, ...) -> ::c_int;
    #[cfg_attr(target_os = "linux", link_name = "__isoc99_fscanf")]
    pub fn fscanf(
        stream: *mut ::FILE,
        format: *const ::c_char,
        ...
    ) -> ::c_int;
    #[cfg_attr(target_os = "linux", link_name = "__isoc99_scanf")]
    pub fn scanf(format: *const ::c_char, ...) -> ::c_int;
    #[cfg_attr(target_os = "linux", link_name = "__isoc99_sscanf")]
    pub fn sscanf(s: *const ::c_char, format: *const ::c_char, ...)
        -> ::c_int;
    pub fn getchar_unlocked() -> ::c_int;
    pub fn putchar_unlocked(c: ::c_int) -> ::c_int;

    #[cfg_attr(target_os = "netbsd", link_name = "__socket30")]
    #[cfg_attr(target_os = "illumos", link_name = "__xnet_socket")]
    pub fn socket(domain: ::c_int, ty: ::c_int, protocol: ::c_int) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "connect$UNIX2003"
    )]
    #[cfg_attr(target_os = "illumos", link_name = "__xnet_connect")]
    pub fn connect(
        socket: ::c_int,
        address: *const sockaddr,
        len: socklen_t,
    ) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "listen$UNIX2003"
    )]
    #[cfg_attr(target_os = "illumos", link_name = "__xnet_listen")]
    pub fn listen(socket: ::c_int, backlog: ::c_int) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "accept$UNIX2003"
    )]
    pub fn accept(
        socket: ::c_int,
        address: *mut sockaddr,
        address_len: *mut socklen_t,
    ) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "getpeername$UNIX2003"
    )]
    pub fn getpeername(
        socket: ::c_int,
        address: *mut sockaddr,
        address_len: *mut socklen_t,
    ) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "getsockname$UNIX2003"
    )]
    pub fn getsockname(
        socket: ::c_int,
        address: *mut sockaddr,
        address_len: *mut socklen_t,
    ) -> ::c_int;
    pub fn setsockopt(
        socket: ::c_int,
        level: ::c_int,
        name: ::c_int,
        value: *const ::c_void,
        option_len: socklen_t,
    ) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "socketpair$UNIX2003"
    )]
    #[cfg_attr(target_os = "illumos", link_name = "__xnet_socketpair")]
    pub fn socketpair(
        domain: ::c_int,
        type_: ::c_int,
        protocol: ::c_int,
        socket_vector: *mut ::c_int,
    ) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "sendto$UNIX2003"
    )]
    #[cfg_attr(target_os = "illumos", link_name = "__xnet_sendto")]
    pub fn sendto(
        socket: ::c_int,
        buf: *const ::c_void,
        len: ::size_t,
        flags: ::c_int,
        addr: *const sockaddr,
        addrlen: socklen_t,
    ) -> ::ssize_t;
    pub fn shutdown(socket: ::c_int, how: ::c_int) -> ::c_int;

    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "chmod$UNIX2003"
    )]
    pub fn chmod(path: *const c_char, mode: mode_t) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "fchmod$UNIX2003"
    )]
    pub fn fchmod(fd: ::c_int, mode: mode_t) -> ::c_int;

    #[cfg_attr(target_os = "macos", link_name = "fstat$INODE64")]
    #[cfg_attr(target_os = "netbsd", link_name = "__fstat50")]
    #[cfg_attr(
        all(target_os = "freebsd", any(freebsd11, freebsd10)),
        link_name = "fstat@FBSD_1.0"
    )]
    pub fn fstat(fildes: ::c_int, buf: *mut stat) -> ::c_int;

    pub fn mkdir(path: *const c_char, mode: mode_t) -> ::c_int;

    #[cfg_attr(target_os = "macos", link_name = "stat$INODE64")]
    #[cfg_attr(target_os = "netbsd", link_name = "__stat50")]
    #[cfg_attr(
        all(target_os = "freebsd", any(freebsd11, freebsd10)),
        link_name = "stat@FBSD_1.0"
    )]
    pub fn stat(path: *const c_char, buf: *mut stat) -> ::c_int;

    pub fn pclose(stream: *mut ::FILE) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "fdopen$UNIX2003"
    )]
    pub fn fdopen(fd: ::c_int, mode: *const c_char) -> *mut ::FILE;
    pub fn fileno(stream: *mut ::FILE) -> ::c_int;

    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "open$UNIX2003"
    )]
    pub fn open(path: *const c_char, oflag: ::c_int, ...) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "creat$UNIX2003"
    )]
    pub fn creat(path: *const c_char, mode: mode_t) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "fcntl$UNIX2003"
    )]
    pub fn fcntl(fd: ::c_int, cmd: ::c_int, ...) -> ::c_int;

    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86_64"),
        link_name = "opendir$INODE64"
    )]
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "opendir$INODE64$UNIX2003"
    )]
    #[cfg_attr(target_os = "netbsd", link_name = "__opendir30")]
    pub fn opendir(dirname: *const c_char) -> *mut ::DIR;

    #[cfg_attr(target_os = "macos", link_name = "readdir$INODE64")]
    #[cfg_attr(target_os = "netbsd", link_name = "__readdir30")]
    #[cfg_attr(
        all(target_os = "freebsd", any(freebsd11, freebsd10)),
        link_name = "readdir@FBSD_1.0"
    )]
    pub fn readdir(dirp: *mut ::DIR) -> *mut ::dirent;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "closedir$UNIX2003"
    )]
    pub fn closedir(dirp: *mut ::DIR) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86_64"),
        link_name = "rewinddir$INODE64"
    )]
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "rewinddir$INODE64$UNIX2003"
    )]
    pub fn rewinddir(dirp: *mut ::DIR);

    pub fn fchmodat(
        dirfd: ::c_int,
        pathname: *const ::c_char,
        mode: ::mode_t,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn fchown(fd: ::c_int, owner: ::uid_t, group: ::gid_t) -> ::c_int;
    pub fn fchownat(
        dirfd: ::c_int,
        pathname: *const ::c_char,
        owner: ::uid_t,
        group: ::gid_t,
        flags: ::c_int,
    ) -> ::c_int;
    #[cfg_attr(target_os = "macos", link_name = "fstatat$INODE64")]
    #[cfg_attr(
        all(target_os = "freebsd", any(freebsd11, freebsd10)),
        link_name = "fstatat@FBSD_1.1"
    )]
    pub fn fstatat(
        dirfd: ::c_int,
        pathname: *const ::c_char,
        buf: *mut stat,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn linkat(
        olddirfd: ::c_int,
        oldpath: *const ::c_char,
        newdirfd: ::c_int,
        newpath: *const ::c_char,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn renameat(
        olddirfd: ::c_int,
        oldpath: *const ::c_char,
        newdirfd: ::c_int,
        newpath: *const ::c_char,
    ) -> ::c_int;
    pub fn symlinkat(
        target: *const ::c_char,
        newdirfd: ::c_int,
        linkpath: *const ::c_char,
    ) -> ::c_int;
    pub fn unlinkat(
        dirfd: ::c_int,
        pathname: *const ::c_char,
        flags: ::c_int,
    ) -> ::c_int;

    pub fn access(path: *const c_char, amode: ::c_int) -> ::c_int;
    pub fn alarm(seconds: ::c_uint) -> ::c_uint;
    pub fn chdir(dir: *const c_char) -> ::c_int;
    pub fn fchdir(dirfd: ::c_int) -> ::c_int;
    pub fn chown(path: *const c_char, uid: uid_t, gid: gid_t) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "lchown$UNIX2003"
    )]
    pub fn lchown(path: *const c_char, uid: uid_t, gid: gid_t) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "close$NOCANCEL$UNIX2003"
    )]
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86_64"),
        link_name = "close$NOCANCEL"
    )]
    pub fn close(fd: ::c_int) -> ::c_int;
    pub fn dup(fd: ::c_int) -> ::c_int;
    pub fn dup2(src: ::c_int, dst: ::c_int) -> ::c_int;
    pub fn execl(path: *const c_char, arg0: *const c_char, ...) -> ::c_int;
    pub fn execle(
        path: *const ::c_char,
        arg0: *const ::c_char,
        ...
    ) -> ::c_int;
    pub fn execlp(
        file: *const ::c_char,
        arg0: *const ::c_char,
        ...
    ) -> ::c_int;
    pub fn execv(prog: *const c_char, argv: *const *const c_char) -> ::c_int;
    pub fn execve(
        prog: *const c_char,
        argv: *const *const c_char,
        envp: *const *const c_char,
    ) -> ::c_int;
    pub fn execvp(c: *const c_char, argv: *const *const c_char) -> ::c_int;
    pub fn fork() -> pid_t;
    pub fn fpathconf(filedes: ::c_int, name: ::c_int) -> c_long;
    pub fn getcwd(buf: *mut c_char, size: ::size_t) -> *mut c_char;
    pub fn getegid() -> gid_t;
    pub fn geteuid() -> uid_t;
    pub fn getgid() -> gid_t;
    pub fn getgroups(ngroups_max: ::c_int, groups: *mut gid_t) -> ::c_int;
    pub fn getlogin() -> *mut c_char;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "getopt$UNIX2003"
    )]
    pub fn getopt(
        argc: ::c_int,
        argv: *const *mut c_char,
        optstr: *const c_char,
    ) -> ::c_int;
    pub fn getpgid(pid: pid_t) -> pid_t;
    pub fn getpgrp() -> pid_t;
    pub fn getpid() -> pid_t;
    pub fn getppid() -> pid_t;
    pub fn getuid() -> uid_t;
    pub fn isatty(fd: ::c_int) -> ::c_int;
    pub fn link(src: *const c_char, dst: *const c_char) -> ::c_int;
    pub fn lseek(fd: ::c_int, offset: off_t, whence: ::c_int) -> off_t;
    pub fn pathconf(path: *const c_char, name: ::c_int) -> c_long;
    pub fn pipe(fds: *mut ::c_int) -> ::c_int;
    pub fn posix_memalign(
        memptr: *mut *mut ::c_void,
        align: ::size_t,
        size: ::size_t,
    ) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "read$UNIX2003"
    )]
    pub fn read(fd: ::c_int, buf: *mut ::c_void, count: ::size_t)
        -> ::ssize_t;
    pub fn rmdir(path: *const c_char) -> ::c_int;
    pub fn seteuid(uid: uid_t) -> ::c_int;
    pub fn setegid(gid: gid_t) -> ::c_int;
    pub fn setgid(gid: gid_t) -> ::c_int;
    pub fn setpgid(pid: pid_t, pgid: pid_t) -> ::c_int;
    pub fn setsid() -> pid_t;
    pub fn setuid(uid: uid_t) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "sleep$UNIX2003"
    )]
    pub fn sleep(secs: ::c_uint) -> ::c_uint;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "nanosleep$UNIX2003"
    )]
    #[cfg_attr(target_os = "netbsd", link_name = "__nanosleep50")]
    pub fn nanosleep(rqtp: *const timespec, rmtp: *mut timespec) -> ::c_int;
    pub fn tcgetpgrp(fd: ::c_int) -> pid_t;
    pub fn tcsetpgrp(fd: ::c_int, pgrp: ::pid_t) -> ::c_int;
    pub fn ttyname(fd: ::c_int) -> *mut c_char;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "ttyname_r$UNIX2003"
    )]
    pub fn ttyname_r(
        fd: ::c_int,
        buf: *mut c_char,
        buflen: ::size_t,
    ) -> ::c_int;
    pub fn unlink(c: *const c_char) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "wait$UNIX2003"
    )]
    pub fn wait(status: *mut ::c_int) -> pid_t;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "waitpid$UNIX2003"
    )]
    pub fn waitpid(
        pid: pid_t,
        status: *mut ::c_int,
        options: ::c_int,
    ) -> pid_t;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "write$UNIX2003"
    )]
    pub fn write(
        fd: ::c_int,
        buf: *const ::c_void,
        count: ::size_t,
    ) -> ::ssize_t;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "pread$UNIX2003"
    )]
    pub fn pread(
        fd: ::c_int,
        buf: *mut ::c_void,
        count: ::size_t,
        offset: off_t,
    ) -> ::ssize_t;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "pwrite$UNIX2003"
    )]
    pub fn pwrite(
        fd: ::c_int,
        buf: *const ::c_void,
        count: ::size_t,
        offset: off_t,
    ) -> ::ssize_t;
    pub fn umask(mask: mode_t) -> mode_t;

    #[cfg_attr(target_os = "netbsd", link_name = "__utime50")]
    pub fn utime(file: *const c_char, buf: *const utimbuf) -> ::c_int;

    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "kill$UNIX2003"
    )]
    pub fn kill(pid: pid_t, sig: ::c_int) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "killpg$UNIX2003"
    )]
    pub fn killpg(pgrp: pid_t, sig: ::c_int) -> ::c_int;

    pub fn mlock(addr: *const ::c_void, len: ::size_t) -> ::c_int;
    pub fn munlock(addr: *const ::c_void, len: ::size_t) -> ::c_int;
    pub fn mlockall(flags: ::c_int) -> ::c_int;
    pub fn munlockall() -> ::c_int;

    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "mmap$UNIX2003"
    )]
    pub fn mmap(
        addr: *mut ::c_void,
        len: ::size_t,
        prot: ::c_int,
        flags: ::c_int,
        fd: ::c_int,
        offset: off_t,
    ) -> *mut ::c_void;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "munmap$UNIX2003"
    )]
    pub fn munmap(addr: *mut ::c_void, len: ::size_t) -> ::c_int;

    pub fn if_nametoindex(ifname: *const c_char) -> ::c_uint;
    pub fn if_indextoname(
        ifindex: ::c_uint,
        ifname: *mut ::c_char,
    ) -> *mut ::c_char;

    #[cfg_attr(target_os = "macos", link_name = "lstat$INODE64")]
    #[cfg_attr(target_os = "netbsd", link_name = "__lstat50")]
    #[cfg_attr(
        all(target_os = "freebsd", any(freebsd11, freebsd10)),
        link_name = "lstat@FBSD_1.0"
    )]
    pub fn lstat(path: *const c_char, buf: *mut stat) -> ::c_int;

    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "fsync$UNIX2003"
    )]
    pub fn fsync(fd: ::c_int) -> ::c_int;

    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "setenv$UNIX2003"
    )]
    pub fn setenv(
        name: *const c_char,
        val: *const c_char,
        overwrite: ::c_int,
    ) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "unsetenv$UNIX2003"
    )]
    #[cfg_attr(target_os = "netbsd", link_name = "__unsetenv13")]
    pub fn unsetenv(name: *const c_char) -> ::c_int;

    pub fn symlink(path1: *const c_char, path2: *const c_char) -> ::c_int;

    pub fn ftruncate(fd: ::c_int, length: off_t) -> ::c_int;

    pub fn signal(signum: ::c_int, handler: sighandler_t) -> sighandler_t;

    #[cfg_attr(target_os = "netbsd", link_name = "__getrusage50")]
    pub fn getrusage(resource: ::c_int, usage: *mut rusage) -> ::c_int;

    #[cfg_attr(
        any(target_os = "macos", target_os = "ios"),
        link_name = "realpath$DARWIN_EXTSN"
    )]
    pub fn realpath(
        pathname: *const ::c_char,
        resolved: *mut ::c_char,
    ) -> *mut ::c_char;

    pub fn flock(fd: ::c_int, operation: ::c_int) -> ::c_int;

    #[cfg_attr(target_os = "netbsd", link_name = "__times13")]
    pub fn times(buf: *mut ::tms) -> ::clock_t;

    pub fn pthread_self() -> ::pthread_t;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "pthread_join$UNIX2003"
    )]
    pub fn pthread_join(
        native: ::pthread_t,
        value: *mut *mut ::c_void,
    ) -> ::c_int;
    pub fn pthread_exit(value: *mut ::c_void) -> !;
    pub fn pthread_attr_init(attr: *mut ::pthread_attr_t) -> ::c_int;
    pub fn pthread_attr_destroy(attr: *mut ::pthread_attr_t) -> ::c_int;
    pub fn pthread_attr_setstacksize(
        attr: *mut ::pthread_attr_t,
        stack_size: ::size_t,
    ) -> ::c_int;
    pub fn pthread_attr_setdetachstate(
        attr: *mut ::pthread_attr_t,
        state: ::c_int,
    ) -> ::c_int;
    pub fn pthread_detach(thread: ::pthread_t) -> ::c_int;
    #[cfg_attr(target_os = "netbsd", link_name = "__libc_thr_yield")]
    pub fn sched_yield() -> ::c_int;
    pub fn pthread_key_create(
        key: *mut pthread_key_t,
        dtor: ::Option<unsafe extern "C" fn(*mut ::c_void)>,
    ) -> ::c_int;
    pub fn pthread_key_delete(key: pthread_key_t) -> ::c_int;
    pub fn pthread_getspecific(key: pthread_key_t) -> *mut ::c_void;
    pub fn pthread_setspecific(
        key: pthread_key_t,
        value: *const ::c_void,
    ) -> ::c_int;
    pub fn pthread_mutex_init(
        lock: *mut pthread_mutex_t,
        attr: *const pthread_mutexattr_t,
    ) -> ::c_int;
    pub fn pthread_mutex_destroy(lock: *mut pthread_mutex_t) -> ::c_int;
    pub fn pthread_mutex_lock(lock: *mut pthread_mutex_t) -> ::c_int;
    pub fn pthread_mutex_trylock(lock: *mut pthread_mutex_t) -> ::c_int;
    pub fn pthread_mutex_unlock(lock: *mut pthread_mutex_t) -> ::c_int;

    pub fn pthread_mutexattr_init(attr: *mut pthread_mutexattr_t) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "pthread_mutexattr_destroy$UNIX2003"
    )]
    pub fn pthread_mutexattr_destroy(
        attr: *mut pthread_mutexattr_t,
    ) -> ::c_int;
    pub fn pthread_mutexattr_settype(
        attr: *mut pthread_mutexattr_t,
        _type: ::c_int,
    ) -> ::c_int;

    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "pthread_cond_init$UNIX2003"
    )]
    pub fn pthread_cond_init(
        cond: *mut pthread_cond_t,
        attr: *const pthread_condattr_t,
    ) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "pthread_cond_wait$UNIX2003"
    )]
    pub fn pthread_cond_wait(
        cond: *mut pthread_cond_t,
        lock: *mut pthread_mutex_t,
    ) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "pthread_cond_timedwait$UNIX2003"
    )]
    pub fn pthread_cond_timedwait(
        cond: *mut pthread_cond_t,
        lock: *mut pthread_mutex_t,
        abstime: *const ::timespec,
    ) -> ::c_int;
    pub fn pthread_cond_signal(cond: *mut pthread_cond_t) -> ::c_int;
    pub fn pthread_cond_broadcast(cond: *mut pthread_cond_t) -> ::c_int;
    pub fn pthread_cond_destroy(cond: *mut pthread_cond_t) -> ::c_int;
    pub fn pthread_condattr_init(attr: *mut pthread_condattr_t) -> ::c_int;
    pub fn pthread_condattr_destroy(attr: *mut pthread_condattr_t) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "pthread_rwlock_init$UNIX2003"
    )]
    pub fn pthread_rwlock_init(
        lock: *mut pthread_rwlock_t,
        attr: *const pthread_rwlockattr_t,
    ) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "pthread_rwlock_destroy$UNIX2003"
    )]
    pub fn pthread_rwlock_destroy(lock: *mut pthread_rwlock_t) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "pthread_rwlock_rdlock$UNIX2003"
    )]
    pub fn pthread_rwlock_rdlock(lock: *mut pthread_rwlock_t) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "pthread_rwlock_tryrdlock$UNIX2003"
    )]
    pub fn pthread_rwlock_tryrdlock(lock: *mut pthread_rwlock_t) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "pthread_rwlock_wrlock$UNIX2003"
    )]
    pub fn pthread_rwlock_wrlock(lock: *mut pthread_rwlock_t) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "pthread_rwlock_trywrlock$UNIX2003"
    )]
    pub fn pthread_rwlock_trywrlock(lock: *mut pthread_rwlock_t) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "pthread_rwlock_unlock$UNIX2003"
    )]
    pub fn pthread_rwlock_unlock(lock: *mut pthread_rwlock_t) -> ::c_int;
    pub fn pthread_rwlockattr_init(attr: *mut pthread_rwlockattr_t)
        -> ::c_int;
    pub fn pthread_rwlockattr_destroy(
        attr: *mut pthread_rwlockattr_t,
    ) -> ::c_int;

    #[cfg_attr(target_os = "illumos", link_name = "__xnet_getsockopt")]
    pub fn getsockopt(
        sockfd: ::c_int,
        level: ::c_int,
        optname: ::c_int,
        optval: *mut ::c_void,
        optlen: *mut ::socklen_t,
    ) -> ::c_int;
    pub fn raise(signum: ::c_int) -> ::c_int;
    #[cfg_attr(target_os = "netbsd", link_name = "__sigaction14")]
    pub fn sigaction(
        signum: ::c_int,
        act: *const sigaction,
        oldact: *mut sigaction,
    ) -> ::c_int;

    #[cfg_attr(target_os = "netbsd", link_name = "__utimes50")]
    pub fn utimes(
        filename: *const ::c_char,
        times: *const ::timeval,
    ) -> ::c_int;
    pub fn dlopen(filename: *const ::c_char, flag: ::c_int) -> *mut ::c_void;
    pub fn dlerror() -> *mut ::c_char;
    pub fn dlsym(
        handle: *mut ::c_void,
        symbol: *const ::c_char,
    ) -> *mut ::c_void;
    pub fn dlclose(handle: *mut ::c_void) -> ::c_int;
    pub fn dladdr(addr: *const ::c_void, info: *mut Dl_info) -> ::c_int;

    pub fn getaddrinfo(
        node: *const c_char,
        service: *const c_char,
        hints: *const addrinfo,
        res: *mut *mut addrinfo,
    ) -> ::c_int;
    pub fn freeaddrinfo(res: *mut addrinfo);
    pub fn gai_strerror(errcode: ::c_int) -> *const ::c_char;
    #[cfg_attr(
        any(
            all(target_os = "linux", not(target_env = "musl")),
            target_os = "freebsd",
            target_os = "dragonfly",
            target_os = "haiku"
        ),
        link_name = "__res_init"
    )]
    #[cfg_attr(
        any(target_os = "macos", target_os = "ios"),
        link_name = "res_9_init"
    )]
    pub fn res_init() -> ::c_int;

    #[cfg_attr(target_os = "netbsd", link_name = "__gmtime_r50")]
    pub fn gmtime_r(time_p: *const time_t, result: *mut tm) -> *mut tm;
    #[cfg_attr(target_os = "netbsd", link_name = "__localtime_r50")]
    pub fn localtime_r(time_p: *const time_t, result: *mut tm) -> *mut tm;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "mktime$UNIX2003"
    )]
    #[cfg_attr(target_os = "netbsd", link_name = "__mktime50")]
    pub fn mktime(tm: *mut tm) -> time_t;
    #[cfg_attr(target_os = "netbsd", link_name = "__time50")]
    pub fn time(time: *mut time_t) -> time_t;
    #[cfg_attr(target_os = "netbsd", link_name = "__gmtime50")]
    pub fn gmtime(time_p: *const time_t) -> *mut tm;
    #[cfg_attr(target_os = "netbsd", link_name = "__locatime50")]
    pub fn localtime(time_p: *const time_t) -> *mut tm;
    #[cfg_attr(target_os = "netbsd", link_name = "__difftime50")]
    pub fn difftime(time1: time_t, time0: time_t) -> ::c_double;

    #[cfg_attr(target_os = "netbsd", link_name = "__mknod50")]
    #[cfg_attr(
        all(target_os = "freebsd", any(freebsd11, freebsd10)),
        link_name = "mknod@FBSD_1.0"
    )]
    pub fn mknod(
        pathname: *const ::c_char,
        mode: ::mode_t,
        dev: ::dev_t,
    ) -> ::c_int;
    pub fn gethostname(name: *mut ::c_char, len: ::size_t) -> ::c_int;
    pub fn getservbyname(
        name: *const ::c_char,
        proto: *const ::c_char,
    ) -> *mut servent;
    pub fn getprotobyname(name: *const ::c_char) -> *mut protoent;
    pub fn getprotobynumber(proto: ::c_int) -> *mut protoent;
    pub fn chroot(name: *const ::c_char) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "usleep$UNIX2003"
    )]
    pub fn usleep(secs: ::c_uint) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "send$UNIX2003"
    )]
    pub fn send(
        socket: ::c_int,
        buf: *const ::c_void,
        len: ::size_t,
        flags: ::c_int,
    ) -> ::ssize_t;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "recv$UNIX2003"
    )]
    pub fn recv(
        socket: ::c_int,
        buf: *mut ::c_void,
        len: ::size_t,
        flags: ::c_int,
    ) -> ::ssize_t;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "putenv$UNIX2003"
    )]
    #[cfg_attr(target_os = "netbsd", link_name = "__putenv50")]
    pub fn putenv(string: *mut c_char) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "poll$UNIX2003"
    )]
    pub fn poll(fds: *mut pollfd, nfds: nfds_t, timeout: ::c_int) -> ::c_int;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86_64"),
        link_name = "select$1050"
    )]
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "select$UNIX2003"
    )]
    #[cfg_attr(target_os = "netbsd", link_name = "__select50")]
    pub fn select(
        nfds: ::c_int,
        readfs: *mut fd_set,
        writefds: *mut fd_set,
        errorfds: *mut fd_set,
        timeout: *mut timeval,
    ) -> ::c_int;
    #[cfg_attr(target_os = "netbsd", link_name = "__setlocale50")]
    pub fn setlocale(
        category: ::c_int,
        locale: *const ::c_char,
    ) -> *mut ::c_char;
    pub fn localeconv() -> *mut lconv;

    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "sem_wait$UNIX2003"
    )]
    pub fn sem_wait(sem: *mut sem_t) -> ::c_int;
    pub fn sem_trywait(sem: *mut sem_t) -> ::c_int;
    pub fn sem_post(sem: *mut sem_t) -> ::c_int;
    pub fn statvfs(path: *const c_char, buf: *mut statvfs) -> ::c_int;
    pub fn fstatvfs(fd: ::c_int, buf: *mut statvfs) -> ::c_int;

    pub fn readlink(
        path: *const c_char,
        buf: *mut c_char,
        bufsz: ::size_t,
    ) -> ::ssize_t;

    #[cfg_attr(target_os = "netbsd", link_name = "__sigemptyset14")]
    pub fn sigemptyset(set: *mut sigset_t) -> ::c_int;
    #[cfg_attr(target_os = "netbsd", link_name = "__sigaddset14")]
    pub fn sigaddset(set: *mut sigset_t, signum: ::c_int) -> ::c_int;
    #[cfg_attr(target_os = "netbsd", link_name = "__sigfillset14")]
    pub fn sigfillset(set: *mut sigset_t) -> ::c_int;
    #[cfg_attr(target_os = "netbsd", link_name = "__sigdelset14")]
    pub fn sigdelset(set: *mut sigset_t, signum: ::c_int) -> ::c_int;
    #[cfg_attr(target_os = "netbsd", link_name = "__sigismember14")]
    pub fn sigismember(set: *const sigset_t, signum: ::c_int) -> ::c_int;

    #[cfg_attr(target_os = "netbsd", link_name = "__sigprocmask14")]
    pub fn sigprocmask(
        how: ::c_int,
        set: *const sigset_t,
        oldset: *mut sigset_t,
    ) -> ::c_int;
    #[cfg_attr(target_os = "netbsd", link_name = "__sigpending14")]
    pub fn sigpending(set: *mut sigset_t) -> ::c_int;

    #[cfg_attr(target_os = "netbsd", link_name = "__timegm50")]
    pub fn timegm(tm: *mut ::tm) -> time_t;

    pub fn sysconf(name: ::c_int) -> ::c_long;

    pub fn mkfifo(path: *const c_char, mode: mode_t) -> ::c_int;

    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86_64"),
        link_name = "pselect$1050"
    )]
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "pselect$UNIX2003"
    )]
    #[cfg_attr(target_os = "netbsd", link_name = "__pselect50")]
    pub fn pselect(
        nfds: ::c_int,
        readfs: *mut fd_set,
        writefds: *mut fd_set,
        errorfds: *mut fd_set,
        timeout: *const timespec,
        sigmask: *const sigset_t,
    ) -> ::c_int;
    pub fn fseeko(
        stream: *mut ::FILE,
        offset: ::off_t,
        whence: ::c_int,
    ) -> ::c_int;
    pub fn ftello(stream: *mut ::FILE) -> ::off_t;
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "tcdrain$UNIX2003"
    )]
    pub fn tcdrain(fd: ::c_int) -> ::c_int;
    pub fn cfgetispeed(termios: *const ::termios) -> ::speed_t;
    pub fn cfgetospeed(termios: *const ::termios) -> ::speed_t;
    pub fn cfsetispeed(termios: *mut ::termios, speed: ::speed_t) -> ::c_int;
    pub fn cfsetospeed(termios: *mut ::termios, speed: ::speed_t) -> ::c_int;
    pub fn tcgetattr(fd: ::c_int, termios: *mut ::termios) -> ::c_int;
    pub fn tcsetattr(
        fd: ::c_int,
        optional_actions: ::c_int,
        termios: *const ::termios,
    ) -> ::c_int;
    pub fn tcflow(fd: ::c_int, action: ::c_int) -> ::c_int;
    pub fn tcflush(fd: ::c_int, action: ::c_int) -> ::c_int;
    pub fn tcgetsid(fd: ::c_int) -> ::pid_t;
    pub fn tcsendbreak(fd: ::c_int, duration: ::c_int) -> ::c_int;
    pub fn mkstemp(template: *mut ::c_char) -> ::c_int;
    pub fn mkdtemp(template: *mut ::c_char) -> *mut ::c_char;

    pub fn tmpnam(ptr: *mut ::c_char) -> *mut ::c_char;

    pub fn openlog(ident: *const ::c_char, logopt: ::c_int, facility: ::c_int);
    pub fn closelog();
    pub fn setlogmask(maskpri: ::c_int) -> ::c_int;
    #[cfg_attr(target_os = "macos", link_name = "syslog$DARWIN_EXTSN")]
    pub fn syslog(priority: ::c_int, message: *const ::c_char, ...);
    #[cfg_attr(
        all(target_os = "macos", target_arch = "x86"),
        link_name = "nice$UNIX2003"
    )]
    pub fn nice(incr: ::c_int) -> ::c_int;

    pub fn grantpt(fd: ::c_int) -> ::c_int;
    pub fn posix_openpt(flags: ::c_int) -> ::c_int;
    pub fn ptsname(fd: ::c_int) -> *mut ::c_char;
    pub fn unlockpt(fd: ::c_int) -> ::c_int;

    pub fn strcasestr(cs: *const c_char, ct: *const c_char) -> *mut c_char;
    pub fn getline(
        lineptr: *mut *mut c_char,
        n: *mut size_t,
        stream: *mut FILE,
    ) -> ssize_t;
}

cfg_if! {
    if #[cfg(not(target_os = "redox"))] {
        extern {
            pub fn getsid(pid: pid_t) -> pid_t;
            pub fn truncate(path: *const c_char, length: off_t) -> ::c_int;
            #[cfg_attr(all(target_os = "macos", target_arch = "x86"),
                       link_name = "pause$UNIX2003")]
            pub fn pause() -> ::c_int;

            pub fn readlinkat(dirfd: ::c_int,
                              pathname: *const ::c_char,
                              buf: *mut ::c_char,
                              bufsiz: ::size_t) -> ::ssize_t;
            pub fn mkdirat(dirfd: ::c_int, pathname: *const ::c_char,
                           mode: ::mode_t) -> ::c_int;
            pub fn openat(dirfd: ::c_int, pathname: *const ::c_char,
                          flags: ::c_int, ...) -> ::c_int;

            #[cfg_attr(all(target_os = "macos", target_arch = "x86_64"),
                       link_name = "fdopendir$INODE64")]
            #[cfg_attr(all(target_os = "macos", target_arch = "x86"),
                       link_name = "fdopendir$INODE64$UNIX2003")]
            pub fn fdopendir(fd: ::c_int) -> *mut ::DIR;

            #[cfg_attr(target_os = "macos", link_name = "readdir_r$INODE64")]
            #[cfg_attr(target_os = "netbsd", link_name = "__readdir_r30")]
            #[cfg_attr(
                all(target_os = "freebsd", any(freebsd11, freebsd10)),
                link_name = "readdir_r@FBSD_1.0"
            )]
            /// The 64-bit libc on Solaris and illumos only has readdir_r.  If a
            /// 32-bit Solaris or illumos target is ever created, it should use
            /// __posix_readdir_r.  See libc(3LIB) on Solaris or illumos:
            /// https://illumos.org/man/3lib/libc
            /// https://docs.oracle.com/cd/E36784_01/html/E36873/libc-3lib.html
            /// https://www.unix.com/man-page/opensolaris/3LIB/libc/
            pub fn readdir_r(dirp: *mut ::DIR, entry: *mut ::dirent,
                             result: *mut *mut ::dirent) -> ::c_int;
        }
    }
}

cfg_if! {
   if #[cfg(not(any(target_os = "solaris", target_os = "illumos")))] {
        extern {
            pub fn cfmakeraw(termios: *mut ::termios);
            pub fn cfsetspeed(termios: *mut ::termios,
                              speed: ::speed_t) -> ::c_int;
        }
   }
}

cfg_if! {
    if #[cfg(target_env = "uclibc")] {
        mod uclibc;
        pub use self::uclibc::*;
    } else if #[cfg(target_env = "newlib")] {
        mod newlib;
        pub use self::newlib::*;
    } else if #[cfg(any(target_os = "linux",
                        target_os = "android",
                        target_os = "emscripten"))] {
        mod linux_like;
        pub use self::linux_like::*;
    } else if #[cfg(any(target_os = "macos",
                        target_os = "ios",
                        target_os = "freebsd",
                        target_os = "dragonfly",
                        target_os = "openbsd",
                        target_os = "netbsd"))] {
        mod bsd;
        pub use self::bsd::*;
    } else if #[cfg(any(target_os = "solaris",
                        target_os = "illumos"))] {
        mod solarish;
        pub use self::solarish::*;
    } else if #[cfg(target_os = "haiku")] {
        mod haiku;
        pub use self::haiku::*;
    } else if #[cfg(target_os = "hermit")] {
        mod hermit;
        pub use self::hermit::*;
    } else if #[cfg(target_os = "redox")] {
        mod redox;
        pub use self::redox::*;
    } else {
        // Unknown target_os
    }
}

cfg_if! {
    if #[cfg(libc_core_cvoid)] {
        pub use ::ffi::c_void;
    } else {
        // Use repr(u8) as LLVM expects `void*` to be the same as `i8*` to help
        // enable more optimization opportunities around it recognizing things
        // like malloc/free.
        #[repr(u8)]
        #[allow(missing_copy_implementations)]
        #[allow(missing_debug_implementations)]
        pub enum c_void {
            // Two dummy variants so the #[repr] attribute can be used.
            #[doc(hidden)]
            __variant1,
            #[doc(hidden)]
            __variant2,
        }
    }
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
