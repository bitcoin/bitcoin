macro_rules! expand_align {
    () => {
        s! {
            pub struct sem_t { // ToDo
                #[cfg(target_pointer_width = "32")]
                __size: [::c_char; 16],
                #[cfg(target_pointer_width = "64")]
                __size: [::c_char; 32],
                __align: [::c_long; 0],
            }

            pub struct pthread_mutex_t { // ToDo
                #[cfg(any(target_arch = "mips",
                          target_arch = "arm",
                          target_arch = "powerpc"))]
                __align: [::c_long; 0],
                #[cfg(not(any(target_arch = "mips",
                              target_arch = "arm",
                              target_arch = "powerpc")))]
                __align: [::c_longlong; 0],
                size: [u8; ::__SIZEOF_PTHREAD_MUTEX_T],
            }

            pub struct pthread_mutexattr_t { // ToDo
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

            pub struct pthread_cond_t { // ToDo
                __align: [::c_longlong; 0],
                size: [u8; ::__SIZEOF_PTHREAD_COND_T],
            }

            pub struct pthread_condattr_t { // ToDo
                __align: [::c_int; 0],
                size: [u8; ::__SIZEOF_PTHREAD_CONDATTR_T],
            }

            pub struct pthread_rwlock_t { // ToDo
                #[cfg(any(target_arch = "mips",
                          target_arch = "arm",
                          target_arch = "powerpc"))]
                __align: [::c_long; 0],
                #[cfg(not(any(target_arch = "mips",
                              target_arch = "arm",
                              target_arch = "powerpc")))]
                __align: [::c_longlong; 0],
                size: [u8; ::__SIZEOF_PTHREAD_RWLOCK_T],
            }
        }
    }
}
