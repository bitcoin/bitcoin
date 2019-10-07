pub type pthread_t = c_ulong;
pub type __priority_which_t = ::c_uint;
pub type __rlimit_resource_t = ::c_uint;

s! {
    pub struct statx {
        pub stx_mask: u32,
        pub stx_blksize: u32,
        pub stx_attributes: u64,
        pub stx_nlink: u32,
        pub stx_uid: u32,
        pub stx_gid: u32,
        pub stx_mode: u16,
        pub __statx_pad1: [u16; 1],
        pub stx_ino: u64,
        pub stx_size: u64,
        pub stx_blocks: u64,
        pub stx_attributes_mask: u64,
        pub stx_atime: ::statx_timestamp,
        pub stx_btime: ::statx_timestamp,
        pub stx_ctime: ::statx_timestamp,
        pub stx_mtime: ::statx_timestamp,
        pub stx_rdev_major: u32,
        pub stx_rdev_minor: u32,
        pub stx_dev_major: u32,
        pub stx_dev_minor: u32,
        pub __statx_pad2: [u64; 14],
    }

    pub struct statx_timestamp {
        pub tv_sec: i64,
        pub tv_nsec: u32,
        pub __statx_timestamp_pad1: [i32; 1],
    }

    pub struct aiocb {
        pub aio_fildes: ::c_int,
        pub aio_lio_opcode: ::c_int,
        pub aio_reqprio: ::c_int,
        pub aio_buf: *mut ::c_void,
        pub aio_nbytes: ::size_t,
        pub aio_sigevent: ::sigevent,
        __next_prio: *mut aiocb,
        __abs_prio: ::c_int,
        __policy: ::c_int,
        __error_code: ::c_int,
        __return_value: ::ssize_t,
        pub aio_offset: off_t,
        #[cfg(all(not(target_arch = "x86_64"), target_pointer_width = "32"))]
        __unused1: [::c_char; 4],
        __glibc_reserved: [::c_char; 32]
    }

    pub struct __exit_status {
        pub e_termination: ::c_short,
        pub e_exit: ::c_short,
    }

    pub struct __timeval {
        pub tv_sec: i32,
        pub tv_usec: i32,
    }

    pub struct glob64_t {
        pub gl_pathc: ::size_t,
        pub gl_pathv: *mut *mut ::c_char,
        pub gl_offs: ::size_t,
        pub gl_flags: ::c_int,

        __unused1: *mut ::c_void,
        __unused2: *mut ::c_void,
        __unused3: *mut ::c_void,
        __unused4: *mut ::c_void,
        __unused5: *mut ::c_void,
    }

    pub struct msghdr {
        pub msg_name: *mut ::c_void,
        pub msg_namelen: ::socklen_t,
        pub msg_iov: *mut ::iovec,
        pub msg_iovlen: ::size_t,
        pub msg_control: *mut ::c_void,
        pub msg_controllen: ::size_t,
        pub msg_flags: ::c_int,
    }

    pub struct cmsghdr {
        pub cmsg_len: ::size_t,
        pub cmsg_level: ::c_int,
        pub cmsg_type: ::c_int,
    }

    pub struct termios {
        pub c_iflag: ::tcflag_t,
        pub c_oflag: ::tcflag_t,
        pub c_cflag: ::tcflag_t,
        pub c_lflag: ::tcflag_t,
        pub c_line: ::cc_t,
        pub c_cc: [::cc_t; ::NCCS],
        #[cfg(not(any(
            target_arch = "sparc64",
            target_arch = "mips",
            target_arch = "mips64")))]
        pub c_ispeed: ::speed_t,
        #[cfg(not(any(
            target_arch = "sparc64",
            target_arch = "mips",
            target_arch = "mips64")))]
        pub c_ospeed: ::speed_t,
    }

    pub struct mallinfo {
        pub arena: ::c_int,
        pub ordblks: ::c_int,
        pub smblks: ::c_int,
        pub hblks: ::c_int,
        pub hblkhd: ::c_int,
        pub usmblks: ::c_int,
        pub fsmblks: ::c_int,
        pub uordblks: ::c_int,
        pub fordblks: ::c_int,
        pub keepcost: ::c_int,
    }

    pub struct nlmsghdr {
        pub nlmsg_len: u32,
        pub nlmsg_type: u16,
        pub nlmsg_flags: u16,
        pub nlmsg_seq: u32,
        pub nlmsg_pid: u32,
    }

    pub struct nlmsgerr {
        pub error: ::c_int,
        pub msg: nlmsghdr,
    }

    pub struct nl_pktinfo {
        pub group: u32,
    }

    pub struct nl_mmap_req {
        pub nm_block_size: ::c_uint,
        pub nm_block_nr: ::c_uint,
        pub nm_frame_size: ::c_uint,
        pub nm_frame_nr: ::c_uint,
    }

    pub struct nl_mmap_hdr {
        pub nm_status: ::c_uint,
        pub nm_len: ::c_uint,
        pub nm_group: u32,
        pub nm_pid: u32,
        pub nm_uid: u32,
        pub nm_gid: u32,
    }

    pub struct nlattr {
        pub nla_len: u16,
        pub nla_type: u16,
    }

    pub struct rtentry {
        pub rt_pad1: ::c_ulong,
        pub rt_dst: ::sockaddr,
        pub rt_gateway: ::sockaddr,
        pub rt_genmask: ::sockaddr,
        pub rt_flags: ::c_ushort,
        pub rt_pad2: ::c_short,
        pub rt_pad3: ::c_ulong,
        pub rt_tos: ::c_uchar,
        pub rt_class: ::c_uchar,
        #[cfg(target_pointer_width = "64")]
        pub rt_pad4: [::c_short; 3usize],
        #[cfg(not(target_pointer_width = "64"))]
        pub rt_pad4: ::c_short,
        pub rt_metric: ::c_short,
        pub rt_dev: *mut ::c_char,
        pub rt_mtu: ::c_ulong,
        pub rt_window: ::c_ulong,
        pub rt_irtt: ::c_ushort,
    }
}

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
        struct siginfo_timer {
            _si_signo: ::c_int,
            _si_errno: ::c_int,
            _si_code: ::c_int,
            _si_tid: ::c_int,
            _si_overrun: ::c_int,
            si_sigval: ::sigval,
        }
        (*(self as *const siginfo_t as *const siginfo_timer)).si_sigval
    }
}

