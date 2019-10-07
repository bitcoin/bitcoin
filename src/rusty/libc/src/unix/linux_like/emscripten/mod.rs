pub type c_char = i8;
pub type wchar_t = i32;
pub type useconds_t = u32;
pub type dev_t = u32;
pub type socklen_t = u32;
pub type pthread_t = c_ulong;
pub type mode_t = u32;
pub type ino64_t = u64;
pub type off64_t = i64;
pub type blkcnt64_t = i32;
pub type rlim64_t = u64;
pub type shmatt_t = ::c_ulong;
pub type mqd_t = ::c_int;
pub type msgqnum_t = ::c_ulong;
pub type msglen_t = ::c_ulong;
pub type nfds_t = ::c_ulong;
pub type nl_item = ::c_int;
pub type idtype_t = ::c_uint;
pub type loff_t = i64;
pub type pthread_key_t = ::c_uint;

pub type clock_t = c_long;
pub type time_t = c_long;
pub type suseconds_t = c_long;
pub type ino_t = u64;
pub type off_t = i64;
pub type blkcnt_t = i32;

pub type blksize_t = c_long;
pub type fsblkcnt_t = u32;
pub type fsfilcnt_t = u32;
pub type rlim_t = ::c_ulonglong;
pub type c_long = i32;
pub type c_ulong = u32;
pub type nlink_t = u32;

#[cfg_attr(feature = "extra_traits", derive(Debug))]
pub enum fpos64_t {} // TODO: fill this out with a struct
impl ::Copy for fpos64_t {}
impl ::Clone for fpos64_t {
    fn clone(&self) -> fpos64_t {
        *self
    }
}

s! {
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
        pub f_fsid: ::c_ulong,
        __f_unused: ::c_int,
        pub f_flag: ::c_ulong,
        pub f_namemax: ::c_ulong,
        __f_spare: [::c_int; 6],
    }

    pub struct dqblk {
        pub dqb_bhardlimit: u64,
        pub dqb_bsoftlimit: u64,
        pub dqb_curspace: u64,
        pub dqb_ihardlimit: u64,
        pub dqb_isoftlimit: u64,
        pub dqb_curinodes: u64,
        pub dqb_btime: u64,
        pub dqb_itime: u64,
        pub dqb_valid: u32,
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
        bits: [u32; 32],
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

    pub struct sembuf {
        pub sem_num: ::c_ushort,
        pub sem_op: ::c_short,
        pub sem_flg: ::c_short,
    }

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
        __dummy4: [::c_char; 24],
    }

    pub struct sigaction {
        pub sa_sigaction: ::sighandler_t,
        pub sa_mask: ::sigset_t,
        pub sa_flags: ::c_int,
        pub sa_restorer: ::Option<extern fn()>,
    }

    pub struct ipc_perm {
        pub __ipc_perm_key: ::key_t,
        pub uid: ::uid_t,
        pub gid: ::gid_t,
        pub cuid: ::uid_t,
        pub cgid: ::gid_t,
        pub mode: ::mode_t,
        pub __seq: ::c_int,
        __unused1: ::c_long,
        __unused2: ::c_long
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

    pub struct pthread_attr_t {
        __size: [u32; 11]
    }

    pub struct sigset_t {
        __val: [::c_ulong; 32],
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

    pub struct sem_t {
        __val: [::c_int; 4],
    }
    pub struct stat {
        pub st_dev: ::dev_t,
        __st_dev_padding: ::c_int,
        __st_ino_truncated: ::c_long,
        pub st_mode: ::mode_t,
        pub st_nlink: ::nlink_t,
        pub st_uid: ::uid_t,
        pub st_gid: ::gid_t,
        pub st_rdev: ::dev_t,
        __st_rdev_padding: ::c_int,
        pub st_size: ::off_t,
        pub st_blksize: ::blksize_t,
        pub st_blocks: ::blkcnt_t,
        pub st_atime: ::time_t,
        pub st_atime_nsec: ::c_long,
        pub st_mtime: ::time_t,
        pub st_mtime_nsec: ::c_long,
        pub st_ctime: ::time_t,
        pub st_ctime_nsec: ::c_long,
        pub st_ino: ::ino_t,
    }

    pub struct stat64 {
        pub st_dev: ::dev_t,
        __st_dev_padding: ::c_int,
        __st_ino_truncated: ::c_long,
        pub st_mode: ::mode_t,
        pub st_nlink: ::nlink_t,
        pub st_uid: ::uid_t,
        pub st_gid: ::gid_t,
        pub st_rdev: ::dev_t,
        __st_rdev_padding: ::c_int,
        pub st_size: ::off_t,
        pub st_blksize: ::blksize_t,
        pub st_blocks: ::blkcnt_t,
        pub st_atime: ::time_t,
        pub st_atime_nsec: ::c_long,
        pub st_mtime: ::time_t,
        pub st_mtime_nsec: ::c_long,
        pub st_ctime: ::time_t,
        pub st_ctime_nsec: ::c_long,
        pub st_ino: ::ino_t,
    }

    pub struct stack_t {
        pub ss_sp: *mut ::c_void,
        pub ss_flags: ::c_int,
        pub ss_size: ::size_t
    }

    pub struct shmid_ds {
        pub shm_perm: ::ipc_perm,
        pub shm_segsz: ::size_t,
        pub shm_atime: ::time_t,
        __unused1: ::c_int,
        pub shm_dtime: ::time_t,
        __unused2: ::c_int,
        pub shm_ctime: ::time_t,
        __unused3: ::c_int,
        pub shm_cpid: ::pid_t,
        pub shm_lpid: ::pid_t,
        pub shm_nattch: ::c_ulong,
        __pad1: ::c_ulong,
        __pad2: ::c_ulong,
    }

    pub struct msqid_ds {
        pub msg_perm: ::ipc_perm,
        pub msg_stime: ::time_t,
        __unused1: ::c_int,
        pub msg_rtime: ::time_t,
        __unused2: ::c_int,
        pub msg_ctime: ::time_t,
        __unused3: ::c_int,
        __msg_cbytes: ::c_ulong,
        pub msg_qnum: ::msgqnum_t,
        pub msg_qbytes: ::msglen_t,
        pub msg_lspid: ::pid_t,
        pub msg_lrpid: ::pid_t,
        __pad1: ::c_ulong,
        __pad2: ::c_ulong,
    }

    pub struct statfs {
        pub f_type: ::c_ulong,
        pub f_bsize: ::c_ulong,
        pub f_blocks: ::fsblkcnt_t,
        pub f_bfree: ::fsblkcnt_t,
        pub f_bavail: ::fsblkcnt_t,
        pub f_files: ::fsfilcnt_t,
        pub f_ffree: ::fsfilcnt_t,
        pub f_fsid: ::fsid_t,
        pub f_namelen: ::c_ulong,
        pub f_frsize: ::c_ulong,
        pub f_flags: ::c_ulong,
        pub f_spare: [::c_ulong; 4],
    }

    pub struct siginfo_t {
        pub si_signo: ::c_int,
        pub si_errno: ::c_int,
        pub si_code: ::c_int,
        pub _pad: [::c_int; 29],
        _align: [usize; 0],
    }

    pub struct statfs64 {
        pub f_type: ::c_ulong,
        pub f_bsize: ::c_ulong,
        pub f_blocks: ::fsblkcnt_t,
        pub f_bfree: ::fsblkcnt_t,
        pub f_bavail: ::fsblkcnt_t,
        pub f_files: ::fsfilcnt_t,
        pub f_ffree: ::fsfilcnt_t,
        pub f_fsid: ::fsid_t,
        pub f_namelen: ::c_ulong,
        pub f_frsize: ::c_ulong,
        pub f_flags: ::c_ulong,
        pub f_spare: [::c_ulong; 4],
    }

    pub struct statvfs64 {
        pub f_bsize: ::c_ulong,
        pub f_frsize: ::c_ulong,
        pub f_blocks: u32,
        pub f_bfree: u32,
        pub f_bavail: u32,
        pub f_files: u32,
        pub f_ffree: u32,
        pub f_favail: u32,
        pub f_fsid: ::c_ulong,
        __f_unused: ::c_int,
        pub f_flag: ::c_ulong,
        pub f_namemax: ::c_ulong,
        __f_spare: [::c_int; 6],
    }

    pub struct arpd_request {
        pub req: ::c_ushort,
        pub ip: u32,
        pub dev: ::c_ulong,
        pub stamp: ::c_ulong,
        pub updated: ::c_ulong,
        pub ha: [::c_uchar; ::MAX_ADDR_LEN],
    }
}

s_no_extra_traits! {
    pub struct dirent {
        pub d_ino: ::ino_t,
        pub d_off: ::off_t,
        pub d_reclen: ::c_ushort,
        pub d_type: ::c_uchar,
        pub d_name: [::c_char; 256],
    }

    pub struct dirent64 {
        pub d_ino: ::ino64_t,
        pub d_off: ::off64_t,
        pub d_reclen: ::c_ushort,
        pub d_type: ::c_uchar,
        pub d_name: [::c_char; 256],
    }

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

    pub struct mq_attr {
        pub mq_flags: ::c_long,
        pub mq_maxmsg: ::c_long,
        pub mq_msgsize: ::c_long,
        pub mq_curmsgs: ::c_long,
        pad: [::c_long; 4]
    }
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

        impl PartialEq for dirent64 {
            fn eq(&self, other: &dirent64) -> bool {
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
        impl Eq for dirent64 {}
        impl ::fmt::Debug for dirent64 {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("dirent64")
                    .field("d_ino", &self.d_ino)
                    .field("d_off", &self.d_off)
                    .field("d_reclen", &self.d_reclen)
                    .field("d_type", &self.d_type)
                    // FIXME: .field("d_name", &self.d_name)
                    .finish()
            }
        }
        impl ::hash::Hash for dirent64 {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.d_ino.hash(state);
                self.d_off.hash(state);
                self.d_reclen.hash(state);
                self.d_type.hash(state);
                self.d_name.hash(state);
            }
        }

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
    }
}

