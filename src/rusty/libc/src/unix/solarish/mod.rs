pub type c_char = i8;
pub type c_long = i64;
pub type c_ulong = u64;

pub type clockid_t = ::c_int;
pub type blkcnt_t = ::c_long;
pub type clock_t = ::c_long;
pub type daddr_t = ::c_long;
pub type dev_t = ::c_ulong;
pub type fsblkcnt_t = ::c_ulong;
pub type fsfilcnt_t = ::c_ulong;
pub type ino_t = ::c_ulong;
pub type key_t = ::c_int;
pub type major_t = ::c_uint;
pub type minor_t = ::c_uint;
pub type mode_t = ::c_uint;
pub type nlink_t = ::c_uint;
pub type rlim_t = ::c_ulong;
pub type speed_t = ::c_uint;
pub type tcflag_t = ::c_uint;
pub type time_t = ::c_long;
pub type wchar_t = ::c_int;
pub type nfds_t = ::c_ulong;

pub type suseconds_t = ::c_long;
pub type off_t = ::c_long;
pub type useconds_t = ::c_uint;
pub type socklen_t = ::c_uint;
pub type sa_family_t = u16;
pub type pthread_t = ::c_uint;
pub type pthread_key_t = ::c_uint;
pub type blksize_t = ::c_int;
pub type nl_item = ::c_int;
pub type mqd_t = *mut ::c_void;
pub type id_t = ::c_int;
pub type idtype_t = ::c_uint;

pub type door_attr_t = ::c_uint;
pub type door_id_t = ::c_ulonglong;

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
        pub sin_zero: [::c_char; 8]
    }

    pub struct sockaddr_in6 {
        pub sin6_family: sa_family_t,
        pub sin6_port: ::in_port_t,
        pub sin6_flowinfo: u32,
        pub sin6_addr: ::in6_addr,
        pub sin6_scope_id: u32,
        pub __sin6_src_id: u32
    }

    pub struct passwd {
        pub pw_name: *mut ::c_char,
        pub pw_passwd: *mut ::c_char,
        pub pw_uid: ::uid_t,
        pub pw_gid: ::gid_t,
        pub pw_age: *mut ::c_char,
        pub pw_comment: *mut ::c_char,
        pub pw_gecos: *mut ::c_char,
        pub pw_dir: *mut ::c_char,
        pub pw_shell: *mut ::c_char
    }

    pub struct ifaddrs {
        pub ifa_next: *mut ifaddrs,
        pub ifa_name: *mut ::c_char,
        pub ifa_flags: ::c_ulong,
        pub ifa_addr: *mut ::sockaddr,
        pub ifa_netmask: *mut ::sockaddr,
        pub ifa_dstaddr: *mut ::sockaddr,
        pub ifa_data: *mut ::c_void
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
        pub tm_isdst: ::c_int
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

    pub struct pthread_attr_t {
        __pthread_attrp: *mut ::c_void
    }

    pub struct pthread_mutex_t {
        __pthread_mutex_flag1: u16,
        __pthread_mutex_flag2: u8,
        __pthread_mutex_ceiling: u8,
        __pthread_mutex_type: u16,
        __pthread_mutex_magic: u16,
        __pthread_mutex_lock: u64,
        __pthread_mutex_data: u64
    }

    pub struct pthread_mutexattr_t {
        __pthread_mutexattrp: *mut ::c_void
    }

    pub struct pthread_cond_t {
        __pthread_cond_flag: [u8; 4],
        __pthread_cond_type: u16,
        __pthread_cond_magic: u16,
        __pthread_cond_data: u64
    }

    pub struct pthread_condattr_t {
        __pthread_condattrp: *mut ::c_void,
    }

    pub struct pthread_rwlock_t {
        __pthread_rwlock_readers: i32,
        __pthread_rwlock_type: u16,
        __pthread_rwlock_magic: u16,
        __pthread_rwlock_mutex: ::pthread_mutex_t,
        __pthread_rwlock_readercv: ::pthread_cond_t,
        __pthread_rwlock_writercv: ::pthread_cond_t
    }

    pub struct pthread_rwlockattr_t {
        __pthread_rwlockattrp: *mut ::c_void,
    }

    pub struct dirent {
        pub d_ino: ::ino_t,
        pub d_off: ::off_t,
        pub d_reclen: u16,
        pub d_name: [::c_char; 3]
    }

    pub struct glob_t {
        pub gl_pathc: ::size_t,
        pub gl_pathv:  *mut *mut ::c_char,
        pub gl_offs: ::size_t,
        __unused1: *mut ::c_void,
        __unused2: ::c_int,
        __unused3: ::c_int,
        __unused4: ::c_int,
        __unused5: *mut ::c_void,
        __unused6: *mut ::c_void,
        __unused7: *mut ::c_void,
        __unused8: *mut ::c_void,
        __unused9: *mut ::c_void,
        __unused10: *mut ::c_void,
    }

    pub struct addrinfo {
        pub ai_flags: ::c_int,
        pub ai_family: ::c_int,
        pub ai_socktype: ::c_int,
        pub ai_protocol: ::c_int,
        #[cfg(target_arch = "sparc64")]
        __sparcv9_pad: ::c_int,
        pub ai_addrlen: ::socklen_t,
        pub ai_canonname: *mut ::c_char,
        pub ai_addr: *mut ::sockaddr,
        pub ai_next: *mut addrinfo,
    }

    pub struct sigset_t {
        bits: [u32; 4],
    }

    pub struct sigaction {
        pub sa_flags: ::c_int,
        pub sa_sigaction: ::sighandler_t,
        pub sa_mask: sigset_t,
    }

    pub struct stack_t {
        pub ss_sp: *mut ::c_void,
        pub ss_size: ::size_t,
        pub ss_flags: ::c_int,
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
        pub f_basetype: [::c_char; 16],
        pub f_flag: ::c_ulong,
        pub f_namemax: ::c_ulong,
        pub f_fstr: [::c_char; 32]
    }

    pub struct sched_param {
        pub sched_priority: ::c_int,
        sched_pad: [::c_int; 8]
    }

    pub struct Dl_info {
        pub dli_fname: *const ::c_char,
        pub dli_fbase: *mut ::c_void,
        pub dli_sname: *const ::c_char,
        pub dli_saddr: *mut ::c_void,
    }

    pub struct stat {
        pub st_dev: ::dev_t,
        pub st_ino: ::ino_t,
        pub st_mode: ::mode_t,
        pub st_nlink: ::nlink_t,
        pub st_uid: ::uid_t,
        pub st_gid: ::gid_t,
        pub st_rdev: ::dev_t,
        pub st_size: ::off_t,
        pub st_atime: ::time_t,
        pub st_atime_nsec: ::c_long,
        pub st_mtime: ::time_t,
        pub st_mtime_nsec: ::c_long,
        pub st_ctime: ::time_t,
        pub st_ctime_nsec: ::c_long,
        pub st_blksize: ::blksize_t,
        pub st_blocks: ::blkcnt_t,
        __unused: [::c_char; 16]
    }

    pub struct termios {
        pub c_iflag: ::tcflag_t,
        pub c_oflag: ::tcflag_t,
        pub c_cflag: ::tcflag_t,
        pub c_lflag: ::tcflag_t,
        pub c_cc: [::cc_t; ::NCCS]
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

    pub struct sem_t {
        pub sem_count: u32,
        pub sem_type: u16,
        pub sem_magic: u16,
        pub sem_pad1: [u64; 3],
        pub sem_pad2: [u64; 2]
    }

    pub struct flock {
        pub l_type: ::c_short,
        pub l_whence: ::c_short,
        pub l_start: ::off_t,
        pub l_len: ::off_t,
        pub l_sysid: ::c_int,
        pub l_pid: ::pid_t,
        pub l_pad: [::c_long; 4]
    }

    pub struct if_nameindex {
        pub if_index: ::c_uint,
        pub if_name: *mut ::c_char,
    }

    pub struct mq_attr {
        pub mq_flags: ::c_long,
        pub mq_maxmsg: ::c_long,
        pub mq_msgsize: ::c_long,
        pub mq_curmsgs: ::c_long,
        _pad: [::c_int; 4]
    }

    pub struct port_event {
        pub portev_events: ::c_int,
        pub portev_source: ::c_ushort,
        pub portev_pad: ::c_ushort,
        pub portev_object: ::uintptr_t,
        pub portev_user: *mut ::c_void,
    }

    pub struct door_desc_t__d_data__d_desc {
        pub d_descriptor: ::c_int,
        pub d_id: ::door_id_t
    }
}

s_no_extra_traits! {
    #[cfg_attr(any(target_arch = "x86", target_arch = "x86_64"), repr(packed))]
    pub struct epoll_event {
        pub events: u32,
        pub u64: u64,
    }

    pub struct sockaddr_un {
        pub sun_family: sa_family_t,
        pub sun_path: [c_char; 108]
    }

    pub struct utsname {
        pub sysname: [::c_char; 257],
        pub nodename: [::c_char; 257],
        pub release: [::c_char; 257],
        pub version: [::c_char; 257],
        pub machine: [::c_char; 257],
    }

    pub struct fd_set {
        #[cfg(target_pointer_width = "64")]
        fds_bits: [i64; FD_SETSIZE / 64],
        #[cfg(target_pointer_width = "32")]
        fds_bits: [i32; FD_SETSIZE / 32],
    }

    pub struct sockaddr_storage {
        pub ss_family: ::sa_family_t,
        __ss_pad1: [u8; 6],
        __ss_align: i64,
        __ss_pad2: [u8; 240],
    }

    pub struct siginfo_t {
        pub si_signo: ::c_int,
        pub si_code: ::c_int,
        pub si_errno: ::c_int,
        pub si_pad: ::c_int,
        pub si_addr: *mut ::c_void,
        __pad: [u8; 232],
    }

    pub struct sockaddr_dl {
        pub sdl_family: ::c_ushort,
        pub sdl_index: ::c_ushort,
        pub sdl_type: ::c_uchar,
        pub sdl_nlen: ::c_uchar,
        pub sdl_alen: ::c_uchar,
        pub sdl_slen: ::c_uchar,
        pub sdl_data: [::c_char; 244],
    }

    pub struct sigevent {
        pub sigev_notify: ::c_int,
        pub sigev_signo: ::c_int,
        pub sigev_value: ::sigval,
        pub ss_sp: *mut ::c_void,
        pub sigev_notify_attributes: *const ::pthread_attr_t,
        __sigev_pad2: ::c_int,
    }

    pub union door_desc_t__d_data {
        pub d_desc: door_desc_t__d_data__d_desc,
        d_resv: [::c_int; 5], /* Check out /usr/include/sys/door.h */
    }

    pub struct door_desc_t {
        pub d_attributes: door_attr_t,
        pub d_data: door_desc_t__d_data,
    }

    pub struct door_arg_t {
        pub data_ptr: *const ::c_char,
        pub data_size: ::size_t,
        pub desc_ptr: *const door_desc_t,
        pub dec_num: ::c_uint,
        pub rbuf: *const ::c_char,
        pub rsize: ::size_t,
    }
}

cfg_if! {
    if #[cfg(feature = "extra_traits")] {
        impl PartialEq for epoll_event {
            fn eq(&self, other: &epoll_event) -> bool {
                self.events == other.events
                    && self.u64 == other.u64
            }
        }
        impl Eq for epoll_event {}
        impl ::fmt::Debug for epoll_event {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                let events = self.events;
                let u64 = self.u64;
                f.debug_struct("epoll_event")
                    .field("events", &events)
                    .field("u64", &u64)
                    .finish()
            }
        }
        impl ::hash::Hash for epoll_event {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                let events = self.events;
                let u64 = self.u64;
                events.hash(state);
                u64.hash(state);
            }
        }

        impl PartialEq for sockaddr_un {
            fn eq(&self, other: &sockaddr_un) -> bool {
                self.sun_family == other.sun_family
                    && self
                    .sun_path
                    .iter()
                    .zip(other.sun_path.iter())
                    .all(|(a, b)| a == b)
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

        impl PartialEq for fd_set {
            fn eq(&self, other: &fd_set) -> bool {
                self.fds_bits
                    .iter()
                    .zip(other.fds_bits.iter())
                    .all(|(a, b)| a == b)
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
                self.ss_family == other.ss_family
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
                    .field("ss_family", &self.ss_family)
                    .field("__ss_pad1", &self.__ss_pad1)
                    .field("__ss_align", &self.__ss_align)
                    // FIXME: .field("__ss_pad2", &self.__ss_pad2)
                    .finish()
            }
        }
        impl ::hash::Hash for sockaddr_storage {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.ss_family.hash(state);
                self.__ss_pad1.hash(state);
                self.__ss_align.hash(state);
                self.__ss_pad2.hash(state);
            }
        }

        impl PartialEq for siginfo_t {
            fn eq(&self, other: &siginfo_t) -> bool {
                self.si_signo == other.si_signo
                    && self.si_code == other.si_code
                    && self.si_errno == other.si_errno
                    && self.si_addr == other.si_addr
                    && self
                    .__pad
                    .iter()
                    .zip(other.__pad.iter())
                    .all(|(a, b)| a == b)
            }
        }
        impl Eq for siginfo_t {}
        impl ::fmt::Debug for siginfo_t {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("siginfo_t")
                    .field("si_signo", &self.si_signo)
                    .field("si_code", &self.si_code)
                    .field("si_errno", &self.si_errno)
                    .field("si_addr", &self.si_addr)
                    // FIXME: .field("__pad", &self.__pad)
                    .finish()
            }
        }
        impl ::hash::Hash for siginfo_t {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.si_signo.hash(state);
                self.si_code.hash(state);
                self.si_errno.hash(state);
                self.si_addr.hash(state);
                self.__pad.hash(state);
            }
        }

        impl PartialEq for sockaddr_dl {
            fn eq(&self, other: &sockaddr_dl) -> bool {
                self.sdl_family == other.sdl_family
                    && self.sdl_index == other.sdl_index
                    && self.sdl_type == other.sdl_type
                    && self.sdl_nlen == other.sdl_nlen
                    && self.sdl_alen == other.sdl_alen
                    && self.sdl_slen == other.sdl_slen
                    && self
                    .sdl_data
                    .iter()
                    .zip(other.sdl_data.iter())
                    .all(|(a,b)| a == b)
            }
        }
        impl Eq for sockaddr_dl {}
        impl ::fmt::Debug for sockaddr_dl {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("sockaddr_dl")
                    .field("sdl_family", &self.sdl_family)
                    .field("sdl_index", &self.sdl_index)
                    .field("sdl_type", &self.sdl_type)
                    .field("sdl_nlen", &self.sdl_nlen)
                    .field("sdl_alen", &self.sdl_alen)
                    .field("sdl_slen", &self.sdl_slen)
                    // FIXME: .field("sdl_data", &self.sdl_data)
                    .finish()
            }
        }
        impl ::hash::Hash for sockaddr_dl {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.sdl_family.hash(state);
                self.sdl_index.hash(state);
                self.sdl_type.hash(state);
                self.sdl_nlen.hash(state);
                self.sdl_alen.hash(state);
                self.sdl_slen.hash(state);
                self.sdl_data.hash(state);
            }
        }

        impl PartialEq for sigevent {
            fn eq(&self, other: &sigevent) -> bool {
                self.sigev_notify == other.sigev_notify
                    && self.sigev_signo == other.sigev_signo
                    && self.sigev_value == other.sigev_value
                    && self.ss_sp == other.ss_sp
                    && self.sigev_notify_attributes
                        == other.sigev_notify_attributes
            }
        }
        impl Eq for sigevent {}
        impl ::fmt::Debug for sigevent {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("sigevent")
                    .field("sigev_notify", &self.sigev_notify)
                    .field("sigev_signo", &self.sigev_signo)
                    .field("sigev_value", &self.sigev_value)
                    .field("ss_sp", &self.ss_sp)
                    .field("sigev_notify_attributes",
                           &self.sigev_notify_attributes)
                    .finish()
            }
        }
        impl ::hash::Hash for sigevent {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.sigev_notify.hash(state);
                self.sigev_signo.hash(state);
                self.sigev_value.hash(state);
                self.ss_sp.hash(state);
                self.sigev_notify_attributes.hash(state);
            }
        }

    }
}

