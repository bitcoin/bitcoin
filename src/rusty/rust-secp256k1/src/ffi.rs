// Bitcoin secp256k1 bindings
// Written in 2014 by
//   Dawid Ciężarkiewicz
//   Andrew Poelstra
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to
// the public domain worldwide. This software is distributed without
// any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication
// along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
//

//! # FFI bindings
//! Direct bindings to the underlying C library functions. These should
//! not be needed for most users.
use core::{mem, hash, slice, ptr};
use types::*;

/// Flag for context to enable no precomputation
pub const SECP256K1_START_NONE: c_uint = 1;
/// Flag for context to enable verification precomputation
pub const SECP256K1_START_VERIFY: c_uint = 1 | (1 << 8);
/// Flag for context to enable signing precomputation
pub const SECP256K1_START_SIGN: c_uint = 1 | (1 << 9);
/// Flag for keys to indicate uncompressed serialization format
pub const SECP256K1_SER_UNCOMPRESSED: c_uint = (1 << 1);
/// Flag for keys to indicate compressed serialization format
pub const SECP256K1_SER_COMPRESSED: c_uint = (1 << 1) | (1 << 8);

/// A nonce generation function. Ordinary users of the library
/// never need to see this type; only if you need to control
/// nonce generation do you need to use it. I have deliberately
/// made this hard to do: you have to write your own wrapper
/// around the FFI functions to use it. And it's an unsafe type.
/// Nonces are generated deterministically by RFC6979 by
/// default; there should be no need to ever change this.
pub type NonceFn = unsafe extern "C" fn(nonce32: *mut c_uchar,
                                        msg32: *const c_uchar,
                                        key32: *const c_uchar,
                                        algo16: *const c_uchar,
                                        attempt: c_uint,
                                        data: *const c_void);

/// Hash function to use to post-process an ECDH point to get
/// a shared secret.
pub type EcdhHashFn = unsafe extern "C" fn(
    output: *mut c_uchar,
    x: *const c_uchar,
    y: *const c_uchar,
    data: *const c_void,
);

/// A Secp256k1 context, containing various precomputed values and such
/// needed to do elliptic curve computations. If you create one of these
/// with `secp256k1_context_create` you MUST destroy it with
/// `secp256k1_context_destroy`, or else you will have a memory leak.
#[derive(Clone, Debug)]
#[repr(C)] pub struct Context(c_int);

#[cfg(feature = "fuzztarget")]
impl Context {
    pub fn flags(&self) -> u32 {
        self.0 as u32
    }
}

/// Library-internal representation of a Secp256k1 public key
#[repr(C)]
pub struct PublicKey([c_uchar; 64]);
impl_array_newtype!(PublicKey, c_uchar, 64);
impl_raw_debug!(PublicKey);

impl PublicKey {
    /// Create a new (zeroed) public key usable for the FFI interface
    pub fn new() -> PublicKey { PublicKey([0; 64]) }
    /// Create a new (uninitialized) public key usable for the FFI interface
    #[deprecated(since = "0.15.3", note = "Please use the new function instead")]
    pub unsafe fn blank() -> PublicKey { PublicKey::new() }
}

impl Default for PublicKey {
    fn default() -> Self {
        PublicKey::new()
    }
}

impl hash::Hash for PublicKey {
    fn hash<H: hash::Hasher>(&self, state: &mut H) {
        state.write(&self.0)
    }
}

/// Library-internal representation of a Secp256k1 signature
#[repr(C)]
pub struct Signature([c_uchar; 64]);
impl_array_newtype!(Signature, c_uchar, 64);
impl_raw_debug!(Signature);

impl Signature {
    /// Create a new (zeroed) signature usable for the FFI interface
    pub fn new() -> Signature { Signature([0; 64]) }
    /// Create a new (uninitialized) signature usable for the FFI interface
    #[deprecated(since = "0.15.3", note = "Please use the new function instead")]
    pub unsafe fn blank() -> Signature { Signature::new() }
}