pub const MADV_SOFT_OFFLINE: ::c_int = 101;
pub const MS_NOUSER: ::c_ulong = 0x80000000;
pub const MS_RMT_MASK: ::c_ulong = 0x02800051;

pub const ABDAY_1: ::nl_item = 0x20000;
pub const ABDAY_2: ::nl_item = 0x20001;
pub const ABDAY_3: ::nl_item = 0x20002;
pub const ABDAY_4: ::nl_item = 0x20003;
pub const ABDAY_5: ::nl_item = 0x20004;
pub const ABDAY_6: ::nl_item = 0x20005;
pub const ABDAY_7: ::nl_item = 0x20006;

pub const DAY_1: ::nl_item = 0x20007;
pub const DAY_2: ::nl_item = 0x20008;
pub const DAY_3: ::nl_item = 0x20009;
pub const DAY_4: ::nl_item = 0x2000A;
pub const DAY_5: ::nl_item = 0x2000B;
pub const DAY_6: ::nl_item = 0x2000C;
pub const DAY_7: ::nl_item = 0x2000D;

pub const ABMON_1: ::nl_item = 0x2000E;
pub const ABMON_2: ::nl_item = 0x2000F;
pub const ABMON_3: ::nl_item = 0x20010;
pub const ABMON_4: ::nl_item = 0x20011;
pub const ABMON_5: ::nl_item = 0x20012;
pub const ABMON_6: ::nl_item = 0x20013;
pub const ABMON_7: ::nl_item = 0x20014;
pub const ABMON_8: ::nl_item = 0x20015;
pub const ABMON_9: ::nl_item = 0x20016;
pub const ABMON_10: ::nl_item = 0x20017;
pub const ABMON_11: ::nl_item = 0x20018;
pub const ABMON_12: ::nl_item = 0x20019;

