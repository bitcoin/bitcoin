pub type fflags_t = u32;
pub type clock_t = i32;

pub type lwpid_t = i32;
pub type blksize_t = i32;
pub type clockid_t = ::c_int;
pub type sem_t = _sem;

pub type fsblkcnt_t = u64;
pub type fsfilcnt_t = u64;
pub type idtype_t = ::c_uint;

pub type key_t = ::c_long;
pub type msglen_t = ::c_ulong;
pub type msgqnum_t = ::c_ulong;

pub type mqd_t = *mut ::c_void;
pub type posix_spawnattr_t = *mut ::c_void;
pub type posix_spawn_file_actions_t = *mut ::c_void;

s! {
    pub struct aiocb {
        pub aio_fildes: ::c_int,
        pub aio_offset: ::off_t,
        pub aio_buf: *mut ::c_void,
        pub aio_nbytes: ::size_t,
        __unused1: [::c_int; 2],
        __unused2: *mut ::c_void,
        pub aio_lio_opcode: ::c_int,
        pub aio_reqprio: ::c_int,
        // unused 3 through 5 are the __aiocb_private structure
        __unused3: ::c_long,
        __unused4: ::c_long,
        __unused5: *mut ::c_void,
        pub aio_sigevent: sigevent
    }

    pub struct jail {
        pub version: u32,
        pub path: *mut ::c_char,
        pub hostname: *mut ::c_char,
        pub jailname: *mut ::c_char,
        pub ip4s: ::c_uint,
        pub ip6s: ::c_uint,
        pub ip4: *mut ::in_addr,
        pub ip6: *mut ::in6_addr,
    }

    pub struct statvfs {
        pub f_bavail: ::fsblkcnt_t,
        pub f_bfree: ::fsblkcnt_t,
        pub f_blocks: ::fsblkcnt_t,
        pub f_favail: ::fsfilcnt_t,
        pub f_ffree: ::fsfilcnt_t,
        pub f_files: ::fsfilcnt_t,
        pub f_bsize: ::c_ulong,
        pub f_flag: ::c_ulong,
        pub f_frsize: ::c_ulong,
        pub f_fsid: ::c_ulong,
        pub f_namemax: ::c_ulong,
    }

    // internal structure has changed over time
    pub struct _sem {
        data: [u32; 4],
    }

    pub struct ipc_perm {
        pub cuid: ::uid_t,
        pub cgid: ::gid_t,
        pub uid: ::uid_t,
        pub gid: ::gid_t,
        pub mode: ::mode_t,
        pub seq: ::c_ushort,
        pub key: ::key_t,
    }

    pub struct msqid_ds {
        pub msg_perm: ::ipc_perm,
        __unused1: *mut ::c_void,
        __unused2: *mut ::c_void,
        pub msg_cbytes: ::msglen_t,
        pub msg_qnum: ::msgqnum_t,
        pub msg_qbytes: ::msglen_t,
        pub msg_lspid: ::pid_t,
        pub msg_lrpid: ::pid_t,
        pub msg_stime: ::time_t,
        pub msg_rtime: ::time_t,
        pub msg_ctime: ::time_t,
    }

    pub struct xucred {
        pub cr_version: ::c_uint,
        pub cr_uid: ::uid_t,
        pub cr_ngroups: ::c_short,
        pub cr_groups: [::gid_t;16],
        __cr_unused1: *mut ::c_void,
    }

    pub struct stack_t {
        pub ss_sp: *mut ::c_void,
        pub ss_size: ::size_t,
        pub ss_flags: ::c_int,
    }

    pub struct mmsghdr {
        pub msg_hdr: ::msghdr,
        pub msg_len: ::ssize_t,
    }
}

s_no_extra_traits! {
    pub struct utmpx {
        pub ut_type: ::c_short,
        pub ut_tv: ::timeval,
        pub ut_id: [::c_char; 8],
        pub ut_pid: ::pid_t,
        pub ut_user: [::c_char; 32],
        pub ut_line: [::c_char; 16],
        pub ut_host: [::c_char; 128],
        pub __ut_spare: [::c_char; 64],
    }

    pub struct sockaddr_dl {
        pub sdl_len: ::c_uchar,
        pub sdl_family: ::c_uchar,
        pub sdl_index: ::c_ushort,
        pub sdl_type: ::c_uchar,
        pub sdl_nlen: ::c_uchar,
        pub sdl_alen: ::c_uchar,
        pub sdl_slen: ::c_uchar,
        pub sdl_data: [::c_char; 46],
    }

    pub struct mq_attr {
        pub mq_flags: ::c_long,
        pub mq_maxmsg: ::c_long,
        pub mq_msgsize: ::c_long,
        pub mq_curmsgs: ::c_long,
        __reserved: [::c_long; 4]
    }

    pub struct sigevent {
        pub sigev_notify: ::c_int,
        pub sigev_signo: ::c_int,
        pub sigev_value: ::sigval,
        //The rest of the structure is actually a union.  We expose only
        //sigev_notify_thread_id because it's the most useful union member.
        pub sigev_notify_thread_id: ::lwpid_t,
        #[cfg(target_pointer_width = "64")]
        __unused1: ::c_int,
        __unused2: [::c_long; 7]
    }
}

