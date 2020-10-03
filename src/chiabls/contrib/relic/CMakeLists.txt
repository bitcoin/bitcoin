cmake_minimum_required(VERSION 3.1)
if(NOT ${CMAKE_VERSION} VERSION_LESS "3.1")
	cmake_policy(SET CMP0054 NEW)
endif()

project(RELIC C CXX)

set(PROJECT_VERSION_MAJOR "0")
set(PROJECT_VERSION_MINOR "4")
set(PROJECT_VERSION_PATCH "1")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(VERSION ${PROJECT_VERSION})

set(INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include/low)
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
message("   TRACE=[off|on] Build with tracing support.")
message("   OVERH=[off|on] Build with overhead estimation.")
message("   DOCUM=[off|on] Build documentation.")
message("   STRIP=[off|on] Build only selected algorithms.")
message("   QUIET=[off|on] Build with printing disabled.")
message("   COLOR=[off|on] Build with colored output.")
message("   BIGED=[off|on] Build with big-endian support.")
message("   SHLIB=[off|on] Build shared library.")
message("   STLIB=[off|on] Build static library.")
message("   STBIN=[off|on] Build static binaries.")
message("   AMALG=[off|on] Build amalgamation for better performance.\n")

option(DEBUG "Build with debugging support" off)
option(PROFL "Build with profiling support" off)
option(CHECK "Build with error-checking support" on)
option(VERBS "Build with detailed error messages" on)
option(TRACE "Build with tracing support" off)
option(OVERH "Build with overhead estimation" off)
option(DOCUM "Build documentation" on)
option(STRIP "Build only the selected algorithms" off)
option(QUIET "Build with printing disabled" off)
option(COLOR "Build with colored output" on)
option(BIGED "Build with big-endian support" off)
option(SHLIB "Build shared library" on)
option(STLIB "Build static library" on)
option(STBIN "Build static binaries" on)
option(AMALG "Build amalgamation" off)

message(STATUS "Number of times each test or benchmark is ran (default = 50, 1000):\n")

message("   TESTS=n        If n > 0, build automated tests and run them n times.")
message("   BENCH=n        If n > 0, build automated benchmarks and run them n * n times.\n")

message(STATUS "Number of available processor cores (default = 1):\n")

message("   CORES=n        If n > 1, please enable multithreading support.\n")

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
message("   WITH=ALL      All of the above.")
message("   Note: the programmer is responsible for not using unselected modules.\n")

message(STATUS "Available arithmetic backends (default = easy):\n")

message("   ARITH=easy     Easy-to-understand implementation.")
message("   ARITH=gmp      GNU Multiple Precision library.\n")

message(STATUS "Available memory-allocation policies (default = AUTO):\n")

message("   ALLOC=AUTO     All memory is automatically allocated.")
message("   ALLOC=STATIC   All memory is allocated statically once.")
message("   ALLOC=DYNAMIC  All memory is allocated dynamically on demand.")
message("   ALLOC=STACK    All memory is allocated from the stack.\n")

message(STATUS "Supported operating systems (default = LINUX):\n")

message("   OPSYS=         Undefined/No specific operating system.")
message("   OPSYS=LINUX    GNU/Linux operating system.")
message("   OPSYS=FREEBSD  FreeBSD operating system.")
message("   OPSYS=MACOSX   Mac OS X operating system.")
message("   OPSYS=WINDOWS  Windows operating system.")
message("   OPSYS=DROID    Android operating system.")
message("   OPSYS=DUINO    Arduino platform.\n")

message(STATUS "Supported multithreading APIs (default = UNDEF):\n")

message("   MULTI=         No multithreading support.")
message("   MULTI=OPENMP   Open Multi-Processing.")
message("   MULTI=PTHREAD  POSIX threads.\n")

message(STATUS "Supported timers (default = HPROC):\n")

message("   TIMER=         No timer.")
message("   TIMER=HREAL    GNU/Linux realtime high-resolution timer.")
message("   TIMER=HPROC    GNU/Linux per-process high-resolution timer.")
message("   TIMER=HTHRD    GNU/Linux per-thread high-resolution timer.")
message("   TIMER=ANSI     ANSI-compatible timer.")
message("   TIMER=POSIX    POSIX-compatible timer.")
message("   TIMER=CYCLE    Cycle-counting timer. (architecture-dependant)\n")

message(STATUS "Prefix to identify this build of the library (default = \"\"):\n")

message("   LABEL=relic\n")

include(cmake/arch.cmake)
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

# Number of tests and benchmarks
set(BENCH 100 CACHE INTEGER "Number of times each benchmark is ran.")
set(TESTS 100 CACHE INTEGER "Number of times each test is ran.")

# Number of cores.
set(CORES 1 CACHE INTEGER "Number of available processor cores.")

