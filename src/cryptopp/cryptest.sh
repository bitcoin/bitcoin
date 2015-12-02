#!/bin/bash

# cryptest.sh - written and placed in public domain by Jeffrey Walton and Uri Blumenthal.
#               Copyright assigned to Crypto++ project.

# This is a test script that can be used on some Linux/Unix/Apple machines
# to automate building the library and running the self test with various
# combinations of flags, options, and conditions.

# Everything is tee'd into cryptest-result.txt. Change it to suite your taste. You
# should be able to use `egrep -a "(Error|error|FAILED|Illegal)" cryptest-result.txt`
# to quickly find errors and failures.

# Set to suite your taste
TEST_RESULTS=cryptest-result.txt
BENCHMARK_RESULTS=cryptest-bench.txt
WARN_TEST_RESULTS=cryptest-warn-result.txt

# Respect user's preferred flags, but filter the stuff we expliclty test
#if [ ! -z "CXXFLAGS" ]; then
#	ADD_CXXFLAGS=$(echo "$CXXFLAGS" | sed 's/\(-DDEBUG\|-DNDEBUG\|-O[0-9]\|-Os\|-Og\|-fsanitize=address\|-fsanitize=undefined\|-DDCRYPTOPP_NO_UNALIGNED_DATA_ACCESS\|-DDCRYPTOPP_NO_UNALIGNED_DATA_ACCESS\|-DDCRYPTOPP_NO_BACKWARDS_COMPATIBILITY_562\)//g')
#else\
#	ADD_CXXFLAGS=""
#fi

# I can't seem to get the expression to work in sed on Apple. It returns the original CXXFLAGS.
#   If you want to test with additional flags, then put them in ADD_CXXFLAGS below.
# ADD_CXXFLAGS="-mrdrnd -mrdseed"
ADD_CXXFLAGS=""

IS_DARWIN=$(uname -s | grep -i -c darwin)
IS_LINUX=$(uname -s | grep -i -c linux)
IS_CYGWIN=$(uname -s | grep -i -c cygwin)
IS_MINGW=$(uname -s | grep -i -c mingw)
IS_OPENBSD=$(uname -s | grep -i -c openbsd)

# We need to use the C++ compiler to determine if c++11 is available. Otherwise
#   a mis-detection occurs on Mac OS X 10.9 and above. Below, we use the same
#   Implicit Variables as make. Also see
# https://www.gnu.org/software/make/manual/html_node/Implicit-Variables.html
if [ -z "$CXX" ]; then
	if [ "$IS_DARWIN" -ne "0" ]; then
		CXX=c++
	else
		# Linux, MinGW, Cygwin and fallback ...
		CXX=g++
	fi
fi

# Fixup
if [ "$CXX" == "gcc" ]; then
	CXX=g++
fi

# Fixup
if [ "$IS_OPENBSD" -ne "0" ]; then
	MAKE=gmake
else
	MAKE=make
fi

if [ -z "$TMP" ]; then
	TMP=/tmp
fi

# Use the compiler driver, and not cpp, to tell us if the flag is consumed.
$CXX -x c++ -dM -E -std=c++11 - < /dev/null > /dev/null 2>&1
if [ "$?" -eq "0" ]; then
	HAVE_CXX11=1
else
	HAVE_CXX11=0
fi

# OpenBSD 5.7 and OS X 10.5 cannot consume -std=c++03
$CXX -x c++ -dM -E -std=c++03 - < /dev/null > /dev/null 2>&1
if [ "$?" -eq "0" ]; then
	HAVE_CXX03=1
else
	HAVE_CXX03=0
fi

# Set to 0 if you don't have UBsan
$CXX -x c++ -fsanitize=undefined adhoc.cpp.proto -o $TMP/adhoc > /dev/null 2>&1
if [ "$?" -eq "0" ]; then
	HAVE_UBSAN=1
else
	HAVE_UBSAN=0
fi

# Fixup...
if [ "$IS_CYGWIN" -ne "0" ] || [ "$IS_MINGW" -ne "0" ]; then
	HAVE_UBSAN=0
fi

# Set to 0 if you don't have Asan
$CXX -x c++ -fsanitize=undefined adhoc.cpp.proto -o $TMP/adhoc > /dev/null 2>&1
if [ "$?" -eq "0" ]; then
	HAVE_ASAN=1
else
	HAVE_ASAN=0
fi

