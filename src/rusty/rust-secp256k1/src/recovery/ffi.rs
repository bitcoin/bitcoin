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

//! # FFI of the recovery module

use core::mem;
use types::*;
use ffi::{Context, NonceFn, PublicKey, Signature, CPtr};

/// Library-internal representation of a Secp256k1 signature + recovery ID
#[repr(C)]
pub struct RecoverableSignature([c_uchar; 65]);
impl_array_newtype!(RecoverableSignature, c_uchar, 65);
impl_raw_debug!(RecoverableSignature);

impl RecoverableSignature {
    /// Create a new (zeroed) signature usable for the FFI interface
    pub fn new() -> RecoverableSignature { RecoverableSignature([0; 65]) }
    /// Create a new (uninitialized) signature usable for the FFI interface
    #[deprecated(since = "0.15.3", note = "Please use the new function instead")]
    pub unsafe fn blank() -> RecoverableSignature { RecoverableSignature::new() }
}

impl Default for RecoverableSignature {
    fn default() -> Self {
        RecoverableSignature::new()
    }
}

#[cfg(not(feature = "fuzztarget"))]
extern "C" {
    pub fn secp256k1_ecdsa_recoverable_signature_parse_compact(cx: *const Context, sig: *mut RecoverableSignature,
                                                               input64: *const c_uchar, recid: c_int)
                                                               -> c_int;

    pub fn secp256k1_ecdsa_recoverable_signature_serialize_compact(cx: *const Context, output64: *const c_uchar,
                                                                   recid: *mut c_int, sig: *const RecoverableSignature)
                                                                   -> c_int;

    pub fn secp256k1_ecdsa_recoverable_signature_convert(cx: *const Context, sig: *mut Signature,
                                                         input: *const RecoverableSignature)
                                                         -> c_int;
    pub fn secp256k1_ecdsa_sign_recoverable(cx: *const Context,
                                            sig: *mut RecoverableSignature,
                                            msg32: *const c_uchar,
                                            sk: *const c_uchar,
                                            noncefn: NonceFn,
                                            noncedata: *const c_void)
                                            -> c_int;

    pub fn secp256k1_ecdsa_recover(cx: *const Context,
                                   pk: *mut PublicKey,
                                   sig: *const RecoverableSignature,
                                   msg32: *const c_uchar)
                                   -> c_int;
}


#[cfg(feature = "fuzztarget")]
mod fuzz_dummy {
    extern crate std;
    use types::*;
    use ffi::*;
    use self::std::ptr;
    use super::RecoverableSignature;

    pub unsafe fn secp256k1_ecdsa_recoverable_signature_parse_compact(_cx: *const Context, _sig: *mut RecoverableSignature,
                                                                      _input64: *const c_uchar, _recid: c_int)
                                                                      -> c_int {
        unimplemented!();
    }

    pub unsafe fn secp256k1_ecdsa_recoverable_signature_serialize_compact(_cx: *const Context, _output64: *const c_uchar,
                                                                          _recid: *mut c_int, _sig: *const RecoverableSignature)
                                                                          -> c_int {
        unimplemented!();
    }

    pub unsafe fn secp256k1_ecdsa_recoverable_signature_convert(_cx: *const Context, _sig: *mut Signature,
                                                                _input: *const RecoverableSignature)
                                                                -> c_int {
        unimplemented!();
    }

    /// Sets sig to (2|3)||msg32||sk
    pub unsafe fn secp256k1_ecdsa_sign_recoverable(cx: *const Context,
                                                   sig: *mut RecoverableSignature,
                                                   msg32: *const c_uchar,
                                                   sk: *const c_uchar,
                                                   _noncefn: NonceFn,
                                                   _noncedata: *const c_void)
                                                   -> c_int {
        assert!(!cx.is_null() && (*cx).flags() & !(SECP256K1_START_NONE | SECP256K1_START_VERIFY | SECP256K1_START_SIGN) == 0);
        assert!((*cx).flags() & SECP256K1_START_SIGN == SECP256K1_START_SIGN);
        if secp256k1_ec_seckey_verify(cx, sk) != 1 { return 0; }
        if *sk.offset(0) > 0x7f {
            (*sig).0[0] = 2;
        } else {
            (*sig).0[0] = 3;
        }
        ptr::copy(msg32, (*sig).0[1..33].as_mut_ptr(), 32);
        ptr::copy(sk, (*sig).0[33..65].as_mut_ptr(), 32);
        1
    }

    pub unsafe fn secp256k1_ecdsa_recover(_cx: *const Context,
                                          _pk: *mut PublicKey,
                                          _sig: *const RecoverableSignature,
                                          _msg32: *const c_uchar)
                                          -> c_int {
        unimplemented!();
    }
}
#[cfg(feature = "fuzztarget")]
pub use self::fuzz_dummy::*;