pub const MON_1: ::nl_item = 0x2001A;
pub const MON_2: ::nl_item = 0x2001B;
pub const MON_3: ::nl_item = 0x2001C;
pub const MON_4: ::nl_item = 0x2001D;
pub const MON_5: ::nl_item = 0x2001E;
pub const MON_6: ::nl_item = 0x2001F;
pub const MON_7: ::nl_item = 0x20020;
pub const MON_8: ::nl_item = 0x20021;
pub const MON_9: ::nl_item = 0x20022;
pub const MON_10: ::nl_item = 0x20023;
pub const MON_11: ::nl_item = 0x20024;
pub const MON_12: ::nl_item = 0x20025;

pub const AM_STR: ::nl_item = 0x20026;
pub const PM_STR: ::nl_item = 0x20027;

pub const D_T_FMT: ::nl_item = 0x20028;
pub const D_FMT: ::nl_item = 0x20029;
pub const T_FMT: ::nl_item = 0x2002A;
pub const T_FMT_AMPM: ::nl_item = 0x2002B;

pub const ERA: ::nl_item = 0x2002C;
pub const ERA_D_FMT: ::nl_item = 0x2002E;
pub const ALT_DIGITS: ::nl_item = 0x2002F;
pub const ERA_D_T_FMT: ::nl_item = 0x20030;
pub const ERA_T_FMT: ::nl_item = 0x20031;

pub const CODESET: ::nl_item = 14;

pub const CRNCYSTR: ::nl_item = 0x4000F;

pub const RUSAGE_THREAD: ::c_int = 1;
pub const RUSAGE_CHILDREN: ::c_int = -1;

pub const RADIXCHAR: ::nl_item = 0x10000;
pub const THOUSEP: ::nl_item = 0x10001;

pub const YESEXPR: ::nl_item = 0x50000;
pub const NOEXPR: ::nl_item = 0x50001;
pub const YESSTR: ::nl_item = 0x50002;
pub const NOSTR: ::nl_item = 0x50003;

pub const FILENAME_MAX: ::c_uint = 4096;
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
pub const _SC_UIO_MAXIOV: ::c_int = 60;
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
pub const _SC_THREAD_PROCESS_SHARED: ::c_int = 82;
pub const _SC_NPROCESSORS_CONF: ::c_int = 83;
pub const _SC_NPROCESSORS_ONLN: ::c_int = 84;
pub const _SC_PHYS_PAGES: ::c_int = 85;
pub const _SC_AVPHYS_PAGES: ::c_int = 86;
pub const _SC_ATEXIT_MAX: ::c_int = 87;
pub const _SC_PASS_MAX: ::c_int = 88;
pub const _SC_XOPEN_VERSION: ::c_int = 89;
pub const _SC_XOPEN_XCU_VERSION: ::c_int = 90;
pub const _SC_XOPEN_UNIX: ::c_int = 91;
pub const _SC_XOPEN_CRYPT: ::c_int = 92;
pub const _SC_XOPEN_ENH_I18N: ::c_int = 93;
pub const _SC_XOPEN_SHM: ::c_int = 94;
pub const _SC_2_CHAR_TERM: ::c_int = 95;
pub const _SC_2_UPE: ::c_int = 97;
pub const _SC_XOPEN_XPG2: ::c_int = 98;
pub const _SC_XOPEN_XPG3: ::c_int = 99;
pub const _SC_XOPEN_XPG4: ::c_int = 100;
pub const _SC_NZERO: ::c_int = 109;
pub const _SC_XBS5_ILP32_OFF32: ::c_int = 125;
pub const _SC_XBS5_ILP32_OFFBIG: ::c_int = 126;
pub const _SC_XBS5_LP64_OFF64: ::c_int = 127;
pub const _SC_XBS5_LPBIG_OFFBIG: ::c_int = 128;
pub const _SC_XOPEN_LEGACY: ::c_int = 129;
pub const _SC_XOPEN_REALTIME: ::c_int = 130;
pub const _SC_XOPEN_REALTIME_THREADS: ::c_int = 131;
pub const _SC_ADVISORY_INFO: ::c_int = 132;
pub const _SC_BARRIERS: ::c_int = 133;
pub const _SC_CLOCK_SELECTION: ::c_int = 137;
pub const _SC_CPUTIME: ::c_int = 138;
pub const _SC_THREAD_CPUTIME: ::c_int = 139;
pub const _SC_MONOTONIC_CLOCK: ::c_int = 149;
pub const _SC_READER_WRITER_LOCKS: ::c_int = 153;
pub const _SC_SPIN_LOCKS: ::c_int = 154;
pub const _SC_REGEXP: ::c_int = 155;
pub const _SC_SHELL: ::c_int = 157;
pub const _SC_SPAWN: ::c_int = 159;
pub const _SC_SPORADIC_SERVER: ::c_int = 160;
pub const _SC_THREAD_SPORADIC_SERVER: ::c_int = 161;
pub const _SC_TIMEOUTS: ::c_int = 164;
pub const _SC_TYPED_MEMORY_OBJECTS: ::c_int = 165;
pub const _SC_2_PBS: ::c_int = 168;
pub const _SC_2_PBS_ACCOUNTING: ::c_int = 169;
pub const _SC_2_PBS_LOCATE: ::c_int = 170;
pub const _SC_2_PBS_MESSAGE: ::c_int = 171;
pub const _SC_2_PBS_TRACK: ::c_int = 172;
pub const _SC_SYMLOOP_MAX: ::c_int = 173;
pub const _SC_STREAMS: ::c_int = 174;
pub const _SC_2_PBS_CHECKPOINT: ::c_int = 175;
pub const _SC_V6_ILP32_OFF32: ::c_int = 176;
pub const _SC_V6_ILP32_OFFBIG: ::c_int = 177;
pub const _SC_V6_LP64_OFF64: ::c_int = 178;
pub const _SC_V6_LPBIG_OFFBIG: ::c_int = 179;
pub const _SC_HOST_NAME_MAX: ::c_int = 180;
pub const _SC_TRACE: ::c_int = 181;
pub const _SC_TRACE_EVENT_FILTER: ::c_int = 182;
pub const _SC_TRACE_INHERIT: ::c_int = 183;
pub const _SC_TRACE_LOG: ::c_int = 184;
pub const _SC_IPV6: ::c_int = 235;
pub const _SC_RAW_SOCKETS: ::c_int = 236;
pub const _SC_V7_ILP32_OFF32: ::c_int = 237;
pub const _SC_V7_ILP32_OFFBIG: ::c_int = 238;
pub const _SC_V7_LP64_OFF64: ::c_int = 239;
pub const _SC_V7_LPBIG_OFFBIG: ::c_int = 240;
pub const _SC_SS_REPL_MAX: ::c_int = 241;
pub const _SC_TRACE_EVENT_NAME_MAX: ::c_int = 242;
pub const _SC_TRACE_NAME_MAX: ::c_int = 243;
pub const _SC_TRACE_SYS_MAX: ::c_int = 244;
pub const _SC_TRACE_USER_EVENT_MAX: ::c_int = 245;
pub const _SC_XOPEN_STREAMS: ::c_int = 246;
pub const _SC_THREAD_ROBUST_PRIO_INHERIT: ::c_int = 247;
pub const _SC_THREAD_ROBUST_PRIO_PROTECT: ::c_int = 248;

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
pub const PTHREAD_PROCESS_PRIVATE: ::c_int = 0;
pub const PTHREAD_PROCESS_SHARED: ::c_int = 1;
pub const __SIZEOF_PTHREAD_COND_T: usize = 48;

