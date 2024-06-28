#!/bin/sh
set -x

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
        GMP_VERSION="6.2.1"
        CURRENT_DIR=$(pwd)
        echo "$CURRENT_DIR"
        # shellcheck disable=SC2039,SC2164
        pushd ${BUILD}
        mkdir -p "contrib"
        if [ ! -s "contrib/gmp-${GMP_VERSION}.tar.bz2" ]; then
            curl -L -o "contrib/gmp-${GMP_VERSION}.tar.bz2" https://gmplib.org/download/gmp/gmp-${GMP_VERSION}.tar.bz2
        fi
        rm -rf "contrib/gmp"
        # shellcheck disable=SC2039,SC2164
        pushd contrib
        tar xfj "gmp-${GMP_VERSION}.tar.bz2"
        mv gmp-${GMP_VERSION} gmp
        rm gmp/compat.c && cp ../../contrib/gmp-patch-6.2.1/compat.c gmp/compat.c
        rm gmp/longlong.h && cp ../../contrib/gmp-patch-6.2.1/longlong.h gmp/longlong.h
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
        mkdir -p "${CURRENT_DIR}/${BUILD}/contrib"
        if [ ! -s "${CURRENT_DIR}/${BUILD}/contrib/relic" ]; then
            # shellcheck disable=SC2039,SC2164
            pushd "${CURRENT_DIR}/${BUILD}/contrib"
            git clone --depth 1 --branch "feat/ios-support" https://github.com/pankcuf/relic
            # shellcheck disable=SC2039,SC2164
            pushd relic
            git fetch --depth 1 origin 19fb6d79a77ade4ae8cd70d2b0ef7aab8720d1ae
            git checkout 19fb6d79a77ade4ae8cd70d2b0ef7aab8720d1ae
            # shellcheck disable=SC2039,SC2164
            popd #relic
            # shellcheck disable=SC2039,SC2164
            popd #contrib
        fi
    }
    rm -rf ${BUILD}
    mkdir -p ${BUILD}
    download_relic
    download_gmp
    download_cmake_toolchain
    mkdir -p ${BUILD}/artefacts/include
}

