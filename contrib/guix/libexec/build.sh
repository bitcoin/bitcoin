#!/usr/bin/env bash
export LC_ALL=C
set -e -o pipefail
export TZ=UTC

if [ -n "$V" ]; then
    # Print both unexpanded (-v) and expanded (-x) forms of commands as they are
    # read from this file.
    set -vx
    # Set VERBOSE for CMake-based builds
    export VERBOSE="$V"
fi

# Check that environment variables assumed to be set by the environment are set
echo "Building for platform triple ${HOST:?not set} with reference timestamp ${SOURCE_DATE_EPOCH:?not set}..."
echo "At most ${MAX_JOBS:?not set} jobs will run at once..."

#####################
# Environment Setup #
#####################

# The depends folder also serves as a base-prefix for depends packages for
# $HOSTs after successfully building.
BASEPREFIX="${PWD}/depends"

# Setup an output directory for our build
OUTDIR="${OUTDIR:-${PWD}/output}"
[ -e "$OUTDIR" ] || mkdir -p "$OUTDIR"

# Setup the directory where our Widecoin Core build for HOST will occur
DISTSRC="${DISTSRC:-${PWD}/distsrc-${HOST}}"
if [ -e "$DISTSRC" ]; then
    echo "DISTSRC directory '${DISTSRC}' exists, probably because of previous builds... Aborting..."
    exit 1
else
    mkdir -p "$DISTSRC"
fi

# Given a package name and an output name, return the path of that output in our
# current guix environment
store_path() {
    grep --extended-regexp "/[^-]{32}-${1}-[^-]+${2:+-${2}}" "${GUIX_ENVIRONMENT}/manifest" \
        | head --lines=1 \
        | sed --expression='s|^[[:space:]]*"||' \
              --expression='s|"[[:space:]]*$||'
}