s_no_extra_traits! {
    pub struct utmpx {
        pub ut_type: ::c_short,
        pub ut_pid: ::pid_t,
        pub ut_line: [::c_char; __UT_LINESIZE],
        pub ut_id: [::c_char; 4],

        pub ut_user: [::c_char; __UT_NAMESIZE],
        pub ut_host: [::c_char; __UT_HOSTSIZE],
        pub ut_exit: __exit_status,

        #[cfg(any(target_arch = "aarch64",
                  target_arch = "s390x",
                  all(target_pointer_width = "32",
                      not(target_arch = "x86_64"))))]
        pub ut_session: ::c_long,
        #[cfg(any(target_arch = "aarch64",
                  target_arch = "s390x",
                  all(target_pointer_width = "32",
                      not(target_arch = "x86_64"))))]
        pub ut_tv: ::timeval,

        #[cfg(not(any(target_arch = "aarch64",
                      target_arch = "s390x",
                      all(target_pointer_width = "32",
                          not(target_arch = "x86_64")))))]
        pub ut_session: i32,
        #[cfg(not(any(target_arch = "aarch64",
                      target_arch = "s390x",
                      all(target_pointer_width = "32",
                          not(target_arch = "x86_64")))))]
        pub ut_tv: __timeval,

        pub ut_addr_v6: [i32; 4],
        __glibc_reserved: [::c_char; 20],
    }
}

cfg_if! {
    if #[cfg(feature = "extra_traits")] {
        impl PartialEq for utmpx {
            fn eq(&self, other: &utmpx) -> bool {
                self.ut_type == other.ut_type
                    && self.ut_pid == other.ut_pid
                    && self.ut_line == other.ut_line
                    && self.ut_id == other.ut_id
                    && self.ut_user == other.ut_user
                    && self
                    .ut_host
                    .iter()
                    .zip(other.ut_host.iter())
                    .all(|(a,b)| a == b)
                    && self.ut_exit == other.ut_exit
                    && self.ut_session == other.ut_session
                    && self.ut_tv == other.ut_tv
                    && self.ut_addr_v6 == other.ut_addr_v6
                    && self.__glibc_reserved == other.__glibc_reserved
            }
        }

        impl Eq for utmpx {}

        impl ::fmt::Debug for utmpx {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("utmpx")
                    .field("ut_type", &self.ut_type)
                    .field("ut_pid", &self.ut_pid)
                    .field("ut_line", &self.ut_line)
                    .field("ut_id", &self.ut_id)
                    .field("ut_user", &self.ut_user)
                // FIXME: .field("ut_host", &self.ut_host)
                    .field("ut_exit", &self.ut_exit)
                    .field("ut_session", &self.ut_session)
                    .field("ut_tv", &self.ut_tv)
                    .field("ut_addr_v6", &self.ut_addr_v6)
                    .field("__glibc_reserved", &self.__glibc_reserved)
                    .finish()
            }
        }

        impl ::hash::Hash for utmpx {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.ut_type.hash(state);
                self.ut_pid.hash(state);
                self.ut_line.hash(state);
                self.ut_id.hash(state);
                self.ut_user.hash(state);
                self.ut_host.hash(state);
                self.ut_exit.hash(state);
                self.ut_session.hash(state);
                self.ut_tv.hash(state);
                self.ut_addr_v6.hash(state);
                self.__glibc_reserved.hash(state);
            }
        }
    }
}

pub const RLIMIT_CPU: ::__rlimit_resource_t = 0;
pub const RLIMIT_FSIZE: ::__rlimit_resource_t = 1;
pub const RLIMIT_DATA: ::__rlimit_resource_t = 2;
pub const RLIMIT_STACK: ::__rlimit_resource_t = 3;
pub const RLIMIT_CORE: ::__rlimit_resource_t = 4;
pub const RLIMIT_LOCKS: ::__rlimit_resource_t = 10;
pub const RLIMIT_SIGPENDING: ::__rlimit_resource_t = 11;
pub const RLIMIT_MSGQUEUE: ::__rlimit_resource_t = 12;
pub const RLIMIT_NICE: ::__rlimit_resource_t = 13;
pub const RLIMIT_RTPRIO: ::__rlimit_resource_t = 14;
pub const RLIMIT_RTTIME: ::__rlimit_resource_t = 15;
pub const RLIMIT_NLIMITS: ::__rlimit_resource_t = 16;

pub const MS_RMT_MASK: ::c_ulong = 0x02800051;

pub const __UT_LINESIZE: usize = 32;
pub const __UT_NAMESIZE: usize = 32;
pub const __UT_HOSTSIZE: usize = 256;
pub const EMPTY: ::c_short = 0;
pub const RUN_LVL: ::c_short = 1;
pub const BOOT_TIME: ::c_short = 2;
pub const NEW_TIME: ::c_short = 3;
pub const OLD_TIME: ::c_short = 4;
pub const INIT_PROCESS: ::c_short = 5;
pub const LOGIN_PROCESS: ::c_short = 6;
pub const USER_PROCESS: ::c_short = 7;
pub const DEAD_PROCESS: ::c_short = 8;
pub const ACCOUNTING: ::c_short = 9;

pub const SOCK_NONBLOCK: ::c_int = O_NONBLOCK;

pub const SOL_RXRPC: ::c_int = 272;
pub const SOL_PPPOL2TP: ::c_int = 273;
pub const SOL_PNPIPE: ::c_int = 275;
pub const SOL_RDS: ::c_int = 276;
pub const SOL_IUCV: ::c_int = 277;
pub const SOL_CAIF: ::c_int = 278;
pub const SOL_NFC: ::c_int = 280;
pub const SOL_XDP: ::c_int = 283;

pub const MSG_TRYHARD: ::c_int = 4;

