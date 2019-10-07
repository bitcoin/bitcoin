//! Compare libc's CMSG(3) family of functions against the actual C macros, for
//! various inputs.

extern crate libc;

#[cfg(unix)]
mod t {

    use libc::{self, c_uchar, c_uint, c_void, cmsghdr, msghdr};
    use std::mem;

    extern "C" {
        pub fn cmsg_firsthdr(msgh: *const msghdr) -> *mut cmsghdr;
        pub fn cmsg_nxthdr(
            mhdr: *const msghdr,
            cmsg: *const cmsghdr,
        ) -> *mut cmsghdr;
        pub fn cmsg_space(length: c_uint) -> usize;
        pub fn cmsg_len(length: c_uint) -> usize;
        pub fn cmsg_data(cmsg: *const cmsghdr) -> *mut c_uchar;
    }

    #[test]
    fn test_cmsg_data() {
        for l in 0..128 {
            let pcmsghdr = l as *const cmsghdr;
            unsafe {
                assert_eq!(libc::CMSG_DATA(pcmsghdr), cmsg_data(pcmsghdr));
            }
        }
    }

    #[test]
    fn test_cmsg_firsthdr() {
        let mut mhdr: msghdr = unsafe { mem::zeroed() };
        mhdr.msg_control = 0xdeadbeef as *mut c_void;
        let pmhdr = &mhdr as *const msghdr;
        for l in 0..128 {
            mhdr.msg_controllen = l;
            unsafe {
                assert_eq!(libc::CMSG_FIRSTHDR(pmhdr), cmsg_firsthdr(pmhdr));
            }
        }
    }

    #[test]
    fn test_cmsg_len() {
        for l in 0..128 {
            unsafe {
                assert_eq!(libc::CMSG_LEN(l) as usize, cmsg_len(l));
            }
        }
    }

    // Skip on sparc64
    // https://github.com/rust-lang/libc/issues/1239
    #[cfg(not(target_arch = "sparc64"))]
    #[test]
    fn test_cmsg_nxthdr() {
        use std::ptr;

        let mut buffer = [0u8; 256];
        let mut mhdr: msghdr = unsafe { mem::zeroed() };
        let pmhdr = &mhdr as *const msghdr;
        for start_ofs in 0..64 {
            let pcmsghdr = &mut buffer[start_ofs] as *mut u8 as *mut cmsghdr;
            mhdr.msg_control = pcmsghdr as *mut c_void;
            mhdr.msg_controllen = (160 - start_ofs) as _;
            for cmsg_len in 0..64 {
                for next_cmsg_len in 0..32 {
                    for i in buffer[start_ofs..].iter_mut() {
                        *i = 0;
                    }
                    unsafe {
                        (*pcmsghdr).cmsg_len = cmsg_len;
                        let libc_next = libc::CMSG_NXTHDR(pmhdr, pcmsghdr);
                        let next = cmsg_nxthdr(pmhdr, pcmsghdr);
                        assert_eq!(libc_next, next);

                        if libc_next != ptr::null_mut() {
                            (*libc_next).cmsg_len = next_cmsg_len;
                            let libc_next = libc::CMSG_NXTHDR(pmhdr, pcmsghdr);
                            let next = cmsg_nxthdr(pmhdr, pcmsghdr);
                            assert_eq!(libc_next, next);
                        }
                    }
                }
            }
        }
    }

    #[test]
    fn test_cmsg_space() {
        unsafe {
            for l in 0..128 {
                assert_eq!(libc::CMSG_SPACE(l) as usize, cmsg_space(l));
            }
        }
    }
}
