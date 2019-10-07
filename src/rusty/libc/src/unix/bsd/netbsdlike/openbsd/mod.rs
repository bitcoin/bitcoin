use unix::bsd::O_SYNC;

pub type clock_t = i64;
pub type suseconds_t = ::c_long;
pub type dev_t = i32;
pub type sigset_t = ::c_uint;
pub type blksize_t = i32;
pub type fsblkcnt_t = u64;
pub type fsfilcnt_t = u64;
pub type pthread_attr_t = *mut ::c_void;
pub type pthread_mutex_t = *mut ::c_void;
pub type pthread_mutexattr_t = *mut ::c_void;
pub type pthread_cond_t = *mut ::c_void;
pub type pthread_condattr_t = *mut ::c_void;
pub type pthread_rwlock_t = *mut ::c_void;
pub type pthread_rwlockattr_t = *mut ::c_void;
pub type caddr_t = *mut ::c_char;

s! {
    pub struct glob_t {
        pub gl_pathc:   ::size_t,
        pub gl_matchc:  ::size_t,
        pub gl_offs:    ::size_t,
        pub gl_flags:   ::c_int,
        pub gl_pathv:   *mut *mut ::c_char,
        __unused1: *mut ::c_void,
        __unused2: *mut ::c_void,
        __unused3: *mut ::c_void,
        __unused4: *mut ::c_void,
        __unused5: *mut ::c_void,
        __unused6: *mut ::c_void,
        __unused7: *mut ::c_void,
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

    pub struct ufs_args {
        pub fspec: *mut ::c_char,
        pub export_info: export_args,
    }

    pub struct mfs_args {
        pub fspec: *mut ::c_char,
        pub export_info: export_args,
        // https://github.com/openbsd/src/blob/master/sys/sys/types.h#L134
        pub base: *mut ::c_char,
        pub size: ::c_ulong,
    }

    pub struct iso_args {
        pub fspec: *mut ::c_char,
        pub export_info: export_args,
        pub flags: ::c_int,
        pub sess: ::c_int,
    }

    pub struct nfs_args {
        pub version: ::c_int,
        pub addr: *mut ::sockaddr,
        pub addrlen: ::c_int,
        pub sotype: ::c_int,
        pub proto: ::c_int,
        pub fh: *mut ::c_uchar,
        pub fhsize: ::c_int,
        pub flags: ::c_int,
        pub wsize: ::c_int,
        pub rsize: ::c_int,
        pub readdirsize: ::c_int,
        pub timeo: ::c_int,
        pub retrans: ::c_int,
        pub maxgrouplist: ::c_int,
        pub readahead: ::c_int,
        pub leaseterm: ::c_int,
        pub deadthresh: ::c_int,
        pub hostname: *mut ::c_char,
        pub acregmin: ::c_int,
        pub acregmax: ::c_int,
        pub acdirmin: ::c_int,
        pub acdirmax: ::c_int,
    }

    pub struct msdosfs_args {
        pub fspec: *mut ::c_char,
        pub export_info: export_args,
        pub uid: ::uid_t,
        pub gid: ::gid_t,
        pub mask: ::mode_t,
        pub flags: ::c_int,
    }

    pub struct ntfs_args {
        pub fspec: *mut ::c_char,
        pub export_info: export_args,
        pub uid: ::uid_t,
        pub gid: ::gid_t,
        pub mode: ::mode_t,
        pub flag: ::c_ulong,
    }

    pub struct udf_args {
        pub fspec: *mut ::c_char,
        pub lastblock: u32,
    }

    pub struct tmpfs_args {
        pub ta_version: ::c_int,
        pub ta_nodes_max: ::ino_t,
        pub ta_size_max: ::off_t,
        pub ta_root_uid: ::uid_t,
        pub ta_root_gid: ::gid_t,
        pub ta_root_mode: ::mode_t,
    }

    pub struct fusefs_args {
        pub name: *mut ::c_char,
        pub fd: ::c_int,
        pub max_read: ::c_int,
        pub allow_other: ::c_int,
    }

    pub struct xucred {
        pub cr_uid: ::uid_t,
        pub cr_gid: ::gid_t,
        pub cr_ngroups: ::c_short,
        //https://github.com/openbsd/src/blob/master/sys/sys/syslimits.h#L44
        pub cr_groups: [::gid_t; 16],
    }

    pub struct export_args {
        pub ex_flags: ::c_int,
        pub ex_root: ::uid_t,
        pub ex_anon: xucred,
        pub ex_addr: *mut ::sockaddr,
        pub ex_addrlen: ::c_int,
        pub ex_mask: *mut ::sockaddr,
        pub ex_masklen: ::c_int,
    }

    pub struct ip_mreq {
        pub imr_multiaddr: in_addr,
        pub imr_interface: in_addr,
    }

    pub struct in_addr {
        pub s_addr: ::in_addr_t,
    }

    pub struct sockaddr_in {
        pub sin_len: u8,
        pub sin_family: ::sa_family_t,
        pub sin_port: ::in_port_t,
        pub sin_addr: ::in_addr,
        pub sin_zero: [i8; 8],
    }

    pub struct kevent {
        pub ident: ::uintptr_t,
        pub filter: ::c_short,
        pub flags: ::c_ushort,
        pub fflags: ::c_uint,
        pub data: i64,
        pub udata: *mut ::c_void,
    }

    pub struct stat {
        pub st_mode: ::mode_t,
        pub st_dev: ::dev_t,
        pub st_ino: ::ino_t,
        pub st_nlink: ::nlink_t,
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
        pub st_blocks: ::blkcnt_t,
        pub st_blksize: ::blksize_t,
        pub st_flags: u32,
        pub st_gen: u32,
        pub st_birthtime: ::time_t,
        pub st_birthtime_nsec: ::c_long,
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

    pub struct addrinfo {
        pub ai_flags: ::c_int,
        pub ai_family: ::c_int,
        pub ai_socktype: ::c_int,
        pub ai_protocol: ::c_int,
        pub ai_addrlen: ::socklen_t,
        pub ai_addr: *mut ::sockaddr,
        pub ai_canonname: *mut ::c_char,
        pub ai_next: *mut ::addrinfo,
    }

    pub struct Dl_info {
        pub dli_fname: *const ::c_char,
        pub dli_fbase: *mut ::c_void,
        pub dli_sname: *const ::c_char,
        pub dli_saddr: *mut ::c_void,
    }

    pub struct if_data {
        pub ifi_type: ::c_uchar,
        pub ifi_addrlen: ::c_uchar,
        pub ifi_hdrlen: ::c_uchar,
        pub ifi_link_state: ::c_uchar,
        pub ifi_mtu: u32,
        pub ifi_metric: u32,
        pub ifi_rdomain: u32,
        pub ifi_baudrate: u64,
        pub ifi_ipackets: u64,
        pub ifi_ierrors: u64,
        pub ifi_opackets: u64,
        pub ifi_oerrors: u64,
        pub ifi_collisions: u64,
        pub ifi_ibytes: u64,
        pub ifi_obytes: u64,
        pub ifi_imcasts: u64,
        pub ifi_omcasts: u64,
        pub ifi_iqdrops: u64,
        pub ifi_oqdrops: u64,
        pub ifi_noproto: u64,
        pub ifi_capabilities: u32,
        pub ifi_lastchange: ::timeval,
    }

    pub struct if_msghdr {
        pub ifm_msglen: ::c_ushort,
        pub ifm_version: ::c_uchar,
        pub ifm_type: ::c_uchar,
        pub ifm_hdrlen: ::c_ushort,
        pub ifm_index: ::c_ushort,
        pub ifm_tableid: ::c_ushort,
        pub ifm_pad1: ::c_uchar,
        pub ifm_pad2: ::c_uchar,
        pub ifm_addrs: ::c_int,
        pub ifm_flags: ::c_int,
        pub ifm_xflags: ::c_int,
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
        pub sdl_data: [::c_char; 24],
    }

    pub struct sockpeercred {
        pub uid: ::uid_t,
        pub gid: ::gid_t,
        pub pid: ::pid_t,
    }

    pub struct arphdr {
        pub ar_hrd: u16,
        pub ar_pro: u16,
        pub ar_hln: u8,
        pub ar_pln: u8,
        pub ar_op: u16,
    }
}

impl siginfo_t {
    pub unsafe fn si_value(&self) -> ::sigval {
        #[repr(C)]
        struct siginfo_timer {
            _si_signo: ::c_int,
            _si_errno: ::c_int,
            _si_code: ::c_int,
            _pid: ::pid_t,
            _uid: ::uid_t,
            value: ::sigval,
        }
        (*(self as *const siginfo_t as *const siginfo_timer)).value
    }
}

s_no_extra_traits! {
    pub struct dirent {
        pub d_fileno: ::ino_t,
        pub d_off: ::off_t,
        pub d_reclen: u16,
        pub d_type: u8,
        pub d_namlen: u8,
        __d_padding: [u8; 4],
        pub d_name: [::c_char; 256],
    }

    pub struct sockaddr_storage {
        pub ss_len: u8,
        pub ss_family: ::sa_family_t,
        __ss_pad1: [u8; 6],
        __ss_pad2: i64,
        __ss_pad3: [u8; 240],
    }

    pub struct siginfo_t {
        pub si_signo: ::c_int,
        pub si_code: ::c_int,
        pub si_errno: ::c_int,
        pub si_addr: *mut ::c_char,
        #[cfg(target_pointer_width = "32")]
        __pad: [u8; 112],
        #[cfg(target_pointer_width = "64")]
        __pad: [u8; 108],
    }

    pub struct lastlog {
        ll_time: ::time_t,
        ll_line: [::c_char; UT_LINESIZE],
        ll_host: [::c_char; UT_HOSTSIZE],
    }

    pub struct utmp {
        pub ut_line: [::c_char; UT_LINESIZE],
        pub ut_name: [::c_char; UT_NAMESIZE],
        pub ut_host: [::c_char; UT_HOSTSIZE],
        pub ut_time: ::time_t,
    }

    pub union mount_info {
        pub ufs_args: ufs_args,
        pub mfs_args: mfs_args,
        pub nfs_args: nfs_args,
        pub iso_args: iso_args,
        pub msdosfs_args: msdosfs_args,
        pub ntfs_args: ntfs_args,
        pub tmpfs_args: tmpfs_args,
        align: [::c_char; 160],
    }

}

cfg_if! {
    if #[cfg(feature = "extra_traits")] {
        impl PartialEq for dirent {
            fn eq(&self, other: &dirent) -> bool {
                self.d_fileno == other.d_fileno
                    && self.d_off == other.d_off
                    && self.d_reclen == other.d_reclen
                    && self.d_type == other.d_type
                    && self.d_namlen == other.d_namlen
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
                    .field("d_off", &self.d_off)
                    .field("d_reclen", &self.d_reclen)
                    .field("d_type", &self.d_type)
                    .field("d_namlen", &self.d_namlen)
                // FIXME: .field("d_name", &self.d_name)
                    .finish()
            }
        }

        impl ::hash::Hash for dirent {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.d_fileno.hash(state);
                self.d_off.hash(state);
                self.d_reclen.hash(state);
                self.d_type.hash(state);
                self.d_namlen.hash(state);
                self.d_name.hash(state);
            }
        }

        impl PartialEq for sockaddr_storage {
            fn eq(&self, other: &sockaddr_storage) -> bool {
                self.ss_len == other.ss_len
                    && self.ss_family == other.ss_family
            }
        }

        impl Eq for sockaddr_storage {}

        impl ::fmt::Debug for sockaddr_storage {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("sockaddr_storage")
                    .field("ss_len", &self.ss_len)
                    .field("ss_family", &self.ss_family)
                    .finish()
            }
        }

        impl ::hash::Hash for sockaddr_storage {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.ss_len.hash(state);
                self.ss_family.hash(state);
            }
        }

        impl PartialEq for siginfo_t {
            fn eq(&self, other: &siginfo_t) -> bool {
                self.si_signo == other.si_signo
                    && self.si_code == other.si_code
                    && self.si_errno == other.si_errno
                    && self.si_addr == other.si_addr
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
                    .finish()
            }
        }

        impl ::hash::Hash for siginfo_t {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.si_signo.hash(state);
                self.si_code.hash(state);
                self.si_errno.hash(state);
                self.si_addr.hash(state);
            }
        }

        impl PartialEq for lastlog {
            fn eq(&self, other: &lastlog) -> bool {
                self.ll_time == other.ll_time
                    && self
                    .ll_line
                    .iter()
                    .zip(other.ll_line.iter())
                    .all(|(a,b)| a == b)
                    && self
                    .ll_host
                    .iter()
                    .zip(other.ll_host.iter())
                    .all(|(a,b)| a == b)
            }
        }

        impl Eq for lastlog {}

        impl ::fmt::Debug for lastlog {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("lastlog")
                    .field("ll_time", &self.ll_time)
                // FIXME: .field("ll_line", &self.ll_line)
                // FIXME: .field("ll_host", &self.ll_host)
                    .finish()
            }
        }

        impl ::hash::Hash for lastlog {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.ll_time.hash(state);
                self.ll_line.hash(state);
                self.ll_host.hash(state);
            }
        }

        impl PartialEq for utmp {
            fn eq(&self, other: &utmp) -> bool {
                self.ut_time == other.ut_time
                    && self
                    .ut_line
                    .iter()
                    .zip(other.ut_line.iter())
                    .all(|(a,b)| a == b)
                    && self
                    .ut_name
                    .iter()
                    .zip(other.ut_name.iter())
                    .all(|(a,b)| a == b)
                    && self
                    .ut_host
                    .iter()
                    .zip(other.ut_host.iter())
                    .all(|(a,b)| a == b)
            }
        }

        impl Eq for utmp {}

        impl ::fmt::Debug for utmp {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("utmp")
                // FIXME: .field("ut_line", &self.ut_line)
                // FIXME: .field("ut_name", &self.ut_name)
                // FIXME: .field("ut_host", &self.ut_host)
                    .field("ut_time", &self.ut_time)
                    .finish()
            }
        }

        impl ::hash::Hash for utmp {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.ut_line.hash(state);
                self.ut_name.hash(state);
                self.ut_host.hash(state);
                self.ut_time.hash(state);
            }
        }

        impl PartialEq for mount_info {
            fn eq(&self, other: &mount_info) -> bool {
                unsafe {
                    self.align
                        .iter()
                        .zip(other.align.iter())
                        .all(|(a,b)| a == b)
                }
            }
        }

        impl Eq for mount_info { }

        impl ::fmt::Debug for mount_info {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("mount_info")
                // FIXME: .field("align", &self.align)
                    .finish()
            }
        }

        impl ::hash::Hash for mount_info {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                unsafe { self.align.hash(state) };
            }
        }
    }
}

