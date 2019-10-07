pub type clock_t = ::c_uint;
pub type suseconds_t = ::c_int;
pub type dev_t = u64;
pub type blksize_t = i32;
pub type fsblkcnt_t = u64;
pub type fsfilcnt_t = u64;
pub type idtype_t = ::c_int;
pub type mqd_t = ::c_int;
type __pthread_spin_t = __cpu_simple_lock_nv_t;
pub type vm_size_t = ::uintptr_t;
pub type lwpid_t = ::c_uint;

impl siginfo_t {
    pub unsafe fn si_value(&self) -> ::sigval {
        #[repr(C)]
        struct siginfo_timer {
            _si_signo: ::c_int,
            _si_errno: ::c_int,
            _si_code: ::c_int,
            __pad1: ::c_int,
            _pid: ::pid_t,
            _uid: ::uid_t,
            value: ::sigval,
        }
        (*(self as *const siginfo_t as *const siginfo_timer)).value
    }
}

s! {
    pub struct aiocb {
        pub aio_offset: ::off_t,
        pub aio_buf: *mut ::c_void,
        pub aio_nbytes: ::size_t,
        pub aio_fildes: ::c_int,
        pub aio_lio_opcode: ::c_int,
        pub aio_reqprio: ::c_int,
        pub aio_sigevent: ::sigevent,
        _state: ::c_int,
        _errno: ::c_int,
        _retval: ::ssize_t
    }

    pub struct glob_t {
        pub gl_pathc:   ::size_t,
        pub gl_matchc:  ::size_t,
        pub gl_offs:    ::size_t,
        pub gl_flags:   ::c_int,
        pub gl_pathv:   *mut *mut ::c_char,

        __unused3: *mut ::c_void,

        __unused4: *mut ::c_void,
        __unused5: *mut ::c_void,
        __unused6: *mut ::c_void,
        __unused7: *mut ::c_void,
        __unused8: *mut ::c_void,
    }

    pub struct mq_attr {
        pub mq_flags: ::c_long,
        pub mq_maxmsg: ::c_long,
        pub mq_msgsize: ::c_long,
        pub mq_curmsgs: ::c_long,
    }

    pub struct sigset_t {
        __bits: [u32; 4],
    }

    pub struct stat {
        pub st_dev: ::dev_t,
        pub st_mode: ::mode_t,
        pub st_ino: ::ino_t,
        pub st_nlink: ::nlink_t,
        pub st_uid: ::uid_t,
        pub st_gid: ::gid_t,
        pub st_rdev: ::dev_t,
        pub st_atime: ::time_t,
        pub st_atimensec: ::c_long,
        pub st_mtime: ::time_t,
        pub st_mtimensec: ::c_long,
        pub st_ctime: ::time_t,
        pub st_ctimensec: ::c_long,
        pub st_birthtime: ::time_t,
        pub st_birthtimensec: ::c_long,
        pub st_size: ::off_t,
        pub st_blocks: ::blkcnt_t,
        pub st_blksize: ::blksize_t,
        pub st_flags: u32,
        pub st_gen: u32,
        pub st_spare: [u32; 2],
    }

     pub struct addrinfo {
        pub ai_flags: ::c_int,
        pub ai_family: ::c_int,
        pub ai_socktype: ::c_int,
        pub ai_protocol: ::c_int,
        pub ai_addrlen: ::socklen_t,
        pub ai_canonname: *mut ::c_char,
        pub ai_addr: *mut ::sockaddr,
        pub ai_next: *mut ::addrinfo,
    }

    pub struct siginfo_t {
        pub si_signo: ::c_int,
        pub si_code: ::c_int,
        pub si_errno: ::c_int,
        __pad1: ::c_int,
        pub si_addr: *mut ::c_void,
        __pad2: [u64; 13],
    }

    pub struct pthread_attr_t {
        pta_magic: ::c_uint,
        pta_flags: ::c_int,
        pta_private: *mut ::c_void,
    }

    pub struct pthread_mutex_t {
        ptm_magic: ::c_uint,
        ptm_errorcheck: __pthread_spin_t,
        #[cfg(any(target_arch = "sparc", target_arch = "sparc64",
                  target_arch = "x86", target_arch = "x86_64"))]
        ptm_pad1: [u8; 3],
        // actually a union with a non-unused, 0-initialized field
        ptm_unused: __pthread_spin_t,
        #[cfg(any(target_arch = "sparc", target_arch = "sparc64",
                  target_arch = "x86", target_arch = "x86_64"))]
        ptm_pad2: [u8; 3],
        ptm_owner: ::pthread_t,
        ptm_waiters: *mut u8,
        ptm_recursed: ::c_uint,
        ptm_spare2: *mut ::c_void,
    }

    pub struct pthread_mutexattr_t {
        ptma_magic: ::c_uint,
        ptma_private: *mut ::c_void,
    }

    pub struct pthread_rwlockattr_t {
        ptra_magic: ::c_uint,
        ptra_private: *mut ::c_void,
    }

    pub struct pthread_cond_t {
        ptc_magic: ::c_uint,
        ptc_lock: __pthread_spin_t,
        ptc_waiters_first: *mut u8,
        ptc_waiters_last: *mut u8,
        ptc_mutex: *mut ::pthread_mutex_t,
        ptc_private: *mut ::c_void,
    }

    pub struct pthread_condattr_t {
        ptca_magic: ::c_uint,
        ptca_private: *mut ::c_void,
    }

    pub struct pthread_rwlock_t {
        ptr_magic: ::c_uint,
        ptr_interlock: __pthread_spin_t,
        ptr_rblocked_first: *mut u8,
        ptr_rblocked_last: *mut u8,
        ptr_wblocked_first: *mut u8,
        ptr_wblocked_last: *mut u8,
        ptr_nreaders: ::c_uint,
        ptr_owner: ::pthread_t,
        ptr_private: *mut ::c_void,
    }

    pub struct kevent {
        pub ident: ::uintptr_t,
        pub filter: u32,
        pub flags: u32,
        pub fflags: u32,
        pub data: i64,
        pub udata: ::intptr_t,
    }

    pub struct dqblk {
        pub dqb_bhardlimit: u32,
        pub dqb_bsoftlimit: u32,
        pub dqb_curblocks: u32,
        pub dqb_ihardlimit: u32,
        pub dqb_isoftlimit: u32,
        pub dqb_curinodes: u32,
        pub dqb_btime: i32,
        pub dqb_itime: i32,
    }

    pub struct Dl_info {
        pub dli_fname: *const ::c_char,
        pub dli_fbase: *mut ::c_void,
        pub dli_sname: *const ::c_char,
        pub dli_saddr: *const ::c_void,
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
        pub int_n_cs_precedes: ::c_char,
        pub int_p_sep_by_space: ::c_char,
        pub int_n_sep_by_space: ::c_char,
        pub int_p_sign_posn: ::c_char,
        pub int_n_sign_posn: ::c_char,
    }

    pub struct if_data {
        pub ifi_type: ::c_uchar,
        pub ifi_addrlen: ::c_uchar,
        pub ifi_hdrlen: ::c_uchar,
        pub ifi_link_state: ::c_int,
        pub ifi_mtu: u64,
        pub ifi_metric: u64,
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
        pub ifi_noproto: u64,
        pub ifi_lastchange: ::timespec,
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

    pub struct sockcred {
        pub sc_pid: ::pid_t,
        pub sc_uid: ::uid_t,
        pub sc_euid: ::uid_t,
        pub sc_gid: ::gid_t,
        pub sc_egid: ::gid_t,
        pub sc_ngroups: ::c_int,
        pub sc_groups: [::gid_t; 1],
    }

    pub struct sockaddr_dl {
        pub sdl_len: ::c_uchar,
        pub sdl_family: ::c_uchar,
        pub sdl_index: ::c_ushort,
        pub sdl_type: u8,
        pub sdl_nlen: u8,
        pub sdl_alen: u8,
        pub sdl_slen: u8,
        pub sdl_data: [::c_char; 12],
    }

    pub struct mmsghdr {
        pub msg_hdr: ::msghdr,
        pub msg_len: ::c_uint,
    }
}