cfg_if! {
    if #[cfg(feature = "extra_traits")] {
        impl PartialEq for utmpx {
            fn eq(&self, other: &utmpx) -> bool {
                self.ut_type == other.ut_type
                    && self.ut_tv == other.ut_tv
                    && self.ut_id == other.ut_id
                    && self.ut_pid == other.ut_pid
                    && self.ut_user == other.ut_user
                    && self.ut_line == other.ut_line
                    && self
                    .ut_host
                    .iter()
                    .zip(other.ut_host.iter())
                    .all(|(a,b)| a == b)
                    && self
                    .__ut_spare
                    .iter()
                    .zip(other.__ut_spare.iter())
                    .all(|(a,b)| a == b)
            }
        }
        impl Eq for utmpx {}
        impl ::fmt::Debug for utmpx {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("utmpx")
                    .field("ut_type", &self.ut_type)
                    .field("ut_tv", &self.ut_tv)
                    .field("ut_id", &self.ut_id)
                    .field("ut_pid", &self.ut_pid)
                    .field("ut_user", &self.ut_user)
                    .field("ut_line", &self.ut_line)
                    // FIXME: .field("ut_host", &self.ut_host)
                    // FIXME: .field("__ut_spare", &self.__ut_spare)
                    .finish()
            }
        }
        impl ::hash::Hash for utmpx {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.ut_type.hash(state);
                self.ut_tv.hash(state);
                self.ut_id.hash(state);
                self.ut_pid.hash(state);
                self.ut_user.hash(state);
                self.ut_line.hash(state);
                self.ut_host.hash(state);
                self.__ut_spare.hash(state);
            }
        }

        impl PartialEq for sockaddr_dl {
            fn eq(&self, other: &sockaddr_dl) -> bool {
                self.sdl_len == other.sdl_len
                    && self.sdl_family == other.sdl_family
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
                    .field("sdl_len", &self.sdl_len)
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
                self.sdl_len.hash(state);
                self.sdl_family.hash(state);
                self.sdl_index.hash(state);
                self.sdl_type.hash(state);
                self.sdl_nlen.hash(state);
                self.sdl_alen.hash(state);
                self.sdl_slen.hash(state);
                self.sdl_data.hash(state);
            }
        }

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

        impl PartialEq for sigevent {
            fn eq(&self, other: &sigevent) -> bool {
                self.sigev_notify == other.sigev_notify
                    && self.sigev_signo == other.sigev_signo
                    && self.sigev_value == other.sigev_value
                    && self.sigev_notify_thread_id
                        == other.sigev_notify_thread_id
            }
        }
        impl Eq for sigevent {}
        impl ::fmt::Debug for sigevent {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("sigevent")
                    .field("sigev_notify", &self.sigev_notify)
                    .field("sigev_signo", &self.sigev_signo)
                    .field("sigev_value", &self.sigev_value)
                    .field("sigev_notify_thread_id",
                           &self.sigev_notify_thread_id)
                    .finish()
            }
        }
        impl ::hash::Hash for sigevent {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.sigev_notify.hash(state);
                self.sigev_signo.hash(state);
                self.sigev_value.hash(state);
                self.sigev_notify_thread_id.hash(state);
            }
        }
    }
}

pub const SIGEV_THREAD_ID: ::c_int = 4;

pub const EXTATTR_NAMESPACE_EMPTY: ::c_int = 0;
pub const EXTATTR_NAMESPACE_USER: ::c_int = 1;
pub const EXTATTR_NAMESPACE_SYSTEM: ::c_int = 2;

pub const RAND_MAX: ::c_int = 0x7fff_fffd;
pub const PTHREAD_STACK_MIN: ::size_t = 2048;
pub const PTHREAD_MUTEX_ADAPTIVE_NP: ::c_int = 4;
pub const SIGSTKSZ: ::size_t = 34816;
pub const SF_NODISKIO: ::c_int = 0x00000001;
pub const SF_MNOWAIT: ::c_int = 0x00000002;
pub const SF_SYNC: ::c_int = 0x00000004;
pub const SF_USER_READAHEAD: ::c_int = 0x00000008;
pub const SF_NOCACHE: ::c_int = 0x00000010;
pub const O_CLOEXEC: ::c_int = 0x00100000;
pub const O_DIRECTORY: ::c_int = 0x00020000;
pub const O_EXEC: ::c_int = 0x00040000;
pub const O_TTY_INIT: ::c_int = 0x00080000;
pub const F_GETLK: ::c_int = 11;
pub const F_SETLK: ::c_int = 12;
pub const F_SETLKW: ::c_int = 13;
pub const ENOTCAPABLE: ::c_int = 93;
pub const ECAPMODE: ::c_int = 94;
pub const ENOTRECOVERABLE: ::c_int = 95;
pub const EOWNERDEAD: ::c_int = 96;
pub const RLIMIT_NPTS: ::c_int = 11;
pub const RLIMIT_SWAP: ::c_int = 12;
pub const RLIMIT_KQUEUES: ::c_int = 13;
pub const RLIMIT_UMTXP: ::c_int = 14;
#[deprecated(since = "0.2.64", note = "Not stable across OS versions")]
pub const RLIM_NLIMITS: ::rlim_t = 15;

pub const Q_GETQUOTA: ::c_int = 0x700;
pub const Q_SETQUOTA: ::c_int = 0x800;

pub const POSIX_FADV_NORMAL: ::c_int = 0;
pub const POSIX_FADV_RANDOM: ::c_int = 1;
pub const POSIX_FADV_SEQUENTIAL: ::c_int = 2;
pub const POSIX_FADV_WILLNEED: ::c_int = 3;
pub const POSIX_FADV_DONTNEED: ::c_int = 4;
pub const POSIX_FADV_NOREUSE: ::c_int = 5;

pub const POLLINIGNEOF: ::c_short = 0x2000;

pub const EVFILT_READ: i16 = -1;
pub const EVFILT_WRITE: i16 = -2;
pub const EVFILT_AIO: i16 = -3;
pub const EVFILT_VNODE: i16 = -4;
pub const EVFILT_PROC: i16 = -5;
pub const EVFILT_SIGNAL: i16 = -6;
pub const EVFILT_TIMER: i16 = -7;
pub const EVFILT_PROCDESC: i16 = -8;
pub const EVFILT_FS: i16 = -9;
pub const EVFILT_LIO: i16 = -10;
pub const EVFILT_USER: i16 = -11;
pub const EVFILT_SENDFILE: i16 = -12;
pub const EVFILT_EMPTY: i16 = -13;

