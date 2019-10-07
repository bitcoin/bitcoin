pub type dev_t = u32;
pub type c_char = i8;
pub type clock_t = u64;
pub type ino_t = u64;
pub type lwpid_t = i32;
pub type nlink_t = u32;
pub type blksize_t = i64;
pub type clockid_t = ::c_ulong;

pub type c_long = i64;
pub type c_ulong = u64;
pub type time_t = i64;
pub type suseconds_t = i64;

pub type uuid_t = ::uuid;

pub type fsblkcnt_t = u64;
pub type fsfilcnt_t = u64;

pub type mqd_t = ::c_int;
pub type sem_t = *mut sem;

#[cfg_attr(feature = "extra_traits", derive(Debug))]
pub enum sem {}
impl ::Copy for sem {}
impl ::Clone for sem {
    fn clone(&self) -> sem {
        *self
    }
}

s! {
    pub struct kevent {
        pub ident: ::uintptr_t,
        pub filter: ::c_short,
        pub flags: ::c_ushort,
        pub fflags: ::c_uint,
        pub data: ::intptr_t,
        pub udata: *mut ::c_void,
    }

    pub struct exit_status {
        pub e_termination: u16,
        pub e_exit: u16
    }

    pub struct aiocb {
        pub aio_fildes: ::c_int,
        pub aio_offset: ::off_t,
        pub aio_buf: *mut ::c_void,
        pub aio_nbytes: ::size_t,
        pub aio_sigevent: sigevent,
        pub aio_lio_opcode: ::c_int,
        pub aio_reqprio: ::c_int,
        _aio_val: ::c_int,
        _aio_err: ::c_int
    }

    pub struct uuid {
        pub time_low: u32,
        pub time_mid: u16,
        pub time_hi_and_version: u16,
        pub clock_seq_hi_and_reserved: u8,
        pub clock_seq_low: u8,
        pub node: [u8; 6],
    }

    pub struct mq_attr {
        pub mq_flags: ::c_long,
        pub mq_maxmsg: ::c_long,
        pub mq_msgsize: ::c_long,
        pub mq_curmsgs: ::c_long,
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
        pub f_owner: ::uid_t,
        pub f_type: ::c_uint,
        pub f_syncreads: u64,
        pub f_syncwrites: u64,
        pub f_asyncreads: u64,
        pub f_asyncwrites: u64,
        pub f_fsid_uuid: ::uuid_t,
        pub f_uid_uuid: ::uuid_t,
    }

    pub struct stat {
        pub st_ino: ::ino_t,
        pub st_nlink: ::nlink_t,
        pub st_dev: ::dev_t,
        pub st_mode: ::mode_t,
        pub st_padding1: u16,
        pub st_uid: ::uid_t,
        pub st_gid: ::gid_t,
        pub st_rdev: ::dev_t,
        pub st_atime: ::time_t,
        pub st_atime_nsec: ::c_long,
        pub st_mtime: ::time_t,
        pub st_mtime_nsec: ::c_long,
        pub st_ctime: ::time_t,
        pub st_ctime_nsec: ::c_long,
        pub st_size: ::off_t,
        pub st_blocks: i64,
        pub st_blksize: u32,
        pub st_flags: u32,
        pub st_gen: u32,
        pub st_lspare: i32,
        pub st_qspare1: i64,
        pub st_qspare2: i64,
    }

    pub struct if_data {
        pub ifi_type: ::c_uchar,
        pub ifi_physical: ::c_uchar,
        pub ifi_addrlen: ::c_uchar,
        pub ifi_hdrlen: ::c_uchar,
        pub ifi_recvquota: ::c_uchar,
        pub ifi_xmitquota: ::c_uchar,
        pub ifi_mtu: ::c_ulong,
        pub ifi_metric: ::c_ulong,
        pub ifi_link_state: ::c_ulong,
        pub ifi_baudrate: u64,
        pub ifi_ipackets: ::c_ulong,
        pub ifi_ierrors: ::c_ulong,
        pub ifi_opackets: ::c_ulong,
        pub ifi_oerrors: ::c_ulong,
        pub ifi_collisions: ::c_ulong,
        pub ifi_ibytes: ::c_ulong,
        pub ifi_obytes: ::c_ulong,
        pub ifi_imcasts: ::c_ulong,
        pub ifi_omcasts: ::c_ulong,
        pub ifi_iqdrops: ::c_ulong,
        pub ifi_noproto: ::c_ulong,
        pub ifi_hwassist: ::c_ulong,
        pub ifi_oqdrops: ::c_ulong,
        pub ifi_lastchange: ::timeval,
    }

    pub struct if_msghdr {
        pub ifm_msglen: ::c_ushort,
        pub ifm_version: ::c_uchar,
        pub ifm_type: ::c_uchar,
        pub ifm_addrs: ::c_int,
        pub ifm_flags: ::c_int,
        pub ifm_index: ::c_ushort,
        pub ifm_data: if_data,
    }

    pub struct sockaddr_dl {
        pub sdl_len: ::c_uchar,
        pub sdl_family: ::c_uchar,
        pub sdl_index: ::c_ushort,
        pub sdl_type: ::c_uchar,
        pub sdl_nlen: ::c_uchar,
        pub sdl_alen: ::c_uchar,
        pub sdl_slen: ::c_uchar,
        pub sdl_data: [::c_char; 12],
        pub sdl_rcf: ::c_ushort,
        pub sdl_route: [::c_ushort; 16],
    }

    pub struct stack_t {
        pub ss_sp: *mut ::c_char,
        pub ss_size: ::size_t,
        pub ss_flags: ::c_int,
    }
}