pub const LC_CTYPE: ::c_int = 0;
pub const LC_NUMERIC: ::c_int = 1;
pub const LC_TIME: ::c_int = 2;
pub const LC_COLLATE: ::c_int = 3;
pub const LC_MONETARY: ::c_int = 4;
pub const LC_MESSAGES: ::c_int = 5;
pub const LC_ALL: ::c_int = 6;
pub const LC_CTYPE_MASK: ::c_int = (1 << LC_CTYPE);
pub const LC_NUMERIC_MASK: ::c_int = (1 << LC_NUMERIC);
pub const LC_TIME_MASK: ::c_int = (1 << LC_TIME);
pub const LC_COLLATE_MASK: ::c_int = (1 << LC_COLLATE);
pub const LC_MONETARY_MASK: ::c_int = (1 << LC_MONETARY);
pub const LC_MESSAGES_MASK: ::c_int = (1 << LC_MESSAGES);
pub const LC_ALL_MASK: ::c_int = LC_CTYPE_MASK
    | LC_NUMERIC_MASK
    | LC_TIME_MASK
    | LC_COLLATE_MASK
    | LC_MONETARY_MASK
    | LC_MESSAGES_MASK;

pub const DAY_1: ::nl_item = 1;
pub const DAY_2: ::nl_item = 2;
pub const DAY_3: ::nl_item = 3;
pub const DAY_4: ::nl_item = 4;
pub const DAY_5: ::nl_item = 5;
pub const DAY_6: ::nl_item = 6;
pub const DAY_7: ::nl_item = 7;

pub const ABDAY_1: ::nl_item = 8;
pub const ABDAY_2: ::nl_item = 9;
pub const ABDAY_3: ::nl_item = 10;
pub const ABDAY_4: ::nl_item = 11;
pub const ABDAY_5: ::nl_item = 12;
pub const ABDAY_6: ::nl_item = 13;
pub const ABDAY_7: ::nl_item = 14;

pub const MON_1: ::nl_item = 15;
pub const MON_2: ::nl_item = 16;
pub const MON_3: ::nl_item = 17;
pub const MON_4: ::nl_item = 18;
pub const MON_5: ::nl_item = 19;
pub const MON_6: ::nl_item = 20;
pub const MON_7: ::nl_item = 21;
pub const MON_8: ::nl_item = 22;
pub const MON_9: ::nl_item = 23;
pub const MON_10: ::nl_item = 24;
pub const MON_11: ::nl_item = 25;
pub const MON_12: ::nl_item = 26;

pub const ABMON_1: ::nl_item = 27;
pub const ABMON_2: ::nl_item = 28;
pub const ABMON_3: ::nl_item = 29;
pub const ABMON_4: ::nl_item = 30;
pub const ABMON_5: ::nl_item = 31;
pub const ABMON_6: ::nl_item = 32;
pub const ABMON_7: ::nl_item = 33;
pub const ABMON_8: ::nl_item = 34;
pub const ABMON_9: ::nl_item = 35;
pub const ABMON_10: ::nl_item = 36;
pub const ABMON_11: ::nl_item = 37;
pub const ABMON_12: ::nl_item = 38;

pub const RADIXCHAR: ::nl_item = 39;
pub const THOUSEP: ::nl_item = 40;
pub const YESSTR: ::nl_item = 41;
pub const NOSTR: ::nl_item = 42;
pub const CRNCYSTR: ::nl_item = 43;

pub const D_T_FMT: ::nl_item = 44;
pub const D_FMT: ::nl_item = 45;
pub const T_FMT: ::nl_item = 46;
pub const AM_STR: ::nl_item = 47;
pub const PM_STR: ::nl_item = 48;

pub const CODESET: ::nl_item = 49;
pub const T_FMT_AMPM: ::nl_item = 50;
pub const ERA: ::nl_item = 51;
pub const ERA_D_FMT: ::nl_item = 52;
pub const ERA_D_T_FMT: ::nl_item = 53;
pub const ERA_T_FMT: ::nl_item = 54;
pub const ALT_DIGITS: ::nl_item = 55;
pub const YESEXPR: ::nl_item = 56;
pub const NOEXPR: ::nl_item = 57;
pub const _DATE_FMT: ::nl_item = 58;
pub const MAXSTRMSG: ::nl_item = 58;

pub const PATH_MAX: ::c_int = 1024;

pub const SA_ONSTACK: ::c_int = 0x00000001;
pub const SA_RESETHAND: ::c_int = 0x00000002;
pub const SA_RESTART: ::c_int = 0x00000004;
pub const SA_SIGINFO: ::c_int = 0x00000008;
pub const SA_NODEFER: ::c_int = 0x00000010;
pub const SA_NOCLDWAIT: ::c_int = 0x00010000;
pub const SA_NOCLDSTOP: ::c_int = 0x00020000;

pub const SS_ONSTACK: ::c_int = 1;
pub const SS_DISABLE: ::c_int = 2;

pub const FIOCLEX: ::c_int = 0x20006601;
pub const FIONCLEX: ::c_int = 0x20006602;
pub const FIONREAD: ::c_int = 0x4004667f;
pub const FIONBIO: ::c_int = 0x8004667e;
pub const FIOASYNC: ::c_int = 0x8004667d;
pub const FIOSETOWN: ::c_int = 0x8004667c;
pub const FIOGETOWN: ::c_int = 0x4004667b;

pub const SIGCHLD: ::c_int = 18;
pub const SIGBUS: ::c_int = 10;
pub const SIGINFO: ::c_int = 41;
pub const SIG_BLOCK: ::c_int = 1;
pub const SIG_UNBLOCK: ::c_int = 2;
pub const SIG_SETMASK: ::c_int = 3;