pub const SCHED_OTHER: ::c_int = 0;
pub const SCHED_FIFO: ::c_int = 1;
pub const SCHED_RR: ::c_int = 2;
pub const SCHED_BATCH: ::c_int = 3;
pub const SCHED_IDLE: ::c_int = 5;

pub const AF_IB: ::c_int = 27;
pub const AF_MPLS: ::c_int = 28;
pub const AF_NFC: ::c_int = 39;
pub const AF_VSOCK: ::c_int = 40;
pub const PF_IB: ::c_int = AF_IB;
pub const PF_MPLS: ::c_int = AF_MPLS;
pub const PF_NFC: ::c_int = AF_NFC;
pub const PF_VSOCK: ::c_int = AF_VSOCK;

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
pub const SHM_EXEC: ::c_int = 0o100000;

pub const SHM_LOCK: ::c_int = 11;
pub const SHM_UNLOCK: ::c_int = 12;

pub const SHM_HUGETLB: ::c_int = 0o4000;
pub const SHM_NORESERVE: ::c_int = 0o10000;

pub const EPOLLRDHUP: ::c_int = 0x2000;
pub const EPOLLEXCLUSIVE: ::c_int = 0x10000000;
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
pub const RB_SW_SUSPEND: ::c_int = 0xd000fce2u32 as i32;
pub const RB_KEXEC: ::c_int = 0x45584543u32 as i32;

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

pub const AIO_CANCELED: ::c_int = 0;
pub const AIO_NOTCANCELED: ::c_int = 1;
pub const AIO_ALLDONE: ::c_int = 2;
pub const LIO_READ: ::c_int = 0;
pub const LIO_WRITE: ::c_int = 1;
pub const LIO_NOP: ::c_int = 2;
pub const LIO_WAIT: ::c_int = 0;
pub const LIO_NOWAIT: ::c_int = 1;

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

pub const PR_MPX_ENABLE_MANAGEMENT: ::c_int = 43;
pub const PR_MPX_DISABLE_MANAGEMENT: ::c_int = 44;

pub const PR_SET_FP_MODE: ::c_int = 45;
pub const PR_GET_FP_MODE: ::c_int = 46;
pub const PR_FP_MODE_FR: ::c_int = 1 << 0;
pub const PR_FP_MODE_FRE: ::c_int = 1 << 1;

pub const PR_CAP_AMBIENT: ::c_int = 47;
pub const PR_CAP_AMBIENT_IS_SET: ::c_int = 1;
pub const PR_CAP_AMBIENT_RAISE: ::c_int = 2;
pub const PR_CAP_AMBIENT_LOWER: ::c_int = 3;
pub const PR_CAP_AMBIENT_CLEAR_ALL: ::c_int = 4;

pub const ITIMER_REAL: ::c_int = 0;
pub const ITIMER_VIRTUAL: ::c_int = 1;
pub const ITIMER_PROF: ::c_int = 2;

pub const XATTR_CREATE: ::c_int = 0x1;
pub const XATTR_REPLACE: ::c_int = 0x2;

pub const _POSIX_VDISABLE: ::cc_t = 0;

pub const FALLOC_FL_KEEP_SIZE: ::c_int = 0x01;
pub const FALLOC_FL_PUNCH_HOLE: ::c_int = 0x02;

// On Linux, libc doesn't define this constant, libattr does instead.
// We still define it for Linux as it's defined by libc on other platforms,
// and it's mentioned in the man pages for getxattr and setxattr.
pub const SFD_CLOEXEC: ::c_int = 0x080000;

pub const NCCS: usize = 32;

pub const O_TRUNC: ::c_int = 512;
pub const O_NOATIME: ::c_int = 0o1000000;
pub const O_CLOEXEC: ::c_int = 0x80000;

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

pub const POSIX_MADV_DONTNEED: ::c_int = 0;

pub const RLIM_INFINITY: ::rlim_t = !0;
pub const RLIMIT_NLIMITS: ::c_int = 15;

pub const MAP_ANONYMOUS: ::c_int = MAP_ANON;

pub const TCP_THIN_LINEAR_TIMEOUTS: ::c_int = 16;
pub const TCP_THIN_DUPACK: ::c_int = 17;
pub const TCP_USER_TIMEOUT: ::c_int = 18;
pub const TCP_REPAIR: ::c_int = 19;
pub const TCP_REPAIR_QUEUE: ::c_int = 20;
pub const TCP_QUEUE_SEQ: ::c_int = 21;
pub const TCP_REPAIR_OPTIONS: ::c_int = 22;
pub const TCP_FASTOPEN: ::c_int = 23;
pub const TCP_TIMESTAMP: ::c_int = 24;

#[doc(hidden)]
#[deprecated(since = "0.2.55", note = "Use SIGSYS instead")]
pub const SIGUNUSED: ::c_int = ::SIGSYS;

pub const __SIZEOF_PTHREAD_CONDATTR_T: usize = 4;
pub const __SIZEOF_PTHREAD_MUTEXATTR_T: usize = 4;
pub const __SIZEOF_PTHREAD_RWLOCKATTR_T: usize = 8;

