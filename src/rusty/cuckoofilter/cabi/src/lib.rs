//! A C interface for the [cuckoofilter crate].
//!
//! You can use this crate to use the `cuckoofilter` crate from C/C++, or almost any other
//! language. There only needs to be a way to call foreign C functions, like in python or ruby.
//!
//! **Note**: For other languages, you'd probably want to add a language-specific layer on top of
//! this, e.g. using [milksnake] for python. Contributions welcome!
//!
//! # Build Setup
//!
//! You need to integrate this crate in your build system somehow, how you do this depends on your
//! specific build system. You can e.g. use a local checkout of the [cuckoofilter crate]:
//!
//! ```bash
//! git clone https://github.com/seiflotfy/rust-cuckoofilter
//! cd rust-cuckoofilter/cabi
//! cargo build --release
//! ```
//!
//! Then, put the generated file `cabi/target/include/rcf_cuckoofilter.h` on your compiler's
//! include path and link against either `cabi/target/release/libcuckoofilter_cabi.a` or
//! `cabi/target/release/libcuckoofilter_cabi.so`, depending on whether you want static or dynamic
//! linking. Alternatively, use the provided Makefile via `sudo make install` to install the header
//! and libraries system-wide. You can see the `tests` directory for basic examples using a
//! Makefile for C and C++, including static and dynamic linking in each case.
//!
//! If you found a nice way to integrate this crate in a build system,
//! please consider contributing the necessary build files!
//!
//! # Usage
//!
//! You can then use the interface like this:
//!
//! ```C
//! #include "rcf_cuckoofilter.h"
//!
//! rcf_cuckoofilter *filter = rcf_cuckoofilter_with_capacity(1000);
//! rcf_cuckoofilter_status result;
//! result = rcf_cuckoofilter_add(filter, 42);
//! assert(result == RCF_OK);
//! result = rcf_cuckoofilter_contains(filter, 42);
//! assert(result == RCF_OK);
//! result = rcf_cuckoofilter_delete(filter, 42);
//! assert(result == RCF_OK);
//! result = rcf_cuckoofilter_contains(filter, 42);
//! assert(result == RCF_NOT_FOUND);
//! ```
//!
//! # Hashing arbitrary data
//! The interface only takes unsigned 64bit integers as `data`.
//! If you want to insert structs or other types, search for something that hashes those to
//! integers. Those hashes don't need to be well-distributed as they're
//! hashed on the rust side again, so a very simple hash function is sufficient,
//! like `std::hash` for C++ or the `__hash__` method in python.
//! There's a good chance something like this is present in the
//! respective standard library, for implementing hash tables and the like.
//!
//! In the future, the interface could accept a pointer to arbitrary memory and a size parameter,
//! and hash that as byte array on the Rust side. But this approach is problematic if not all given
//! bytes are the same for two equal objects.
//!
//! # Naming
//! The prefix `rcf` is short for *rust cuckoo filter*.
//! It's used for C-style namespacing to avoid name conflicts with other libraries.
//!
//! [cuckoofilter crate]: https://crates.io/crates/cuckoofilter
//! [milksnake]: https://github.com/getsentry/milksnake

extern crate cuckoofilter;

use cuckoofilter::CuckooError;
use std::collections::hash_map::DefaultHasher;

/// Opaque type for a cuckoo filter using Rust's `std::collections::hash_map::DefaultHasher` as
/// Hasher. The C ABI only supports that specific Hasher, currently.
#[allow(non_camel_case_types)]
pub type rcf_cuckoofilter = cuckoofilter::CuckooFilter<DefaultHasher>;

#[allow(non_camel_case_types)]
#[repr(C)]
pub enum rcf_cuckoofilter_status {
    RCF_OK,
    RCF_NOT_FOUND,
    RCF_NOT_ENOUGH_SPACE,
}

/// Constructs a cuckoo filter with a given max capacity.
/// The various wrapper methods of this crate operate on the returned reference.
/// At the end of its life, use [`rcf_cuckoofilter_free`] to free the allocated memory.
///
/// [`rcf_cuckoofilter_free`]: fn.rcf_cuckoofilter_free.html
#[no_mangle]
pub extern "C" fn rcf_cuckoofilter_with_capacity(capacity: usize) -> *mut rcf_cuckoofilter {
    let filter = cuckoofilter::CuckooFilter::with_capacity(capacity);
    let filter = Box::new(filter);
    Box::into_raw(filter)
}