pub const EV_ADD: u16 = 0x1;
pub const EV_DELETE: u16 = 0x2;
pub const EV_ENABLE: u16 = 0x4;
pub const EV_DISABLE: u16 = 0x8;
pub const EV_ONESHOT: u16 = 0x10;
pub const EV_CLEAR: u16 = 0x20;
pub const EV_RECEIPT: u16 = 0x40;
pub const EV_DISPATCH: u16 = 0x80;
pub const EV_DROP: u16 = 0x1000;
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
pub const NOTE_SECONDS: u32 = 0x00000001;
pub const NOTE_MSECONDS: u32 = 0x00000002;
pub const NOTE_USECONDS: u32 = 0x00000004;
pub const NOTE_NSECONDS: u32 = 0x00000008;

pub const MADV_PROTECT: ::c_int = 10;
pub const RUSAGE_THREAD: ::c_int = 1;

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
pub const KERN_HOSTUUID: ::c_int = 36;
pub const KERN_ARND: ::c_int = 37;
pub const KERN_PROC_ALL: ::c_int = 0;
pub const KERN_PROC_PID: ::c_int = 1;
pub const KERN_PROC_PGRP: ::c_int = 2;
pub const KERN_PROC_SESSION: ::c_int = 3;
pub const KERN_PROC_TTY: ::c_int = 4;
pub const KERN_PROC_UID: ::c_int = 5;
pub const KERN_PROC_RUID: ::c_int = 6;
pub const KERN_PROC_ARGS: ::c_int = 7;
pub const KERN_PROC_PROC: ::c_int = 8;
pub const KERN_PROC_SV_NAME: ::c_int = 9;
pub const KERN_PROC_RGID: ::c_int = 10;
pub const KERN_PROC_GID: ::c_int = 11;
pub const KERN_PROC_PATHNAME: ::c_int = 12;
pub const KERN_PROC_OVMMAP: ::c_int = 13;
pub const KERN_PROC_OFILEDESC: ::c_int = 14;
pub const KERN_PROC_KSTACK: ::c_int = 15;
pub const KERN_PROC_INC_THREAD: ::c_int = 0x10;
pub const KERN_PROC_VMMAP: ::c_int = 32;
pub const KERN_PROC_FILEDESC: ::c_int = 33;
pub const KERN_PROC_GROUPS: ::c_int = 34;
pub const KERN_PROC_ENV: ::c_int = 35;
pub const KERN_PROC_AUXV: ::c_int = 36;
pub const KERN_PROC_RLIMIT: ::c_int = 37;
pub const KERN_PROC_PS_STRINGS: ::c_int = 38;
pub const KERN_PROC_UMASK: ::c_int = 39;
pub const KERN_PROC_OSREL: ::c_int = 40;
pub const KERN_PROC_SIGTRAMP: ::c_int = 41;
pub const KIPC_MAXSOCKBUF: ::c_int = 1;
pub const KIPC_SOCKBUF_WASTE: ::c_int = 2;
pub const KIPC_SOMAXCONN: ::c_int = 3;
pub const KIPC_MAX_LINKHDR: ::c_int = 4;
pub const KIPC_MAX_PROTOHDR: ::c_int = 5;
pub const KIPC_MAX_HDR: ::c_int = 6;
pub const KIPC_MAX_DATALEN: ::c_int = 7;
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
pub const HW_REALMEM: ::c_int = 12;
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
pub const CTL_P1003_1B_MQ_OPEN_MAX: ::c_int = 19;
pub const CTL_P1003_1B_PAGESIZE: ::c_int = 20;
pub const CTL_P1003_1B_RTSIG_MAX: ::c_int = 21;
pub const CTL_P1003_1B_SEM_NSEMS_MAX: ::c_int = 22;
pub const CTL_P1003_1B_SEM_VALUE_MAX: ::c_int = 23;
pub const CTL_P1003_1B_SIGQUEUE_MAX: ::c_int = 24;
pub const CTL_P1003_1B_TIMER_MAX: ::c_int = 25;
pub const TIOCGPTN: ::c_uint = 0x4004740f;
pub const TIOCPTMASTER: ::c_uint = 0x2000741c;
pub const TIOCSIG: ::c_uint = 0x2004745f;
pub const TIOCM_DCD: ::c_int = 0x40;
pub const H4DISC: ::c_int = 0x7;

pub const BIOCSETFNR: ::c_ulong = 0x80104282;

pub const FIONWRITE: ::c_ulong = 0x40046677;
pub const FIONSPACE: ::c_ulong = 0x40046676;
pub const FIOSEEKDATA: ::c_ulong = 0xc0086661;
pub const FIOSEEKHOLE: ::c_ulong = 0xc0086662;

pub const JAIL_API_VERSION: u32 = 2;
pub const JAIL_CREATE: ::c_int = 0x01;
pub const JAIL_UPDATE: ::c_int = 0x02;
pub const JAIL_ATTACH: ::c_int = 0x04;
pub const JAIL_DYING: ::c_int = 0x08;
pub const JAIL_SET_MASK: ::c_int = 0x0f;
pub const JAIL_GET_MASK: ::c_int = 0x08;
pub const JAIL_SYS_DISABLE: ::c_int = 0;
pub const JAIL_SYS_NEW: ::c_int = 1;
pub const JAIL_SYS_INHERIT: ::c_int = 2;

pub const SO_BINTIME: ::c_int = 0x2000;
pub const SO_NO_OFFLOAD: ::c_int = 0x4000;
pub const SO_NO_DDP: ::c_int = 0x8000;
pub const SO_REUSEPORT_LB: ::c_int = 0x10000;
pub const SO_LABEL: ::c_int = 0x1009;
pub const SO_PEERLABEL: ::c_int = 0x1010;
pub const SO_LISTENQLIMIT: ::c_int = 0x1011;
pub const SO_LISTENQLEN: ::c_int = 0x1012;
pub const SO_LISTENINCQLEN: ::c_int = 0x1013;
pub const SO_SETFIB: ::c_int = 0x1014;
pub const SO_USER_COOKIE: ::c_int = 0x1015;
pub const SO_PROTOCOL: ::c_int = 0x1016;
pub const SO_PROTOTYPE: ::c_int = SO_PROTOCOL;
pub const SO_VENDOR: ::c_int = 0x80000000;

