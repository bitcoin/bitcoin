
# 0.15.4 - 2019-09-06

- Add `rand-std` feature.
- Pin the cc build-dep version to `< 1.0.42` to remain
  compatible with rustc 1.22.0.
- Changed all `as_*ptr()` to a new safer `CPtr` trait

# 0.15.2 - 2019-08-08

- Add feature `lowmemory` that reduces the EC mult window size to require
  significantly less memory for the validation context (~680B instead of
  ~520kB), at the cost of slower validation. It does not affect the speed of
  signing, nor the size of the signing context.

# 0.15.0 - 2019-07-25

* Implement hex human-readable serde for PublicKey
* Implement fmt::LowerHex for SecretKey and PublicKey
* Relax `cc` dependency requirements
* Add links manifest key to prevent cross-version linkage

# 0.14.1 - 2019-07-14

* Implemented FFI functions: `secp256k1_context_create` and `secp256k1_context_destroy` in rust.

# 0.14.0 - 2019-07-08

* [Feature-gate endormorphism optimization](https://github.com/rust-bitcoin/rust-secp256k1/pull/120)
  because of a lack of clarity with respect to patents
* Got full no-std support including eliminating all use of libc in C bindings.
  [PR 1](https://github.com/rust-bitcoin/rust-secp256k1/pull/115)
  [PR 2](https://github.com/rust-bitcoin/rust-secp256k1/pull/125).
  This library should be usable in bare-metal environments and with rust-wasm.
  Thanks to Elichai Turkel for driving this forward!

# 0.13.0 - 2019-05-21

* Update minimum supported rust compiler 1.22.
* Replace `serialize_der` function with `SerializedSignature` struct.
* Allow building without a standard library (`no_std`). `std` feature is on by default.
* Add human readable serialization to `Signatures` and `SecretKeys`.
* Stop displaying 0 bytes if a `Signature` is less than 72 bytes.
* Only compile recovery module if feature `recovery` is set (non-default).
* Update `rand` dependency from 0.4 to 0.6 and add `rand_core` 0.4 dependency.
* Relax `cc` dependency requirements.

# 0.12.2 - 2019-01-18

* Fuzzer bug fix

# 0.12.1 - 2019-01-15

* Minor bug fixes
* Fixed `cc` crate version to maintain minimum compiler version without breakage
* Removed `libc` dependency as it our uses have been subsumed into stdlib

# 0.12.0 - 2018-12-03

* **Overhaul API to remove context object when no precomputation is needed**
* Add `ThirtyTwoByteHash` trait which allows infallible conversions to `Message`s
* Disallow 0-valued `Message` objects since signatures on them are forgeable for all keys
* Remove `ops::Index` implementations for `Signature`
* Remove depecated constants and unsafe `ZERO_KEY` constant

# 0.11.5 - 2018-11-09

* Use `pub extern crate` to export dependencies whose types are exported

# 0.11.4 - 2018-11-04

* Add `FromStr` and `Display` for `Signature` and both key types
* Fix `build.rs` for Windows and rustfmt configuration for docs.rs
* Correct endianness issue for `Signature` `Debug` output

# 0.11.3 - 2018-10-28

* No changes, just fixed docs.rs configuration

# 0.11.2 - 2018-09-11

* Correct endianness issue in RFC6979 nonce generation

# 0.11.1 - 2018-08-22

* Put `PublicKey::combine` back because it is currently needed to implement Lightning BOLT 3

# 0.11.0 - 2018-08-22

* Update `rand` to 0.4 and `gcc` 0.3 to `cc` 1.0. (`rand` 0.5 exists but has a lot of breaking changes and no longer compiles with 1.14.0.)
* Remove `PublicKey::combine` from API since it cannot be used with anything else in the API
* Detect whether 64-bit compilation is possible, and do it if we can (big performance improvement)

# 0.10.0 - 2018-07-25

* A [complete API overhaul](https://github.com/rust-bitcoin/rust-secp256k1/pull/27) to move many runtime errors into compiletime errors
* Update [libsecp256k1 to `1e6f1f5ad5e7f1e3ef79313ec02023902bf8`](https://github.com/rust-bitcoin/rust-secp256k1/pull/32). Should be no visible changes.
* [Remove `PublicKey::new()` and `PublicKey::is_valid()`](https://github.com/rust-bitcoin/rust-secp256k1/pull/37) since `new` was unsafe and it should now be impossible to create invalid `PublicKey` objects through the API
* [Reintroduce serde support](https://github.com/rust-bitcoin/rust-secp256k1/pull/38) behind a feature gate using serde 1.0
* Clean up build process and various typos