s_no_extra_traits! {
    pub struct utmpx {
        pub ut_name: [::c_char; 32],
        pub ut_id: [::c_char; 4],

        pub ut_line: [::c_char; 32],
        pub ut_host: [::c_char; 256],

        pub ut_unused: [u8; 16],
        pub ut_session: u16,
        pub ut_type: u16,
        pub ut_pid: ::pid_t,
        ut_exit: exit_status,
        ut_ss: ::sockaddr_storage,
        pub ut_tv: ::timeval,
        pub ut_unused2: [u8; 16],
    }

    pub struct dirent {
        pub d_fileno: ::ino_t,
        pub d_namlen: u16,
        pub d_type: u8,
        __unused1: u8,
        __unused2: u32,
        pub d_name: [::c_char; 256],
    }

    pub struct statfs {
        pub f_bsize: ::c_long,
        pub f_iosize: ::c_long,
        pub f_blocks: ::c_long,
        pub f_bfree: ::c_long,
        pub f_bavail: ::c_long,
        pub f_files: ::c_long,
        pub f_ffree: ::c_long,
        pub f_fsid: ::fsid_t,
        pub f_owner: ::uid_t,
        pub f_type: i32,
        pub f_flags: i32,
        pub f_syncwrites: ::c_long,
        pub f_asyncwrites: ::c_long,
        pub f_fstypename: [::c_char; 16],
        pub f_mntonname: [::c_char; 90],
        pub f_syncreads: ::c_long,
        pub f_asyncreads: ::c_long,
        pub f_mntfromname: [::c_char; 90],
    }

    pub struct sigevent {
        pub sigev_notify: ::c_int,
        // The union is 8-byte in size, so it is aligned at a 8-byte offset.
        #[cfg(target_pointer_width = "64")]
        __unused1: ::c_int,
        pub sigev_signo: ::c_int,       //actually a union
        // pad the union
        #[cfg(target_pointer_width = "64")]
        __unused2: ::c_int,
        pub sigev_value: ::sigval,
        __unused3: *mut ::c_void        //actually a function pointer
    }

}

cfg_if! {
    if #[cfg(feature = "extra_traits")] {
        impl PartialEq for utmpx {
            fn eq(&self, other: &utmpx) -> bool {
                self.ut_name == other.ut_name
                    && self.ut_id == other.ut_id
                    && self.ut_line == other.ut_line
                    && self
                    .ut_host
                    .iter()
                    .zip(other.ut_host.iter())
                    .all(|(a,b)| a == b)
                    && self.ut_unused == other.ut_unused
                    && self.ut_session == other.ut_session
                    && self.ut_type == other.ut_type
                    && self.ut_pid == other.ut_pid
                    && self.ut_exit == other.ut_exit
                    && self.ut_ss == other.ut_ss
                    && self.ut_tv == other.ut_tv
                    && self.ut_unused2 == other.ut_unused2
            }
        }
        impl Eq for utmpx {}
        impl ::fmt::Debug for utmpx {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("utmpx")
                    .field("ut_name", &self.ut_name)
                    .field("ut_id", &self.ut_id)
                    .field("ut_line", &self.ut_line)
                    // FIXME: .field("ut_host", &self.ut_host)
                    .field("ut_unused", &self.ut_unused)
                    .field("ut_session", &self.ut_session)
                    .field("ut_type", &self.ut_type)
                    .field("ut_pid", &self.ut_pid)
                    .field("ut_exit", &self.ut_exit)
                    .field("ut_ss", &self.ut_ss)
                    .field("ut_tv", &self.ut_tv)
                    .field("ut_unused2", &self.ut_unused2)
                    .finish()
            }
        }
        impl ::hash::Hash for utmpx {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.ut_name.hash(state);
                self.ut_id.hash(state);
                self.ut_line.hash(state);
                self.ut_host.hash(state);
                self.ut_unused.hash(state);
                self.ut_session.hash(state);
                self.ut_type.hash(state);
                self.ut_pid.hash(state);
                self.ut_exit.hash(state);
                self.ut_ss.hash(state);
                self.ut_tv.hash(state);
                self.ut_unused2.hash(state);
            }
        }

        impl PartialEq for dirent {
            fn eq(&self, other: &dirent) -> bool {
                self.d_fileno == other.d_fileno
                    && self.d_namlen == other.d_namlen
                    && self.d_type == other.d_type
                    // Ignore __unused1
                    // Ignore __unused2
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
                    .field("d_fileno", &self.d_fileno)
                    .field("d_namlen", &self.d_namlen)
                    .field("d_type", &self.d_type)
                    // Ignore __unused1
                    // Ignore __unused2
                    // FIXME: .field("d_name", &self.d_name)
                    .finish()
            }
        }
        impl ::hash::Hash for dirent {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.d_fileno.hash(state);
                self.d_namlen.hash(state);
                self.d_type.hash(state);
                    // Ignore __unused1
                    // Ignore __unused2
                self.d_name.hash(state);
            }
        }

        impl PartialEq for statfs {
            fn eq(&self, other: &statfs) -> bool {
                self.f_bsize == other.f_bsize
                    && self.f_iosize == other.f_iosize
                    && self.f_blocks == other.f_blocks
                    && self.f_bfree == other.f_bfree
                    && self.f_bavail == other.f_bavail
                    && self.f_files == other.f_files
                    && self.f_ffree == other.f_ffree
                    && self.f_fsid == other.f_fsid
                    && self.f_owner == other.f_owner
                    && self.f_type == other.f_type
                    && self.f_flags == other.f_flags
                    && self.f_syncwrites == other.f_syncwrites
                    && self.f_asyncwrites == other.f_asyncwrites
                    && self.f_fstypename == other.f_fstypename
                    && self
                    .f_mntonname
                    .iter()
                    .zip(other.f_mntonname.iter())
                    .all(|(a,b)| a == b)
                    && self.f_syncreads == other.f_syncreads
                    && self.f_asyncreads == other.f_asyncreads
                    && self
                    .f_mntfromname
                    .iter()
                    .zip(other.f_mntfromname.iter())
                    .all(|(a,b)| a == b)
            }
        }
        impl Eq for statfs {}
        impl ::fmt::Debug for statfs {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("statfs")
                    .field("f_bsize", &self.f_bsize)
                    .field("f_iosize", &self.f_iosize)
                    .field("f_blocks", &self.f_blocks)
                    .field("f_bfree", &self.f_bfree)
                    .field("f_bavail", &self.f_bavail)
                    .field("f_files", &self.f_files)
                    .field("f_ffree", &self.f_ffree)
                    .field("f_fsid", &self.f_fsid)
                    .field("f_owner", &self.f_owner)
                    .field("f_type", &self.f_type)
                    .field("f_flags", &self.f_flags)
                    .field("f_syncwrites", &self.f_syncwrites)
                    .field("f_asyncwrites", &self.f_asyncwrites)
                    // FIXME: .field("f_mntonname", &self.f_mntonname)
                    .field("f_syncreads", &self.f_syncreads)
                    .field("f_asyncreads", &self.f_asyncreads)
                    // FIXME: .field("f_mntfromname", &self.f_mntfromname)
                    .finish()
            }
        }
        impl ::hash::Hash for statfs {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.f_bsize.hash(state);
                self.f_iosize.hash(state);
                self.f_blocks.hash(state);
                self.f_bfree.hash(state);
                self.f_bavail.hash(state);
                self.f_files.hash(state);
                self.f_ffree.hash(state);
                self.f_fsid.hash(state);
                self.f_owner.hash(state);
                self.f_type.hash(state);
                self.f_flags.hash(state);
                self.f_syncwrites.hash(state);
                self.f_asyncwrites.hash(state);
                self.f_fstypename.hash(state);
                self.f_mntonname.hash(state);
                self.f_syncreads.hash(state);
                self.f_asyncreads.hash(state);
                self.f_mntfromname.hash(state);
            }
        }

        impl PartialEq for sigevent {
            fn eq(&self, other: &sigevent) -> bool {
                self.sigev_notify == other.sigev_notify
                    && self.sigev_signo == other.sigev_signo
                    && self.sigev_value == other.sigev_value
            }
        }
        impl Eq for sigevent {}
        impl ::fmt::Debug for sigevent {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("sigevent")
                    .field("sigev_notify", &self.sigev_notify)
                    .field("sigev_signo", &self.sigev_signo)
                    .field("sigev_value", &self.sigev_value)
                    .finish()
            }
        }
        impl ::hash::Hash for sigevent {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.sigev_notify.hash(state);
                self.sigev_signo.hash(state);
                self.sigev_value.hash(state);
            }
        }
    }
}

