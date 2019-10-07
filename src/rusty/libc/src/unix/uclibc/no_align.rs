macro_rules! expand_align {
    () => {
        s! {
            pub struct pthread_mutex_t {
                #[cfg(any(target_arch = "mips",
                          target_arch = "arm",
                          target_arch = "powerpc"))]
                __align: [::c_long; 0],
                #[cfg(any(libc_align,
                          target_arch = "mips",
                          target_arch = "arm",
                          target_arch = "powerpc"))]
                __align: [::c_longlong; 0],
                size: [u8; ::__SIZEOF_PTHREAD_MUTEX_T],
            }

            pub struct pthread_rwlock_t {
                #[cfg(any(target_arch = "mips",
                          target_arch = "arm",
                          target_arch = "powerpc"))]
                __align: [::c_long; 0],
                #[cfg(not(any(
                    target_arch = "mips",
                    target_arch = "arm",
                    target_arch = "powerpc")))]
                __align: [::c_longlong; 0],
                size: [u8; ::__SIZEOF_PTHREAD_RWLOCK_T],
            }

            pub struct pthread_mutexattr_t {
                #[cfg(any(target_arch = "x86_64", target_arch = "powerpc64",
                          target_arch = "mips64", target_arch = "s390x",
                          target_arch = "sparc64"))]
                __align: [::c_int; 0],
                #[cfg(not(any(target_arch = "x86_64", target_arch = "powerpc64",
                              target_arch = "mips64", target_arch = "s390x",
                              target_arch = "sparc64")))]
                __align: [::c_long; 0],
                size: [u8; ::__SIZEOF_PTHREAD_MUTEXATTR_T],
            }

            pub struct pthread_cond_t {
                __align: [::c_longlong; 0],
                size: [u8; ::__SIZEOF_PTHREAD_COND_T],
            }

            pub struct pthread_condattr_t {
                __align: [::c_int; 0],
                size: [u8; ::__SIZEOF_PTHREAD_CONDATTR_T],
            }
        }
    }
}
