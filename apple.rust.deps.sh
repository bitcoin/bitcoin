#!/bin/sh
set -x
set -e
# "x86_64-apple-ios"
# "aarch64-apple-ios"
# "aarch64-apple-ios-sim"
# "x86_64-apple-darwin"
# "aarch64-apple-darwin"
# TODO: it's probably needs to be optimized in order to increase better build velocity
# TODO: so we need to combine multiple targets
TARGET=$1
git submodule update --init
MIN_IOS="13.0"
MIN_WATCHOS="5.0"
MIN_TVOS=$MIN_IOS
MIN_MACOS="10.15"

IPHONEOS=iphoneos
IPHONESIMULATOR=iphonesimulator
WATCHOS=watchos
WATCHSIMULATOR=watchsimulator
TVOS=appletvos
TVSIMULATOR=appletvsimulator
MACOS=macosx

LOGICALCPU_MAX=$(sysctl -n hw.logicalcpu_max)
BUILD=build

version_min_flag() {
    PLATFORM=$1
    FLAG=""
    # shellcheck disable=SC2039
    # shellcheck disable=SC2053
    if [[ $PLATFORM = $IPHONEOS ]]; then
        FLAG="-miphoneos-version-min=${MIN_IOS}"
    elif [[ $PLATFORM = $IPHONESIMULATOR ]]; then
        FLAG="-mios-simulator-version-min=${MIN_IOS}"
    elif [[ $PLATFORM = $WATCHOS ]]; then
        FLAG="-mwatchos-version-min=${MIN_WATCHOS}"
    elif [[ $PLATFORM = $WATCHSIMULATOR ]]; then
        FLAG="-mwatchos-simulator-version-min=${MIN_WATCHOS}"
    elif [[ $PLATFORM = $TVOS ]]; then
        FLAG="-mtvos-version-min=${MIN_TVOS}"
    elif [[ $PLATFORM = $TVSIMULATOR ]]; then
        FLAG="-mtvos-simulator-version-min=${MIN_TVOS}"
    elif [[ $PLATFORM = $MACOS ]]; then
        FLAG="-mmacosx-version-min=${MIN_MACOS}"
    fi
    echo $FLAG
}


prepare() {
    download_gmp() {
        GMP_VERSION="6.3.0"
        CURRENT_DIR=$(pwd)
        echo "$CURRENT_DIR"
        # shellcheck disable=SC2039,SC2164
        pushd ${BUILD}
        mkdir -p "contrib"
        if [ ! -s "contrib/gmp-${GMP_VERSION}.tar.bz2" ]; then
            curl -L -o "contrib/gmp-${GMP_VERSION}.tar.bz2" https://ftp.gnu.org/gnu/gmp/gmp-${GMP_VERSION}.tar.bz2
        fi
        rm -rf "contrib/gmp"
        # shellcheck disable=SC2039,SC2164
        pushd contrib
        tar xfj "gmp-${GMP_VERSION}.tar.bz2"
        mv gmp-${GMP_VERSION} gmp
        # shellcheck disable=SC2039,SC2164
        popd #contrib
        # shellcheck disable=SC2039,SC2164
        popd #build
    }

    download_cmake_toolchain() {
        if [ ! -s "${BUILD}/ios.toolchain.cmake" ]; then
            SHA256_HASH="d02857ff6bd64f1d7109ca59c3e4f3b2f89d0663c412146e6977c679801b3243"
            curl -o "${BUILD}/ios.toolchain.cmake" https://raw.githubusercontent.com/leetal/ios-cmake/c55677a4445b138c9ef2650d3c21f22cc78c2357/ios.toolchain.cmake
            DOWNLOADED_HASH=$(shasum -a 256 ${BUILD}/ios.toolchain.cmake | cut -f 1 -d " ")
            if [ $SHA256_HASH != "$DOWNLOADED_HASH" ]; then
              echo "Error: sha256 checksum of ios.toolchain.cmake mismatch" >&2
              exit 1
            fi
        fi
    }

    download_relic() {
        CURRENT_DIR=$(pwd)
        echo "$CURRENT_DIR"
        mkdir -p "${CURRENT_DIR}/${BUILD}/depends"
        if [ ! -s "${CURRENT_DIR}/${BUILD}/depends/relic" ]; then
            # shellcheck disable=SC2039,SC2164
            pushd "${CURRENT_DIR}/${BUILD}/depends"
            git clone --depth 1 --branch "feat/ios-support" https://github.com/pankcuf/relic
            # shellcheck disable=SC2039,SC2164
            pushd relic
            git fetch --depth 1 origin 19fb6d79a77ade4ae8cd70d2b0ef7aab8720d1ae
            git checkout 19fb6d79a77ade4ae8cd70d2b0ef7aab8720d1ae
            # shellcheck disable=SC2039,SC2164
            popd #relic
            # shellcheck disable=SC2039,SC2164
            popd #depends
        fi
    }
    rm -rf ${BUILD}
    mkdir -p ${BUILD}
    download_relic
    download_gmp
    download_cmake_toolchain
}

