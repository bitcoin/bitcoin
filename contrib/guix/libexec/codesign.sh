#!/usr/bin/env bash
# Copyright (c) 2021-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
export LC_ALL=C
set -e -o pipefail
export TZ=UTC

# Although Guix _does_ set umask when building its own packages (in our case,
# this is all packages in manifest.scm), it does not set it for `guix
# environment`. It does make sense for at least `guix environment --container`
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
    UNSIGNED_TARBALL: ${UNSIGNED_TARBALL:?not set}
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

    tar -xf "$UNSIGNED_TARBALL"

    mkdir -p codesignatures
    tar -C codesignatures -xf "$CODESIGNATURE_GIT_ARCHIVE"

    case "$HOST" in
        *mingw*)
            find "$PWD" -name "*-unsigned.exe" | while read -r infile; do
                infile_base="$(basename "$infile")"

                # Codesigned *-unsigned.exe and output to OUTDIR
                osslsigncode attach-signature \
                                 -in "$infile" \
                                 -out "${OUTDIR}/${infile_base/-unsigned}" \
                                 -sigin codesignatures/win/"$infile_base".pem
            done
            ;;
        *darwin*)
            # Apply detached codesignatures to dist/ (in-place)
            signapple apply dist/Syscoin-Qt.app codesignatures/osx/dist

            # Make a DMG from dist/
            xorrisofs -D -l -V "$(< osx_volname)" -no-pad -r -dir-mode 0755 \
                      -o "${OUTDIR}/${DISTNAME}-${HOST}.dmg" \
                      dist \
                      -- -volume_date all_file_dates ="$SOURCE_DATE_EPOCH"
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
        echo "$UNSIGNED_TARBALL"
        echo "$CODESIGNATURE_GIT_ARCHIVE"
        find "$ACTUAL_OUTDIR" -type f
    } | xargs realpath --relative-base="$PWD" \
        | xargs sha256sum \
        | sort -k2 \
        | sponge "$ACTUAL_OUTDIR"/SHA256SUMS.part
)
