pub type pthread_t = *mut ::c_void;
pub type clock_t = c_long;
pub type time_t = c_long;
pub type suseconds_t = c_long;
pub type ino_t = u64;
pub type off_t = i64;
pub type blkcnt_t = i64;

pub type shmatt_t = ::c_ulong;
pub type msgqnum_t = ::c_ulong;
pub type msglen_t = ::c_ulong;
pub type fsblkcnt_t = ::c_ulonglong;
pub type fsfilcnt_t = ::c_ulonglong;
pub type rlim_t = ::c_ulonglong;

impl siginfo_t {
    pub unsafe fn si_addr(&self) -> *mut ::c_void {
        #[repr(C)]
        struct siginfo_sigfault {
            _si_signo: ::c_int,
            _si_errno: ::c_int,
            _si_code: ::c_int,
            si_addr: *mut ::c_void,
        }

        (*(self as *const siginfo_t as *const siginfo_sigfault)).si_addr
    }

    pub unsafe fn si_value(&self) -> ::sigval {
        #[repr(C)]
        struct siginfo_si_value {
            _si_signo: ::c_int,
            _si_errno: ::c_int,
            _si_code: ::c_int,
            _si_timerid: ::c_int,
            _si_overrun: ::c_int,
            si_value: ::sigval,
        }

        (*(self as *const siginfo_t as *const siginfo_si_value)).si_value
    }
}

s! {
    pub struct aiocb {
        pub aio_fildes: ::c_int,
        pub aio_lio_opcode: ::c_int,
        pub aio_reqprio: ::c_int,
        pub aio_buf: *mut ::c_void,
        pub aio_nbytes: ::size_t,
        pub aio_sigevent: ::sigevent,
        __td: *mut ::c_void,
        __lock: [::c_int; 2],
        __err: ::c_int,
        __ret: ::ssize_t,
        pub aio_offset: off_t,
        __next: *mut ::c_void,
        __prev: *mut ::c_void,
        #[cfg(target_pointer_width = "32")]
        __dummy4: [::c_char; 24],
        #[cfg(target_pointer_width = "64")]
        __dummy4: [::c_char; 16],
    }

    pub struct sigaction {
        pub sa_sigaction: ::sighandler_t,
        pub sa_mask: ::sigset_t,
        pub sa_flags: ::c_int,
        pub sa_restorer: ::Option<extern fn()>,
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

    pub struct termios {
        pub c_iflag: ::tcflag_t,
        pub c_oflag: ::tcflag_t,
        pub c_cflag: ::tcflag_t,
        pub c_lflag: ::tcflag_t,
        pub c_line: ::cc_t,
        pub c_cc: [::cc_t; ::NCCS],
        pub __c_ispeed: ::speed_t,
        pub __c_ospeed: ::speed_t,
    }

    pub struct flock {
        pub l_type: ::c_short,
        pub l_whence: ::c_short,
        pub l_start: ::off_t,
        pub l_len: ::off_t,
        pub l_pid: ::pid_t,
    }
}

s_no_extra_traits! {
    pub struct sysinfo {
        pub uptime: ::c_ulong,
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
        pub __reserved: [::c_char; 256],
    }
}

cfg_if! {
    if #[cfg(feature = "extra_traits")] {
        impl PartialEq for sysinfo {
            fn eq(&self, other: &sysinfo) -> bool {
                self.uptime == other.uptime
                    && self.loads == other.loads
                    && self.totalram == other.totalram
                    && self.freeram == other.freeram
                    && self.sharedram == other.sharedram
                    && self.bufferram == other.bufferram
                    && self.totalswap == other.totalswap
                    && self.freeswap == other.freeswap
                    && self.procs == other.procs
                    && self.pad == other.pad
                    && self.totalhigh == other.totalhigh
                    && self.freehigh == other.freehigh
                    && self.mem_unit == other.mem_unit
                    && self
                        .__reserved
                        .iter()
                        .zip(other.__reserved.iter())
                        .all(|(a,b)| a == b)
            }
        }

        impl Eq for sysinfo {}

        impl ::fmt::Debug for sysinfo {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("sysinfo")
                    .field("uptime", &self.uptime)
                    .field("loads", &self.loads)
                    .field("totalram", &self.totalram)
                    .field("freeram", &self.freeram)
                    .field("sharedram", &self.sharedram)
                    .field("bufferram", &self.bufferram)
                    .field("totalswap", &self.totalswap)
                    .field("freeswap", &self.freeswap)
                    .field("procs", &self.procs)
                    .field("pad", &self.pad)
                    .field("totalhigh", &self.totalhigh)
                    .field("freehigh", &self.freehigh)
                    .field("mem_unit", &self.mem_unit)
                    // FIXME: .field("__reserved", &self.__reserved)
                    .finish()
            }
        }

        impl ::hash::Hash for sysinfo {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.uptime.hash(state);
                self.loads.hash(state);
                self.totalram.hash(state);
                self.freeram.hash(state);
                self.sharedram.hash(state);
                self.bufferram.hash(state);
                self.totalswap.hash(state);
                self.freeswap.hash(state);
                self.procs.hash(state);
                self.pad.hash(state);
                self.totalhigh.hash(state);
                self.freehigh.hash(state);
                self.mem_unit.hash(state);
                self.__reserved.hash(state);
            }
        }
    }
}

