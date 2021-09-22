#!/usr/bin/env bash

export LC_ALL=C

set -e
cd "$(dirname $0)/../.."
test/lint/lint-files.py
