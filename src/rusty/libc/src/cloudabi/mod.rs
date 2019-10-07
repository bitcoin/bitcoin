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

pub type in_addr_t = u32;
pub type in_port_t = u16;
pub type pthread_key_t = usize;
pub type pthread_t = usize;
pub type sa_family_t = u8;
pub type socklen_t = usize;
pub type time_t = i64;

s! {
    pub struct addrinfo {
        pub ai_flags: ::c_int,
        pub ai_family: ::c_int,
        pub ai_socktype: ::c_int,
        pub ai_protocol: ::c_int,
        pub ai_addrlen: ::socklen_t,
        pub ai_addr: *mut ::sockaddr,
        pub ai_canonname: *mut ::c_char,
        pub ai_next: *mut addrinfo,
    }

    pub struct in_addr {
        pub s_addr: in_addr_t,
    }

    pub struct in6_addr {
        pub s6_addr: [u8; 16],
    }

    pub struct pthread_attr_t {
        __detachstate: ::c_int,
        __stacksize: usize,
    }

    pub struct sockaddr {
        pub sa_family: sa_family_t,
        pub sa_data: [::c_char; 0],
    }

    pub struct sockaddr_in {
        pub sin_family: ::sa_family_t,
        pub sin_port: ::in_port_t,
        pub sin_addr: ::in_addr,
    }

    pub struct sockaddr_in6 {
        pub sin6_family: sa_family_t,
        pub sin6_port: ::in_port_t,
        pub sin6_flowinfo: u32,
        pub sin6_addr: ::in6_addr,
        pub sin6_scope_id: u32,
    }

    pub struct sockaddr_storage {
        pub ss_family: ::sa_family_t,
        __ss_data: [u8; 32],
    }
}

pub const INT_MIN: c_int = -2147483648;
pub const INT_MAX: c_int = 2147483647;

pub const _SC_NPROCESSORS_ONLN: ::c_int = 52;
pub const _SC_PAGESIZE: ::c_int = 54;

pub const AF_INET: ::c_int = 1;
pub const AF_INET6: ::c_int = 2;

pub const EACCES: ::c_int = 2;
pub const EADDRINUSE: ::c_int = 3;
pub const EADDRNOTAVAIL: ::c_int = 4;
pub const EAGAIN: ::c_int = 6;
pub const ECONNABORTED: ::c_int = 13;
pub const ECONNREFUSED: ::c_int = 14;
pub const ECONNRESET: ::c_int = 15;
pub const EEXIST: ::c_int = 20;
pub const EINTR: ::c_int = 27;
pub const EINVAL: ::c_int = 28;
pub const ENOENT: ::c_int = 44;
pub const ENOTCONN: ::c_int = 53;
pub const EPERM: ::c_int = 63;
pub const EPIPE: ::c_int = 64;
pub const ETIMEDOUT: ::c_int = 73;
pub const EWOULDBLOCK: ::c_int = EAGAIN;

pub const EAI_SYSTEM: ::c_int = 9;

pub const EXIT_FAILURE: ::c_int = 1;
pub const EXIT_SUCCESS: ::c_int = 0;

pub const PTHREAD_STACK_MIN: ::size_t = 1024;

