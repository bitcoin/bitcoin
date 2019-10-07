cfg_if! {
    if #[cfg(target_pointer_width = "64")] {
        s_no_extra_traits! {
            #[allow(missing_debug_implementations)]
            #[repr(align(16))]
            pub struct max_align_t {
                priv_: [f64; 4]
            }
        }
    } else if #[cfg(target_pointer_width = "32")] {
        s_no_extra_traits! {
            #[allow(missing_debug_implementations)]
            #[repr(align(16))]
            pub struct max_align_t {
                priv_: [i64; 6]
            }
        }
    }
}
