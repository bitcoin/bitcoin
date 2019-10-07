[![Azure Status]][Azure] [![Cirrus-CI Status]][Cirrus-CI] [![Latest Version]][crates.io] [![Documentation]][docs.rs] ![License]

libc - Raw FFI bindings to platforms' system libraries
====

`libc` provides all of the definitions necessary to easily interoperate with C
code (or "C-like" code) on each of the platforms that Rust supports. This
includes type definitions (e.g. `c_int`), constants (e.g. `EINVAL`) as well as
function headers (e.g. `malloc`).

This crate exports all underlying platform types, functions, and constants under
the crate root, so all items are accessible as `libc::foo`. The types and values
of all the exported APIs match the platform that libc is compiled for.

More detailed information about the design of this library can be found in its
[associated RFC][rfc].

[rfc]: https://github.com/rust-lang/rfcs/blob/master/text/1291-promote-libc.md

## Usage

Add the following to your `Cargo.toml`:

```toml
[dependencies]
libc = "0.2"
```

## Features

* `std`: by default `libc` links to the standard library. Disable this
  feature remove this dependency and be able to use `libc` in `#![no_std]`
  crates.

* `extra_traits`: all `struct`s implemented in `libc` are `Copy` and `Clone`.
  This feature derives `Debug`, `Eq`, `Hash`, and `PartialEq`.

* **deprecated**: `use_std` is deprecated, and is equivalent to `std`.

## Rust version support

The minimum supported Rust toolchain version is **Rust 1.13.0** . APIs requiring
newer Rust features are only available on newer Rust toolchains:

| Feature              | Version |
|----------------------|---------|
| `union`              |  1.19.0 |
| `const mem::size_of` |  1.24.0 |
| `repr(align)`        |  1.25.0 |
| `extra_traits`      |  1.25.0 |
| `core::ffi::c_void`  |  1.30.0 |
| `repr(packed(N))` |  1.33.0 |

## Platform support

[Platform-specific documentation (master branch)][docs.master].

See
[`ci/build.sh`](https://github.com/rust-lang/libc/blob/master/ci/build.sh)
for the platforms on which `libc` is guaranteed to build for each Rust
toolchain. The test-matrix at [Travis-CI], [Appveyor], and [Cirrus-CI] show the
platforms in which `libc` tests are run.

<div class="platform_docs"></div>

## License

This project is licensed under either of

* [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0)
  ([LICENSE-APACHE](LICENSE-APACHE))

* [MIT License](http://opensource.org/licenses/MIT)
  ([LICENSE-MIT](LICENSE-MIT))

at your option.

## Contributing

We welcome all people who want to contribute. Please see the [contributing
instructions] for more information.

[contributing instructions]: CONTRIBUTING.md

Contributions in any form (issues, pull requests, etc.) to this project
must adhere to Rust's [Code of Conduct].

[Code of Conduct]: https://www.rust-lang.org/en-US/conduct.html

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in `libc` by you, as defined in the Apache-2.0 license, shall be
dual licensed as above, without any additional terms or conditions.

[Azure Status]: https://dev.azure.com/rust-lang2/libc/_apis/build/status/rust-lang.libc?branchName=master
[Azure]: https://dev.azure.com/rust-lang2/libc/_build/latest?definitionId=1&branchName=master
[Cirrus-CI]: https://cirrus-ci.com/github/rust-lang/libc
[Cirrus-CI Status]: https://api.cirrus-ci.com/github/rust-lang/libc.svg
[crates.io]: https://crates.io/crates/libc
[Latest Version]: https://img.shields.io/crates/v/libc.svg
[Documentation]: https://docs.rs/libc/badge.svg
[docs.rs]: https://docs.rs/libc
[License]: https://img.shields.io/crates/l/libc.svg
[docs.master]: https://rust-lang.github.io/libc/#platform-specific-documentation