pub const SIGEV_NONE: ::c_int = 1;
pub const SIGEV_SIGNAL: ::c_int = 2;
pub const SIGEV_THREAD: ::c_int = 3;

pub const IPV6_UNICAST_HOPS: ::c_int = 0x5;
pub const IPV6_MULTICAST_IF: ::c_int = 0x6;
pub const IPV6_MULTICAST_HOPS: ::c_int = 0x7;
pub const IPV6_MULTICAST_LOOP: ::c_int = 0x8;
pub const IPV6_V6ONLY: ::c_int = 0x27;

cfg_if! {
    if #[cfg(target_pointer_width = "64")] {
        pub const FD_SETSIZE: usize = 65536;
    } else {
        pub const FD_SETSIZE: usize = 1024;
    }
}

pub const ST_RDONLY: ::c_ulong = 1;
pub const ST_NOSUID: ::c_ulong = 2;

pub const NI_MAXHOST: ::socklen_t = 1025;

pub const EXIT_FAILURE: ::c_int = 1;
pub const EXIT_SUCCESS: ::c_int = 0;
pub const RAND_MAX: ::c_int = 32767;
pub const EOF: ::c_int = -1;
pub const SEEK_SET: ::c_int = 0;
pub const SEEK_CUR: ::c_int = 1;
pub const SEEK_END: ::c_int = 2;
pub const _IOFBF: ::c_int = 0;
pub const _IONBF: ::c_int = 4;
pub const _IOLBF: ::c_int = 64;
pub const BUFSIZ: ::c_uint = 1024;
pub const FOPEN_MAX: ::c_uint = 20;
pub const FILENAME_MAX: ::c_uint = 1024;
pub const L_tmpnam: ::c_uint = 25;
pub const TMP_MAX: ::c_uint = 17576;

pub const O_RDONLY: ::c_int = 0;
pub const O_WRONLY: ::c_int = 1;
pub const O_RDWR: ::c_int = 2;
pub const O_NDELAY: ::c_int = 0x04;
pub const O_APPEND: ::c_int = 8;
pub const O_DSYNC: ::c_int = 0x40;
pub const O_CREAT: ::c_int = 256;
pub const O_EXCL: ::c_int = 1024;
pub const O_NOCTTY: ::c_int = 2048;
pub const O_TRUNC: ::c_int = 512;
pub const O_NOFOLLOW: ::c_int = 0x200000;
pub const O_SEARCH: ::c_int = 0x200000;
pub const O_EXEC: ::c_int = 0x400000;
pub const O_CLOEXEC: ::c_int = 0x800000;
pub const O_ACCMODE: ::c_int = 0x600003;
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
pub const F_DUPFD_CLOEXEC: ::c_int = 37;
pub const F_SETLK: ::c_int = 6;
pub const F_SETLKW: ::c_int = 7;
pub const F_GETLK: ::c_int = 14;
pub const SIGHUP: ::c_int = 1;
pub const SIGINT: ::c_int = 2;
pub const SIGQUIT: ::c_int = 3;
pub const SIGILL: ::c_int = 4;
pub const SIGABRT: ::c_int = 6;
pub const SIGEMT: ::c_int = 7;
pub const SIGFPE: ::c_int = 8;
pub const SIGKILL: ::c_int = 9;
pub const SIGSEGV: ::c_int = 11;
pub const SIGSYS: ::c_int = 12;
pub const SIGPIPE: ::c_int = 13;
pub const SIGALRM: ::c_int = 14;
pub const SIGTERM: ::c_int = 15;
pub const SIGUSR1: ::c_int = 16;
pub const SIGUSR2: ::c_int = 17;
pub const SIGPWR: ::c_int = 19;
pub const SIGWINCH: ::c_int = 20;
pub const SIGURG: ::c_int = 21;
pub const SIGPOLL: ::c_int = 22;
pub const SIGIO: ::c_int = SIGPOLL;
pub const SIGSTOP: ::c_int = 23;
pub const SIGTSTP: ::c_int = 24;
pub const SIGCONT: ::c_int = 25;
pub const SIGTTIN: ::c_int = 26;
pub const SIGTTOU: ::c_int = 27;
pub const SIGVTALRM: ::c_int = 28;
pub const SIGPROF: ::c_int = 29;
pub const SIGXCPU: ::c_int = 30;
pub const SIGXFSZ: ::c_int = 31;

pub const WNOHANG: ::c_int = 0x40;
pub const WUNTRACED: ::c_int = 0x04;

pub const WEXITED: ::c_int = 0x01;
pub const WTRAPPED: ::c_int = 0x02;
pub const WSTOPPED: ::c_int = WUNTRACED;
pub const WCONTINUED: ::c_int = 0x08;
pub const WNOWAIT: ::c_int = 0x80;

pub const AT_FDCWD: ::c_int = 0xffd19553;
pub const AT_SYMLINK_NOFOLLOW: ::c_int = 0x1000;

pub const P_PID: idtype_t = 0;
pub const P_PPID: idtype_t = 1;
pub const P_PGID: idtype_t = 2;
pub const P_SID: idtype_t = 3;
pub const P_CID: idtype_t = 4;
pub const P_UID: idtype_t = 5;
pub const P_GID: idtype_t = 6;
pub const P_ALL: idtype_t = 7;
pub const P_LWPID: idtype_t = 8;
pub const P_TASKID: idtype_t = 9;
pub const P_PROJID: idtype_t = 10;
pub const P_POOLID: idtype_t = 11;
pub const P_ZONEID: idtype_t = 12;
pub const P_CTID: idtype_t = 13;
pub const P_CPUID: idtype_t = 14;
pub const P_PSETID: idtype_t = 15;

pub const UTIME_OMIT: c_long = -2;
pub const UTIME_NOW: c_long = -1;

pub const PROT_NONE: ::c_int = 0;
pub const PROT_READ: ::c_int = 1;
pub const PROT_WRITE: ::c_int = 2;
pub const PROT_EXEC: ::c_int = 4;

pub const MAP_FILE: ::c_int = 0;
pub const MAP_SHARED: ::c_int = 0x0001;
pub const MAP_PRIVATE: ::c_int = 0x0002;
pub const MAP_FIXED: ::c_int = 0x0010;
pub const MAP_NORESERVE: ::c_int = 0x40;
pub const MAP_ANON: ::c_int = 0x0100;
pub const MAP_RENAME: ::c_int = 0x20;
pub const MAP_ALIGN: ::c_int = 0x200;
pub const MAP_TEXT: ::c_int = 0x400;
pub const MAP_INITDATA: ::c_int = 0x800;
pub const MAP_FAILED: *mut ::c_void = !0 as *mut ::c_void;

pub const MCL_CURRENT: ::c_int = 0x0001;
pub const MCL_FUTURE: ::c_int = 0x0002;

pub const MS_SYNC: ::c_int = 0x0004;
pub const MS_ASYNC: ::c_int = 0x0001;
pub const MS_INVALIDATE: ::c_int = 0x0002;
pub const MS_INVALCURPROC: ::c_int = 0x0008;

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
pub const ENOMSG: ::c_int = 35;
pub const EIDRM: ::c_int = 36;
pub const ECHRNG: ::c_int = 37;
pub const EL2NSYNC: ::c_int = 38;
pub const EL3HLT: ::c_int = 39;
pub const EL3RST: ::c_int = 40;
pub const ELNRNG: ::c_int = 41;
pub const EUNATCH: ::c_int = 42;
pub const ENOCSI: ::c_int = 43;
pub const EL2HLT: ::c_int = 44;
pub const EDEADLK: ::c_int = 45;
pub const ENOLCK: ::c_int = 46;
pub const ECANCELED: ::c_int = 47;
pub const ENOTSUP: ::c_int = 48;
pub const EDQUOT: ::c_int = 49;
pub const EBADE: ::c_int = 50;
pub const EBADR: ::c_int = 51;
pub const EXFULL: ::c_int = 52;
pub const ENOANO: ::c_int = 53;
pub const EBADRQC: ::c_int = 54;
pub const EBADSLT: ::c_int = 55;
pub const EDEADLOCK: ::c_int = 56;
pub const EBFONT: ::c_int = 57;
pub const EOWNERDEAD: ::c_int = 58;
pub const ENOTRECOVERABLE: ::c_int = 59;
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
pub const ELOCKUNMAPPED: ::c_int = 72;
pub const ENOTACTIVE: ::c_int = 73;
pub const EMULTIHOP: ::c_int = 74;
pub const EADI: ::c_int = 75;
pub const EBADMSG: ::c_int = 77;
pub const ENAMETOOLONG: ::c_int = 78;
pub const EOVERFLOW: ::c_int = 79;
pub const ENOTUNIQ: ::c_int = 80;
pub const EBADFD: ::c_int = 81;
pub const EREMCHG: ::c_int = 82;
pub const ELIBACC: ::c_int = 83;
pub const ELIBBAD: ::c_int = 84;
pub const ELIBSCN: ::c_int = 85;
pub const ELIBMAX: ::c_int = 86;
pub const ELIBEXEC: ::c_int = 87;
pub const EILSEQ: ::c_int = 88;
pub const ENOSYS: ::c_int = 89;
pub const ELOOP: ::c_int = 90;
pub const ERESTART: ::c_int = 91;
pub const ESTRPIPE: ::c_int = 92;
pub const ENOTEMPTY: ::c_int = 93;
pub const EUSERS: ::c_int = 94;
pub const ENOTSOCK: ::c_int = 95;
pub const EDESTADDRREQ: ::c_int = 96;
pub const EMSGSIZE: ::c_int = 97;
pub const EPROTOTYPE: ::c_int = 98;
pub const ENOPROTOOPT: ::c_int = 99;
pub const EPROTONOSUPPORT: ::c_int = 120;
pub const ESOCKTNOSUPPORT: ::c_int = 121;
pub const EOPNOTSUPP: ::c_int = 122;
pub const EPFNOSUPPORT: ::c_int = 123;
pub const EAFNOSUPPORT: ::c_int = 124;
pub const EADDRINUSE: ::c_int = 125;
pub const EADDRNOTAVAIL: ::c_int = 126;
pub const ENETDOWN: ::c_int = 127;
pub const ENETUNREACH: ::c_int = 128;
pub const ENETRESET: ::c_int = 129;
pub const ECONNABORTED: ::c_int = 130;
pub const ECONNRESET: ::c_int = 131;
pub const ENOBUFS: ::c_int = 132;
pub const EISCONN: ::c_int = 133;
pub const ENOTCONN: ::c_int = 134;
pub const ESHUTDOWN: ::c_int = 143;
pub const ETOOMANYREFS: ::c_int = 144;
pub const ETIMEDOUT: ::c_int = 145;
pub const ECONNREFUSED: ::c_int = 146;
pub const EHOSTDOWN: ::c_int = 147;
pub const EHOSTUNREACH: ::c_int = 148;
pub const EWOULDBLOCK: ::c_int = EAGAIN;
pub const EALREADY: ::c_int = 149;
pub const EINPROGRESS: ::c_int = 150;
pub const ESTALE: ::c_int = 151;

