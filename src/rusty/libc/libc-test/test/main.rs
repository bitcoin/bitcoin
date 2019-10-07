#![allow(bad_style, improper_ctypes, deprecated)]
extern crate libc;

use libc::*;

include!(concat!(env!("OUT_DIR"), "/main.rs"));