cfg_if! {
    if #[cfg(libc_union)] {
        s_no_extra_traits! {
            // This type uses the union mount_info:
            pub struct statfs {
                pub f_flags: u32,
                pub f_bsize: u32,
                pub f_iosize: u32,
                pub f_blocks: u64,
                pub f_bfree: u64,
                pub f_bavail: i64,
                pub f_files: u64,
                pub f_ffree: u64,
                pub f_favail: i64,
                pub f_syncwrites: u64,
                pub f_syncreads: u64,
                pub f_asyncwrites: u64,
                pub f_asyncreads: u64,
                pub f_fsid: ::fsid_t,
                pub f_namemax: u32,
                pub f_owner: ::uid_t,
                pub f_ctime: u64,
                pub f_fstypename: [::c_char; 16],
                pub f_mntonname: [::c_char; 90],
                pub f_mntfromname: [::c_char; 90],
                pub f_mntfromspec: [::c_char; 90],
                pub mount_info: mount_info,
            }
        }

        cfg_if! {
            if #[cfg(feature = "extra_traits")] {
                impl PartialEq for statfs {
                    fn eq(&self, other: &statfs) -> bool {
                        self.f_flags == other.f_flags
                            && self.f_bsize == other.f_bsize
                            && self.f_iosize == other.f_iosize
                            && self.f_blocks == other.f_blocks
                            && self.f_bfree == other.f_bfree
                            && self.f_bavail == other.f_bavail
                            && self.f_files == other.f_files
                            && self.f_ffree == other.f_ffree
                            && self.f_favail == other.f_favail
                            && self.f_syncwrites == other.f_syncwrites
                            && self.f_syncreads == other.f_syncreads
                            && self.f_asyncwrites == other.f_asyncwrites
                            && self.f_asyncreads == other.f_asyncreads
                            && self.f_fsid == other.f_fsid
                            && self.f_namemax == other.f_namemax
                            && self.f_owner == other.f_owner
                            && self.f_ctime == other.f_ctime
                            && self.f_fstypename
                            .iter()
                            .zip(other.f_fstypename.iter())
                            .all(|(a,b)| a == b)
                            && self.f_mntonname
                            .iter()
                            .zip(other.f_mntonname.iter())
                            .all(|(a,b)| a == b)
                            && self.f_mntfromname
                            .iter()
                            .zip(other.f_mntfromname.iter())
                            .all(|(a,b)| a == b)
                            && self.f_mntfromspec
                            .iter()
                            .zip(other.f_mntfromspec.iter())
                            .all(|(a,b)| a == b)
                            && self.mount_info == other.mount_info
                    }
                }

                impl Eq for statfs { }

                impl ::fmt::Debug for statfs {
                    fn fmt(&self, f: &mut ::fmt::Formatter)
                           -> ::fmt::Result {
                        f.debug_struct("statfs")
                            .field("f_flags", &self.f_flags)
                            .field("f_bsize", &self.f_bsize)
                            .field("f_iosize", &self.f_iosize)
                            .field("f_blocks", &self.f_blocks)
                            .field("f_bfree", &self.f_bfree)
                            .field("f_bavail", &self.f_bavail)
                            .field("f_files", &self.f_files)
                            .field("f_ffree", &self.f_ffree)
                            .field("f_favail", &self.f_favail)
                            .field("f_syncwrites", &self.f_syncwrites)
                            .field("f_syncreads", &self.f_syncreads)
                            .field("f_asyncwrites", &self.f_asyncwrites)
                            .field("f_asyncreads", &self.f_asyncreads)
                            .field("f_fsid", &self.f_fsid)
                            .field("f_namemax", &self.f_namemax)
                            .field("f_owner", &self.f_owner)
                            .field("f_ctime", &self.f_ctime)
                        // FIXME: .field("f_fstypename", &self.f_fstypename)
                        // FIXME: .field("f_mntonname", &self.f_mntonname)
                        // FIXME: .field("f_mntfromname", &self.f_mntfromname)
                        // FIXME: .field("f_mntfromspec", &self.f_mntfromspec)
                            .field("mount_info", &self.mount_info)
                            .finish()
                    }
                }

                impl ::hash::Hash for statfs {
                    fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                        self.f_flags.hash(state);
                        self.f_bsize.hash(state);
                        self.f_iosize.hash(state);
                        self.f_blocks.hash(state);
                        self.f_bfree.hash(state);
                        self.f_bavail.hash(state);
                        self.f_files.hash(state);
                        self.f_ffree.hash(state);
                        self.f_favail.hash(state);
                        self.f_syncwrites.hash(state);
                        self.f_syncreads.hash(state);
                        self.f_asyncwrites.hash(state);
                        self.f_asyncreads.hash(state);
                        self.f_fsid.hash(state);
                        self.f_namemax.hash(state);
                        self.f_owner.hash(state);
                        self.f_ctime.hash(state);
                        self.f_fstypename.hash(state);
                        self.f_mntonname.hash(state);
                        self.f_mntfromname.hash(state);
                        self.f_mntfromspec.hash(state);
                        self.mount_info.hash(state);
                    }
                }
            }
        }
    }
}

