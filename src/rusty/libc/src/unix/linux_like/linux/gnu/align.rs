s! {
    // FIXME this is actually a union
    #[cfg_attr(target_pointer_width = "32",
               repr(align(4)))]
    #[cfg_attr(target_pointer_width = "64",
               repr(align(8)))]
    pub struct sem_t {
        #[cfg(target_pointer_width = "32")]
        __size: [::c_char; 16],
        #[cfg(target_pointer_width = "64")]
        __size: [::c_char; 32],
    }
}