s_no_extra_traits! {
    pub struct in_pktinfo {
        pub ipi_addr: ::in_addr,
        pub ipi_ifindex: ::c_uint,
    }

    #[repr(packed)]
    pub struct arphdr {
        pub ar_hrd: u16,
        pub ar_pro: u16,
        pub ar_hln: u8,
        pub ar_pln: u8,
        pub ar_op: u16,
    }

    #[repr(packed)]
    pub struct in_addr {
        pub s_addr: ::in_addr_t,
    }

    pub struct ip_mreq {
        pub imr_multiaddr: in_addr,
        pub imr_interface: in_addr,
    }

    pub struct sockaddr_in {
        pub sin_len: u8,
        pub sin_family: ::sa_family_t,
        pub sin_port: ::in_port_t,
        pub sin_addr: ::in_addr,
        pub sin_zero: [i8; 8],
    }

    pub struct dirent {
        pub d_fileno: ::ino_t,
        pub d_reclen: u16,
        pub d_namlen: u16,
        pub d_type: u8,
        pub d_name: [::c_char; 512],
    }

    pub struct statvfs {
        pub f_flag: ::c_ulong,
        pub f_bsize: ::c_ulong,
        pub f_frsize: ::c_ulong,
        pub f_iosize: ::c_ulong,

        pub f_blocks: ::fsblkcnt_t,
        pub f_bfree: ::fsblkcnt_t,
        pub f_bavail: ::fsblkcnt_t,
        pub f_bresvd: ::fsblkcnt_t,

        pub f_files: ::fsfilcnt_t,
        pub f_ffree: ::fsfilcnt_t,
        pub f_favail: ::fsfilcnt_t,
        pub f_fresvd: ::fsfilcnt_t,

        pub f_syncreads: u64,
        pub f_syncwrites: u64,

        pub f_asyncreads: u64,
        pub f_asyncwrites: u64,

        pub f_fsidx: ::fsid_t,
        pub f_fsid: ::c_ulong,
        pub f_namemax: ::c_ulong,
        pub f_owner: ::uid_t,

        pub f_spare: [u32; 4],

        pub f_fstypename: [::c_char; 32],
        pub f_mntonname: [::c_char; 1024],
        pub f_mntfromname: [::c_char; 1024],
    }

    pub struct sockaddr_storage {
        pub ss_len: u8,
        pub ss_family: ::sa_family_t,
        __ss_pad1: [u8; 6],
        __ss_pad2: i64,
        __ss_pad3: [u8; 112],
    }

    pub struct sigevent {
        pub sigev_notify: ::c_int,
        pub sigev_signo: ::c_int,
        pub sigev_value: ::sigval,
        __unused1: *mut ::c_void,       //actually a function pointer
        pub sigev_notify_attributes: *mut ::c_void
    }
}

cfg_if! {
    if #[cfg(feature = "extra_traits")] {
        impl PartialEq for in_pktinfo {
            fn eq(&self, other: &in_pktinfo) -> bool {
                self.ipi_addr == other.ipi_addr
                    && self.ipi_ifindex == other.ipi_ifindex
            }
        }
        impl Eq for in_pktinfo {}
        impl ::fmt::Debug for in_pktinfo {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("in_pktinfo")
                    .field("ipi_addr", &self.ipi_addr)
                    .field("ipi_ifindex", &self.ipi_ifindex)
                    .finish()
            }
        }
        impl ::hash::Hash for in_pktinfo {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.ipi_addr.hash(state);
                self.ipi_ifindex.hash(state);
            }
        }

        impl PartialEq for arphdr {
            fn eq(&self, other: &arphdr) -> bool {
                self.ar_hrd == other.ar_hrd
                    && self.ar_pro == other.ar_pro
                    && self.ar_hln == other.ar_hln
                    && self.ar_pln == other.ar_pln
                    && self.ar_op == other.ar_op
            }
        }
        impl Eq for arphdr {}
        impl ::fmt::Debug for arphdr {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                let ar_hrd = self.ar_hrd;
                let ar_pro = self.ar_pro;
                let ar_op = self.ar_op;
                f.debug_struct("arphdr")
                    .field("ar_hrd", &ar_hrd)
                    .field("ar_pro", &ar_pro)
                    .field("ar_hln", &self.ar_hln)
                    .field("ar_pln", &self.ar_pln)
                    .field("ar_op", &ar_op)
                    .finish()
            }
        }
        impl ::hash::Hash for arphdr {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                let ar_hrd = self.ar_hrd;
                let ar_pro = self.ar_pro;
                let ar_op = self.ar_op;
                ar_hrd.hash(state);
                ar_pro.hash(state);
                self.ar_hln.hash(state);
                self.ar_pln.hash(state);
                ar_op.hash(state);
            }
        }

        impl PartialEq for in_addr {
            fn eq(&self, other: &in_addr) -> bool {
                self.s_addr == other.s_addr
            }
        }
        impl Eq for in_addr {}
        impl ::fmt::Debug for in_addr {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                let s_addr = self.s_addr;
                f.debug_struct("in_addr")
                    .field("s_addr", &s_addr)
                    .finish()
            }
        }
        impl ::hash::Hash for in_addr {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                let s_addr = self.s_addr;
                s_addr.hash(state);
            }
        }

        impl PartialEq for ip_mreq {
            fn eq(&self, other: &ip_mreq) -> bool {
                self.imr_multiaddr == other.imr_multiaddr
                    && self.imr_interface == other.imr_interface
            }
        }
        impl Eq for ip_mreq {}
        impl ::fmt::Debug for ip_mreq {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("ip_mreq")
                    .field("imr_multiaddr", &self.imr_multiaddr)
                    .field("imr_interface", &self.imr_interface)
                    .finish()
            }
        }
        impl ::hash::Hash for ip_mreq {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.imr_multiaddr.hash(state);
                self.imr_interface.hash(state);
            }
        }

        impl PartialEq for sockaddr_in {
            fn eq(&self, other: &sockaddr_in) -> bool {
                self.sin_len == other.sin_len
                    && self.sin_family == other.sin_family
                    && self.sin_port == other.sin_port
                    && self.sin_addr == other.sin_addr
                    && self.sin_zero == other.sin_zero
            }
        }
        impl Eq for sockaddr_in {}
        impl ::fmt::Debug for sockaddr_in {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("sockaddr_in")
                    .field("sin_len", &self.sin_len)
                    .field("sin_family", &self.sin_family)
                    .field("sin_port", &self.sin_port)
                    .field("sin_addr", &self.sin_addr)
                    .field("sin_zero", &self.sin_zero)
                    .finish()
            }
        }
        impl ::hash::Hash for sockaddr_in {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.sin_len.hash(state);
                self.sin_family.hash(state);
                self.sin_port.hash(state);
                self.sin_addr.hash(state);
                self.sin_zero.hash(state);
            }
        }

        impl PartialEq for dirent {
            fn eq(&self, other: &dirent) -> bool {
                self.d_fileno == other.d_fileno
                    && self.d_reclen == other.d_reclen
                    && self.d_namlen == other.d_namlen
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
                    .field("d_fileno", &self.d_fileno)
                    .field("d_reclen", &self.d_reclen)
                    .field("d_namlen", &self.d_namlen)
                    .field("d_type", &self.d_type)
                    // FIXME: .field("d_name", &self.d_name)
                    .finish()
            }
        }
        impl ::hash::Hash for dirent {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.d_fileno.hash(state);
                self.d_reclen.hash(state);
                self.d_namlen.hash(state);
                self.d_type.hash(state);
                self.d_name.hash(state);
            }
        }

        impl PartialEq for statvfs {
            fn eq(&self, other: &statvfs) -> bool {
                self.f_flag == other.f_flag
                    && self.f_bsize == other.f_bsize
                    && self.f_frsize == other.f_frsize
                    && self.f_iosize == other.f_iosize
                    && self.f_blocks == other.f_blocks
                    && self.f_bfree == other.f_bfree
                    && self.f_bavail == other.f_bavail
                    && self.f_bresvd == other.f_bresvd
                    && self.f_files == other.f_files
                    && self.f_ffree == other.f_ffree
                    && self.f_favail == other.f_favail
                    && self.f_fresvd == other.f_fresvd
                    && self.f_syncreads == other.f_syncreads
                    && self.f_syncwrites == other.f_syncwrites
                    && self.f_asyncreads == other.f_asyncreads
                    && self.f_asyncwrites == other.f_asyncwrites
                    && self.f_fsidx == other.f_fsidx
                    && self.f_fsid == other.f_fsid
                    && self.f_namemax == other.f_namemax
                    && self.f_owner == other.f_owner
                    && self.f_spare == other.f_spare
                    && self.f_fstypename == other.f_fstypename
                    && self
                    .f_mntonname
                    .iter()
                    .zip(other.f_mntonname.iter())
                    .all(|(a,b)| a == b)
                    && self
                    .f_mntfromname
                    .iter()
                    .zip(other.f_mntfromname.iter())
                    .all(|(a,b)| a == b)
            }
        }
        impl Eq for statvfs {}
        impl ::fmt::Debug for statvfs {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("statvfs")
                    .field("f_flag", &self.f_flag)
                    .field("f_bsize", &self.f_bsize)
                    .field("f_frsize", &self.f_frsize)
                    .field("f_iosize", &self.f_iosize)
                    .field("f_blocks", &self.f_blocks)
                    .field("f_bfree", &self.f_bfree)
                    .field("f_bavail", &self.f_bavail)
                    .field("f_bresvd", &self.f_bresvd)
                    .field("f_files", &self.f_files)
                    .field("f_ffree", &self.f_ffree)
                    .field("f_favail", &self.f_favail)
                    .field("f_fresvd", &self.f_fresvd)
                    .field("f_syncreads", &self.f_syncreads)
                    .field("f_syncwrites", &self.f_syncwrites)
                    .field("f_asyncreads", &self.f_asyncreads)
                    .field("f_asyncwrites", &self.f_asyncwrites)
                    .field("f_fsidx", &self.f_fsidx)
                    .field("f_fsid", &self.f_fsid)
                    .field("f_namemax", &self.f_namemax)
                    .field("f_owner", &self.f_owner)
                    .field("f_spare", &self.f_spare)
                    .field("f_fstypename", &self.f_fstypename)
                    // FIXME: .field("f_mntonname", &self.f_mntonname)
                    // FIXME: .field("f_mntfromname", &self.f_mntfromname)
                    .finish()
            }
        }
        impl ::hash::Hash for statvfs {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.f_flag.hash(state);
                self.f_bsize.hash(state);
                self.f_frsize.hash(state);
                self.f_iosize.hash(state);
                self.f_blocks.hash(state);
                self.f_bfree.hash(state);
                self.f_bavail.hash(state);
                self.f_bresvd.hash(state);
                self.f_files.hash(state);
                self.f_ffree.hash(state);
                self.f_favail.hash(state);
                self.f_fresvd.hash(state);
                self.f_syncreads.hash(state);
                self.f_syncwrites.hash(state);
                self.f_asyncreads.hash(state);
                self.f_asyncwrites.hash(state);
                self.f_fsidx.hash(state);
                self.f_fsid.hash(state);
                self.f_namemax.hash(state);
                self.f_owner.hash(state);
                self.f_spare.hash(state);
                self.f_fstypename.hash(state);
                self.f_mntonname.hash(state);
                self.f_mntfromname.hash(state);
            }
        }

        impl PartialEq for sockaddr_storage {
            fn eq(&self, other: &sockaddr_storage) -> bool {
                self.ss_len == other.ss_len
                    && self.ss_family == other.ss_family
                    && self.__ss_pad1 == other.__ss_pad1
                    && self.__ss_pad2 == other.__ss_pad2
                    && self
                    .__ss_pad3
                    .iter()
                    .zip(other.__ss_pad3.iter())
                    .all(|(a,b)| a == b)
            }
        }
        impl Eq for sockaddr_storage {}
        impl ::fmt::Debug for sockaddr_storage {
            fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                f.debug_struct("sockaddr_storage")
                    .field("ss_len", &self.ss_len)
                    .field("ss_family", &self.ss_family)
                    .field("__ss_pad1", &self.__ss_pad1)
                    .field("__ss_pad2", &self.__ss_pad2)
                    // FIXME: .field("__ss_pad3", &self.__ss_pad3)
                    .finish()
            }
        }
        impl ::hash::Hash for sockaddr_storage {
            fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                self.ss_len.hash(state);
                self.ss_family.hash(state);
                self.__ss_pad1.hash(state);
                self.__ss_pad2.hash(state);
                self.__ss_pad3.hash(state);
            }
        }

        impl PartialEq for sigevent {
            fn eq(&self, other: &sigevent) -> bool {
                self.sigev_notify == other.sigev_notify
                    && self.sigev_signo == other.sigev_signo
                    && self.sigev_value == other.sigev_value
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
                self.sigev_notify_attributes.hash(state);
            }
        }
    }
}

