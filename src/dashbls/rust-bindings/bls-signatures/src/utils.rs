use core::slice;
use std::{
    ffi::{c_void, CStr},
    ops::Deref,
};

use bls_dash_sys::{GetLastErrorMsg, SecAllocBytes, SecFree};

use crate::BlsError;

pub(crate) fn c_err_to_result<T, F>(f: F) -> Result<T, BlsError>
where
    F: FnOnce(&mut bool) -> T,
{
    let mut did_error = false;
    let result = f(&mut did_error);

    if did_error {
        let error_message = unsafe { CStr::from_ptr(GetLastErrorMsg()) };
        Err(BlsError {
            msg: String::from_utf8_lossy(error_message.to_bytes()).into_owned(),
        })
    } else {
        Ok(result)
    }
}

pub struct SecureBox {
    c_sec_alloc: *mut u8,
    len: usize,
}

impl SecureBox {
    #[allow(dead_code)]
    pub(crate) fn new(len: usize) -> Self {
        SecureBox {
            c_sec_alloc: unsafe { SecAllocBytes(len) },
            len,
        }
    }

    pub(crate) unsafe fn from_ptr(ptr: *mut u8, len: usize) -> Self {
        SecureBox {
            c_sec_alloc: ptr,
            len,
        }
    }

    // Somewhere it returns *mut c_void
    pub(crate) fn as_mut_ptr(&mut self) -> *mut c_void {
        self.c_sec_alloc as *mut c_void
    }

    pub fn as_slice(&self) -> &[u8] {
        unsafe { slice::from_raw_parts(self.c_sec_alloc, self.len) }
    }
}

impl Deref for SecureBox {
    type Target = [u8];

    fn deref(&self) -> &Self::Target {
        self.as_slice()
    }
}

impl Drop for SecureBox {
    fn drop(&mut self) {
        unsafe { SecFree(self.as_mut_ptr()) }
    }
}