pub const RAND_MAX: ::c_int = 0x7fff_ffff;
pub const PTHREAD_STACK_MIN: ::size_t = 16384;
pub const SIGSTKSZ: ::size_t = 40960;
pub const MADV_INVAL: ::c_int = 10;
pub const MADV_SETMAP: ::c_int = 11;
pub const O_CLOEXEC: ::c_int = 0x00020000;
pub const O_DIRECTORY: ::c_int = 0x08000000;
pub const F_GETLK: ::c_int = 7;
pub const F_SETLK: ::c_int = 8;
pub const F_SETLKW: ::c_int = 9;
pub const ENOMEDIUM: ::c_int = 93;
pub const EASYNC: ::c_int = 99;
pub const ELAST: ::c_int = 99;
pub const RLIMIT_POSIXLOCKS: ::c_int = 11;
#[deprecated(since = "0.2.64", note = "Not stable across OS versions")]
pub const RLIM_NLIMITS: ::rlim_t = 12;

pub const Q_GETQUOTA: ::c_int = 0x300;
pub const Q_SETQUOTA: ::c_int = 0x400;

pub const CLOCK_REALTIME: ::clockid_t = 0;
pub const CLOCK_VIRTUAL: ::clockid_t = 1;
pub const CLOCK_PROF: ::clockid_t = 2;
pub const CLOCK_MONOTONIC: ::clockid_t = 4;
pub const CLOCK_UPTIME: ::clockid_t = 5;
pub const CLOCK_UPTIME_PRECISE: ::clockid_t = 7;
pub const CLOCK_UPTIME_FAST: ::clockid_t = 8;
pub const CLOCK_REALTIME_PRECISE: ::clockid_t = 9;
pub const CLOCK_REALTIME_FAST: ::clockid_t = 10;
pub const CLOCK_MONOTONIC_PRECISE: ::clockid_t = 11;
pub const CLOCK_MONOTONIC_FAST: ::clockid_t = 12;
pub const CLOCK_SECOND: ::clockid_t = 13;
pub const CLOCK_THREAD_CPUTIME_ID: ::clockid_t = 14;
pub const CLOCK_PROCESS_CPUTIME_ID: ::clockid_t = 15;

