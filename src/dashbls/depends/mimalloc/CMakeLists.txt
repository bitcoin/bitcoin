cmake_minimum_required(VERSION 3.0)
project(libmimalloc C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

option(MI_SECURE            "Use full security mitigations (like guard pages, allocation randomization, double-free mitigation, and free-list corruption detection)" OFF)
option(MI_DEBUG_FULL        "Use full internal heap invariant checking in DEBUG mode (expensive)" OFF)
option(MI_PADDING           "Enable padding to detect heap block overflow (used only in DEBUG mode or with Valgrind)" ON)
option(MI_OVERRIDE          "Override the standard malloc interface (e.g. define entry points for malloc() etc)" ON)
option(MI_XMALLOC           "Enable abort() call on memory allocation failure by default" OFF)
option(MI_SHOW_ERRORS       "Show error and warning messages by default (only enabled by default in DEBUG mode)" OFF)
option(MI_VALGRIND          "Compile with Valgrind support (adds a small overhead)" OFF)
option(MI_USE_CXX           "Use the C++ compiler to compile the library (instead of the C compiler)" OFF)
option(MI_SEE_ASM           "Generate assembly files" OFF)
option(MI_OSX_INTERPOSE     "Use interpose to override standard malloc on macOS" ON)
option(MI_OSX_ZONE          "Use malloc zone to override standard malloc on macOS" ON) 
option(MI_WIN_REDIRECT      "Use redirection module ('mimalloc-redirect') on Windows if compiling mimalloc as a DLL" ON)
option(MI_LOCAL_DYNAMIC_TLS "Use slightly slower, dlopen-compatible TLS mechanism (Unix)" OFF)
option(MI_BUILD_SHARED      "Build shared library" ON)
option(MI_BUILD_STATIC      "Build static library" ON)
option(MI_BUILD_OBJECT      "Build object library" ON)
option(MI_BUILD_TESTS       "Build test executables" ON)
option(MI_DEBUG_TSAN        "Build with thread sanitizer (needs clang)" OFF)
option(MI_DEBUG_UBSAN       "Build with undefined-behavior sanitizer (needs clang++)" OFF)
option(MI_SKIP_COLLECT_ON_EXIT, "Skip collecting memory on program exit" OFF)

# deprecated options
option(MI_CHECK_FULL        "Use full internal invariant checking in DEBUG mode (deprecated, use MI_DEBUG_FULL instead)" OFF)
option(MI_INSTALL_TOPLEVEL  "Install directly into $CMAKE_INSTALL_PREFIX instead of PREFIX/lib/mimalloc-version (deprecated)" OFF)
option(MI_USE_LIBATOMIC     "Explicitly link with -latomic (on older systems) (deprecated and detected automatically)" OFF)

include(CheckIncludeFiles)
include(GNUInstallDirs)
include("cmake/mimalloc-config-version.cmake")

set(mi_sources
    src/stats.c
    src/random.c
    src/os.c
    src/bitmap.c
    src/arena.c
    src/segment-cache.c
    src/segment.c
    src/page.c
    src/alloc.c
    src/alloc-aligned.c
    src/alloc-posix.c
    src/heap.c
    src/options.c
    src/init.c)


# -----------------------------------------------------------------------------
# Convenience: set default build type depending on the build directory
# -----------------------------------------------------------------------------

message(STATUS "")    
if (NOT CMAKE_BUILD_TYPE)
  if ("${CMAKE_BINARY_DIR}" MATCHES ".*(D|d)ebug$" OR  MI_DEBUG_FULL)
    message(STATUS "No build type selected, default to: Debug")
    set(CMAKE_BUILD_TYPE "Debug")
  else()
    message(STATUS "No build type selected, default to: Release")
    set(CMAKE_BUILD_TYPE "Release")
  endif()
endif()

if("${CMAKE_BINARY_DIR}" MATCHES ".*(S|s)ecure$")
  message(STATUS "Default to secure build")
  set(MI_SECURE "ON")
endif()


# -----------------------------------------------------------------------------
# Process options
# -----------------------------------------------------------------------------

if(CMAKE_C_COMPILER_ID MATCHES "MSVC|Intel")
  set(MI_USE_CXX "ON")
endif()

if(MI_OVERRIDE)
  message(STATUS "Override standard malloc (MI_OVERRIDE=ON)")
  if(APPLE)
    if(MI_OSX_ZONE)
      # use zone's on macOS
      message(STATUS "  Use malloc zone to override malloc (MI_OSX_ZONE=ON)")
      list(APPEND mi_sources src/alloc-override-osx.c)
      list(APPEND mi_defines MI_OSX_ZONE=1)      
      if (NOT MI_OSX_INTERPOSE)
        message(STATUS "  WARNING: zone overriding usually also needs interpose (use -DMI_OSX_INTERPOSE=ON)")
      endif()
    endif()
    if(MI_OSX_INTERPOSE)
      # use interpose on macOS
      message(STATUS "  Use interpose to override malloc (MI_OSX_INTERPOSE=ON)")
      list(APPEND mi_defines MI_OSX_INTERPOSE=1)
      if (NOT MI_OSX_ZONE)
        message(STATUS "  WARNING: interpose usually also needs zone overriding (use -DMI_OSX_INTERPOSE=ON)")
      endif()
    endif()
    if(MI_USE_CXX AND MI_OSX_INTERPOSE)
      message(STATUS "  WARNING: if dynamically overriding malloc/free, it is more reliable to build mimalloc as C code (use -DMI_USE_CXX=OFF)")
    endif()
  endif()
endif()

if(WIN32)
  if (MI_WIN_REDIRECT)
    if (MSVC_C_ARCHITECTURE_ID MATCHES "ARM")
      message(STATUS "Cannot use redirection on Windows ARM (MI_WIN_REDIRECT=OFF)")
      set(MI_WIN_REDIRECT OFF)
    endif()
  endif()
  if (NOT MI_WIN_REDIRECT)
    # use a negative define for backward compatibility
    list(APPEND mi_defines MI_WIN_NOREDIRECT=1)
  endif()
endif()

if(MI_SECURE)
  message(STATUS "Set full secure build (MI_SECURE=ON)")
  list(APPEND mi_defines MI_SECURE=4)
  #if (MI_VALGRIND)
  #  message(WARNING "Secure mode is a bit weakened when compiling with Valgrind support as buffer overflow detection is no longer byte-precise (if running without valgrind)")
  #endif()
endif()

if(MI_VALGRIND)
  CHECK_INCLUDE_FILES("valgrind/valgrind.h;valgrind/memcheck.h" MI_HAS_VALGRINDH)
  if (NOT MI_HAS_VALGRINDH)
    set(MI_VALGRIND OFF)
    message(WARNING "Cannot find the 'valgrind/valgrind.h' and 'valgrind/memcheck.h' -- install valgrind first")
    message(STATUS  "Compile **without** Valgrind support (MI_VALGRIND=OFF)")
  else()
    message(STATUS "Compile with Valgrind support (MI_VALGRIND=ON)")
    list(APPEND mi_defines MI_VALGRIND=1)
  endif()
endif()

if(MI_SEE_ASM)
  message(STATUS "Generate assembly listings (MI_SEE_ASM=ON)")
  list(APPEND mi_cflags -save-temps)
endif()

if(MI_CHECK_FULL)
  message(STATUS "The MI_CHECK_FULL option is deprecated, use MI_DEBUG_FULL instead")
  set(MI_DEBUG_FULL "ON")
endif()

if (MI_SKIP_COLLECT_ON_EXIT)
  message(STATUS "Skip collecting memory on program exit (MI_SKIP_COLLECT_ON_EXIT=ON)")
  list(APPEND mi_defines MI_SKIP_COLLECT_ON_EXIT=1)
endif()

if(MI_DEBUG_FULL)
  message(STATUS "Set debug level to full internal invariant checking (MI_DEBUG_FULL=ON)")
  list(APPEND mi_defines MI_DEBUG=3)   # full invariant checking
endif()

if(NOT MI_PADDING)
  message(STATUS "Disable padding of heap blocks in debug mode (MI_PADDING=OFF)")
  list(APPEND mi_defines MI_PADDING=0)
endif()

if(MI_XMALLOC)
  message(STATUS "Enable abort() calls on memory allocation failure (MI_XMALLOC=ON)")
  list(APPEND mi_defines MI_XMALLOC=1)
endif()

if(MI_SHOW_ERRORS)
  message(STATUS "Enable printing of error and warning messages by default (MI_SHOW_ERRORS=ON)")
  list(APPEND mi_defines MI_SHOW_ERRORS=1)
endif()

if(MI_DEBUG_TSAN)
  if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(STATUS "Build with thread sanitizer (MI_DEBUG_TSAN=ON)")
    list(APPEND mi_defines MI_TSAN=1)
    list(APPEND mi_cflags -fsanitize=thread -g -O1)
    list(APPEND CMAKE_EXE_LINKER_FLAGS -fsanitize=thread)
  else()
    message(WARNING "Can only use thread sanitizer with clang (MI_DEBUG_TSAN=ON but ignored)")    
  endif()  
endif()

if(MI_DEBUG_UBSAN)
  if(CMAKE_BUILD_TYPE MATCHES "Debug")    
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      message(STATUS "Build with undefined-behavior sanitizer (MI_DEBUG_UBSAN=ON)")
      list(APPEND mi_cflags -fsanitize=undefined -g -fno-sanitize-recover=undefined)
      list(APPEND CMAKE_EXE_LINKER_FLAGS -fsanitize=undefined)
      if (NOT MI_USE_CXX)
        message(STATUS "(switch to use C++ due to MI_DEBUG_UBSAN)")
        set(MI_USE_CXX "ON")
      endif()
    else()
      message(WARNING "Can only use undefined-behavior sanitizer with clang++ (MI_DEBUG_UBSAN=ON but ignored)")    
    endif()  
  else()
    message(WARNING "Can only use thread sanitizer with a debug build (CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})")    
  endif()
endif()

if(MI_USE_CXX)
  message(STATUS "Use the C++ compiler to compile (MI_USE_CXX=ON)")
  set_source_files_properties(${mi_sources} PROPERTIES LANGUAGE CXX )
  set_source_files_properties(src/static.c test/test-api.c test/test-api-fill test/test-stress PROPERTIES LANGUAGE CXX )
  if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang")
    list(APPEND mi_cflags -Wno-deprecated)
  endif()
  if(CMAKE_CXX_COMPILER_ID MATCHES "Intel" AND NOT CMAKE_CXX_COMPILER_ID MATCHES "IntelLLVM")
    list(APPEND mi_cflags -Kc++)
  endif()
endif()

# Compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
  list(APPEND mi_cflags -Wall -Wextra -Wno-unknown-pragmas -fvisibility=hidden)
  if(NOT MI_USE_CXX)
    list(APPEND mi_cflags -Wstrict-prototypes)
  endif()  
  if(CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang")
    list(APPEND mi_cflags -Wpedantic -Wno-static-in-inline)
  endif()
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Intel")
  list(APPEND mi_cflags -Wall -fvisibility=hidden)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang|GNU|Intel" AND NOT CMAKE_SYSTEM_NAME MATCHES "Haiku")
  if(MI_LOCAL_DYNAMIC_TLS)
    list(APPEND mi_cflags -ftls-model=local-dynamic)
  else()
    list(APPEND mi_cflags -ftls-model=initial-exec)
  endif()
  if(MI_OVERRIDE)
    list(APPEND mi_cflags -fno-builtin-malloc)
  endif()
endif()

if (MSVC AND MSVC_VERSION GREATER_EQUAL 1914)
  list(APPEND mi_cflags /Zc:__cplusplus)
endif()

# extra needed libraries
if(WIN32)
  list(APPEND mi_libraries psapi shell32 user32 advapi32 bcrypt)
  set(pc_libraries "-lpsapi -lshell32 -luser32 -ladvapi32 -lbcrypt")
else()
  set(pc_libraries "")
  find_library(MI_LIBPTHREAD pthread)
  if (MI_LIBPTHREAD)                      
    list(APPEND mi_libraries ${MI_LIBPTHREAD})
    set(pc_libraries "${pc_libraries} -pthread")
  endif()
  find_library(MI_LIBRT rt)
  if(MI_LIBRT)
    list(APPEND mi_libraries ${MI_LIBRT})
    set(pc_libraries "${pc_libraries} -lrt")
  endif()
  find_library(MI_LIBATOMIC atomic)
  if (NOT MI_LIBATOMIC AND MI_USE_LIBATOMIC) 
    set(MI_LIBATOMIC atomic)
  endif()
  if (MI_LIBATOMIC) 
    list(APPEND mi_libraries ${MI_LIBATOMIC})
    set(pc_libraries "${pc_libraries} -latomic")
  endif()
endif()

# -----------------------------------------------------------------------------
# Install and output names
# -----------------------------------------------------------------------------

# dynamic/shared library and symlinks always go to /usr/local/lib equivalent
set(mi_install_libdir   "${CMAKE_INSTALL_LIBDIR}")

# static libraries and object files, includes, and cmake config files
# are either installed at top level, or use versioned directories for side-by-side installation (default)
if (MI_INSTALL_TOPLEVEL)
  set(mi_install_objdir     "${CMAKE_INSTALL_LIBDIR}")
  set(mi_install_incdir     "${CMAKE_INSTALL_INCLUDEDIR}")        
  set(mi_install_cmakedir   "${CMAKE_INSTALL_LIBDIR}/cmake/mimalloc")
else()
  set(mi_install_objdir     "${CMAKE_INSTALL_LIBDIR}/mimalloc-${mi_version}")       # for static library and object files
  set(mi_install_incdir     "${CMAKE_INSTALL_INCLUDEDIR}/mimalloc-${mi_version}")   # for includes
  set(mi_install_cmakedir   "${CMAKE_INSTALL_LIBDIR}/cmake/mimalloc-${mi_version}") # for cmake package info
endif()

set(mi_basename "mimalloc")
if(MI_SECURE)
  set(mi_basename "${mi_basename}-secure")
endif()
if(MI_VALGRIND)
  set(mi_basename "${mi_basename}-valgrind")
endif()
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LC)
if(NOT(CMAKE_BUILD_TYPE_LC MATCHES "^(release|relwithdebinfo|minsizerel|none)$"))
  set(mi_basename "${mi_basename}-${CMAKE_BUILD_TYPE_LC}") #append build type (e.g. -debug) if not a release version
endif()

if(MI_BUILD_SHARED)
  list(APPEND mi_build_targets "shared")
endif()
if(MI_BUILD_STATIC)
  list(APPEND mi_build_targets "static")
endif()
if(MI_BUILD_OBJECT)
  list(APPEND mi_build_targets "object")
endif()
if(MI_BUILD_TESTS)
  list(APPEND mi_build_targets "tests")
endif()

message(STATUS "")
message(STATUS "Library base name: ${mi_basename}")
message(STATUS "Version          : ${mi_version}")
message(STATUS "Build type       : ${CMAKE_BUILD_TYPE_LC}")
if(MI_USE_CXX)
  message(STATUS "C++ Compiler     : ${CMAKE_CXX_COMPILER}")
else()
  message(STATUS "C Compiler       : ${CMAKE_C_COMPILER}")
endif()
message(STATUS "Compiler flags   : ${mi_cflags}")
message(STATUS "Compiler defines : ${mi_defines}")
message(STATUS "Link libraries   : ${mi_libraries}")
message(STATUS "Build targets    : ${mi_build_targets}")
message(STATUS "")

# -----------------------------------------------------------------------------
# Main targets
# -----------------------------------------------------------------------------

# shared library
if(MI_BUILD_SHARED)
  add_library(mimalloc SHARED ${mi_sources})
  set_target_properties(mimalloc PROPERTIES VERSION ${mi_version} SOVERSION ${mi_version_major} OUTPUT_NAME ${mi_basename} )
  target_compile_definitions(mimalloc PRIVATE ${mi_defines} MI_SHARED_LIB MI_SHARED_LIB_EXPORT)
  target_compile_options(mimalloc PRIVATE ${mi_cflags})
  target_link_libraries(mimalloc PUBLIC ${mi_libraries})
  target_include_directories(mimalloc PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${mi_install_incdir}>
  )
  if(WIN32 AND MI_WIN_REDIRECT)
    # On windows, link and copy the mimalloc redirection dll too.
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(MIMALLOC_REDIRECT_SUFFIX "32")
    else()
      set(MIMALLOC_REDIRECT_SUFFIX "")
    endif()

    target_link_libraries(mimalloc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bin/mimalloc-redirect${MIMALLOC_REDIRECT_SUFFIX}.lib)
    add_custom_command(TARGET mimalloc POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/bin/mimalloc-redirect${MIMALLOC_REDIRECT_SUFFIX}.dll" $<TARGET_FILE_DIR:mimalloc>
      COMMENT "Copy mimalloc-redirect${MIMALLOC_REDIRECT_SUFFIX}.dll to output directory")
    install(FILES "$<TARGET_FILE_DIR:mimalloc>/mimalloc-redirect${MIMALLOC_REDIRECT_SUFFIX}.dll" DESTINATION ${mi_install_libdir})
  endif()

  install(TARGETS mimalloc EXPORT mimalloc DESTINATION ${mi_install_libdir} LIBRARY)  
  install(EXPORT mimalloc DESTINATION ${mi_install_cmakedir})
endif()

# static library
if (MI_BUILD_STATIC)
  add_library(mimalloc-static STATIC ${mi_sources})
  set_property(TARGET mimalloc-static PROPERTY POSITION_INDEPENDENT_CODE ON)
  target_compile_definitions(mimalloc-static PRIVATE ${mi_defines} MI_STATIC_LIB)
  target_compile_options(mimalloc-static PRIVATE ${mi_cflags})
  target_link_libraries(mimalloc-static PUBLIC ${mi_libraries})
  target_include_directories(mimalloc-static PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${mi_install_incdir}>
  )
  if(WIN32)
    # When building both static and shared libraries on Windows, a static library should use a
    # different output name to avoid the conflict with the import library of a shared one.
    string(REPLACE "mimalloc" "mimalloc-static" mi_output_name ${mi_basename})
    set_target_properties(mimalloc-static PROPERTIES OUTPUT_NAME ${mi_output_name})
  else()
    set_target_properties(mimalloc-static PROPERTIES OUTPUT_NAME ${mi_basename})
  endif()

  install(TARGETS mimalloc-static EXPORT mimalloc DESTINATION ${mi_install_objdir} LIBRARY)
  install(EXPORT mimalloc DESTINATION ${mi_install_cmakedir})
endif()

# install include files
install(FILES include/mimalloc.h DESTINATION ${mi_install_incdir})
install(FILES include/mimalloc-override.h DESTINATION ${mi_install_incdir})
install(FILES include/mimalloc-new-delete.h DESTINATION ${mi_install_incdir})
install(FILES cmake/mimalloc-config.cmake DESTINATION ${mi_install_cmakedir})
install(FILES cmake/mimalloc-config-version.cmake DESTINATION ${mi_install_cmakedir})


# single object file for more predictable static overriding
if (MI_BUILD_OBJECT)
  add_library(mimalloc-obj OBJECT src/static.c)
  set_property(TARGET mimalloc-obj PROPERTY POSITION_INDEPENDENT_CODE ON)
  target_compile_definitions(mimalloc-obj PRIVATE ${mi_defines})
  target_compile_options(mimalloc-obj PRIVATE ${mi_cflags})
  target_include_directories(mimalloc-obj PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${mi_install_incdir}>
  )

  # the following seems to lead to cmake warnings/errors on some systems, disable for now :-(
  # install(TARGETS mimalloc-obj EXPORT mimalloc DESTINATION ${mi_install_objdir})

  # the FILES expression can also be: $<TARGET_OBJECTS:mimalloc-obj>
  # but that fails cmake versions less than 3.10 so we leave it as is for now
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/mimalloc-obj.dir/src/static.c${CMAKE_C_OUTPUT_EXTENSION}
          DESTINATION ${mi_install_objdir}
          RENAME ${mi_basename}${CMAKE_C_OUTPUT_EXTENSION} )
