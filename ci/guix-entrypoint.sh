#!/usr/bin/env bash
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
set -o errexit -o pipefail

# Guix expires cached time-machine profiles after 30 days. Refresh the image's
# prepared profile so an older cached image still reuses it.
touch -h /var/guix/profiles/per-user/root/inferiors/*

guix-daemon --build-users-group=guixbuild &
daemon_pid=$!
until guix gc --list-failures > /dev/null; do
    kill -0 "$daemon_pid" || exit 1
    sleep 0.1
done
guix archive --authorize < /guix-fish-signing-key.pub

exec "$@"