impl Default for Signature {
    fn default() -> Self {
        Signature::new()
    }
}

/// Library-internal representation of an ECDH shared secret
#[repr(C)]
pub struct SharedSecret([c_uchar; 32]);
impl_array_newtype!(SharedSecret, c_uchar, 32);
impl_raw_debug!(SharedSecret);

impl SharedSecret {
    /// Create a new (zeroed) signature usable for the FFI interface
    pub fn new() -> SharedSecret { SharedSecret([0; 32]) }
    /// Create a new (uninitialized) signature usable for the FFI interface
    #[deprecated(since = "0.15.3", note = "Please use the new function instead")]
    pub unsafe fn blank() -> SharedSecret { SharedSecret::new() }
}

impl Default for SharedSecret {
    fn default() -> Self {
        SharedSecret::new()
    }
}

#[cfg(not(feature = "fuzztarget"))]
extern "C" {
    /// Default ECDH hash function
    pub static secp256k1_ecdh_hash_function_default: EcdhHashFn;

    pub static secp256k1_nonce_function_rfc6979: NonceFn;

    pub static secp256k1_nonce_function_default: NonceFn;

    pub static secp256k1_context_no_precomp: *const Context;

    // Contexts
    pub fn secp256k1_context_preallocated_size(flags: c_uint) -> usize;

    pub fn secp256k1_context_preallocated_create(prealloc: *mut c_void, flags: c_uint) -> *mut Context;

    pub fn secp256k1_context_preallocated_destroy(cx: *mut Context);

    pub fn secp256k1_context_preallocated_clone_size(cx: *const Context) -> usize;

    pub fn secp256k1_context_preallocated_clone(cx: *const Context, prealloc: *mut c_void) -> *mut Context;

    pub fn secp256k1_context_randomize(cx: *mut Context,
                                       seed32: *const c_uchar)
                                       -> c_int;

    // Pubkeys
    pub fn secp256k1_ec_pubkey_parse(cx: *const Context, pk: *mut PublicKey,
                                     input: *const c_uchar, in_len: usize)
                                     -> c_int;

    pub fn secp256k1_ec_pubkey_serialize(cx: *const Context, output: *mut c_uchar,
                                         out_len: *mut usize, pk: *const PublicKey,
                                         compressed: c_uint)
                                         -> c_int;

    // Signatures
    pub fn secp256k1_ecdsa_signature_parse_der(cx: *const Context, sig: *mut Signature,
                                               input: *const c_uchar, in_len: usize)
                                               -> c_int;

    pub fn secp256k1_ecdsa_signature_parse_compact(cx: *const Context, sig: *mut Signature,
                                                   input64: *const c_uchar)
                                                   -> c_int;

    pub fn ecdsa_signature_parse_der_lax(cx: *const Context, sig: *mut Signature,
                                         input: *const c_uchar, in_len: usize)
                                         -> c_int;

    pub fn secp256k1_ecdsa_signature_serialize_der(cx: *const Context, output: *mut c_uchar,
                                                   out_len: *mut usize, sig: *const Signature)
                                                   -> c_int;

    pub fn secp256k1_ecdsa_signature_serialize_compact(cx: *const Context, output64: *const c_uchar,
                                                       sig: *const Signature)
                                                       -> c_int;

    pub fn secp256k1_ecdsa_signature_normalize(cx: *const Context, out_sig: *mut Signature,
                                               in_sig: *const Signature)
                                               -> c_int;

    // ECDSA
    pub fn secp256k1_ecdsa_verify(cx: *const Context,
                                  sig: *const Signature,
                                  msg32: *const c_uchar,
                                  pk: *const PublicKey)
                                  -> c_int;

    pub fn secp256k1_ecdsa_sign(cx: *const Context,
                                sig: *mut Signature,
                                msg32: *const c_uchar,
                                sk: *const c_uchar,
                                noncefn: NonceFn,
                                noncedata: *const c_void)
                                -> c_int;

    // EC
    pub fn secp256k1_ec_seckey_verify(cx: *const Context,
                                      sk: *const c_uchar) -> c_int;

    pub fn secp256k1_ec_pubkey_create(cx: *const Context, pk: *mut PublicKey,
                                      sk: *const c_uchar) -> c_int;

//TODO secp256k1_ec_privkey_export
//TODO secp256k1_ec_privkey_import

    pub fn secp256k1_ec_privkey_tweak_add(cx: *const Context,
                                          sk: *mut c_uchar,
                                          tweak: *const c_uchar)
                                          -> c_int;

    pub fn secp256k1_ec_pubkey_tweak_add(cx: *const Context,
                                         pk: *mut PublicKey,
                                         tweak: *const c_uchar)
                                         -> c_int;

    pub fn secp256k1_ec_privkey_tweak_mul(cx: *const Context,
                                          sk: *mut c_uchar,
                                          tweak: *const c_uchar)
                                          -> c_int;

    pub fn secp256k1_ec_pubkey_tweak_mul(cx: *const Context,
                                         pk: *mut PublicKey,
                                         tweak: *const c_uchar)
                                         -> c_int;

    pub fn secp256k1_ec_pubkey_combine(cx: *const Context,
                                       out: *mut PublicKey,
                                       ins: *const *const PublicKey,
                                       n: c_int)
                                       -> c_int;

    pub fn secp256k1_ecdh(
        cx: *const Context,
        output: *mut SharedSecret,
        pubkey: *const PublicKey,
        privkey: *const c_uchar,
        hashfp: EcdhHashFn,
        data: *mut c_void,
    ) -> c_int;
}


