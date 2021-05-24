#!/usr/bin/env bash
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
cat << EOF > "$OUTDIR"/inputs.SHA256SUMS
$(sha256sum "$UNSIGNED_TARBALL" | cut -d' ' -f1)  inputs/$(basename "$UNSIGNED_TARBALL")
$(sha256sum "$CODESIGNATURE_GIT_ARCHIVE" | cut -d' ' -f1)  inputs/$(basename "$CODESIGNATURE_GIT_ARCHIVE")
EOF

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

            # Make an uncompressed DMG from dist/
            xorrisofs -D -l -V "$(< osx_volname)" -no-pad -r -dir-mode 0755 \
                      -o uncompressed.dmg \
                      dist \
                      -- -volume_date all_file_dates ="$SOURCE_DATE_EPOCH"

            # Compress uncompressed.dmg and output to OUTDIR
            ./dmg dmg uncompressed.dmg "${OUTDIR}/${DISTNAME}-osx-signed.dmg"
            ;;
        *)
            exit 1
            ;;
    esac
)  # $DISTSRC

mv --no-target-directory "$OUTDIR" "$ACTUAL_OUTDIR"