pub const AT_FDCWD: ::c_int = -100;
pub const AT_EACCESS: ::c_int = 0x100;
pub const AT_SYMLINK_NOFOLLOW: ::c_int = 0x200;
pub const AT_SYMLINK_FOLLOW: ::c_int = 0x400;
pub const AT_REMOVEDIR: ::c_int = 0x800;

pub const EXTATTR_NAMESPACE_USER: ::c_int = 1;
pub const EXTATTR_NAMESPACE_SYSTEM: ::c_int = 2;

pub const LC_COLLATE_MASK: ::c_int = (1 << ::LC_COLLATE);
pub const LC_CTYPE_MASK: ::c_int = (1 << ::LC_CTYPE);
pub const LC_MONETARY_MASK: ::c_int = (1 << ::LC_MONETARY);
pub const LC_NUMERIC_MASK: ::c_int = (1 << ::LC_NUMERIC);
pub const LC_TIME_MASK: ::c_int = (1 << ::LC_TIME);
pub const LC_MESSAGES_MASK: ::c_int = (1 << ::LC_MESSAGES);
pub const LC_ALL_MASK: ::c_int = !0;

pub const ERA: ::nl_item = 52;
pub const ERA_D_FMT: ::nl_item = 53;
pub const ERA_D_T_FMT: ::nl_item = 54;
pub const ERA_T_FMT: ::nl_item = 55;
pub const ALT_DIGITS: ::nl_item = 56;

pub const O_CLOEXEC: ::c_int = 0x400000;
pub const O_ALT_IO: ::c_int = 0x40000;
pub const O_NOSIGPIPE: ::c_int = 0x1000000;
pub const O_SEARCH: ::c_int = 0x800000;
pub const O_DIRECTORY: ::c_int = 0x200000;
pub const O_DIRECT: ::c_int = 0x00080000;
pub const O_RSYNC: ::c_int = 0x00020000;

pub const MS_SYNC: ::c_int = 0x4;
pub const MS_INVALIDATE: ::c_int = 0x2;

#[deprecated(since = "0.2.64", note = "Not stable across OS versions")]
pub const RLIM_NLIMITS: ::c_int = 12;

pub const EIDRM: ::c_int = 82;
pub const ENOMSG: ::c_int = 83;
pub const EOVERFLOW: ::c_int = 84;
pub const EILSEQ: ::c_int = 85;
pub const ENOTSUP: ::c_int = 86;
pub const ECANCELED: ::c_int = 87;
pub const EBADMSG: ::c_int = 88;
pub const ENODATA: ::c_int = 89;
pub const ENOSR: ::c_int = 90;
pub const ENOSTR: ::c_int = 91;
pub const ETIME: ::c_int = 92;
pub const ENOATTR: ::c_int = 93;
pub const EMULTIHOP: ::c_int = 94;
pub const ENOLINK: ::c_int = 95;
pub const EPROTO: ::c_int = 96;
pub const ELAST: ::c_int = 96;

pub const F_DUPFD_CLOEXEC: ::c_int = 12;
pub const F_CLOSEM: ::c_int = 10;
pub const F_GETNOSIGPIPE: ::c_int = 13;
pub const F_SETNOSIGPIPE: ::c_int = 14;
pub const F_MAXFD: ::c_int = 11;

pub const IP_RECVDSTADDR: ::c_int = 7;
pub const IP_SENDSRCADDR: ::c_int = IP_RECVDSTADDR;
pub const IP_RECVIF: ::c_int = 20;
pub const IP_PKTINFO: ::c_int = 25;
pub const IP_RECVPKTINFO: ::c_int = 26;
pub const IPV6_JOIN_GROUP: ::c_int = 12;
pub const IPV6_LEAVE_GROUP: ::c_int = 13;