build_gmp_arch() {
    PLATFORM=$1
    ARCH=$2
    PFX=${PLATFORM}-${ARCH}
    # why this works with this host only?
    HOST=aarch64-apple-darwin
    # shellcheck disable=SC2039,SC2164
    pushd ${BUILD}
    SDK=$(xcrun --sdk "$PLATFORM" --show-sdk-path)
    PLATFORM_PATH=$(xcrun --sdk "$PLATFORM" --show-sdk-platform-path)
    CLANG=$(xcrun --sdk "$PLATFORM" --find clang)
    DEVELOPER=$(xcode-select --print-path)
    CURRENT_DIR=$(pwd)
    export PATH="${PLATFORM_PATH}/Developer/usr/bin:${DEVELOPER}/usr/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:/opt/homebrew/bin"
    mkdir gmplib-"${PLATFORM}"-"${ARCH}"
    CFLAGS="-Wno-unused-value -fembed-bitcode -arch ${ARCH} --sysroot=${SDK} $(version_min_flag "$PLATFORM")"
    CONFIGURESCRIPT="gmp_configure_script.sh"
    # shellcheck disable=SC2039,SC2164
    pushd contrib
    # shellcheck disable=SC2039,SC2164
    pushd gmp
    make clean || true
    make distclean || true
    echo "HOST: $HOST"
    echo "PREFIX: ${CURRENT_DIR}/gmplib-${PFX}"

    cat >"$CONFIGURESCRIPT" << EOF
#!/bin/sh
./configure \
CC="$CLANG" CFLAGS="$CFLAGS" CPPFLAGS="$CFLAGS" LDFLAGS="$CFLAGS" \
--host=${HOST} --prefix="${CURRENT_DIR}/gmplib-${PFX}" \
--disable-shared --enable-static --disable-assembly -v
EOF
    
    chmod a+x "$CONFIGURESCRIPT"
    sh "$CONFIGURESCRIPT"
    rm "$CONFIGURESCRIPT"

    # shellcheck disable=SC2039
    mkdir -p "${CURRENT_DIR}/log"
    # shellcheck disable=SC2039
    make -j "$LOGICALCPU_MAX" &> "${CURRENT_DIR}"/log/gmplib-"${PFX}"-build.log
    # shellcheck disable=SC2039
    make install &> "${CURRENT_DIR}"/log/gmplib-"${PFX}"-install.log
    #make check
    #exit 1
    # shellcheck disable=SC2039,SC2164
    popd # gmp
    # shellcheck disable=SC2039,SC2164
    popd # contrib
    # shellcheck disable=SC2039,SC2164
    popd # build
}