pub const LC_PAPER: ::c_int = 7;
pub const LC_NAME: ::c_int = 8;
pub const LC_ADDRESS: ::c_int = 9;
pub const LC_TELEPHONE: ::c_int = 10;
pub const LC_MEASUREMENT: ::c_int = 11;
pub const LC_IDENTIFICATION: ::c_int = 12;
pub const LC_PAPER_MASK: ::c_int = (1 << LC_PAPER);
pub const LC_NAME_MASK: ::c_int = (1 << LC_NAME);
pub const LC_ADDRESS_MASK: ::c_int = (1 << LC_ADDRESS);
pub const LC_TELEPHONE_MASK: ::c_int = (1 << LC_TELEPHONE);
pub const LC_MEASUREMENT_MASK: ::c_int = (1 << LC_MEASUREMENT);
pub const LC_IDENTIFICATION_MASK: ::c_int = (1 << LC_IDENTIFICATION);
pub const LC_ALL_MASK: ::c_int = ::LC_CTYPE_MASK
    | ::LC_NUMERIC_MASK
    | ::LC_TIME_MASK
    | ::LC_COLLATE_MASK
    | ::LC_MONETARY_MASK
    | ::LC_MESSAGES_MASK
    | LC_PAPER_MASK
    | LC_NAME_MASK
    | LC_ADDRESS_MASK
    | LC_TELEPHONE_MASK
    | LC_MEASUREMENT_MASK
    | LC_IDENTIFICATION_MASK;

pub const MAP_SHARED_VALIDATE: ::c_int = 0x3;
pub const MAP_FIXED_NOREPLACE: ::c_int = 0x100000;

pub const ENOTSUP: ::c_int = EOPNOTSUPP;

pub const SOCK_SEQPACKET: ::c_int = 5;
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

/* DCCP socket options */
pub const DCCP_SOCKOPT_PACKET_SIZE: ::c_int = 1;
pub const DCCP_SOCKOPT_SERVICE: ::c_int = 2;
pub const DCCP_SOCKOPT_CHANGE_L: ::c_int = 3;
pub const DCCP_SOCKOPT_CHANGE_R: ::c_int = 4;
pub const DCCP_SOCKOPT_GET_CUR_MPS: ::c_int = 5;
pub const DCCP_SOCKOPT_SERVER_TIMEWAIT: ::c_int = 6;
pub const DCCP_SOCKOPT_SEND_CSCOV: ::c_int = 10;
pub const DCCP_SOCKOPT_RECV_CSCOV: ::c_int = 11;
pub const DCCP_SOCKOPT_AVAILABLE_CCIDS: ::c_int = 12;
pub const DCCP_SOCKOPT_CCID: ::c_int = 13;
pub const DCCP_SOCKOPT_TX_CCID: ::c_int = 14;
pub const DCCP_SOCKOPT_RX_CCID: ::c_int = 15;
pub const DCCP_SOCKOPT_QPOLICY_ID: ::c_int = 16;
pub const DCCP_SOCKOPT_QPOLICY_TXQLEN: ::c_int = 17;
pub const DCCP_SOCKOPT_CCID_RX_INFO: ::c_int = 128;
pub const DCCP_SOCKOPT_CCID_TX_INFO: ::c_int = 192;

/// maximum number of services provided on the same listening port
pub const DCCP_SERVICE_LIST_MAX_LEN: ::c_int = 32;

pub const SIGEV_THREAD_ID: ::c_int = 4;

pub const BUFSIZ: ::c_uint = 8192;
pub const TMP_MAX: ::c_uint = 238328;
pub const FOPEN_MAX: ::c_uint = 16;
pub const POSIX_MADV_DONTNEED: ::c_int = 4;
pub const _SC_EQUIV_CLASS_MAX: ::c_int = 41;
pub const _SC_CHARCLASS_NAME_MAX: ::c_int = 45;
pub const _SC_PII: ::c_int = 53;
pub const _SC_PII_XTI: ::c_int = 54;
pub const _SC_PII_SOCKET: ::c_int = 55;
pub const _SC_PII_INTERNET: ::c_int = 56;
pub const _SC_PII_OSI: ::c_int = 57;
pub const _SC_POLL: ::c_int = 58;
pub const _SC_SELECT: ::c_int = 59;
pub const _SC_PII_INTERNET_STREAM: ::c_int = 61;
pub const _SC_PII_INTERNET_DGRAM: ::c_int = 62;
pub const _SC_PII_OSI_COTS: ::c_int = 63;
pub const _SC_PII_OSI_CLTS: ::c_int = 64;
pub const _SC_PII_OSI_M: ::c_int = 65;
pub const _SC_T_IOV_MAX: ::c_int = 66;
pub const _SC_2_C_VERSION: ::c_int = 96;
pub const _SC_CHAR_BIT: ::c_int = 101;
pub const _SC_CHAR_MAX: ::c_int = 102;
pub const _SC_CHAR_MIN: ::c_int = 103;
pub const _SC_INT_MAX: ::c_int = 104;
pub const _SC_INT_MIN: ::c_int = 105;
pub const _SC_LONG_BIT: ::c_int = 106;
pub const _SC_WORD_BIT: ::c_int = 107;
pub const _SC_MB_LEN_MAX: ::c_int = 108;
pub const _SC_SSIZE_MAX: ::c_int = 110;
pub const _SC_SCHAR_MAX: ::c_int = 111;
pub const _SC_SCHAR_MIN: ::c_int = 112;
pub const _SC_SHRT_MAX: ::c_int = 113;
pub const _SC_SHRT_MIN: ::c_int = 114;
pub const _SC_UCHAR_MAX: ::c_int = 115;
pub const _SC_UINT_MAX: ::c_int = 116;
pub const _SC_ULONG_MAX: ::c_int = 117;
pub const _SC_USHRT_MAX: ::c_int = 118;
pub const _SC_NL_ARGMAX: ::c_int = 119;
pub const _SC_NL_LANGMAX: ::c_int = 120;
pub const _SC_NL_MSGMAX: ::c_int = 121;
pub const _SC_NL_NMAX: ::c_int = 122;
pub const _SC_NL_SETMAX: ::c_int = 123;
pub const _SC_NL_TEXTMAX: ::c_int = 124;
pub const _SC_BASE: ::c_int = 134;
pub const _SC_C_LANG_SUPPORT: ::c_int = 135;
pub const _SC_C_LANG_SUPPORT_R: ::c_int = 136;
pub const _SC_DEVICE_IO: ::c_int = 140;
pub const _SC_DEVICE_SPECIFIC: ::c_int = 141;
pub const _SC_DEVICE_SPECIFIC_R: ::c_int = 142;
pub const _SC_FD_MGMT: ::c_int = 143;
pub const _SC_FIFO: ::c_int = 144;
pub const _SC_PIPE: ::c_int = 145;
pub const _SC_FILE_ATTRIBUTES: ::c_int = 146;
pub const _SC_FILE_LOCKING: ::c_int = 147;
pub const _SC_FILE_SYSTEM: ::c_int = 148;
pub const _SC_MULTI_PROCESS: ::c_int = 150;
pub const _SC_SINGLE_PROCESS: ::c_int = 151;
pub const _SC_NETWORKING: ::c_int = 152;
pub const _SC_REGEX_VERSION: ::c_int = 156;
pub const _SC_SIGNALS: ::c_int = 158;
pub const _SC_SYSTEM_DATABASE: ::c_int = 162;
pub const _SC_SYSTEM_DATABASE_R: ::c_int = 163;
pub const _SC_USER_GROUPS: ::c_int = 166;
pub const _SC_USER_GROUPS_R: ::c_int = 167;
pub const _SC_LEVEL1_ICACHE_SIZE: ::c_int = 185;
pub const _SC_LEVEL1_ICACHE_ASSOC: ::c_int = 186;
pub const _SC_LEVEL1_ICACHE_LINESIZE: ::c_int = 187;
pub const _SC_LEVEL1_DCACHE_SIZE: ::c_int = 188;
pub const _SC_LEVEL1_DCACHE_ASSOC: ::c_int = 189;
pub const _SC_LEVEL1_DCACHE_LINESIZE: ::c_int = 190;
pub const _SC_LEVEL2_CACHE_SIZE: ::c_int = 191;
pub const _SC_LEVEL2_CACHE_ASSOC: ::c_int = 192;
pub const _SC_LEVEL2_CACHE_LINESIZE: ::c_int = 193;
pub const _SC_LEVEL3_CACHE_SIZE: ::c_int = 194;
pub const _SC_LEVEL3_CACHE_ASSOC: ::c_int = 195;
pub const _SC_LEVEL3_CACHE_LINESIZE: ::c_int = 196;
pub const _SC_LEVEL4_CACHE_SIZE: ::c_int = 197;
pub const _SC_LEVEL4_CACHE_ASSOC: ::c_int = 198;
pub const _SC_LEVEL4_CACHE_LINESIZE: ::c_int = 199;
pub const O_ACCMODE: ::c_int = 3;
pub const ST_RELATIME: ::c_ulong = 4096;
pub const NI_MAXHOST: ::socklen_t = 1025;

