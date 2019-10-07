s! {
    // FIXME this is actually a union
    pub struct sem_t {
        __size: [::c_char; 32],
        __align: [::c_long; 0],
    }
}