/// Free the given `filter`, releasing its allocated memory.
#[no_mangle]
pub extern "C" fn rcf_cuckoofilter_free(filter: *mut rcf_cuckoofilter) {
    let filter = unsafe { Box::from_raw(filter) };
    drop(filter);
}

/// Checks if the given `data` is in the `filter`.
///
/// Returns `rcf_cuckoofilter_status::RCF_OK` if the given `data` is in the `filter`,
/// `rcf_cuckoofilter_status::RCF_NOT_FOUND` otherwise.
/// Aborts if the given `filter` is a null pointer.
#[no_mangle]
pub extern "C" fn rcf_cuckoofilter_contains(
    filter: *const rcf_cuckoofilter,
    data: u64,
) -> rcf_cuckoofilter_status {
    let filter = unsafe { filter.as_ref() };
    let found = filter
        .expect("Given rcf_cuckoofilter* is a null pointer")
        .contains(&data);
    if found {
        rcf_cuckoofilter_status::RCF_OK
    } else {
        rcf_cuckoofilter_status::RCF_NOT_FOUND
    }
}

/// Adds `data` to the `filter`.
///
/// Returns `rcf_cuckoofilter_status::RCF_OK` if the given `data` was successfully added to the
/// `filter`, `rcf_cuckoofilter_status::RCF_NOT_ENOUGH_SPACE` if the filter could not find a free
/// space for it.
/// Aborts if the given `filter` is a null pointer.
#[no_mangle]
pub extern "C" fn rcf_cuckoofilter_add(
    filter: *mut rcf_cuckoofilter,
    data: u64,
) -> rcf_cuckoofilter_status {
    let filter = unsafe { filter.as_mut() };
    match filter
        .expect("Given rcf_cuckoofilter* is a null pointer")
        .add(&data)
    {
        Ok(_) => rcf_cuckoofilter_status::RCF_OK,
        Err(CuckooError::NotEnoughSpace) => rcf_cuckoofilter_status::RCF_NOT_ENOUGH_SPACE,
    }
}

/// Returns the number of items in the `filter`.
/// Aborts if the given `filter` is a null pointer.
#[no_mangle]
pub extern "C" fn rcf_cuckoofilter_len(filter: *const rcf_cuckoofilter) -> usize {
    let filter = unsafe { filter.as_ref() };
    filter
        .expect("Given rcf_cuckoofilter* is a null pointer")
        .len()
}

/// Checks if `filter` is empty.
/// This is equivalent to `rcf_cuckoofilter_len(filter) == 0`
/// Aborts if the given `filter` is a null pointer.
#[no_mangle]
pub extern "C" fn rcf_cuckoofilter_is_empty(filter: *const rcf_cuckoofilter) -> bool {
    let filter = unsafe { filter.as_ref() };
    filter
        .expect("Given rcf_cuckoofilter* is a null pointer")
        .is_empty()
}

/// Returns the number of bytes the `filter` occupies in memory.
/// Aborts if the given `filter` is a null pointer.
#[no_mangle]
pub extern "C" fn rcf_cuckoofilter_memory_usage(filter: *const rcf_cuckoofilter) -> usize {
    let filter = unsafe { filter.as_ref() };
    filter
        .expect("Given rcf_cuckoofilter* is a null pointer")
        .memory_usage()
}

/// Deletes `data` from the `filter`.
/// Returns `rcf_cuckoofilter_status::RCF_OK` if `data` existed in the filter before,
/// `rcf_cuckoofilter_status::RCF_NOT_FOUND` if `data` did not exist.
/// Aborts if the given `filter` is a null pointer.
#[no_mangle]
pub extern "C" fn rcf_cuckoofilter_delete(
    filter: *mut rcf_cuckoofilter,
    data: u64,
) -> rcf_cuckoofilter_status {
    let filter = unsafe { filter.as_mut() };
    let found = filter
        .expect("Given rcf_cuckoofilter* is a null pointer")
        .delete(&data);
    if found {
        rcf_cuckoofilter_status::RCF_OK
    } else {
        rcf_cuckoofilter_status::RCF_NOT_FOUND
    }
}