pub const LOCAL_PEERCRED: ::c_int = 1;
pub const LOCAL_CREDS: ::c_int = 2;
pub const LOCAL_CONNWAIT: ::c_int = 4;
pub const LOCAL_VENDOR: ::c_int = SO_VENDOR;

pub const PT_LWPINFO: ::c_int = 13;
pub const PT_GETNUMLWPS: ::c_int = 14;
pub const PT_GETLWPLIST: ::c_int = 15;
pub const PT_CLEARSTEP: ::c_int = 16;
pub const PT_SETSTEP: ::c_int = 17;
pub const PT_SUSPEND: ::c_int = 18;
pub const PT_RESUME: ::c_int = 19;
pub const PT_TO_SCE: ::c_int = 20;
pub const PT_TO_SCX: ::c_int = 21;
pub const PT_SYSCALL: ::c_int = 22;
pub const PT_FOLLOW_FORK: ::c_int = 23;
pub const PT_LWP_EVENTS: ::c_int = 24;
pub const PT_GET_EVENT_MASK: ::c_int = 25;
pub const PT_SET_EVENT_MASK: ::c_int = 26;
pub const PT_GETREGS: ::c_int = 33;
pub const PT_SETREGS: ::c_int = 34;
pub const PT_GETFPREGS: ::c_int = 35;
pub const PT_SETFPREGS: ::c_int = 36;
pub const PT_GETDBREGS: ::c_int = 37;
pub const PT_SETDBREGS: ::c_int = 38;
pub const PT_VM_TIMESTAMP: ::c_int = 40;
pub const PT_VM_ENTRY: ::c_int = 41;
pub const PT_FIRSTMACH: ::c_int = 64;

pub const PTRACE_EXEC: ::c_int = 0x0001;
pub const PTRACE_SCE: ::c_int = 0x0002;
pub const PTRACE_SCX: ::c_int = 0x0004;
pub const PTRACE_SYSCALL: ::c_int = PTRACE_SCE | PTRACE_SCX;
pub const PTRACE_FORK: ::c_int = 0x0008;
pub const PTRACE_LWP: ::c_int = 0x0010;
pub const PTRACE_VFORK: ::c_int = 0x0020;
pub const PTRACE_DEFAULT: ::c_int = PTRACE_EXEC;

pub const AF_SLOW: ::c_int = 33;
pub const AF_SCLUSTER: ::c_int = 34;
pub const AF_ARP: ::c_int = 35;
pub const AF_BLUETOOTH: ::c_int = 36;
pub const AF_IEEE80211: ::c_int = 37;
pub const AF_INET_SDP: ::c_int = 40;
pub const AF_INET6_SDP: ::c_int = 42;

// https://github.com/freebsd/freebsd/blob/master/sys/net/if.h#L140
pub const IFF_UP: ::c_int = 0x1; // (n) interface is up
pub const IFF_BROADCAST: ::c_int = 0x2; // (i) broadcast address valid
pub const IFF_DEBUG: ::c_int = 0x4; // (n) turn on debugging
pub const IFF_LOOPBACK: ::c_int = 0x8; // (i) is a loopback net
pub const IFF_POINTOPOINT: ::c_int = 0x10; // (i) is a point-to-point link
                                           // 0x20           was IFF_SMART
pub const IFF_RUNNING: ::c_int = 0x40; // (d) resources allocated
#[doc(hidden)]
#[deprecated(
    since = "0.2.54",
    note = "IFF_DRV_RUNNING is deprecated. Use the portable IFF_RUNNING instead"
)]
pub const IFF_DRV_RUNNING: ::c_int = 0x40;
pub const IFF_NOARP: ::c_int = 0x80; // (n) no address resolution protocol
pub const IFF_PROMISC: ::c_int = 0x100; // (n) receive all packets
pub const IFF_ALLMULTI: ::c_int = 0x200; // (n) receive all multicast packets
pub const IFF_OACTIVE: ::c_int = 0x400; // (d) tx hardware queue is full
#[doc(hidden)]
#[deprecated(
    since = "0.2.54",
    note = "Use the portable `IFF_OACTIVE` instead"
)]
pub const IFF_DRV_OACTIVE: ::c_int = 0x400;
pub const IFF_SIMPLEX: ::c_int = 0x800; // (i) can't hear own transmissions
pub const IFF_LINK0: ::c_int = 0x1000; // per link layer defined bit
pub const IFF_LINK1: ::c_int = 0x2000; // per link layer defined bit
pub const IFF_LINK2: ::c_int = 0x4000; // per link layer defined bit
pub const IFF_ALTPHYS: ::c_int = IFF_LINK2; // use alternate physical connection
pub const IFF_MULTICAST: ::c_int = 0x8000; // (i) supports multicast
                                           // (i) unconfigurable using ioctl(2)
pub const IFF_CANTCONFIG: ::c_int = 0x10000;
pub const IFF_PPROMISC: ::c_int = 0x20000; // (n) user-requested promisc mode
pub const IFF_MONITOR: ::c_int = 0x40000; // (n) user-requested monitor mode
pub const IFF_STATICARP: ::c_int = 0x80000; // (n) static ARP
pub const IFF_DYING: ::c_int = 0x200000; // (n) interface is winding down
pub const IFF_RENAMING: ::c_int = 0x400000; // (n) interface is being renamed

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
/// SCTP
pub const IPPROTO_SCTP: ::c_int = 132;
/// IPv6 Mobility Header
pub const IPPROTO_MH: ::c_int = 135;
/// UDP-Lite
pub const IPPROTO_UDPLITE: ::c_int = 136;
/// IP6 Host Identity Protocol
pub const IPPROTO_HIP: ::c_int = 139;
/// IP6 Shim6 Protocol
pub const IPPROTO_SHIM6: ::c_int = 140;

