use core::marker::PhantomData;
use ffi::{self, CPtr};
use types::{c_uint, c_void};
use Error;
use Secp256k1;

#[cfg(feature = "std")]
pub use self::std_only::*;

/// A trait for all kinds of Context's that Lets you define the exact flags and a function to deallocate memory.
/// * DO NOT * implement it for your own types.
pub unsafe trait Context {
    /// Flags for the ffi.
    const FLAGS: c_uint;
    /// A constant description of the context.
    const DESCRIPTION: &'static str;
    /// A function to deallocate the memory when the context is dropped.
    fn deallocate(ptr: *mut [u8]);
}

/// Marker trait for indicating that an instance of `Secp256k1` can be used for signing.
pub trait Signing: Context {}

/// Marker trait for indicating that an instance of `Secp256k1` can be used for verification.
pub trait Verification: Context {}

/// Represents the set of capabilities needed for signing with a user preallocated memory.
pub struct SignOnlyPreallocated<'buf> {
    phantom: PhantomData<&'buf ()>,
}

/// Represents the set of capabilities needed for verification with a user preallocated memory.
pub struct VerifyOnlyPreallocated<'buf> {
    phantom: PhantomData<&'buf ()>,
}

/// Represents the set of all capabilities with a user preallocated memory.
pub struct AllPreallocated<'buf> {
    phantom: PhantomData<&'buf ()>,
}

#[cfg(feature = "std")]
mod std_only {
    use super::*;

    /// Represents the set of capabilities needed for signing.
    pub enum SignOnly {}

    /// Represents the set of capabilities needed for verification.
    pub enum VerifyOnly {}

    /// Represents the set of all capabilities.
    pub enum All {}

    impl Signing for SignOnly {}
    impl Signing for All {}

    impl Verification for VerifyOnly {}
    impl Verification for All {}

    unsafe impl Context for SignOnly {
        const FLAGS: c_uint = ffi::SECP256K1_START_SIGN;
        const DESCRIPTION: &'static str = "signing only";

        fn deallocate(ptr: *mut [u8]) {
            let _ = unsafe { Box::from_raw(ptr) };
        }
    }

    unsafe impl Context for VerifyOnly {
        const FLAGS: c_uint = ffi::SECP256K1_START_VERIFY;
        const DESCRIPTION: &'static str = "verification only";

        fn deallocate(ptr: *mut [u8]) {
            let _ = unsafe { Box::from_raw(ptr) };
        }
    }

    unsafe impl Context for All {
        const FLAGS: c_uint = VerifyOnly::FLAGS | SignOnly::FLAGS;
        const DESCRIPTION: &'static str = "all capabilities";

        fn deallocate(ptr: *mut [u8]) {
            let _ = unsafe { Box::from_raw(ptr) };
        }
    }

    impl<C: Context> Secp256k1<C> {
        /// Lets you create a context in a generic manner(sign/verify/all)
        pub fn gen_new() -> Secp256k1<C> {
            let buf = vec![0u8; Self::preallocate_size_gen()].into_boxed_slice();
            let ptr = Box::into_raw(buf);
            Secp256k1 {
                ctx: unsafe { ffi::secp256k1_context_preallocated_create(ptr as *mut c_void, C::FLAGS) },
                phantom: PhantomData,
                buf: ptr,
            }
        }
    }

    impl Secp256k1<All> {
        /// Creates a new Secp256k1 context with all capabilities
        pub fn new() -> Secp256k1<All> {
            Secp256k1::gen_new()
        }
    }

    impl Secp256k1<SignOnly> {
        /// Creates a new Secp256k1 context that can only be used for signing
        pub fn signing_only() -> Secp256k1<SignOnly> {
            Secp256k1::gen_new()
        }
    }

    impl Secp256k1<VerifyOnly> {
        /// Creates a new Secp256k1 context that can only be used for verification
        pub fn verification_only() -> Secp256k1<VerifyOnly> {
            Secp256k1::gen_new()
        }
    }

    impl Default for Secp256k1<All> {
        fn default() -> Self {
            Self::new()
        }
    }