pub const EAI_AGAIN: ::c_int = 2;
pub const EAI_BADFLAGS: ::c_int = 3;
pub const EAI_FAIL: ::c_int = 4;
pub const EAI_FAMILY: ::c_int = 5;
pub const EAI_MEMORY: ::c_int = 6;
pub const EAI_NODATA: ::c_int = 7;
pub const EAI_NONAME: ::c_int = 8;
pub const EAI_SERVICE: ::c_int = 9;
pub const EAI_SOCKTYPE: ::c_int = 10;
pub const EAI_SYSTEM: ::c_int = 11;
pub const EAI_OVERFLOW: ::c_int = 12;

pub const F_DUPFD: ::c_int = 0;
pub const F_GETFD: ::c_int = 1;
pub const F_SETFD: ::c_int = 2;
pub const F_GETFL: ::c_int = 3;
pub const F_SETFL: ::c_int = 4;

pub const SIGTRAP: ::c_int = 5;

pub const GLOB_APPEND: ::c_int = 32;
pub const GLOB_DOOFFS: ::c_int = 16;
pub const GLOB_ERR: ::c_int = 1;
pub const GLOB_MARK: ::c_int = 2;
pub const GLOB_NOCHECK: ::c_int = 8;
pub const GLOB_NOSORT: ::c_int = 4;
pub const GLOB_NOESCAPE: ::c_int = 64;

pub const GLOB_NOSPACE: ::c_int = -2;
pub const GLOB_ABORTED: ::c_int = -1;
pub const GLOB_NOMATCH: ::c_int = -3;

pub const POLLIN: ::c_short = 0x1;
pub const POLLPRI: ::c_short = 0x2;
pub const POLLOUT: ::c_short = 0x4;
pub const POLLERR: ::c_short = 0x8;
pub const POLLHUP: ::c_short = 0x10;
pub const POLLNVAL: ::c_short = 0x20;
pub const POLLNORM: ::c_short = 0x0040;
pub const POLLRDNORM: ::c_short = 0x0040;
pub const POLLWRNORM: ::c_short = 0x4; /* POLLOUT */
pub const POLLRDBAND: ::c_short = 0x0080;
pub const POLLWRBAND: ::c_short = 0x0100;

pub const POSIX_MADV_NORMAL: ::c_int = 0;
pub const POSIX_MADV_RANDOM: ::c_int = 1;
pub const POSIX_MADV_SEQUENTIAL: ::c_int = 2;
pub const POSIX_MADV_WILLNEED: ::c_int = 3;
pub const POSIX_MADV_DONTNEED: ::c_int = 4;

pub const PTHREAD_CREATE_JOINABLE: ::c_int = 0;
pub const PTHREAD_CREATE_DETACHED: ::c_int = 0x40;
pub const PTHREAD_PROCESS_SHARED: ::c_int = 1;
pub const PTHREAD_PROCESS_PRIVATE: ::c_ushort = 0;
pub const PTHREAD_STACK_MIN: ::size_t = 4096;

pub const SIGSTKSZ: ::size_t = 8192;

// https://illumos.org/man/3c/clock_gettime
// https://github.com/illumos/illumos-gate/
//   blob/HEAD/usr/src/lib/libc/amd64/sys/__clock_gettime.s
// clock_gettime(3c) doesn't seem to accept anything other than CLOCK_REALTIME
// or __CLOCK_REALTIME0
//
// https://github.com/illumos/illumos-gate/
//   blob/HEAD/usr/src/uts/common/sys/time_impl.h
// Confusing! CLOCK_HIGHRES==CLOCK_MONOTONIC==4
// __CLOCK_REALTIME0==0 is an obsoleted version of CLOCK_REALTIME==3
pub const CLOCK_REALTIME: ::clockid_t = 3;
pub const CLOCK_MONOTONIC: ::clockid_t = 4;
pub const TIMER_RELTIME: ::c_int = 0;
pub const TIMER_ABSTIME: ::c_int = 1;

pub const RLIMIT_CPU: ::c_int = 0;
pub const RLIMIT_FSIZE: ::c_int = 1;
pub const RLIMIT_DATA: ::c_int = 2;
pub const RLIMIT_STACK: ::c_int = 3;
pub const RLIMIT_CORE: ::c_int = 4;
pub const RLIMIT_NOFILE: ::c_int = 5;
pub const RLIMIT_VMEM: ::c_int = 6;
pub const RLIMIT_AS: ::c_int = RLIMIT_VMEM;

#[deprecated(since = "0.2.64", note = "Not stable across OS versions")]
pub const RLIM_NLIMITS: rlim_t = 7;
pub const RLIM_INFINITY: rlim_t = 0x7fffffff;

pub const RUSAGE_SELF: ::c_int = 0;
pub const RUSAGE_CHILDREN: ::c_int = -1;

pub const MADV_NORMAL: ::c_int = 0;
pub const MADV_RANDOM: ::c_int = 1;
pub const MADV_SEQUENTIAL: ::c_int = 2;
pub const MADV_WILLNEED: ::c_int = 3;
pub const MADV_DONTNEED: ::c_int = 4;
pub const MADV_FREE: ::c_int = 5;

pub const AF_UNSPEC: ::c_int = 0;
pub const AF_UNIX: ::c_int = 1;
pub const AF_LOCAL: ::c_int = 0;
pub const AF_FILE: ::c_int = 0;
pub const AF_INET: ::c_int = 2;
pub const AF_IMPLINK: ::c_int = 3;
pub const AF_PUP: ::c_int = 4;
pub const AF_CHAOS: ::c_int = 5;
pub const AF_NS: ::c_int = 6;
pub const AF_NBS: ::c_int = 7;
pub const AF_ECMA: ::c_int = 8;
pub const AF_DATAKIT: ::c_int = 9;
pub const AF_CCITT: ::c_int = 10;
pub const AF_SNA: ::c_int = 11;
pub const AF_DECnet: ::c_int = 12;
pub const AF_DLI: ::c_int = 13;
pub const AF_LAT: ::c_int = 14;
pub const AF_HYLINK: ::c_int = 15;
pub const AF_APPLETALK: ::c_int = 16;
pub const AF_NIT: ::c_int = 17;
pub const AF_802: ::c_int = 18;
pub const AF_OSI: ::c_int = 19;
pub const AF_X25: ::c_int = 20;
pub const AF_OSINET: ::c_int = 21;
pub const AF_GOSIP: ::c_int = 22;
pub const AF_IPX: ::c_int = 23;
pub const AF_ROUTE: ::c_int = 24;
pub const AF_LINK: ::c_int = 25;
pub const AF_INET6: ::c_int = 26;
pub const AF_KEY: ::c_int = 27;
pub const AF_NCA: ::c_int = 28;
pub const AF_POLICY: ::c_int = 29;
pub const AF_INET_OFFLOAD: ::c_int = 30;
pub const AF_TRILL: ::c_int = 31;
pub const AF_PACKET: ::c_int = 32;
pub const AF_LX_NETLINK: ::c_int = 33;

pub const SOCK_DGRAM: ::c_int = 1;
pub const SOCK_STREAM: ::c_int = 2;
pub const SOCK_RAW: ::c_int = 4;
pub const SOCK_RDM: ::c_int = 5;
pub const SOCK_SEQPACKET: ::c_int = 6;
pub const IP_MULTICAST_IF: ::c_int = 16;
pub const IP_MULTICAST_TTL: ::c_int = 17;
pub const IP_MULTICAST_LOOP: ::c_int = 18;
pub const IP_TTL: ::c_int = 4;
pub const IP_HDRINCL: ::c_int = 2;
pub const IP_ADD_MEMBERSHIP: ::c_int = 19;
pub const IP_DROP_MEMBERSHIP: ::c_int = 20;
pub const IPV6_JOIN_GROUP: ::c_int = 9;
pub const IPV6_LEAVE_GROUP: ::c_int = 10;

pub const TCP_NODELAY: ::c_int = 1;
pub const TCP_KEEPIDLE: ::c_int = 34;
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
pub const SO_SNDBUF: ::c_int = 0x1001;
pub const SO_RCVBUF: ::c_int = 0x1002;
pub const SO_SNDLOWAT: ::c_int = 0x1003;
pub const SO_RCVLOWAT: ::c_int = 0x1004;
pub const SO_SNDTIMEO: ::c_int = 0x1005;
pub const SO_RCVTIMEO: ::c_int = 0x1006;
pub const SO_ERROR: ::c_int = 0x1007;
pub const SO_TYPE: ::c_int = 0x1008;
pub const SO_TIMESTAMP: ::c_int = 0x1013;

pub const SCM_RIGHTS: ::c_int = 0x1010;
pub const SCM_UCRED: ::c_int = 0x1012;
pub const SCM_TIMESTAMP: ::c_int = SO_TIMESTAMP;

pub const MSG_OOB: ::c_int = 0x1;
pub const MSG_PEEK: ::c_int = 0x2;
pub const MSG_DONTROUTE: ::c_int = 0x4;
pub const MSG_EOR: ::c_int = 0x8;
pub const MSG_CTRUNC: ::c_int = 0x10;
pub const MSG_TRUNC: ::c_int = 0x20;
pub const MSG_WAITALL: ::c_int = 0x40;
pub const MSG_DONTWAIT: ::c_int = 0x80;
pub const MSG_NOTIFICATION: ::c_int = 0x100;
pub const MSG_NOSIGNAL: ::c_int = 0x200;
pub const MSG_DUPCTRL: ::c_int = 0x800;
pub const MSG_XPG4_2: ::c_int = 0x8000;
pub const MSG_MAXIOVLEN: ::c_int = 16;

// https://docs.oracle.com/cd/E23824_01/html/821-1475/if-7p.html
pub const IFF_UP: ::c_int = 0x0000000001; // Address is up
pub const IFF_BROADCAST: ::c_int = 0x0000000002; // Broadcast address valid
pub const IFF_DEBUG: ::c_int = 0x0000000004; // Turn on debugging
pub const IFF_LOOPBACK: ::c_int = 0x0000000008; // Loopback net
pub const IFF_POINTOPOINT: ::c_int = 0x0000000010; // Interface is p-to-p
pub const IFF_NOTRAILERS: ::c_int = 0x0000000020; // Avoid use of trailers
pub const IFF_RUNNING: ::c_int = 0x0000000040; // Resources allocated
pub const IFF_NOARP: ::c_int = 0x0000000080; // No address res. protocol
pub const IFF_PROMISC: ::c_int = 0x0000000100; // Receive all packets
pub const IFF_ALLMULTI: ::c_int = 0x0000000200; // Receive all multicast pkts
pub const IFF_INTELLIGENT: ::c_int = 0x0000000400; // Protocol code on board
pub const IFF_MULTICAST: ::c_int = 0x0000000800; // Supports multicast

