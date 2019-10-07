s_no_extra_traits! {
    #[allow(missing_debug_implementations)]
    #[repr(align(8))]
    pub struct max_align_t {
        priv_: [f64; 2]
    }
}