pub const UT_NAMESIZE: usize = 32;
pub const UT_LINESIZE: usize = 8;
pub const UT_HOSTSIZE: usize = 256;

pub const O_CLOEXEC: ::c_int = 0x10000;
pub const O_DIRECTORY: ::c_int = 0x20000;
pub const O_RSYNC: ::c_int = O_SYNC;

pub const MS_SYNC: ::c_int = 0x0002;
pub const MS_INVALIDATE: ::c_int = 0x0004;

pub const POLLNORM: ::c_short = ::POLLRDNORM;

pub const ENOATTR: ::c_int = 83;
pub const EILSEQ: ::c_int = 84;
pub const EOVERFLOW: ::c_int = 87;
pub const ECANCELED: ::c_int = 88;
pub const EIDRM: ::c_int = 89;
pub const ENOMSG: ::c_int = 90;
pub const ENOTSUP: ::c_int = 91;
pub const EBADMSG: ::c_int = 92;
pub const ENOTRECOVERABLE: ::c_int = 93;
pub const EOWNERDEAD: ::c_int = 94;
pub const EPROTO: ::c_int = 95;
pub const ELAST: ::c_int = 95;

pub const F_DUPFD_CLOEXEC: ::c_int = 10;

pub const UTIME_OMIT: c_long = -1;
pub const UTIME_NOW: c_long = -2;

pub const AT_FDCWD: ::c_int = -100;
pub const AT_EACCESS: ::c_int = 0x01;
pub const AT_SYMLINK_NOFOLLOW: ::c_int = 0x02;
pub const AT_SYMLINK_FOLLOW: ::c_int = 0x04;
pub const AT_REMOVEDIR: ::c_int = 0x08;

#[deprecated(since = "0.2.64", note = "Not stable across OS versions")]
pub const RLIM_NLIMITS: ::c_int = 9;

pub const SO_TIMESTAMP: ::c_int = 0x0800;
pub const SO_SNDTIMEO: ::c_int = 0x1005;
pub const SO_RCVTIMEO: ::c_int = 0x1006;
pub const SO_BINDANY: ::c_int = 0x1000;
pub const SO_NETPROC: ::c_int = 0x1020;
pub const SO_RTABLE: ::c_int = 0x1021;
pub const SO_PEERCRED: ::c_int = 0x1022;
pub const SO_SPLICE: ::c_int = 0x1023;