// Multicast using broadcst. add.
pub const IFF_MULTI_BCAST: ::c_int = 0x0000001000;
pub const IFF_UNNUMBERED: ::c_int = 0x0000002000; // Non-unique address
pub const IFF_DHCPRUNNING: ::c_int = 0x0000004000; // DHCP controls interface
pub const IFF_PRIVATE: ::c_int = 0x0000008000; // Do not advertise
pub const IFF_NOXMIT: ::c_int = 0x0000010000; // Do not transmit pkts

// No address - just on-link subnet
pub const IFF_NOLOCAL: ::c_int = 0x0000020000;
pub const IFF_DEPRECATED: ::c_int = 0x0000040000; // Address is deprecated
pub const IFF_ADDRCONF: ::c_int = 0x0000080000; // Addr. from stateless addrconf
pub const IFF_ROUTER: ::c_int = 0x0000100000; // Router on interface
pub const IFF_NONUD: ::c_int = 0x0000200000; // No NUD on interface
pub const IFF_ANYCAST: ::c_int = 0x0000400000; // Anycast address
pub const IFF_NORTEXCH: ::c_int = 0x0000800000; // Don't xchange rout. info
pub const IFF_IPV4: ::c_int = 0x0001000000; // IPv4 interface
pub const IFF_IPV6: ::c_int = 0x0002000000; // IPv6 interface
pub const IFF_NOFAILOVER: ::c_int = 0x0008000000; // in.mpathd test address
pub const IFF_FAILED: ::c_int = 0x0010000000; // Interface has failed
pub const IFF_STANDBY: ::c_int = 0x0020000000; // Interface is a hot-spare
pub const IFF_INACTIVE: ::c_int = 0x0040000000; // Functioning but not used
pub const IFF_OFFLINE: ::c_int = 0x0080000000; // Interface is offline
                                               // If CoS marking is supported
pub const IFF_COS_ENABLED: ::c_longlong = 0x0200000000;
pub const IFF_PREFERRED: ::c_longlong = 0x0400000000; // Prefer as source addr.
pub const IFF_TEMPORARY: ::c_longlong = 0x0800000000; // RFC3041
pub const IFF_FIXEDMTU: ::c_longlong = 0x1000000000; // MTU set with SIOCSLIFMTU
pub const IFF_VIRTUAL: ::c_longlong = 0x2000000000; // Cannot send/receive pkts
pub const IFF_DUPLICATE: ::c_longlong = 0x4000000000; // Local address in use
pub const IFF_IPMP: ::c_longlong = 0x8000000000; // IPMP IP interface

pub const SHUT_RD: ::c_int = 0;
pub const SHUT_WR: ::c_int = 1;
pub const SHUT_RDWR: ::c_int = 2;

pub const LOCK_SH: ::c_int = 1;
pub const LOCK_EX: ::c_int = 2;
pub const LOCK_NB: ::c_int = 4;
pub const LOCK_UN: ::c_int = 8;

pub const F_RDLCK: ::c_short = 1;
pub const F_WRLCK: ::c_short = 2;
pub const F_UNLCK: ::c_short = 3;

pub const O_SYNC: ::c_int = 16;
pub const O_NONBLOCK: ::c_int = 128;

pub const IPPROTO_RAW: ::c_int = 255;

pub const _PC_LINK_MAX: ::c_int = 1;
pub const _PC_MAX_CANON: ::c_int = 2;
pub const _PC_MAX_INPUT: ::c_int = 3;
pub const _PC_NAME_MAX: ::c_int = 4;
pub const _PC_PATH_MAX: ::c_int = 5;
pub const _PC_PIPE_BUF: ::c_int = 6;
pub const _PC_NO_TRUNC: ::c_int = 7;
pub const _PC_VDISABLE: ::c_int = 8;
pub const _PC_CHOWN_RESTRICTED: ::c_int = 9;
pub const _PC_ASYNC_IO: ::c_int = 10;
pub const _PC_PRIO_IO: ::c_int = 11;
pub const _PC_SYNC_IO: ::c_int = 12;
pub const _PC_ALLOC_SIZE_MIN: ::c_int = 13;
pub const _PC_REC_INCR_XFER_SIZE: ::c_int = 14;
pub const _PC_REC_MAX_XFER_SIZE: ::c_int = 15;
pub const _PC_REC_MIN_XFER_SIZE: ::c_int = 16;
pub const _PC_REC_XFER_ALIGN: ::c_int = 17;
pub const _PC_SYMLINK_MAX: ::c_int = 18;
pub const _PC_2_SYMLINKS: ::c_int = 19;
pub const _PC_ACL_ENABLED: ::c_int = 20;
pub const _PC_MIN_HOLE_SIZE: ::c_int = 21;
pub const _PC_CASE_BEHAVIOR: ::c_int = 22;
pub const _PC_SATTR_ENABLED: ::c_int = 23;
pub const _PC_SATTR_EXISTS: ::c_int = 24;
pub const _PC_ACCESS_FILTERING: ::c_int = 25;
pub const _PC_TIMESTAMP_RESOLUTION: ::c_int = 26;
pub const _PC_FILESIZEBITS: ::c_int = 67;
pub const _PC_XATTR_ENABLED: ::c_int = 100;
pub const _PC_LAST: ::c_int = 101;
pub const _PC_XATTR_EXISTS: ::c_int = 101;

pub const _SC_ARG_MAX: ::c_int = 1;
pub const _SC_CHILD_MAX: ::c_int = 2;
pub const _SC_CLK_TCK: ::c_int = 3;
pub const _SC_NGROUPS_MAX: ::c_int = 4;
pub const _SC_OPEN_MAX: ::c_int = 5;
pub const _SC_JOB_CONTROL: ::c_int = 6;
pub const _SC_SAVED_IDS: ::c_int = 7;
pub const _SC_VERSION: ::c_int = 8;
pub const _SC_PASS_MAX: ::c_int = 9;
pub const _SC_LOGNAME_MAX: ::c_int = 10;
pub const _SC_PAGESIZE: ::c_int = 11;
pub const _SC_PAGE_SIZE: ::c_int = _SC_PAGESIZE;
pub const _SC_XOPEN_VERSION: ::c_int = 12;
pub const _SC_NPROCESSORS_CONF: ::c_int = 14;
pub const _SC_NPROCESSORS_ONLN: ::c_int = 15;
pub const _SC_STREAM_MAX: ::c_int = 16;
pub const _SC_TZNAME_MAX: ::c_int = 17;
pub const _SC_AIO_LISTIO_MAX: ::c_int = 18;
pub const _SC_AIO_MAX: ::c_int = 19;
pub const _SC_AIO_PRIO_DELTA_MAX: ::c_int = 20;
pub const _SC_ASYNCHRONOUS_IO: ::c_int = 21;
pub const _SC_DELAYTIMER_MAX: ::c_int = 22;
pub const _SC_FSYNC: ::c_int = 23;
pub const _SC_MAPPED_FILES: ::c_int = 24;
pub const _SC_MEMLOCK: ::c_int = 25;
pub const _SC_MEMLOCK_RANGE: ::c_int = 26;
pub const _SC_MEMORY_PROTECTION: ::c_int = 27;
pub const _SC_MESSAGE_PASSING: ::c_int = 28;
pub const _SC_MQ_OPEN_MAX: ::c_int = 29;
pub const _SC_MQ_PRIO_MAX: ::c_int = 30;
pub const _SC_PRIORITIZED_IO: ::c_int = 31;
pub const _SC_PRIORITY_SCHEDULING: ::c_int = 32;
pub const _SC_REALTIME_SIGNALS: ::c_int = 33;
pub const _SC_RTSIG_MAX: ::c_int = 34;
pub const _SC_SEMAPHORES: ::c_int = 35;
pub const _SC_SEM_NSEMS_MAX: ::c_int = 36;
pub const _SC_SEM_VALUE_MAX: ::c_int = 37;
pub const _SC_SHARED_MEMORY_OBJECTS: ::c_int = 38;
pub const _SC_SIGQUEUE_MAX: ::c_int = 39;
pub const _SC_SIGRT_MIN: ::c_int = 40;
pub const _SC_SIGRT_MAX: ::c_int = 41;
pub const _SC_SYNCHRONIZED_IO: ::c_int = 42;
pub const _SC_TIMERS: ::c_int = 43;
pub const _SC_TIMER_MAX: ::c_int = 44;
pub const _SC_2_C_BIND: ::c_int = 45;
pub const _SC_2_C_DEV: ::c_int = 46;
pub const _SC_2_C_VERSION: ::c_int = 47;
pub const _SC_2_FORT_DEV: ::c_int = 48;
pub const _SC_2_FORT_RUN: ::c_int = 49;
pub const _SC_2_LOCALEDEF: ::c_int = 50;
pub const _SC_2_SW_DEV: ::c_int = 51;
pub const _SC_2_UPE: ::c_int = 52;
pub const _SC_2_VERSION: ::c_int = 53;
pub const _SC_BC_BASE_MAX: ::c_int = 54;
pub const _SC_BC_DIM_MAX: ::c_int = 55;
pub const _SC_BC_SCALE_MAX: ::c_int = 56;
pub const _SC_BC_STRING_MAX: ::c_int = 57;
pub const _SC_COLL_WEIGHTS_MAX: ::c_int = 58;
pub const _SC_EXPR_NEST_MAX: ::c_int = 59;
pub const _SC_LINE_MAX: ::c_int = 60;
pub const _SC_RE_DUP_MAX: ::c_int = 61;
pub const _SC_XOPEN_CRYPT: ::c_int = 62;
pub const _SC_XOPEN_ENH_I18N: ::c_int = 63;
pub const _SC_XOPEN_SHM: ::c_int = 64;
pub const _SC_2_CHAR_TERM: ::c_int = 66;
pub const _SC_XOPEN_XCU_VERSION: ::c_int = 67;
pub const _SC_ATEXIT_MAX: ::c_int = 76;
pub const _SC_IOV_MAX: ::c_int = 77;
pub const _SC_XOPEN_UNIX: ::c_int = 78;
pub const _SC_T_IOV_MAX: ::c_int = 79;
pub const _SC_PHYS_PAGES: ::c_int = 500;
pub const _SC_AVPHYS_PAGES: ::c_int = 501;
pub const _SC_COHER_BLKSZ: ::c_int = 503;
pub const _SC_SPLIT_CACHE: ::c_int = 504;
pub const _SC_ICACHE_SZ: ::c_int = 505;
pub const _SC_DCACHE_SZ: ::c_int = 506;
pub const _SC_ICACHE_LINESZ: ::c_int = 507;
pub const _SC_DCACHE_LINESZ: ::c_int = 508;
pub const _SC_ICACHE_BLKSZ: ::c_int = 509;
pub const _SC_DCACHE_BLKSZ: ::c_int = 510;
pub const _SC_DCACHE_TBLKSZ: ::c_int = 511;
pub const _SC_ICACHE_ASSOC: ::c_int = 512;
pub const _SC_DCACHE_ASSOC: ::c_int = 513;
pub const _SC_MAXPID: ::c_int = 514;
pub const _SC_STACK_PROT: ::c_int = 515;
pub const _SC_NPROCESSORS_MAX: ::c_int = 516;
pub const _SC_CPUID_MAX: ::c_int = 517;
pub const _SC_EPHID_MAX: ::c_int = 518;
pub const _SC_THREAD_DESTRUCTOR_ITERATIONS: ::c_int = 568;
pub const _SC_GETGR_R_SIZE_MAX: ::c_int = 569;
pub const _SC_GETPW_R_SIZE_MAX: ::c_int = 570;
pub const _SC_LOGIN_NAME_MAX: ::c_int = 571;
pub const _SC_THREAD_KEYS_MAX: ::c_int = 572;
pub const _SC_THREAD_STACK_MIN: ::c_int = 573;
pub const _SC_THREAD_THREADS_MAX: ::c_int = 574;
pub const _SC_TTY_NAME_MAX: ::c_int = 575;
pub const _SC_THREADS: ::c_int = 576;
pub const _SC_THREAD_ATTR_STACKADDR: ::c_int = 577;
pub const _SC_THREAD_ATTR_STACKSIZE: ::c_int = 578;
pub const _SC_THREAD_PRIORITY_SCHEDULING: ::c_int = 579;
pub const _SC_THREAD_PRIO_INHERIT: ::c_int = 580;
pub const _SC_THREAD_PRIO_PROTECT: ::c_int = 581;
pub const _SC_THREAD_PROCESS_SHARED: ::c_int = 582;
pub const _SC_THREAD_SAFE_FUNCTIONS: ::c_int = 583;
pub const _SC_XOPEN_LEGACY: ::c_int = 717;
pub const _SC_XOPEN_REALTIME: ::c_int = 718;
pub const _SC_XOPEN_REALTIME_THREADS: ::c_int = 719;
pub const _SC_XBS5_ILP32_OFF32: ::c_int = 720;
pub const _SC_XBS5_ILP32_OFFBIG: ::c_int = 721;
pub const _SC_XBS5_LP64_OFF64: ::c_int = 722;
pub const _SC_XBS5_LPBIG_OFFBIG: ::c_int = 723;
pub const _SC_2_PBS: ::c_int = 724;
pub const _SC_2_PBS_ACCOUNTING: ::c_int = 725;
pub const _SC_2_PBS_CHECKPOINT: ::c_int = 726;
pub const _SC_2_PBS_LOCATE: ::c_int = 728;
pub const _SC_2_PBS_MESSAGE: ::c_int = 729;
pub const _SC_2_PBS_TRACK: ::c_int = 730;
pub const _SC_ADVISORY_INFO: ::c_int = 731;
pub const _SC_BARRIERS: ::c_int = 732;
pub const _SC_CLOCK_SELECTION: ::c_int = 733;
pub const _SC_CPUTIME: ::c_int = 734;
pub const _SC_HOST_NAME_MAX: ::c_int = 735;
pub const _SC_MONOTONIC_CLOCK: ::c_int = 736;
pub const _SC_READER_WRITER_LOCKS: ::c_int = 737;
pub const _SC_REGEXP: ::c_int = 738;
pub const _SC_SHELL: ::c_int = 739;
pub const _SC_SPAWN: ::c_int = 740;
pub const _SC_SPIN_LOCKS: ::c_int = 741;
pub const _SC_SPORADIC_SERVER: ::c_int = 742;
pub const _SC_SS_REPL_MAX: ::c_int = 743;
pub const _SC_SYMLOOP_MAX: ::c_int = 744;
pub const _SC_THREAD_CPUTIME: ::c_int = 745;
pub const _SC_THREAD_SPORADIC_SERVER: ::c_int = 746;
pub const _SC_TIMEOUTS: ::c_int = 747;
pub const _SC_TRACE: ::c_int = 748;
pub const _SC_TRACE_EVENT_FILTER: ::c_int = 749;
pub const _SC_TRACE_EVENT_NAME_MAX: ::c_int = 750;
pub const _SC_TRACE_INHERIT: ::c_int = 751;
pub const _SC_TRACE_LOG: ::c_int = 752;
pub const _SC_TRACE_NAME_MAX: ::c_int = 753;
pub const _SC_TRACE_SYS_MAX: ::c_int = 754;
pub const _SC_TRACE_USER_EVENT_MAX: ::c_int = 755;
pub const _SC_TYPED_MEMORY_OBJECTS: ::c_int = 756;
pub const _SC_V6_ILP32_OFF32: ::c_int = 757;
pub const _SC_V6_ILP32_OFFBIG: ::c_int = 758;
pub const _SC_V6_LP64_OFF64: ::c_int = 759;
pub const _SC_V6_LPBIG_OFFBIG: ::c_int = 760;
pub const _SC_XOPEN_STREAMS: ::c_int = 761;
pub const _SC_IPV6: ::c_int = 762;
pub const _SC_RAW_SOCKETS: ::c_int = 763;

