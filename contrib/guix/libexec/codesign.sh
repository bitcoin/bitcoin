#!/usr/bin/env bash
# Copyright (c) 2021-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
export LC_ALL=C
set -e -o pipefail

# Environment variables for determinism
export TAR_OPTIONS="--owner=0 --group=0 --numeric-owner --mtime='@${SOURCE_DATE_EPOCH}' --sort=name"
export TZ=UTC

# Although Guix _does_ set umask when building its own packages (in our case,
# this is all packages in manifest.scm), it does not set it for `guix
# shell`. It does make sense for at least `guix shell --container`
# to set umask, so if that change gets merged upstream and we bump the
# time-machine to a commit which includes the aforementioned change, we can
# remove this line.
#
# This line should be placed before any commands which creates files.
umask 0022

if [ -n "$V" ]; then
    # Print both unexpanded (-v) and expanded (-x) forms of commands as they are
    # read from this file.
    set -vx
    # Set VERBOSE for CMake-based builds
    export VERBOSE="$V"
fi

# Check that required environment variables are set
cat << EOF
Required environment variables as seen inside the container:
    CODESIGNING_TARBALL: ${CODESIGNING_TARBALL:?not set}
    DETACHED_SIGS_REPO: ${DETACHED_SIGS_REPO:?not set}
    DIST_ARCHIVE_BASE: ${DIST_ARCHIVE_BASE:?not set}
    DISTNAME: ${DISTNAME:?not set}
    HOST: ${HOST:?not set}
    SOURCE_DATE_EPOCH: ${SOURCE_DATE_EPOCH:?not set}
    DISTSRC: ${DISTSRC:?not set}
    OUTDIR: ${OUTDIR:?not set}
EOF

ACTUAL_OUTDIR="${OUTDIR}"
OUTDIR="${DISTSRC}/output"

git_head_version() {
    local recent_tag
    if recent_tag="$(git -C "$1" describe --exact-match HEAD 2> /dev/null)"; then
        echo "${recent_tag#v}"
    else
        git -C "$1" rev-parse --short=12 HEAD
    fi
}

CODESIGNATURE_GIT_ARCHIVE="${DIST_ARCHIVE_BASE}/${DISTNAME}-codesignatures-$(git_head_version "$DETACHED_SIGS_REPO").tar.gz"

# Create the codesignature tarball if not already there
if [ ! -e "$CODESIGNATURE_GIT_ARCHIVE" ]; then
    mkdir -p "$(dirname "$CODESIGNATURE_GIT_ARCHIVE")"
    git -C "$DETACHED_SIGS_REPO" archive --output="$CODESIGNATURE_GIT_ARCHIVE" HEAD
fi

mkdir -p "$OUTDIR"

mkdir -p "$DISTSRC"
(
    cd "$DISTSRC"

    tar -xf "$CODESIGNING_TARBALL"

    mkdir -p codesignatures
    tar -C codesignatures -xf "$CODESIGNATURE_GIT_ARCHIVE"

    case "$HOST" in
        *mingw*)
            # Apply detached codesignatures
            WORKDIR=".tmp"
            mkdir -p ${WORKDIR}
            cp -r --target-directory="${WORKDIR}" "unsigned/${DISTNAME}"
            find "${WORKDIR}/${DISTNAME}" -name "*.exe" -type f -exec rm {} \;
            find unsigned/ -name "*.exe" -type f | while read -r bin
            do
                bin_base="$(realpath --relative-to=unsigned/ "${bin}")"
                mkdir -p "${WORKDIR}/$(dirname "${bin_base}")"
                osslsigncode attach-signature \
                                 -in "${bin}" \
                                 -out "${WORKDIR}/${bin_base/-unsigned}" \
                                 -CAfile "$GUIX_ENVIRONMENT/etc/ssl/certs/ca-certificates.crt" \
                                 -sigin codesignatures/win/"${bin_base}".pem
            done

            # Move installer to outdir
            cd "${WORKDIR}"
            find . -name "*setup.exe" -print0 \
                | xargs -0r mv --target-directory="${OUTDIR}"

            # Make .zip from binaries
            find "${DISTNAME}" -print0 \
                | xargs -0r touch --no-dereference --date="@${SOURCE_DATE_EPOCH}"
            find "${DISTNAME}" \
                | sort \
                | zip -X@ "${OUTDIR}/${DISTNAME}-${HOST//x86_64-w64-mingw32/win64}.zip" \
                || ( rm -f "${OUTDIR}/${DISTNAME}-${HOST//x86_64-w64-mingw32/win64}.zip" && exit 1 )
            ;;
        *darwin*)
            case "$HOST" in
                arm64*) ARCH="arm64" ;;
                x86_64*) ARCH="x86_64" ;;
            esac

            # Apply detached codesignatures (in-place)
            signapple apply dist/Bitcoin-Qt.app codesignatures/osx/"${HOST}"/dist/Bitcoin-Qt.app
            find "${DISTNAME}" \( -wholename "*/bin/*" -o -wholename "*/libexec/*" \) -type f | while read -r bin
            do
                signapple apply "${bin}" "codesignatures/osx/${HOST}/${bin}.${ARCH}sign"
            done

            # Make a .zip from dist/
            cd dist/
            find . -print0 \
                | xargs -0r touch --no-dereference --date="@${SOURCE_DATE_EPOCH}"
            find . | sort \
                | zip -X@ "${OUTDIR}/${DISTNAME}-${HOST}.zip"
            cd ..

            # Make a .tar.gz from bins
            find "${DISTNAME}" -print0 \
                | sort --zero-terminated \
                | tar --create --no-recursion --mode='u+rw,go+r-w,a+X' --null --files-from=- \
                | gzip -9n > "${OUTDIR}/${DISTNAME}-${HOST}.tar.gz" \
                || ( rm -f "${OUTDIR}/${DISTNAME}-${HOST}.tar.gz" && exit 1 )
            ;;
        *)
            exit 1
            ;;
    esac
)  # $DISTSRC

rm -rf "$ACTUAL_OUTDIR"
mv --no-target-directory "$OUTDIR" "$ACTUAL_OUTDIR" \
    || ( rm -rf "$ACTUAL_OUTDIR" && exit 1 )

(
    cd /outdir-base
    {
        echo "$CODESIGNING_TARBALL"
        echo "$CODESIGNATURE_GIT_ARCHIVE"
        find "$ACTUAL_OUTDIR" -type f
    } | xargs realpath --relative-base="$PWD" \
        | xargs sha256sum \
        | sort -k2 \
        | sponge "$ACTUAL_OUTDIR"/SHA256SUMS.part
)