pub const TCP_KEEPIDLE: ::c_int = 3;
pub const TCP_KEEPINTVL: ::c_int = 5;
pub const TCP_KEEPCNT: ::c_int = 6;
pub const TCP_KEEPINIT: ::c_int = 7;
pub const TCP_INFO: ::c_int = 9;
pub const TCP_MD5SIG: ::c_int = 0x10;
pub const TCP_CONGCTL: ::c_int = 0x20;

pub const SOCK_CONN_DGRAM: ::c_int = 6;
pub const SOCK_DCCP: ::c_int = SOCK_CONN_DGRAM;
pub const SOCK_NOSIGPIPE: ::c_int = 0x40000000;
pub const SOCK_FLAGS_MASK: ::c_int = 0xf0000000;

pub const SO_SNDTIMEO: ::c_int = 0x100b;
pub const SO_RCVTIMEO: ::c_int = 0x100c;
pub const SO_ACCEPTFILTER: ::c_int = 0x1000;
pub const SO_TIMESTAMP: ::c_int = 0x2000;
pub const SO_OVERFLOWED: ::c_int = 0x1009;
pub const SO_NOHEADER: ::c_int = 0x100a;

// https://github.com/NetBSD/src/blob/trunk/sys/net/if.h#L373
pub const IFF_UP: ::c_int = 0x0001; // interface is up
pub const IFF_BROADCAST: ::c_int = 0x0002; // broadcast address valid
pub const IFF_DEBUG: ::c_int = 0x0004; // turn on debugging
pub const IFF_LOOPBACK: ::c_int = 0x0008; // is a loopback net
pub const IFF_POINTOPOINT: ::c_int = 0x0010; // interface is point-to-point link
pub const IFF_NOTRAILERS: ::c_int = 0x0020; // avoid use of trailers
pub const IFF_RUNNING: ::c_int = 0x0040; // resources allocated
pub const IFF_NOARP: ::c_int = 0x0080; // no address resolution protocol
pub const IFF_PROMISC: ::c_int = 0x0100; // receive all packets
pub const IFF_ALLMULTI: ::c_int = 0x0200; // receive all multicast packets
pub const IFF_OACTIVE: ::c_int = 0x0400; // transmission in progress
pub const IFF_SIMPLEX: ::c_int = 0x0800; // can't hear own transmissions
pub const IFF_LINK0: ::c_int = 0x1000; // per link layer defined bit
pub const IFF_LINK1: ::c_int = 0x2000; // per link layer defined bit
pub const IFF_LINK2: ::c_int = 0x4000; // per link layer defined bit
pub const IFF_MULTICAST: ::c_int = 0x8000; // supports multicast

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
/// DCCP
pub const IPPROTO_DCCP: ::c_int = 33;
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
/// IPv6 ICMP
pub const IPPROTO_IPV6_ICMP: ::c_int = 58;
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
/// VRRP RFC 2338
pub const IPPROTO_VRRP: ::c_int = 112;
/// Common Address Resolution Protocol
pub const IPPROTO_CARP: ::c_int = 112;
/// L2TPv3
// TEMP: Disabled for now; this constant was added to NetBSD on 2017-02-16,
// but isn't yet supported by the NetBSD rumprun kernel image used for
// libc testing.
//pub const IPPROTO_L2TP: ::c_int = 115;
/// SCTP
pub const IPPROTO_SCTP: ::c_int = 132;
/// PFSYNC
pub const IPPROTO_PFSYNC: ::c_int = 240;
pub const IPPROTO_MAX: ::c_int = 256;

/// last return value of *_input(), meaning "all job for this pkt is done".
pub const IPPROTO_DONE: ::c_int = 257;

/// sysctl placeholder for (FAST_)IPSEC
pub const CTL_IPPROTO_IPSEC: ::c_int = 258;

pub const AF_OROUTE: ::c_int = 17;
pub const AF_ARP: ::c_int = 28;
pub const pseudo_AF_KEY: ::c_int = 29;
pub const pseudo_AF_HDRCMPLT: ::c_int = 30;
pub const AF_BLUETOOTH: ::c_int = 31;
pub const AF_IEEE80211: ::c_int = 32;
pub const AF_MPLS: ::c_int = 33;
pub const AF_ROUTE: ::c_int = 34;
pub const NET_RT_DUMP: ::c_int = 1;
pub const NET_RT_FLAGS: ::c_int = 2;
pub const NET_RT_OOOIFLIST: ::c_int = 3;
pub const NET_RT_OOIFLIST: ::c_int = 4;
pub const NET_RT_OIFLIST: ::c_int = 5;
pub const NET_RT_IFLIST: ::c_int = 6;
pub const NET_RT_MAXID: ::c_int = 7;

pub const PF_OROUTE: ::c_int = AF_OROUTE;
pub const PF_ARP: ::c_int = AF_ARP;
pub const PF_KEY: ::c_int = pseudo_AF_KEY;
pub const PF_BLUETOOTH: ::c_int = AF_BLUETOOTH;
pub const PF_MPLS: ::c_int = AF_MPLS;
pub const PF_ROUTE: ::c_int = AF_ROUTE;

pub const MSG_NBIO: ::c_int = 0x1000;
pub const MSG_WAITFORONE: ::c_int = 0x2000;
pub const MSG_NOTIFICATION: ::c_int = 0x4000;

pub const SCM_TIMESTAMP: ::c_int = 0x08;
pub const SCM_CREDS: ::c_int = 0x10;

pub const O_DSYNC: ::c_int = 0x10000;

pub const MAP_RENAME: ::c_int = 0x20;
pub const MAP_NORESERVE: ::c_int = 0x40;
pub const MAP_HASSEMAPHORE: ::c_int = 0x200;
pub const MAP_WIRED: ::c_int = 0x800;

pub const DCCP_TYPE_REQUEST: ::c_int = 0;
pub const DCCP_TYPE_RESPONSE: ::c_int = 1;
pub const DCCP_TYPE_DATA: ::c_int = 2;
pub const DCCP_TYPE_ACK: ::c_int = 3;
pub const DCCP_TYPE_DATAACK: ::c_int = 4;
pub const DCCP_TYPE_CLOSEREQ: ::c_int = 5;
pub const DCCP_TYPE_CLOSE: ::c_int = 6;
pub const DCCP_TYPE_RESET: ::c_int = 7;
pub const DCCP_TYPE_MOVE: ::c_int = 8;

pub const DCCP_FEATURE_CC: ::c_int = 1;
pub const DCCP_FEATURE_ECN: ::c_int = 2;
pub const DCCP_FEATURE_ACKRATIO: ::c_int = 3;
pub const DCCP_FEATURE_ACKVECTOR: ::c_int = 4;
pub const DCCP_FEATURE_MOBILITY: ::c_int = 5;
pub const DCCP_FEATURE_LOSSWINDOW: ::c_int = 6;
pub const DCCP_FEATURE_CONN_NONCE: ::c_int = 8;
pub const DCCP_FEATURE_IDENTREG: ::c_int = 7;

pub const DCCP_OPT_PADDING: ::c_int = 0;
pub const DCCP_OPT_DATA_DISCARD: ::c_int = 1;
pub const DCCP_OPT_SLOW_RECV: ::c_int = 2;
pub const DCCP_OPT_BUF_CLOSED: ::c_int = 3;
pub const DCCP_OPT_CHANGE_L: ::c_int = 32;
pub const DCCP_OPT_CONFIRM_L: ::c_int = 33;
pub const DCCP_OPT_CHANGE_R: ::c_int = 34;
pub const DCCP_OPT_CONFIRM_R: ::c_int = 35;
pub const DCCP_OPT_INIT_COOKIE: ::c_int = 36;
pub const DCCP_OPT_NDP_COUNT: ::c_int = 37;
pub const DCCP_OPT_ACK_VECTOR0: ::c_int = 38;
pub const DCCP_OPT_ACK_VECTOR1: ::c_int = 39;
pub const DCCP_OPT_RECV_BUF_DROPS: ::c_int = 40;
pub const DCCP_OPT_TIMESTAMP: ::c_int = 41;
pub const DCCP_OPT_TIMESTAMP_ECHO: ::c_int = 42;
pub const DCCP_OPT_ELAPSEDTIME: ::c_int = 43;
pub const DCCP_OPT_DATACHECKSUM: ::c_int = 44;