endif()

# pkg-config file support
include("cmake/JoinPaths.cmake")
join_paths(includedir_for_pc_file "\${prefix}" "${CMAKE_INSTALL_INCLUDEDIR}")
join_paths(libdir_for_pc_file "\${prefix}" "${CMAKE_INSTALL_LIBDIR}")

configure_file(mimalloc.pc.in mimalloc.pc @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mimalloc.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")

# -----------------------------------------------------------------------------
# API surface testing
# -----------------------------------------------------------------------------

if (MI_BUILD_TESTS)
  enable_testing()

  foreach(TEST_NAME api api-fill stress)
    add_executable(mimalloc-test-${TEST_NAME} test/test-${TEST_NAME}.c)
    target_compile_definitions(mimalloc-test-${TEST_NAME} PRIVATE ${mi_defines})
    target_compile_options(mimalloc-test-${TEST_NAME} PRIVATE ${mi_cflags})
    target_include_directories(mimalloc-test-${TEST_NAME} PRIVATE include)
    target_link_libraries(mimalloc-test-${TEST_NAME} PRIVATE mimalloc ${mi_libraries})

    add_test(NAME test-${TEST_NAME} COMMAND mimalloc-test-${TEST_NAME})
  endforeach()
endif()

# -----------------------------------------------------------------------------
# Set override properties
# -----------------------------------------------------------------------------
if (MI_OVERRIDE)
  if (MI_BUILD_SHARED)
    target_compile_definitions(mimalloc PRIVATE MI_MALLOC_OVERRIDE)
  endif()
  if(NOT WIN32)
    # It is only possible to override malloc on Windows when building as a DLL.
    if (MI_BUILD_STATIC)
      target_compile_definitions(mimalloc-static PRIVATE MI_MALLOC_OVERRIDE)
    endif()
    if (MI_BUILD_OBJECT)
      target_compile_definitions(mimalloc-obj PRIVATE MI_MALLOC_OVERRIDE)
    endif()
  endif()
endif()