pub const CPU_SETSIZE: ::c_int = 128;

pub const QFMT_VFS_V1: ::c_int = 4;

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
pub const PTRACE_ATTACH: ::c_int = 16;
pub const PTRACE_DETACH: ::c_int = 17;
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

pub const PTRACE_GETFPREGS: ::c_uint = 14;
pub const PTRACE_SETFPREGS: ::c_uint = 15;
pub const PTRACE_GETFPXREGS: ::c_uint = 18;
pub const PTRACE_SETFPXREGS: ::c_uint = 19;
pub const PTRACE_GETREGS: ::c_uint = 12;
pub const PTRACE_SETREGS: ::c_uint = 13;

pub const EFD_NONBLOCK: ::c_int = ::O_NONBLOCK;

pub const SFD_NONBLOCK: ::c_int = ::O_NONBLOCK;

pub const TCSANOW: ::c_int = 0;
pub const TCSADRAIN: ::c_int = 1;
pub const TCSAFLUSH: ::c_int = 2;

pub const TIOCINQ: ::c_int = ::FIONREAD;

pub const RTLD_GLOBAL: ::c_int = 0x100;
pub const RTLD_NOLOAD: ::c_int = 0x4;

// TODO(#247) Temporarily musl-specific (available since musl 0.9.12 / Linux
// kernel 3.10).  See also linux_like/mod.rs
pub const CLOCK_SGI_CYCLE: ::clockid_t = 10;
pub const CLOCK_TAI: ::clockid_t = 11;

pub const MCL_CURRENT: ::c_int = 0x0001;
pub const MCL_FUTURE: ::c_int = 0x0002;

pub const SIGSTKSZ: ::size_t = 8192;
pub const MINSIGSTKSZ: ::size_t = 2048;
pub const CBAUD: ::tcflag_t = 0o0010017;
pub const TAB1: ::c_int = 0x00000800;
pub const TAB2: ::c_int = 0x00001000;
pub const TAB3: ::c_int = 0x00001800;
pub const CR1: ::c_int = 0x00000200;
pub const CR2: ::c_int = 0x00000400;
pub const CR3: ::c_int = 0x00000600;
pub const FF1: ::c_int = 0x00008000;
pub const BS1: ::c_int = 0x00002000;
pub const VT1: ::c_int = 0x00004000;
pub const VWERASE: usize = 14;
pub const VREPRINT: usize = 12;
pub const VSUSP: usize = 10;
pub const VSTART: usize = 8;
pub const VSTOP: usize = 9;
pub const VDISCARD: usize = 13;
pub const VTIME: usize = 5;
pub const IXON: ::tcflag_t = 0x00000400;
pub const IXOFF: ::tcflag_t = 0x00001000;
pub const ONLCR: ::tcflag_t = 0x4;
pub const CSIZE: ::tcflag_t = 0x00000030;
pub const CS6: ::tcflag_t = 0x00000010;
pub const CS7: ::tcflag_t = 0x00000020;
pub const CS8: ::tcflag_t = 0x00000030;
pub const CSTOPB: ::tcflag_t = 0x00000040;
pub const CREAD: ::tcflag_t = 0x00000080;
pub const PARENB: ::tcflag_t = 0x00000100;
pub const PARODD: ::tcflag_t = 0x00000200;
pub const HUPCL: ::tcflag_t = 0x00000400;
pub const CLOCAL: ::tcflag_t = 0x00000800;
pub const ECHOKE: ::tcflag_t = 0x00000800;
pub const ECHOE: ::tcflag_t = 0x00000010;
pub const ECHOK: ::tcflag_t = 0x00000020;
pub const ECHONL: ::tcflag_t = 0x00000040;
pub const ECHOPRT: ::tcflag_t = 0x00000400;
pub const ECHOCTL: ::tcflag_t = 0x00000200;
pub const ISIG: ::tcflag_t = 0x00000001;
pub const ICANON: ::tcflag_t = 0x00000002;
pub const PENDIN: ::tcflag_t = 0x00004000;
pub const NOFLSH: ::tcflag_t = 0x00000080;
pub const CBAUDEX: ::tcflag_t = 0o010000;
pub const VSWTC: usize = 7;
pub const OLCUC: ::tcflag_t = 0o000002;
pub const NLDLY: ::tcflag_t = 0o000400;
pub const CRDLY: ::tcflag_t = 0o003000;
pub const TABDLY: ::tcflag_t = 0o014000;
pub const BSDLY: ::tcflag_t = 0o020000;
pub const FFDLY: ::tcflag_t = 0o100000;
pub const VTDLY: ::tcflag_t = 0o040000;
pub const XTABS: ::tcflag_t = 0o014000;

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
pub const B57600: ::speed_t = 0o010001;
pub const B115200: ::speed_t = 0o010002;
pub const B230400: ::speed_t = 0o010003;
pub const B460800: ::speed_t = 0o010004;
pub const B500000: ::speed_t = 0o010005;
pub const B576000: ::speed_t = 0o010006;
pub const B921600: ::speed_t = 0o010007;
pub const B1000000: ::speed_t = 0o010010;
pub const B1152000: ::speed_t = 0o010011;
pub const B1500000: ::speed_t = 0o010012;
pub const B2000000: ::speed_t = 0o010013;
pub const B2500000: ::speed_t = 0o010014;
pub const B3000000: ::speed_t = 0o010015;
pub const B3500000: ::speed_t = 0o010016;
pub const B4000000: ::speed_t = 0o010017;

pub const SO_BINDTODEVICE: ::c_int = 25;
pub const SO_TIMESTAMP: ::c_int = 29;
pub const SO_MARK: ::c_int = 36;
pub const SO_RXQ_OVFL: ::c_int = 40;
pub const SO_PEEK_OFF: ::c_int = 42;
pub const SO_BUSY_POLL: ::c_int = 46;

pub const __SIZEOF_PTHREAD_RWLOCK_T: usize = 32;
pub const __SIZEOF_PTHREAD_MUTEX_T: usize = 28;