pub const MS_RMT_MASK: ::c_ulong = 0x02800051;

pub const SFD_CLOEXEC: ::c_int = 0x080000;

pub const NCCS: usize = 32;

pub const O_TRUNC: ::c_int = 512;
pub const O_NOATIME: ::c_int = 0o1000000;
pub const O_CLOEXEC: ::c_int = 0x80000;
pub const O_TMPFILE: ::c_int = 0o20000000 | O_DIRECTORY;

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
pub const EDOTDOT: ::c_int = 73;

pub const F_RDLCK: ::c_int = 0;
pub const F_WRLCK: ::c_int = 1;
pub const F_UNLCK: ::c_int = 2;

pub const SA_NODEFER: ::c_int = 0x40000000;
pub const SA_RESETHAND: ::c_int = 0x80000000;
pub const SA_RESTART: ::c_int = 0x10000000;
pub const SA_NOCLDSTOP: ::c_int = 0x00000001;

pub const EPOLL_CLOEXEC: ::c_int = 0x80000;

pub const EFD_CLOEXEC: ::c_int = 0x80000;

pub const BUFSIZ: ::c_uint = 1024;
pub const TMP_MAX: ::c_uint = 10000;
pub const FOPEN_MAX: ::c_uint = 1000;
pub const O_PATH: ::c_int = 0o10000000;
pub const O_EXEC: ::c_int = 0o10000000;
pub const O_SEARCH: ::c_int = 0o10000000;
pub const O_ACCMODE: ::c_int = 0o10000003;
pub const O_NDELAY: ::c_int = O_NONBLOCK;
pub const NI_MAXHOST: ::socklen_t = 255;
pub const PTHREAD_STACK_MIN: ::size_t = 2048;
pub const POSIX_FADV_DONTNEED: ::c_int = 4;
pub const POSIX_FADV_NOREUSE: ::c_int = 5;

pub const POSIX_MADV_DONTNEED: ::c_int = 4;

pub const RLIM_INFINITY: ::rlim_t = !0;
pub const RLIMIT_RTTIME: ::c_int = 15;

pub const MAP_ANONYMOUS: ::c_int = MAP_ANON;

pub const SOCK_DCCP: ::c_int = 6;
pub const SOCK_PACKET: ::c_int = 10;

pub const TCP_COOKIE_TRANSACTIONS: ::c_int = 15;
pub const TCP_THIN_LINEAR_TIMEOUTS: ::c_int = 16;
pub const TCP_THIN_DUPACK: ::c_int = 17;
pub const TCP_USER_TIMEOUT: ::c_int = 18;
pub const TCP_REPAIR: ::c_int = 19;
pub const TCP_REPAIR_QUEUE: ::c_int = 20;
pub const TCP_QUEUE_SEQ: ::c_int = 21;
pub const TCP_REPAIR_OPTIONS: ::c_int = 22;
pub const TCP_FASTOPEN: ::c_int = 23;
pub const TCP_TIMESTAMP: ::c_int = 24;

#[deprecated(since = "0.2.55", note = "Use SIGSYS instead")]
pub const SIGUNUSED: ::c_int = ::SIGSYS;

pub const __SIZEOF_PTHREAD_CONDATTR_T: usize = 4;
pub const __SIZEOF_PTHREAD_MUTEXATTR_T: usize = 4;
pub const __SIZEOF_PTHREAD_RWLOCKATTR_T: usize = 8;

pub const CPU_SETSIZE: ::c_int = 128;

pub const PTRACE_TRACEME: ::c_int = 0;
pub const PTRACE_PEEKTEXT: ::c_int = 1;
pub const PTRACE_PEEKDATA: ::c_int = 2;
pub const PTRACE_PEEKUSER: ::c_int = 3;
pub const PTRACE_POKETEXT: ::c_int = 4;
pub const PTRACE_POKEDATA: ::c_int = 5;
pub const PTRACE_POKEUSER: ::c_int = 6;
pub const PTRACE_CONT: ::c_int = 7;
pub const PTRACE_KILL: ::c_int = 8;
pub const PTRACE_SINGLESTEP: ::c_int = 9;
pub const PTRACE_GETREGS: ::c_int = 12;
pub const PTRACE_SETREGS: ::c_int = 13;
pub const PTRACE_GETFPREGS: ::c_int = 14;
pub const PTRACE_SETFPREGS: ::c_int = 15;
pub const PTRACE_ATTACH: ::c_int = 16;
pub const PTRACE_DETACH: ::c_int = 17;
pub const PTRACE_GETFPXREGS: ::c_int = 18;
pub const PTRACE_SETFPXREGS: ::c_int = 19;
pub const PTRACE_SYSCALL: ::c_int = 24;
pub const PTRACE_SETOPTIONS: ::c_int = 0x4200;
pub const PTRACE_GETEVENTMSG: ::c_int = 0x4201;
pub const PTRACE_GETSIGINFO: ::c_int = 0x4202;
pub const PTRACE_SETSIGINFO: ::c_int = 0x4203;
pub const PTRACE_GETREGSET: ::c_int = 0x4204;
pub const PTRACE_SETREGSET: ::c_int = 0x4205;
pub const PTRACE_SEIZE: ::c_int = 0x4206;
pub const PTRACE_INTERRUPT: ::c_int = 0x4207;
pub const PTRACE_LISTEN: ::c_int = 0x4208;
pub const PTRACE_PEEKSIGINFO: ::c_int = 0x4209;