pub const CTL_UNSPEC: ::c_int = 0;
pub const CTL_KERN: ::c_int = 1;
pub const CTL_VM: ::c_int = 2;
pub const CTL_VFS: ::c_int = 3;
pub const CTL_NET: ::c_int = 4;
pub const CTL_DEBUG: ::c_int = 5;
pub const CTL_HW: ::c_int = 6;
pub const CTL_MACHDEP: ::c_int = 7;
pub const CTL_USER: ::c_int = 8;
pub const CTL_P1003_1B: ::c_int = 9;
pub const CTL_LWKT: ::c_int = 10;
pub const CTL_MAXID: ::c_int = 11;
pub const KERN_OSTYPE: ::c_int = 1;
pub const KERN_OSRELEASE: ::c_int = 2;
pub const KERN_OSREV: ::c_int = 3;
pub const KERN_VERSION: ::c_int = 4;
pub const KERN_MAXVNODES: ::c_int = 5;
pub const KERN_MAXPROC: ::c_int = 6;
pub const KERN_MAXFILES: ::c_int = 7;
pub const KERN_ARGMAX: ::c_int = 8;
pub const KERN_SECURELVL: ::c_int = 9;
pub const KERN_HOSTNAME: ::c_int = 10;
pub const KERN_HOSTID: ::c_int = 11;
pub const KERN_CLOCKRATE: ::c_int = 12;
pub const KERN_VNODE: ::c_int = 13;
pub const KERN_PROC: ::c_int = 14;
pub const KERN_FILE: ::c_int = 15;
pub const KERN_PROF: ::c_int = 16;
pub const KERN_POSIX1: ::c_int = 17;
pub const KERN_NGROUPS: ::c_int = 18;
pub const KERN_JOB_CONTROL: ::c_int = 19;
pub const KERN_SAVED_IDS: ::c_int = 20;
pub const KERN_BOOTTIME: ::c_int = 21;
pub const KERN_NISDOMAINNAME: ::c_int = 22;
pub const KERN_UPDATEINTERVAL: ::c_int = 23;
pub const KERN_OSRELDATE: ::c_int = 24;
pub const KERN_NTP_PLL: ::c_int = 25;
pub const KERN_BOOTFILE: ::c_int = 26;
pub const KERN_MAXFILESPERPROC: ::c_int = 27;
pub const KERN_MAXPROCPERUID: ::c_int = 28;
pub const KERN_DUMPDEV: ::c_int = 29;
pub const KERN_IPC: ::c_int = 30;
pub const KERN_DUMMY: ::c_int = 31;
pub const KERN_PS_STRINGS: ::c_int = 32;
pub const KERN_USRSTACK: ::c_int = 33;
pub const KERN_LOGSIGEXIT: ::c_int = 34;
pub const KERN_IOV_MAX: ::c_int = 35;
pub const KERN_MAXPOSIXLOCKSPERUID: ::c_int = 36;
pub const KERN_MAXID: ::c_int = 37;
pub const KERN_PROC_ALL: ::c_int = 0;
pub const KERN_PROC_PID: ::c_int = 1;
pub const KERN_PROC_PGRP: ::c_int = 2;
pub const KERN_PROC_SESSION: ::c_int = 3;
pub const KERN_PROC_TTY: ::c_int = 4;
pub const KERN_PROC_UID: ::c_int = 5;
pub const KERN_PROC_RUID: ::c_int = 6;
pub const KERN_PROC_ARGS: ::c_int = 7;
pub const KERN_PROC_CWD: ::c_int = 8;
pub const KERN_PROC_PATHNAME: ::c_int = 9;
pub const KERN_PROC_FLAGMASK: ::c_int = 0x10;
pub const KERN_PROC_FLAG_LWP: ::c_int = 0x10;
pub const KIPC_MAXSOCKBUF: ::c_int = 1;
pub const KIPC_SOCKBUF_WASTE: ::c_int = 2;
pub const KIPC_SOMAXCONN: ::c_int = 3;
pub const KIPC_MAX_LINKHDR: ::c_int = 4;
pub const KIPC_MAX_PROTOHDR: ::c_int = 5;
pub const KIPC_MAX_HDR: ::c_int = 6;
pub const KIPC_MAX_DATALEN: ::c_int = 7;
pub const KIPC_MBSTAT: ::c_int = 8;
pub const KIPC_NMBCLUSTERS: ::c_int = 9;
pub const HW_MACHINE: ::c_int = 1;
pub const HW_MODEL: ::c_int = 2;
pub const HW_NCPU: ::c_int = 3;
pub const HW_BYTEORDER: ::c_int = 4;
pub const HW_PHYSMEM: ::c_int = 5;
pub const HW_USERMEM: ::c_int = 6;
pub const HW_PAGESIZE: ::c_int = 7;
pub const HW_DISKNAMES: ::c_int = 8;
pub const HW_DISKSTATS: ::c_int = 9;
pub const HW_FLOATINGPT: ::c_int = 10;
pub const HW_MACHINE_ARCH: ::c_int = 11;
pub const HW_MACHINE_PLATFORM: ::c_int = 12;
pub const HW_SENSORS: ::c_int = 13;
pub const HW_MAXID: ::c_int = 14;
pub const USER_CS_PATH: ::c_int = 1;
pub const USER_BC_BASE_MAX: ::c_int = 2;
pub const USER_BC_DIM_MAX: ::c_int = 3;
pub const USER_BC_SCALE_MAX: ::c_int = 4;
pub const USER_BC_STRING_MAX: ::c_int = 5;
pub const USER_COLL_WEIGHTS_MAX: ::c_int = 6;
pub const USER_EXPR_NEST_MAX: ::c_int = 7;
pub const USER_LINE_MAX: ::c_int = 8;
pub const USER_RE_DUP_MAX: ::c_int = 9;
pub const USER_POSIX2_VERSION: ::c_int = 10;
pub const USER_POSIX2_C_BIND: ::c_int = 11;
pub const USER_POSIX2_C_DEV: ::c_int = 12;
pub const USER_POSIX2_CHAR_TERM: ::c_int = 13;
pub const USER_POSIX2_FORT_DEV: ::c_int = 14;
pub const USER_POSIX2_FORT_RUN: ::c_int = 15;
pub const USER_POSIX2_LOCALEDEF: ::c_int = 16;
pub const USER_POSIX2_SW_DEV: ::c_int = 17;
pub const USER_POSIX2_UPE: ::c_int = 18;
pub const USER_STREAM_MAX: ::c_int = 19;
pub const USER_TZNAME_MAX: ::c_int = 20;
pub const USER_MAXID: ::c_int = 21;
pub const CTL_P1003_1B_ASYNCHRONOUS_IO: ::c_int = 1;
pub const CTL_P1003_1B_MAPPED_FILES: ::c_int = 2;
pub const CTL_P1003_1B_MEMLOCK: ::c_int = 3;
pub const CTL_P1003_1B_MEMLOCK_RANGE: ::c_int = 4;
pub const CTL_P1003_1B_MEMORY_PROTECTION: ::c_int = 5;
pub const CTL_P1003_1B_MESSAGE_PASSING: ::c_int = 6;
pub const CTL_P1003_1B_PRIORITIZED_IO: ::c_int = 7;
pub const CTL_P1003_1B_PRIORITY_SCHEDULING: ::c_int = 8;
pub const CTL_P1003_1B_REALTIME_SIGNALS: ::c_int = 9;
pub const CTL_P1003_1B_SEMAPHORES: ::c_int = 10;
pub const CTL_P1003_1B_FSYNC: ::c_int = 11;
pub const CTL_P1003_1B_SHARED_MEMORY_OBJECTS: ::c_int = 12;
pub const CTL_P1003_1B_SYNCHRONIZED_IO: ::c_int = 13;
pub const CTL_P1003_1B_TIMERS: ::c_int = 14;
pub const CTL_P1003_1B_AIO_LISTIO_MAX: ::c_int = 15;
pub const CTL_P1003_1B_AIO_MAX: ::c_int = 16;
pub const CTL_P1003_1B_AIO_PRIO_DELTA_MAX: ::c_int = 17;
pub const CTL_P1003_1B_DELAYTIMER_MAX: ::c_int = 18;
pub const CTL_P1003_1B_UNUSED1: ::c_int = 19;
pub const CTL_P1003_1B_PAGESIZE: ::c_int = 20;
pub const CTL_P1003_1B_RTSIG_MAX: ::c_int = 21;
pub const CTL_P1003_1B_SEM_NSEMS_MAX: ::c_int = 22;
pub const CTL_P1003_1B_SEM_VALUE_MAX: ::c_int = 23;
pub const CTL_P1003_1B_SIGQUEUE_MAX: ::c_int = 24;
pub const CTL_P1003_1B_TIMER_MAX: ::c_int = 25;
pub const CTL_P1003_1B_MAXID: ::c_int = 26;

