//! Cuckoo filter probabilistic data structure for membership testing and cardinality counting.
//!
//! # Usage
//!
//! This crate is [on crates.io](https://crates.io/crates/cuckoofilter) and can be
//! used by adding `cuckoofilter` to the dependencies in your project's `Cargo.toml`.
//!
//! ```toml
//! [dependencies]
//! cuckoofilter = "0.3"
//! ```
//!
//! And this in your crate root:
//!
//! ```rust
//! extern crate cuckoofilter;
//! ```

mod bucket;
mod util;

use crate::bucket::{Bucket, Fingerprint, BUCKET_SIZE, FINGERPRINT_SIZE};
use crate::util::{get_alt_index, get_fai, FaI};

use std::cmp;
use std::collections::hash_map::DefaultHasher;
use std::error::Error as StdError;
use std::fmt;
use std::hash::{Hash, Hasher};
use std::iter::repeat;
use std::marker::PhantomData;
use std::mem;

use rand::Rng;
#[cfg(feature = "serde_support")]
use serde_derive::{Deserialize, Serialize};

/// If insertion fails, we will retry this many times.
pub const MAX_REBUCKET: u32 = 500;

/// The default number of buckets.
pub const DEFAULT_CAPACITY: usize = (1 << 20) - 1;

#[derive(Debug)]
pub enum CuckooError {
    NotEnoughSpace,
}

impl fmt::Display for CuckooError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.write_str("NotEnoughSpace")
    }
}

impl StdError for CuckooError {
    fn description(&self) -> &str {
        "Not enough space to store this item, rebucketing failed."
    }
}

/// A cuckoo filter class exposes a Bloomier filter interface,
/// providing methods of add, delete, contains.
///
/// # Examples
///
/// ```
/// extern crate cuckoofilter;
///
/// let words = vec!["foo", "bar", "xylophone", "milagro"];
/// let mut cf = cuckoofilter::CuckooFilter::new();
///
/// let mut insertions = 0;
/// for s in &words {
///     if cf.test_and_add(s).unwrap() {
///         insertions += 1;
///     }
/// }
///
/// assert_eq!(insertions, words.len());
/// assert_eq!(cf.len(), words.len());
///
/// // Re-add the first element.
/// cf.add(words[0]);
///
/// assert_eq!(cf.len(), words.len() + 1);
///
/// for s in &words {
///     cf.delete(s);
/// }
///
/// assert_eq!(cf.len(), 1);
/// assert!(!cf.is_empty());
///
/// cf.delete(words[0]);
///
/// assert_eq!(cf.len(), 0);
/// assert!(cf.is_empty());
///
/// for s in &words {
///     if cf.test_and_add(s).unwrap() {
///         insertions += 1;
///     }
/// }
///
/// cf.clear();
///
/// assert!(cf.is_empty());
///
/// ```
pub struct CuckooFilter<H> {
    buckets: Box<[Bucket]>,
    len: usize,
    _hasher: std::marker::PhantomData<H>,
}

impl Default for CuckooFilter<DefaultHasher> {
    fn default() -> Self {
        Self::new()
    }
}

impl CuckooFilter<DefaultHasher> {
    /// Construct a CuckooFilter with default capacity and hasher.
    pub fn new() -> Self {
        Self::with_capacity(DEFAULT_CAPACITY)
    }
}

