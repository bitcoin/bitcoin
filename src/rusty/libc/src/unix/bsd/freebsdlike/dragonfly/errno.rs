// DragonFlyBSD's __error function is declared with "static inline", so it must
// be implemented in the libc crate, as a pointer to a static thread_local.
f! {
    pub fn __error() -> *mut ::c_int {
        &mut errno
    }
}

extern "C" {
    #[thread_local]
    pub static mut errno: ::c_int;
}