build_relic_arch() {
    PLATFORM=$1
    ARCH=$2
    PFX=${PLATFORM}-${ARCH}

    # shellcheck disable=SC2039,SC2164
    pushd ${BUILD}

    SDK=$(xcrun --sdk "$PLATFORM" --show-sdk-path)

    BUILDDIR=relic-"${PFX}"
    TOOLCHAIN=$(pwd)/ios.toolchain.cmake
    GMP_PFX=$(pwd)/gmplib-${PFX}
    rm -rf "$BUILDDIR"
    mkdir "$BUILDDIR"
    # shellcheck disable=SC2039,SC2164
    pushd "$BUILDDIR"

    unset CC
    # shellcheck disable=SC2155
    export CC=$(xcrun --sdk "${PLATFORM}" --find clang)

    WSIZE=0
    IOS_PLATFORM=""
    OPTIMIZATIONFLAGS=""
    DEPLOYMENT_TARGET=""

    # shellcheck disable=SC2039
    # shellcheck disable=SC2053
    if [[ $PLATFORM = $IPHONEOS ]]; then
        if [[ $ARCH = "arm64" ]] || [[ $ARCH = "arm64e" ]]; then
            IOS_PLATFORM=OS64
            DEPLOYMENT_TARGET=$MIN_IOS
            WSIZE=64
            OPTIMIZATIONFLAGS=-fomit-frame-pointer
        else
            IOS_PLATFORM=OS
            WSIZE=32
        fi
    elif [[ $PLATFORM = $IPHONESIMULATOR ]]; then
        if [[ $ARCH = "x86_64" ]]; then
            IOS_PLATFORM=SIMULATOR64
            DEPLOYMENT_TARGET=$MIN_IOS
            WSIZE=64
            OPTIMIZATIONFLAGS=-fomit-frame-pointer
        elif [[ $ARCH = "arm64" ]]; then
            IOS_PLATFORM=SIMULATORARM64
            DEPLOYMENT_TARGET=$MIN_IOS
            WSIZE=64
        else
            IOS_PLATFORM=SIMULATOR
            WSIZE=32
        fi
    elif [[ $PLATFORM = $WATCHOS ]]; then
        IOS_PLATFORM=WATCHOS
        DEPLOYMENT_TARGET=$MIN_WATCHOS
        WSIZE=32
    elif [[ $PLATFORM = $WATCHSIMULATOR ]]; then
        IOS_PLATFORM=SIMULATOR_WATCHOS
        DEPLOYMENT_TARGET=$MIN_WATCHOS
        WSIZE=32
    elif [[ $PLATFORM = $TVOS ]]; then
        IOS_PLATFORM=TVOS
        DEPLOYMENT_TARGET=$MIN_TVOS
        WSIZE=64
        OPTIMIZATIONFLAGS=-fomit-frame-pointer
    elif [[ $PLATFORM = $TVSIMULATOR ]]; then
        IOS_PLATFORM=SIMULATOR_TVOS
        #TODO
        if [[ $ARCH = "arm64" ]]
        then
            IOS_PLATFORM=OS64
        fi
        DEPLOYMENT_TARGET=$MIN_TVOS
        WSIZE=64
        OPTIMIZATIONFLAGS=-fomit-frame-pointer
    elif [[ $PLATFORM = $MACOS ]]; then
        WSIZE=64
        IOS_PLATFORM=MAC
        if [[ $ARCH = "arm64" ]]
        then
            IOS_PLATFORM=MAC_ARM64
        fi
        DEPLOYMENT_TARGET=$MIN_MACOS
        OPTIMIZATIONFLAGS=-fomit-frame-pointer
    fi

    COMPILER_ARGS="$(version_min_flag "$PLATFORM") -Wno-unused-functions"

    EXTRA_ARGS="-DOPSYS=NONE -DPLATFORM=$IOS_PLATFORM -DDEPLOYMENT_TARGET=$DEPLOYMENT_TARGET -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN"

    # shellcheck disable=SC2039
    if [[ $ARCH = "i386" ]]; then
        EXTRA_ARGS+=" -DARCH=X86"
    elif [[ $ARCH = "x86_64" ]]; then
        EXTRA_ARGS+=" -DARCH=X64"
    elif [[ $ARCH = "arm64" ]]; then
       # Relic doesn't support aarch64 yet, "ARCH=ARM" is for ARM 32-bit architecture only
       EXTRA_ARGS+=" -DIOS_ARCH=arm64 -DARCH="
    elif [[ $ARCH = "armv7s" ]]; then
        EXTRA_ARGS+=" -DIOS_ARCH=armv7s -DARCH=ARM"
    elif [[ $ARCH = "armv7k" ]]; then
        EXTRA_ARGS+=" -DIOS_ARCH=armv7k -DARCH=ARM"
    elif [[ $ARCH = "arm64_32" ]]; then
        EXTRA_ARGS+=" -DIOS_ARCH=arm64_32 -DARCH=ARM"
    fi

    CURRENT_DIR=$(pwd)
    cmake -DCMAKE_PREFIX_PATH:PATH="${GMP_PFX}" -DTESTS=0 -DBENCH=0 -DBUILD_BLS_JS_BINDINGS=0 -DBUILD_BLS_PYTHON_BINDINGS=0 \
    -DBUILD_BLS_BENCHMARKS=0 -DBUILD_BLS_TESTS=0 -DCHECK=off -DARITH=gmp -DTIMER=HPROC -DFP_PRIME=381 -DMULTI=PTHREAD \
    -DFP_QNRES=on -DFP_METHD="INTEG;INTEG;INTEG;MONTY;EXGCD;SLIDE" -DFPX_METHD="INTEG;INTEG;LAZYR" -DPP_METHD="LAZYR;OATEP" \
    -DCOMP_FLAGS="-pipe -std=c99 -O3 -funroll-loops $OPTIMIZATIONFLAGS -isysroot $SDK -arch $ARCH -fembed-bitcode ${COMPILER_ARGS}" \
    -DWSIZE=$WSIZE -DVERBS=off -DSHLIB=off -DALLOC="AUTO" -DEP_PLAIN=off -DEP_SUPER=off -DPP_EXT="LAZYR" \
    -DWITH="DV;BN;MD;FP;EP;FPX;EPX;PP;PC;CP" -DBN_METHD="COMBA;COMBA;MONTY;SLIDE;STEIN;BASIC" ${EXTRA_ARGS} ../../

    make -j "$LOGICALCPU_MAX"
    # shellcheck disable=SC2039,SC2164
    popd # "$BUILDDIR"
    # shellcheck disable=SC2039,SC2164
    popd # depends/relic
}

