# Contributing to `libc`

Welcome! If you are reading this document, it means you are interested in contributing
to the `libc` crate.

## Adding an API

Want to use an API which currently isn't bound in `libc`? It's quite easy to add
one!

The internal structure of this crate is designed to minimize the number of
`#[cfg]` attributes in order to easily be able to add new items which apply
to all platforms in the future. As a result, the crate is organized
hierarchically based on platform. Each module has a number of `#[cfg]`'d
children, but only one is ever actually compiled. Each module then reexports all
the contents of its children.

This means that for each platform that libc supports, the path from a
leaf module to the root will contain all bindings for the platform in question.
Consequently, this indicates where an API should be added! Adding an API at a
particular level in the hierarchy means that it is supported on all the child
platforms of that level. For example, when adding a Unix API it should be added
to `src/unix/mod.rs`, but when adding a Linux-only API it should be added to
`src/unix/linux_like/linux/mod.rs`.

If you're not 100% sure at what level of the hierarchy an API should be added
at, fear not! This crate has CI support which tests any binding against all
platforms supported, so you'll see failures if an API is added at the wrong
level or has different signatures across platforms.

With that in mind, the steps for adding a new API are:

1. Determine where in the module hierarchy your API should be added.
2. Add the API.
3. Send a PR to this repo.
4. Wait for CI to pass, fixing errors.
5. Wait for a merge!

### Test before you commit

We have two automated tests running on [Azure Pipelines](https://dev.azure.com/rust-lang2/libc/_build?definitionId=1&_a=summary):

1. [`libc-test`](https://github.com/gnzlbg/ctest)
  - `cd libc-test && cargo test`
  - Use the `skip_*()` functions in `build.rs` if you really need a workaround.
2. Style checker
  - `rustc ci/style.rs && ./style src`

### Releasing your change to crates.io

Now that you've done the amazing job of landing your new API or your new
platform in this crate, the next step is to get that sweet, sweet usage from
crates.io! The only next step is to bump the version of libc and then publish
it. If you'd like to get a release out ASAP you can follow these steps:

1. Update the version number in `Cargo.toml`, you'll just be bumping the patch
   version number.
2. Run `cargo update` to regenerate the lockfile to encode your version bump in
   the lock file. You may pull in some other updated dependencies, that's ok.
3. Send a PR to this repository. It should [look like this][example], but it'd
   also be nice to fill out the description with a small rationale for the
   release (any rationale is ok though!)
4. Once merged the release will be tagged and published by one of the libc crate
   maintainers.

[example]: https://github.com/rust-lang/libc/pull/583