pub const DCCP_REASON_UNSPEC: ::c_int = 0;
pub const DCCP_REASON_CLOSED: ::c_int = 1;
pub const DCCP_REASON_INVALID: ::c_int = 2;
pub const DCCP_REASON_OPTION_ERR: ::c_int = 3;
pub const DCCP_REASON_FEA_ERR: ::c_int = 4;
pub const DCCP_REASON_CONN_REF: ::c_int = 5;
pub const DCCP_REASON_BAD_SNAME: ::c_int = 6;
pub const DCCP_REASON_BAD_COOKIE: ::c_int = 7;
pub const DCCP_REASON_INV_MOVE: ::c_int = 8;
pub const DCCP_REASON_UNANSW_CH: ::c_int = 10;
pub const DCCP_REASON_FRUITLESS_NEG: ::c_int = 11;

pub const DCCP_CCID: ::c_int = 1;
pub const DCCP_CSLEN: ::c_int = 2;
pub const DCCP_MAXSEG: ::c_int = 4;
pub const DCCP_SERVICE: ::c_int = 8;

pub const DCCP_NDP_LIMIT: ::c_int = 16;
pub const DCCP_SEQ_NUM_LIMIT: ::c_int = 16777216;
pub const DCCP_MAX_OPTIONS: ::c_int = 32;
pub const DCCP_MAX_PKTS: ::c_int = 100;

pub const _PC_LINK_MAX: ::c_int = 1;
pub const _PC_MAX_CANON: ::c_int = 2;
pub const _PC_MAX_INPUT: ::c_int = 3;
pub const _PC_NAME_MAX: ::c_int = 4;
pub const _PC_PATH_MAX: ::c_int = 5;
pub const _PC_PIPE_BUF: ::c_int = 6;
pub const _PC_CHOWN_RESTRICTED: ::c_int = 7;
pub const _PC_NO_TRUNC: ::c_int = 8;
pub const _PC_VDISABLE: ::c_int = 9;
pub const _PC_SYNC_IO: ::c_int = 10;
pub const _PC_FILESIZEBITS: ::c_int = 11;
pub const _PC_SYMLINK_MAX: ::c_int = 12;
pub const _PC_2_SYMLINKS: ::c_int = 13;
pub const _PC_ACL_EXTENDED: ::c_int = 14;
pub const _PC_MIN_HOLE_SIZE: ::c_int = 15;

pub const _SC_SYNCHRONIZED_IO: ::c_int = 31;
pub const _SC_IOV_MAX: ::c_int = 32;
pub const _SC_MAPPED_FILES: ::c_int = 33;
pub const _SC_MEMLOCK: ::c_int = 34;
pub const _SC_MEMLOCK_RANGE: ::c_int = 35;
pub const _SC_MEMORY_PROTECTION: ::c_int = 36;
pub const _SC_LOGIN_NAME_MAX: ::c_int = 37;
pub const _SC_MONOTONIC_CLOCK: ::c_int = 38;
pub const _SC_CLK_TCK: ::c_int = 39;
pub const _SC_ATEXIT_MAX: ::c_int = 40;
pub const _SC_THREADS: ::c_int = 41;
pub const _SC_SEMAPHORES: ::c_int = 42;
pub const _SC_BARRIERS: ::c_int = 43;
pub const _SC_TIMERS: ::c_int = 44;
pub const _SC_SPIN_LOCKS: ::c_int = 45;
pub const _SC_READER_WRITER_LOCKS: ::c_int = 46;
pub const _SC_GETGR_R_SIZE_MAX: ::c_int = 47;
pub const _SC_GETPW_R_SIZE_MAX: ::c_int = 48;
pub const _SC_CLOCK_SELECTION: ::c_int = 49;
pub const _SC_ASYNCHRONOUS_IO: ::c_int = 50;
pub const _SC_AIO_LISTIO_MAX: ::c_int = 51;
pub const _SC_AIO_MAX: ::c_int = 52;
pub const _SC_MESSAGE_PASSING: ::c_int = 53;
pub const _SC_MQ_OPEN_MAX: ::c_int = 54;
pub const _SC_MQ_PRIO_MAX: ::c_int = 55;
pub const _SC_PRIORITY_SCHEDULING: ::c_int = 56;
pub const _SC_THREAD_DESTRUCTOR_ITERATIONS: ::c_int = 57;
pub const _SC_THREAD_KEYS_MAX: ::c_int = 58;
pub const _SC_THREAD_STACK_MIN: ::c_int = 59;
pub const _SC_THREAD_THREADS_MAX: ::c_int = 60;
pub const _SC_THREAD_ATTR_STACKADDR: ::c_int = 61;
pub const _SC_THREAD_ATTR_STACKSIZE: ::c_int = 62;
pub const _SC_THREAD_PRIORITY_SCHEDULING: ::c_int = 63;
pub const _SC_THREAD_PRIO_INHERIT: ::c_int = 64;
pub const _SC_THREAD_PRIO_PROTECT: ::c_int = 65;
pub const _SC_THREAD_PROCESS_SHARED: ::c_int = 66;
pub const _SC_THREAD_SAFE_FUNCTIONS: ::c_int = 67;
pub const _SC_TTY_NAME_MAX: ::c_int = 68;
pub const _SC_HOST_NAME_MAX: ::c_int = 69;
pub const _SC_PASS_MAX: ::c_int = 70;
pub const _SC_REGEXP: ::c_int = 71;
pub const _SC_SHELL: ::c_int = 72;
pub const _SC_SYMLOOP_MAX: ::c_int = 73;
pub const _SC_V6_ILP32_OFF32: ::c_int = 74;
pub const _SC_V6_ILP32_OFFBIG: ::c_int = 75;
pub const _SC_V6_LP64_OFF64: ::c_int = 76;
pub const _SC_V6_LPBIG_OFFBIG: ::c_int = 77;
pub const _SC_2_PBS: ::c_int = 80;
pub const _SC_2_PBS_ACCOUNTING: ::c_int = 81;
pub const _SC_2_PBS_CHECKPOINT: ::c_int = 82;
pub const _SC_2_PBS_LOCATE: ::c_int = 83;
pub const _SC_2_PBS_MESSAGE: ::c_int = 84;
pub const _SC_2_PBS_TRACK: ::c_int = 85;
pub const _SC_SPAWN: ::c_int = 86;
pub const _SC_SHARED_MEMORY_OBJECTS: ::c_int = 87;
pub const _SC_TIMER_MAX: ::c_int = 88;
pub const _SC_SEM_NSEMS_MAX: ::c_int = 89;
pub const _SC_CPUTIME: ::c_int = 90;
pub const _SC_THREAD_CPUTIME: ::c_int = 91;
pub const _SC_DELAYTIMER_MAX: ::c_int = 92;
// These two variables will be supported in NetBSD 8.0
// pub const _SC_SIGQUEUE_MAX : ::c_int = 93;
// pub const _SC_REALTIME_SIGNALS : ::c_int = 94;
pub const _SC_PHYS_PAGES: ::c_int = 121;
pub const _SC_NPROCESSORS_CONF: ::c_int = 1001;
pub const _SC_NPROCESSORS_ONLN: ::c_int = 1002;
pub const _SC_SCHED_RT_TS: ::c_int = 2001;
pub const _SC_SCHED_PRI_MIN: ::c_int = 2002;
pub const _SC_SCHED_PRI_MAX: ::c_int = 2003;

pub const FD_SETSIZE: usize = 0x100;

pub const ST_NOSUID: ::c_ulong = 8;

pub const BIOCGRSIG: ::c_ulong = 0x40044272;
pub const BIOCSRSIG: ::c_ulong = 0x80044273;
pub const BIOCSDLT: ::c_ulong = 0x80044278;
pub const BIOCGSEESENT: ::c_ulong = 0x40044276;
pub const BIOCSSEESENT: ::c_ulong = 0x80044277;