pub const _MUTEX_MAGIC: u16 = 0x4d58; // MX
pub const _COND_MAGIC: u16 = 0x4356; // CV
pub const _RWL_MAGIC: u16 = 0x5257; // RW

pub const NCCS: usize = 19;

pub const LOG_CRON: ::c_int = 15 << 3;

pub const PTHREAD_MUTEX_INITIALIZER: pthread_mutex_t = pthread_mutex_t {
    __pthread_mutex_flag1: 0,
    __pthread_mutex_flag2: 0,
    __pthread_mutex_ceiling: 0,
    __pthread_mutex_type: PTHREAD_PROCESS_PRIVATE,
    __pthread_mutex_magic: _MUTEX_MAGIC,
    __pthread_mutex_lock: 0,
    __pthread_mutex_data: 0,
};
pub const PTHREAD_COND_INITIALIZER: pthread_cond_t = pthread_cond_t {
    __pthread_cond_flag: [0; 4],
    __pthread_cond_type: PTHREAD_PROCESS_PRIVATE,
    __pthread_cond_magic: _COND_MAGIC,
    __pthread_cond_data: 0,
};
pub const PTHREAD_RWLOCK_INITIALIZER: pthread_rwlock_t = pthread_rwlock_t {
    __pthread_rwlock_readers: 0,
    __pthread_rwlock_type: PTHREAD_PROCESS_PRIVATE,
    __pthread_rwlock_magic: _RWL_MAGIC,
    __pthread_rwlock_mutex: PTHREAD_MUTEX_INITIALIZER,
    __pthread_rwlock_readercv: PTHREAD_COND_INITIALIZER,
    __pthread_rwlock_writercv: PTHREAD_COND_INITIALIZER,
};
pub const PTHREAD_MUTEX_NORMAL: ::c_int = 0;
pub const PTHREAD_MUTEX_ERRORCHECK: ::c_int = 2;
pub const PTHREAD_MUTEX_RECURSIVE: ::c_int = 4;
pub const PTHREAD_MUTEX_DEFAULT: ::c_int = PTHREAD_MUTEX_NORMAL;

pub const RTLD_NEXT: *mut ::c_void = -1isize as *mut ::c_void;
pub const RTLD_DEFAULT: *mut ::c_void = -2isize as *mut ::c_void;
pub const RTLD_SELF: *mut ::c_void = -3isize as *mut ::c_void;
pub const RTLD_PROBE: *mut ::c_void = -4isize as *mut ::c_void;

pub const RTLD_LAZY: ::c_int = 0x1;
pub const RTLD_NOW: ::c_int = 0x2;
pub const RTLD_NOLOAD: ::c_int = 0x4;
pub const RTLD_GLOBAL: ::c_int = 0x100;
pub const RTLD_LOCAL: ::c_int = 0x0;
pub const RTLD_PARENT: ::c_int = 0x200;
pub const RTLD_GROUP: ::c_int = 0x400;
pub const RTLD_WORLD: ::c_int = 0x800;
pub const RTLD_NODELETE: ::c_int = 0x1000;
pub const RTLD_FIRST: ::c_int = 0x2000;
pub const RTLD_CONFGEN: ::c_int = 0x10000;

pub const PORT_SOURCE_AIO: ::c_int = 1;
pub const PORT_SOURCE_TIMER: ::c_int = 2;
pub const PORT_SOURCE_USER: ::c_int = 3;
pub const PORT_SOURCE_FD: ::c_int = 4;
pub const PORT_SOURCE_ALERT: ::c_int = 5;
pub const PORT_SOURCE_MQ: ::c_int = 6;
pub const PORT_SOURCE_FILE: ::c_int = 7;
pub const PORT_SOURCE_POSTWAIT: ::c_int = 8;
pub const PORT_SOURCE_SIGNAL: ::c_int = 9;

