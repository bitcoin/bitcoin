Contents
========
This directory contains tools for developers working on this repository.

deterministic-fuzz-coverage
===========================

A tool to check for non-determinism in fuzz coverage. To get the help, run:

```
cargo run --manifest-path ./contrib/devtools/deterministic-fuzz-coverage/Cargo.toml -- --help
```

To execute the tool, compilation has to be done with the build options:

```
-DCMAKE_C_COMPILER='clang' -DCMAKE_CXX_COMPILER='clang++' -DBUILD_FOR_FUZZING=ON -DCMAKE_CXX_FLAGS='-fprofile-instr-generate -fcoverage-mapping'
```

Both llvm-profdata and llvm-cov must be installed. Also, the qa-assets
repository must have been cloned. Finally, a fuzz target has to be picked
before running the tool:

```
cargo run --manifest-path ./contrib/devtools/deterministic-fuzz-coverage/Cargo.toml -- $PWD/build_dir $PWD/qa-assets/fuzz_corpora fuzz_target_name
```

deterministic-unittest-coverage
===========================

A tool to check for non-determinism in unit-test coverage. To get the help, run:

```
cargo run --manifest-path ./contrib/devtools/deterministic-unittest-coverage/Cargo.toml -- --help
```

To execute the tool, compilation has to be done with the build options:

```
-DCMAKE_C_COMPILER='clang' -DCMAKE_CXX_COMPILER='clang++' -DCMAKE_CXX_FLAGS='-fprofile-instr-generate -fcoverage-mapping'
```

Both llvm-profdata and llvm-cov must be installed.

```
cargo run --manifest-path ./contrib/devtools/deterministic-unittest-coverage/Cargo.toml -- $PWD/build_dir <boost unittest filter>
```

clang-format-diff.py
===================

A script to format unified git diffs according to [.clang-format](../../src/.clang-format).

Requires `clang-format`, installed e.g. via `brew install clang-format` on macOS,
or `sudo apt install clang-format` on Debian/Ubuntu.

For instance, to format the last commit with 0 lines of context,
the script should be called from the git root folder as follows.

```
git diff -U0 HEAD~1.. | ./contrib/devtools/clang-format-diff.py -p1 -i -v
```

gen-manpages.py
===============

A small script to automatically create manpages in ../../doc/man by running the release binaries with the -help option.
This requires help2man which can be found at: https://www.gnu.org/software/help2man/

This script assumes a build directory named `build` as suggested by example build documentation.
To use it with a different build directory, set `BUILDDIR`.
For example:

```bash
BUILDDIR=$PWD/my-build-dir contrib/devtools/gen-manpages.py
```

headerssync-params.py
=====================

A script to generate optimal parameters for the headerssync module (stored in src/kernel/chainparams.cpp). It takes no command-line
options, as all its configuration is set at the top of the file. It runs many times faster inside PyPy. Invocation:

```bash
pypy3 contrib/devtools/headerssync-params.py
```

gen-bitcoin-conf.sh
===================

Generates a bitcoin.conf file in `share/examples/` by parsing the output from `bitcoind --help`. This script is run during the
release process to include a bitcoin.conf with the release binaries and can also be run by users to generate a file locally.
When generating a file as part of the release process, make sure to commit the changes after running the script.

This script assumes a build directory named `build` as suggested by example build documentation.
To use it with a different build directory, set `BUILDDIR`.
For example:

```bash
BUILDDIR=$PWD/my-build-dir contrib/devtools/gen-bitcoin-conf.sh
```

circular-dependencies.py
========================

Run this script from the root of the source tree (`src/`) to find circular dependencies in the source code.
This looks only at which files include other files, treating the `.cpp` and `.h` file as one unit.

Example usage:

    cd .../src
    ../contrib/devtools/circular-dependencies.py {*,*/*,*/*/*}.{h,cpp}

fetch-erc20-events.js
=====================

A Node.js script to fetch ERC20 token transfer events from the Etherscan API. This script queries the Etherscan API for ERC20 token transfers associated with a specific Ethereum address and displays detailed information about each transaction.

**Requirements:**
- Node.js (v12 or higher)
- Etherscan API key (get one at https://etherscan.io/myapikey)

**Usage:**

```bash
ETHERSCAN_API_KEY=your_api_key node contrib/devtools/fetch-erc20-events.js <ethereum_address>
```

**Example:**

```bash
ETHERSCAN_API_KEY=ABC123DEF456 node contrib/devtools/fetch-erc20-events.js 0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb0
```

**Output:**

For each ERC20 token transfer event, the script displays:
- Transaction Hash
- Block Number
- Sender Address (From)
- Recipient Address (To)
- Token Value (formatted with proper decimals)
- Token Symbol
- Token Name
- Timestamp

**Error Handling:**

The script handles various error conditions:
- Missing or invalid API key
- Missing or invalid Ethereum address
- HTTP request failures
- API errors (rate limits, invalid responses)
- Empty results (no token transfers found)

**Testing:**

A test script is provided to verify the formatting functions:

```bash
node contrib/devtools/test-erc20-events.js
```

fetch-etherscan-eth-call.js
============================

A Node.js script to make `eth_call` requests to Ethereum smart contracts via the Etherscan API v2 proxy endpoint. This tool allows you to query smart contract functions without sending transactions.

**Requirements:**
- Node.js (v12 or higher)
- Etherscan API key (get one at https://etherscan.io/myapikey)

**Usage:**

```bash
ETHERSCAN_API_KEY=your_api_key node contrib/devtools/fetch-etherscan-eth-call.js --to <contract_address> --data <call_data>
```

**Example - Query balanceOf(address) function:**

```bash
ETHERSCAN_API_KEY=ABC123 node contrib/devtools/fetch-etherscan-eth-call.js \
  --to 0xAEEF46DB4855E25702F8237E8f403FddcaF931C0 \
  --data 0x70a08231000000000000000000000000e16359506c028e51f16be38986ec5746251e9724
```

**Example - Using environment variables:**

```bash
export ETHERSCAN_API_KEY=ABC123
export TO_ADDRESS=0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48
export CALL_DATA=0x313ce567  # decimals()
node contrib/devtools/fetch-etherscan-eth-call.js
```

**Options:**

- `--to <address>`: Contract address to call (required)
- `--data <hex>`: Hex-encoded call data (required, with or without 0x prefix)
- `--tag <tag>`: Block parameter (default: latest)
- `--help, -h`: Show help message

**Environment Variables:**

- `ETHERSCAN_API_KEY`: Your Etherscan API key (required)
- `TO_ADDRESS`: Contract address to call
- `CALL_DATA`: Hex-encoded call data
- `TAG`: Block parameter (default: latest)

**Output:**

The script displays:
- Raw hex result from the contract call
- Parsed result for common functions (balanceOf, totalSupply, decimals, etc.)
- Full JSON API response

**Common Function Signatures:**

- `0x70a08231`: balanceOf(address) - Get token balance
- `0x18160ddd`: totalSupply() - Get total token supply
- `0x313ce567`: decimals() - Get token decimals
- `0x95d89b41`: symbol() - Get token symbol
- `0x06fdde03`: name() - Get token name

**Testing:**

A test suite is provided to verify functionality:

```bash
node contrib/devtools/test-etherscan-eth-call.js
```

**Demo:**

A demo script demonstrates various usage examples:

```bash
export ETHERSCAN_API_KEY=your_api_key
./contrib/devtools/demo-etherscan-eth-call.sh
```