cfg_if! {
    if #[cfg(any(target_arch = "sparc", target_arch = "sparc64",
                 target_arch = "x86", target_arch = "x86_64"))] {
        pub const PTHREAD_MUTEX_INITIALIZER: pthread_mutex_t
          = pthread_mutex_t {
            ptm_magic: 0x33330003,
            ptm_errorcheck: 0,
            ptm_pad1: [0; 3],
            ptm_unused: 0,
            ptm_pad2: [0; 3],
            ptm_waiters: 0 as *mut _,
            ptm_owner: 0,
            ptm_recursed: 0,
            ptm_spare2: 0 as *mut _,
        };
    } else {
        pub const PTHREAD_MUTEX_INITIALIZER: pthread_mutex_t
          = pthread_mutex_t {
            ptm_magic: 0x33330003,
            ptm_errorcheck: 0,
            ptm_unused: 0,
            ptm_waiters: 0 as *mut _,
            ptm_owner: 0,
            ptm_recursed: 0,
            ptm_spare2: 0 as *mut _,
        };
    }
}

pub const PTHREAD_COND_INITIALIZER: pthread_cond_t = pthread_cond_t {
    ptc_magic: 0x55550005,
    ptc_lock: 0,
    ptc_waiters_first: 0 as *mut _,
    ptc_waiters_last: 0 as *mut _,
    ptc_mutex: 0 as *mut _,
    ptc_private: 0 as *mut _,
};
pub const PTHREAD_RWLOCK_INITIALIZER: pthread_rwlock_t = pthread_rwlock_t {
    ptr_magic: 0x99990009,
    ptr_interlock: 0,
    ptr_rblocked_first: 0 as *mut _,
    ptr_rblocked_last: 0 as *mut _,
    ptr_wblocked_first: 0 as *mut _,
    ptr_wblocked_last: 0 as *mut _,
    ptr_nreaders: 0,
    ptr_owner: 0,
    ptr_private: 0 as *mut _,
};
pub const PTHREAD_MUTEX_NORMAL: ::c_int = 0;
pub const PTHREAD_MUTEX_ERRORCHECK: ::c_int = 1;
pub const PTHREAD_MUTEX_RECURSIVE: ::c_int = 2;
pub const PTHREAD_MUTEX_DEFAULT: ::c_int = PTHREAD_MUTEX_NORMAL;

pub const EVFILT_AIO: u32 = 2;
pub const EVFILT_PROC: u32 = 4;
pub const EVFILT_READ: u32 = 0;
pub const EVFILT_SIGNAL: u32 = 5;
pub const EVFILT_TIMER: u32 = 6;
pub const EVFILT_VNODE: u32 = 3;
pub const EVFILT_WRITE: u32 = 1;

pub const EV_ADD: u32 = 0x1;
pub const EV_DELETE: u32 = 0x2;
pub const EV_ENABLE: u32 = 0x4;
pub const EV_DISABLE: u32 = 0x8;
pub const EV_ONESHOT: u32 = 0x10;
pub const EV_CLEAR: u32 = 0x20;
pub const EV_RECEIPT: u32 = 0x40;
pub const EV_DISPATCH: u32 = 0x80;
pub const EV_FLAG1: u32 = 0x2000;
pub const EV_ERROR: u32 = 0x4000;
pub const EV_EOF: u32 = 0x8000;
pub const EV_SYSFLAGS: u32 = 0xf000;

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

pub const TMP_MAX: ::c_uint = 308915776;

pub const NI_MAXHOST: ::socklen_t = 1025;

pub const RTLD_NOLOAD: ::c_int = 0x2000;
pub const RTLD_LOCAL: ::c_int = 0x200;

pub const CTL_MAXNAME: ::c_int = 12;
pub const SYSCTL_NAMELEN: ::c_int = 32;
pub const SYSCTL_DEFSIZE: ::c_int = 8;
pub const CTLTYPE_NODE: ::c_int = 1;
pub const CTLTYPE_INT: ::c_int = 2;
pub const CTLTYPE_STRING: ::c_int = 3;
pub const CTLTYPE_QUAD: ::c_int = 4;
pub const CTLTYPE_STRUCT: ::c_int = 5;
pub const CTLTYPE_BOOL: ::c_int = 6;
pub const CTLFLAG_READONLY: ::c_int = 0x00000000;
pub const CTLFLAG_READWRITE: ::c_int = 0x00000070;
pub const CTLFLAG_ANYWRITE: ::c_int = 0x00000080;
pub const CTLFLAG_PRIVATE: ::c_int = 0x00000100;
pub const CTLFLAG_PERMANENT: ::c_int = 0x00000200;
pub const CTLFLAG_OWNDATA: ::c_int = 0x00000400;
pub const CTLFLAG_IMMEDIATE: ::c_int = 0x00000800;
pub const CTLFLAG_HEX: ::c_int = 0x00001000;
pub const CTLFLAG_ROOT: ::c_int = 0x00002000;
pub const CTLFLAG_ANYNUMBER: ::c_int = 0x00004000;
pub const CTLFLAG_HIDDEN: ::c_int = 0x00008000;
pub const CTLFLAG_ALIAS: ::c_int = 0x00010000;
pub const CTLFLAG_MMAP: ::c_int = 0x00020000;
pub const CTLFLAG_OWNDESC: ::c_int = 0x00040000;
pub const CTLFLAG_UNSIGNED: ::c_int = 0x00080000;
pub const SYSCTL_VERS_MASK: ::c_int = 0xff000000;
pub const SYSCTL_VERS_0: ::c_int = 0x00000000;
pub const SYSCTL_VERS_1: ::c_int = 0x01000000;
pub const SYSCTL_VERSION: ::c_int = SYSCTL_VERS_1;
pub const CTL_EOL: ::c_int = -1;
pub const CTL_QUERY: ::c_int = -2;
pub const CTL_CREATE: ::c_int = -3;
pub const CTL_CREATESYM: ::c_int = -4;
pub const CTL_DESTROY: ::c_int = -5;
pub const CTL_MMAP: ::c_int = -6;
pub const CTL_DESCRIBE: ::c_int = -7;
pub const CTL_UNSPEC: ::c_int = 0;
pub const CTL_KERN: ::c_int = 1;
pub const CTL_VM: ::c_int = 2;
pub const CTL_VFS: ::c_int = 3;
pub const CTL_NET: ::c_int = 4;
pub const CTL_DEBUG: ::c_int = 5;
pub const CTL_HW: ::c_int = 6;
pub const CTL_MACHDEP: ::c_int = 7;
pub const CTL_USER: ::c_int = 8;
pub const CTL_DDB: ::c_int = 9;
pub const CTL_PROC: ::c_int = 10;
pub const CTL_VENDOR: ::c_int = 11;
pub const CTL_EMUL: ::c_int = 12;
pub const CTL_SECURITY: ::c_int = 13;
pub const CTL_MAXID: ::c_int = 14;
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
pub const KERN_OBOOTTIME: ::c_int = 21;
pub const KERN_DOMAINNAME: ::c_int = 22;
pub const KERN_MAXPARTITIONS: ::c_int = 23;
pub const KERN_RAWPARTITION: ::c_int = 24;
pub const KERN_NTPTIME: ::c_int = 25;
pub const KERN_TIMEX: ::c_int = 26;
pub const KERN_AUTONICETIME: ::c_int = 27;
pub const KERN_AUTONICEVAL: ::c_int = 28;
pub const KERN_RTC_OFFSET: ::c_int = 29;
pub const KERN_ROOT_DEVICE: ::c_int = 30;
pub const KERN_MSGBUFSIZE: ::c_int = 31;
pub const KERN_FSYNC: ::c_int = 32;
pub const KERN_OLDSYSVMSG: ::c_int = 33;
pub const KERN_OLDSYSVSEM: ::c_int = 34;
pub const KERN_OLDSYSVSHM: ::c_int = 35;
pub const KERN_OLDSHORTCORENAME: ::c_int = 36;
pub const KERN_SYNCHRONIZED_IO: ::c_int = 37;
pub const KERN_IOV_MAX: ::c_int = 38;
pub const KERN_MBUF: ::c_int = 39;
pub const KERN_MAPPED_FILES: ::c_int = 40;
pub const KERN_MEMLOCK: ::c_int = 41;
pub const KERN_MEMLOCK_RANGE: ::c_int = 42;
pub const KERN_MEMORY_PROTECTION: ::c_int = 43;
pub const KERN_LOGIN_NAME_MAX: ::c_int = 44;
pub const KERN_DEFCORENAME: ::c_int = 45;
pub const KERN_LOGSIGEXIT: ::c_int = 46;
pub const KERN_PROC2: ::c_int = 47;
pub const KERN_PROC_ARGS: ::c_int = 48;
pub const KERN_FSCALE: ::c_int = 49;
pub const KERN_CCPU: ::c_int = 50;
pub const KERN_CP_TIME: ::c_int = 51;
pub const KERN_OLDSYSVIPC_INFO: ::c_int = 52;
pub const KERN_MSGBUF: ::c_int = 53;
pub const KERN_CONSDEV: ::c_int = 54;
pub const KERN_MAXPTYS: ::c_int = 55;
pub const KERN_PIPE: ::c_int = 56;
pub const KERN_MAXPHYS: ::c_int = 57;
pub const KERN_SBMAX: ::c_int = 58;
pub const KERN_TKSTAT: ::c_int = 59;
pub const KERN_MONOTONIC_CLOCK: ::c_int = 60;
pub const KERN_URND: ::c_int = 61;
pub const KERN_LABELSECTOR: ::c_int = 62;
pub const KERN_LABELOFFSET: ::c_int = 63;
pub const KERN_LWP: ::c_int = 64;
pub const KERN_FORKFSLEEP: ::c_int = 65;
pub const KERN_POSIX_THREADS: ::c_int = 66;
pub const KERN_POSIX_SEMAPHORES: ::c_int = 67;
pub const KERN_POSIX_BARRIERS: ::c_int = 68;
pub const KERN_POSIX_TIMERS: ::c_int = 69;
pub const KERN_POSIX_SPIN_LOCKS: ::c_int = 70;
pub const KERN_POSIX_READER_WRITER_LOCKS: ::c_int = 71;
pub const KERN_DUMP_ON_PANIC: ::c_int = 72;
pub const KERN_SOMAXKVA: ::c_int = 73;
pub const KERN_ROOT_PARTITION: ::c_int = 74;
pub const KERN_DRIVERS: ::c_int = 75;
pub const KERN_BUF: ::c_int = 76;
pub const KERN_FILE2: ::c_int = 77;
pub const KERN_VERIEXEC: ::c_int = 78;
pub const KERN_CP_ID: ::c_int = 79;
pub const KERN_HARDCLOCK_TICKS: ::c_int = 80;
pub const KERN_ARND: ::c_int = 81;
pub const KERN_SYSVIPC: ::c_int = 82;
pub const KERN_BOOTTIME: ::c_int = 83;
pub const KERN_EVCNT: ::c_int = 84;
pub const KERN_MAXID: ::c_int = 85;
pub const KERN_PROC_ALL: ::c_int = 0;
pub const KERN_PROC_PID: ::c_int = 1;
pub const KERN_PROC_PGRP: ::c_int = 2;
pub const KERN_PROC_SESSION: ::c_int = 3;
pub const KERN_PROC_TTY: ::c_int = 4;
pub const KERN_PROC_UID: ::c_int = 5;
pub const KERN_PROC_RUID: ::c_int = 6;
pub const KERN_PROC_GID: ::c_int = 7;
pub const KERN_PROC_RGID: ::c_int = 8;
pub const KERN_PROC_ARGV: ::c_int = 1;
pub const KERN_PROC_NARGV: ::c_int = 2;
pub const KERN_PROC_ENV: ::c_int = 3;
pub const KERN_PROC_NENV: ::c_int = 4;
pub const KERN_PROC_PATHNAME: ::c_int = 5;

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
pub const EAI_OVERFLOW: ::c_int = 14;