// sys/netinet/in.h
// Protocols (RFC 1700)
// NOTE: These are in addition to the constants defined in src/unix/mod.rs

// IPPROTO_IP defined in src/unix/mod.rs
/// Hop-by-hop option header
pub const IPPROTO_HOPOPTS: ::c_int = 0;
// IPPROTO_ICMP defined in src/unix/mod.rs
/// group mgmt protocol
pub const IPPROTO_IGMP: ::c_int = 2;
/// gateway^2 (deprecated)
pub const IPPROTO_GGP: ::c_int = 3;
/// for compatibility
pub const IPPROTO_IPIP: ::c_int = 4;
// IPPROTO_TCP defined in src/unix/mod.rs
/// exterior gateway protocol
pub const IPPROTO_EGP: ::c_int = 8;
/// pup
pub const IPPROTO_PUP: ::c_int = 12;
// IPPROTO_UDP defined in src/unix/mod.rs
/// xns idp
pub const IPPROTO_IDP: ::c_int = 22;
/// tp-4 w/ class negotiation
pub const IPPROTO_TP: ::c_int = 29;
// IPPROTO_IPV6 defined in src/unix/mod.rs
/// IP6 routing header
pub const IPPROTO_ROUTING: ::c_int = 43;
/// IP6 fragmentation header
pub const IPPROTO_FRAGMENT: ::c_int = 44;
/// resource reservation
pub const IPPROTO_RSVP: ::c_int = 46;
/// General Routing Encap.
pub const IPPROTO_GRE: ::c_int = 47;
/// IP6 Encap Sec. Payload
pub const IPPROTO_ESP: ::c_int = 50;
/// IP6 Auth Header
pub const IPPROTO_AH: ::c_int = 51;
/// IP Mobility RFC 2004
pub const IPPROTO_MOBILE: ::c_int = 55;
// IPPROTO_ICMPV6 defined in src/unix/mod.rs
/// IP6 no next header
pub const IPPROTO_NONE: ::c_int = 59;
/// IP6 destination option
pub const IPPROTO_DSTOPTS: ::c_int = 60;
/// ISO cnlp
pub const IPPROTO_EON: ::c_int = 80;
/// Ethernet-in-IP
pub const IPPROTO_ETHERIP: ::c_int = 97;
/// encapsulation header
pub const IPPROTO_ENCAP: ::c_int = 98;
/// Protocol indep. multicast
pub const IPPROTO_PIM: ::c_int = 103;
/// IP Payload Comp. Protocol
pub const IPPROTO_IPCOMP: ::c_int = 108;
/// CARP
pub const IPPROTO_CARP: ::c_int = 112;
/// unicast MPLS packet
pub const IPPROTO_MPLS: ::c_int = 137;
/// PFSYNC
pub const IPPROTO_PFSYNC: ::c_int = 240;
pub const IPPROTO_MAX: ::c_int = 256;

// Only used internally, so it can be outside the range of valid IP protocols
pub const IPPROTO_DIVERT: ::c_int = 258;

pub const IP_RECVDSTADDR: ::c_int = 7;
pub const IP_SENDSRCADDR: ::c_int = IP_RECVDSTADDR;
pub const IP_RECVIF: ::c_int = 30;

// sys/netinet/in.h
pub const TCP_MD5SIG: ::c_int = 0x04;
pub const TCP_NOPUSH: ::c_int = 0x10;

pub const AF_ECMA: ::c_int = 8;
pub const AF_ROUTE: ::c_int = 17;
pub const AF_ENCAP: ::c_int = 28;
pub const AF_SIP: ::c_int = 29;
pub const AF_KEY: ::c_int = 30;
pub const pseudo_AF_HDRCMPLT: ::c_int = 31;
pub const AF_BLUETOOTH: ::c_int = 32;
pub const AF_MPLS: ::c_int = 33;
pub const pseudo_AF_PFLOW: ::c_int = 34;
pub const pseudo_AF_PIPEX: ::c_int = 35;
pub const NET_RT_DUMP: ::c_int = 1;
pub const NET_RT_FLAGS: ::c_int = 2;
pub const NET_RT_IFLIST: ::c_int = 3;
pub const NET_RT_STATS: ::c_int = 4;
pub const NET_RT_TABLE: ::c_int = 5;
pub const NET_RT_IFNAMES: ::c_int = 6;
#[doc(hidden)]
pub const NET_RT_MAXID: ::c_int = 7;

pub const IPV6_JOIN_GROUP: ::c_int = 12;
pub const IPV6_LEAVE_GROUP: ::c_int = 13;

pub const PF_ROUTE: ::c_int = AF_ROUTE;
pub const PF_ECMA: ::c_int = AF_ECMA;
pub const PF_ENCAP: ::c_int = AF_ENCAP;
pub const PF_SIP: ::c_int = AF_SIP;
pub const PF_KEY: ::c_int = AF_KEY;
pub const PF_BPF: ::c_int = pseudo_AF_HDRCMPLT;
pub const PF_BLUETOOTH: ::c_int = AF_BLUETOOTH;
pub const PF_MPLS: ::c_int = AF_MPLS;
pub const PF_PFLOW: ::c_int = pseudo_AF_PFLOW;
pub const PF_PIPEX: ::c_int = pseudo_AF_PIPEX;

pub const SCM_TIMESTAMP: ::c_int = 0x04;

pub const O_DSYNC: ::c_int = 128;

pub const MAP_RENAME: ::c_int = 0x0000;
pub const MAP_NORESERVE: ::c_int = 0x0000;
pub const MAP_HASSEMAPHORE: ::c_int = 0x0000;

pub const EIPSEC: ::c_int = 82;
pub const ENOMEDIUM: ::c_int = 85;
pub const EMEDIUMTYPE: ::c_int = 86;

pub const EAI_BADFLAGS: ::c_int = -1;
pub const EAI_NONAME: ::c_int = -2;
pub const EAI_AGAIN: ::c_int = -3;
pub const EAI_FAIL: ::c_int = -4;
pub const EAI_NODATA: ::c_int = -5;
pub const EAI_FAMILY: ::c_int = -6;
pub const EAI_SOCKTYPE: ::c_int = -7;
pub const EAI_SERVICE: ::c_int = -8;
pub const EAI_MEMORY: ::c_int = -10;
pub const EAI_SYSTEM: ::c_int = -11;
pub const EAI_OVERFLOW: ::c_int = -14;

pub const RUSAGE_THREAD: ::c_int = 1;

pub const MAP_COPY: ::c_int = 0x0002;
pub const MAP_NOEXTEND: ::c_int = 0x0000;

pub const _PC_LINK_MAX: ::c_int = 1;
pub const _PC_MAX_CANON: ::c_int = 2;
pub const _PC_MAX_INPUT: ::c_int = 3;
pub const _PC_NAME_MAX: ::c_int = 4;
pub const _PC_PATH_MAX: ::c_int = 5;
pub const _PC_PIPE_BUF: ::c_int = 6;
pub const _PC_CHOWN_RESTRICTED: ::c_int = 7;
pub const _PC_NO_TRUNC: ::c_int = 8;
pub const _PC_VDISABLE: ::c_int = 9;
pub const _PC_2_SYMLINKS: ::c_int = 10;
pub const _PC_ALLOC_SIZE_MIN: ::c_int = 11;
pub const _PC_ASYNC_IO: ::c_int = 12;
pub const _PC_FILESIZEBITS: ::c_int = 13;
pub const _PC_PRIO_IO: ::c_int = 14;
pub const _PC_REC_INCR_XFER_SIZE: ::c_int = 15;
pub const _PC_REC_MAX_XFER_SIZE: ::c_int = 16;
pub const _PC_REC_MIN_XFER_SIZE: ::c_int = 17;
pub const _PC_REC_XFER_ALIGN: ::c_int = 18;
pub const _PC_SYMLINK_MAX: ::c_int = 19;
pub const _PC_SYNC_IO: ::c_int = 20;
pub const _PC_TIMESTAMP_RESOLUTION: ::c_int = 21;