build_gmp_arch() {
    PLATFORM=$1
    ARCH=$2
    PFX=${PLATFORM}-${ARCH}
    # why this works with this host only?
    HOST=arm-apple-darwin
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
    else
        EXTRA_ARGS+=" -DARCH=ARM"
        if [[ $ARCH = "armv7s" ]]; then
            EXTRA_ARGS+=" -DIOS_ARCH=armv7s"
        elif [[ $ARCH = "armv7k" ]]; then
            EXTRA_ARGS+=" -DIOS_ARCH=armv7k"
        elif [[ $ARCH = "arm64_32" ]]; then
            EXTRA_ARGS+=" -DIOS_ARCH=arm64_32"
        fi
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
    popd # contrib/relic
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
          -I"../relic-${PFX}/_deps/relic-build/include" \
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

build_all() {
    BUILD_IN=$1
    TARGET_DIR=build/artefacts
    # shellcheck disable=SC2039
    IFS='|' read -ra BUILD_PAIRS <<< "$BUILD_IN"
    # shellcheck disable=SC2039
    for BUILD_PAIR in "${BUILD_PAIRS[@]}"
    do
        # shellcheck disable=SC2039
        IFS=';' read -ra PARSED_PAIR <<< "$BUILD_PAIR"
        # shellcheck disable=SC2039
        PLATFORM=${PARSED_PAIR[0]}
        # shellcheck disable=SC2039
        ARCH=${PARSED_PAIR[1]}

        GMP_LIPOARGS=""
        RELIC_LIPOARGS=""
        BLS_LIPOARGS=""

        # shellcheck disable=SC2039
        local NEED_LIPO=0
        # shellcheck disable=SC2039
        IFS='+' read -ra ARCHS <<< "$ARCH"
        # shellcheck disable=SC2039
        for i in "${!ARCHS[@]}"
        do
            # shellcheck disable=SC2039
            local SINGLEARCH=${ARCHS[i]}

            # build for every platform+arch
            build_all_arch "$PLATFORM" "$SINGLEARCH"

            PFX="${PLATFORM}"-"${SINGLEARCH}"
            ARCH_TARGET_DIR=${TARGET_DIR}/${PFX}
            rm -rf "${ARCH_TARGET_DIR}"
            mkdir -p "${ARCH_TARGET_DIR}"
            #mv "${BUILD}/gmplib-${PFX}/lib/libgmp.a" "${ARCH_TARGET_DIR}/libgmp.a"
            #mv "${BUILD}/relic-${PFX}/_deps/relic-build/lib/librelic_s.a" "${ARCH_TARGET_DIR}/librelic.a"
            #mv "${BUILD}/bls-${PFX}/libbls.a" "${ARCH_TARGET_DIR}/libbls.a"

            libtool -static -o "${ARCH_TARGET_DIR}/libbls.a" \
              "${BUILD}/gmplib-${PFX}/lib/libgmp.a" \
              "${BUILD}/relic-${PFX}/_deps/relic-build/lib/librelic_s.a" \
              "${BUILD}/bls-${PFX}/libbls.a"

            # shellcheck disable=SC2039
            GMP_LIPOARGS+="${ARCH_TARGET_DIR}/libgmp.a "
            # shellcheck disable=SC2039
            RELIC_LIPOARGS+="${ARCH_TARGET_DIR}/librelic.a "
            # shellcheck disable=SC2039
            BLS_LIPOARGS+="${ARCH_TARGET_DIR}/libbls.a "

            NEED_LIPO=i
        done

        # Do lipo if we need https://developer.apple.com/forums/thread/666335?answerId=645963022#645963022
#        if [[ $NEED_LIPO -gt 0 ]]
#        then
#            FAT_TARGET_DIR=${TARGET_DIR}/${PLATFORM}-fat
#            rm -rf "${FAT_TARGET_DIR}"
#            mkdir -p "${FAT_TARGET_DIR}"
#            # shellcheck disable=SC2086
#            xcrun lipo $GMP_LIPOARGS -create -output "${FAT_TARGET_DIR}/libgmp.a"
#            # shellcheck disable=SC2086
#            xcrun lipo $RELIC_LIPOARGS -create -output "${FAT_TARGET_DIR}/librelic.a"
#            # shellcheck disable=SC2086
#            xcrun lipo $BLS_LIPOARGS -create -output "${FAT_TARGET_DIR}/libbls.a"
#            libtool -static -o "${FAT_TARGET_DIR}/libbls_combined.a" "${FAT_TARGET_DIR}/libgmp.a" "${FAT_TARGET_DIR}/librelic.a" "${FAT_TARGET_DIR}/libbls.a"
#            rm "${FAT_TARGET_DIR}/libgmp.a"
#            rm "${FAT_TARGET_DIR}/librelic.a"
#            rm "${FAT_TARGET_DIR}/libbls.a"
#            mv "${FAT_TARGET_DIR}/libbls_combined.a" "${FAT_TARGET_DIR}/libbls.a"
#            # clean up
#            # shellcheck disable=SC2039
#            for i in "${!ARCHS[@]}"
#            do
#                local SINGLEARCH=${ARCHS[i]}
#                rm -rf "${TARGET_DIR}-${SINGLEARCH}"
#            done
#        fi
    done
}

#make_relic_headers_universal() {
#    RELIC_TARGET_DIR=relic-iphoneos-arm64
#    perl -p -e 's/#define WSIZE.*/#ifdef __LP64__\n#define WSIZE 64\n#else\n#define WSIZE 32\n#endif/' \
#    "build/contrib/relic/${RELIC_TARGET_DIR}/include/relic_conf.h" \
#    > "build/contrib/relic/${RELIC_TARGET_DIR}/include/relic_conf.h.new"
#
#    rm "build/contrib/relic/${RELIC_TARGET_DIR}/include/relic_conf.h"
#    mv "build/contrib/relic/${RELIC_TARGET_DIR}/include/relic_conf.h.new" "build/contrib/relic/${RELIC_TARGET_DIR}/include/relic_conf.h"
#}

#copy_headers() {
#    mkdir build/artefacts/include
#    # Copy all headers we will need
#    cp -rf src/*.hpp build/artefacts/include
#    cp -rf build/gmp/include/gmp.h build/artefacts/include
#    cp -rf build/contrib/relic/include/*.h build/artefacts/include
#    cp -rf build/contrib/relic/include/low/*.h build/artefacts/include
#    cp -rf build/contrib/relic/relic-iphoneos-arm64/include/*.h build/artefacts/include
#    rm -rf build/artefacts/include/test-utils.hpp
#}

#function make_fat_binary()
#{
#    pushd artefacts
#
#    XCFRAMEWORK_ARGS=""
#
#    for dir in */; do
#        if [ -d "$dir" ]; then
#            if [[ "$dir" != "include/" ]]; then
#                libtool -static -o "${dir}libbls_combined.a" "${dir}libgmp.a" "${dir}librelic.a" "${dir}libbls.a"
#
#                XCFRAMEWORK_ARGS+="-library ${dir}libbls_combined.a -headers include "
#            fi
#        fi
#    done
#
#    #xcodebuild -create-xcframework $XCFRAMEWORK_ARGS -output "libbls.xcframework"
#}

prepare
build_all "${MACOS};x86_64+arm64"
build_all "${IPHONEOS};arm64|${IPHONESIMULATOR};arm64+x86_64"

#make_relic_headers_universal
#copy_headers
#make_fat_binary