pub const ADFS_SUPER_MAGIC: ::c_long = 0x0000adf5;
pub const AFFS_SUPER_MAGIC: ::c_long = 0x0000adff;
pub const CODA_SUPER_MAGIC: ::c_long = 0x73757245;
pub const CRAMFS_MAGIC: ::c_long = 0x28cd3d45;
pub const EFS_SUPER_MAGIC: ::c_long = 0x00414a53;
pub const EXT2_SUPER_MAGIC: ::c_long = 0x0000ef53;
pub const EXT3_SUPER_MAGIC: ::c_long = 0x0000ef53;
pub const EXT4_SUPER_MAGIC: ::c_long = 0x0000ef53;
pub const HPFS_SUPER_MAGIC: ::c_long = 0xf995e849;
pub const HUGETLBFS_MAGIC: ::c_long = 0x958458f6;
pub const ISOFS_SUPER_MAGIC: ::c_long = 0x00009660;
pub const JFFS2_SUPER_MAGIC: ::c_long = 0x000072b6;
pub const MINIX_SUPER_MAGIC: ::c_long = 0x0000137f;
pub const MINIX_SUPER_MAGIC2: ::c_long = 0x0000138f;
pub const MINIX2_SUPER_MAGIC: ::c_long = 0x00002468;
pub const MINIX2_SUPER_MAGIC2: ::c_long = 0x00002478;
pub const MSDOS_SUPER_MAGIC: ::c_long = 0x00004d44;
pub const NCP_SUPER_MAGIC: ::c_long = 0x0000564c;
pub const NFS_SUPER_MAGIC: ::c_long = 0x00006969;
pub const OPENPROM_SUPER_MAGIC: ::c_long = 0x00009fa1;
pub const PROC_SUPER_MAGIC: ::c_long = 0x00009fa0;
pub const QNX4_SUPER_MAGIC: ::c_long = 0x0000002f;
pub const REISERFS_SUPER_MAGIC: ::c_long = 0x52654973;
pub const SMB_SUPER_MAGIC: ::c_long = 0x0000517b;
pub const TMPFS_MAGIC: ::c_long = 0x01021994;
pub const USBDEVICE_SUPER_MAGIC: ::c_long = 0x00009fa2;

pub const CPU_SETSIZE: ::c_int = 0x400;

pub const PTRACE_TRACEME: ::c_uint = 0;
pub const PTRACE_PEEKTEXT: ::c_uint = 1;
pub const PTRACE_PEEKDATA: ::c_uint = 2;
pub const PTRACE_PEEKUSER: ::c_uint = 3;
pub const PTRACE_POKETEXT: ::c_uint = 4;
pub const PTRACE_POKEDATA: ::c_uint = 5;
pub const PTRACE_POKEUSER: ::c_uint = 6;
pub const PTRACE_CONT: ::c_uint = 7;
pub const PTRACE_KILL: ::c_uint = 8;
pub const PTRACE_SINGLESTEP: ::c_uint = 9;
pub const PTRACE_ATTACH: ::c_uint = 16;
pub const PTRACE_SYSCALL: ::c_uint = 24;
pub const PTRACE_SETOPTIONS: ::c_uint = 0x4200;
pub const PTRACE_GETEVENTMSG: ::c_uint = 0x4201;
pub const PTRACE_GETSIGINFO: ::c_uint = 0x4202;
pub const PTRACE_SETSIGINFO: ::c_uint = 0x4203;
pub const PTRACE_GETREGSET: ::c_uint = 0x4204;
pub const PTRACE_SETREGSET: ::c_uint = 0x4205;
pub const PTRACE_SEIZE: ::c_uint = 0x4206;
pub const PTRACE_INTERRUPT: ::c_uint = 0x4207;
pub const PTRACE_LISTEN: ::c_uint = 0x4208;
pub const PTRACE_PEEKSIGINFO: ::c_uint = 0x4209;