# Fixup...
if [ "$IS_CYGWIN" -ne "0" ] || [ "$IS_MINGW" -ne "0" ]; then
	HAVE_ASAN=0
fi

#Final fixups for compilers liek GCC on ARM64
if [ "$HAVE_UBSAN" -eq "0" ] || [ "$HAVE_ASAN" -eq "0" ]; then
	HAVE_UBAN=0
	HAVE_ASAN=0
fi

# Set to 0 if you don't have Valgrind. Valgrind tests take a long time...
HAVE_VALGRIND=$(which valgrind 2>&1 | grep -v "no valgrind" | grep -i -c valgrind)

# Echo back to ensure something is not missed.
echo
echo "HAVE_CXX03: $HAVE_CXX03"
echo "HAVE_CXX11: $HAVE_CXX11"
echo "HAVE_ASAN: $HAVE_ASAN"
echo "HAVE_UBSAN: $HAVE_UBSAN"

if [ "$HAVE_VALGRIND" -ne "0" ]; then
	echo "HAVE_VALGRIND: $HAVE_VALGRIND"
fi
if [ "$IS_DARWIN" -ne "0" ]; then
	echo "IS_DARWIN: $IS_DARWIN"
	unset MallocScribble MallocPreScribble MallocGuardEdges
fi
if [ "$IS_LINUX" -ne "0" ]; then
	echo "IS_LINUX: $IS_LINUX"
fi
if [ "$IS_CYGWIN" -ne "0" ]; then
	echo "IS_CYGWIN: $IS_CYGWIN"
fi
if [ "$IS_MINGW" -ne "0" ]; then
	echo "IS_MINGW: $IS_MINGW"
fi

echo "User CXXFLAGS: $CXXFLAGS"
echo "Retained CXXFLAGS: $ADD_CXXFLAGS"
echo "Compiler:" $($CXX --version | head -1)

TEST_BEGIN=$(date)
echo
echo "Start time: $TEST_BEGIN"

############################################
############################################

# Remove previous test results
rm -f "$TEST_RESULTS" > /dev/null 2>&1
touch "$TEST_RESULTS"

rm -f "$BENCHMARK_RESULTS" > /dev/null 2>&1
touch "$BENCHMARK_RESULTS"

rm -f "$WARN_RESULTS" > /dev/null 2>&1
touch "$WARN_RESULTS"

############################################
# Basic debug build
echo
echo "************************************" | tee -a "$TEST_RESULTS"
echo "Testing: debug, default CXXFLAGS" | tee -a "$TEST_RESULTS"
echo

unset CXXFLAGS
"$MAKE" clean > /dev/null 2>&1
export CXXFLAGS="-DDEBUG -g2 -O2"
"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"

############################################
# Basic release build
echo
echo "************************************" | tee -a "$TEST_RESULTS"
echo "Testing: release, default CXXFLAGS" | tee -a "$TEST_RESULTS"
echo

unset CXXFLAGS
"$MAKE" clean > /dev/null 2>&1
export CXXFLAGS="-DNDEBUG -g2 -O2"
"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"

############################################
# Basic debug build, DISABLE_ASM
echo
echo "************************************" | tee -a "$TEST_RESULTS"
echo "Testing: debug, default CXXFLAGS, DISABLE_ASM" | tee -a "$TEST_RESULTS"
echo

unset CXXFLAGS
"$MAKE" clean > /dev/null 2>&1
export CXXFLAGS="-DDEBUG -g2 -O2 -DCRYPTOPP_DISABLE_ASM"
"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"

############################################
# Basic release build, DISABLE_ASM
echo
echo "************************************" | tee -a "$TEST_RESULTS"
echo "Testing: release, default CXXFLAGS, DISABLE_ASM" | tee -a "$TEST_RESULTS"
echo

unset CXXFLAGS
"$MAKE" clean > /dev/null 2>&1
export CXXFLAGS="-DNDEBUG -g2 -O2 -DCRYPTOPP_DISABLE_ASM"
"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"