# Default modules.
set(WITH "ALL" CACHE STRING "Selected modules")
LIST(FIND WITH "ALL" TEMP)
if(TEMP GREATER -1)
	set(WITH_BN 1)
	set(WITH_DV 1)
	set(WITH_FP 1)
	set(WITH_FPX 1)
	set(WITH_FB 1)
	set(WITH_FBX 1)
	set(WITH_FT 1)
	set(WITH_EP 1)
	set(WITH_EPX 1)
	set(WITH_EB 1)
	set(WITH_ED 1)
	set(WITH_EC 1)
	set(WITH_PP 1)
	set(WITH_PB 1)
	set(WITH_PC 1)
	set(WITH_BC 1)
	set(WITH_MD 1)
	set(WITH_CP 1)
endif(TEMP GREATER -1)

# Check if multiple precision integer arithmetic is required.
LIST(FIND WITH "BN" TEMP)
if(TEMP GREATER -1)
	set(WITH_BN 1)
endif(TEMP GREATER -1)

# Check if temporary vectors are required.
LIST(FIND WITH "DV" TEMP)
if(TEMP GREATER -1)
	set(WITH_DV 1)
endif(TEMP GREATER -1)

# Check if prime field arithmetic is required.
LIST(FIND WITH "FP" TEMP)
if(TEMP GREATER -1)
	set(WITH_FP 1)
endif(TEMP GREATER -1)

# Check if prime extension field arithmetic is required.
LIST(FIND WITH "FPX" TEMP)
if(TEMP GREATER -1)
	set(WITH_FPX 1)
endif(TEMP GREATER -1)

# Check if binary field arithmetic is required.
LIST(FIND WITH "FB" TEMP)
if(TEMP GREATER -1)
	set(WITH_FB 1)
endif(TEMP GREATER -1)

# Check if binary extension field arithmetic is required.
LIST(FIND WITH "FBX" TEMP)
if(TEMP GREATER -1)
	set(WITH_FBX 1)
endif(TEMP GREATER -1)

# Check if prime elliptic curve support is required.
LIST(FIND WITH "EP" TEMP)
if(TEMP GREATER -1)
	set(WITH_EP 1)
endif(TEMP GREATER -1)

#Check if support for elliptic curves defined over prime field extensions is required.
LIST(FIND WITH "EPX" TEMP)
if (TEMP GREATER -1)
	set(WITH_EPX 1)
endif(TEMP GREATER -1)

# Check if binary elliptic curve support is required.
LIST(FIND WITH "EB" TEMP)
if(TEMP GREATER -1)
	set(WITH_EB 1)
endif(TEMP GREATER -1)

# Check if binary elliptic curve support is required.
LIST(FIND WITH "ED" TEMP)
if(TEMP GREATER -1)
	set(WITH_ED 1)
endif(TEMP GREATER -1)

# Check if elliptic curve cryptography support is required.
LIST(FIND WITH "EC" TEMP)
if(TEMP GREATER -1)
	set(WITH_EC 1)
endif(TEMP GREATER -1)

# Check if support for pairings over prime curves is required.
LIST(FIND WITH "PP" TEMP)
if(TEMP GREATER -1)
	set(WITH_PP 1)
endif(TEMP GREATER -1)

# Check if support for pairings over binary curves is required.
LIST(FIND WITH "PB" TEMP)
if(TEMP GREATER -1)
	set(WITH_PB 1)
endif(TEMP GREATER -1)

# Check if elliptic curve cryptography support is required.
LIST(FIND WITH "PC" TEMP)
if(TEMP GREATER -1)
	set(WITH_PC 1)
endif(TEMP GREATER -1)

# Check if support for block ciphers is required.
LIST(FIND WITH "BC" TEMP)
if(TEMP GREATER -1)
	set(WITH_BC 1)
endif(TEMP GREATER -1)

# Check if support for hash functions is required.
LIST(FIND WITH "MD" TEMP)
if(TEMP GREATER -1)
	set(WITH_MD 1)
endif(TEMP GREATER -1)

# Check if support for crptographic protocols is required.
LIST(FIND WITH "CP" TEMP)
if(TEMP GREATER -1)
	set(WITH_CP 1)
endif(TEMP GREATER -1)

# Choose the arithmetic backend.
set(ARITH "easy" CACHE STRING "Arithmetic backend")

# Choose the memory-allocation policy.
set(ALLOC "AUTO" CACHE STRING "Allocation policy")

# Compiler flags.
if("$ENV{COMP}" STREQUAL "")
	set(COMP "-O2 -funroll-loops -fomit-frame-pointer" CACHE STRING "User-chosen compiler flags.")
else("$ENV{COMP}" STREQUAL "")
	set(COMP "$ENV{COMP}" CACHE STRING "User-chosen compiler flags.")
endif("$ENV{COMP}" STREQUAL "")

# Simulator of the target platform.
set(SIMUL "$ENV{SIMUL}" CACHE STRING "Path to call a simulator of the target platform.")
set(SIMAR "$ENV{SIMAR}" CACHE STRING "Arguments to call a simulator of the target platform.")
string(REPLACE " " ";" SIMAR "${SIMAR}")