#[cfg(all(feature = "std", not(feature = "dont_replace_c_symbols")))]
#[no_mangle]
/// A reimplementation of the C function `secp256k1_context_create` in rust.
///
/// This function allocates memory, the pointer should be deallocated using `secp256k1_context_destroy`
/// A failure to do so will result in a memory leak.
///
/// This will create a secp256k1 raw context.
// Returns: a newly created context object.
//  In:      flags: which parts of the context to initialize.
pub unsafe extern "C" fn secp256k1_context_create(flags: c_uint) -> *mut Context {
    assert!(mem::align_of::<usize>() >= mem::align_of::<u8>());
    assert_eq!(mem::size_of::<usize>(), mem::size_of::<&usize>());

    let word_size = mem::size_of::<usize>();
    let n_words = (secp256k1_context_preallocated_size(flags) + word_size - 1) / word_size;

    let buf = vec![0usize; n_words + 1].into_boxed_slice();
    let ptr = Box::into_raw(buf) as *mut usize;
    ::core::ptr::write(ptr, n_words);
    let ptr: *mut usize = ptr.offset(1);

    secp256k1_context_preallocated_create(ptr as *mut c_void, flags)
}

#[cfg(all(feature = "std", not(feature = "dont_replace_c_symbols")))]
#[no_mangle]
/// A reimplementation of the C function `secp256k1_context_destroy` in rust.
///
/// This function destroys and deallcates the context created by `secp256k1_context_create`.
///
/// The pointer shouldn't be used after passing to this function, consider it as passing it to `free()`.
///
pub unsafe extern "C" fn secp256k1_context_destroy(ctx: *mut Context) {
    secp256k1_context_preallocated_destroy(ctx);
    let ctx: *mut usize = ctx as *mut usize;

    let n_words_ptr: *mut usize = ctx.offset(-1);
    let n_words: usize = ::core::ptr::read(n_words_ptr);
    let slice: &mut [usize] = slice::from_raw_parts_mut(n_words_ptr , n_words+1);
    let _ = Box::from_raw(slice as *mut [usize]);
}