pub const EVFILT_READ: i16 = -1;
pub const EVFILT_WRITE: i16 = -2;
pub const EVFILT_AIO: i16 = -3;
pub const EVFILT_VNODE: i16 = -4;
pub const EVFILT_PROC: i16 = -5;
pub const EVFILT_SIGNAL: i16 = -6;
pub const EVFILT_TIMER: i16 = -7;
pub const EVFILT_EXCEPT: i16 = -8;
pub const EVFILT_USER: i16 = -9;
pub const EVFILT_FS: i16 = -10;

pub const EV_ADD: u16 = 0x1;
pub const EV_DELETE: u16 = 0x2;
pub const EV_ENABLE: u16 = 0x4;
pub const EV_DISABLE: u16 = 0x8;
pub const EV_ONESHOT: u16 = 0x10;
pub const EV_CLEAR: u16 = 0x20;
pub const EV_RECEIPT: u16 = 0x40;
pub const EV_DISPATCH: u16 = 0x80;
pub const EV_NODATA: u16 = 0x1000;
pub const EV_FLAG1: u16 = 0x2000;
pub const EV_ERROR: u16 = 0x4000;
pub const EV_EOF: u16 = 0x8000;
pub const EV_SYSFLAGS: u16 = 0xf000;

pub const NOTE_TRIGGER: u32 = 0x01000000;
pub const NOTE_FFNOP: u32 = 0x00000000;
pub const NOTE_FFAND: u32 = 0x40000000;
pub const NOTE_FFOR: u32 = 0x80000000;
pub const NOTE_FFCOPY: u32 = 0xc0000000;
pub const NOTE_FFCTRLMASK: u32 = 0xc0000000;
pub const NOTE_FFLAGSMASK: u32 = 0x00ffffff;
pub const NOTE_LOWAT: u32 = 0x00000001;
pub const NOTE_OOB: u32 = 0x00000002;
pub const NOTE_DELETE: u32 = 0x00000001;
pub const NOTE_WRITE: u32 = 0x00000002;
pub const NOTE_EXTEND: u32 = 0x00000004;
pub const NOTE_ATTRIB: u32 = 0x00000008;
pub const NOTE_LINK: u32 = 0x00000010;
pub const NOTE_RENAME: u32 = 0x00000020;
pub const NOTE_REVOKE: u32 = 0x00000040;
pub const NOTE_EXIT: u32 = 0x80000000;
pub const NOTE_FORK: u32 = 0x40000000;
pub const NOTE_EXEC: u32 = 0x20000000;
pub const NOTE_PDATAMASK: u32 = 0x000fffff;
pub const NOTE_PCTRLMASK: u32 = 0xf0000000;
pub const NOTE_TRACK: u32 = 0x00000001;
pub const NOTE_TRACKERR: u32 = 0x00000002;
pub const NOTE_CHILD: u32 = 0x00000004;

pub const SO_SNDSPACE: ::c_int = 0x100a;
pub const SO_CPUHINT: ::c_int = 0x1030;

pub const PT_FIRSTMACH: ::c_int = 32;

// https://github.com/DragonFlyBSD/DragonFlyBSD/blob/master/sys/net/if.h#L101
pub const IFF_UP: ::c_int = 0x1; // interface is up
pub const IFF_BROADCAST: ::c_int = 0x2; // broadcast address valid
pub const IFF_DEBUG: ::c_int = 0x4; // turn on debugging
pub const IFF_LOOPBACK: ::c_int = 0x8; // is a loopback net
pub const IFF_POINTOPOINT: ::c_int = 0x10; // interface is point-to-point link
pub const IFF_SMART: ::c_int = 0x20; // interface manages own routes
pub const IFF_RUNNING: ::c_int = 0x40; // resources allocated
pub const IFF_NOARP: ::c_int = 0x80; // no address resolution protocol
pub const IFF_PROMISC: ::c_int = 0x100; // receive all packets
pub const IFF_ALLMULTI: ::c_int = 0x200; // receive all multicast packets
pub const IFF_OACTIVE_COMPAT: ::c_int = 0x400; // was transmission in progress
pub const IFF_SIMPLEX: ::c_int = 0x800; // can't hear own transmissions
pub const IFF_LINK0: ::c_int = 0x1000; // per link layer defined bit
pub const IFF_LINK1: ::c_int = 0x2000; // per link layer defined bit
pub const IFF_LINK2: ::c_int = 0x4000; // per link layer defined bit
pub const IFF_ALTPHYS: ::c_int = IFF_LINK2; // use alternate physical connection
pub const IFF_MULTICAST: ::c_int = 0x8000; // supports multicast
                                           // was interface is in polling mode
pub const IFF_POLLING_COMPAT: ::c_int = 0x10000;
pub const IFF_PPROMISC: ::c_int = 0x20000; // user-requested promisc mode
pub const IFF_MONITOR: ::c_int = 0x40000; // user-requested monitor mode
pub const IFF_STATICARP: ::c_int = 0x80000; // static ARP
pub const IFF_NPOLLING: ::c_int = 0x100000; // interface is in polling mode
pub const IFF_IDIRECT: ::c_int = 0x200000; // direct input

//
// sys/netinet/in.h
// Protocols (RFC 1700)
// NOTE: These are in addition to the constants defined in src/unix/mod.rs