impl<H> CuckooFilter<H>
where
    H: Hasher + Default,
{
    /// Constructs a Cuckoo Filter with a given max capacity
    pub fn with_capacity(cap: usize) -> Self {
        let capacity = cmp::max(1, cap.next_power_of_two() / BUCKET_SIZE);

        Self {
            buckets: repeat(Bucket::new())
                .take(capacity)
                .collect::<Vec<_>>()
                .into_boxed_slice(),
            len: 0,
            _hasher: PhantomData,
        }
    }

    /// Checks if `data` is in the filter.
    pub fn contains<T: ?Sized + Hash>(&self, data: &T) -> bool {
        let FaI { fp, i1, i2 } = get_fai::<T, H>(data);
        let len = self.buckets.len();
        self.buckets[i1 % len]
            .get_fingerprint_index(fp)
            .or_else(|| self.buckets[i2 % len].get_fingerprint_index(fp))
            .is_some()
    }

    /// Adds `data` to the filter. Returns `Ok` if the insertion was successful,
    /// but could fail with a `NotEnoughSpace` error, especially when the filter
    /// is nearing its capacity.
    /// Note that while you can put any hashable type in the same filter, beware
    /// for side effects like that the same number can have diferent hashes
    /// depending on the type.
    /// So for the filter, 4711i64 isn't the same as 4711u64.
    ///
    /// **Note:** When this returns `NotEnoughSpace`, the element given was
    /// actually added to the filter, but some random *other* element was
    /// removed. This might improve in the future.
    pub fn add<T: ?Sized + Hash>(&mut self, data: &T) -> Result<(), CuckooError> {
        let fai = get_fai::<T, H>(data);
        if self.put(fai.fp, fai.i1) || self.put(fai.fp, fai.i2) {
            return Ok(());
        }
        let len = self.buckets.len();
        let mut rng = rand::thread_rng();
        let mut i = fai.random_index(&mut rng);
        let mut fp = fai.fp;
        for _ in 0..MAX_REBUCKET {
            let other_fp;
            {
                let loc = &mut self.buckets[i % len].buffer[rng.gen_range(0, BUCKET_SIZE)];
                other_fp = *loc;
                *loc = fp;
                i = get_alt_index::<H>(other_fp, i);
            }
            if self.put(other_fp, i) {
                return Ok(());
            }
            fp = other_fp;
        }
        // fp is dropped here, which means that the last item that was
        // rebucketed gets removed from the filter.
        // TODO: One could introduce a single-item cache for this element,
        // check this cache in all methods additionally to the actual filter,
        // and return NotEnoughSpace if that cache is already in use.
        // This would complicate the code, but stop random elements from
        // getting removed and result in nicer behaviour for the user.
        Err(CuckooError::NotEnoughSpace)
    }

    /// Adds `data` to the filter if it does not exist in the filter yet.
    /// Returns `Ok(true)` if `data` was not yet present in the filter and added
    /// successfully.
    pub fn test_and_add<T: ?Sized + Hash>(&mut self, data: &T) -> Result<bool, CuckooError> {
        if self.contains(data) {
            Ok(false)
        } else {
            self.add(data).map(|_| true)
        }
    }

    /// Number of items in the filter.
    pub fn len(&self) -> usize {
        self.len
    }

    /// Exports fingerprints in all buckets, along with the filter's length for storage.
    /// The filter can be recovered by passing the `ExportedCuckooFilter` struct to the
    /// `from` method of `CuckooFilter`.
    pub fn export(&self) -> ExportedCuckooFilter {
        self.into()
    }

    /// Number of bytes the filter occupies in memory
    pub fn memory_usage(&self) -> usize {
        mem::size_of_val(self) + self.buckets.len() * mem::size_of::<Bucket>()
    }

    /// Check if filter is empty
    pub fn is_empty(&self) -> bool {
        self.len == 0
    }

    /// Deletes `data` from the filter. Returns true if `data` existed in the
    /// filter before.
    pub fn delete<T: ?Sized + Hash>(&mut self, data: &T) -> bool {
        let FaI { fp, i1, i2 } = get_fai::<T, H>(data);
        self.remove(fp, i1) || self.remove(fp, i2)
    }

    /// Empty all the buckets in a filter and reset the number of items.
    pub fn clear(&mut self) {
        if self.is_empty() {
            return;
        }

        for bucket in self.buckets.iter_mut() {
            bucket.clear();
        }
        self.len = 0;
    }

    /// Extracts fingerprint values from all buckets, used for exporting the filters data.
    fn values(&self) -> Vec<u8> {
        self.buckets
            .iter()
            .flat_map(|b| b.get_fingerprint_data().into_iter())
            .collect()
    }

    /// Removes the item with the given fingerprint from the bucket indexed by i.
    fn remove(&mut self, fp: Fingerprint, i: usize) -> bool {
        let len = self.buckets.len();
        if self.buckets[i % len].delete(fp) {
            self.len -= 1;
            true
        } else {
            false
        }
    }

    fn put(&mut self, fp: Fingerprint, i: usize) -> bool {
        let len = self.buckets.len();
        if self.buckets[i % len].insert(fp) {
            self.len += 1;
            true
        } else {
            false
        }
    }
}

/// A minimal representation of the CuckooFilter which can be transfered or stored, then recovered at a later stage.
#[derive(Debug)]
#[cfg_attr(feature = "serde_support", derive(Deserialize, Serialize))]
pub struct ExportedCuckooFilter {
    #[cfg_attr(feature = "serde_support", serde(with = "serde_bytes"))]
    pub values: Vec<u8>,
    pub length: usize,
}

impl<H> From<ExportedCuckooFilter> for CuckooFilter<H> {
    /// Converts a simplified representation of a filter used for export to a
    /// fully functioning version.
    ///
    /// # Contents
    ///
    /// * `values` - A serialized version of the `CuckooFilter`'s memory, where the
    /// fingerprints in each bucket are chained one after another, then in turn all
    /// buckets are chained together.
    /// * `length` - The number of valid fingerprints inside the `CuckooFilter`.
    /// This value is used as a time saving method, otherwise all fingerprints
    /// would need to be checked for equivalence against the null pattern.
    fn from(exported: ExportedCuckooFilter) -> Self {
        // Assumes that the `BUCKET_SIZE` and `FINGERPRINT_SIZE` constants do not change.
        Self {
            buckets: exported
                .values
                .chunks(BUCKET_SIZE * FINGERPRINT_SIZE)
                .map(Bucket::from)
                .collect::<Vec<_>>()
                .into_boxed_slice(),
            len: exported.length,
            _hasher: PhantomData,
        }
    }
}

impl<H> From<&CuckooFilter<H>> for ExportedCuckooFilter
where
    H: Hasher + Default,
{
    /// Converts a `CuckooFilter` into a simplified version which can be serialized and stored
    /// for later use.
    fn from(cuckoo: &CuckooFilter<H>) -> Self {
        Self {
            values: cuckoo.values(),
            length: cuckoo.len(),
        }
    }
}