build_bls_arch() {
    # shellcheck disable=SC2039
    BLS_FILES=( "bls" "chaincode" "elements" "extendedprivatekey" "extendedpublickey" "legacy" "privatekey" "schemes" "threshold" )
    # shellcheck disable=SC2039
    ALL_BLS_OBJ_FILES=$(printf "%s.o " "${BLS_FILES[@]}")

    PLATFORM=$1
    ARCH=$2
    PFX=${PLATFORM}-${ARCH}
    SDK=$(xcrun --sdk "$PLATFORM" --show-sdk-path)

    BUILDDIR=${BUILD}/bls-"${PFX}"
    rm -rf "$BUILDDIR"
    mkdir "$BUILDDIR"
    # shellcheck disable=SC2039,SC2164
    pushd "$BUILDDIR"

    EXTRA_ARGS="$(version_min_flag "$PLATFORM")"

    CURRENT_DIR=$(pwd)

    # shellcheck disable=SC2039
    for F in "${BLS_FILES[@]}"
    do
        clang -I"../contrib/relic/include" \
          -I"../../depends/relic/include" \
          -I"../../include/dashbls" \
          -I"../relic-${PFX}/depends/relic/include" \
          -I"../../src/" \
          -I"../gmplib-${PFX}/include" \
          -x c++ -std=c++14 -stdlib=libc++ -fembed-bitcode -arch "${ARCH}" -isysroot "${SDK}" "${EXTRA_ARGS}" \
          -c "../../src/${F}.cpp" -o "${F}.o"
    done

    # shellcheck disable=SC2086
    xcrun -sdk "$PLATFORM" ar -cvq libbls.a $ALL_BLS_OBJ_FILES

    # shellcheck disable=SC2039,SC2164
    popd # "$BUILDDIR"
}

build_all_arch() {
    PLATFORM=$1
    ARCH=$2
    build_gmp_arch "$PLATFORM" "$ARCH"
    build_relic_arch "$PLATFORM" "$ARCH"
    build_bls_arch "$PLATFORM" "$ARCH"
}

build_target() {
    BUILD_IN=$1
    echo "Build target: $BUILD_IN"
    ARCH=""
    PLATFORM=""
    # shellcheck disable=SC2039
    if [[ $BUILD_IN = "x86_64-apple-ios" ]]; then
      ARCH=x86_64
      PLATFORM=$IPHONESIMULATOR
    elif [[ $BUILD_IN = "aarch64-apple-ios" ]]; then
      ARCH=arm64
      PLATFORM=$IPHONEOS
    elif [[ $BUILD_IN = "aarch64-apple-ios-sim" ]]; then
      ARCH=arm64
      PLATFORM=$IPHONESIMULATOR
    elif [[ $BUILD_IN = "x86_64-apple-darwin" ]]; then
      ARCH=x86_64
      PLATFORM=$MACOS
    elif [[ $BUILD_IN = "aarch64-apple-darwin" ]]; then
      ARCH=arm64
      PLATFORM=$MACOS
    fi
    build_all_arch "$PLATFORM" "$ARCH"
    PFX="${PLATFORM}"-"${ARCH}"
    rm -rf "build/artefacts/${BUILD_IN}"
    mkdir -p "build/artefacts/${BUILD_IN}"
    cp "build/gmplib-${PFX}/lib/libgmp.a" "build/artefacts/${BUILD_IN}"
    cp "build/relic-${PFX}/depends/relic/lib/librelic_s.a" "build/artefacts/${BUILD_IN}"
#    cp "build/relic-${PFX}/depends/sodium/libsodium.a" "build/artefacts/${BUILD_IN}"
    cp "build/bls-${PFX}/libbls.a" "build/artefacts/${BUILD_IN}"
#    cp -rf build/bls-"${PFX}"/*.o build/artefacts/"${BUILD_IN}"/include
#    cp -rf src/*.hpp build/artefacts/"${BUILD_IN}"/include
#    cp -rf build/gmplib-"${PFX}"/include/gmp.h build/artefacts/"${BUILD_IN}"/include
#    cp -rf build/relic-"${PFX}"/_deps/relic-build/include/*.h build/artefacts/"${BUILD_IN}"/include
}

prepare
build_target "$TARGET"