/* 101-254: Partly Unassigned */
/// Protocol Independent Mcast
pub const IPPROTO_PIM: ::c_int = 103;
/// CARP
pub const IPPROTO_CARP: ::c_int = 112;
/// PGM
pub const IPPROTO_PGM: ::c_int = 113;
/// MPLS-in-IP
pub const IPPROTO_MPLS: ::c_int = 137;
/// PFSYNC
pub const IPPROTO_PFSYNC: ::c_int = 240;

/* 255: Reserved */
/* BSD Private, local use, namespace incursion, no longer used */
/// OLD divert pseudo-proto
pub const IPPROTO_OLD_DIVERT: ::c_int = 254;
pub const IPPROTO_MAX: ::c_int = 256;
/// last return value of *_input(), meaning "all job for this pkt is done".
pub const IPPROTO_DONE: ::c_int = 257;

/* Only used internally, so can be outside the range of valid IP protocols. */
/// divert pseudo-protocol
pub const IPPROTO_DIVERT: ::c_int = 258;
/// SeND pseudo-protocol
pub const IPPROTO_SEND: ::c_int = 259;

// sys/netinet/TCP.h
pub const TCP_MD5SIG: ::c_int = 16;
pub const TCP_INFO: ::c_int = 32;
pub const TCP_CONGESTION: ::c_int = 64;
pub const TCP_CCALGOOPT: ::c_int = 65;
pub const TCP_KEEPINIT: ::c_int = 128;
pub const TCP_FASTOPEN: ::c_int = 1025;
pub const TCP_PCAP_OUT: ::c_int = 2048;
pub const TCP_PCAP_IN: ::c_int = 4096;

pub const IP_BINDANY: ::c_int = 24;
pub const IP_BINDMULTI: ::c_int = 25;
pub const IP_RSS_LISTEN_BUCKET: ::c_int = 26;
pub const IP_ORIGDSTADDR: ::c_int = 27;
pub const IP_RECVORIGDSTADDR: ::c_int = IP_ORIGDSTADDR;

pub const IP_RECVTOS: ::c_int = 68;

pub const IPV6_ORIGDSTADDR: ::c_int = 72;
pub const IPV6_RECVORIGDSTADDR: ::c_int = IPV6_ORIGDSTADDR;

pub const PF_SLOW: ::c_int = AF_SLOW;
pub const PF_SCLUSTER: ::c_int = AF_SCLUSTER;
pub const PF_ARP: ::c_int = AF_ARP;
pub const PF_BLUETOOTH: ::c_int = AF_BLUETOOTH;
pub const PF_IEEE80211: ::c_int = AF_IEEE80211;
pub const PF_INET_SDP: ::c_int = AF_INET_SDP;
pub const PF_INET6_SDP: ::c_int = AF_INET6_SDP;

pub const NET_RT_DUMP: ::c_int = 1;
pub const NET_RT_FLAGS: ::c_int = 2;
pub const NET_RT_IFLIST: ::c_int = 3;
pub const NET_RT_IFMALIST: ::c_int = 4;
pub const NET_RT_IFLISTL: ::c_int = 5;

// System V IPC
pub const IPC_PRIVATE: ::key_t = 0;
pub const IPC_CREAT: ::c_int = 0o1000;
pub const IPC_EXCL: ::c_int = 0o2000;
pub const IPC_NOWAIT: ::c_int = 0o4000;
pub const IPC_RMID: ::c_int = 0;
pub const IPC_SET: ::c_int = 1;
pub const IPC_STAT: ::c_int = 2;
pub const IPC_INFO: ::c_int = 3;
pub const IPC_R: ::c_int = 0o400;
pub const IPC_W: ::c_int = 0o200;
pub const IPC_M: ::c_int = 0o10000;
pub const MSG_NOERROR: ::c_int = 0o10000;
pub const SHM_RDONLY: ::c_int = 0o10000;
pub const SHM_RND: ::c_int = 0o20000;
pub const SHM_R: ::c_int = 0o400;
pub const SHM_W: ::c_int = 0o200;
pub const SHM_LOCK: ::c_int = 11;
pub const SHM_UNLOCK: ::c_int = 12;
pub const SHM_STAT: ::c_int = 13;
pub const SHM_INFO: ::c_int = 14;
pub const SHM_ANON: *mut ::c_char = 1 as *mut ::c_char;

// The *_MAXID constants never should've been used outside of the
// FreeBSD base system.  And with the exception of CTL_P1003_1B_MAXID,
// they were all removed in svn r262489.  They remain here for backwards
// compatibility only, and are scheduled to be removed in libc 1.0.0.
#[doc(hidden)]
#[deprecated(since = "0.2.54", note = "Removed in FreeBSD 11")]
pub const CTL_MAXID: ::c_int = 10;
#[doc(hidden)]
#[deprecated(since = "0.2.54", note = "Removed in FreeBSD 11")]
pub const KERN_MAXID: ::c_int = 38;
#[doc(hidden)]
#[deprecated(since = "0.2.54", note = "Removed in FreeBSD 11")]
pub const HW_MAXID: ::c_int = 13;
#[doc(hidden)]
#[deprecated(since = "0.2.54", note = "Removed in FreeBSD 11")]
pub const USER_MAXID: ::c_int = 21;
#[doc(hidden)]
pub const CTL_P1003_1B_MAXID: ::c_int = 26;

pub const MSG_NOTIFICATION: ::c_int = 0x00002000;
pub const MSG_NBIO: ::c_int = 0x00004000;
pub const MSG_COMPAT: ::c_int = 0x00008000;
pub const MSG_CMSG_CLOEXEC: ::c_int = 0x00040000;
pub const MSG_NOSIGNAL: ::c_int = 0x20000;