#[cfg(not(feature = "dont_replace_c_symbols"))]
#[no_mangle]
/// **This function is an override for the C function, this is the an edited version of the original description:**
///
/// A callback function to be called when an illegal argument is passed to
/// an API call. It will only trigger for violations that are mentioned
/// explicitly in the header. **This will cause a panic**.
///
/// The philosophy is that these shouldn't be dealt with through a
/// specific return value, as calling code should not have branches to deal with
/// the case that this code itself is broken.
///
/// On the other hand, during debug stage, one would want to be informed about
/// such mistakes, and the default (crashing) may be inadvisable.
/// When this callback is triggered, the API function called is guaranteed not
/// to cause a crash, though its return value and output arguments are
/// undefined.
///
/// See also secp256k1_default_error_callback_fn.
///
pub unsafe extern "C" fn secp256k1_default_illegal_callback_fn(message: *const c_char, _data: *mut c_void) {
    use core::str;
    let msg_slice = slice::from_raw_parts(message as *const u8, strlen(message));
    let msg = str::from_utf8_unchecked(msg_slice);
    panic!("[libsecp256k1] illegal argument. {}", msg);
}

#[cfg(not(feature = "dont_replace_c_symbols"))]
#[no_mangle]
/// **This function is an override for the C function, this is the an edited version of the original description:**
///
/// A callback function to be called when an internal consistency check
/// fails. **This will cause a panic**.
///
/// This can only trigger in case of a hardware failure, miscompilation,
/// memory corruption, serious bug in the library, or other error would can
/// otherwise result in undefined behaviour. It will not trigger due to mere
/// incorrect usage of the API (see secp256k1_default_illegal_callback_fn
/// for that). After this callback returns, anything may happen, including
/// crashing.
///
/// See also secp256k1_default_illegal_callback_fn.
///
pub unsafe extern "C" fn secp256k1_default_error_callback_fn(message: *const c_char, _data: *mut c_void) {
    use core::str;
    let msg_slice = slice::from_raw_parts(message as *const u8, strlen(message));
    let msg = str::from_utf8_unchecked(msg_slice);
    panic!("[libsecp256k1] internal consistency check failed {}", msg);
}


unsafe fn strlen(mut str_ptr: *const c_char) -> usize {
    let mut ctr = 0;
    while *str_ptr != '\0' as c_char {
        ctr += 1;
        str_ptr = str_ptr.offset(1);
    }
    ctr
}


/// A trait for producing pointers that will always be valid in C. (assuming NULL pointer is a valid no-op)
/// Rust doesn't promise what pointers does it give to ZST (https://doc.rust-lang.org/nomicon/exotic-sizes.html#zero-sized-types-zsts)
/// In case the type is empty this trait will give a NULL pointer, which should be handled in C.
/// 
pub(crate) trait CPtr {
    type Target;
    fn as_c_ptr(&self) -> *const Self::Target;
    fn as_mut_c_ptr(&mut self) -> *mut Self::Target;
}

impl<T> CPtr for [T] {
    type Target = T;
    fn as_c_ptr(&self) -> *const Self::Target {
        if self.is_empty() {
            ptr::null()
        } else {
            self.as_ptr()
        }
    }

    fn as_mut_c_ptr(&mut self) -> *mut Self::Target {
        if self.is_empty() {
            ptr::null::<Self::Target>() as *mut _
        } else {
            self.as_mut_ptr()
        }
    }
}




#[cfg(feature = "fuzztarget")]
mod fuzz_dummy {
    extern crate std;
    use types::*;
    use ffi::*;
    use self::std::{ptr, mem};
    use self::std::boxed::Box;

    extern "C" {
        pub static secp256k1_ecdh_hash_function_default: EcdhHashFn;
        pub static secp256k1_nonce_function_rfc6979: NonceFn;
        pub static secp256k1_context_no_precomp: *const Context;
    }

