#!/bin/sh

set -e
set -x

if [ "$HOST" = "i686-linux-gnu" ]
then
    export CC="$CC -m32"
fi
if [ "$TRAVIS_OS_NAME" = "osx" ] && [ "$TRAVIS_COMPILER" = "gcc" ]
then
    export CC="gcc-9"
fi

./configure \
    --enable-experimental="$EXPERIMENTAL" \
    --with-test-override-wide-multiply="$WIDEMUL" --with-bignum="$BIGNUM" --with-asm="$ASM" \
    --enable-ecmult-static-precomputation="$STATICPRECOMPUTATION" --with-ecmult-gen-precision="$ECMULTGENPRECISION" \
    --enable-module-ecdh="$ECDH" --enable-module-recovery="$RECOVERY" \
    --enable-module-schnorrsig="$SCHNORRSIG" \
    --with-valgrind="$WITH_VALGRIND" \
    --host="$HOST" $EXTRAFLAGS

if [ -n "$BUILD" ]
then
    make -j2 "$BUILD"
fi
if [ "$RUN_VALGRIND" = "yes" ]
then
    make -j2
    # the `--error-exitcode` is required to make the test fail if valgrind found errors, otherwise it'll return 0 (http://valgrind.org/docs/manual/manual-core.html)
    valgrind --error-exitcode=42 ./tests 16
    valgrind --error-exitcode=42 ./exhaustive_tests
fi
if [ "$BENCH" = "yes" ]
then
    if [ "$RUN_VALGRIND" = "yes" ]
    then
        # Using the local `libtool` because on macOS the system's libtool has nothing to do with GNU libtool
        EXEC='./libtool --mode=execute valgrind --error-exitcode=42'
    else
        EXEC=
    fi
    # This limits the iterations in the benchmarks below to ITER(set in .travis.yml) iterations.
    export SECP256K1_BENCH_ITERS="$ITERS"
    {
        $EXEC ./bench_ecmult
        $EXEC ./bench_internal
        $EXEC ./bench_sign
        $EXEC ./bench_verify
    } >> bench.log 2>&1
    if [ "$RECOVERY" = "yes" ]
    then
        $EXEC ./bench_recover >> bench.log 2>&1
    fi
    if [ "$ECDH" = "yes" ]
    then
        $EXEC ./bench_ecdh >> bench.log 2>&1
    fi
    if [ "$SCHNORRSIG" = "yes" ]
    then
        $EXEC ./bench_schnorrsig >> bench.log 2>&1
    fi
fi
if [ "$CTIMETEST" = "yes" ]
then
    ./libtool --mode=execute valgrind --error-exitcode=42 ./valgrind_ctime_test > valgrind_ctime_test.log 2>&1
fi
