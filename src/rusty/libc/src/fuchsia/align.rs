macro_rules! expand_align {
    () => {
        s! {
            #[cfg_attr(
                any(
                    target_pointer_width = "32",
                    target_arch = "x86_64"
                ),
                repr(align(4)))]
            #[cfg_attr(
                not(any(
                    target_pointer_width = "32",
                    target_arch = "x86_64"
                )),
                repr(align(8)))]
            pub struct pthread_mutexattr_t {
                size: [u8; ::__SIZEOF_PTHREAD_MUTEXATTR_T],
            }

            #[cfg_attr(target_pointer_width = "32",
                       repr(align(4)))]
            #[cfg_attr(target_pointer_width = "64",
                       repr(align(8)))]
            pub struct pthread_rwlockattr_t {
                size: [u8; ::__SIZEOF_PTHREAD_RWLOCKATTR_T],
            }

            #[repr(align(4))]
            pub struct pthread_condattr_t {
                size: [u8; ::__SIZEOF_PTHREAD_CONDATTR_T],
            }
        }

        s_no_extra_traits! {
            #[cfg_attr(all(target_pointer_width = "32",
                           any(target_arch = "arm",
                               target_arch = "x86_64")),
                       repr(align(4)))]
            #[cfg_attr(any(target_pointer_width = "64",
                           not(any(target_arch = "arm",
                                   target_arch = "x86_64"))),
                       repr(align(8)))]
            pub struct pthread_mutex_t {
                size: [u8; ::__SIZEOF_PTHREAD_MUTEX_T],
            }

            #[cfg_attr(all(target_pointer_width = "32",
                           any(target_arch = "arm",
                               target_arch = "x86_64")),
                       repr(align(4)))]
            #[cfg_attr(any(target_pointer_width = "64",
                           not(any(target_arch = "arm",
                                   target_arch = "x86_64"))),
                       repr(align(8)))]
            pub struct pthread_rwlock_t {
                size: [u8; ::__SIZEOF_PTHREAD_RWLOCK_T],
            }

            #[cfg_attr(target_pointer_width = "32",
                       repr(align(4)))]
            #[cfg_attr(target_pointer_width = "64",
                       repr(align(8)))]
            #[cfg_attr(target_arch = "x86",
                       repr(align(4)))]
            #[cfg_attr(not(target_arch = "x86"),
                       repr(align(8)))]
            pub struct pthread_cond_t {
                size: [u8; ::__SIZEOF_PTHREAD_COND_T],
            }
        }

        cfg_if! {
            if #[cfg(feature = "extra_traits")] {
                impl PartialEq for pthread_cond_t {
                    fn eq(&self, other: &pthread_cond_t) -> bool {
                        self.size
                            .iter()
                            .zip(other.size.iter())
                            .all(|(a,b)| a == b)
                    }
                }
                impl Eq for pthread_cond_t {}
                impl ::fmt::Debug for pthread_cond_t {
                    fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                        f.debug_struct("pthread_cond_t")
                            // FIXME: .field("size", &self.size)
                            .finish()
                    }
                }
                impl ::hash::Hash for pthread_cond_t {
                    fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                        self.size.hash(state);
                    }
                }

                impl PartialEq for pthread_mutex_t {
                    fn eq(&self, other: &pthread_mutex_t) -> bool {
                        self.size
                            .iter()
                            .zip(other.size.iter())
                            .all(|(a,b)| a == b)
                    }
                }
                impl Eq for pthread_mutex_t {}
                impl ::fmt::Debug for pthread_mutex_t {
                    fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                        f.debug_struct("pthread_mutex_t")
                            // FIXME: .field("size", &self.size)
                            .finish()
                    }
                }
                impl ::hash::Hash for pthread_mutex_t {
                    fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                        self.size.hash(state);
                    }
                }

                impl PartialEq for pthread_rwlock_t {
                    fn eq(&self, other: &pthread_rwlock_t) -> bool {
                        self.size
                            .iter()
                            .zip(other.size.iter())
                            .all(|(a,b)| a == b)
                    }
                }
                impl Eq for pthread_rwlock_t {}
                impl ::fmt::Debug for pthread_rwlock_t {
                    fn fmt(&self, f: &mut ::fmt::Formatter) -> ::fmt::Result {
                        f.debug_struct("pthread_rwlock_t")
                            // FIXME: .field("size", &self.size)
                            .finish()
                    }
                }
                impl ::hash::Hash for pthread_rwlock_t {
                    fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                        self.size.hash(state);
                    }
                }
            }
        }
    };
}