// IPPROTO_IP defined in src/unix/mod.rs
/// IP6 hop-by-hop options
pub const IPPROTO_HOPOPTS: ::c_int = 0;
// IPPROTO_ICMP defined in src/unix/mod.rs
/// group mgmt protocol
pub const IPPROTO_IGMP: ::c_int = 2;
/// gateway^2 (deprecated)
pub const IPPROTO_GGP: ::c_int = 3;
/// for compatibility
pub const IPPROTO_IPIP: ::c_int = 4;
// IPPROTO_TCP defined in src/unix/mod.rs
/// Stream protocol II.
pub const IPPROTO_ST: ::c_int = 7;
/// exterior gateway protocol
pub const IPPROTO_EGP: ::c_int = 8;
/// private interior gateway
pub const IPPROTO_PIGP: ::c_int = 9;
/// BBN RCC Monitoring
pub const IPPROTO_RCCMON: ::c_int = 10;
/// network voice protocol
pub const IPPROTO_NVPII: ::c_int = 11;
/// pup
pub const IPPROTO_PUP: ::c_int = 12;
/// Argus
pub const IPPROTO_ARGUS: ::c_int = 13;
/// EMCON
pub const IPPROTO_EMCON: ::c_int = 14;
/// Cross Net Debugger
pub const IPPROTO_XNET: ::c_int = 15;
/// Chaos
pub const IPPROTO_CHAOS: ::c_int = 16;
// IPPROTO_UDP defined in src/unix/mod.rs
/// Multiplexing
pub const IPPROTO_MUX: ::c_int = 18;
/// DCN Measurement Subsystems
pub const IPPROTO_MEAS: ::c_int = 19;
/// Host Monitoring
pub const IPPROTO_HMP: ::c_int = 20;
/// Packet Radio Measurement
pub const IPPROTO_PRM: ::c_int = 21;
/// xns idp
pub const IPPROTO_IDP: ::c_int = 22;
/// Trunk-1
pub const IPPROTO_TRUNK1: ::c_int = 23;
/// Trunk-2
pub const IPPROTO_TRUNK2: ::c_int = 24;
/// Leaf-1
pub const IPPROTO_LEAF1: ::c_int = 25;
/// Leaf-2
pub const IPPROTO_LEAF2: ::c_int = 26;
/// Reliable Data
pub const IPPROTO_RDP: ::c_int = 27;
/// Reliable Transaction
pub const IPPROTO_IRTP: ::c_int = 28;
/// tp-4 w/ class negotiation
pub const IPPROTO_TP: ::c_int = 29;
/// Bulk Data Transfer
pub const IPPROTO_BLT: ::c_int = 30;
/// Network Services
pub const IPPROTO_NSP: ::c_int = 31;
/// Merit Internodal
pub const IPPROTO_INP: ::c_int = 32;
/// Sequential Exchange
pub const IPPROTO_SEP: ::c_int = 33;
/// Third Party Connect
pub const IPPROTO_3PC: ::c_int = 34;
/// InterDomain Policy Routing
pub const IPPROTO_IDPR: ::c_int = 35;
/// XTP
pub const IPPROTO_XTP: ::c_int = 36;
/// Datagram Delivery
pub const IPPROTO_DDP: ::c_int = 37;
/// Control Message Transport
pub const IPPROTO_CMTP: ::c_int = 38;
/// TP++ Transport
pub const IPPROTO_TPXX: ::c_int = 39;
/// IL transport protocol
pub const IPPROTO_IL: ::c_int = 40;
// IPPROTO_IPV6 defined in src/unix/mod.rs
/// Source Demand Routing
pub const IPPROTO_SDRP: ::c_int = 42;
/// IP6 routing header
pub const IPPROTO_ROUTING: ::c_int = 43;
/// IP6 fragmentation header
pub const IPPROTO_FRAGMENT: ::c_int = 44;
/// InterDomain Routing
pub const IPPROTO_IDRP: ::c_int = 45;
/// resource reservation
pub const IPPROTO_RSVP: ::c_int = 46;
/// General Routing Encap.
pub const IPPROTO_GRE: ::c_int = 47;
/// Mobile Host Routing
pub const IPPROTO_MHRP: ::c_int = 48;
/// BHA
pub const IPPROTO_BHA: ::c_int = 49;
/// IP6 Encap Sec. Payload
pub const IPPROTO_ESP: ::c_int = 50;
/// IP6 Auth Header
pub const IPPROTO_AH: ::c_int = 51;
/// Integ. Net Layer Security
pub const IPPROTO_INLSP: ::c_int = 52;
/// IP with encryption
pub const IPPROTO_SWIPE: ::c_int = 53;
/// Next Hop Resolution
pub const IPPROTO_NHRP: ::c_int = 54;
/// IP Mobility
pub const IPPROTO_MOBILE: ::c_int = 55;
/// Transport Layer Security
pub const IPPROTO_TLSP: ::c_int = 56;
/// SKIP
pub const IPPROTO_SKIP: ::c_int = 57;
// IPPROTO_ICMPV6 defined in src/unix/mod.rs
/// IP6 no next header
pub const IPPROTO_NONE: ::c_int = 59;
/// IP6 destination option
pub const IPPROTO_DSTOPTS: ::c_int = 60;
/// any host internal protocol
pub const IPPROTO_AHIP: ::c_int = 61;
/// CFTP
pub const IPPROTO_CFTP: ::c_int = 62;
/// "hello" routing protocol
pub const IPPROTO_HELLO: ::c_int = 63;
/// SATNET/Backroom EXPAK
pub const IPPROTO_SATEXPAK: ::c_int = 64;
/// Kryptolan
pub const IPPROTO_KRYPTOLAN: ::c_int = 65;
/// Remote Virtual Disk
pub const IPPROTO_RVD: ::c_int = 66;
/// Pluribus Packet Core
pub const IPPROTO_IPPC: ::c_int = 67;
/// Any distributed FS
pub const IPPROTO_ADFS: ::c_int = 68;
/// Satnet Monitoring
pub const IPPROTO_SATMON: ::c_int = 69;
/// VISA Protocol
pub const IPPROTO_VISA: ::c_int = 70;
/// Packet Core Utility
pub const IPPROTO_IPCV: ::c_int = 71;
/// Comp. Prot. Net. Executive
pub const IPPROTO_CPNX: ::c_int = 72;
/// Comp. Prot. HeartBeat
pub const IPPROTO_CPHB: ::c_int = 73;
/// Wang Span Network
pub const IPPROTO_WSN: ::c_int = 74;
/// Packet Video Protocol
pub const IPPROTO_PVP: ::c_int = 75;
/// BackRoom SATNET Monitoring
pub const IPPROTO_BRSATMON: ::c_int = 76;
/// Sun net disk proto (temp.)
pub const IPPROTO_ND: ::c_int = 77;
/// WIDEBAND Monitoring
pub const IPPROTO_WBMON: ::c_int = 78;
/// WIDEBAND EXPAK
pub const IPPROTO_WBEXPAK: ::c_int = 79;
/// ISO cnlp
pub const IPPROTO_EON: ::c_int = 80;
/// VMTP
pub const IPPROTO_VMTP: ::c_int = 81;
/// Secure VMTP
pub const IPPROTO_SVMTP: ::c_int = 82;
/// Banyon VINES
pub const IPPROTO_VINES: ::c_int = 83;
/// TTP
pub const IPPROTO_TTP: ::c_int = 84;
/// NSFNET-IGP
pub const IPPROTO_IGP: ::c_int = 85;
/// dissimilar gateway prot.
pub const IPPROTO_DGP: ::c_int = 86;
/// TCF
pub const IPPROTO_TCF: ::c_int = 87;
/// Cisco/GXS IGRP
pub const IPPROTO_IGRP: ::c_int = 88;
/// OSPFIGP
pub const IPPROTO_OSPFIGP: ::c_int = 89;
/// Strite RPC protocol
pub const IPPROTO_SRPC: ::c_int = 90;
/// Locus Address Resoloution
pub const IPPROTO_LARP: ::c_int = 91;
/// Multicast Transport
pub const IPPROTO_MTP: ::c_int = 92;
/// AX.25 Frames
pub const IPPROTO_AX25: ::c_int = 93;
/// IP encapsulated in IP
pub const IPPROTO_IPEIP: ::c_int = 94;
/// Mobile Int.ing control
pub const IPPROTO_MICP: ::c_int = 95;
/// Semaphore Comm. security
pub const IPPROTO_SCCSP: ::c_int = 96;
/// Ethernet IP encapsulation
pub const IPPROTO_ETHERIP: ::c_int = 97;
/// encapsulation header
pub const IPPROTO_ENCAP: ::c_int = 98;
/// any private encr. scheme
pub const IPPROTO_APES: ::c_int = 99;
/// GMTP
pub const IPPROTO_GMTP: ::c_int = 100;
/// payload compression (IPComp)
pub const IPPROTO_IPCOMP: ::c_int = 108;

