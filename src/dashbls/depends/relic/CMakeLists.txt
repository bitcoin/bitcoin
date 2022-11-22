cmake_minimum_required(VERSION 3.1)
if(NOT ${CMAKE_VERSION} VERSION_LESS "3.1")
	cmake_policy(SET CMP0054 NEW)
endif()

project(RELIC C CXX)
set(PROJECT_VERSION_MAJOR "0")
set(PROJECT_VERSION_MINOR "5")
set(PROJECT_VERSION_PATCH "0")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(VERSION ${PROJECT_VERSION})

set(INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include/low ${CMAKE_CURRENT_SOURCE_DIR}/src/tmpl)
include_directories(${INCLUDE})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

set(WFLAGS "-Wall")

message("\n-- Configuring ${PROJECT_NAME} ${PROJECT_VERSION}...\n")

message(STATUS "Available switches (default = CHECK, VERBS, DOCUM):\n")

message("   DEBUG=[off|on] Build with debugging support.")
message("   PROFL=[off|on] Build with profiling support.")
message("   CHECK=[off|on] Build with error-checking support.")
message("   VERBS=[off|on] Build with detailed error messages.")
message("   OVERH=[off|on] Build with overhead estimation.")
message("   DOCUM=[off|on] Build documentation.")
message("   STRIP=[off|on] Build only selected algorithms.")
message("   QUIET=[off|on] Build with printing disabled.")
message("   COLOR=[off|on] Build with colored output.")
message("   BIGED=[off|on] Build with big-endian support.")
message("   SHLIB=[off|on] Build shared library.")
message("   STLIB=[off|on] Build static library.")
message("   STBIN=[off|on] Build static binaries.")
message("   AMALG=[off|on] Build amalgamation for better performance.")
message("   AUSAN=[off|on] Build with ASan and UBSan (gcc/clang only).\n")

option(DEBUG "Build with debugging support" off)
option(PROFL "Build with profiling support" off)
option(CHECK "Build with error-checking support" on)
option(VERBS "Build with detailed error messages" on)
option(OVERH "Build with overhead estimation" off)
option(DOCUM "Build documentation" on)
option(STRIP "Build only the selected algorithms" off)
option(QUIET "Build with printing disabled" off)
option(COLOR "Build with colored output" on)
option(BIGED "Build with big-endian support" off)
option(SHLIB "Build shared library" on)
option(STLIB "Build static library" on)
option(STBIN "Build static binaries" off)
option(AMALG "Build amalgamation" off)
option(AUSAN "Build with ASan and UBSan (gcc/clang only)" off)

message(STATUS "Number of times each test or benchmark is ran (default = 50, 1000):\n")

message("   TESTS=n        If n > 0, build automated tests and run them n times.")
message("   BENCH=n        If n > 0, build automated benchmarks and run them n * n times.\n")

message(STATUS "Number of available processor cores (default = 1):\n")

message("   CORES=n        If n > 1, enable multithreading support.\n")

message(STATUS "Available modules (default = ALL)\n")

message("   WITH=BN       Multiple precision arithmetic.")
message("   WITH=DV       Temporary double-precision digit vectors.")
message("   WITH=FP       Prime field arithmetic.")
message("   WITH=FPX      Prime extension field arithmetic.")
message("   WITH=FB       Binary field arithmetic.")
message("   WITH=EP       Elliptic curves over prime fields.")
message("   WITH=EPX      Elliptic curves over quadratic extensions of prime fields.")
message("   WITH=EB       Elliptic curves over binary fields.")
message("   WITH=ED       Elliptic Edwards curves over prime fields.")
message("   WTTH=EC       Elliptic curve cryptography.")
message("   WITH=PB       Pairings over binary elliptic curves.")
message("   WITH=PP       Pairings over prime elliptic curves.")
message("   WTTH=PC       Pairing-based cryptography.")
message("   WITH=BC       Block ciphers (symmetric encryption).")
message("   WITH=MD       Message digests (hash functions).")
message("   WITH=CP       Cryptographic protocols.")
message("   WITH=MPC      Multi-party computation primitives.")
message("   WITH=ALL      All of the above.")
message("   Note: the programmer is responsible for not using unselected modules.\n")

message(STATUS "Available arithmetic backends (default = easy):\n")

message("   ARITH=easy     Easy-to-understand and portable, but slow backend.")
message("   ARITH=fiat     Backend based on code generated from Fiat-Crypto.")
message("   ARITH=gmp      Backend based on GNU Multiple Precision library.\n")
message("   ARITH=gmp-sec  Same as above, but using constant-time code.\n")

message(STATUS "Available memory-allocation policies (default = AUTO):\n")

message("   ALLOC=AUTO     All memory is automatically allocated.")
message("   ALLOC=DYNAMIC  All memory is allocated dynamically on demand.\n")