pub const EPOLLWAKEUP: ::c_int = 0x20000000;

pub const SEEK_DATA: ::c_int = 3;
pub const SEEK_HOLE: ::c_int = 4;

pub const LINUX_REBOOT_MAGIC1: ::c_int = 0xfee1dead;
pub const LINUX_REBOOT_MAGIC2: ::c_int = 672274793;
pub const LINUX_REBOOT_MAGIC2A: ::c_int = 85072278;
pub const LINUX_REBOOT_MAGIC2B: ::c_int = 369367448;
pub const LINUX_REBOOT_MAGIC2C: ::c_int = 537993216;

pub const LINUX_REBOOT_CMD_RESTART: ::c_int = 0x01234567;
pub const LINUX_REBOOT_CMD_HALT: ::c_int = 0xCDEF0123;
pub const LINUX_REBOOT_CMD_CAD_ON: ::c_int = 0x89ABCDEF;
pub const LINUX_REBOOT_CMD_CAD_OFF: ::c_int = 0x00000000;
pub const LINUX_REBOOT_CMD_POWER_OFF: ::c_int = 0x4321FEDC;
pub const LINUX_REBOOT_CMD_RESTART2: ::c_int = 0xA1B2C3D4;
pub const LINUX_REBOOT_CMD_SW_SUSPEND: ::c_int = 0xD000FCE2;
pub const LINUX_REBOOT_CMD_KEXEC: ::c_int = 0x45584543;

// linux/rtnetlink.h
pub const TCA_PAD: ::c_ushort = 9;
pub const TCA_DUMP_INVISIBLE: ::c_ushort = 10;
pub const TCA_CHAIN: ::c_ushort = 11;
pub const TCA_HW_OFFLOAD: ::c_ushort = 12;

pub const RTM_DELNETCONF: u16 = 81;
pub const RTM_NEWSTATS: u16 = 92;
pub const RTM_GETSTATS: u16 = 94;
pub const RTM_NEWCACHEREPORT: u16 = 96;

pub const RTM_F_LOOKUP_TABLE: ::c_uint = 0x1000;
pub const RTM_F_FIB_MATCH: ::c_uint = 0x2000;

pub const RTA_VIA: ::c_ushort = 18;
pub const RTA_NEWDST: ::c_ushort = 19;
pub const RTA_PREF: ::c_ushort = 20;
pub const RTA_ENCAP_TYPE: ::c_ushort = 21;
pub const RTA_ENCAP: ::c_ushort = 22;
pub const RTA_EXPIRES: ::c_ushort = 23;
pub const RTA_PAD: ::c_ushort = 24;
pub const RTA_UID: ::c_ushort = 25;
pub const RTA_TTL_PROPAGATE: ::c_ushort = 26;

// linux/neighbor.h
pub const NTF_EXT_LEARNED: u8 = 0x10;
pub const NTF_OFFLOADED: u8 = 0x20;

pub const NDA_MASTER: ::c_ushort = 9;
pub const NDA_LINK_NETNSID: ::c_ushort = 10;
pub const NDA_SRC_VNI: ::c_ushort = 11;

// linux/if_addr.h
pub const IFA_FLAGS: ::c_ushort = 8;

pub const IFA_F_MANAGETEMPADDR: u32 = 0x100;
pub const IFA_F_NOPREFIXROUTE: u32 = 0x200;
pub const IFA_F_MCAUTOJOIN: u32 = 0x400;
pub const IFA_F_STABLE_PRIVACY: u32 = 0x800;

pub const MAX_LINKS: ::c_int = 32;

pub const GENL_UNS_ADMIN_PERM: ::c_int = 0x10;

pub const GENL_ID_VFS_DQUOT: ::c_int = ::NLMSG_MIN_TYPE + 1;
pub const GENL_ID_PMCRAID: ::c_int = ::NLMSG_MIN_TYPE + 2;

pub const TIOCM_LE: ::c_int = 0x001;
pub const TIOCM_DTR: ::c_int = 0x002;
pub const TIOCM_RTS: ::c_int = 0x004;
pub const TIOCM_CD: ::c_int = TIOCM_CAR;
pub const TIOCM_RI: ::c_int = TIOCM_RNG;

pub const NF_NETDEV_INGRESS: ::c_int = 0;
pub const NF_NETDEV_NUMHOOKS: ::c_int = 1;

pub const NFPROTO_INET: ::c_int = 1;
pub const NFPROTO_NETDEV: ::c_int = 5;

// linux/netfilter/nf_tables.h
pub const NFT_TABLE_MAXNAMELEN: ::c_int = 256;
pub const NFT_CHAIN_MAXNAMELEN: ::c_int = 256;
pub const NFT_SET_MAXNAMELEN: ::c_int = 256;
pub const NFT_OBJ_MAXNAMELEN: ::c_int = 256;
pub const NFT_USERDATA_MAXLEN: ::c_int = 256;

pub const NFT_REG_VERDICT: ::c_int = 0;
pub const NFT_REG_1: ::c_int = 1;
pub const NFT_REG_2: ::c_int = 2;
pub const NFT_REG_3: ::c_int = 3;
pub const NFT_REG_4: ::c_int = 4;
pub const __NFT_REG_MAX: ::c_int = 5;
pub const NFT_REG32_00: ::c_int = 8;
pub const NFT_REG32_01: ::c_int = 9;
pub const NFT_REG32_02: ::c_int = 10;
pub const NFT_REG32_03: ::c_int = 11;
pub const NFT_REG32_04: ::c_int = 12;
pub const NFT_REG32_05: ::c_int = 13;
pub const NFT_REG32_06: ::c_int = 14;
pub const NFT_REG32_07: ::c_int = 15;
pub const NFT_REG32_08: ::c_int = 16;
pub const NFT_REG32_09: ::c_int = 17;
pub const NFT_REG32_10: ::c_int = 18;
pub const NFT_REG32_11: ::c_int = 19;
pub const NFT_REG32_12: ::c_int = 20;
pub const NFT_REG32_13: ::c_int = 21;
pub const NFT_REG32_14: ::c_int = 22;
pub const NFT_REG32_15: ::c_int = 23;