# Linker flags.
string(TOLOWER ${ARITH} LFLAGS)
set(LFLAGS "-L${CMAKE_CURRENT_SOURCE_DIR}/src/low/${LFLAGS}/")
set(LINK "$ENV{LINK}" CACHE STRING "User-chosen linker flags.")
set(LFLAGS "${LFLAGS} ${LINK}")

if(STBIN)
	set(LFLAGS "${LFLAGS} -static")
endif(STBIN)

# Discover the operating system.
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
	set(OPSYS "LINUX" CACHE STRING "Operating system")
else(CMAKE_SYSTEM_NAME STREQUAL Linux)
	if(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
		set(OPSYS "FREEBSD" CACHE STRING "Operating system")
	else(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
		if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
			set(OPSYS "MACOSX" CACHE STRING "Operating system")
		else(CMAKE_SYSTEM_NAME STREQUAL Darwin)
			set(OPSYS "RELIC_NONE" CACHE STRING "Operating system")
		endif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
	endif(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
endif(CMAKE_SYSTEM_NAME STREQUAL Linux)
if(OPSYS STREQUAL LINUX)
	add_definitions(-D_GNU_SOURCE)
endif(OPSYS STREQUAL LINUX)
message(STATUS "Configured operating system: ${OPSYS}")

if(OPSYS STREQUAL LINUX)
	set(TIMER "HPROC" CACHE STRING "Timer")
else(OPSYS STREQUAL LINUX)
	set(TIMER "ANSI" CACHE STRING "Timer")
endif(OPSYS STREQUAL LINUX)

if(DEBUG)
	# If the user did not specify compile flags, we turn off all optimizations.
	set(CFLAGS "-O0 -fno-omit-frame-pointer")
	set(DFLAGS "-ggdb")
else(DEBUG)
	# If the user did not specify compile flags, we use sane defaults.
	set(CFLAGS "${COMP}")
	set(DFLAGS "")
endif(DEBUG)

if(MULTI STREQUAL OPENMP)
	find_package(OpenMP REQUIRED)
	set(CFLAGS "${CFLAGS} -fopenmp")
	set(MULTI "OPENMP" CACHE STRING "Multithreading interface")
else(MULTI STREQUAL OPENMP)
	if(MULTI STREQUAL PTHREAD)
		find_package(Threads REQUIRED)
		set(CFLAGS "${CFLAGS} -pthread")
		set(MULTI "PTHREAD" CACHE STRING "Multithreading interface")
	else(MULTI STREQUAL PTHREAD)
		set(MULTI "RELIC_NONE" CACHE STRING "Operating system")
	endif(MULTI STREQUAL PTHREAD)
endif(MULTI STREQUAL OPENMP)

if(PROFL)
	set(PFLAGS "-pg -fno-omit-frame-pointer")
else(PROFL)
	set(PFLAGS "")
endif(PROFL)

if(TRACE)
	set(DFLAGS "${DFLAGS} -fPIC -finstrument-functions")
endif(TRACE)

if(ARITH STREQUAL "gmp" OR ARITH STREQUAL "gmp-sec")
	include(cmake/gmp.cmake)
	if(GMP_FOUND)
		include_directories(${GMP_INCLUDE_DIR})
		set(ARITH_LIBS ${GMP_LIBRARIES})
	endif(GMP_FOUND)
endif(ARITH STREQUAL "gmp" OR ARITH STREQUAL "gmp-sec")

set(CMAKE_C_FLAGS "-pipe -std=c99 ${AFLAGS} ${WFLAGS} ${DFLAGS} ${PFLAGS} ${CFLAGS}")
set(CMAKE_EXE_LINKER_FLAGS ${LFLAGS})

message(STATUS "Compiler flags: ${CMAKE_C_FLAGS}")
message(STATUS "Linker flags: ${LFLAGS}")

string(TOUPPER ${ARITH} ARITH)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/relic_conf.h.in
	${CMAKE_CURRENT_BINARY_DIR}/include/relic_conf.h @ONLY)
message(STATUS "Configured ${CMAKE_CURRENT_SOURCE_DIR}/include/relic_conf.h.in")
string(TOLOWER ${ARITH} ARITH)

if (LABEL)
	set(RELIC "relic_${LABEL}")
	set(RELIC_S "relic_s_${LABEL}")
else(LABEL)
	set(RELIC "relic")
	set(RELIC_S "relic_s")
endif(LABEL)

# Choose the arithmetic backend.
set(LABEL "" CACHE STRING "Build label")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB includes "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
install(FILES ${includes} DESTINATION include)
file(GLOB includes "${CMAKE_CURRENT_SOURCE_DIR}/include/low/*.h")
install(FILES ${includes} DESTINATION include/low)
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/" DESTINATION include)

install(FILES cmake/relic-config.cmake DESTINATION cmake/)

if(DOCUM)
	include(cmake/doxygen.cmake)
endif(DOCUM)

if(TESTS GREATER 0)
	enable_testing()
	add_subdirectory(test)
endif(TESTS GREATER 0)

if(BENCH GREATER 0)
	add_subdirectory(bench)
endif(BENCH GREATER 0)
