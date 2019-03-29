# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Changed
- Serde support is now behind the feature flag `serde_support` and is disabled by default.

## [v0.4.0] - 2018-04-1
### Added
- `ExportedCuckooFilter` adds the ability to serialize the memory map of a `CuckooFilter` via Serde; reducing communication overhead between nodes for example, or the ability to store the current state on disk for retrieval at a later time.
- Added a C interface for embedding this crate into other languages.
  The interface is an additional crate, located in the cabi/ subfolder.
### Changed
- add() now returns Result<(), CuckooError> instead of a bool, and returns a NotEnoughSpaceError instead of panicking
  when insertion fails.
- len() now returns usize instead of u64 to match std's data structures' len() functions.
- with_capacity() now takes an usize instead of an u64 to match std's data structures' with_capacity() functions.

## [v0.3.2]
### Added
- Filters now have a memory_usage() function that return how much bytes a given filter occupies in memory.
  Let's show how little memory the filters need for their capacity!
### Fixed
- Use std::collections::hash_map::DefaultHasher as replacement for std::hah::SipHasher as default hasher, as
  SipHasher is deprecated since Rust 1.13.
- The same part of the item hash was used for generating the fingerprint as well as the index positions. This means that
  equal fingerprints always had the same index positions, resulting in increased rebucketing and less items fitting in
  the filter.

[v0.4.0]: https://github.com/seiflotfy/rust-cuckoofilter/compare/v0.4.0...HEAD
[v0.4.0]: https://github.com/seiflotfy/rust-cuckoofilter/compare/v0.3.2...v0.4.0
[v0.3.2]: https://github.com/seiflotfy/rust-cuckoofilter/compare/v0.3.1...v0.3.2