pub const NFT_REG_SIZE: ::c_int = 16;
pub const NFT_REG32_SIZE: ::c_int = 4;

pub const NFT_CONTINUE: ::c_int = -1;
pub const NFT_BREAK: ::c_int = -2;
pub const NFT_JUMP: ::c_int = -3;
pub const NFT_GOTO: ::c_int = -4;
pub const NFT_RETURN: ::c_int = -5;

pub const NFT_MSG_NEWTABLE: ::c_int = 0;
pub const NFT_MSG_GETTABLE: ::c_int = 1;
pub const NFT_MSG_DELTABLE: ::c_int = 2;
pub const NFT_MSG_NEWCHAIN: ::c_int = 3;
pub const NFT_MSG_GETCHAIN: ::c_int = 4;
pub const NFT_MSG_DELCHAIN: ::c_int = 5;
pub const NFT_MSG_NEWRULE: ::c_int = 6;
pub const NFT_MSG_GETRULE: ::c_int = 7;
pub const NFT_MSG_DELRULE: ::c_int = 8;
pub const NFT_MSG_NEWSET: ::c_int = 9;
pub const NFT_MSG_GETSET: ::c_int = 10;
pub const NFT_MSG_DELSET: ::c_int = 11;
pub const NFT_MSG_NEWSETELEM: ::c_int = 12;
pub const NFT_MSG_GETSETELEM: ::c_int = 13;
pub const NFT_MSG_DELSETELEM: ::c_int = 14;
pub const NFT_MSG_NEWGEN: ::c_int = 15;
pub const NFT_MSG_GETGEN: ::c_int = 16;
pub const NFT_MSG_TRACE: ::c_int = 17;
cfg_if! {
    if #[cfg(not(target_arch = "sparc64"))] {
        pub const NFT_MSG_NEWOBJ: ::c_int = 18;
        pub const NFT_MSG_GETOBJ: ::c_int = 19;
        pub const NFT_MSG_DELOBJ: ::c_int = 20;
        pub const NFT_MSG_GETOBJ_RESET: ::c_int = 21;
    }
}
pub const NFT_MSG_MAX: ::c_int = 25;

pub const NFT_SET_ANONYMOUS: ::c_int = 0x1;
pub const NFT_SET_CONSTANT: ::c_int = 0x2;
pub const NFT_SET_INTERVAL: ::c_int = 0x4;
pub const NFT_SET_MAP: ::c_int = 0x8;
pub const NFT_SET_TIMEOUT: ::c_int = 0x10;
pub const NFT_SET_EVAL: ::c_int = 0x20;

pub const NFT_SET_POL_PERFORMANCE: ::c_int = 0;
pub const NFT_SET_POL_MEMORY: ::c_int = 1;

pub const NFT_SET_ELEM_INTERVAL_END: ::c_int = 0x1;

pub const NFT_DATA_VALUE: ::c_uint = 0;
pub const NFT_DATA_VERDICT: ::c_uint = 0xffffff00;

pub const NFT_DATA_RESERVED_MASK: ::c_uint = 0xffffff00;

pub const NFT_DATA_VALUE_MAXLEN: ::c_int = 64;

pub const NFT_BYTEORDER_NTOH: ::c_int = 0;
pub const NFT_BYTEORDER_HTON: ::c_int = 1;

pub const NFT_CMP_EQ: ::c_int = 0;
pub const NFT_CMP_NEQ: ::c_int = 1;
pub const NFT_CMP_LT: ::c_int = 2;
pub const NFT_CMP_LTE: ::c_int = 3;
pub const NFT_CMP_GT: ::c_int = 4;
pub const NFT_CMP_GTE: ::c_int = 5;

pub const NFT_RANGE_EQ: ::c_int = 0;
pub const NFT_RANGE_NEQ: ::c_int = 1;

pub const NFT_LOOKUP_F_INV: ::c_int = (1 << 0);

pub const NFT_DYNSET_OP_ADD: ::c_int = 0;
pub const NFT_DYNSET_OP_UPDATE: ::c_int = 1;

pub const NFT_DYNSET_F_INV: ::c_int = (1 << 0);

pub const NFT_PAYLOAD_LL_HEADER: ::c_int = 0;
pub const NFT_PAYLOAD_NETWORK_HEADER: ::c_int = 1;
pub const NFT_PAYLOAD_TRANSPORT_HEADER: ::c_int = 2;

pub const NFT_PAYLOAD_CSUM_NONE: ::c_int = 0;
pub const NFT_PAYLOAD_CSUM_INET: ::c_int = 1;

pub const NFT_META_LEN: ::c_int = 0;
pub const NFT_META_PROTOCOL: ::c_int = 1;
pub const NFT_META_PRIORITY: ::c_int = 2;
pub const NFT_META_MARK: ::c_int = 3;
pub const NFT_META_IIF: ::c_int = 4;
pub const NFT_META_OIF: ::c_int = 5;
pub const NFT_META_IIFNAME: ::c_int = 6;
pub const NFT_META_OIFNAME: ::c_int = 7;
pub const NFT_META_IIFTYPE: ::c_int = 8;
pub const NFT_META_OIFTYPE: ::c_int = 9;
pub const NFT_META_SKUID: ::c_int = 10;
pub const NFT_META_SKGID: ::c_int = 11;
pub const NFT_META_NFTRACE: ::c_int = 12;
pub const NFT_META_RTCLASSID: ::c_int = 13;
pub const NFT_META_SECMARK: ::c_int = 14;
pub const NFT_META_NFPROTO: ::c_int = 15;
pub const NFT_META_L4PROTO: ::c_int = 16;
pub const NFT_META_BRI_IIFNAME: ::c_int = 17;
pub const NFT_META_BRI_OIFNAME: ::c_int = 18;
pub const NFT_META_PKTTYPE: ::c_int = 19;
pub const NFT_META_CPU: ::c_int = 20;
pub const NFT_META_IIFGROUP: ::c_int = 21;
pub const NFT_META_OIFGROUP: ::c_int = 22;
pub const NFT_META_CGROUP: ::c_int = 23;
pub const NFT_META_PRANDOM: ::c_int = 24;