pub const _SC_CLK_TCK: ::c_int = 3;
pub const _SC_SEM_NSEMS_MAX: ::c_int = 31;
pub const _SC_SEM_VALUE_MAX: ::c_int = 32;
pub const _SC_HOST_NAME_MAX: ::c_int = 33;
pub const _SC_MONOTONIC_CLOCK: ::c_int = 34;
pub const _SC_2_PBS: ::c_int = 35;
pub const _SC_2_PBS_ACCOUNTING: ::c_int = 36;
pub const _SC_2_PBS_CHECKPOINT: ::c_int = 37;
pub const _SC_2_PBS_LOCATE: ::c_int = 38;
pub const _SC_2_PBS_MESSAGE: ::c_int = 39;
pub const _SC_2_PBS_TRACK: ::c_int = 40;
pub const _SC_ADVISORY_INFO: ::c_int = 41;
pub const _SC_AIO_LISTIO_MAX: ::c_int = 42;
pub const _SC_AIO_MAX: ::c_int = 43;
pub const _SC_AIO_PRIO_DELTA_MAX: ::c_int = 44;
pub const _SC_ASYNCHRONOUS_IO: ::c_int = 45;
pub const _SC_ATEXIT_MAX: ::c_int = 46;
pub const _SC_BARRIERS: ::c_int = 47;
pub const _SC_CLOCK_SELECTION: ::c_int = 48;
pub const _SC_CPUTIME: ::c_int = 49;
pub const _SC_DELAYTIMER_MAX: ::c_int = 50;
pub const _SC_IOV_MAX: ::c_int = 51;
pub const _SC_IPV6: ::c_int = 52;
pub const _SC_MAPPED_FILES: ::c_int = 53;
pub const _SC_MEMLOCK: ::c_int = 54;
pub const _SC_MEMLOCK_RANGE: ::c_int = 55;
pub const _SC_MEMORY_PROTECTION: ::c_int = 56;
pub const _SC_MESSAGE_PASSING: ::c_int = 57;
pub const _SC_MQ_OPEN_MAX: ::c_int = 58;
pub const _SC_MQ_PRIO_MAX: ::c_int = 59;
pub const _SC_PRIORITIZED_IO: ::c_int = 60;
pub const _SC_PRIORITY_SCHEDULING: ::c_int = 61;
pub const _SC_RAW_SOCKETS: ::c_int = 62;
pub const _SC_READER_WRITER_LOCKS: ::c_int = 63;
pub const _SC_REALTIME_SIGNALS: ::c_int = 64;
pub const _SC_REGEXP: ::c_int = 65;
pub const _SC_RTSIG_MAX: ::c_int = 66;
pub const _SC_SEMAPHORES: ::c_int = 67;
pub const _SC_SHARED_MEMORY_OBJECTS: ::c_int = 68;
pub const _SC_SHELL: ::c_int = 69;
pub const _SC_SIGQUEUE_MAX: ::c_int = 70;
pub const _SC_SPAWN: ::c_int = 71;
pub const _SC_SPIN_LOCKS: ::c_int = 72;
pub const _SC_SPORADIC_SERVER: ::c_int = 73;
pub const _SC_SS_REPL_MAX: ::c_int = 74;
pub const _SC_SYNCHRONIZED_IO: ::c_int = 75;
pub const _SC_SYMLOOP_MAX: ::c_int = 76;
pub const _SC_THREAD_ATTR_STACKADDR: ::c_int = 77;
pub const _SC_THREAD_ATTR_STACKSIZE: ::c_int = 78;
pub const _SC_THREAD_CPUTIME: ::c_int = 79;
pub const _SC_THREAD_DESTRUCTOR_ITERATIONS: ::c_int = 80;
pub const _SC_THREAD_KEYS_MAX: ::c_int = 81;
pub const _SC_THREAD_PRIO_INHERIT: ::c_int = 82;
pub const _SC_THREAD_PRIO_PROTECT: ::c_int = 83;
pub const _SC_THREAD_PRIORITY_SCHEDULING: ::c_int = 84;
pub const _SC_THREAD_PROCESS_SHARED: ::c_int = 85;
pub const _SC_THREAD_ROBUST_PRIO_INHERIT: ::c_int = 86;
pub const _SC_THREAD_ROBUST_PRIO_PROTECT: ::c_int = 87;
pub const _SC_THREAD_SPORADIC_SERVER: ::c_int = 88;
pub const _SC_THREAD_STACK_MIN: ::c_int = 89;
pub const _SC_THREAD_THREADS_MAX: ::c_int = 90;
pub const _SC_THREADS: ::c_int = 91;
pub const _SC_TIMEOUTS: ::c_int = 92;
pub const _SC_TIMER_MAX: ::c_int = 93;
pub const _SC_TIMERS: ::c_int = 94;
pub const _SC_TRACE: ::c_int = 95;
pub const _SC_TRACE_EVENT_FILTER: ::c_int = 96;
pub const _SC_TRACE_EVENT_NAME_MAX: ::c_int = 97;
pub const _SC_TRACE_INHERIT: ::c_int = 98;
pub const _SC_TRACE_LOG: ::c_int = 99;
pub const _SC_GETGR_R_SIZE_MAX: ::c_int = 100;
pub const _SC_GETPW_R_SIZE_MAX: ::c_int = 101;
pub const _SC_LOGIN_NAME_MAX: ::c_int = 102;
pub const _SC_THREAD_SAFE_FUNCTIONS: ::c_int = 103;
pub const _SC_TRACE_NAME_MAX: ::c_int = 104;
pub const _SC_TRACE_SYS_MAX: ::c_int = 105;
pub const _SC_TRACE_USER_EVENT_MAX: ::c_int = 106;
pub const _SC_TTY_NAME_MAX: ::c_int = 107;
pub const _SC_TYPED_MEMORY_OBJECTS: ::c_int = 108;
pub const _SC_V6_ILP32_OFF32: ::c_int = 109;
pub const _SC_V6_ILP32_OFFBIG: ::c_int = 110;
pub const _SC_V6_LP64_OFF64: ::c_int = 111;
pub const _SC_V6_LPBIG_OFFBIG: ::c_int = 112;
pub const _SC_V7_ILP32_OFF32: ::c_int = 113;
pub const _SC_V7_ILP32_OFFBIG: ::c_int = 114;
pub const _SC_V7_LP64_OFF64: ::c_int = 115;
pub const _SC_V7_LPBIG_OFFBIG: ::c_int = 116;
pub const _SC_XOPEN_CRYPT: ::c_int = 117;
pub const _SC_XOPEN_ENH_I18N: ::c_int = 118;
pub const _SC_XOPEN_LEGACY: ::c_int = 119;
pub const _SC_XOPEN_REALTIME: ::c_int = 120;
pub const _SC_XOPEN_REALTIME_THREADS: ::c_int = 121;
pub const _SC_XOPEN_STREAMS: ::c_int = 122;
pub const _SC_XOPEN_UNIX: ::c_int = 123;
pub const _SC_XOPEN_UUCP: ::c_int = 124;
pub const _SC_XOPEN_VERSION: ::c_int = 125;
pub const _SC_PHYS_PAGES: ::c_int = 500;
pub const _SC_AVPHYS_PAGES: ::c_int = 501;
pub const _SC_NPROCESSORS_CONF: ::c_int = 502;
pub const _SC_NPROCESSORS_ONLN: ::c_int = 503;