message(STATUS "Supported operating systems (default = LINUX):\n")

message("   OPSYS=         Undefined/No specific operating system.")
message("   OPSYS=LINUX    GNU/Linux operating system.")
message("   OPSYS=FREEBSD  FreeBSD operating system.")
message("   OPSYS=NETBSD   NetBSD operating system.")
message("   OPSYS=MACOSX   Mac OS X operating system.")
message("   OPSYS=WINDOWS  Windows operating system.")
message("   OPSYS=DROID    Android operating system.")
message("   OPSYS=DUINO    Arduino platform.\n")

message(STATUS "Supported multithreading APIs (default = UNDEF):\n")

message("   MULTI=         No multithreading support.")
message("   MULTI=OPENMP   Open Multi-Processing.")
message("   MULTI=PTHREAD  POSIX threads.\n")

message(STATUS "Supported timers (default = HREAL):\n")

message("   TIMER=         No timer.")
message("   TIMER=HREAL    GNU/Linux realtime high-resolution timer.")
message("   TIMER=HPROC    GNU/Linux per-process high-resolution timer.")
message("   TIMER=HTHRD    GNU/Linux per-thread high-resolution timer.")
message("   TIMER=ANSI     ANSI-compatible timer.")
message("   TIMER=POSIX    POSIX-compatible timer.")
message("   TIMER=CYCLE    Cycle-counting timer. (architecture-dependant)")
message("   TIMER=PERF     GNU/Linux performance monitoring framework.\n")

message(STATUS "Prefix to identify this build of the library (default = \"\"):\n")

message("   LABEL=relic\n")

include(cmake/arch.cmake)
include(cmake/err.cmake)
include(cmake/bn.cmake)
include(cmake/fp.cmake)
include(cmake/fpx.cmake)
include(cmake/fb.cmake)
include(cmake/ep.cmake)
include(cmake/eb.cmake)
include(cmake/ed.cmake)
include(cmake/ec.cmake)
include(cmake/pp.cmake)
include(cmake/md.cmake)
include(cmake/cp.cmake)
include(cmake/rand.cmake)
include(cmake/with.cmake)

# Number of tests and benchmarks
set(BENCH "100" CACHE STRING "Number of times each benchmark is ran.")
set(TESTS "100" CACHE STRING "Number of times each test is ran.")

# Number of cores.
set(CORES "1" CACHE STRING "Number of available processor cores.")

# Choose the arithmetic backend.
set(ARITH "easy" CACHE STRING "Arithmetic backend")

# Choose the memory-allocation policy.
set(ALLOC "AUTO" CACHE STRING "Allocation policy")

# Simulator of the target platform.
set(SIMUL "$ENV{SIMUL}" CACHE STRING "Path to call a simulator of the target platform.")
set(SIMAR "$ENV{SIMAR}" CACHE STRING "Arguments to call a simulator of the target platform.")
string(REPLACE " " ";" SIMAR "${SIMAR}")

# Linker flags.
string(TOLOWER ${ARITH} LFLAGS)
set(LFLAGS "-L${CMAKE_CURRENT_SOURCE_DIR}/src/low/${LFLAGS}/")
set(LFLAGS "${LFLAGS} $ENV{LDFLAGS}")

if(STBIN)
	set(LFLAGS "${LFLAGS} -static")
	set(CMAKE_LINK_SEARCH_START_STATIC ON)
	set(CMAKE_LINK_SEARCH_END_STATIC ON)
endif()

# Discover the operating system.
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
	set(OPSYS "LINUX" CACHE STRING "Operating system")