pub const O_DIRECT: ::c_int = 0x4000;
pub const O_DIRECTORY: ::c_int = 0x10000;
pub const O_NOFOLLOW: ::c_int = 0x20000;
pub const O_ASYNC: ::c_int = 0x2000;

pub const FIOCLEX: ::c_int = 0x5451;
pub const FIONBIO: ::c_int = 0x5421;

pub const RLIMIT_RSS: ::c_int = 5;
pub const RLIMIT_NOFILE: ::c_int = 7;
pub const RLIMIT_AS: ::c_int = 9;
pub const RLIMIT_NPROC: ::c_int = 6;
pub const RLIMIT_MEMLOCK: ::c_int = 8;
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

pub const O_APPEND: ::c_int = 1024;
pub const O_CREAT: ::c_int = 64;
pub const O_EXCL: ::c_int = 128;
pub const O_NOCTTY: ::c_int = 256;
pub const O_NONBLOCK: ::c_int = 2048;
pub const O_SYNC: ::c_int = 1052672;
pub const O_RSYNC: ::c_int = 1052672;
pub const O_DSYNC: ::c_int = 4096;

pub const SOCK_NONBLOCK: ::c_int = 2048;

pub const MAP_ANON: ::c_int = 0x0020;
pub const MAP_GROWSDOWN: ::c_int = 0x0100;
pub const MAP_DENYWRITE: ::c_int = 0x0800;
pub const MAP_EXECUTABLE: ::c_int = 0x01000;
pub const MAP_LOCKED: ::c_int = 0x02000;
pub const MAP_NORESERVE: ::c_int = 0x04000;
pub const MAP_POPULATE: ::c_int = 0x08000;
pub const MAP_NONBLOCK: ::c_int = 0x010000;
pub const MAP_STACK: ::c_int = 0x020000;

pub const SOCK_STREAM: ::c_int = 1;
pub const SOCK_DGRAM: ::c_int = 2;
pub const SOCK_SEQPACKET: ::c_int = 5;

pub const SOL_SOCKET: ::c_int = 1;

pub const EDEADLK: ::c_int = 35;
pub const ENAMETOOLONG: ::c_int = 36;
pub const ENOLCK: ::c_int = 37;
pub const ENOSYS: ::c_int = 38;
pub const ENOTEMPTY: ::c_int = 39;
pub const ELOOP: ::c_int = 40;
pub const ENOMSG: ::c_int = 42;
pub const EIDRM: ::c_int = 43;
pub const ECHRNG: ::c_int = 44;
pub const EL2NSYNC: ::c_int = 45;
pub const EL3HLT: ::c_int = 46;
pub const EL3RST: ::c_int = 47;
pub const ELNRNG: ::c_int = 48;
pub const EUNATCH: ::c_int = 49;
pub const ENOCSI: ::c_int = 50;
pub const EL2HLT: ::c_int = 51;
pub const EBADE: ::c_int = 52;
pub const EBADR: ::c_int = 53;
pub const EXFULL: ::c_int = 54;
pub const ENOANO: ::c_int = 55;
pub const EBADRQC: ::c_int = 56;
pub const EBADSLT: ::c_int = 57;
pub const EDEADLOCK: ::c_int = EDEADLK;
pub const EMULTIHOP: ::c_int = 72;
pub const EBADMSG: ::c_int = 74;
pub const EOVERFLOW: ::c_int = 75;
pub const ENOTUNIQ: ::c_int = 76;
pub const EBADFD: ::c_int = 77;
pub const EREMCHG: ::c_int = 78;
pub const ELIBACC: ::c_int = 79;
pub const ELIBBAD: ::c_int = 80;
pub const ELIBSCN: ::c_int = 81;
pub const ELIBMAX: ::c_int = 82;
pub const ELIBEXEC: ::c_int = 83;
pub const EILSEQ: ::c_int = 84;
pub const ERESTART: ::c_int = 85;
pub const ESTRPIPE: ::c_int = 86;
pub const EUSERS: ::c_int = 87;
pub const ENOTSOCK: ::c_int = 88;
pub const EDESTADDRREQ: ::c_int = 89;
pub const EMSGSIZE: ::c_int = 90;
pub const EPROTOTYPE: ::c_int = 91;
pub const ENOPROTOOPT: ::c_int = 92;
pub const EPROTONOSUPPORT: ::c_int = 93;
pub const ESOCKTNOSUPPORT: ::c_int = 94;
pub const EOPNOTSUPP: ::c_int = 95;
pub const ENOTSUP: ::c_int = EOPNOTSUPP;
pub const EPFNOSUPPORT: ::c_int = 96;
pub const EAFNOSUPPORT: ::c_int = 97;
pub const EADDRINUSE: ::c_int = 98;
pub const EADDRNOTAVAIL: ::c_int = 99;
pub const ENETDOWN: ::c_int = 100;
pub const ENETUNREACH: ::c_int = 101;
pub const ENETRESET: ::c_int = 102;
pub const ECONNABORTED: ::c_int = 103;
pub const ECONNRESET: ::c_int = 104;
pub const ENOBUFS: ::c_int = 105;
pub const EISCONN: ::c_int = 106;
pub const ENOTCONN: ::c_int = 107;
pub const ESHUTDOWN: ::c_int = 108;
pub const ETOOMANYREFS: ::c_int = 109;
pub const ETIMEDOUT: ::c_int = 110;
pub const ECONNREFUSED: ::c_int = 111;
pub const EHOSTDOWN: ::c_int = 112;
pub const EHOSTUNREACH: ::c_int = 113;
pub const EALREADY: ::c_int = 114;
pub const EINPROGRESS: ::c_int = 115;
pub const ESTALE: ::c_int = 116;
pub const EUCLEAN: ::c_int = 117;
pub const ENOTNAM: ::c_int = 118;
pub const ENAVAIL: ::c_int = 119;
pub const EISNAM: ::c_int = 120;
pub const EREMOTEIO: ::c_int = 121;
pub const EDQUOT: ::c_int = 122;
pub const ENOMEDIUM: ::c_int = 123;
pub const EMEDIUMTYPE: ::c_int = 124;
pub const ECANCELED: ::c_int = 125;
pub const ENOKEY: ::c_int = 126;
pub const EKEYEXPIRED: ::c_int = 127;
pub const EKEYREVOKED: ::c_int = 128;
pub const EKEYREJECTED: ::c_int = 129;
pub const EOWNERDEAD: ::c_int = 130;
pub const ENOTRECOVERABLE: ::c_int = 131;
pub const ERFKILL: ::c_int = 132;
pub const EHWPOISON: ::c_int = 133;