pub const FD_SETSIZE: usize = 1024;

pub const ST_NOSUID: ::c_ulong = 2;

pub const PTHREAD_MUTEX_INITIALIZER: pthread_mutex_t = 0 as *mut _;
pub const PTHREAD_COND_INITIALIZER: pthread_cond_t = 0 as *mut _;
pub const PTHREAD_RWLOCK_INITIALIZER: pthread_rwlock_t = 0 as *mut _;

pub const PTHREAD_MUTEX_ERRORCHECK: ::c_int = 1;
pub const PTHREAD_MUTEX_RECURSIVE: ::c_int = 2;
pub const PTHREAD_MUTEX_NORMAL: ::c_int = 3;
pub const PTHREAD_MUTEX_STRICT_NP: ::c_int = 4;
pub const PTHREAD_MUTEX_DEFAULT: ::c_int = PTHREAD_MUTEX_STRICT_NP;

pub const EVFILT_AIO: i16 = -3;
pub const EVFILT_PROC: i16 = -5;
pub const EVFILT_READ: i16 = -1;
pub const EVFILT_SIGNAL: i16 = -6;
pub const EVFILT_TIMER: i16 = -7;
pub const EVFILT_VNODE: i16 = -4;
pub const EVFILT_WRITE: i16 = -2;

pub const EV_ADD: u16 = 0x1;
pub const EV_DELETE: u16 = 0x2;
pub const EV_ENABLE: u16 = 0x4;
pub const EV_DISABLE: u16 = 0x8;
pub const EV_ONESHOT: u16 = 0x10;
pub const EV_CLEAR: u16 = 0x20;
pub const EV_RECEIPT: u16 = 0x40;
pub const EV_DISPATCH: u16 = 0x80;
pub const EV_FLAG1: u16 = 0x2000;
pub const EV_ERROR: u16 = 0x4000;
pub const EV_EOF: u16 = 0x8000;
pub const EV_SYSFLAGS: u16 = 0xf000;

pub const NOTE_LOWAT: u32 = 0x00000001;
pub const NOTE_EOF: u32 = 0x00000002;
pub const NOTE_DELETE: u32 = 0x00000001;
pub const NOTE_WRITE: u32 = 0x00000002;
pub const NOTE_EXTEND: u32 = 0x00000004;
pub const NOTE_ATTRIB: u32 = 0x00000008;
pub const NOTE_LINK: u32 = 0x00000010;
pub const NOTE_RENAME: u32 = 0x00000020;
pub const NOTE_REVOKE: u32 = 0x00000040;
pub const NOTE_TRUNCATE: u32 = 0x00000080;
pub const NOTE_EXIT: u32 = 0x80000000;
pub const NOTE_FORK: u32 = 0x40000000;
pub const NOTE_EXEC: u32 = 0x20000000;
pub const NOTE_PDATAMASK: u32 = 0x000fffff;
pub const NOTE_PCTRLMASK: u32 = 0xf0000000;
pub const NOTE_TRACK: u32 = 0x00000001;
pub const NOTE_TRACKERR: u32 = 0x00000002;
pub const NOTE_CHILD: u32 = 0x00000004;

pub const TMP_MAX: ::c_uint = 0x7fffffff;

pub const NI_MAXHOST: ::size_t = 256;

pub const RTLD_LOCAL: ::c_int = 0;

pub const CTL_MAXNAME: ::c_int = 12;

pub const CTLTYPE_NODE: ::c_int = 1;
pub const CTLTYPE_INT: ::c_int = 2;
pub const CTLTYPE_STRING: ::c_int = 3;
pub const CTLTYPE_QUAD: ::c_int = 4;
pub const CTLTYPE_STRUCT: ::c_int = 5;

pub const CTL_UNSPEC: ::c_int = 0;
pub const CTL_KERN: ::c_int = 1;
pub const CTL_VM: ::c_int = 2;
pub const CTL_FS: ::c_int = 3;
pub const CTL_NET: ::c_int = 4;
pub const CTL_DEBUG: ::c_int = 5;
pub const CTL_HW: ::c_int = 6;
pub const CTL_MACHDEP: ::c_int = 7;
pub const CTL_DDB: ::c_int = 9;
pub const CTL_VFS: ::c_int = 10;
pub const CTL_MAXID: ::c_int = 11;

pub const HW_NCPUONLINE: ::c_int = 25;

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
pub const KERN_PROF: ::c_int = 16;
pub const KERN_POSIX1: ::c_int = 17;
pub const KERN_NGROUPS: ::c_int = 18;
pub const KERN_JOB_CONTROL: ::c_int = 19;
pub const KERN_SAVED_IDS: ::c_int = 20;
pub const KERN_BOOTTIME: ::c_int = 21;
pub const KERN_DOMAINNAME: ::c_int = 22;
pub const KERN_MAXPARTITIONS: ::c_int = 23;
pub const KERN_RAWPARTITION: ::c_int = 24;
pub const KERN_MAXTHREAD: ::c_int = 25;
pub const KERN_NTHREADS: ::c_int = 26;
pub const KERN_OSVERSION: ::c_int = 27;
pub const KERN_SOMAXCONN: ::c_int = 28;
pub const KERN_SOMINCONN: ::c_int = 29;
pub const KERN_USERMOUNT: ::c_int = 30;
pub const KERN_NOSUIDCOREDUMP: ::c_int = 32;
pub const KERN_FSYNC: ::c_int = 33;
pub const KERN_SYSVMSG: ::c_int = 34;
pub const KERN_SYSVSEM: ::c_int = 35;
pub const KERN_SYSVSHM: ::c_int = 36;
pub const KERN_ARND: ::c_int = 37;
pub const KERN_MSGBUFSIZE: ::c_int = 38;
pub const KERN_MALLOCSTATS: ::c_int = 39;
pub const KERN_CPTIME: ::c_int = 40;
pub const KERN_NCHSTATS: ::c_int = 41;
pub const KERN_FORKSTAT: ::c_int = 42;
pub const KERN_NSELCOLL: ::c_int = 43;
pub const KERN_TTY: ::c_int = 44;
pub const KERN_CCPU: ::c_int = 45;
pub const KERN_FSCALE: ::c_int = 46;
pub const KERN_NPROCS: ::c_int = 47;
pub const KERN_MSGBUF: ::c_int = 48;
pub const KERN_POOL: ::c_int = 49;
pub const KERN_STACKGAPRANDOM: ::c_int = 50;
pub const KERN_SYSVIPC_INFO: ::c_int = 51;
pub const KERN_SPLASSERT: ::c_int = 54;
pub const KERN_PROC_ARGS: ::c_int = 55;
pub const KERN_NFILES: ::c_int = 56;
pub const KERN_TTYCOUNT: ::c_int = 57;
pub const KERN_NUMVNODES: ::c_int = 58;
pub const KERN_MBSTAT: ::c_int = 59;
pub const KERN_SEMINFO: ::c_int = 61;
pub const KERN_SHMINFO: ::c_int = 62;
pub const KERN_INTRCNT: ::c_int = 63;
pub const KERN_WATCHDOG: ::c_int = 64;
pub const KERN_PROC: ::c_int = 66;
pub const KERN_MAXCLUSTERS: ::c_int = 67;
pub const KERN_EVCOUNT: ::c_int = 68;
pub const KERN_TIMECOUNTER: ::c_int = 69;
pub const KERN_MAXLOCKSPERUID: ::c_int = 70;
pub const KERN_CPTIME2: ::c_int = 71;
pub const KERN_CACHEPCT: ::c_int = 72;
pub const KERN_FILE: ::c_int = 73;
pub const KERN_CONSDEV: ::c_int = 75;
pub const KERN_NETLIVELOCKS: ::c_int = 76;
pub const KERN_POOL_DEBUG: ::c_int = 77;
pub const KERN_PROC_CWD: ::c_int = 78;
pub const KERN_PROC_NOBROADCASTKILL: ::c_int = 79;
pub const KERN_PROC_VMMAP: ::c_int = 80;
pub const KERN_GLOBAL_PTRACE: ::c_int = 81;
pub const KERN_CONSBUFSIZE: ::c_int = 82;
pub const KERN_CONSBUF: ::c_int = 83;
pub const KERN_AUDIO: ::c_int = 84;
pub const KERN_CPUSTATS: ::c_int = 85;
pub const KERN_PFSTATUS: ::c_int = 86;
pub const KERN_TIMEOUT_STATS: ::c_int = 87;
pub const KERN_MAXID: ::c_int = 88;