const _TIOC: ::c_int = ('T' as i32) << 8;
const tIOC: ::c_int = ('t' as i32) << 8;
pub const TCGETA: ::c_int = (_TIOC | 1);
pub const TCSETA: ::c_int = (_TIOC | 2);
pub const TCSETAW: ::c_int = (_TIOC | 3);
pub const TCSETAF: ::c_int = (_TIOC | 4);
pub const TCSBRK: ::c_int = (_TIOC | 5);
pub const TCXONC: ::c_int = (_TIOC | 6);
pub const TCFLSH: ::c_int = (_TIOC | 7);
pub const TCDSET: ::c_int = (_TIOC | 32);
pub const TCGETS: ::c_int = (_TIOC | 13);
pub const TCSETS: ::c_int = (_TIOC | 14);
pub const TCSANOW: ::c_int = (_TIOC | 14);
pub const TCSETSW: ::c_int = (_TIOC | 15);
pub const TCSADRAIN: ::c_int = (_TIOC | 15);
pub const TCSETSF: ::c_int = (_TIOC | 16);
pub const TCSAFLUSH: ::c_int = (_TIOC | 16);
pub const TCIFLUSH: ::c_int = 0;
pub const TCOFLUSH: ::c_int = 1;
pub const TCIOFLUSH: ::c_int = 2;
pub const TCOOFF: ::c_int = 0;
pub const TCOON: ::c_int = 1;
pub const TCIOFF: ::c_int = 2;
pub const TCION: ::c_int = 3;
pub const TIOC: ::c_int = _TIOC;
pub const TIOCKBON: ::c_int = (_TIOC | 8);
pub const TIOCKBOF: ::c_int = (_TIOC | 9);
pub const TIOCGWINSZ: ::c_int = (_TIOC | 104);
pub const TIOCSWINSZ: ::c_int = (_TIOC | 103);
pub const TIOCGSOFTCAR: ::c_int = (_TIOC | 105);
pub const TIOCSSOFTCAR: ::c_int = (_TIOC | 106);
pub const TIOCSETLD: ::c_int = (_TIOC | 123);
pub const TIOCGETLD: ::c_int = (_TIOC | 124);
pub const TIOCGPPS: ::c_int = (_TIOC | 125);
pub const TIOCSPPS: ::c_int = (_TIOC | 126);
pub const TIOCGPPSEV: ::c_int = (_TIOC | 127);
pub const TIOCGETD: ::c_int = (tIOC | 0);
pub const TIOCSETD: ::c_int = (tIOC | 1);
pub const TIOCHPCL: ::c_int = (tIOC | 2);
pub const TIOCGETP: ::c_int = (tIOC | 8);
pub const TIOCSETP: ::c_int = (tIOC | 9);
pub const TIOCSETN: ::c_int = (tIOC | 10);
pub const TIOCEXCL: ::c_int = (tIOC | 13);
pub const TIOCNXCL: ::c_int = (tIOC | 14);
pub const TIOCFLUSH: ::c_int = (tIOC | 16);
pub const TIOCSETC: ::c_int = (tIOC | 17);
pub const TIOCGETC: ::c_int = (tIOC | 18);
pub const TIOCLBIS: ::c_int = (tIOC | 127);
pub const TIOCLBIC: ::c_int = (tIOC | 126);
pub const TIOCLSET: ::c_int = (tIOC | 125);
pub const TIOCLGET: ::c_int = (tIOC | 124);
pub const TIOCSBRK: ::c_int = (tIOC | 123);
pub const TIOCCBRK: ::c_int = (tIOC | 122);
pub const TIOCSDTR: ::c_int = (tIOC | 121);
pub const TIOCCDTR: ::c_int = (tIOC | 120);
pub const TIOCSLTC: ::c_int = (tIOC | 117);
pub const TIOCGLTC: ::c_int = (tIOC | 116);
pub const TIOCOUTQ: ::c_int = (tIOC | 115);
pub const TIOCNOTTY: ::c_int = (tIOC | 113);
pub const TIOCSCTTY: ::c_int = (tIOC | 132);
pub const TIOCSTOP: ::c_int = (tIOC | 111);
pub const TIOCSTART: ::c_int = (tIOC | 110);
pub const TIOCSILOOP: ::c_int = (tIOC | 109);
pub const TIOCCILOOP: ::c_int = (tIOC | 108);
pub const TIOCGPGRP: ::c_int = (tIOC | 20);
pub const TIOCSPGRP: ::c_int = (tIOC | 21);
pub const TIOCGSID: ::c_int = (tIOC | 22);
pub const TIOCSTI: ::c_int = (tIOC | 23);
pub const TIOCMSET: ::c_int = (tIOC | 26);
pub const TIOCMBIS: ::c_int = (tIOC | 27);
pub const TIOCMBIC: ::c_int = (tIOC | 28);
pub const TIOCMGET: ::c_int = (tIOC | 29);
pub const TIOCREMOTE: ::c_int = (tIOC | 30);
pub const TIOCSIGNAL: ::c_int = (tIOC | 31);

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
pub const EPOLLRDHUP: ::c_int = 0x2000;
pub const EPOLLEXCLUSIVE: ::c_int = 0x10000000;
pub const EPOLLONESHOT: ::c_int = 0x40000000;
pub const EPOLL_CLOEXEC: ::c_int = 0x80000;
pub const EPOLL_CTL_ADD: ::c_int = 1;
pub const EPOLL_CTL_MOD: ::c_int = 3;
pub const EPOLL_CTL_DEL: ::c_int = 2;

/* termios */
pub const B0: speed_t = 0;
pub const B50: speed_t = 1;
pub const B75: speed_t = 2;
pub const B110: speed_t = 3;
pub const B134: speed_t = 4;
pub const B150: speed_t = 5;
pub const B200: speed_t = 6;
pub const B300: speed_t = 7;
pub const B600: speed_t = 8;
pub const B1200: speed_t = 9;
pub const B1800: speed_t = 10;
pub const B2400: speed_t = 11;
pub const B4800: speed_t = 12;
pub const B9600: speed_t = 13;
pub const B19200: speed_t = 14;
pub const B38400: speed_t = 15;
pub const B57600: speed_t = 16;
pub const B76800: speed_t = 17;
pub const B115200: speed_t = 18;
pub const B153600: speed_t = 19;
pub const B230400: speed_t = 20;
pub const B307200: speed_t = 21;
pub const B460800: speed_t = 22;
pub const B921600: speed_t = 23;
pub const CSTART: ::tcflag_t = 021;
pub const CSTOP: ::tcflag_t = 023;
pub const CSWTCH: ::tcflag_t = 032;
pub const CSIZE: ::tcflag_t = 0o000060;
pub const CS5: ::tcflag_t = 0;
pub const CS6: ::tcflag_t = 0o000020;
pub const CS7: ::tcflag_t = 0o000040;
pub const CS8: ::tcflag_t = 0o000060;
pub const CSTOPB: ::tcflag_t = 0o000100;
pub const ECHO: ::tcflag_t = 0o000010;
pub const ECHOE: ::tcflag_t = 0o000020;
pub const ECHOK: ::tcflag_t = 0o000040;
pub const ECHONL: ::tcflag_t = 0o000100;
pub const ECHOCTL: ::tcflag_t = 0o001000;
pub const ECHOPRT: ::tcflag_t = 0o002000;
pub const ECHOKE: ::tcflag_t = 0o004000;
pub const EXTPROC: ::tcflag_t = 0o200000;
pub const IGNBRK: ::tcflag_t = 0o000001;
pub const BRKINT: ::tcflag_t = 0o000002;
pub const IGNPAR: ::tcflag_t = 0o000004;
pub const PARMRK: ::tcflag_t = 0o000010;
pub const INPCK: ::tcflag_t = 0o000020;
pub const ISTRIP: ::tcflag_t = 0o000040;
pub const INLCR: ::tcflag_t = 0o000100;
pub const IGNCR: ::tcflag_t = 0o000200;
pub const ICRNL: ::tcflag_t = 0o000400;
pub const IXON: ::tcflag_t = 0o002000;
pub const IXOFF: ::tcflag_t = 0o010000;
pub const IXANY: ::tcflag_t = 0o004000;
pub const IMAXBEL: ::tcflag_t = 0o020000;
pub const OPOST: ::tcflag_t = 0o000001;
pub const ONLCR: ::tcflag_t = 0o000004;
pub const OCRNL: ::tcflag_t = 0o000010;
pub const ONOCR: ::tcflag_t = 0o000020;
pub const ONLRET: ::tcflag_t = 0o000040;
pub const CREAD: ::tcflag_t = 0o000200;
pub const PARENB: ::tcflag_t = 0o000400;
pub const PARODD: ::tcflag_t = 0o001000;
pub const HUPCL: ::tcflag_t = 0o002000;
pub const CLOCAL: ::tcflag_t = 0o004000;
pub const CRTSCTS: ::tcflag_t = 0o20000000000;
pub const ISIG: ::tcflag_t = 0o000001;
pub const ICANON: ::tcflag_t = 0o000002;
pub const IEXTEN: ::tcflag_t = 0o100000;
pub const TOSTOP: ::tcflag_t = 0o000400;
pub const FLUSHO: ::tcflag_t = 0o020000;
pub const PENDIN: ::tcflag_t = 0o040000;
pub const NOFLSH: ::tcflag_t = 0o000200;
pub const VINTR: usize = 0;
pub const VQUIT: usize = 1;
pub const VERASE: usize = 2;
pub const VKILL: usize = 3;
pub const VEOF: usize = 4;
pub const VEOL: usize = 5;
pub const VEOL2: usize = 6;
pub const VMIN: usize = 4;
pub const VTIME: usize = 5;
pub const VSWTCH: usize = 7;
pub const VSTART: usize = 8;
pub const VSTOP: usize = 9;
pub const VSUSP: usize = 10;
pub const VDSUSP: usize = 11;
pub const VREPRINT: usize = 12;
pub const VDISCARD: usize = 13;
pub const VWERASE: usize = 14;
pub const VLNEXT: usize = 15;
pub const VSTATUS: usize = 16;
pub const VERASE2: usize = 17;