    // Contexts
    /// Creates a dummy context, tracking flags to ensure proper calling semantics
    pub unsafe fn secp256k1_context_preallocated_create(_ptr: *mut c_void, flags: c_uint) -> *mut Context {
        let b = Box::new(Context(flags as i32));
        Box::into_raw(b)
    }

    /// Return dummy size of context struct.
    pub unsafe fn secp256k1_context_preallocated_size(_flags: c_uint) -> usize {
        mem::size_of::<Context>()
    }

    /// Return dummy size of context struct.
    pub unsafe fn secp256k1_context_preallocated_clone_size(cx: *mut Context) -> usize {
        mem::size_of::<Context>()
    }

    /// Copies a dummy context
    pub unsafe fn secp256k1_context_preallocated_clone(cx: *const Context, prealloc: *mut c_void) -> *mut Context {
        let ret = prealloc as *mut Context;
        *ret = (*cx).clone();
        ret
    }

    /// "Destroys" a dummy context
    pub unsafe fn secp256k1_context_preallocated_destroy(cx: *mut Context) {
        (*cx).0 = 0;
    }

    /// Asserts that cx is properly initialized
    pub unsafe fn secp256k1_context_randomize(cx: *mut Context,
                                              _seed32: *const c_uchar)
                                              -> c_int {
        assert!(!cx.is_null() && (*cx).0 as u32 & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);
        1
    }

    // TODO secp256k1_context_set_illegal_callback
    // TODO secp256k1_context_set_error_callback
    // (Actually, I don't really want these exposed; if either of these
    // are ever triggered it indicates a bug in rust-secp256k1, since
    // one goal is to use Rust's type system to eliminate all possible
    // bad inputs.)

    // Pubkeys
    /// Parse 33/65 byte pubkey into PublicKey, losing compressed information
    pub unsafe fn secp256k1_ec_pubkey_parse(cx: *const Context, pk: *mut PublicKey,
                                            input: *const c_uchar, in_len: usize)
                                            -> c_int {
        assert!(!cx.is_null() && (*cx).0 as u32 & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);
        match in_len {
            33 => {
                if (*input.offset(1) > 0x7f && *input != 2) || (*input.offset(1) <= 0x7f && *input != 3) {
                    0
                } else {
                    ptr::copy(input.offset(1), (*pk).0[0..32].as_mut_ptr(), 32);
                    ptr::copy(input.offset(1), (*pk).0[32..64].as_mut_ptr(), 32);
                    test_pk_validate(cx, pk)
                }
            },
            65 => {
                if *input != 4 && *input != 6 && *input != 7 {
                    0
                } else {
                    ptr::copy(input.offset(1), (*pk).0.as_mut_ptr(), 64);
                    test_pk_validate(cx, pk)
                }
            },
            _ => 0
        }
    }

    /// Serialize PublicKey back to 33/65 byte pubkey
    pub unsafe fn secp256k1_ec_pubkey_serialize(cx: *const Context, output: *mut c_uchar,
                                                out_len: *mut usize, pk: *const PublicKey,
                                                compressed: c_uint)
                                                -> c_int {
        assert!(!cx.is_null() && (*cx).0 as u32 & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);
        if test_pk_validate(cx, pk) != 1 { return 0; }
        if compressed == SECP256K1_SER_COMPRESSED {
            assert_eq!(*out_len, 33);
            if (*pk).0[0] > 0x7f {
                *output = 2;
            } else {
                *output = 3;
            }
            ptr::copy((*pk).0.as_ptr(), output.offset(1), 32);
        } else if compressed == SECP256K1_SER_UNCOMPRESSED {
            assert_eq!(*out_len, 65);
            *output = 4;
            ptr::copy((*pk).0.as_ptr(), output.offset(1), 64);
        } else {
            panic!("Bad flags");
        }
        1
     }

    // Signatures
    pub unsafe fn secp256k1_ecdsa_signature_parse_der(_cx: *const Context, _sig: *mut Signature,
                                                      _input: *const c_uchar, _in_len: usize)
                                                      -> c_int {
        unimplemented!();
    }

