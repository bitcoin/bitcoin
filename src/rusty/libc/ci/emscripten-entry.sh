#!/usr/bin/env bash
# Copyright 2017 The Rust Project Developers. See the COPYRIGHT
# file at the top-level directory of this distribution and at
# http://rust-lang.org/COPYRIGHT.
#
# Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
# http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
# <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
# option. This file may not be copied, modified, or distributed
# except according to those terms.

set -ex

# shellcheck disable=SC1091
source /emsdk-portable/emsdk_env.sh &> /dev/null

# emsdk-portable provides a node binary, but we need version 8 to run wasm
export PATH="/node-v12.3.1-linux-x64/bin:$PATH"

exec "$@"