f! {
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

    pub fn WIFEXITED(status: ::c_int) -> bool {
        (status & 0xFF) == 0
    }

    pub fn WEXITSTATUS(status: ::c_int) -> ::c_int {
        (status >> 8) & 0xFF
    }

    pub fn WTERMSIG(status: ::c_int) -> ::c_int {
        status & 0x7F
    }

    pub fn WIFCONTINUED(status: ::c_int) -> bool {
        (status & 0xffff) == 0xffff
    }

    pub fn WSTOPSIG(status: ::c_int) -> ::c_int {
        (status & 0xff00) >> 8
    }

    pub fn WIFSIGNALED(status: ::c_int) -> bool {
        ((status & 0xff) > 0) && (status & 0xff00 == 0)
    }

    pub fn WIFSTOPPED(status: ::c_int) -> bool {
        ((status & 0xff) == 0x7f) && ((status & 0xff00) != 0)
    }

    pub fn WCOREDUMP(status: ::c_int) -> bool {
        (status & 0x80) != 0
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
    pub fn acct(filename: *const ::c_char) -> ::c_int;
    pub fn atof(s: *const ::c_char) -> ::c_double;
    pub fn dirfd(dirp: *mut ::DIR) -> ::c_int;
    pub fn labs(i: ::c_long) -> ::c_long;
    pub fn rand() -> ::c_int;
    pub fn srand(seed: ::c_uint);

    pub fn gettimeofday(tp: *mut ::timeval, tz: *mut ::c_void) -> ::c_int;
    pub fn getifaddrs(ifap: *mut *mut ::ifaddrs) -> ::c_int;
    pub fn freeifaddrs(ifa: *mut ::ifaddrs);

    pub fn stack_getbounds(sp: *mut ::stack_t) -> ::c_int;
    pub fn mincore(
        addr: *const ::c_void,
        len: ::size_t,
        vec: *mut c_char,
    ) -> ::c_int;
    pub fn initgroups(name: *const ::c_char, basegid: ::gid_t) -> ::c_int;
    pub fn setgroups(ngroups: ::c_int, ptr: *const ::gid_t) -> ::c_int;
    pub fn ioctl(fildes: ::c_int, request: ::c_int, ...) -> ::c_int;
    pub fn mprotect(
        addr: *const ::c_void,
        len: ::size_t,
        prot: ::c_int,
    ) -> ::c_int;
    pub fn ___errno() -> *mut ::c_int;
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
    pub fn getnameinfo(
        sa: *const ::sockaddr,
        salen: ::socklen_t,
        host: *mut ::c_char,
        hostlen: ::socklen_t,
        serv: *mut ::c_char,
        sevlen: ::socklen_t,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn setpwent();
    pub fn endpwent();
    pub fn getpwent() -> *mut passwd;
    pub fn fdatasync(fd: ::c_int) -> ::c_int;
    pub fn nl_langinfo_l(item: ::nl_item, locale: ::locale_t)
        -> *mut ::c_char;
    pub fn duplocale(base: ::locale_t) -> ::locale_t;
    pub fn freelocale(loc: ::locale_t);
    pub fn newlocale(
        mask: ::c_int,
        locale: *const ::c_char,
        base: ::locale_t,
    ) -> ::locale_t;
    pub fn uselocale(loc: ::locale_t) -> ::locale_t;
    pub fn getprogname() -> *const ::c_char;
    pub fn setprogname(name: *const ::c_char);
    pub fn getloadavg(loadavg: *mut ::c_double, nelem: ::c_int) -> ::c_int;
    pub fn getpriority(which: ::c_int, who: ::c_int) -> ::c_int;
    pub fn setpriority(which: ::c_int, who: ::c_int, prio: ::c_int)
        -> ::c_int;

    pub fn mknodat(
        dirfd: ::c_int,
        pathname: *const ::c_char,
        mode: ::mode_t,
        dev: dev_t,
    ) -> ::c_int;
    pub fn mkfifoat(
        dirfd: ::c_int,
        pathname: *const ::c_char,
        mode: ::mode_t,
    ) -> ::c_int;
    pub fn sethostname(name: *const ::c_char, len: ::c_int) -> ::c_int;
    pub fn if_nameindex() -> *mut if_nameindex;
    pub fn if_freenameindex(ptr: *mut if_nameindex);
    pub fn pthread_create(
        native: *mut ::pthread_t,
        attr: *const ::pthread_attr_t,
        f: extern "C" fn(*mut ::c_void) -> *mut ::c_void,
        value: *mut ::c_void,
    ) -> ::c_int;
    pub fn pthread_condattr_getclock(
        attr: *const pthread_condattr_t,
        clock_id: *mut clockid_t,
    ) -> ::c_int;
    pub fn pthread_condattr_setclock(
        attr: *mut pthread_condattr_t,
        clock_id: ::clockid_t,
    ) -> ::c_int;
    pub fn sem_timedwait(
        sem: *mut sem_t,
        abstime: *const ::timespec,
    ) -> ::c_int;
    pub fn sem_getvalue(sem: *mut sem_t, sval: *mut ::c_int) -> ::c_int;
    pub fn pthread_mutex_timedlock(
        lock: *mut pthread_mutex_t,
        abstime: *const ::timespec,
    ) -> ::c_int;
    pub fn waitid(
        idtype: idtype_t,
        id: id_t,
        infop: *mut ::siginfo_t,
        options: ::c_int,
    ) -> ::c_int;

    pub fn glob(
        pattern: *const ::c_char,
        flags: ::c_int,
        errfunc: ::Option<
            extern "C" fn(epath: *const ::c_char, errno: ::c_int) -> ::c_int,
        >,
        pglob: *mut ::glob_t,
    ) -> ::c_int;

    pub fn globfree(pglob: *mut ::glob_t);

    pub fn posix_madvise(
        addr: *mut ::c_void,
        len: ::size_t,
        advice: ::c_int,
    ) -> ::c_int;

    pub fn shm_open(
        name: *const ::c_char,
        oflag: ::c_int,
        mode: ::mode_t,
    ) -> ::c_int;
    pub fn shm_unlink(name: *const ::c_char) -> ::c_int;

    pub fn seekdir(dirp: *mut ::DIR, loc: ::c_long);

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

    pub fn memalign(align: ::size_t, size: ::size_t) -> *mut ::c_void;

    pub fn recvfrom(
        socket: ::c_int,
        buf: *mut ::c_void,
        len: ::size_t,
        flags: ::c_int,
        addr: *mut ::sockaddr,
        addrlen: *mut ::socklen_t,
    ) -> ::ssize_t;
    pub fn mkstemps(template: *mut ::c_char, suffixlen: ::c_int) -> ::c_int;
    pub fn futimesat(
        fd: ::c_int,
        path: *const ::c_char,
        times: *const ::timeval,
    ) -> ::c_int;
    pub fn utimensat(
        dirfd: ::c_int,
        path: *const ::c_char,
        times: *const ::timespec,
        flag: ::c_int,
    ) -> ::c_int;
    pub fn nl_langinfo(item: ::nl_item) -> *mut ::c_char;

    #[cfg_attr(target_os = "illumos", link_name = "__xnet_bind")]
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

    #[cfg_attr(target_os = "illumos", link_name = "__xnet_sendmsg")]
    pub fn sendmsg(
        fd: ::c_int,
        msg: *const ::msghdr,
        flags: ::c_int,
    ) -> ::ssize_t;
    #[cfg_attr(target_os = "illumos", link_name = "__xnet_recvmsg")]
    pub fn recvmsg(
        fd: ::c_int,
        msg: *mut ::msghdr,
        flags: ::c_int,
    ) -> ::ssize_t;

    pub fn mq_open(name: *const ::c_char, oflag: ::c_int, ...) -> ::mqd_t;
    pub fn mq_close(mqd: ::mqd_t) -> ::c_int;
    pub fn mq_unlink(name: *const ::c_char) -> ::c_int;
    pub fn mq_receive(
        mqd: ::mqd_t,
        msg_ptr: *mut ::c_char,
        msg_len: ::size_t,
        msg_prio: *mut ::c_uint,
    ) -> ::ssize_t;
    pub fn mq_timedreceive(
        mqd: ::mqd_t,
        msg_ptr: *mut ::c_char,
        msg_len: ::size_t,
        msg_prio: *mut ::c_uint,
        abs_timeout: *const ::timespec,
    ) -> ::ssize_t;
    pub fn mq_send(
        mqd: ::mqd_t,
        msg_ptr: *const ::c_char,
        msg_len: ::size_t,
        msg_prio: ::c_uint,
    ) -> ::c_int;
    pub fn mq_timedsend(
        mqd: ::mqd_t,
        msg_ptr: *const ::c_char,
        msg_len: ::size_t,
        msg_prio: ::c_uint,
        abs_timeout: *const ::timespec,
    ) -> ::c_int;
    pub fn mq_getattr(mqd: ::mqd_t, attr: *mut ::mq_attr) -> ::c_int;
    pub fn mq_setattr(
        mqd: ::mqd_t,
        newattr: *const ::mq_attr,
        oldattr: *mut ::mq_attr,
    ) -> ::c_int;
    pub fn port_create() -> ::c_int;
    pub fn port_associate(
        port: ::c_int,
        source: ::c_int,
        object: ::uintptr_t,
        events: ::c_int,
        user: *mut ::c_void,
    ) -> ::c_int;
    pub fn port_dissociate(
        port: ::c_int,
        source: ::c_int,
        object: ::uintptr_t,
    ) -> ::c_int;
    pub fn port_get(
        port: ::c_int,
        pe: *mut port_event,
        timeout: *mut ::timespec,
    ) -> ::c_int;
    pub fn port_getn(
        port: ::c_int,
        pe_list: *mut port_event,
        max: ::c_uint,
        nget: *mut ::c_uint,
        timeout: *mut ::timespec,
    ) -> ::c_int;
    pub fn fexecve(
        fd: ::c_int,
        argv: *const *const ::c_char,
        envp: *const *const ::c_char,
    ) -> ::c_int;
    #[cfg_attr(
        any(target_os = "solaris", target_os = "illumos"),
        link_name = "__posix_getgrgid_r"
    )]
    pub fn getgrgid_r(
        gid: ::gid_t,
        grp: *mut ::group,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut ::group,
    ) -> ::c_int;
    pub fn sigaltstack(ss: *const stack_t, oss: *mut stack_t) -> ::c_int;
    pub fn sem_close(sem: *mut sem_t) -> ::c_int;
    pub fn getdtablesize() -> ::c_int;

    // The epoll functions are actually only present on illumos.  However,
    // there are things using epoll on illumos (built using the
    // x86_64-sun-solaris target) which would break until the illumos target is
    // present in rustc.
    pub fn epoll_pwait(
        epfd: ::c_int,
        events: *mut ::epoll_event,
        maxevents: ::c_int,
        timeout: ::c_int,
        sigmask: *const ::sigset_t,
    ) -> ::c_int;

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

    #[cfg_attr(
        any(target_os = "solaris", target_os = "illumos"),
        link_name = "__posix_getgrnam_r"
    )]
    pub fn getgrnam_r(
        name: *const ::c_char,
        grp: *mut ::group,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut ::group,
    ) -> ::c_int;
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
    #[cfg_attr(
        any(target_os = "solaris", target_os = "illumos"),
        link_name = "__posix_getpwnam_r"
    )]
    pub fn getpwnam_r(
        name: *const ::c_char,
        pwd: *mut passwd,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut passwd,
    ) -> ::c_int;
    #[cfg_attr(
        any(target_os = "solaris", target_os = "illumos"),
        link_name = "__posix_getpwuid_r"
    )]
    pub fn getpwuid_r(
        uid: ::uid_t,
        pwd: *mut passwd,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut passwd,
    ) -> ::c_int;
    #[cfg_attr(
        any(target_os = "solaris", target_os = "illumos"),
        link_name = "__posix_getpwent_r"
    )]
    pub fn getpwent_r(
        pwd: *mut passwd,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut passwd,
    ) -> ::c_int;
    #[cfg_attr(
        any(target_os = "solaris", target_os = "illumos"),
        link_name = "__posix_getgrent_r"
    )]
    pub fn getgrent_r(
        grp: *mut ::group,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut ::group,
    ) -> ::c_int;
    #[cfg_attr(
        any(target_os = "solaris", target_os = "illumos"),
        link_name = "__posix_sigwait"
    )]
    pub fn sigwait(set: *const sigset_t, sig: *mut ::c_int) -> ::c_int;
    pub fn pthread_atfork(
        prepare: ::Option<unsafe extern "C" fn()>,
        parent: ::Option<unsafe extern "C" fn()>,
        child: ::Option<unsafe extern "C" fn()>,
    ) -> ::c_int;
    pub fn getgrgid(gid: ::gid_t) -> *mut ::group;
    pub fn setgrent();
    pub fn endgrent();
    pub fn getgrent() -> *mut ::group;
    pub fn popen(command: *const c_char, mode: *const c_char) -> *mut ::FILE;

    pub fn dup3(src: ::c_int, dst: ::c_int, flags: ::c_int) -> ::c_int;
    pub fn uname(buf: *mut ::utsname) -> ::c_int;
    pub fn pipe2(fds: *mut ::c_int, flags: ::c_int) -> ::c_int;
    pub fn door_call(d: ::c_int, params: *const door_arg_t) -> ::c_int;
    pub fn door_return(
        data_ptr: *const ::c_char,
        data_size: ::size_t,
        desc_ptr: *const door_desc_t,
        num_desc: ::c_uint,
    );
    pub fn door_create(
        server_procedure: extern "C" fn(
            cookie: *const ::c_void,
            argp: *const ::c_char,
            arg_size: ::size_t,
            dp: *const door_desc_t,
            n_desc: ::c_uint,
        ),
        cookie: *const ::c_void,
        attributes: door_attr_t,
    ) -> ::c_int;
    pub fn fattach(fildes: ::c_int, path: *const ::c_char) -> ::c_int;
}

mod compat;
pub use self::compat::*;