    /// Copies input64 to sig, checking the pubkey part is valid
    pub unsafe fn secp256k1_ecdsa_signature_parse_compact(cx: *const Context, sig: *mut Signature,
                                                          input64: *const c_uchar)
                                                          -> c_int {
        assert!(!cx.is_null() && (*cx).0 as u32 & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);
        if secp256k1_ec_seckey_verify(cx, input64.offset(32)) != 1 { return 0; } // sig should be msg32||sk
        ptr::copy(input64, (*sig).0[..].as_mut_ptr(), 64);
        1
    }

    pub unsafe fn ecdsa_signature_parse_der_lax(_cx: *const Context, _sig: *mut Signature,
                                                _input: *const c_uchar, _in_len: usize)
                                                -> c_int {
        unimplemented!();
    }

    /// Copies up to 72 bytes into output from sig
    pub unsafe fn secp256k1_ecdsa_signature_serialize_der(cx: *const Context, output: *mut c_uchar,
                                                          out_len: *mut usize, sig: *const Signature)
                                                          -> c_int {
        assert!(!cx.is_null() && (*cx).0 as u32 & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);

        let mut len_r = 33;
        if *(*sig).0.as_ptr().offset(0) < 0x80 {
            len_r -= 1;
        }
        let mut len_s = 33;
        if *(*sig).0.as_ptr().offset(32) < 0x80 {
            len_s -= 1;
        }

        assert!(*out_len >= (6 + len_s + len_r) as usize);

        *output.offset(0) = 0x30;
        *output.offset(1) = 4 + len_r + len_s;
        *output.offset(2) = 0x02;
        *output.offset(3) = len_r;
        if len_r == 33 {
            *output.offset(4) = 0;
            ptr::copy((*sig).0[..].as_ptr(), output.offset(5), 32);
        } else {
            ptr::copy((*sig).0[..].as_ptr(), output.offset(4), 32);
        }
        *output.offset(4 + len_r as isize) = 0x02;
        *output.offset(5 + len_r as isize) = len_s;
        if len_s == 33 {
            *output.offset(6 + len_r as isize) = 0;
            ptr::copy((*sig).0[..].as_ptr().offset(32), output.offset(7 + len_r as isize), 32);
        } else {
            ptr::copy((*sig).0[..].as_ptr().offset(32), output.offset(6 + len_r as isize), 32);
        }
        1
    }

    /// Copies sig to output64
    pub unsafe fn secp256k1_ecdsa_signature_serialize_compact(cx: *const Context, output64: *mut c_uchar,
                                                              sig: *const Signature)
                                                              -> c_int {
        assert!(!cx.is_null() && (*cx).0 as u32 & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);
        ptr::copy((*sig).0[..].as_ptr(), output64, 64);
        1
    }

    pub unsafe fn secp256k1_ecdsa_signature_normalize(_cx: *const Context, _out_sig: *mut Signature,
                                                      _in_sig: *const Signature)
                                                      -> c_int {
        unimplemented!();
    }

    // ECDSA
    /// Verifies that sig is msg32||pk[0..32]
    pub unsafe fn secp256k1_ecdsa_verify(cx: *const Context,
                                         sig: *const Signature,
                                         msg32: *const c_uchar,
                                         pk: *const PublicKey)
                                         -> c_int {
        assert!(!cx.is_null() && (*cx).0 as u32 & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);
        assert!((*cx).0 as u32 & SECP256K1_START_VERIFY == SECP256K1_START_VERIFY);
        if test_pk_validate(cx, pk) != 1 { return 0; }
        for i in 0..32 {
            if (*sig).0[i] != *msg32.offset(i as isize) {
                return 0;
            }
        }
        if (*sig).0[32..64] != (*pk).0[0..32] {
            0
        } else {
            1
        }
    }