// utmpx entry types
pub const EMPTY: ::c_short = 0;
pub const BOOT_TIME: ::c_short = 1;
pub const OLD_TIME: ::c_short = 2;
pub const NEW_TIME: ::c_short = 3;
pub const USER_PROCESS: ::c_short = 4;
pub const INIT_PROCESS: ::c_short = 5;
pub const LOGIN_PROCESS: ::c_short = 6;
pub const DEAD_PROCESS: ::c_short = 7;
pub const SHUTDOWN_TIME: ::c_short = 8;
// utmp database types
pub const UTXDB_ACTIVE: ::c_int = 0;
pub const UTXDB_LASTLOGIN: ::c_int = 1;
pub const UTXDB_LOG: ::c_int = 2;

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

pub const WSTOPPED: ::c_int = 2; // same as WUNTRACED
pub const WCONTINUED: ::c_int = 4;
pub const WNOWAIT: ::c_int = 8;
pub const WEXITED: ::c_int = 16;
pub const WTRAPPED: ::c_int = 32;

// FreeBSD defines a great many more of these, we only expose the
// standardized ones.
pub const P_PID: idtype_t = 0;
pub const P_PGID: idtype_t = 2;
pub const P_ALL: idtype_t = 7;

pub const UTIME_OMIT: c_long = -2;
pub const UTIME_NOW: c_long = -1;

pub const B460800: ::speed_t = 460800;
pub const B921600: ::speed_t = 921600;

pub const AT_FDCWD: ::c_int = -100;
pub const AT_EACCESS: ::c_int = 0x100;
pub const AT_SYMLINK_NOFOLLOW: ::c_int = 0x200;
pub const AT_SYMLINK_FOLLOW: ::c_int = 0x400;
pub const AT_REMOVEDIR: ::c_int = 0x800;

pub const TABDLY: ::tcflag_t = 0x00000004;
pub const TAB0: ::tcflag_t = 0x00000000;
pub const TAB3: ::tcflag_t = 0x00000004;

pub const _PC_ACL_NFS4: ::c_int = 64;

pub const _SC_CPUSET_SIZE: ::c_int = 122;

pub const XU_NGROUPS: ::c_int = 16;
pub const XUCRED_VERSION: ::c_uint = 0;

// Flags which can be passed to pdfork(2)
pub const PD_DAEMON: ::c_int = 0x00000001;
pub const PD_CLOEXEC: ::c_int = 0x00000002;
pub const PD_ALLOWED_AT_FORK: ::c_int = PD_DAEMON | PD_CLOEXEC;

// Values for struct rtprio (type_ field)
pub const RTP_PRIO_REALTIME: ::c_ushort = 2;
pub const RTP_PRIO_NORMAL: ::c_ushort = 3;
pub const RTP_PRIO_IDLE: ::c_ushort = 4;

pub const POSIX_SPAWN_RESETIDS: ::c_int = 0x01;
pub const POSIX_SPAWN_SETPGROUP: ::c_int = 0x02;
pub const POSIX_SPAWN_SETSCHEDPARAM: ::c_int = 0x04;
pub const POSIX_SPAWN_SETSCHEDULER: ::c_int = 0x08;
pub const POSIX_SPAWN_SETSIGDEF: ::c_int = 0x10;
pub const POSIX_SPAWN_SETSIGMASK: ::c_int = 0x20;

// Flags for chflags(2)
pub const UF_SYSTEM: ::c_ulong = 0x00000080;
pub const UF_SPARSE: ::c_ulong = 0x00000100;
pub const UF_OFFLINE: ::c_ulong = 0x00000200;
pub const UF_REPARSE: ::c_ulong = 0x00000400;
pub const UF_ARCHIVE: ::c_ulong = 0x00000800;
pub const UF_READONLY: ::c_ulong = 0x00001000;
pub const UF_HIDDEN: ::c_ulong = 0x00008000;
pub const SF_SNAPSHOT: ::c_ulong = 0x00200000;

fn _ALIGN(p: usize) -> usize {
    (p + _ALIGNBYTES) & !_ALIGNBYTES
}

f! {
    pub fn CMSG_DATA(cmsg: *const ::cmsghdr) -> *mut ::c_uchar {
        (cmsg as *mut ::c_uchar)
            .offset(_ALIGN(::mem::size_of::<::cmsghdr>()) as isize)
    }

    pub fn CMSG_LEN(length: ::c_uint) -> ::c_uint {
        _ALIGN(::mem::size_of::<::cmsghdr>()) as ::c_uint + length
    }

    pub fn CMSG_NXTHDR(mhdr: *const ::msghdr, cmsg: *const ::cmsghdr)
        -> *mut ::cmsghdr
    {
        if cmsg.is_null() {
            return ::CMSG_FIRSTHDR(mhdr);
        };
        let next = cmsg as usize + _ALIGN((*cmsg).cmsg_len as usize)
            + _ALIGN(::mem::size_of::<::cmsghdr>());
        let max = (*mhdr).msg_control as usize
            + (*mhdr).msg_controllen as usize;
        if next > max {
            0 as *mut ::cmsghdr
        } else {
            (cmsg as usize + _ALIGN((*cmsg).cmsg_len as usize))
                as *mut ::cmsghdr
        }
    }

    pub fn CMSG_SPACE(length: ::c_uint) -> ::c_uint {
        (_ALIGN(::mem::size_of::<::cmsghdr>()) + _ALIGN(length as usize))
            as ::c_uint
    }

    pub fn uname(buf: *mut ::utsname) -> ::c_int {
        __xuname(256, buf as *mut ::c_void)
    }
}

