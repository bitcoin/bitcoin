use std::ffi::c_void;

use bls_dash_sys::{BIP32ChainCodeFree, BIP32ChainCodeIsEqual, BIP32ChainCodeSerialize};

pub const BIP32_CHAIN_CODE_SIZE: usize = 32;

#[derive(Debug)]
pub struct ChainCode {
    pub(crate) c_chain_code: *mut c_void,
}

impl ChainCode {
    pub fn serialize(&self) -> Box<[u8; BIP32_CHAIN_CODE_SIZE]> {
        unsafe {
            let malloc_ptr = BIP32ChainCodeSerialize(self.c_chain_code);
            Box::from_raw(malloc_ptr as *mut _)
        }
    }
}

impl PartialEq for ChainCode {
    fn eq(&self, other: &Self) -> bool {
        unsafe { BIP32ChainCodeIsEqual(self.c_chain_code, other.c_chain_code) }
    }
}

impl Eq for ChainCode {}

impl Drop for ChainCode {
    fn drop(&mut self) {
        unsafe { BIP32ChainCodeFree(self.c_chain_code) }
    }
}