pub const NFT_CT_STATE: ::c_int = 0;
pub const NFT_CT_DIRECTION: ::c_int = 1;
pub const NFT_CT_STATUS: ::c_int = 2;
pub const NFT_CT_MARK: ::c_int = 3;
pub const NFT_CT_SECMARK: ::c_int = 4;
pub const NFT_CT_EXPIRATION: ::c_int = 5;
pub const NFT_CT_HELPER: ::c_int = 6;
pub const NFT_CT_L3PROTOCOL: ::c_int = 7;
pub const NFT_CT_SRC: ::c_int = 8;
pub const NFT_CT_DST: ::c_int = 9;
pub const NFT_CT_PROTOCOL: ::c_int = 10;
pub const NFT_CT_PROTO_SRC: ::c_int = 11;
pub const NFT_CT_PROTO_DST: ::c_int = 12;
pub const NFT_CT_LABELS: ::c_int = 13;
pub const NFT_CT_PKTS: ::c_int = 14;
pub const NFT_CT_BYTES: ::c_int = 15;

pub const NFT_LIMIT_PKTS: ::c_int = 0;
pub const NFT_LIMIT_PKT_BYTES: ::c_int = 1;

pub const NFT_LIMIT_F_INV: ::c_int = (1 << 0);

pub const NFT_QUEUE_FLAG_BYPASS: ::c_int = 0x01;
pub const NFT_QUEUE_FLAG_CPU_FANOUT: ::c_int = 0x02;
pub const NFT_QUEUE_FLAG_MASK: ::c_int = 0x03;

pub const NFT_QUOTA_F_INV: ::c_int = (1 << 0);

pub const NFT_REJECT_ICMP_UNREACH: ::c_int = 0;
pub const NFT_REJECT_TCP_RST: ::c_int = 1;
pub const NFT_REJECT_ICMPX_UNREACH: ::c_int = 2;

pub const NFT_REJECT_ICMPX_NO_ROUTE: ::c_int = 0;
pub const NFT_REJECT_ICMPX_PORT_UNREACH: ::c_int = 1;
pub const NFT_REJECT_ICMPX_HOST_UNREACH: ::c_int = 2;
pub const NFT_REJECT_ICMPX_ADMIN_PROHIBITED: ::c_int = 3;

pub const NFT_NAT_SNAT: ::c_int = 0;
pub const NFT_NAT_DNAT: ::c_int = 1;

pub const NFT_TRACETYPE_UNSPEC: ::c_int = 0;
pub const NFT_TRACETYPE_POLICY: ::c_int = 1;
pub const NFT_TRACETYPE_RETURN: ::c_int = 2;
pub const NFT_TRACETYPE_RULE: ::c_int = 3;

pub const NFT_NG_INCREMENTAL: ::c_int = 0;
pub const NFT_NG_RANDOM: ::c_int = 1;

pub const M_MXFAST: ::c_int = 1;
pub const M_NLBLKS: ::c_int = 2;
pub const M_GRAIN: ::c_int = 3;
pub const M_KEEP: ::c_int = 4;
pub const M_TRIM_THRESHOLD: ::c_int = -1;
pub const M_TOP_PAD: ::c_int = -2;
pub const M_MMAP_THRESHOLD: ::c_int = -3;
pub const M_MMAP_MAX: ::c_int = -4;
pub const M_CHECK_ACTION: ::c_int = -5;
pub const M_PERTURB: ::c_int = -6;
pub const M_ARENA_TEST: ::c_int = -7;
pub const M_ARENA_MAX: ::c_int = -8;

pub const AT_STATX_SYNC_TYPE: ::c_int = 0x6000;
pub const AT_STATX_SYNC_AS_STAT: ::c_int = 0x0000;
pub const AT_STATX_FORCE_SYNC: ::c_int = 0x2000;
pub const AT_STATX_DONT_SYNC: ::c_int = 0x4000;
pub const STATX_TYPE: ::c_uint = 0x0001;
pub const STATX_MODE: ::c_uint = 0x0002;
pub const STATX_NLINK: ::c_uint = 0x0004;
pub const STATX_UID: ::c_uint = 0x0008;
pub const STATX_GID: ::c_uint = 0x0010;
pub const STATX_ATIME: ::c_uint = 0x0020;
pub const STATX_MTIME: ::c_uint = 0x0040;
pub const STATX_CTIME: ::c_uint = 0x0080;
pub const STATX_INO: ::c_uint = 0x0100;
pub const STATX_SIZE: ::c_uint = 0x0200;
pub const STATX_BLOCKS: ::c_uint = 0x0400;
pub const STATX_BASIC_STATS: ::c_uint = 0x07ff;
pub const STATX_BTIME: ::c_uint = 0x0800;
pub const STATX_ALL: ::c_uint = 0x0fff;
pub const STATX__RESERVED: ::c_int = 0x80000000;
pub const STATX_ATTR_COMPRESSED: ::c_int = 0x0004;
pub const STATX_ATTR_IMMUTABLE: ::c_int = 0x0010;
pub const STATX_ATTR_APPEND: ::c_int = 0x0020;
pub const STATX_ATTR_NODUMP: ::c_int = 0x0040;
pub const STATX_ATTR_ENCRYPTED: ::c_int = 0x0800;
pub const STATX_ATTR_AUTOMOUNT: ::c_int = 0x1000;

cfg_if! {
    if #[cfg(any(
        target_arch = "arm",
        target_arch = "x86",
        target_arch = "x86_64",
        target_arch = "s390x"
    ))] {
        pub const PTHREAD_STACK_MIN: ::size_t = 16384;
    } else if #[cfg(target_arch = "sparc64")] {
        pub const PTHREAD_STACK_MIN: ::size_t = 0x6000;
    } else {
        pub const PTHREAD_STACK_MIN: ::size_t = 131072;
    }
}
pub const PTHREAD_MUTEX_ADAPTIVE_NP: ::c_int = 3;

