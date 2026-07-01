# AGENTS.md

Guidance for AI coding agents working in the Bitcoin Core repository.

## About Bitcoin

> A purely peer-to-peer version of electronic cash would allow online payments
> to be sent directly from one party to another without going through a
> financial institution. Digital signatures provide part of the solution, but
> the main benefits are lost if a trusted third party is still required to
> prevent double-spending. We propose a solution to the double-spending problem
> using a peer-to-peer network.
>
> — Satoshi Nakamoto, *Bitcoin: A Peer-to-Peer Electronic Cash System*

## Project overview

Bitcoin Core is the reference implementation of the Bitcoin protocol. It is a
large C++ codebase (with a Python-based functional test suite) that builds the
`bitcoind` daemon, the `bitcoin-cli`/`bitcoin-tx`/`bitcoin-util` tools, the
`bitcoin-qt` GUI, and the `bitcoin-wallet` tool.

## Repository layout

- `src/` — C++ source for the node, wallet, GUI, and tools.
- `src/test/` — C++ unit tests (Boost).
- `src/test/fuzz/` — fuzz targets.
- `test/functional/` — Python end-to-end/functional tests.
- `test/lint/` — linters run in CI.
- `doc/` — developer and user documentation. Start with
  [`doc/developer-notes.md`](doc/developer-notes.md) and
  [`doc/build-unix.md`](doc/build-unix.md).
- `contrib/` — helper scripts and tooling.

## Build

Bitcoin Core uses CMake.

```sh
cmake -B build
cmake --build build -j"$(nproc)"
```

See [`doc/build-unix.md`](doc/build-unix.md) (and the other `doc/build-*.md`
files) for platform-specific dependencies and options.

## Testing

- Unit tests: `ctest --test-dir build` or run `build/bin/test_bitcoin`.
- Functional tests: `build/test/functional/test_runner.py`.
- Run a single functional test: `build/test/functional/test_runner.py <name>`.

Always run the relevant tests for the area you change before committing.

## Coding style

Follow [`doc/developer-notes.md`](doc/developer-notes.md). Key points:

- Formatting is defined by [`src/.clang-format`](src/.clang-format); run
  `clang-format` on changed lines (see
  [`contrib/devtools/README.md`](contrib/devtools/README.md)).
- 4-space indentation, no tabs. Braces on new lines for classes/functions,
  same line for everything else.
- Naming: `snake_case` for variables, `m_` prefix for members, `g_` prefix for
  globals, `UpperCamelCase` for classes/functions, `ALL_CAPS` for constants.
- Prefer `++i` over `i++`, `static_assert` over `assert`, and named/functional
  casts over C-style casts.
- Don't reformat unrelated code; keep diffs minimal and focused.

## Contributing conventions

- Read [`CONTRIBUTING.md`](CONTRIBUTING.md) before opening a pull request.
- Keep commits small, logically separate, and well described.
- Update or add tests alongside behavior changes.
- Run the lint suite with `./ci/lint.py` before pushing (see
  [`test/lint/README.md`](test/lint/README.md)).
