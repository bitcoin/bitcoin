macro_rules! expand_align {
    () => {
        s! {
            #[cfg_attr(target_pointer_width = "32",
                       repr(align(4)))]
            #[cfg_attr(target_pointer_width = "64",
                       repr(align(8)))]
            pub struct sem_t { // ToDo
                #[cfg(target_pointer_width = "32")]
                __size: [::c_char; 16],
                #[cfg(target_pointer_width = "64")]
                __size: [::c_char; 32],
            }

            #[cfg_attr(any(target_pointer_width = "32",
                           target_arch = "x86_64",
                           target_arch = "powerpc64",
                           target_arch = "mips64",
                           target_arch = "s390x",
                           target_arch = "sparc64"),
                       repr(align(4)))]
            #[cfg_attr(not(any(target_pointer_width = "32",
                               target_arch = "x86_64",
                               target_arch = "powerpc64",
                               target_arch = "mips64",
                               target_arch = "s390x",
                               target_arch = "sparc64")),
                       repr(align(8)))]
            pub struct pthread_mutexattr_t { // ToDo
                size: [u8; ::__SIZEOF_PTHREAD_MUTEXATTR_T],
            }

            #[repr(align(4))]
            pub struct pthread_condattr_t { // ToDo
                size: [u8; ::__SIZEOF_PTHREAD_CONDATTR_T],
            }
        }

        s_no_extra_traits! {
            #[cfg_attr(all(target_pointer_width = "32",
                           any(target_arch = "mips",
                               target_arch = "arm",
                               target_arch = "powerpc")),
                       repr(align(4)))]
            #[cfg_attr(all(any(target_pointer_width = "64",
                               not(any(target_arch = "mips",
                                       target_arch = "arm",
                                       target_arch = "powerpc")))),
                       repr(align(8)))]
            #[allow(missing_debug_implementations)]
            pub struct pthread_mutex_t { // ToDo
                size: [u8; ::__SIZEOF_PTHREAD_MUTEX_T],
            }

            #[repr(align(8))]
            #[allow(missing_debug_implementations)]
            pub struct pthread_cond_t { // ToDo
                size: [u8; ::__SIZEOF_PTHREAD_COND_T],
            }

            #[cfg_attr(all(target_pointer_width = "32",
                           any(target_arch = "mips",
                               target_arch = "arm",
                               target_arch = "powerpc")),
                       repr(align(4)))]
            #[cfg_attr(any(target_pointer_width = "64",
                           not(any(target_arch = "mips",
                                   target_arch = "arm",
                                   target_arch = "powerpc"))),
                       repr(align(8)))]
            #[allow(missing_debug_implementations)]
            pub struct pthread_rwlock_t { // ToDo
                size: [u8; ::__SIZEOF_PTHREAD_RWLOCK_T],
            }
        }
    };
}
