#!/usr/bin/env sh

# Builds and runs tests for a particular target passed as an argument to this
# script.

set -ex

MIRRORS_URL="https://rust-lang-ci-mirrors.s3-us-west-1.amazonaws.com/libc"

TARGET="${1}"

# If we're going to run tests inside of a qemu image, then we don't need any of
# the scripts below. Instead, download the image, prepare a filesystem which has
# the current state of this repository, and then run the image.
#
# It's assume that all images, when run with two disks, will run the `run.sh`
# script from the second which we place inside.
if [ "$QEMU" != "" ]; then
  tmpdir=/tmp/qemu-img-creation
  mkdir -p "${tmpdir}"

  if [ -z "${QEMU#*.gz}" ]; then
    # image is .gz : download and uncompress it
    qemufile="$(echo "${QEMU%.gz}" | sed 's/\//__/g')"
    if [ ! -f "${tmpdir}/${qemufile}" ]; then
      curl --retry 5 "${MIRRORS_URL}/${QEMU}" | \
          gunzip -d > "${tmpdir}/${qemufile}"
    fi
  elif [ -z "${QEMU#*.xz}" ]; then
    # image is .xz : download and uncompress it
    qemufile="$(echo "${QEMU%.xz}" | sed 's/\//__/g')"
    if [ ! -f "${tmpdir}/${qemufile}" ]; then
      curl --retry 5 "${MIRRORS_URL}/${QEMU}" | \
          unxz > "${tmpdir}/${qemufile}"
    fi
  else
    # plain qcow2 image: just download it
    qemufile="$(echo "${QEMU}" | sed 's/\//__/g')"
    if [ ! -f "${tmpdir}/${qemufile}" ]; then
      curl --retry 5 "${MIRRORS_URL}/${QEMU}" | \
        > "${tmpdir}/${qemufile}"
    fi
  fi

  # Create a mount a fresh new filesystem image that we'll later pass to QEMU.
  # This will have a `run.sh` script will which use the artifacts inside to run
  # on the host.
  rm -f "${tmpdir}/libc-test.img"
  mkdir "${tmpdir}/mount"

  # Do the standard rigamarole of cross-compiling an executable and then the
  # script to run just executes the binary.
  cargo build \
    --manifest-path libc-test/Cargo.toml \
    --target "${TARGET}" \
    --test main
  rm "${CARGO_TARGET_DIR}/${TARGET}"/debug/main-*.d
  cp "${CARGO_TARGET_DIR}/${TARGET}"/debug/main-* "${tmpdir}"/mount/libc-test
  # shellcheck disable=SC2016
  echo 'exec $1/libc-test' > "${tmpdir}/mount/run.sh"

  du -sh "${tmpdir}/mount"
  genext2fs \
      --root "${tmpdir}/mount" \
      --size-in-blocks 100000 \
      "${tmpdir}/libc-test.img"

  # Pass -snapshot to prevent tampering with the disk images, this helps when
  # running this script in development. The two drives are then passed next,
  # first is the OS and second is the one we just made. Next the network is
  # configured to work (I'm not entirely sure how), and then finally we turn off
  # graphics and redirect the serial console output to out.log.
  qemu-system-x86_64 \
    -m 1024 \
    -snapshot \
    -drive if=virtio,file="${tmpdir}/${qemufile}" \
    -drive if=virtio,file="${tmpdir}/libc-test.img" \
    -net nic,model=virtio \
    -net user \
    -nographic \
    -vga none 2>&1 | tee "${CARGO_TARGET_DIR}/out.log"
  exec egrep "^(PASSED)|(test result: ok)" "${CARGO_TARGET_DIR}/out.log"
fi

# FIXME: x86_64-unknown-linux-gnux32 fail to compile without --release
# See https://github.com/rust-lang/rust/issues/45417
opt=
if [ "$TARGET" = "x86_64-unknown-linux-gnux32" ]; then
  opt="--release"
fi

cargo test -vv $opt --no-default-features --manifest-path libc-test/Cargo.toml \
      --target "${TARGET}"

cargo test -vv $opt --manifest-path libc-test/Cargo.toml --target "${TARGET}"

cargo test -vv $opt --features extra_traits --manifest-path libc-test/Cargo.toml \
      --target "${TARGET}"