extern "C" {
    pub fn __error() -> *mut ::c_int;

    pub fn clock_getres(clk_id: ::clockid_t, tp: *mut ::timespec) -> ::c_int;
    pub fn clock_gettime(clk_id: ::clockid_t, tp: *mut ::timespec) -> ::c_int;
    pub fn clock_settime(
        clk_id: ::clockid_t,
        tp: *const ::timespec,
    ) -> ::c_int;

    pub fn extattr_delete_fd(
        fd: ::c_int,
        attrnamespace: ::c_int,
        attrname: *const ::c_char,
    ) -> ::c_int;
    pub fn extattr_delete_file(
        path: *const ::c_char,
        attrnamespace: ::c_int,
        attrname: *const ::c_char,
    ) -> ::c_int;
    pub fn extattr_delete_link(
        path: *const ::c_char,
        attrnamespace: ::c_int,
        attrname: *const ::c_char,
    ) -> ::c_int;
    pub fn extattr_get_fd(
        fd: ::c_int,
        attrnamespace: ::c_int,
        attrname: *const ::c_char,
        data: *mut ::c_void,
        nbytes: ::size_t,
    ) -> ::ssize_t;
    pub fn extattr_get_file(
        path: *const ::c_char,
        attrnamespace: ::c_int,
        attrname: *const ::c_char,
        data: *mut ::c_void,
        nbytes: ::size_t,
    ) -> ::ssize_t;
    pub fn extattr_get_link(
        path: *const ::c_char,
        attrnamespace: ::c_int,
        attrname: *const ::c_char,
        data: *mut ::c_void,
        nbytes: ::size_t,
    ) -> ::ssize_t;
    pub fn extattr_list_fd(
        fd: ::c_int,
        attrnamespace: ::c_int,
        data: *mut ::c_void,
        nbytes: ::size_t,
    ) -> ::ssize_t;
    pub fn extattr_list_file(
        path: *const ::c_char,
        attrnamespace: ::c_int,
        data: *mut ::c_void,
        nbytes: ::size_t,
    ) -> ::ssize_t;
    pub fn extattr_list_link(
        path: *const ::c_char,
        attrnamespace: ::c_int,
        data: *mut ::c_void,
        nbytes: ::size_t,
    ) -> ::ssize_t;
    pub fn extattr_set_fd(
        fd: ::c_int,
        attrnamespace: ::c_int,
        attrname: *const ::c_char,
        data: *const ::c_void,
        nbytes: ::size_t,
    ) -> ::ssize_t;
    pub fn extattr_set_file(
        path: *const ::c_char,
        attrnamespace: ::c_int,
        attrname: *const ::c_char,
        data: *const ::c_void,
        nbytes: ::size_t,
    ) -> ::ssize_t;
    pub fn extattr_set_link(
        path: *const ::c_char,
        attrnamespace: ::c_int,
        attrname: *const ::c_char,
        data: *const ::c_void,
        nbytes: ::size_t,
    ) -> ::ssize_t;

    pub fn jail(jail: *mut ::jail) -> ::c_int;
    pub fn jail_attach(jid: ::c_int) -> ::c_int;
    pub fn jail_remove(jid: ::c_int) -> ::c_int;
    pub fn jail_get(
        iov: *mut ::iovec,
        niov: ::c_uint,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn jail_set(
        iov: *mut ::iovec,
        niov: ::c_uint,
        flags: ::c_int,
    ) -> ::c_int;

    pub fn fdatasync(fd: ::c_int) -> ::c_int;
    pub fn posix_fallocate(
        fd: ::c_int,
        offset: ::off_t,
        len: ::off_t,
    ) -> ::c_int;
    pub fn posix_fadvise(
        fd: ::c_int,
        offset: ::off_t,
        len: ::off_t,
        advise: ::c_int,
    ) -> ::c_int;
    pub fn mkostemp(template: *mut ::c_char, flags: ::c_int) -> ::c_int;
    pub fn mkostemps(
        template: *mut ::c_char,
        suffixlen: ::c_int,
        flags: ::c_int,
    ) -> ::c_int;

    pub fn getutxuser(user: *const ::c_char) -> *mut utmpx;
    pub fn setutxdb(_type: ::c_int, file: *const ::c_char) -> ::c_int;

    pub fn aio_waitcomplete(
        iocbp: *mut *mut aiocb,
        timeout: *mut ::timespec,
    ) -> ::ssize_t;
    pub fn mq_getfd_np(mqd: ::mqd_t) -> ::c_int;

    pub fn waitid(
        idtype: idtype_t,
        id: ::id_t,
        infop: *mut ::siginfo_t,
        options: ::c_int,
    ) -> ::c_int;

    pub fn ftok(pathname: *const ::c_char, proj_id: ::c_int) -> ::key_t;
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
    pub fn msgctl(
        msqid: ::c_int,
        cmd: ::c_int,
        buf: *mut ::msqid_ds,
    ) -> ::c_int;
    pub fn msgget(key: ::key_t, msgflg: ::c_int) -> ::c_int;
    pub fn msgsnd(
        msqid: ::c_int,
        msgp: *const ::c_void,
        msgsz: ::size_t,
        msgflg: ::c_int,
    ) -> ::c_int;
    pub fn cfmakesane(termios: *mut ::termios);
    pub fn fexecve(
        fd: ::c_int,
        argv: *const *const ::c_char,
        envp: *const *const ::c_char,
    ) -> ::c_int;

    pub fn pdfork(fdp: *mut ::c_int, flags: ::c_int) -> ::pid_t;
    pub fn pdgetpid(fd: ::c_int, pidp: *mut ::pid_t) -> ::c_int;
    pub fn pdkill(fd: ::c_int, signum: ::c_int) -> ::c_int;

    pub fn rtprio_thread(
        function: ::c_int,
        lwpid: ::lwpid_t,
        rtp: *mut super::rtprio,
    ) -> ::c_int;

    pub fn posix_spawn(
        pid: *mut ::pid_t,
        path: *const ::c_char,
        file_actions: *const ::posix_spawn_file_actions_t,
        attrp: *const ::posix_spawnattr_t,
        argv: *const *mut ::c_char,
        envp: *const *mut ::c_char,
    ) -> ::c_int;
    pub fn posix_spawnp(
        pid: *mut ::pid_t,
        file: *const ::c_char,
        file_actions: *const ::posix_spawn_file_actions_t,
        attrp: *const ::posix_spawnattr_t,
        argv: *const *mut ::c_char,
        envp: *const *mut ::c_char,
    ) -> ::c_int;
    pub fn posix_spawnattr_init(attr: *mut posix_spawnattr_t) -> ::c_int;
    pub fn posix_spawnattr_destroy(attr: *mut posix_spawnattr_t) -> ::c_int;
    pub fn posix_spawnattr_getsigdefault(
        attr: *const posix_spawnattr_t,
        default: *mut ::sigset_t,
    ) -> ::c_int;
    pub fn posix_spawnattr_setsigdefault(
        attr: *mut posix_spawnattr_t,
        default: *const ::sigset_t,
    ) -> ::c_int;
    pub fn posix_spawnattr_getsigmask(
        attr: *const posix_spawnattr_t,
        default: *mut ::sigset_t,
    ) -> ::c_int;
    pub fn posix_spawnattr_setsigmask(
        attr: *mut posix_spawnattr_t,
        default: *const ::sigset_t,
    ) -> ::c_int;
    pub fn posix_spawnattr_getflags(
        attr: *const posix_spawnattr_t,
        flags: *mut ::c_short,
    ) -> ::c_int;
    pub fn posix_spawnattr_setflags(
        attr: *mut posix_spawnattr_t,
        flags: ::c_short,
    ) -> ::c_int;
    pub fn posix_spawnattr_getpgroup(
        attr: *const posix_spawnattr_t,
        flags: *mut ::pid_t,
    ) -> ::c_int;
    pub fn posix_spawnattr_setpgroup(
        attr: *mut posix_spawnattr_t,
        flags: ::pid_t,
    ) -> ::c_int;
    pub fn posix_spawnattr_getschedpolicy(
        attr: *const posix_spawnattr_t,
        flags: *mut ::c_int,
    ) -> ::c_int;
    pub fn posix_spawnattr_setschedpolicy(
        attr: *mut posix_spawnattr_t,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn posix_spawnattr_getschedparam(
        attr: *const posix_spawnattr_t,
        param: *mut ::sched_param,
    ) -> ::c_int;
    pub fn posix_spawnattr_setschedparam(
        attr: *mut posix_spawnattr_t,
        param: *const ::sched_param,
    ) -> ::c_int;

    pub fn posix_spawn_file_actions_init(
        actions: *mut posix_spawn_file_actions_t,
    ) -> ::c_int;
    pub fn posix_spawn_file_actions_destroy(
        actions: *mut posix_spawn_file_actions_t,
    ) -> ::c_int;
    pub fn posix_spawn_file_actions_addopen(
        actions: *mut posix_spawn_file_actions_t,
        fd: ::c_int,
        path: *const ::c_char,
        oflag: ::c_int,
        mode: ::mode_t,
    ) -> ::c_int;
    pub fn posix_spawn_file_actions_addclose(
        actions: *mut posix_spawn_file_actions_t,
        fd: ::c_int,
    ) -> ::c_int;
    pub fn posix_spawn_file_actions_adddup2(
        actions: *mut posix_spawn_file_actions_t,
        fd: ::c_int,
        newfd: ::c_int,
    ) -> ::c_int;

    #[cfg_attr(
        all(target_os = "freebsd", freebsd11),
        link_name = "statfs@FBSD_1.0"
    )]
    pub fn statfs(path: *const ::c_char, buf: *mut statfs) -> ::c_int;
    #[cfg_attr(
        all(target_os = "freebsd", freebsd11),
        link_name = "fstatfs@FBSD_1.0"
    )]
    pub fn fstatfs(fd: ::c_int, buf: *mut statfs) -> ::c_int;

    pub fn dup3(src: ::c_int, dst: ::c_int, flags: ::c_int) -> ::c_int;
    pub fn __xuname(nmln: ::c_int, buf: *mut ::c_void) -> ::c_int;

    pub fn sendmmsg(
        sockfd: ::c_int,
        msgvec: *mut ::mmsghdr,
        vlen: ::size_t,
        flags: ::c_int,
    ) -> ::ssize_t;
    pub fn recvmmsg(
        sockfd: ::c_int,
        msgvec: *mut ::mmsghdr,
        vlen: ::size_t,
        flags: ::c_int,
        timeout: *const ::timespec,
    ) -> ::ssize_t;
}

