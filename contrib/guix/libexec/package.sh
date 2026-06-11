#!/usr/bin/env bash
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.
export LC_ALL=C
set -e -o pipefail

(
    cd "$DISTSRC"

    (
        cd installed

        case "$HOST" in
            *darwin*) ;;
            *)
                # Split binaries from their debug symbols
                {
                    find "${DISTNAME}/bin" "${DISTNAME}/libexec" -type f -executable -print0
                } | xargs -0 -P"$JOBS" -I{} "${DISTSRC}/build/split-debug.sh" {} {} {}.dbg
                ;;
        esac

        case "$HOST" in
            *mingw*)
                cp "${DISTSRC}/doc/README_windows.txt" "${DISTNAME}/readme.txt"
                ;;
            *linux*)
                cp "${DISTSRC}/README.md" "${DISTNAME}/"
                cp "${DISTSRC}/doc/INSTALL_linux.md" "${DISTNAME}/INSTALL.md"
                ;;
        esac

        # copy over the example bitcoin.conf file. if contrib/devtools/gen-bitcoin-conf.sh
        # has not been run before buildling, this file will be a stub
        cp "${DISTSRC}/share/examples/bitcoin.conf" "${DISTNAME}/"

        cp -r "${DISTSRC}/share/rpcauth" "${DISTNAME}/share/"

        # Deterministically produce {non-,}debug binary tarballs ready
        # for release
        case "$HOST" in
            *mingw*)
                find "${DISTNAME}" -not -name "*.dbg" -print0 \
                    | xargs -0r touch --no-dereference --date="@${SOURCE_DATE_EPOCH}"
                find "${DISTNAME}" -not -name "*.dbg" \
                    | sort \
                    | zip -X@ "${OUTDIR}/${DISTNAME}-${HOST//x86_64-w64-mingw32/win64}-unsigned.zip" \
                    || ( rm -f "${OUTDIR}/${DISTNAME}-${HOST//x86_64-w64-mingw32/win64}-unsigned.zip" && exit 1 )
                find "${DISTNAME}" -name "*.dbg" -print0 \
                    | xargs -0r touch --no-dereference --date="@${SOURCE_DATE_EPOCH}"
                find "${DISTNAME}" -name "*.dbg" \
                    | sort \
                    | zip -X@ "${OUTDIR}/${DISTNAME}-${HOST//x86_64-w64-mingw32/win64}-debug.zip" \
                    || ( rm -f "${OUTDIR}/${DISTNAME}-${HOST//x86_64-w64-mingw32/win64}-debug.zip" && exit 1 )
                ;;
            *linux*)
                find "${DISTNAME}" -not -name "*.dbg" -print0 \
                    | sort --zero-terminated \
                    | tar --create --no-recursion --mode='u+rw,go+r-w,a+X' --null --files-from=- \
                    | gzip -9n > "${OUTDIR}/${DISTNAME}-${HOST}.tar.gz" \
                    || ( rm -f "${OUTDIR}/${DISTNAME}-${HOST}.tar.gz" && exit 1 )
                find "${DISTNAME}" -name "*.dbg" -print0 \
                    | sort --zero-terminated \
                    | tar --create --no-recursion --mode='u+rw,go+r-w,a+X' --null --files-from=- \
                    | gzip -9n > "${OUTDIR}/${DISTNAME}-${HOST}-debug.tar.gz" \
                    || ( rm -f "${OUTDIR}/${DISTNAME}-${HOST}-debug.tar.gz" && exit 1 )
                ;;
            *darwin*)
                find "${DISTNAME}" -print0 \
                    | sort --zero-terminated \
                    | tar --create --no-recursion --mode='u+rw,go+r-w,a+X' --null --files-from=- \
                    | gzip -9n > "${OUTDIR}/${DISTNAME}-${HOST}-unsigned.tar.gz" \
                    || ( rm -f "${OUTDIR}/${DISTNAME}-${HOST}-unsigned.tar.gz" && exit 1 )
                ;;
        esac
    )  # $DISTSRC/installed

    # Finally make tarballs for codesigning
    case "$HOST" in
        *mingw*)
            cp -rf --target-directory=. contrib/windeploy
            (
                cd ./windeploy
                mkdir -p unsigned
                cp --target-directory=unsigned/ "${OUTDIR}/${DISTNAME}-win64-setup-unsigned.exe"
                cp -r --target-directory=unsigned/ "${INSTALLPATH}"
                find unsigned/ -name "*.dbg" -print0 \
                    | xargs -0r rm
                find . -print0 \
                    | sort --zero-terminated \
                    | tar --create --no-recursion --mode='u+rw,go+r-w,a+X' --null --files-from=- \
                    | gzip -9n > "${OUTDIR}/${DISTNAME}-win64-codesigning.tar.gz" \
                    || ( rm -f "${OUTDIR}/${DISTNAME}-win64-codesigning.tar.gz" && exit 1 )
            )
            ;;
        *darwin*)
            cmake --build build --target deploy
            mv build/dist/bitcoin-macos-app.zip "${OUTDIR}/${DISTNAME}-${HOST}-unsigned.zip"
            mkdir -p "unsigned-app-${HOST}"
            cp  --target-directory="unsigned-app-${HOST}" \
                contrib/macdeploy/detached-sig-create.sh
            mv --target-directory="unsigned-app-${HOST}" build/dist
            cp -r --target-directory="unsigned-app-${HOST}" "${INSTALLPATH}"
            (
                cd "unsigned-app-${HOST}"
                find . -print0 \
                    | sort --zero-terminated \
                    | tar --create --no-recursion --mode='u+rw,go+r-w,a+X' --null --files-from=- \
                    | gzip -9n > "${OUTDIR}/${DISTNAME}-${HOST}-codesigning.tar.gz" \
                    || ( rm -f "${OUTDIR}/${DISTNAME}-${HOST}-codesigning.tar.gz" && exit 1 )
            )
            ;;
    esac

) # $DISTSRC

rm -rf "$ACTUAL_OUTDIR"
mv --no-target-directory "$OUTDIR" "$ACTUAL_OUTDIR" \
    || ( rm -rf "$ACTUAL_OUTDIR" && exit 1 )

(
    tmp="$(mktemp)"
    cd /outdir-base
    {
        echo "$GIT_ARCHIVE"
        find "$ACTUAL_OUTDIR" -type f
    } | xargs realpath --relative-base="$PWD" \
        | xargs sha256sum \
        | sort -k2 \
        > "$tmp";
    mv "$tmp" "$ACTUAL_OUTDIR"/SHA256SUMS.part
)