pub const EPOLLWAKEUP: ::c_int = 0x20000000;

pub const SEEK_DATA: ::c_int = 3;
pub const SEEK_HOLE: ::c_int = 4;

pub const EFD_NONBLOCK: ::c_int = ::O_NONBLOCK;

pub const SFD_NONBLOCK: ::c_int = ::O_NONBLOCK;

pub const TCSANOW: ::c_int = 0;
pub const TCSADRAIN: ::c_int = 1;
pub const TCSAFLUSH: ::c_int = 2;

pub const RTLD_GLOBAL: ::c_int = 0x100;
pub const RTLD_NOLOAD: ::c_int = 0x4;

// TODO(#247) Temporarily musl-specific (available since musl 0.9.12 / Linux
// kernel 3.10).  See also linux_like/mod.rs
pub const CLOCK_SGI_CYCLE: ::clockid_t = 10;
pub const CLOCK_TAI: ::clockid_t = 11;

pub const B0: ::speed_t = 0o000000;
pub const B50: ::speed_t = 0o000001;
pub const B75: ::speed_t = 0o000002;
pub const B110: ::speed_t = 0o000003;
pub const B134: ::speed_t = 0o000004;
pub const B150: ::speed_t = 0o000005;
pub const B200: ::speed_t = 0o000006;
pub const B300: ::speed_t = 0o000007;
pub const B600: ::speed_t = 0o000010;
pub const B1200: ::speed_t = 0o000011;
pub const B1800: ::speed_t = 0o000012;
pub const B2400: ::speed_t = 0o000013;
pub const B4800: ::speed_t = 0o000014;
pub const B9600: ::speed_t = 0o000015;
pub const B19200: ::speed_t = 0o000016;
pub const B38400: ::speed_t = 0o000017;
pub const EXTA: ::speed_t = B19200;
pub const EXTB: ::speed_t = B38400;

pub const SO_BINDTODEVICE: ::c_int = 25;
pub const SO_TIMESTAMP: ::c_int = 29;
pub const SO_MARK: ::c_int = 36;
pub const SO_RXQ_OVFL: ::c_int = 40;
pub const SO_PEEK_OFF: ::c_int = 42;
pub const SO_BUSY_POLL: ::c_int = 46;

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

extern "C" {
    pub fn sendmmsg(
        sockfd: ::c_int,
        msgvec: *mut ::mmsghdr,
        vlen: ::c_uint,
        flags: ::c_uint,
    ) -> ::c_int;
    pub fn recvmmsg(
        sockfd: ::c_int,
        msgvec: *mut ::mmsghdr,
        vlen: ::c_uint,
        flags: ::c_uint,
        timeout: *mut ::timespec,
    ) -> ::c_int;

    pub fn getrlimit64(resource: ::c_int, rlim: *mut ::rlimit64) -> ::c_int;
    pub fn setrlimit64(resource: ::c_int, rlim: *const ::rlimit64) -> ::c_int;
    pub fn getrlimit(resource: ::c_int, rlim: *mut ::rlimit) -> ::c_int;
    pub fn setrlimit(resource: ::c_int, rlim: *const ::rlimit) -> ::c_int;
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

    pub fn gettimeofday(tp: *mut ::timeval, tz: *mut ::c_void) -> ::c_int;
    pub fn ptrace(request: ::c_int, ...) -> ::c_long;
    pub fn getpriority(which: ::c_int, who: ::id_t) -> ::c_int;
    pub fn setpriority(which: ::c_int, who: ::id_t, prio: ::c_int) -> ::c_int;
    pub fn pthread_getaffinity_np(
        thread: ::pthread_t,
        cpusetsize: ::size_t,
        cpuset: *mut ::cpu_set_t,
    ) -> ::c_int;
    pub fn pthread_setaffinity_np(
        thread: ::pthread_t,
        cpusetsize: ::size_t,
        cpuset: *const ::cpu_set_t,
    ) -> ::c_int;
    pub fn sched_getcpu() -> ::c_int;
}

cfg_if! {
    if #[cfg(any(target_arch = "x86_64",
                 target_arch = "aarch64",
                 target_arch = "mips64",
                 target_arch = "powerpc64"))] {
        mod b64;
        pub use self::b64::*;
    } else if #[cfg(any(target_arch = "x86",
                        target_arch = "mips",
                        target_arch = "powerpc",
                        target_arch = "hexagon",
                        target_arch = "arm"))] {
        mod b32;
        pub use self::b32::*;
    } else { }
}