#[link(name = "util")]
extern "C" {
    pub fn extattr_namespace_to_string(
        attrnamespace: ::c_int,
        string: *mut *mut ::c_char,
    ) -> ::c_int;
    pub fn extattr_string_to_namespace(
        string: *const ::c_char,
        attrnamespace: *mut ::c_int,
    ) -> ::c_int;
}

cfg_if! {
    if #[cfg(freebsd12)] {
        mod freebsd12;
        pub use self::freebsd12::*;
    } else if #[cfg(freebsd13)] {
        mod freebsd12;
        pub use self::freebsd12::*;
    } else if #[cfg(any(freebsd10, freebsd11))] {
        mod freebsd11;
        pub use self::freebsd11::*;
    } else {
        // Unknown freebsd version
    }
}

cfg_if! {
    if #[cfg(target_arch = "x86")] {
        mod x86;
        pub use self::x86::*;
    } else if #[cfg(target_arch = "x86_64")] {
        mod x86_64;
        pub use self::x86_64::*;
    } else if #[cfg(target_arch = "aarch64")] {
        mod aarch64;
        pub use self::aarch64::*;
    } else if #[cfg(target_arch = "arm")] {
        mod arm;
        pub use self::arm::*;
    } else if #[cfg(target_arch = "powerpc64")] {
        mod powerpc64;
        pub use self::powerpc64::*;
    } else {
        // Unknown target_arch
    }
}