pub const SO_REUSEADDR: ::c_int = 2;
pub const SO_TYPE: ::c_int = 3;
pub const SO_ERROR: ::c_int = 4;
pub const SO_DONTROUTE: ::c_int = 5;
pub const SO_BROADCAST: ::c_int = 6;
pub const SO_SNDBUF: ::c_int = 7;
pub const SO_RCVBUF: ::c_int = 8;
pub const SO_KEEPALIVE: ::c_int = 9;
pub const SO_OOBINLINE: ::c_int = 10;
pub const SO_LINGER: ::c_int = 13;
pub const SO_REUSEPORT: ::c_int = 15;
pub const SO_RCVLOWAT: ::c_int = 18;
pub const SO_SNDLOWAT: ::c_int = 19;
pub const SO_RCVTIMEO: ::c_int = 20;
pub const SO_SNDTIMEO: ::c_int = 21;
pub const SO_ACCEPTCONN: ::c_int = 30;

pub const SA_ONSTACK: ::c_int = 0x08000000;
pub const SA_SIGINFO: ::c_int = 0x00000004;
pub const SA_NOCLDWAIT: ::c_int = 0x00000002;

pub const SIGCHLD: ::c_int = 17;
pub const SIGBUS: ::c_int = 7;
pub const SIGTTIN: ::c_int = 21;
pub const SIGTTOU: ::c_int = 22;
pub const SIGXCPU: ::c_int = 24;
pub const SIGXFSZ: ::c_int = 25;
pub const SIGVTALRM: ::c_int = 26;
pub const SIGPROF: ::c_int = 27;
pub const SIGWINCH: ::c_int = 28;
pub const SIGUSR1: ::c_int = 10;
pub const SIGUSR2: ::c_int = 12;
pub const SIGCONT: ::c_int = 18;
pub const SIGSTOP: ::c_int = 19;
pub const SIGTSTP: ::c_int = 20;
pub const SIGURG: ::c_int = 23;
pub const SIGIO: ::c_int = 29;
pub const SIGSYS: ::c_int = 31;
pub const SIGSTKFLT: ::c_int = 16;
pub const SIGPOLL: ::c_int = 29;
pub const SIGPWR: ::c_int = 30;
pub const SIG_SETMASK: ::c_int = 2;
pub const SIG_BLOCK: ::c_int = 0x000000;
pub const SIG_UNBLOCK: ::c_int = 0x01;

pub const EXTPROC: ::tcflag_t = 0x00010000;

pub const MAP_HUGETLB: ::c_int = 0x040000;

pub const F_GETLK: ::c_int = 12;
pub const F_GETOWN: ::c_int = 9;
pub const F_SETLK: ::c_int = 13;
pub const F_SETLKW: ::c_int = 14;
pub const F_SETOWN: ::c_int = 8;

pub const VEOF: usize = 4;
pub const VEOL: usize = 11;
pub const VEOL2: usize = 16;
pub const VMIN: usize = 6;
pub const IEXTEN: ::tcflag_t = 0x00008000;
pub const TOSTOP: ::tcflag_t = 0x00000100;
pub const FLUSHO: ::tcflag_t = 0x00001000;

pub const TCGETS: ::c_int = 0x5401;
pub const TCSETS: ::c_int = 0x5402;
pub const TCSETSW: ::c_int = 0x5403;
pub const TCSETSF: ::c_int = 0x5404;
pub const TCGETA: ::c_int = 0x5405;
pub const TCSETA: ::c_int = 0x5406;
pub const TCSETAW: ::c_int = 0x5407;
pub const TCSETAF: ::c_int = 0x5408;
pub const TCSBRK: ::c_int = 0x5409;
pub const TCXONC: ::c_int = 0x540A;
pub const TCFLSH: ::c_int = 0x540B;
pub const TIOCGSOFTCAR: ::c_int = 0x5419;
pub const TIOCSSOFTCAR: ::c_int = 0x541A;
pub const TIOCLINUX: ::c_int = 0x541C;
pub const TIOCGSERIAL: ::c_int = 0x541E;
pub const TIOCEXCL: ::c_int = 0x540C;
pub const TIOCNXCL: ::c_int = 0x540D;
pub const TIOCSCTTY: ::c_int = 0x540E;
pub const TIOCGPGRP: ::c_int = 0x540F;
pub const TIOCSPGRP: ::c_int = 0x5410;
pub const TIOCOUTQ: ::c_int = 0x5411;
pub const TIOCSTI: ::c_int = 0x5412;
pub const TIOCGWINSZ: ::c_int = 0x5413;
pub const TIOCSWINSZ: ::c_int = 0x5414;
pub const TIOCMGET: ::c_int = 0x5415;
pub const TIOCMBIS: ::c_int = 0x5416;
pub const TIOCMBIC: ::c_int = 0x5417;
pub const TIOCMSET: ::c_int = 0x5418;
pub const FIONREAD: ::c_int = 0x541B;
pub const TIOCCONS: ::c_int = 0x541D;

pub const SYS_gettid: ::c_long = 224; // Valid for arm (32-bit) and x86 (32-bit)

pub const POLLWRNORM: ::c_short = 0x100;
pub const POLLWRBAND: ::c_short = 0x200;

pub const TIOCM_LE: ::c_int = 0x001;
pub const TIOCM_DTR: ::c_int = 0x002;
pub const TIOCM_RTS: ::c_int = 0x004;
pub const TIOCM_ST: ::c_int = 0x008;
pub const TIOCM_SR: ::c_int = 0x010;
pub const TIOCM_CTS: ::c_int = 0x020;
pub const TIOCM_CAR: ::c_int = 0x040;
pub const TIOCM_RNG: ::c_int = 0x080;
pub const TIOCM_DSR: ::c_int = 0x100;
pub const TIOCM_CD: ::c_int = TIOCM_CAR;
pub const TIOCM_RI: ::c_int = TIOCM_RNG;
pub const O_TMPFILE: ::c_int = 0x400000;