/* 101-254: Partly Unassigned */
/// Protocol Independent Mcast
pub const IPPROTO_PIM: ::c_int = 103;
/// CARP
pub const IPPROTO_CARP: ::c_int = 112;
/// PGM
pub const IPPROTO_PGM: ::c_int = 113;
/// PFSYNC
pub const IPPROTO_PFSYNC: ::c_int = 240;

/* 255: Reserved */
/* BSD Private, local use, namespace incursion, no longer used */
/// divert pseudo-protocol
pub const IPPROTO_DIVERT: ::c_int = 254;
pub const IPPROTO_MAX: ::c_int = 256;
/// last return value of *_input(), meaning "all job for this pkt is done".
pub const IPPROTO_DONE: ::c_int = 257;

/// Used by RSS: the layer3 protocol is unknown
pub const IPPROTO_UNKNOWN: ::c_int = 258;

// sys/netinet/tcp.h
pub const TCP_SIGNATURE_ENABLE: ::c_int = 16;
pub const TCP_KEEPINIT: ::c_int = 32;
pub const TCP_FASTKEEP: ::c_int = 128;

pub const AF_BLUETOOTH: ::c_int = 33;
pub const AF_MPLS: ::c_int = 34;
pub const AF_IEEE80211: ::c_int = 35;

pub const PF_BLUETOOTH: ::c_int = AF_BLUETOOTH;

pub const NET_RT_DUMP: ::c_int = 1;
pub const NET_RT_FLAGS: ::c_int = 2;
pub const NET_RT_IFLIST: ::c_int = 3;
pub const NET_RT_MAXID: ::c_int = 4;

pub const SOMAXOPT_SIZE: ::c_int = 65536;

pub const MSG_UNUSED09: ::c_int = 0x00000200;
pub const MSG_NOSIGNAL: ::c_int = 0x00000400;
pub const MSG_SYNC: ::c_int = 0x00000800;
pub const MSG_CMSG_CLOEXEC: ::c_int = 0x00001000;
pub const MSG_FBLOCKING: ::c_int = 0x00010000;
pub const MSG_FNONBLOCKING: ::c_int = 0x00020000;
pub const MSG_FMASK: ::c_int = 0xFFFF0000;

// utmpx entry types
pub const EMPTY: ::c_short = 0;
pub const RUN_LVL: ::c_short = 1;
pub const BOOT_TIME: ::c_short = 2;
pub const OLD_TIME: ::c_short = 3;
pub const NEW_TIME: ::c_short = 4;
pub const INIT_PROCESS: ::c_short = 5;
pub const LOGIN_PROCESS: ::c_short = 6;
pub const USER_PROCESS: ::c_short = 7;
pub const DEAD_PROCESS: ::c_short = 8;
pub const ACCOUNTING: ::c_short = 9;
pub const SIGNATURE: ::c_short = 10;
pub const DOWNTIME: ::c_short = 11;
// utmpx database types
pub const UTX_DB_UTMPX: ::c_uint = 0;
pub const UTX_DB_WTMPX: ::c_uint = 1;
pub const UTX_DB_LASTLOG: ::c_uint = 2;