else()
	if(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
		set(OPSYS "FREEBSD" CACHE STRING "Operating system")
	else()
		if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
			set(OPSYS "MACOSX" CACHE STRING "Operating system")
		else()
			if(CMAKE_SYSTEM_NAME STREQUAL NetBSD)
				set(OPSYS "NETBSD" CACHE STRING "Operating system")
			else()
				if(CMAKE_SYSTEM_NAME STREQUAL Windows)
					set(OPSYS "WINDOWS" CACHE STRING "Operating system")
				endif()
			endif()
		endif()
	endif()
endif()
if(OPSYS STREQUAL LINUX)
	add_definitions(-D_GNU_SOURCE)
endif()
message(STATUS "Configured operating system: ${OPSYS}")

if(OPSYS STREQUAL LINUX)
	set(TIMER "HREAL" CACHE STRING "Timer")
else()
	set(TIMER "ANSI" CACHE STRING "Timer")
endif()

if(MSVC)
	#defaults to the DLL runtime library.
	set(RUNTIME "MD" CACHE STRING "MSVC runtime libraries")
	message(STATUS "Supported MSVC runtime libraries (default = MD):\n")
	message("   RUNTIME=MD     DLL runtime library (/MD,/MDd).")
	message("   RUNTIME=MT     Static runtime library (/MT,/MTd).\n")

	#loop over the compile flags to swap /MD and /MT.
	set(CompilerFlags
		CMAKE_C_FLAGS_DEBUG
		CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_RELWITHDEBINFO
		CMAKE_C_FLAGS_MINSIZEREL)
	foreach(CompilerFlag ${CompilerFlags})
		if(RUNTIME STREQUAL MT)
			string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
		else()
			string(REPLACE "/MT" "/MD" ${CompilerFlag} "${${CompilerFlag}}")
		endif()
	endforeach()
endif()

if("$ENV{CFLAGS}" STREQUAL "")
	# If the user did not specify compile flags, we use sane defaults.
	if(DEBUG)
		if (MSVC)
			set(CFLAGS "/Od" CACHE STRING "Default compiler flags.")
		else()
			# Turn off all optimizations.
			set(CFLAGS "-pipe -std=c99 -O0 -fno-omit-frame-pointer" CACHE STRING "Default compiler flags.")
			set(DFLAGS "-ggdb")
		endif()
	else()
		if (MSVC)
			set(CFLAGS "/O2" CACHE STRING "Default compiler flags.")
		else()
			set(CFLAGS "-pipe -std=c99 -O2 -funroll-loops -fomit-frame-pointer" CACHE STRING "Default compiler flags.")
		endif()
	endif()
else()
	# Turn on debugging symbols with custom flags.
	if(DEBUG)
		if (MSVC)
			set(DFLAGS "/Z7")
		else ()
			set(DFLAGS "-ggdb")
		endif()
	endif()
	set(CFLAGS "$ENV{CFLAGS}" CACHE STRING "User-chosen compiler flags.")
endif()

if(AUSAN)
	set(DFLAGS "${DFLAGS} -ggdb -fsanitize=address -fsanitize=undefined")
endif()

if(MULTI STREQUAL OPENMP)
	if (MSVC)
		set(CFLAGS "${CFLAGS} /openmp")
	else ()
		find_package(OpenMP REQUIRED)
		set(CFLAGS "${CFLAGS} -fopenmp")
	endif ()
	set(MULTI "OPENMP" CACHE STRING "Multithreading interface")
else()
	if(MULTI STREQUAL PTHREAD)
		find_package(Threads REQUIRED)
		set(CFLAGS "${CFLAGS} -pthread")
		set(MULTI "PTHREAD" CACHE STRING "Multithreading interface")
	endif()
endif()

if(PROFL)
	set(PFLAGS "-pg -fno-omit-frame-pointer")
else()
	set(PFLAGS "")
endif()

if(ARITH STREQUAL "gmp" OR ARITH STREQUAL "gmp-sec")
	include(cmake/gmp.cmake)
	if(GMP_FOUND)
		include_directories(${GMP_INCLUDE_DIR})
		set(ARITH_LIBS ${GMP_LIBRARIES})
	endif()
endif()

if(ARITH STREQUAL "fiat")
	message(STATUS "Configured Fiat-Crypto: After build completes, set FIAT_CRYPTO to root folder and run: make fiat; cmake; make.")
endif()

set(CMAKE_C_FLAGS "${AFLAGS} ${WFLAGS} ${DFLAGS} ${PFLAGS} ${CFLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${LFLAGS}")

message(STATUS "Compiler flags: ${CMAKE_C_FLAGS}")
message(STATUS "Linker flags: ${CMAKE_EXE_LINKER_FLAGS}")

string(TOUPPER ${ARITH} ARITH)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/relic_conf.h.in
	${CMAKE_CURRENT_BINARY_DIR}/include/relic_conf.h @ONLY)
message(STATUS "Configured ${CMAKE_CURRENT_SOURCE_DIR}/include/relic_conf.h.in")
string(TOLOWER ${ARITH} ARITH)

if (LABEL)
	set(RELIC "relic_${LABEL}")
	set(RELIC_S "relic_s_${LABEL}")
else()
	set(RELIC "relic")
	set(RELIC_S "relic_s")
endif()

# Choose the arithmetic backend.
set(LABEL "" CACHE STRING "Build label")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB includes "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
install(FILES ${includes} DESTINATION include/${RELIC})
file(GLOB includes "${CMAKE_CURRENT_SOURCE_DIR}/include/low/*.h")
install(FILES ${includes} DESTINATION include/${RELIC}/low)
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/" DESTINATION include/${RELIC})

install(FILES cmake/relic-config.cmake DESTINATION cmake/)

if(DOCUM)
	include(cmake/doxygen.cmake)
endif()

if(TESTS GREATER 0)
	enable_testing()
	add_subdirectory(test)
endif(TESTS GREATER 0)

if(BENCH GREATER 0)
	add_subdirectory(bench)
endif(BENCH GREATER 0)
