macro_rules! expand_align {
    () => {
        s! {
            pub struct pthread_mutexattr_t {
                #[cfg(target_arch = "x86_64")]
                __align: [::c_int; 0],
                #[cfg(not(target_arch = "x86_64"))]
                __align: [::c_long; 0],
                size: [u8; ::__SIZEOF_PTHREAD_MUTEXATTR_T],
            }

            pub struct pthread_rwlockattr_t {
                __align: [::c_long; 0],
                size: [u8; ::__SIZEOF_PTHREAD_RWLOCKATTR_T],
            }

            pub struct pthread_condattr_t {
                __align: [::c_int; 0],
                size: [u8; ::__SIZEOF_PTHREAD_CONDATTR_T],
            }
        }

        s_no_extra_traits! {
            pub struct pthread_mutex_t {
                #[cfg(any(target_arch = "arm",
                          all(target_arch = "x86_64",
                              target_pointer_width = "32")))]
                __align: [::c_long; 0],
                #[cfg(not(any(target_arch = "arm",
                              all(target_arch = "x86_64",
                                  target_pointer_width = "32"))))]
                __align: [::c_longlong; 0],
                size: [u8; ::__SIZEOF_PTHREAD_MUTEX_T],
            }

            pub struct pthread_rwlock_t {
                __align: [::c_long; 0],
                __align: [::c_longlong; 0],
                size: [u8; ::__SIZEOF_PTHREAD_RWLOCK_T],
            }

            pub struct pthread_cond_t {
                __align: [*const ::c_void; 0],
                #[cfg(not(target_env = "musl"))]
                __align: [::c_longlong; 0],
                size: [u8; ::__SIZEOF_PTHREAD_COND_T],
            }
        }

        cfg_if! {
            if #[cfg(feature = "extra_traits")] {
                impl PartialEq for pthread_cond_t {
                    fn eq(&self, other: &pthread_cond_t) -> bool {
                        // Ignore __align field
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
                            // Ignore __align field
                            // FIXME: .field("size", &self.size)
                            .finish()
                    }
                }
                impl ::hash::Hash for pthread_cond_t {
                    fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                        // Ignore __align field
                        self.size.hash(state);
                    }
                }

                impl PartialEq for pthread_mutex_t {
                    fn eq(&self, other: &pthread_mutex_t) -> bool {
                        // Ignore __align field
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
                            // Ignore __align field
                            // FIXME: .field("size", &self.size)
                            .finish()
                    }
                }
                impl ::hash::Hash for pthread_mutex_t {
                    fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                        // Ignore __align field
                        self.size.hash(state);
                    }
                }

                impl PartialEq for pthread_rwlock_t {
                    fn eq(&self, other: &pthread_rwlock_t) -> bool {
                        // Ignore __align field
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
                            // Ignore __align field
                            // FIXME: .field("size", &self.size)
                            .finish()
                    }
                }
                impl ::hash::Hash for pthread_rwlock_t {
                    fn hash<H: ::hash::Hasher>(&self, state: &mut H) {
                        // Ignore __align field
                        self.size.hash(state);
                    }
                }
            }
        }
    };
}
