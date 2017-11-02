#!/bin/bash

set -e
set -x

kvm-ok
service apt-cacher-ng start
ls -l /dev/kvm
