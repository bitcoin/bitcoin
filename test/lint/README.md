This folder contains lint scripts.

Running locally
===============

To run linters locally with the same versions as the CI environment, use the included
Dockerfile:

```sh
DOCKER_BUILDKIT=1 docker build -t bitcoin-linter --file "./ci/lint_imagefile" ./ && docker run --rm -v $(pwd):/bitcoin -it bitcoin-linter
```

Building the container can be done every time, because it is fast when the
result is cached and it prevents issues when the image changes.

test runner
===========

To run all the lint checks in the test runner outside the docker you first need
to install the rust toolchain using your package manager of choice or
[rustup](https://www.rust-lang.org/tools/install).

Then you can use:

```sh
( cd ./test/lint/test_runner/ && cargo fmt && cargo clippy && RUST_BACKTRACE=1 cargo run )
```

If you wish to run individual lint checks, run the test_runner with
`--lint=TEST_TO_RUN` arguments. If running with `cargo run`, arguments after
`--` are passed to the binary you are running e.g.:

```sh
( cd ./test/lint/test_runner/ && RUST_BACKTRACE=1 cargo run -- --lint=doc --lint=trailing_whitespace )
```

To see a list of all individual lint checks available in test_runner, use `-h`
or `--help`:

```sh
( cd ./test/lint/test_runner/ && RUST_BACKTRACE=1 cargo run -- --help )
```

#### Dependencies

| Lint test | Dependency |
|-----------|:----------:|
| [`lint-python.py`](/test/lint/lint-python.py) | [lief](https://github.com/lief-project/LIEF)
| [`lint-python.py`](/test/lint/lint-python.py) | [mypy](https://github.com/python/mypy)
| [`lint-python.py`](/test/lint/lint-python.py) | [pyzmq](https://github.com/zeromq/pyzmq)
| [`lint-python-dead-code.py`](/test/lint/lint-python-dead-code.py) | [vulture](https://github.com/jendrikseipp/vulture)
| [`lint-shell.py`](/test/lint/lint-shell.py) | [ShellCheck](https://github.com/koalaman/shellcheck)
| [`lint-spelling.py`](/test/lint/lint-spelling.py) | [codespell](https://github.com/codespell-project/codespell)
| `py_lint` | [ruff](https://github.com/astral-sh/ruff)
| markdown link check | [mlc](https://github.com/becheran/mlc)

In use versions and install instructions are available in the [CI setup](../../ci/lint/04_install.sh).

Please be aware that on Linux distributions all dependencies are usually available as packages, but could be outdated.

#### Running the tests

Individual tests can be run by directly calling the test script, e.g.:

```
test/lint/lint-files.py
```

check-doc.py
============
Check for missing documentation of command line options.

commit-script-check.sh
======================
Verification of [scripted diffs](/doc/developer-notes.md#scripted-diffs).
Scripted diffs are only assumed to run on the latest LTS release of Ubuntu. Running them on other operating systems
might require installing GNU tools, such as GNU sed.

git-subtree-check.sh
====================
Run this script from the root of the repository to verify that a subtree matches the contents of
the commit it claims to have been updated to.

```
Usage: test/lint/git-subtree-check.sh [-r] DIR [COMMIT]
       test/lint/git-subtree-check.sh -?
```

- `DIR` is the prefix within the repository to check.
- `COMMIT` is the commit to check, if it is not provided, HEAD will be used.
- `-r` checks that subtree commit is present in repository.

To do a full check with `-r`, make sure that you have fetched the upstream repository branch in which the subtree is
maintained:
* for `src/crc32c`: https://github.com/bitcoin-core/crc32c-subtree.git (branch bitcoin-fork)
* for `src/crypto/ctaes`: https://github.com/bitcoin-core/ctaes.git (branch master)
* for `src/ipc/libmultiprocess`: https://github.com/bitcoin-core/libmultiprocess (branch master)
* for `src/leveldb`: https://github.com/bitcoin-core/leveldb-subtree.git (branch bitcoin-fork)
* for `src/minisketch`: https://github.com/bitcoin-core/minisketch.git (branch master)
* for `src/secp256k1`: https://github.com/bitcoin-core/secp256k1.git (branch master)

Keep this list in sync with `fn get_subtrees()` in the lint runner.

To do so, add the upstream repository as remote:

```
git remote add --fetch secp256k1 https://github.com/bitcoin-core/secp256k1.git
```

lint_ignore_dirs.py
===================
Add list of common directories to ignore when running tests
