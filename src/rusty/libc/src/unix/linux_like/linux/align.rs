macro_rules! expand_align {
    () => {
        s! {
            #[cfg_attr(any(target_pointer_width = "32",
                           target_arch = "x86_64",
                           target_arch = "powerpc64",
                           target_arch = "mips64",
                           target_arch = "s390x",
                           target_arch = "sparc64",
                           target_arch = "aarch64"),
                       repr(align(4)))]
            #[cfg_attr(not(any(target_pointer_width = "32",
                               target_arch = "x86_64",
                               target_arch = "powerpc64",
                               target_arch = "mips64",
                               target_arch = "s390x",
                               target_arch = "sparc64",
                               target_arch = "aarch64")),
                       repr(align(8)))]
            pub struct pthread_mutexattr_t {
                #[doc(hidden)]
                size: [u8; ::__SIZEOF_PTHREAD_MUTEXATTR_T],
            }

            #[cfg_attr(any(target_env = "musl", target_pointer_width = "32"),
                       repr(align(4)))]
            #[cfg_attr(all(not(target_env = "musl"),
                           target_pointer_width = "64"),
                       repr(align(8)))]
            pub struct pthread_rwlockattr_t {
                #[doc(hidden)]
                size: [u8; ::__SIZEOF_PTHREAD_RWLOCKATTR_T],
            }

            #[repr(align(4))]
            pub struct pthread_condattr_t {
                #[doc(hidden)]
                size: [u8; ::__SIZEOF_PTHREAD_CONDATTR_T],
            }
        }

        s_no_extra_traits! {
            #[cfg_attr(all(target_env = "musl",
                           target_pointer_width = "32"),
                       repr(align(4)))]
            #[cfg_attr(all(target_env = "musl",
                           target_pointer_width = "64"),
                       repr(align(8)))]
            #[cfg_attr(all(not(target_env = "musl"),
                           target_arch = "x86"),
                       repr(align(4)))]
            #[cfg_attr(all(not(target_env = "musl"),
                           not(target_arch = "x86")),
                       repr(align(8)))]
            pub struct pthread_cond_t {
                #[doc(hidden)]
                size: [u8; ::__SIZEOF_PTHREAD_COND_T],
            }

            #[cfg_attr(all(target_pointer_width = "32",
                           any(target_arch = "mips",
                               target_arch = "arm",
                               target_arch = "hexagon",
                               target_arch = "powerpc",
                               target_arch = "x86_64",
                               target_arch = "x86")),
                       repr(align(4)))]
            #[cfg_attr(any(target_pointer_width = "64",
                           not(any(target_arch = "mips",
                                   target_arch = "arm",
                                   target_arch = "hexagon",
                                   target_arch = "powerpc",
                                   target_arch = "x86_64",
                                   target_arch = "x86"))),
                       repr(align(8)))]
            pub struct pthread_mutex_t {
                #[doc(hidden)]
                size: [u8; ::__SIZEOF_PTHREAD_MUTEX_T],
            }

            #[cfg_attr(all(target_pointer_width = "32",
                           any(target_arch = "mips",
                               target_arch = "arm",
                               target_arch = "hexagon",
                               target_arch = "powerpc",
                               target_arch = "x86_64",
                               target_arch = "x86")),
                       repr(align(4)))]
            #[cfg_attr(any(target_pointer_width = "64",
                           not(any(target_arch = "mips",
                                   target_arch = "arm",
                                   target_arch = "hexagon",
                                   target_arch = "powerpc",
                                   target_arch = "x86_64",
                                   target_arch = "x86"))),
                       repr(align(8)))]
            pub struct pthread_rwlock_t {
                size: [u8; ::__SIZEOF_PTHREAD_RWLOCK_T],
            }
        }
    };
}