    impl<C: Context> Clone for Secp256k1<C> {
        fn clone(&self) -> Secp256k1<C> {
            let clone_size = unsafe {ffi::secp256k1_context_preallocated_clone_size(self.ctx)};
            let ptr_buf = Box::into_raw(vec![0u8; clone_size].into_boxed_slice());
            Secp256k1 {
                ctx: unsafe { ffi::secp256k1_context_preallocated_clone(self.ctx, ptr_buf as *mut c_void) },
                phantom: PhantomData,
                buf: ptr_buf,
            }
        }
    }

}

impl<'buf> Signing for SignOnlyPreallocated<'buf> {}
impl<'buf> Signing for AllPreallocated<'buf> {}

impl<'buf> Verification for VerifyOnlyPreallocated<'buf> {}
impl<'buf> Verification for AllPreallocated<'buf> {}

unsafe impl<'buf> Context for SignOnlyPreallocated<'buf> {
    const FLAGS: c_uint = ffi::SECP256K1_START_SIGN;
    const DESCRIPTION: &'static str = "signing only";

    fn deallocate(_ptr: *mut [u8]) {
        // Allocated by the user
    }
}

unsafe impl<'buf> Context for VerifyOnlyPreallocated<'buf> {
    const FLAGS: c_uint = ffi::SECP256K1_START_VERIFY;
    const DESCRIPTION: &'static str = "verification only";

    fn deallocate(_ptr: *mut [u8]) {
        // Allocated by the user
    }
}

unsafe impl<'buf> Context for AllPreallocated<'buf> {
    const FLAGS: c_uint = SignOnlyPreallocated::FLAGS | VerifyOnlyPreallocated::FLAGS;
    const DESCRIPTION: &'static str = "all capabilities";

    fn deallocate(_ptr: *mut [u8]) {
        // Allocated by the user
    }
}

impl<'buf, C: Context + 'buf> Secp256k1<C> {
    /// Lets you create a context with preallocated buffer in a generic manner(sign/verify/all)
    pub fn preallocated_gen_new(buf: &'buf mut [u8]) -> Result<Secp256k1<C>, Error> {
        if buf.len() < Self::preallocate_size_gen() {
            return Err(Error::NotEnoughMemory);
        }
        Ok(Secp256k1 {
            ctx: unsafe {
                ffi::secp256k1_context_preallocated_create(
                    buf.as_mut_c_ptr() as *mut c_void,
                    C::FLAGS)
            },
            phantom: PhantomData,
            buf: buf as *mut [u8],
        })
    }
}

impl<'buf> Secp256k1<AllPreallocated<'buf>> {
    /// Creates a new Secp256k1 context with all capabilities
    pub fn preallocated_new(buf: &'buf mut [u8]) -> Result<Secp256k1<AllPreallocated<'buf>>, Error> {
        Secp256k1::preallocated_gen_new(buf)
    }
    /// Uses the ffi `secp256k1_context_preallocated_size` to check the memory size needed for a context
    pub fn preallocate_size() -> usize {
        Self::preallocate_size_gen()
    }
}

impl<'buf> Secp256k1<SignOnlyPreallocated<'buf>> {
    /// Creates a new Secp256k1 context that can only be used for signing
    pub fn preallocated_signing_only(buf: &'buf mut [u8]) -> Result<Secp256k1<SignOnlyPreallocated<'buf>>, Error> {
        Secp256k1::preallocated_gen_new(buf)
    }

    /// Uses the ffi `secp256k1_context_preallocated_size` to check the memory size needed for the context
    #[inline]
    pub fn preallocate_signing_size() -> usize {
        Self::preallocate_size_gen()
    }
}

impl<'buf> Secp256k1<VerifyOnlyPreallocated<'buf>> {
    /// Creates a new Secp256k1 context that can only be used for verification
    pub fn preallocated_verification_only(buf: &'buf mut [u8]) -> Result<Secp256k1<VerifyOnlyPreallocated<'buf>>, Error> {
        Secp256k1::preallocated_gen_new(buf)
    }

    /// Uses the ffi `secp256k1_context_preallocated_size` to check the memory size needed for the context
    #[inline]
    pub fn preallocate_verification_size() -> usize {
        Self::preallocate_size_gen()
    }
}