pub const LC_COLLATE_MASK: ::c_int = (1 << 0);
pub const LC_CTYPE_MASK: ::c_int = (1 << 1);
pub const LC_MONETARY_MASK: ::c_int = (1 << 2);
pub const LC_NUMERIC_MASK: ::c_int = (1 << 3);
pub const LC_TIME_MASK: ::c_int = (1 << 4);
pub const LC_MESSAGES_MASK: ::c_int = (1 << 5);
pub const LC_ALL_MASK: ::c_int = LC_COLLATE_MASK
    | LC_CTYPE_MASK
    | LC_MESSAGES_MASK
    | LC_MONETARY_MASK
    | LC_NUMERIC_MASK
    | LC_TIME_MASK;

pub const TIOCSIG: ::c_uint = 0x2000745f;
pub const BTUARTDISC: ::c_int = 0x7;
pub const TIOCDCDTIMESTAMP: ::c_uint = 0x40107458;
pub const TIOCISPTMASTER: ::c_uint = 0x20007455;
pub const TIOCMODG: ::c_uint = 0x40047403;
pub const TIOCMODS: ::c_ulong = 0x80047404;
pub const TIOCREMOTE: ::c_ulong = 0x80047469;

// Constants used by "at" family of system calls.
pub const AT_FDCWD: ::c_int = 0xFFFAFDCD; // invalid file descriptor
pub const AT_SYMLINK_NOFOLLOW: ::c_int = 1;
pub const AT_REMOVEDIR: ::c_int = 2;
pub const AT_EACCESS: ::c_int = 4;
pub const AT_SYMLINK_FOLLOW: ::c_int = 8;

pub const VCHECKPT: usize = 19;

pub const _PC_2_SYMLINKS: ::c_int = 22;
pub const _PC_TIMESTAMP_RESOLUTION: ::c_int = 23;

pub const _SC_V7_ILP32_OFF32: ::c_int = 122;
pub const _SC_V7_ILP32_OFFBIG: ::c_int = 123;
pub const _SC_V7_LP64_OFF64: ::c_int = 124;
pub const _SC_V7_LPBIG_OFFBIG: ::c_int = 125;
pub const _SC_THREAD_ROBUST_PRIO_INHERIT: ::c_int = 126;
pub const _SC_THREAD_ROBUST_PRIO_PROTECT: ::c_int = 127;

pub const WCONTINUED: ::c_int = 4;
pub const WSTOPPED: ::c_int = 0o177;

// Values for struct rtprio (type_ field)
pub const RTP_PRIO_REALTIME: ::c_ushort = 0;
pub const RTP_PRIO_NORMAL: ::c_ushort = 1;
pub const RTP_PRIO_IDLE: ::c_ushort = 2;
pub const RTP_PRIO_THREAD: ::c_ushort = 3;

// Flags for chflags(2)
pub const UF_NOHISTORY: ::c_ulong = 0x00000040;
pub const UF_CACHE: ::c_ulong = 0x00000080;
pub const UF_XLINK: ::c_ulong = 0x00000100;
pub const SF_NOHISTORY: ::c_ulong = 0x00400000;
pub const SF_CACHE: ::c_ulong = 0x00800000;
pub const SF_XLINK: ::c_ulong = 0x01000000;

// timespec constants
pub const UTIME_OMIT: c_long = -2;
pub const UTIME_NOW: c_long = -1;

fn _CMSG_ALIGN(n: usize) -> usize {
    (n + 3) & !3
}

f! {
    pub fn CMSG_DATA(cmsg: *const ::cmsghdr) -> *mut ::c_uchar {
        (cmsg as *mut ::c_uchar)
            .offset(_CMSG_ALIGN(::mem::size_of::<::cmsghdr>()) as isize)
    }

    pub fn CMSG_LEN(length: ::c_uint) -> ::c_uint {
        (_CMSG_ALIGN(::mem::size_of::<::cmsghdr>()) + length as usize)
            as ::c_uint
    }

    pub fn CMSG_NXTHDR(mhdr: *const ::msghdr, cmsg: *const ::cmsghdr)
        -> *mut ::cmsghdr
    {
        let next = cmsg as usize + _CMSG_ALIGN((*cmsg).cmsg_len as usize)
            + _CMSG_ALIGN(::mem::size_of::<::cmsghdr>());
        let max = (*mhdr).msg_control as usize
            + (*mhdr).msg_controllen as usize;
        if next <= max {
            (cmsg as usize + _CMSG_ALIGN((*cmsg).cmsg_len as usize))
                as *mut ::cmsghdr
        } else {
            0 as *mut ::cmsghdr
        }
    }

    pub fn CMSG_SPACE(length: ::c_uint) -> ::c_uint {
        (_CMSG_ALIGN(::mem::size_of::<::cmsghdr>()) +
            _CMSG_ALIGN(length as usize)) as ::c_uint
    }
}

extern "C" {
    pub fn setgrent();
    pub fn mprotect(
        addr: *mut ::c_void,
        len: ::size_t,
        prot: ::c_int,
    ) -> ::c_int;
    pub fn clock_getres(clk_id: ::clockid_t, tp: *mut ::timespec) -> ::c_int;
    pub fn clock_gettime(clk_id: ::clockid_t, tp: *mut ::timespec) -> ::c_int;
    pub fn clock_settime(
        clk_id: ::clockid_t,
        tp: *const ::timespec,
    ) -> ::c_int;

    pub fn setutxdb(_type: ::c_uint, file: *mut ::c_char) -> ::c_int;

    pub fn aio_waitcomplete(
        iocbp: *mut *mut aiocb,
        timeout: *mut ::timespec,
    ) -> ::c_int;

    pub fn freelocale(loc: ::locale_t);

    pub fn lwp_rtprio(
        function: ::c_int,
        pid: ::pid_t,
        lwpid: lwpid_t,
        rtp: *mut super::rtprio,
    ) -> ::c_int;

    pub fn statfs(path: *const ::c_char, buf: *mut statfs) -> ::c_int;
    pub fn fstatfs(fd: ::c_int, buf: *mut statfs) -> ::c_int;
    pub fn uname(buf: *mut ::utsname) -> ::c_int;
}

cfg_if! {
    if #[cfg(libc_thread_local)] {
        mod errno;
        pub use self::errno::*;
    }
}