pub const KERN_PROC_ALL: ::c_int = 0;
pub const KERN_PROC_PID: ::c_int = 1;
pub const KERN_PROC_PGRP: ::c_int = 2;
pub const KERN_PROC_SESSION: ::c_int = 3;
pub const KERN_PROC_TTY: ::c_int = 4;
pub const KERN_PROC_UID: ::c_int = 5;
pub const KERN_PROC_RUID: ::c_int = 6;
pub const KERN_PROC_KTHREAD: ::c_int = 7;
pub const KERN_PROC_SHOW_THREADS: ::c_int = 0x40000000;

pub const KERN_SYSVIPC_MSG_INFO: ::c_int = 1;
pub const KERN_SYSVIPC_SEM_INFO: ::c_int = 2;
pub const KERN_SYSVIPC_SHM_INFO: ::c_int = 3;

pub const KERN_PROC_ARGV: ::c_int = 1;
pub const KERN_PROC_NARGV: ::c_int = 2;
pub const KERN_PROC_ENV: ::c_int = 3;
pub const KERN_PROC_NENV: ::c_int = 4;

pub const KI_NGROUPS: ::c_int = 16;
pub const KI_MAXCOMLEN: ::c_int = 24;
pub const KI_WMESGLEN: ::c_int = 8;
pub const KI_MAXLOGNAME: ::c_int = 32;
pub const KI_EMULNAMELEN: ::c_int = 8;

pub const CHWFLOW: ::tcflag_t = ::MDMBUF | ::CRTSCTS;
pub const OLCUC: ::tcflag_t = 0x20;
pub const ONOCR: ::tcflag_t = 0x40;
pub const ONLRET: ::tcflag_t = 0x80;

//https://github.com/openbsd/src/blob/master/sys/sys/mount.h
pub const ISOFSMNT_NORRIP: ::c_int = 0x1; // disable Rock Ridge Ext
pub const ISOFSMNT_GENS: ::c_int = 0x2; // enable generation numbers
pub const ISOFSMNT_EXTATT: ::c_int = 0x4; // enable extended attr
pub const ISOFSMNT_NOJOLIET: ::c_int = 0x8; // disable Joliet Ext
pub const ISOFSMNT_SESS: ::c_int = 0x10; // use iso_args.sess

pub const NFS_ARGSVERSION: ::c_int = 4; // change when nfs_args changes

pub const NFSMNT_RESVPORT: ::c_int = 0; // always use reserved ports
pub const NFSMNT_SOFT: ::c_int = 0x1; // soft mount (hard is default)
pub const NFSMNT_WSIZE: ::c_int = 0x2; // set write size
pub const NFSMNT_RSIZE: ::c_int = 0x4; // set read size
pub const NFSMNT_TIMEO: ::c_int = 0x8; // set initial timeout
pub const NFSMNT_RETRANS: ::c_int = 0x10; // set number of request retries
pub const NFSMNT_MAXGRPS: ::c_int = 0x20; // set maximum grouplist size
pub const NFSMNT_INT: ::c_int = 0x40; // allow interrupts on hard mount
pub const NFSMNT_NOCONN: ::c_int = 0x80; // Don't Connect the socket
pub const NFSMNT_NQNFS: ::c_int = 0x100; // Use Nqnfs protocol
pub const NFSMNT_NFSV3: ::c_int = 0x200; // Use NFS Version 3 protocol
pub const NFSMNT_KERB: ::c_int = 0x400; // Use Kerberos authentication
pub const NFSMNT_DUMBTIMR: ::c_int = 0x800; // Don't estimate rtt dynamically
pub const NFSMNT_LEASETERM: ::c_int = 0x1000; // set lease term (nqnfs)
pub const NFSMNT_READAHEAD: ::c_int = 0x2000; // set read ahead
pub const NFSMNT_DEADTHRESH: ::c_int = 0x4000; // set dead server retry thresh
pub const NFSMNT_NOAC: ::c_int = 0x8000; // disable attribute cache
pub const NFSMNT_RDIRPLUS: ::c_int = 0x10000; // Use Readdirplus for V3
pub const NFSMNT_READDIRSIZE: ::c_int = 0x20000; // Set readdir size

/* Flags valid only in mount syscall arguments */
pub const NFSMNT_ACREGMIN: ::c_int = 0x40000; // acregmin field valid
pub const NFSMNT_ACREGMAX: ::c_int = 0x80000; // acregmax field valid
pub const NFSMNT_ACDIRMIN: ::c_int = 0x100000; // acdirmin field valid
pub const NFSMNT_ACDIRMAX: ::c_int = 0x200000; // acdirmax field valid

/* Flags valid only in kernel */
pub const NFSMNT_INTERNAL: ::c_int = 0xfffc0000; // Bits set internally
pub const NFSMNT_HASWRITEVERF: ::c_int = 0x40000; // Has write verifier for V3
pub const NFSMNT_GOTPATHCONF: ::c_int = 0x80000; // Got the V3 pathconf info
pub const NFSMNT_GOTFSINFO: ::c_int = 0x100000; // Got the V3 fsinfo
pub const NFSMNT_MNTD: ::c_int = 0x200000; // Mnt server for mnt point
pub const NFSMNT_DISMINPROG: ::c_int = 0x400000; // Dismount in progress
pub const NFSMNT_DISMNT: ::c_int = 0x800000; // Dismounted
pub const NFSMNT_SNDLOCK: ::c_int = 0x1000000; // Send socket lock
pub const NFSMNT_WANTSND: ::c_int = 0x2000000; // Want above
pub const NFSMNT_RCVLOCK: ::c_int = 0x4000000; // Rcv socket lock
pub const NFSMNT_WANTRCV: ::c_int = 0x8000000; // Want above
pub const NFSMNT_WAITAUTH: ::c_int = 0x10000000; // Wait for authentication
pub const NFSMNT_HASAUTH: ::c_int = 0x20000000; // Has authenticator
pub const NFSMNT_WANTAUTH: ::c_int = 0x40000000; // Wants an authenticator
pub const NFSMNT_AUTHERR: ::c_int = 0x80000000; // Authentication error

