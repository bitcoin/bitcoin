#!/usr/bin/env sh

set -ex

rustc ci/style.rs && ./style src

if rustup component add rustfmt-preview ; then
    which rustfmt
    rustfmt -V
    cargo fmt --all -- --check
fi

if shellcheck --version ; then
    shellcheck -e SC2103 ci/*.sh
else
    echo "shellcheck not found"
    exit 1
fi

