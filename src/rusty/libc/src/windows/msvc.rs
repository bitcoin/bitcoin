pub const L_tmpnam: ::c_uint = 260;
pub const TMP_MAX: ::c_uint = 0x7fff_ffff;

extern "C" {
    #[link_name = "_stricmp"]
    pub fn stricmp(s1: *const ::c_char, s2: *const ::c_char) -> ::c_int;
    #[link_name = "_strnicmp"]
    pub fn strnicmp(
        s1: *const ::c_char,
        s2: *const ::c_char,
        n: ::size_t,
    ) -> ::c_int;
}