extern "C" {
    pub fn sendmmsg(
        sockfd: ::c_int,
        msgvec: *mut ::mmsghdr,
        vlen: ::c_uint,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn recvmmsg(
        sockfd: ::c_int,
        msgvec: *mut ::mmsghdr,
        vlen: ::c_uint,
        flags: ::c_int,
        timeout: *mut ::timespec,
    ) -> ::c_int;

    pub fn getrlimit64(
        resource: ::__rlimit_resource_t,
        rlim: *mut ::rlimit64,
    ) -> ::c_int;
    pub fn setrlimit64(
        resource: ::__rlimit_resource_t,
        rlim: *const ::rlimit64,
    ) -> ::c_int;
    pub fn getrlimit(
        resource: ::__rlimit_resource_t,
        rlim: *mut ::rlimit,
    ) -> ::c_int;
    pub fn setrlimit(
        resource: ::__rlimit_resource_t,
        rlim: *const ::rlimit,
    ) -> ::c_int;
    pub fn prlimit(
        pid: ::pid_t,
        resource: ::__rlimit_resource_t,
        new_limit: *const ::rlimit,
        old_limit: *mut ::rlimit,
    ) -> ::c_int;
    pub fn prlimit64(
        pid: ::pid_t,
        resource: ::__rlimit_resource_t,
        new_limit: *const ::rlimit64,
        old_limit: *mut ::rlimit64,
    ) -> ::c_int;
    pub fn utmpname(file: *const ::c_char) -> ::c_int;
    pub fn utmpxname(file: *const ::c_char) -> ::c_int;
    pub fn getutxent() -> *mut utmpx;
    pub fn getutxid(ut: *const utmpx) -> *mut utmpx;
    pub fn getutxline(ut: *const utmpx) -> *mut utmpx;
    pub fn pututxline(ut: *const utmpx) -> *mut utmpx;
    pub fn setutxent();
    pub fn endutxent();
    pub fn getpt() -> ::c_int;
    pub fn mallopt(param: ::c_int, value: ::c_int) -> ::c_int;
    pub fn gettimeofday(tp: *mut ::timeval, tz: *mut ::timezone) -> ::c_int;
    pub fn statx(
        dirfd: ::c_int,
        pathname: *const c_char,
        flags: ::c_int,
        mask: ::c_uint,
        statxbuf: *mut statx,
    ) -> ::c_int;
    pub fn getrandom(
        buf: *mut ::c_void,
        buflen: ::size_t,
        flags: ::c_uint,
    ) -> ::ssize_t;
}

#[link(name = "util")]
extern "C" {
    pub fn ioctl(fd: ::c_int, request: ::c_ulong, ...) -> ::c_int;
    pub fn backtrace(buf: *mut *mut ::c_void, sz: ::c_int) -> ::c_int;
    pub fn glob64(
        pattern: *const ::c_char,
        flags: ::c_int,
        errfunc: ::Option<
            extern "C" fn(epath: *const ::c_char, errno: ::c_int) -> ::c_int,
        >,
        pglob: *mut glob64_t,
    ) -> ::c_int;
    pub fn globfree64(pglob: *mut glob64_t);
    pub fn ptrace(request: ::c_uint, ...) -> ::c_long;
    pub fn pthread_attr_getaffinity_np(
        attr: *const ::pthread_attr_t,
        cpusetsize: ::size_t,
        cpuset: *mut ::cpu_set_t,
    ) -> ::c_int;
    pub fn pthread_attr_setaffinity_np(
        attr: *mut ::pthread_attr_t,
        cpusetsize: ::size_t,
        cpuset: *const ::cpu_set_t,
    ) -> ::c_int;
    pub fn getpriority(which: ::__priority_which_t, who: ::id_t) -> ::c_int;
    pub fn setpriority(
        which: ::__priority_which_t,
        who: ::id_t,
        prio: ::c_int,
    ) -> ::c_int;
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
    pub fn pthread_rwlockattr_getkind_np(
        attr: *const ::pthread_rwlockattr_t,
        val: *mut ::c_int,
    ) -> ::c_int;
    pub fn pthread_rwlockattr_setkind_np(
        attr: *mut ::pthread_rwlockattr_t,
        val: ::c_int,
    ) -> ::c_int;
    pub fn sched_getcpu() -> ::c_int;
    pub fn mallinfo() -> ::mallinfo;
    pub fn malloc_usable_size(ptr: *mut ::c_void) -> ::size_t;
    pub fn getauxval(type_: ::c_ulong) -> ::c_ulong;
    #[cfg_attr(target_os = "netbsd", link_name = "__getpwent_r50")]
    #[cfg_attr(target_os = "solaris", link_name = "__posix_getpwent_r")]
    pub fn getpwent_r(
        pwd: *mut ::passwd,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut ::passwd,
    ) -> ::c_int;
    #[cfg_attr(target_os = "netbsd", link_name = "__getgrent_r50")]
    #[cfg_attr(target_os = "solaris", link_name = "__posix_getgrent_r")]
    pub fn getgrent_r(
        grp: *mut ::group,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut ::group,
    ) -> ::c_int;
    pub fn pthread_getname_np(
        thread: ::pthread_t,
        name: *mut ::c_char,
        len: ::size_t,
    ) -> ::c_int;
    pub fn pthread_setname_np(
        thread: ::pthread_t,
        name: *const ::c_char,
    ) -> ::c_int;
}

cfg_if! {
    if #[cfg(any(target_arch = "x86",
                 target_arch = "arm",
                 target_arch = "mips",
                 target_arch = "powerpc"))] {
        mod b32;
        pub use self::b32::*;
    } else if #[cfg(any(target_arch = "x86_64",
                        target_arch = "aarch64",
                        target_arch = "powerpc64",
                        target_arch = "mips64",
                        target_arch = "s390x",
                        target_arch = "sparc64"))] {
        mod b64;
        pub use self::b64::*;
    } else {
        // Unknown target_arch
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
