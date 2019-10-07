s! {
    #[repr(align(4))]
    pub struct in6_addr {
        pub s6_addr: [u8; 16],
    }
}