############################################
# c++03 debug build
if [ "$HAVE_CXX03" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: debug, c++03" | tee -a "$TEST_RESULTS"
	echo

	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DDEBUG -g2 -O2 -std=c++03 $ADD_CXXFLAGS"
	"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
# c++03 release build
if [ "$HAVE_CXX03" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: release, c++03" | tee -a "$TEST_RESULTS"
	echo

	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -g2 -O2 -std=c++03 $ADD_CXXFLAGS"
	"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
# c++11 debug build
if [ "$HAVE_CXX11" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: debug, c++11" | tee -a "$TEST_RESULTS"
	echo

	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DDEBUG -g2 -O2 -std=c++11 $ADD_CXXFLAGS"
	"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
# c++11 release build
if [ "$HAVE_CXX11" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: release, c++11" | tee -a "$TEST_RESULTS"
	echo

	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -g2 -O2 -std=c++11 $ADD_CXXFLAGS"
	"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
# Debug build, all backwards compatibility.
echo
echo "************************************" | tee -a "$TEST_RESULTS"
echo "Testing: debug, MAINTAIN_BACKWARDS_COMPATIBILITY" | tee -a "$TEST_RESULTS"
echo

unset CXXFLAGS
"$MAKE" clean > /dev/null 2>&1
export CXXFLAGS="-DDEBUG -g2 -O2 -DCRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY $ADD_CXXFLAGS"
"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"

############################################
# Release build, all backwards compatibility.
echo
echo "************************************" | tee -a "$TEST_RESULTS"
echo "Testing: release, MAINTAIN_BACKWARDS_COMPATIBILITY" | tee -a "$TEST_RESULTS"
echo

unset CXXFLAGS
"$MAKE" clean > /dev/null 2>&1
export CXXFLAGS="-DNDEBUG -g2 -O2 -DCRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY $ADD_CXXFLAGS"
"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"

############################################
# Debug build, init_priority
echo
echo "************************************" | tee -a "$TEST_RESULTS"
echo "Testing: debug, INIT_PRIORITY" | tee -a "$TEST_RESULTS"
echo

unset CXXFLAGS
"$MAKE" clean > /dev/null 2>&1
export CXXFLAGS="-DDEBUG -g2 -O1 -DCRYPTOPP_INIT_PRIORITY=250 $ADD_CXXFLAGS"
"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"

############################################
# Release build, init_priority
echo
echo "************************************" | tee -a "$TEST_RESULTS"
echo "Testing: release, INIT_PRIORITY" | tee -a "$TEST_RESULTS"
echo

unset CXXFLAGS
"$MAKE" clean > /dev/null 2>&1
export CXXFLAGS="-DNDEBUG -g2 -O2 -DCRYPTOPP_INIT_PRIORITY=250 $ADD_CXXFLAGS"
"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"

############################################
# Release build, no unaligned data access
#  This test will not be needed in Crypto++ 5.7 and above
echo
echo "************************************" | tee -a "$TEST_RESULTS"
echo "Testing: release, NO_UNALIGNED_DATA_ACCESS" | tee -a "$TEST_RESULTS"
echo

unset CXXFLAGS
"$MAKE" clean > /dev/null 2>&1
export CXXFLAGS="-DNDEBUG -g2 -O2 -DCRYPTOPP_NO_UNALIGNED_DATA_ACCESS $ADD_CXXFLAGS"
"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"

############################################
# Release build, no backwards compatibility with Crypto++ 5.6.2.
#  This test will not be needed in Crypto++ 5.7 and above
echo
echo "************************************" | tee -a "$TEST_RESULTS"
echo "Testing: release, NO_BACKWARDS_COMPATIBILITY_562" | tee -a "$TEST_RESULTS"
echo

unset CXXFLAGS
"$MAKE" clean > /dev/null 2>&1
export CXXFLAGS="-DNDEBUG -g2 -O2 -DCRYPTOPP_NO_BACKWARDS_COMPATIBILITY_562 $ADD_CXXFLAGS"
"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"

############################################
# Debug build, OS Independence
echo
echo "************************************" | tee -a "$TEST_RESULTS"
echo "Testing: debug, NO_OS_DEPENDENCE" | tee -a "$TEST_RESULTS"
echo

unset CXXFLAGS
"$MAKE" clean > /dev/null 2>&1
export CXXFLAGS="-DDEBUG -g2 -O1 -DNO_OS_DEPENDENCE $ADD_CXXFLAGS"
"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"

############################################
# Release build, OS Independence
echo
echo "************************************" | tee -a "$TEST_RESULTS"
echo "Testing: release, NO_OS_DEPENDENCE" | tee -a "$TEST_RESULTS"
echo

unset CXXFLAGS
"$MAKE" clean > /dev/null 2>&1
export CXXFLAGS="-DNDEBUG -g2 -O2 -DNO_OS_DEPENDENCE $ADD_CXXFLAGS"
"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"

############################################
# Debug build at -O3
echo
echo "************************************" | tee -a "$TEST_RESULTS"
echo "Testing: debug, -O3 optimizations" | tee -a "$TEST_RESULTS"
echo

unset CXXFLAGS
"$MAKE" clean > /dev/null 2>&1
export CXXFLAGS="-DDEBUG -g2 -O3 $ADD_CXXFLAGS"
"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"

############################################
# Release build at -O3
echo
echo "************************************" | tee -a "$TEST_RESULTS"
echo "Testing: release, -O3 optimizations" | tee -a "$TEST_RESULTS"
echo

unset CXXFLAGS
"$MAKE" clean > /dev/null 2>&1
export CXXFLAGS="-DNDEBUG -g2 -O3 $ADD_CXXFLAGS"
"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"

############################################
# Debug build at -Os
echo
echo "************************************" | tee -a "$TEST_RESULTS"
echo "Testing: debug, -Os optimizations" | tee -a "$TEST_RESULTS"
echo

unset CXXFLAGS
"$MAKE" clean > /dev/null 2>&1
export CXXFLAGS="-DDEBUG -g2 -Os $ADD_CXXFLAGS"
"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"

############################################
# Release build at -Os
echo
echo "************************************" | tee -a "$TEST_RESULTS"
echo "Testing: release, -Os optimizations" | tee -a "$TEST_RESULTS"
echo

unset CXXFLAGS
"$MAKE" clean > /dev/null 2>&1
export CXXFLAGS="-DNDEBUG -g2 -Os $ADD_CXXFLAGS"
"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"

############################################
# Debug build, UBSan, c++03
if [ "$HAVE_CXX03" -ne "0" ] && [ "$HAVE_UBSAN" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: debug, c++03, UBsan" | tee -a "$TEST_RESULTS"
	echo
	
	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DDEBUG -g2 -O1 -std=c++03 $ADD_CXXFLAGS"
	"$MAKE" ubsan | tee -a "$TEST_RESULTS"
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
# Release build, UBSan, c++03
if [ "$HAVE_CXX03" -ne "0" ] && [ "$HAVE_UBSAN" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: release, c++03, UBsan" | tee -a "$TEST_RESULTS"
	echo
	
	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -g2 -O2 -std=c++03 $ADD_CXXFLAGS"
	"$MAKE" ubsan | tee -a "$TEST_RESULTS"
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
# Debug build, Asan, c++03
if [ "$HAVE_CXX03" -ne "0" ] && [ "$HAVE_ASAN" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: debug, c++03, Asan" | tee -a "$TEST_RESULTS"
	echo
	
	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DDEBUG -g2 -O1 -std=c++03 $ADD_CXXFLAGS"
	"$MAKE" asan | tee -a "$TEST_RESULTS"
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
# Release build, Asan, c++03
if [ "$HAVE_CXX03" -ne "0" ] && [ "$HAVE_ASAN" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: release, c++03, Asan" | tee -a "$TEST_RESULTS"
	echo
	
	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -g2 -O2 -std=c++03 $ADD_CXXFLAGS"
	"$MAKE" asan | tee -a "$TEST_RESULTS"
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
# Release build, UBSan, c++11
if [ "$HAVE_CXX11" -ne "0" ] && [ "$HAVE_UBSAN" -ne "0" ]; then
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: c++11, UBsan" | tee -a "$TEST_RESULTS"
	echo
	
	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -g2 -O2 -std=c++11 $ADD_CXXFLAGS"
	"$MAKE" ubsan | tee -a "$TEST_RESULTS"
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
# Release build, Asan, c++11
if [ "$HAVE_CXX11" -ne "0" ] && [ "$HAVE_ASAN" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: c++11, Asan" | tee -a "$TEST_RESULTS"
	echo
	
	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -g2 -O2 -std=c++11 $ADD_CXXFLAGS"
	"$MAKE" asan | tee -a "$TEST_RESULTS"
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

# For Darwin, we need to test both -stdlib=libstdc++ (GNU) and
#  -stdlib=libc++ (LLVM) crossed with -std=c++03 and -std=c++11.

############################################
# Darwin, c++03, libc++
if [ "$HAVE_CXX03" -ne "0" ] && [ "$IS_DARWIN" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: Darwin, c++03, libc++ (LLVM)" | tee -a "$TEST_RESULTS"
	echo

	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -g2 -O2 -std=c++03 -stdlib=libc++ $ADD_CXXFLAGS"
	"$MAKE" static cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
# Darwin, c++03, libstdc++
if [ "$HAVE_CXX03" -ne "0" ] && [ "$IS_DARWIN" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: Darwin, c++03, libstdc++ (GNU)" | tee -a "$TEST_RESULTS"
	echo

	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -g2 -O2 -std=c++03 -stdlib=libstdc++ $ADD_CXXFLAGS"
	"$MAKE" static cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
# Darwin, c++11, libc++
if [ "$IS_DARWIN" -ne "0" ] && [ "$HAVE_CXX11" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: Darwin, c++11, libc++ (LLVM)" | tee -a "$TEST_RESULTS"
	echo

	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -g2 -O2 -std=c++11 -stdlib=libc++ $ADD_CXXFLAGS"
	"$MAKE" static cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
# Darwin, c++11, libstdc++
if [ "$IS_DARWIN" -ne "0" ] && [ "$HAVE_CXX11" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: Darwin, c++11, libstdc++ (GNU)" | tee -a "$TEST_RESULTS"
	echo

	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -g2 -O2 -std=c++11 -stdlib=libstdc++ $ADD_CXXFLAGS"
	"$MAKE" static cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
# Darwin, c++03, Malloc Guards
if [ "$IS_DARWIN" -ne "0" ] && [ "$HAVE_CXX03" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: Darwin, c++03, Malloc Guards" | tee -a "$TEST_RESULTS"
	echo

	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -g2 -O2 -std=c++03 $ADD_CXXFLAGS"
	"$MAKE" static cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
	
	export MallocScribble=1
	export MallocPreScribble=1
	export MallocGuardEdges=1
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
	unset MallocScribble MallocPreScribble MallocGuardEdges
fi

############################################
# Darwin, c++11, Malloc Guards
if [ "$IS_DARWIN" -ne "0" ] && [ "$HAVE_CXX11" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: Darwin, c++11, Malloc Guards" | tee -a "$TEST_RESULTS"
	echo

	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -g2 -O2 -std=c++11 $ADD_CXXFLAGS"
	"$MAKE" static cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
	
	export MallocScribble=1
	export MallocPreScribble=1
	export MallocGuardEdges=1
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
	unset MallocScribble MallocPreScribble MallocGuardEdges
fi

# Try to locate a Xcode compiler for testing under Darwin
XCODE_COMPILER=$(find /Applications/Xcode*.app/Contents/Developer -name clang++ | head -1)

############################################
# Xcode compiler
if [ "$IS_DARWIN" -ne "0" ] && [ -z "$XCODE_COMPILER" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: Xcode Clang compiler" | tee -a "$TEST_RESULTS"
	echo

	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	expot CXX="$XCODE_COMPILER"
	export CXXFLAGS="-DNDEBUG -g2 -O2 -std=c++11 $ADD_CXXFLAGS"
	"$MAKE" static cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
# Benchmarks, c++03
if [ "$HAVE_CXX03" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: Benchmarks, c++03" | tee -a "$TEST_RESULTS"
	echo

	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -O3 -std=c++03 $ADD_CXXFLAGS"
	"$MAKE" static cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe b 3 2.4+1e9 2>&1 | tee -a "$BENCHMARK_RESULTS"
fi

############################################
# Benchmarks, c++11
if [ "$HAVE_CXX11" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: Benchmarks, c++11" | tee -a "$TEST_RESULTS"
	echo

	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -O3 -std=c++11 $ADD_CXXFLAGS"
	"$MAKE" static cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe b 3 2.4+1e9 2>&1 | tee -a "$BENCHMARK_RESULTS"
fi

# For Cygwin, we need to test both PREFER_BERKELEY_STYLE_SOCKETS
#   and PREFER_WINDOWS_STYLE_SOCKETS

############################################
# MinGW and PREFER_BERKELEY_STYLE_SOCKETS
if [ "$IS_MINGW" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: MinGW, PREFER_BERKELEY_STYLE_SOCKETS" | tee -a "$TEST_RESULTS"
	echo

	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -g2 -O2 -DPREFER_BERKELEY_STYLE_SOCKETS -DNO_WINDOWS_STYLE_SOCKETS $ADD_CXXFLAGS"
	"$MAKE" static cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
# MinGW and PREFER_WINDOWS_STYLE_SOCKETS
if [ "$IS_MINGW" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: MinGW, PREFER_WINDOWS_STYLE_SOCKETS" | tee -a "$TEST_RESULTS"
	echo

	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -g2 -O2 -DPREFER_WINDOWS_STYLE_SOCKETS -DNO_BERKELEY_STYLE_SOCKETS $ADD_CXXFLAGS"
	"$MAKE" static cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
# Valgrind, c++03. Requires -O1 for accurate results
if [ "$HAVE_CXX03" -ne "0" ] && [ "$HAVE_VALGRIND" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: Valgrind, c++03" | tee -a "$TEST_RESULTS"
	echo
	
	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -std=c++03 -g3 -O1 $ADD_CXXFLAGS"
	"$MAKE" static cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
	valgrind --track-origins=yes ./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	valgrind --track-origins=yes ./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
# Valgrind, c++11. Requires -O1 for accurate results
if [ "$HAVE_VALGRIND" -ne "0" ] && [ "$HAVE_CXX11" -ne "0" ]; then
	echo
	echo "************************************" | tee -a "$TEST_RESULTS"
	echo "Testing: Valgrind, c++11" | tee -a "$TEST_RESULTS"
	echo
	
	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -std=c++11 -g3 -O1 $ADD_CXXFLAGS"
	"$MAKE" static cryptest.exe 2>&1 | tee -a "$TEST_RESULTS"
	valgrind --track-origins=yes ./cryptest.exe v 2>&1 | tee -a "$TEST_RESULTS"
	valgrind --track-origins=yes ./cryptest.exe tv all 2>&1 | tee -a "$TEST_RESULTS"
fi

############################################
############################################

if [ "$CXX" == "g++" ] && [ "$HAVE_CXX11" -ne "0" ]; then

	############################################
	# Basic debug build
	echo
	echo "************************************" | tee -a "$WARN_TEST_RESULTS"
	echo "Testing: debug, c++11, elevated warnings" | tee -a "$WARN_TEST_RESULTS"
	echo

	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DDEBUG -g2 -O2 -std=c++11 -DCRYPTOPP_NO_BACKWARDS_COMPATIBILITY_562 -Wall -Wextra -Wno-unknown-pragmas"
	"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$WARN_TEST_RESULTS"

	############################################
	# Basic release build
	echo
	echo "************************************" | tee -a "$WARN_TEST_RESULTS"
	echo "Testing: release, c++11, elevated warnings" | tee -a "$WARN_TEST_RESULTS"
	echo

	unset CXXFLAGS
	"$MAKE" clean > /dev/null 2>&1
	export CXXFLAGS="-DNDEBUG -g2 -O2 -std=c++11 -DCRYPTOPP_NO_BACKWARDS_COMPATIBILITY_562 -Wall -Wextra -Wno-unknown-pragmas"
	"$MAKE" static dynamic cryptest.exe 2>&1 | tee -a "$WARN_TEST_RESULTS"
fi

############################################
############################################

TEST_END=$(date)

echo "************************************************" | tee -a "$TEST_RESULTS"
echo "************************************************" | tee -a "$TEST_RESULTS"
echo | tee -a "$TEST_RESULTS"

echo "Testing started: $TEST_BEGIN" | tee -a "$TEST_RESULTS"
echo "Testing finished: $TEST_END" | tee -a "$TEST_RESULTS"
echo | tee -a "$TEST_RESULTS"

COUNT=$(grep -a "Testing: " cryptest-result.txt | wc -l)
if [ "$COUNT" -eq "0" ]; then
	echo "No configurations tested" | tee -a "$TEST_RESULTS"
else
	echo "$COUNT configurations tested" | tee -a "$TEST_RESULTS"
fi
echo | tee -a "$TEST_RESULTS"

# "FAILED" is from Crypto++
# "Error" is from the GNU assembler
# "error" is from the sanitizers
# "Illegal", "0 errors" and "suppressed errors" are from Valgrind.
COUNT=$(egrep -a '(Error|error|FAILED|Illegal)' cryptest-result.txt | egrep -v "( 0 errors|suppressed errors|memory error detector)" | wc -l)
if [ "$COUNT" -eq "0" ]; then
	echo "No failures detected" | tee -a "$TEST_RESULTS"
else
	echo "$COUNT errors detected" | tee -a "$TEST_RESULTS"
	echo
	egrep -an "(Error|error|FAILED|Illegal)" cryptest-result.txt
fi
echo | tee -a "$TEST_RESULTS"
	
echo "************************************************" | tee -a "$TEST_RESULTS"
echo "************************************************" | tee -a "$TEST_RESULTS"