    /// Sets sig to msg32||sk
    pub unsafe fn secp256k1_ecdsa_sign(cx: *const Context,
                                       sig: *mut Signature,
                                       msg32: *const c_uchar,
                                       sk: *const c_uchar,
                                       _noncefn: NonceFn,
                                       _noncedata: *const c_void)
                                       -> c_int {
        assert!(!cx.is_null() && (*cx).0 as u32 & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);
        assert!((*cx).0 as u32 & SECP256K1_START_SIGN == SECP256K1_START_SIGN);
        if secp256k1_ec_seckey_verify(cx, sk) != 1 { return 0; }
        ptr::copy(msg32, (*sig).0[0..32].as_mut_ptr(), 32);
        ptr::copy(sk, (*sig).0[32..64].as_mut_ptr(), 32);
        1
    }

    // EC
    /// Checks that pk != 0xffff...ffff and pk[0..32] == pk[32..64]
    pub unsafe fn test_pk_validate(cx: *const Context,
                                   pk: *const PublicKey) -> c_int {
        assert!(!cx.is_null() && (*cx).0 as u32 & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);
        if (*pk).0[0..32] != (*pk).0[32..64] || secp256k1_ec_seckey_verify(cx, (*pk).0[0..32].as_ptr()) == 0 {
            0
        } else {
            1
        }
    }

    /// Checks that sk != 0xffff...ffff
    pub unsafe fn secp256k1_ec_seckey_verify(cx: *const Context,
                                             sk: *const c_uchar) -> c_int {
        assert!(!cx.is_null() && (*cx).0 as u32 & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);
        let mut res = 0;
        for i in 0..32 {
            if *sk.offset(i as isize) != 0xff { res = 1 };
        }
        res
    }

    /// Sets pk to sk||sk
    pub unsafe fn secp256k1_ec_pubkey_create(cx: *const Context, pk: *mut PublicKey,
                                             sk: *const c_uchar) -> c_int {
        assert!(!cx.is_null() && (*cx).0 as u32 & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);
        if secp256k1_ec_seckey_verify(cx, sk) != 1 { return 0; }
        ptr::copy(sk, (*pk).0[0..32].as_mut_ptr(), 32);
        ptr::copy(sk, (*pk).0[32..64].as_mut_ptr(), 32);
        1
    }

//TODO secp256k1_ec_privkey_export
//TODO secp256k1_ec_privkey_import

    /// Copies the first 16 bytes of tweak into the last 16 bytes of sk
    pub unsafe fn secp256k1_ec_privkey_tweak_add(cx: *const Context,
                                                 sk: *mut c_uchar,
                                                 tweak: *const c_uchar)
                                                 -> c_int {
        assert!(!cx.is_null() && (*cx).0 as u32 & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);
        if secp256k1_ec_seckey_verify(cx, sk) != 1 { return 0; }
        ptr::copy(tweak.offset(16), sk.offset(16), 16);
        *sk.offset(24) = 0x7f; // Ensure sk remains valid no matter what tweak was
        1
    }

    /// The PublicKey equivalent of secp256k1_ec_privkey_tweak_add
    pub unsafe fn secp256k1_ec_pubkey_tweak_add(cx: *const Context,
                                                pk: *mut PublicKey,
                                                tweak: *const c_uchar)
                                                -> c_int {
        assert!(!cx.is_null() && (*cx).0 as u32 & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);
        if test_pk_validate(cx, pk) != 1 { return 0; }
        ptr::copy(tweak.offset(16), (*pk).0[16..32].as_mut_ptr(), 16);
        ptr::copy(tweak.offset(16), (*pk).0[16+32..64].as_mut_ptr(), 16);
        (*pk).0[24] = 0x7f; // Ensure pk remains valid no matter what tweak was
        (*pk).0[24+32] = 0x7f; // Ensure pk remains valid no matter what tweak was
        1
    }

