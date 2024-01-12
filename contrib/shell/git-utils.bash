#!/usr/bin/env bash

git_root() {
    git rev-parse --show-toplevel 2> /dev/null
}

git_head_version() {
    recent_tag="$(git describe --abbrev=12 --dirty 2> /dev/null)"
    echo "${recent_tag#v}"
}