pub const MAX_ADDR_LEN: usize = 7;
pub const ARPD_UPDATE: ::c_ushort = 0x01;
pub const ARPD_LOOKUP: ::c_ushort = 0x02;
pub const ARPD_FLUSH: ::c_ushort = 0x03;
pub const ATF_MAGIC: ::c_int = 0x80;

f! {
    pub fn CMSG_NXTHDR(mhdr: *const msghdr,
                       cmsg: *const cmsghdr) -> *mut cmsghdr {
        if ((*cmsg).cmsg_len as usize) < ::mem::size_of::<cmsghdr>() {
            return 0 as *mut cmsghdr;
        };
        let next = (cmsg as usize +
                    super::CMSG_ALIGN((*cmsg).cmsg_len as usize))
            as *mut cmsghdr;
        let max = (*mhdr).msg_control as usize
            + (*mhdr).msg_controllen as usize;
        if (next.offset(1)) as usize > max {
            0 as *mut cmsghdr
        } else {
            next as *mut cmsghdr
        }
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

    pub fn major(dev: ::dev_t) -> ::c_uint {
        // see
        // https://github.com/kripken/emscripten/blob/
        // master/system/include/libc/sys/sysmacros.h
        let mut major = 0;
        major |= (dev & 0x00000fff) >> 8;
        major |= (dev & 0xfffff000) >> 31 >> 1;
        major as ::c_uint
    }

    pub fn minor(dev: ::dev_t) -> ::c_uint {
        // see
        // https://github.com/kripken/emscripten/blob/
        // master/system/include/libc/sys/sysmacros.h
        let mut minor = 0;
        minor |= (dev & 0x000000ff) >> 0;
        minor |= (dev & 0xffffff00) >> 12;
        minor as ::c_uint
    }

    pub fn makedev(major: ::c_uint, minor: ::c_uint) -> ::dev_t {
        let major = major as ::dev_t;
        let minor = minor as ::dev_t;
        let mut dev = 0;
        dev |= (major & 0x00000fff) << 8;
        dev |= (major & 0xfffff000) << 31 << 1;
        dev |= (minor & 0x000000ff) << 0;
        dev |= (minor & 0xffffff00) << 12;
        dev
    }
}

extern "C" {
    pub fn getrlimit64(resource: ::c_int, rlim: *mut rlimit64) -> ::c_int;
    pub fn setrlimit64(resource: ::c_int, rlim: *const rlimit64) -> ::c_int;
    pub fn getrlimit(resource: ::c_int, rlim: *mut ::rlimit) -> ::c_int;
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

    pub fn gettimeofday(tp: *mut ::timeval, tz: *mut ::c_void) -> ::c_int;

    pub fn setpwent();
    pub fn endpwent();
    pub fn getpwent() -> *mut passwd;

    pub fn shm_open(
        name: *const c_char,
        oflag: ::c_int,
        mode: mode_t,
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
    pub fn posix_fallocate(
        fd: ::c_int,
        offset: ::off_t,
        len: ::off_t,
    ) -> ::c_int;
    pub fn pwritev(
        fd: ::c_int,
        iov: *const ::iovec,
        iovcnt: ::c_int,
        offset: ::off_t,
    ) -> ::ssize_t;
    pub fn preadv(
        fd: ::c_int,
        iov: *const ::iovec,
        iovcnt: ::c_int,
        offset: ::off_t,
    ) -> ::ssize_t;
    pub fn dup3(oldfd: ::c_int, newfd: ::c_int, flags: ::c_int) -> ::c_int;
    pub fn mkostemp(template: *mut ::c_char, flags: ::c_int) -> ::c_int;
    pub fn mkostemps(
        template: *mut ::c_char,
        suffixlen: ::c_int,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn nl_langinfo_l(item: ::nl_item, locale: ::locale_t)
        -> *mut ::c_char;
    pub fn getnameinfo(
        sa: *const ::sockaddr,
        salen: ::socklen_t,
        host: *mut ::c_char,
        hostlen: ::socklen_t,
        serv: *mut ::c_char,
        sevlen: ::socklen_t,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn getloadavg(loadavg: *mut ::c_double, nelem: ::c_int) -> ::c_int;

    // Not available now on Android
    pub fn mkfifoat(
        dirfd: ::c_int,
        pathname: *const ::c_char,
        mode: ::mode_t,
    ) -> ::c_int;
    pub fn if_nameindex() -> *mut if_nameindex;
    pub fn if_freenameindex(ptr: *mut if_nameindex);

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

    pub fn posix_madvise(
        addr: *mut ::c_void,
        len: ::size_t,
        advice: ::c_int,
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

    pub fn recvfrom(
        socket: ::c_int,
        buf: *mut ::c_void,
        len: ::size_t,
        flags: ::c_int,
        addr: *mut ::sockaddr,
        addrlen: *mut ::socklen_t,
    ) -> ::ssize_t;
    pub fn mkstemps(template: *mut ::c_char, suffixlen: ::c_int) -> ::c_int;
    pub fn nl_langinfo(item: ::nl_item) -> *mut ::c_char;

    pub fn getdomainname(name: *mut ::c_char, len: ::size_t) -> ::c_int;
    pub fn setdomainname(name: *const ::c_char, len: ::size_t) -> ::c_int;
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
    pub fn sync();
    pub fn ioctl(fd: ::c_int, request: ::c_int, ...) -> ::c_int;
    pub fn getpriority(which: ::c_int, who: ::id_t) -> ::c_int;
    pub fn setpriority(which: ::c_int, who: ::id_t, prio: ::c_int) -> ::c_int;
    pub fn pthread_create(
        native: *mut ::pthread_t,
        attr: *const ::pthread_attr_t,
        f: extern "C" fn(*mut ::c_void) -> *mut ::c_void,
        value: *mut ::c_void,
    ) -> ::c_int;
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
