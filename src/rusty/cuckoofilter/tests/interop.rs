use cuckoofilter::{CuckooFilter, ExportedCuckooFilter};

use std::collections::hash_map::DefaultHasher;

#[test]
fn interoperability() {
    let total_items = 1_000_000;

    let mut filter = CuckooFilter::<DefaultHasher>::with_capacity(total_items);

    let mut num_inserted: u64 = 0;
    // Fit as many values in as possible, count how many made it in.
    for i in 0..total_items {
        match filter.add(&i) {
            Ok(_) => num_inserted += 1,
            Err(_) => break,
        }
    }

    // Export the fingerprint data stored in the filter,
    // along with the filter's current length.
    let store: ExportedCuckooFilter = filter.export();

    // Create a new filter using the `recover` method and the values previously exported.
    let recovered_filter = CuckooFilter::<DefaultHasher>::from(store);

    // The range 0..num_inserted are all known to be in the filter.
    // The filters shouldn't return false negatives, and therefore they should all be contained.
    // Both filters should also be identical.
    for i in 0..num_inserted {
        assert!(filter.contains(&i));
        assert!(recovered_filter.contains(&i));
    }

    // The range total_items..(2 * total_items) are all known *not* to be in the filter.
    // Every element for which the filter claims that it is contained is therefore a false positive, and both the original filter and recovered filter should exhibit the same false positive behaviour.
    for i in total_items..(2 * total_items) {
        assert_eq!(filter.contains(&i), recovered_filter.contains(&i));
    }
}

#[test]
#[cfg(feature = "serde_support")]
fn serialization() {
    // Just a small filter to test serialization.
    let mut filter = CuckooFilter::<DefaultHasher>::with_capacity(100);

    // Fill a few values.
    for i in 0..50 {
        filter.add(&i).unwrap();
    }
    // export data.
    let store: ExportedCuckooFilter = filter.export();

    // serialize using json (for example, any serde format can be used).
    let saved_json = serde_json::to_string(&store).unwrap();

    // create a new filter from the json string.
    let restore_json: ExportedCuckooFilter = serde_json::from_str(&saved_json).unwrap();
    let recovered_filter = CuckooFilter::<DefaultHasher>::from(restore_json);

    // Check our values exist within the reconstructed filter.
    for i in 0..50 {
        assert!(recovered_filter.contains(&i));
    }
}