# Set environment variables to point Guix's cross-toolchain to the right
# includes/libs for $HOST
case "$HOST" in
    *mingw*)
        # Determine output paths to use in CROSS_* environment variables
        CROSS_GLIBC="$(store_path "mingw-w64-x86_64-winpthreads")"
        CROSS_GCC="$(store_path "gcc-cross-${HOST}")"
        CROSS_GCC_LIBS=( "${CROSS_GCC}/lib/gcc/${HOST}"/* ) # This expands to an array of directories...
        CROSS_GCC_LIB="${CROSS_GCC_LIBS[0]}" # ...we just want the first one (there should only be one)

        NATIVE_GCC="$(store_path gcc-glibc-2.27-toolchain)"
        export LIBRARY_PATH="${NATIVE_GCC}/lib:${NATIVE_GCC}/lib64"
        export CPATH="${NATIVE_GCC}/include"

        export CROSS_C_INCLUDE_PATH="${CROSS_GCC_LIB}/include:${CROSS_GCC_LIB}/include-fixed:${CROSS_GLIBC}/include"
        export CROSS_CPLUS_INCLUDE_PATH="${CROSS_GCC}/include/c++:${CROSS_GCC}/include/c++/${HOST}:${CROSS_GCC}/include/c++/backward:${CROSS_C_INCLUDE_PATH}"
        export CROSS_LIBRARY_PATH="${CROSS_GCC}/lib:${CROSS_GCC}/${HOST}/lib:${CROSS_GCC_LIB}:${CROSS_GLIBC}/lib"
        ;;
    *linux*)
        CROSS_GLIBC="$(store_path "glibc-cross-${HOST}")"
        CROSS_GLIBC_STATIC="$(store_path "glibc-cross-${HOST}" static)"
        CROSS_KERNEL="$(store_path "linux-libre-headers-cross-${HOST}")"
        CROSS_GCC="$(store_path "gcc-cross-${HOST}")"
        CROSS_GCC_LIBS=( "${CROSS_GCC}/lib/gcc/${HOST}"/* ) # This expands to an array of directories...
        CROSS_GCC_LIB="${CROSS_GCC_LIBS[0]}" # ...we just want the first one (there should only be one)

        # NOTE: CROSS_C_INCLUDE_PATH is missing ${CROSS_GCC_LIB}/include-fixed, because
        # the limits.h in it is missing a '#include_next <limits.h>'
        export CROSS_C_INCLUDE_PATH="${CROSS_GCC_LIB}/include:${CROSS_GLIBC}/include:${CROSS_KERNEL}/include"
        export CROSS_CPLUS_INCLUDE_PATH="${CROSS_GCC}/include/c++:${CROSS_GCC}/include/c++/${HOST}:${CROSS_GCC}/include/c++/backward:${CROSS_C_INCLUDE_PATH}"
        export CROSS_LIBRARY_PATH="${CROSS_GCC}/lib:${CROSS_GCC}/${HOST}/lib:${CROSS_GCC_LIB}:${CROSS_GLIBC}/lib:${CROSS_GLIBC_STATIC}/lib"
        ;;
    *)
        exit 1 ;;
esac

# Sanity check CROSS_*_PATH directories
IFS=':' read -ra PATHS <<< "${CROSS_C_INCLUDE_PATH}:${CROSS_CPLUS_INCLUDE_PATH}:${CROSS_LIBRARY_PATH}"
for p in "${PATHS[@]}"; do
    if [ ! -d "$p" ]; then
        echo "'$p' doesn't exist or isn't a directory... Aborting..."
        exit 1
    fi
done

# Disable Guix ld auto-rpath behavior
export GUIX_LD_WRAPPER_DISABLE_RPATH=yes

# Make /usr/bin if it doesn't exist
[ -e /usr/bin ] || mkdir -p /usr/bin

# Symlink file and env to a conventional path
[ -e /usr/bin/file ] || ln -s --no-dereference "$(command -v file)" /usr/bin/file
[ -e /usr/bin/env ]  || ln -s --no-dereference "$(command -v env)"  /usr/bin/env

# Determine the correct value for -Wl,--dynamic-linker for the current $HOST
case "$HOST" in
    *linux*)
        glibc_dynamic_linker=$(
            case "$HOST" in
                i686-linux-gnu)      echo /lib/ld-linux.so.2 ;;
                x86_64-linux-gnu)    echo /lib64/ld-linux-x86-64.so.2 ;;
                arm-linux-gnueabihf) echo /lib/ld-linux-armhf.so.3 ;;
                aarch64-linux-gnu)   echo /lib/ld-linux-aarch64.so.1 ;;
                riscv64-linux-gnu)   echo /lib/ld-linux-riscv64-lp64d.so.1 ;;
                *)                   exit 1 ;;
            esac
        )
        ;;
esac

# Environment variables for determinism
export QT_RCC_TEST=1
export QT_RCC_SOURCE_DATE_OVERRIDE=1
export TAR_OPTIONS="--owner=0 --group=0 --numeric-owner --mtime='@${SOURCE_DATE_EPOCH}' --sort=name"
export TZ="UTC"

####################
# Depends Building #
####################

# Build the depends tree, overriding variables that assume multilib gcc
make -C depends --jobs="$MAX_JOBS" HOST="$HOST" \
                                   ${V:+V=1} \
                                   ${SOURCES_PATH+SOURCES_PATH="$SOURCES_PATH"} \
                                   i686_linux_CC=i686-linux-gnu-gcc \
                                   i686_linux_CXX=i686-linux-gnu-g++ \
                                   i686_linux_AR=i686-linux-gnu-ar \
                                   i686_linux_RANLIB=i686-linux-gnu-ranlib \
                                   i686_linux_NM=i686-linux-gnu-nm \
                                   i686_linux_STRIP=i686-linux-gnu-strip \
                                   x86_64_linux_CC=x86_64-linux-gnu-gcc \
                                   x86_64_linux_CXX=x86_64-linux-gnu-g++ \
                                   x86_64_linux_AR=x86_64-linux-gnu-ar \
                                   x86_64_linux_RANLIB=x86_64-linux-gnu-ranlib \
                                   x86_64_linux_NM=x86_64-linux-gnu-nm \
                                   x86_64_linux_STRIP=x86_64-linux-gnu-strip \
                                   qt_config_opts_i686_linux='-platform linux-g++ -xplatform widecoin-linux-g++'


###########################
# Source Tarball Building #
###########################

# Define DISTNAME variable.
# shellcheck source=contrib/gitian-descriptors/assign_DISTNAME
source contrib/gitian-descriptors/assign_DISTNAME

GIT_ARCHIVE="${OUTDIR}/src/${DISTNAME}.tar.gz"

# Create the source tarball if not already there
if [ ! -e "$GIT_ARCHIVE" ]; then
    mkdir -p "$(dirname "$GIT_ARCHIVE")"
    git archive --prefix="${DISTNAME}/" --output="$GIT_ARCHIVE" HEAD
fi

###########################
# Binary Tarball Building #
###########################

# CONFIGFLAGS
CONFIGFLAGS="--enable-reduce-exports --disable-bench --disable-gui-tests"
case "$HOST" in
    *linux*) CONFIGFLAGS+=" --enable-glibc-back-compat" ;;
esac

# CFLAGS
HOST_CFLAGS="-O2 -g"
case "$HOST" in
    *linux*)  HOST_CFLAGS+=" -ffile-prefix-map=${PWD}=." ;;
    *mingw*)  HOST_CFLAGS+=" -fno-ident" ;;
esac

# CXXFLAGS
HOST_CXXFLAGS="$HOST_CFLAGS"

# LDFLAGS
case "$HOST" in
    *linux*)  HOST_LDFLAGS="-Wl,--as-needed -Wl,--dynamic-linker=$glibc_dynamic_linker -static-libstdc++ -Wl,-O2" ;;
    *mingw*)  HOST_LDFLAGS="-Wl,--no-insert-timestamp" ;;
esac

# Make $HOST-specific native binaries from depends available in $PATH
export PATH="${BASEPREFIX}/${HOST}/native/bin:${PATH}"
(
    cd "$DISTSRC"

    # Extract the source tarball
    tar --strip-components=1 -xf "${GIT_ARCHIVE}"

    ./autogen.sh

    # Configure this DISTSRC for $HOST
    # shellcheck disable=SC2086
    env CONFIG_SITE="${BASEPREFIX}/${HOST}/share/config.site" \
        ./configure --prefix=/ \
                    --disable-ccache \
                    --disable-maintainer-mode \
                    --disable-dependency-tracking \
                    ${CONFIGFLAGS} \
                    CFLAGS="${HOST_CFLAGS}" \
                    CXXFLAGS="${HOST_CXXFLAGS}" \
                    ${HOST_LDFLAGS:+LDFLAGS="${HOST_LDFLAGS}"}

    sed -i.old 's/-lstdc++ //g' config.status libtool src/univalue/config.status src/univalue/libtool

    # Build Widecoin Core
    make --jobs="$MAX_JOBS" ${V:+V=1}

    # Perform basic ELF security checks on a series of executables.
    make -C src --jobs=1 check-security ${V:+V=1}

    case "$HOST" in
        *linux*|*mingw*)
            # Check that executables only contain allowed gcc, glibc and libstdc++
            # version symbols for Linux distro back-compatibility.
            make -C src --jobs=1 check-symbols  ${V:+V=1}
            ;;
    esac

    # Make the os-specific installers
    case "$HOST" in
        *mingw*)
            make deploy ${V:+V=1} WIDECOIN_WIN_INSTALLER="${OUTDIR}/${DISTNAME}-win64-setup-unsigned.exe"
            ;;
    esac

    # Setup the directory where our Widecoin Core build for HOST will be
    # installed. This directory will also later serve as the input for our
    # binary tarballs.
    INSTALLPATH="${PWD}/installed/${DISTNAME}"
    mkdir -p "${INSTALLPATH}"
    # Install built Widecoin Core to $INSTALLPATH
    make install DESTDIR="${INSTALLPATH}" ${V:+V=1}

    (
        cd installed

        case "$HOST" in
            *mingw*)
                mv --target-directory="$DISTNAME"/lib/ "$DISTNAME"/bin/*.dll
                ;;
        esac

        # Prune libtool and object archives
        find . -name "lib*.la" -delete
        find . -name "lib*.a" -delete

        # Prune pkg-config files
        rm -r "${DISTNAME}/lib/pkgconfig"

        # Split binaries and libraries from their debug symbols
        {
            find "${DISTNAME}/bin" -type f -executable -print0
            find "${DISTNAME}/lib" -type f -print0
        } | xargs -0 -n1 -P"$MAX_JOBS" -I{} "${DISTSRC}/contrib/devtools/split-debug.sh" {} {} {}.dbg

        case "$HOST" in
            *mingw*)
                cp "${DISTSRC}/doc/README_windows.txt" "${DISTNAME}/readme.txt"
                ;;
            *linux*)
                cp "${DISTSRC}/README.md" "${DISTNAME}/"
                ;;
        esac

        # Finally, deterministically produce {non-,}debug binary tarballs ready
        # for release
        case "$HOST" in
            *mingw*)
                find "${DISTNAME}" -not -name "*.dbg" -print0 \
                    | xargs -0r touch --no-dereference --date="@${SOURCE_DATE_EPOCH}"
                find "${DISTNAME}" -not -name "*.dbg" \
                    | sort \
                    | zip -X@ "${OUTDIR}/${DISTNAME}-${HOST//x86_64-w64-mingw32/win64}.zip" \
                    || ( rm -f "${OUTDIR}/${DISTNAME}-${HOST//x86_64-w64-mingw32/win64}.zip" && exit 1 )
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
        esac
    )
)

case "$HOST" in
    *mingw*)
        cp -rf --target-directory=. contrib/windeploy
        (
            cd ./windeploy
            mkdir unsigned
            cp --target-directory=unsigned/ "${OUTDIR}/${DISTNAME}-win64-setup-unsigned.exe"
            find . -print0 \
                | sort --zero-terminated \
                | tar --create --no-recursion --mode='u+rw,go+r-w,a+X' --null --files-from=- \
                | gzip -9n > "${OUTDIR}/${DISTNAME}-win-unsigned.tar.gz" \
                || ( rm -f "${OUTDIR}/${DISTNAME}-win-unsigned.tar.gz" && exit 1 )
        )
        ;;
esac