pub const SOCK_DGRAM: ::c_int = 128;
pub const SOCK_STREAM: ::c_int = 130;

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
    pub fn fopen(filename: *const c_char, mode: *const c_char) -> *mut FILE;
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
    pub fn fputs(s: *const c_char, stream: *mut FILE) -> c_int;
    pub fn puts(s: *const c_char) -> c_int;
    pub fn ungetc(c: c_int, stream: *mut FILE) -> c_int;
    pub fn fread(
        ptr: *mut c_void,
        size: size_t,
        nobj: size_t,
        stream: *mut FILE,
    ) -> size_t;
    pub fn fwrite(
        ptr: *const c_void,
        size: size_t,
        nobj: size_t,
        stream: *mut FILE,
    ) -> size_t;
    pub fn fseek(stream: *mut FILE, offset: c_long, whence: c_int) -> c_int;
    pub fn ftell(stream: *mut FILE) -> c_long;
    pub fn rewind(stream: *mut FILE);
    pub fn fgetpos(stream: *mut FILE, ptr: *mut fpos_t) -> c_int;
    pub fn fsetpos(stream: *mut FILE, ptr: *const fpos_t) -> c_int;
    pub fn feof(stream: *mut FILE) -> c_int;
    pub fn ferror(stream: *mut FILE) -> c_int;
    pub fn perror(s: *const c_char);
    pub fn atoi(s: *const c_char) -> c_int;
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
    pub fn system(s: *const c_char) -> c_int;
    pub fn getenv(s: *const c_char) -> *mut c_char;
    pub fn getline(
        lineptr: *mut *mut c_char,
        n: *mut size_t,
        stream: *mut FILE,
    ) -> ssize_t;

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

    pub fn abs(i: c_int) -> c_int;
    pub fn atof(s: *const c_char) -> c_double;
    pub fn labs(i: c_long) -> c_long;
    pub fn rand() -> c_int;
    pub fn srand(seed: c_uint);

    pub fn arc4random_buf(buf: *const ::c_void, len: ::size_t);
    pub fn freeaddrinfo(res: *mut addrinfo);
    pub fn gai_strerror(errcode: ::c_int) -> *const ::c_char;
    pub fn getaddrinfo(
        node: *const c_char,
        service: *const c_char,
        hints: *const addrinfo,
        res: *mut *mut addrinfo,
    ) -> ::c_int;
    pub fn getsockopt(
        sockfd: ::c_int,
        level: ::c_int,
        optname: ::c_int,
        optval: *mut ::c_void,
        optlen: *mut ::socklen_t,
    ) -> ::c_int;
    pub fn posix_memalign(
        memptr: *mut *mut ::c_void,
        align: ::size_t,
        size: ::size_t,
    ) -> ::c_int;
    pub fn pthread_attr_destroy(attr: *mut ::pthread_attr_t) -> ::c_int;
    pub fn pthread_attr_init(attr: *mut ::pthread_attr_t) -> ::c_int;
    pub fn pthread_attr_setstacksize(
        attr: *mut ::pthread_attr_t,
        stack_size: ::size_t,
    ) -> ::c_int;
    pub fn pthread_create(
        native: *mut ::pthread_t,
        attr: *const ::pthread_attr_t,
        f: extern "C" fn(*mut ::c_void) -> *mut ::c_void,
        value: *mut ::c_void,
    ) -> ::c_int;
    pub fn pthread_detach(thread: ::pthread_t) -> ::c_int;
    pub fn pthread_getspecific(key: pthread_key_t) -> *mut ::c_void;
    pub fn pthread_join(
        native: ::pthread_t,
        value: *mut *mut ::c_void,
    ) -> ::c_int;
    pub fn pthread_key_create(
        key: *mut pthread_key_t,
        dtor: ::Option<unsafe extern "C" fn(*mut ::c_void)>,
    ) -> ::c_int;
    pub fn pthread_key_delete(key: pthread_key_t) -> ::c_int;
    pub fn pthread_setspecific(
        key: pthread_key_t,
        value: *const ::c_void,
    ) -> ::c_int;
    pub fn send(
        socket: ::c_int,
        buf: *const ::c_void,
        len: ::size_t,
        flags: ::c_int,
    ) -> ::ssize_t;
    pub fn sysconf(name: ::c_int) -> ::c_long;
}

cfg_if! {
    if #[cfg(target_arch = "aarch64")] {
        mod aarch64;
        pub use self::aarch64::*;
    } else if #[cfg(any(target_arch = "arm"))] {
        mod arm;
        pub use self::arm::*;
    } else if #[cfg(any(target_arch = "x86"))] {
        mod x86;
        pub use self::x86::*;
    } else if #[cfg(any(target_arch = "x86_64"))] {
        mod x86_64;
        pub use self::x86_64::*;
    } else {
        // Unknown target_arch
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