    /// Copies the last 16 bytes of tweak into the last 16 bytes of sk
    pub unsafe fn secp256k1_ec_privkey_tweak_mul(cx: *const Context,
                                                 sk: *mut c_uchar,
                                                 tweak: *const c_uchar)
                                                 -> c_int {
        assert!(!cx.is_null() && (*cx).0 as u32 & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);
        if secp256k1_ec_seckey_verify(cx, sk) != 1 { return 0; }
        ptr::copy(tweak.offset(16), sk.offset(16), 16);
        *sk.offset(24) = 0x00; // Ensure sk remains valid no matter what tweak was
        1
    }

    /// The PublicKey equivalent of secp256k1_ec_privkey_tweak_mul
    pub unsafe fn secp256k1_ec_pubkey_tweak_mul(cx: *const Context,
                                                pk: *mut PublicKey,
                                                tweak: *const c_uchar)
                                                -> c_int {
        assert!(!cx.is_null() && (*cx).0 as u32 & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);
        if test_pk_validate(cx, pk) != 1 { return 0; }
        ptr::copy(tweak.offset(16), (*pk).0[16..32].as_mut_ptr(), 16);
        ptr::copy(tweak.offset(16), (*pk).0[16+32..64].as_mut_ptr(), 16);
        (*pk).0[24] = 0x00; // Ensure pk remains valid no matter what tweak was
        (*pk).0[24+32] = 0x00; // Ensure pk remains valid no matter what tweak was
        1
    }

    pub unsafe fn secp256k1_ec_pubkey_combine(cx: *const Context,
                                              out: *mut PublicKey,
                                              ins: *const *const PublicKey,
                                              n: c_int)
                                              -> c_int {
        assert!(!cx.is_null() && (*cx).0 as u32 & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);
        assert!(n <= 32 && n >= 0); //TODO: Remove this restriction?
        for i in 0..n {
            if test_pk_validate(cx, *ins.offset(i as isize)) != 1 { return 0; }
            (*out).0[(i*32/n) as usize..((i+1)*32/n) as usize].copy_from_slice(&(**ins.offset(i as isize)).0[(i*32/n) as usize..((i+1)*32/n) as usize]);
        }
        ptr::copy((*out).0[0..32].as_ptr(), (*out).0[32..64].as_mut_ptr(), 32);
        (*out).0[24] = 0x7f; // pk should always be valid
        (*out).0[24+32] = 0x7f; // pk should always be valid
        test_pk_validate(cx, out)
    }

    /// Sets out to point[0..16]||scalar[0..16]
    pub unsafe fn secp256k1_ecdh(
        cx: *const Context,
        out: *mut SharedSecret,
        point: *const PublicKey,
        scalar: *const c_uchar,
        _hashfp: EcdhHashFn,
        _data: *mut c_void,
    ) -> c_int {
        assert!(!cx.is_null() && (*cx).0 as u32 & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);
        if secp256k1_ec_seckey_verify(cx, scalar) != 1 { return 0; }

        let mut scalar_prefix = [0; 16];
        ptr::copy(scalar, scalar_prefix[..].as_mut_ptr(), 16);

        if (*point).0[0..16] > scalar_prefix[0..16] {
            (*out).0[0..16].copy_from_slice(&(*point).0[0..16]);
            ptr::copy(scalar, (*out).0[16..32].as_mut_ptr(), 16);
        } else {
            ptr::copy(scalar, (*out).0[0..16].as_mut_ptr(), 16);
            (*out).0[16..32].copy_from_slice(&(*point).0[0..16]);
        }
        (*out).0[16] = 0x00; // result should always be a valid secret key
        1
    }
}
#[cfg(feature = "fuzztarget")]
pub use self::fuzz_dummy::*;


#[cfg(test)]
mod tests {
    use std::ffi::CString;
    use super::strlen;

    #[test]
    fn test_strlen() {
        let orig = "test strlen \t \n";
        let test = CString::new(orig).unwrap();

        assert_eq!(orig.len(), unsafe {strlen(test.as_ptr())});
    }
}
