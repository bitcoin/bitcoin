# Cuckoo Filter

[![Crates.io](https://img.shields.io/crates/v/cuckoofilter.svg?maxAge=2592000)](https://crates.io/crates/cuckoofilter)

[Documentation](https://docs.rs/cuckoofilter)


Cuckoo filter is a Bloom filter replacement for approximated set-membership queries. While Bloom filters are well-known space-efficient data structures to serve queries like "if item x is in a set?", they do not support deletion. Their variances to enable deletion (like counting Bloom filters) usually require much more space.

Cuckoo ﬁlters provide the ﬂexibility to add and remove items dynamically. A cuckoo filter is based on cuckoo hashing (and therefore named as cuckoo filter). It is essentially a cuckoo hash table storing each key's fingerprint. Cuckoo hash tables can be highly compact, thus a cuckoo filter could use less space than conventional Bloom ﬁlters, for applications that require low false positive rates (< 3%).

For details about the algorithm and citations please use this article for now

["Cuckoo Filter: Better Than Bloom" by Bin Fan, Dave Andersen and Michael Kaminsky](https://www.cs.cmu.edu/~dga/papers/cuckoo-conext2014.pdf)


## Example usage

```rust
extern crate cuckoofilter;

...

let value: &str = "hello world";

// Create cuckoo filter with default max capacity of 1000000 items
let mut cf = cuckoofilter::new();

// Add data to the filter
let success = cf.add(value).unwrap();
// success ==> Ok(())

// Lookup if data is in the filter
let success = cf.contains(value);
// success ==> true

// Test and add to the filter (if data does not exists then add)
let success = cf.test_and_add(value).unwrap();
// success ==> Ok(false)

// Remove data from the filter.
let success = cf.delete(value);
// success ==> true
```

## C Interface
This crate has a C interface for embedding it into other languages than Rust.
See the [C Interface Documentation](https://docs.rs/cuckoofilter_cabi) for more details.


## Notes & TODOs
* This implementation uses a a static bucket size of 4 fingerprints and a fingerprint size of 1 byte based on my understanding of an optimal bucket/fingerprint/size ratio from the aforementioned paper.
* When the filter returns `NotEnoughSpace`, the element given is actually added to the filter, but some random *other*
  element gets removed. This could be improved by implementing a single-item eviction cache for that removed item.
* There are no high-level bindings for other languages than C.
  One could add them e.g. for python using [milksnake](https://github.com/getsentry/milksnake).