pub const AIO_CANCELED: ::c_int = 1;
pub const AIO_NOTCANCELED: ::c_int = 2;
pub const AIO_ALLDONE: ::c_int = 3;
pub const LIO_NOP: ::c_int = 0;
pub const LIO_WRITE: ::c_int = 1;
pub const LIO_READ: ::c_int = 2;
pub const LIO_WAIT: ::c_int = 1;
pub const LIO_NOWAIT: ::c_int = 0;

pub const SIGEV_NONE: ::c_int = 0;
pub const SIGEV_SIGNAL: ::c_int = 1;
pub const SIGEV_THREAD: ::c_int = 2;

pub const WSTOPPED: ::c_int = 0x00000002; // same as WUNTRACED
pub const WCONTINUED: ::c_int = 0x00000010;
pub const WEXITED: ::c_int = 0x000000020;
pub const WNOWAIT: ::c_int = 0x00010000;

pub const P_ALL: idtype_t = 0;
pub const P_PID: idtype_t = 1;
pub const P_PGID: idtype_t = 4;

pub const UTIME_OMIT: c_long = 1073741822;
pub const UTIME_NOW: c_long = 1073741823;

pub const B460800: ::speed_t = 460800;
pub const B921600: ::speed_t = 921600;

pub const ONOCR: ::tcflag_t = 0x20;
pub const ONLRET: ::tcflag_t = 0x40;
pub const CDTRCTS: ::tcflag_t = 0x00020000;
pub const CHWFLOW: ::tcflag_t = ::MDMBUF | ::CRTSCTS | ::CDTRCTS;

pub const SOCK_CLOEXEC: ::c_int = 0x10000000;
pub const SOCK_NONBLOCK: ::c_int = 0x20000000;

// Uncomment on next NetBSD release
// pub const FIOSEEKDATA: ::c_ulong = 0xc0086661;
// pub const FIOSEEKHOLE: ::c_ulong = 0xc0086662;
pub const OFIOGETBMAP: ::c_ulong = 0xc004667a;
pub const FIOGETBMAP: ::c_ulong = 0xc008667a;
pub const FIONWRITE: ::c_ulong = 0x40046679;
pub const FIONSPACE: ::c_ulong = 0x40046678;
pub const FIBMAP: ::c_ulong = 0xc008667a;

pub const SIGSTKSZ: ::size_t = 40960;

pub const PT_DUMPCORE: ::c_int = 12;
pub const PT_LWPINFO: ::c_int = 13;
pub const PT_SYSCALL: ::c_int = 14;
pub const PT_SYSCALLEMU: ::c_int = 15;
pub const PT_SET_EVENT_MASK: ::c_int = 16;
pub const PT_GET_EVENT_MASK: ::c_int = 17;
pub const PT_GET_PROCESS_STATE: ::c_int = 18;
pub const PT_FIRSTMACH: ::c_int = 32;

// Flags for chflags(2)
pub const SF_SNAPSHOT: ::c_ulong = 0x00200000;
pub const SF_LOG: ::c_ulong = 0x00400000;
pub const SF_SNAPINVAL: ::c_ulong = 0x00800000;

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

    pub fn WSTOPSIG(status: ::c_int) -> ::c_int {
        status >> 8
    }

    pub fn WIFSIGNALED(status: ::c_int) -> bool {
        (status & 0o177) != 0o177 && (status & 0o177) != 0
    }

    pub fn WIFSTOPPED(status: ::c_int) -> bool {
        (status & 0o177) == 0o177
    }

    // dirfd() is a macro on netbsd to access
    // the first field of the struct where dirp points to:
    // http://cvsweb.netbsd.org/bsdweb.cgi/src/include/dirent.h?rev=1.36
    pub fn dirfd(dirp: *mut ::DIR) -> ::c_int {
        *(dirp as *const ::c_int)
    }

    pub fn WIFCONTINUED(status: ::c_int) -> bool {
        status == 0xffff
    }

    pub fn SOCKCREDSIZE(ngrps: usize) -> usize {
        let ngrps = if ngrps > 0 {
            ngrps - 1
        } else {
            0
        };
        ::mem::size_of::<sockcred>() + ::mem::size_of::<::gid_t>() * ngrps
    }
}

#[link(name = "rt")]
extern "C" {
    pub fn aio_read(aiocbp: *mut aiocb) -> ::c_int;
    pub fn aio_write(aiocbp: *mut aiocb) -> ::c_int;
    pub fn aio_fsync(op: ::c_int, aiocbp: *mut aiocb) -> ::c_int;
    pub fn aio_error(aiocbp: *const aiocb) -> ::c_int;
    pub fn aio_return(aiocbp: *mut aiocb) -> ::ssize_t;
    #[link_name = "__aio_suspend50"]
    pub fn aio_suspend(
        aiocb_list: *const *const aiocb,
        nitems: ::c_int,
        timeout: *const ::timespec,
    ) -> ::c_int;
    pub fn aio_cancel(fd: ::c_int, aiocbp: *mut aiocb) -> ::c_int;
    pub fn lio_listio(
        mode: ::c_int,
        aiocb_list: *const *mut aiocb,
        nitems: ::c_int,
        sevp: *mut sigevent,
    ) -> ::c_int;
}