pub const MSDOSFSMNT_SHORTNAME: ::c_int = 0x1; // Force old DOS short names only
pub const MSDOSFSMNT_LONGNAME: ::c_int = 0x2; // Force Win'95 long names
pub const MSDOSFSMNT_NOWIN95: ::c_int = 0x4; // Completely ignore Win95 entries

pub const NTFS_MFLAG_CASEINS: ::c_int = 0x1;
pub const NTFS_MFLAG_ALLNAMES: ::c_int = 0x2;

pub const TMPFS_ARGS_VERSION: ::c_int = 1;

pub const MAP_STACK: ::c_int = 0x4000;

// https://github.com/openbsd/src/blob/master/sys/net/if.h#L187
pub const IFF_UP: ::c_int = 0x1; // interface is up
pub const IFF_BROADCAST: ::c_int = 0x2; // broadcast address valid
pub const IFF_DEBUG: ::c_int = 0x4; // turn on debugging
pub const IFF_LOOPBACK: ::c_int = 0x8; // is a loopback net
pub const IFF_POINTOPOINT: ::c_int = 0x10; // interface is point-to-point link
pub const IFF_STATICARP: ::c_int = 0x20; // only static ARP
pub const IFF_RUNNING: ::c_int = 0x40; // resources allocated
pub const IFF_NOARP: ::c_int = 0x80; // no address resolution protocol
pub const IFF_PROMISC: ::c_int = 0x100; // receive all packets
pub const IFF_ALLMULTI: ::c_int = 0x200; // receive all multicast packets
pub const IFF_OACTIVE: ::c_int = 0x400; // transmission in progress
pub const IFF_SIMPLEX: ::c_int = 0x800; // can't hear own transmissions
pub const IFF_LINK0: ::c_int = 0x1000; // per link layer defined bit
pub const IFF_LINK1: ::c_int = 0x2000; // per link layer defined bit
pub const IFF_LINK2: ::c_int = 0x4000; // per link layer defined bit
pub const IFF_MULTICAST: ::c_int = 0x8000; // supports multicast

pub const PTHREAD_STACK_MIN: ::size_t = (1_usize << _MAX_PAGE_SHIFT);
pub const MINSIGSTKSZ: ::size_t = (3_usize << _MAX_PAGE_SHIFT);
pub const SIGSTKSZ: ::size_t = MINSIGSTKSZ + (1_usize << _MAX_PAGE_SHIFT) * 4;

pub const PT_FIRSTMACH: ::c_int = 32;

pub const SOCK_CLOEXEC: ::c_int = 0x8000;
pub const SOCK_NONBLOCK: ::c_int = 0x4000;
pub const SOCK_DNS: ::c_int = 0x1000;

pub const BIOCGRSIG: ::c_ulong = 0x40044273;
pub const BIOCSRSIG: ::c_ulong = 0x80044272;
pub const BIOCSDLT: ::c_ulong = 0x8004427a;

pub const PTRACE_FORK: ::c_int = 0x0002;

pub const WCONTINUED: ::c_int = 8;

fn _ALIGN(p: usize) -> usize {
    (p + _ALIGNBYTES) & !_ALIGNBYTES
}

f! {
    pub fn WIFCONTINUED(status: ::c_int) -> bool {
        status & 0o177777 == 0o177777
    }

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

    pub fn WSTOPSIG(status: ::c_int) -> ::c_int {
        status >> 8
    }

    pub fn WIFSIGNALED(status: ::c_int) -> bool {
        (status & 0o177) != 0o177 && (status & 0o177) != 0
    }

    pub fn WIFSTOPPED(status: ::c_int) -> bool {
        (status & 0o177) == 0o177
    }
}

extern "C" {
    pub fn gettimeofday(tp: *mut ::timeval, tz: *mut ::timezone) -> ::c_int;
    pub fn accept4(
        s: ::c_int,
        addr: *mut ::sockaddr,
        addrlen: *mut ::socklen_t,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn execvpe(
        file: *const ::c_char,
        argv: *const *const ::c_char,
        envp: *const *const ::c_char,
    ) -> ::c_int;
    pub fn pledge(
        promises: *const ::c_char,
        execpromises: *const ::c_char,
    ) -> ::c_int;
    pub fn strtonum(
        nptr: *const ::c_char,
        minval: ::c_longlong,
        maxval: ::c_longlong,
        errstr: *mut *const ::c_char,
    ) -> ::c_longlong;
    pub fn dup3(src: ::c_int, dst: ::c_int, flags: ::c_int) -> ::c_int;
    pub fn chflags(path: *const ::c_char, flags: ::c_uint) -> ::c_int;
    pub fn fchflags(fd: ::c_int, flags: ::c_uint) -> ::c_int;
    pub fn chflagsat(
        fd: ::c_int,
        path: *const ::c_char,
        flags: ::c_uint,
        atflag: ::c_int,
    ) -> ::c_int;
    pub fn dirfd(dirp: *mut ::DIR) -> ::c_int;
    pub fn getnameinfo(
        sa: *const ::sockaddr,
        salen: ::socklen_t,
        host: *mut ::c_char,
        hostlen: ::size_t,
        serv: *mut ::c_char,
        servlen: ::size_t,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn kevent(
        kq: ::c_int,
        changelist: *const ::kevent,
        nchanges: ::c_int,
        eventlist: *mut ::kevent,
        nevents: ::c_int,
        timeout: *const ::timespec,
    ) -> ::c_int;
    pub fn mprotect(
        addr: *mut ::c_void,
        len: ::size_t,
        prot: ::c_int,
    ) -> ::c_int;
    pub fn pthread_main_np() -> ::c_int;
    pub fn pthread_set_name_np(tid: ::pthread_t, name: *const ::c_char);
    pub fn pthread_stackseg_np(
        thread: ::pthread_t,
        sinfo: *mut ::stack_t,
    ) -> ::c_int;
    pub fn sysctl(
        name: *const ::c_int,
        namelen: ::c_uint,
        oldp: *mut ::c_void,
        oldlenp: *mut ::size_t,
        newp: *mut ::c_void,
        newlen: ::size_t,
    ) -> ::c_int;
    pub fn getentropy(buf: *mut ::c_void, buflen: ::size_t) -> ::c_int;
    pub fn setresgid(rgid: ::gid_t, egid: ::gid_t, sgid: ::gid_t) -> ::c_int;
    pub fn setresuid(ruid: ::uid_t, euid: ::uid_t, suid: ::uid_t) -> ::c_int;
    pub fn ptrace(
        request: ::c_int,
        pid: ::pid_t,
        addr: caddr_t,
        data: ::c_int,
    ) -> ::c_int;
}

cfg_if! {
    if #[cfg(libc_union)] {
        extern {
            // these functions use statfs which uses the union mount_info:
            pub fn statfs(path: *const ::c_char, buf: *mut statfs) -> ::c_int;
            pub fn fstatfs(fd: ::c_int, buf: *mut statfs) -> ::c_int;
        }
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
    } else if #[cfg(target_arch = "sparc64")] {
        mod sparc64;
        pub use self::sparc64::*;
    } else {
        // Unknown target_arch
    }
}