extern "C" {
    pub fn chflags(path: *const ::c_char, flags: ::c_ulong) -> ::c_int;
    pub fn fchflags(fd: ::c_int, flags: ::c_ulong) -> ::c_int;
    pub fn lchflags(path: *const ::c_char, flags: ::c_ulong) -> ::c_int;

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
    pub fn extattr_namespace_to_string(
        attrnamespace: ::c_int,
        string: *mut *mut ::c_char,
    ) -> ::c_int;
    pub fn extattr_set_fd(
        fd: ::c_int,
        attrnamespace: ::c_int,
        attrname: *const ::c_char,
        data: *const ::c_void,
        nbytes: ::size_t,
    ) -> ::c_int;
    pub fn extattr_set_file(
        path: *const ::c_char,
        attrnamespace: ::c_int,
        attrname: *const ::c_char,
        data: *const ::c_void,
        nbytes: ::size_t,
    ) -> ::c_int;
    pub fn extattr_set_link(
        path: *const ::c_char,
        attrnamespace: ::c_int,
        attrname: *const ::c_char,
        data: *const ::c_void,
        nbytes: ::size_t,
    ) -> ::c_int;
    pub fn extattr_string_to_namespace(
        string: *const ::c_char,
        attrnamespace: *mut ::c_int,
    ) -> ::c_int;

    #[link_name = "__lutimes50"]
    pub fn lutimes(file: *const ::c_char, times: *const ::timeval) -> ::c_int;
    #[link_name = "__gettimeofday50"]
    pub fn gettimeofday(tp: *mut ::timeval, tz: *mut ::c_void) -> ::c_int;
    pub fn getnameinfo(
        sa: *const ::sockaddr,
        salen: ::socklen_t,
        host: *mut ::c_char,
        hostlen: ::socklen_t,
        serv: *mut ::c_char,
        sevlen: ::socklen_t,
        flags: ::c_int,
    ) -> ::c_int;
    pub fn mprotect(
        addr: *mut ::c_void,
        len: ::size_t,
        prot: ::c_int,
    ) -> ::c_int;
    pub fn sysctl(
        name: *const ::c_int,
        namelen: ::c_uint,
        oldp: *mut ::c_void,
        oldlenp: *mut ::size_t,
        newp: *const ::c_void,
        newlen: ::size_t,
    ) -> ::c_int;
    pub fn sysctlbyname(
        name: *const ::c_char,
        oldp: *mut ::c_void,
        oldlenp: *mut ::size_t,
        newp: *const ::c_void,
        newlen: ::size_t,
    ) -> ::c_int;
    #[link_name = "__kevent50"]
    pub fn kevent(
        kq: ::c_int,
        changelist: *const ::kevent,
        nchanges: ::size_t,
        eventlist: *mut ::kevent,
        nevents: ::size_t,
        timeout: *const ::timespec,
    ) -> ::c_int;
    #[link_name = "__mount50"]
    pub fn mount(
        src: *const ::c_char,
        target: *const ::c_char,
        flags: ::c_int,
        data: *mut ::c_void,
        size: ::size_t,
    ) -> ::c_int;
    pub fn mq_open(name: *const ::c_char, oflag: ::c_int, ...) -> ::mqd_t;
    pub fn mq_close(mqd: ::mqd_t) -> ::c_int;
    pub fn mq_getattr(mqd: ::mqd_t, attr: *mut ::mq_attr) -> ::c_int;
    pub fn mq_notify(mqd: ::mqd_t, notification: *const ::sigevent)
        -> ::c_int;
    pub fn mq_receive(
        mqd: ::mqd_t,
        msg_ptr: *mut ::c_char,
        msg_len: ::size_t,
        msg_prio: *mut ::c_uint,
    ) -> ::ssize_t;
    pub fn mq_send(
        mqd: ::mqd_t,
        msg_ptr: *const ::c_char,
        msg_len: ::size_t,
        msg_prio: ::c_uint,
    ) -> ::c_int;
    pub fn mq_setattr(
        mqd: ::mqd_t,
        newattr: *const ::mq_attr,
        oldattr: *mut ::mq_attr,
    ) -> ::c_int;
    #[link_name = "__mq_timedreceive50"]
    pub fn mq_timedreceive(
        mqd: ::mqd_t,
        msg_ptr: *mut ::c_char,
        msg_len: ::size_t,
        msg_prio: *mut ::c_uint,
        abs_timeout: *const ::timespec,
    ) -> ::ssize_t;
    #[link_name = "__mq_timedsend50"]
    pub fn mq_timedsend(
        mqd: ::mqd_t,
        msg_ptr: *const ::c_char,
        msg_len: ::size_t,
        msg_prio: ::c_uint,
        abs_timeout: *const ::timespec,
    ) -> ::c_int;
    pub fn mq_unlink(name: *const ::c_char) -> ::c_int;
    pub fn ptrace(
        request: ::c_int,
        pid: ::pid_t,
        addr: *mut ::c_void,
        data: ::c_int,
    ) -> ::c_int;
    pub fn pthread_setname_np(
        t: ::pthread_t,
        name: *const ::c_char,
        arg: *mut ::c_void,
    ) -> ::c_int;
    pub fn pthread_getattr_np(
        native: ::pthread_t,
        attr: *mut ::pthread_attr_t,
    ) -> ::c_int;
    pub fn pthread_attr_getguardsize(
        attr: *const ::pthread_attr_t,
        guardsize: *mut ::size_t,
    ) -> ::c_int;
    pub fn pthread_attr_getstack(
        attr: *const ::pthread_attr_t,
        stackaddr: *mut *mut ::c_void,
        stacksize: *mut ::size_t,
    ) -> ::c_int;
    #[link_name = "__sigtimedwait50"]
    pub fn sigtimedwait(
        set: *const sigset_t,
        info: *mut siginfo_t,
        timeout: *const ::timespec,
    ) -> ::c_int;
    pub fn sigwaitinfo(set: *const sigset_t, info: *mut siginfo_t) -> ::c_int;
    pub fn duplocale(base: ::locale_t) -> ::locale_t;
    pub fn freelocale(loc: ::locale_t);
    pub fn localeconv_l(loc: ::locale_t) -> *mut lconv;
    pub fn newlocale(
        mask: ::c_int,
        locale: *const ::c_char,
        base: ::locale_t,
    ) -> ::locale_t;
    #[link_name = "__settimeofday50"]
    pub fn settimeofday(tv: *const ::timeval, tz: *const ::c_void) -> ::c_int;

    pub fn dup3(src: ::c_int, dst: ::c_int, flags: ::c_int) -> ::c_int;

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

    pub fn _lwp_self() -> lwpid_t;
}

#[link(name = "util")]
extern "C" {
    #[cfg_attr(target_os = "netbsd", link_name = "__getpwent_r50")]
    pub fn getpwent_r(
        pwd: *mut ::passwd,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut ::passwd,
    ) -> ::c_int;
    pub fn getgrent_r(
        grp: *mut ::group,
        buf: *mut ::c_char,
        buflen: ::size_t,
        result: *mut *mut ::group,
    ) -> ::c_int;
}

cfg_if! {
    if #[cfg(target_arch = "aarch64")] {
        mod aarch64;
        pub use self::aarch64::*;
    } else if #[cfg(target_arch = "arm")] {
        mod arm;
        pub use self::arm::*;
    } else if #[cfg(target_arch = "powerpc")] {
        mod powerpc;
        pub use self::powerpc::*;
    } else if #[cfg(target_arch = "sparc64")] {
        mod sparc64;
        pub use self::sparc64::*;
    } else if #[cfg(target_arch = "x86_64")] {
        mod x86_64;
        pub use self::x86_64::*;
    } else if #[cfg(target_arch = "x86")] {
        mod x86;
        pub use self::x86::*;
    } else {
        // Unknown target_arch
    }
}
