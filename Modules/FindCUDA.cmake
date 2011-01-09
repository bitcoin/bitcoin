# - Tools for building CUDA C files: libraries and build dependencies.
# This script locates the NVIDIA CUDA C tools. It should work on linux, windows,
# and mac and should be reasonably up to date with CUDA C releases.
#
# This script makes use of the standard find_package arguments of <VERSION>,
# REQUIRED and QUIET.  CUDA_FOUND will report if an acceptable version of CUDA
# was found.
#
# The script will prompt the user to specify CUDA_TOOLKIT_ROOT_DIR if the prefix
# cannot be determined by the location of nvcc in the system path and REQUIRED
# is specified to find_package(). To use a different installed version of the
# toolkit set the environment variable CUDA_BIN_PATH before running cmake
# (e.g. CUDA_BIN_PATH=/usr/local/cuda1.0 instead of the default /usr/local/cuda)
# or set CUDA_TOOLKIT_ROOT_DIR after configuring.  If you change the value of
# CUDA_TOOLKIT_ROOT_DIR, various components that depend on the path will be
# relocated.
#
# It might be necessary to set CUDA_TOOLKIT_ROOT_DIR manually on certain
# platforms, or to use a cuda runtime not installed in the default location. In
# newer versions of the toolkit the cuda library is included with the graphics
# driver- be sure that the driver version matches what is needed by the cuda
# runtime version.
#
# The following variables affect the behavior of the macros in the script (in
# alphebetical order).  Note that any of these flags can be changed multiple
# times in the same directory before calling CUDA_ADD_EXECUTABLE,
# CUDA_ADD_LIBRARY, CUDA_COMPILE, CUDA_COMPILE_PTX or CUDA_WRAP_SRCS.
#
#  CUDA_64_BIT_DEVICE_CODE (Default matches host bit size)
#  -- Set to ON to compile for 64 bit device code, OFF for 32 bit device code.
#     Note that making this different from the host code when generating object
#     or C files from CUDA code just won't work, because size_t gets defined by
#     nvcc in the generated source.  If you compile to PTX and then load the
#     file yourself, you can mix bit sizes between device and host.
#
#  CUDA_ATTACH_VS_BUILD_RULE_TO_CUDA_FILE (Default ON)
#  -- Set to ON if you want the custom build rule to be attached to the source
#     file in Visual Studio.  Turn OFF if you add the same cuda file to multiple
#     targets.
#
#     This allows the user to build the target from the CUDA file; however, bad
#     things can happen if the CUDA source file is added to multiple targets.
#     When performing parallel builds it is possible for the custom build
#     command to be run more than once and in parallel causing cryptic build
#     errors.  VS runs the rules for every source file in the target, and a
#     source can have only one rule no matter how many projects it is added to.
#     When the rule is run from multiple targets race conditions can occur on
#     the generated file.  Eventually everything will get built, but if the user
#     is unaware of this behavior, there may be confusion.  It would be nice if
#     this script could detect the reuse of source files across multiple targets
#     and turn the option off for the user, but no good solution could be found.
#
#  CUDA_BUILD_CUBIN (Default OFF)
#  -- Set to ON to enable and extra compilation pass with the -cubin option in
#     Device mode. The output is parsed and register, shared memory usage is
#     printed during build.
#
#  CUDA_BUILD_EMULATION (Default OFF for device mode)
#  -- Set to ON for Emulation mode. -D_DEVICEEMU is defined for CUDA C files
#     when CUDA_BUILD_EMULATION is TRUE.
#
#  CUDA_GENERATED_OUTPUT_DIR (Default CMAKE_CURRENT_BINARY_DIR)
#  -- Set to the path you wish to have the generated files placed.  If it is
#     blank output files will be placed in CMAKE_CURRENT_BINARY_DIR.
#     Intermediate files will always be placed in
#     CMAKE_CURRENT_BINARY_DIR/CMakeFiles.
#
#  CUDA_HOST_COMPILATION_CPP (Default ON)
#  -- Set to OFF for C compilation of host code.
#
#  CUDA_NVCC_FLAGS
#  CUDA_NVCC_FLAGS_<CONFIG>
#  -- Additional NVCC command line arguments.  NOTE: multiple arguments must be
#     semi-colon delimited (e.g. --compiler-options;-Wall)
#
#  CUDA_PROPAGATE_HOST_FLAGS (Default ON)
#  -- Set to ON to propagate CMAKE_{C,CXX}_FLAGS and their configuration
#     dependent counterparts (e.g. CMAKE_C_FLAGS_DEBUG) automatically to the
#     host compiler through nvcc's -Xcompiler flag.  This helps make the
#     generated host code match the rest of the system better.  Sometimes
#     certain flags give nvcc problems, and this will help you turn the flag
#     propagation off.  This does not affect the flags supplied directly to nvcc
#     via CUDA_NVCC_FLAGS or through the OPTION flags specified through
#     CUDA_ADD_LIBRARY, CUDA_ADD_EXECUTABLE, or CUDA_WRAP_SRCS.  Flags used for
#     shared library compilation are not affected by this flag.
#
#  CUDA_VERBOSE_BUILD (Default OFF)
#  -- Set to ON to see all the commands used when building the CUDA file.  When
#     using a Makefile generator the value defaults to VERBOSE (run make
#     VERBOSE=1 to see output), although setting CUDA_VERBOSE_BUILD to ON will
#     always print the output.
#
# The script creates the following macros (in alphebetical order):
#
#  CUDA_ADD_CUFFT_TO_TARGET( cuda_target )
#  -- Adds the cufft library to the target (can be any target).  Handles whether
#     you are in emulation mode or not.
#
#  CUDA_ADD_CUBLAS_TO_TARGET( cuda_target )
#  -- Adds the cublas library to the target (can be any target).  Handles
#     whether you are in emulation mode or not.
#
#  CUDA_ADD_EXECUTABLE( cuda_target file0 file1 ...
#                       [WIN32] [MACOSX_BUNDLE] [EXCLUDE_FROM_ALL] [OPTIONS ...] )
#  -- Creates an executable "cuda_target" which is made up of the files
#     specified.  All of the non CUDA C files are compiled using the standard
#     build rules specified by CMAKE and the cuda files are compiled to object
#     files using nvcc and the host compiler.  In addition CUDA_INCLUDE_DIRS is
#     added automatically to include_directories().  Some standard CMake target
#     calls can be used on the target after calling this macro
#     (e.g. set_target_properties and target_link_libraries), but setting
#     properties that adjust compilation flags will not affect code compiled by
#     nvcc.  Such flags should be modified before calling CUDA_ADD_EXECUTABLE,
#     CUDA_ADD_LIBRARY or CUDA_WRAP_SRCS.
#
#  CUDA_ADD_LIBRARY( cuda_target file0 file1 ...
#                    [STATIC | SHARED | MODULE] [EXCLUDE_FROM_ALL] [OPTIONS ...] )
#  -- Same as CUDA_ADD_EXECUTABLE except that a library is created.
#
#  CUDA_BUILD_CLEAN_TARGET()
#  -- Creates a convience target that deletes all the dependency files
#     generated.  You should make clean after running this target to ensure the
#     dependency files get regenerated.
#
#  CUDA_COMPILE( generated_files file0 file1 ... [STATIC | SHARED | MODULE]
#                [OPTIONS ...] )
#  -- Returns a list of generated files from the input source files to be used
#     with ADD_LIBRARY or ADD_EXECUTABLE.
#
#  CUDA_COMPILE_PTX( generated_files file0 file1 ... [OPTIONS ...] )
#  -- Returns a list of PTX files generated from the input source files.
#
#  CUDA_INCLUDE_DIRECTORIES( path0 path1 ... )
#  -- Sets the directories that should be passed to nvcc
#     (e.g. nvcc -Ipath0 -Ipath1 ... ). These paths usually contain other .cu
#     files.
#
#  CUDA_WRAP_SRCS ( cuda_target format generated_files file0 file1 ...
#                   [STATIC | SHARED | MODULE] [OPTIONS ...] )
#  -- This is where all the magic happens.  CUDA_ADD_EXECUTABLE,
#     CUDA_ADD_LIBRARY, CUDA_COMPILE, and CUDA_COMPILE_PTX all call this
#     function under the hood.
#
#     Given the list of files (file0 file1 ... fileN) this macro generates
#     custom commands that generate either PTX or linkable objects (use "PTX" or
#     "OBJ" for the format argument to switch).  Files that don't end with .cu
#     or have the HEADER_FILE_ONLY property are ignored.
#
#     The arguments passed in after OPTIONS are extra command line options to
#     give to nvcc.  You can also specify per configuration options by
#     specifying the name of the configuration followed by the options.  General
#     options must preceed configuration specific options.  Not all
#     configurations need to be specified, only the ones provided will be used.
#
#        OPTIONS -DFLAG=2 "-DFLAG_OTHER=space in flag"
#        DEBUG -g
#        RELEASE --use_fast_math
#        RELWITHDEBINFO --use_fast_math;-g
#        MINSIZEREL --use_fast_math
#
#     For certain configurations (namely VS generating object files with
#     CUDA_ATTACH_VS_BUILD_RULE_TO_CUDA_FILE set to ON), no generated file will
#     be produced for the given cuda file.  This is because when you add the
#     cuda file to Visual Studio it knows that this file produces an object file
#     and will link in the resulting object file automatically.
#
#     This script will also generate a separate cmake script that is used at
#     build time to invoke nvcc.  This is for serveral reasons.
#
#       1. nvcc can return negative numbers as return values which confuses
#       Visual Studio into thinking that the command succeeded.  The script now
#       checks the error codes and produces errors when there was a problem.
#
#       2. nvcc has been known to not delete incomplete results when it
#       encounters problems.  This confuses build systems into thinking the
#       target was generated when in fact an unusable file exists.  The script
#       now deletes the output files if there was an error.
#
#       3. By putting all the options that affect the build into a file and then
#       make the build rule dependent on the file, the output files will be
#       regenerated when the options change.
#
#     This script also looks at optional arguments STATIC, SHARED, or MODULE to
#     determine when to target the object compilation for a shared library.
#     BUILD_SHARED_LIBS is ignored in CUDA_WRAP_SRCS, but it is respected in
#     CUDA_ADD_LIBRARY.  On some systems special flags are added for building
#     objects intended for shared libraries.  A preprocessor macro,
#     <target_name>_EXPORTS is defined when a shared library compilation is
#     detected.
#
#     Flags passed into add_definitions with -D or /D are passed along to nvcc.
#
# The script defines the following variables:
#
#  CUDA_VERSION_MAJOR    -- The major version of cuda as reported by nvcc.
#  CUDA_VERSION_MINOR    -- The minor version.
#  CUDA_VERSION
#  CUDA_VERSION_STRING   -- CUDA_VERSION_MAJOR.CUDA_VERSION_MINOR
#
#  CUDA_TOOLKIT_ROOT_DIR -- Path to the CUDA Toolkit (defined if not set).
#  CUDA_SDK_ROOT_DIR     -- Path to the CUDA SDK.  Use this to find files in the
#                           SDK.  This script will not directly support finding
#                           specific libraries or headers, as that isn't
#                           supported by NVIDIA.  If you want to change
#                           libraries when the path changes see the
#                           FindCUDA.cmake script for an example of how to clear
#                           these variables.  There are also examples of how to
#                           use the CUDA_SDK_ROOT_DIR to locate headers or
#                           libraries, if you so choose (at your own risk).
#  CUDA_INCLUDE_DIRS     -- Include directory for cuda headers.  Added automatically
#                           for CUDA_ADD_EXECUTABLE and CUDA_ADD_LIBRARY.
#  CUDA_LIBRARIES        -- Cuda RT library.
#  CUDA_CUFFT_LIBRARIES  -- Device or emulation library for the Cuda FFT
#                           implementation (alternative to:
#                           CUDA_ADD_CUFFT_TO_TARGET macro)
#  CUDA_CUBLAS_LIBRARIES -- Device or emulation library for the Cuda BLAS
#                           implementation (alterative to:
#                           CUDA_ADD_CUBLAS_TO_TARGET macro).
#
#
#  James Bigler, NVIDIA Corp (nvidia.com - jbigler)
#  Abe Stephens, SCI Institute -- http://www.sci.utah.edu/~abe/FindCuda.html
#
#  Copyright (c) 2008 - 2009 NVIDIA Corporation.  All rights reserved.
#
#  Copyright (c) 2007-2009
#  Scientific Computing and Imaging Institute, University of Utah
#
#  This code is licensed under the MIT License.  See the FindCUDA.cmake script
#  for the text of the license.

# The MIT License
#
# License for the specific language governing rights and limitations under
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#
###############################################################################

# FindCUDA.cmake

# We need to have at least this version to support the VERSION_LESS argument to 'if' (2.6.2) and unset (2.6.3)
cmake_policy(PUSH)
cmake_minimum_required(VERSION 2.6.3)
cmake_policy(POP)

# This macro helps us find the location of helper files we will need the full path to
macro(CUDA_FIND_HELPER_FILE _name _extension)
  set(_full_name "${_name}.${_extension}")
  # CMAKE_CURRENT_LIST_FILE contains the full path to the file currently being
  # processed.  Using this variable, we can pull out the current path, and
  # provide a way to get access to the other files we need local to here.
  get_filename_component(CMAKE_CURRENT_LIST_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
  find_file(CUDA_${_name} ${_full_name} PATHS ${CMAKE_CURRENT_LIST_DIR}/FindCUDA NO_DEFAULT_PATH)
  if(NOT CUDA_${_name})
    set(error_message "${_full_name} not found in CMAKE_MODULE_PATH")
    if(CUDA_FIND_REQUIRED)
      message(FATAL_ERROR "${error_message}")
    else(CUDA_FIND_REQUIRED)
      if(NOT CUDA_FIND_QUIETLY)
        message(STATUS "${error_message}")
      endif(NOT CUDA_FIND_QUIETLY)
    endif(CUDA_FIND_REQUIRED)
  endif(NOT CUDA_${_name})
  # Set this variable as internal, so the user isn't bugged with it.
  set(CUDA_${_name} ${CUDA_${_name}} CACHE INTERNAL "Location of ${_full_name}" FORCE)
endmacro(CUDA_FIND_HELPER_FILE)

#####################################################################
## CUDA_INCLUDE_NVCC_DEPENDENCIES
##

# So we want to try and include the dependency file if it exists.  If
# it doesn't exist then we need to create an empty one, so we can
# include it.

# If it does exist, then we need to check to see if all the files it
# depends on exist.  If they don't then we should clear the dependency
# file and regenerate it later.  This covers the case where a header
# file has disappeared or moved.

macro(CUDA_INCLUDE_NVCC_DEPENDENCIES dependency_file)
  set(CUDA_NVCC_DEPEND)
  set(CUDA_NVCC_DEPEND_REGENERATE FALSE)


  # Include the dependency file.  Create it first if it doesn't exist .  The
  # INCLUDE puts a dependency that will force CMake to rerun and bring in the
  # new info when it changes.  DO NOT REMOVE THIS (as I did and spent a few
  # hours figuring out why it didn't work.
  if(NOT EXISTS ${dependency_file})
    file(WRITE ${dependency_file} "#FindCUDA.cmake generated file.  Do not edit.\n")
  endif()
  # Always include this file to force CMake to run again next
  # invocation and rebuild the dependencies.
  #message("including dependency_file = ${dependency_file}")
  include(${dependency_file})

  # Now we need to verify the existence of all the included files
  # here.  If they aren't there we need to just blank this variable and
  # make the file regenerate again.
#   if(DEFINED CUDA_NVCC_DEPEND)
#     message("CUDA_NVCC_DEPEND set")
#   else()
#     message("CUDA_NVCC_DEPEND NOT set")
#   endif()
  if(CUDA_NVCC_DEPEND)
    #message("CUDA_NVCC_DEPEND true")
    foreach(f ${CUDA_NVCC_DEPEND})
      #message("searching for ${f}")
      if(NOT EXISTS ${f})
        #message("file ${f} not found")
        set(CUDA_NVCC_DEPEND_REGENERATE TRUE)
      endif()
    endforeach(f)
  else(CUDA_NVCC_DEPEND)
    #message("CUDA_NVCC_DEPEND false")
    # No dependencies, so regenerate the file.
    set(CUDA_NVCC_DEPEND_REGENERATE TRUE)
  endif(CUDA_NVCC_DEPEND)

  #message("CUDA_NVCC_DEPEND_REGENERATE = ${CUDA_NVCC_DEPEND_REGENERATE}")
  # No incoming dependencies, so we need to generate them.  Make the
  # output depend on the dependency file itself, which should cause the
  # rule to re-run.
  if(CUDA_NVCC_DEPEND_REGENERATE)
    file(WRITE ${dependency_file} "#FindCUDA.cmake generated file.  Do not edit.\n")
  endif(CUDA_NVCC_DEPEND_REGENERATE)

endmacro(CUDA_INCLUDE_NVCC_DEPENDENCIES)

###############################################################################
###############################################################################
# Setup variables' defaults
###############################################################################
###############################################################################

# Allow the user to specify if the device code is supposed to be 32 or 64 bit.
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(CUDA_64_BIT_DEVICE_CODE_DEFAULT ON)
else()
  set(CUDA_64_BIT_DEVICE_CODE_DEFAULT OFF)
endif()
option(CUDA_64_BIT_DEVICE_CODE "Compile device code in 64 bit mode" ${CUDA_64_BIT_DEVICE_CODE_DEFAULT})

# Attach the build rule to the source file in VS.  This option
option(CUDA_ATTACH_VS_BUILD_RULE_TO_CUDA_FILE "Attach the build rule to the CUDA source file.  Enable only when the CUDA source file is added to at most one target." ON)

# Prints out extra information about the cuda file during compilation
option(CUDA_BUILD_CUBIN "Generate and parse .cubin files in Device mode." OFF)

# Set whether we are using emulation or device mode.
option(CUDA_BUILD_EMULATION "Build in Emulation mode" OFF)

# Where to put the generated output.
set(CUDA_GENERATED_OUTPUT_DIR "" CACHE PATH "Directory to put all the output files.  If blank it will default to the CMAKE_CURRENT_BINARY_DIR")

# Parse HOST_COMPILATION mode.
option(CUDA_HOST_COMPILATION_CPP "Generated file extension" ON)

# Extra user settable flags
set(CUDA_NVCC_FLAGS "" CACHE STRING "Semi-colon delimit multiple arguments.")

# Propagate the host flags to the host compiler via -Xcompiler
option(CUDA_PROPAGATE_HOST_FLAGS "Propage C/CXX_FLAGS and friends to the host compiler via -Xcompile" ON)

# Specifies whether the commands used when compiling the .cu file will be printed out.
option(CUDA_VERBOSE_BUILD "Print out the commands run while compiling the CUDA source file.  With the Makefile generator this defaults to VERBOSE variable specified on the command line, but can be forced on with this option." OFF)

mark_as_advanced(
  CUDA_64_BIT_DEVICE_CODE
  CUDA_ATTACH_VS_BUILD_RULE_TO_CUDA_FILE
  CUDA_GENERATED_OUTPUT_DIR
  CUDA_HOST_COMPILATION_CPP
  CUDA_NVCC_FLAGS
  CUDA_PROPAGATE_HOST_FLAGS
  )

# Makefile and similar generators don't define CMAKE_CONFIGURATION_TYPES, so we
# need to add another entry for the CMAKE_BUILD_TYPE.  We also need to add the
# standerd set of 4 build types (Debug, MinSizeRel, Release, and RelWithDebInfo)
# for completeness.  We need run this loop in order to accomodate the addition
# of extra configuration types.  Duplicate entries will be removed by
# REMOVE_DUPLICATES.
set(CUDA_configuration_types ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE} Debug MinSizeRel Release RelWithDebInfo)
list(REMOVE_DUPLICATES CUDA_configuration_types)
foreach(config ${CUDA_configuration_types})
    string(TOUPPER ${config} config_upper)
    set(CUDA_NVCC_FLAGS_${config_upper} "" CACHE STRING "Semi-colon delimit multiple arguments.")
    mark_as_advanced(CUDA_NVCC_FLAGS_${config_upper})
endforeach()

###############################################################################
###############################################################################
# Locate CUDA, Set Build Type, etc.
###############################################################################
###############################################################################

# Check to see if the CUDA_TOOLKIT_ROOT_DIR and CUDA_SDK_ROOT_DIR have changed,
# if they have then clear the cache variables, so that will be detected again.
if(NOT "${CUDA_TOOLKIT_ROOT_DIR}" STREQUAL "${CUDA_TOOLKIT_ROOT_DIR_INTERNAL}")
  unset(CUDA_NVCC_EXECUTABLE CACHE)
  unset(CUDA_VERSION CACHE)
  unset(CUDA_TOOLKIT_INCLUDE CACHE)
  unset(CUDA_CUDART_LIBRARY CACHE)
  if(CUDA_VERSION VERSION_EQUAL "3.0")
    # This only existed in the 3.0 version of the CUDA toolkit
    unset(CUDA_CUDARTEMU_LIBRARY CACHE)
  endif()
  unset(CUDA_CUDA_LIBRARY CACHE)
  unset(CUDA_cublas_LIBRARY CACHE)
  unset(CUDA_cublasemu_LIBRARY CACHE)
  unset(CUDA_cufft_LIBRARY CACHE)
  unset(CUDA_cufftemu_LIBRARY CACHE)
endif()

if(NOT "${CUDA_SDK_ROOT_DIR}" STREQUAL "${CUDA_SDK_ROOT_DIR_INTERNAL}")
  # No specific variables to catch.  Use this kind of code before calling
  # find_package(CUDA) to clean up any variables that may depend on this path.

  #   unset(MY_SPECIAL_CUDA_SDK_INCLUDE_DIR CACHE)
  #   unset(MY_SPECIAL_CUDA_SDK_LIBRARY CACHE)
endif()

# Search for the cuda distribution.
if(NOT CUDA_TOOLKIT_ROOT_DIR)

  # Search in the CUDA_BIN_PATH first.
  find_path(CUDA_TOOLKIT_ROOT_DIR
    NAMES nvcc nvcc.exe
    PATHS ENV CUDA_BIN_PATH
    DOC "Toolkit location."
    NO_DEFAULT_PATH
    )
  # Now search default paths
  find_path(CUDA_TOOLKIT_ROOT_DIR
    NAMES nvcc nvcc.exe
    PATHS /usr/local/bin
          /usr/local/cuda/bin
    DOC "Toolkit location."
    )

  if (CUDA_TOOLKIT_ROOT_DIR)
    string(REGEX REPLACE "[/\\\\]?bin[64]*[/\\\\]?$" "" CUDA_TOOLKIT_ROOT_DIR ${CUDA_TOOLKIT_ROOT_DIR})
    # We need to force this back into the cache.
    set(CUDA_TOOLKIT_ROOT_DIR ${CUDA_TOOLKIT_ROOT_DIR} CACHE PATH "Toolkit location." FORCE)
  endif(CUDA_TOOLKIT_ROOT_DIR)
  if (NOT EXISTS ${CUDA_TOOLKIT_ROOT_DIR})
    if(CUDA_FIND_REQUIRED)
      message(FATAL_ERROR "Specify CUDA_TOOLKIT_ROOT_DIR")
    elseif(NOT CUDA_FIND_QUIETLY)
      message("CUDA_TOOLKIT_ROOT_DIR not found or specified")
    endif()
  endif (NOT EXISTS ${CUDA_TOOLKIT_ROOT_DIR})
endif (NOT CUDA_TOOLKIT_ROOT_DIR)

# CUDA_NVCC_EXECUTABLE
find_program(CUDA_NVCC_EXECUTABLE
  NAMES nvcc
  PATHS "${CUDA_TOOLKIT_ROOT_DIR}/bin"
        "${CUDA_TOOLKIT_ROOT_DIR}/bin64"
  ENV CUDA_BIN_PATH
  NO_DEFAULT_PATH
  )
# Search default search paths, after we search our own set of paths.
find_program(CUDA_NVCC_EXECUTABLE nvcc)
mark_as_advanced(CUDA_NVCC_EXECUTABLE)

if(CUDA_NVCC_EXECUTABLE AND NOT CUDA_VERSION)
  # Compute the version.
  execute_process (COMMAND ${CUDA_NVCC_EXECUTABLE} "--version" OUTPUT_VARIABLE NVCC_OUT)
  string(REGEX REPLACE ".*release ([0-9]+)\\.([0-9]+).*" "\\1" CUDA_VERSION_MAJOR ${NVCC_OUT})
  string(REGEX REPLACE ".*release ([0-9]+)\\.([0-9]+).*" "\\2" CUDA_VERSION_MINOR ${NVCC_OUT})
  set(CUDA_VERSION "${CUDA_VERSION_MAJOR}.${CUDA_VERSION_MINOR}" CACHE STRING "Version of CUDA as computed from nvcc.")
  mark_as_advanced(CUDA_VERSION)
else()
  # Need to set these based off of the cached value
  string(REGEX REPLACE "([0-9]+)\\.([0-9]+).*" "\\1" CUDA_VERSION_MAJOR "${CUDA_VERSION}")
  string(REGEX REPLACE "([0-9]+)\\.([0-9]+).*" "\\2" CUDA_VERSION_MINOR "${CUDA_VERSION}")
endif()

# Always set this convenience variable
set(CUDA_VERSION_STRING "${CUDA_VERSION}")

# Here we need to determine if the version we found is acceptable.  We will
# assume that is unless CUDA_FIND_VERSION_EXACT or CUDA_FIND_VERSION is
# specified.  The presence of either of these options checks the version
# string and signals if the version is acceptable or not.
set(_cuda_version_acceptable TRUE)
#
if(CUDA_FIND_VERSION_EXACT AND NOT CUDA_VERSION VERSION_EQUAL CUDA_FIND_VERSION)
  set(_cuda_version_acceptable FALSE)
endif()
#
if(CUDA_FIND_VERSION       AND     CUDA_VERSION VERSION_LESS  CUDA_FIND_VERSION)
  set(_cuda_version_acceptable FALSE)
endif()
#
if(NOT _cuda_version_acceptable)
  set(_cuda_error_message "Requested CUDA version ${CUDA_FIND_VERSION}, but found unacceptable version ${CUDA_VERSION}")
  if(CUDA_FIND_REQUIRED)
    message("${_cuda_error_message}")
  elseif(NOT CUDA_FIND_QUIETLY)
    message("${_cuda_error_message}")
  endif()
endif()

# CUDA_TOOLKIT_INCLUDE
find_path(CUDA_TOOLKIT_INCLUDE
  device_functions.h # Header included in toolkit
  PATHS "${CUDA_TOOLKIT_ROOT_DIR}/include"
  ENV CUDA_INC_PATH
  NO_DEFAULT_PATH
  )
# Search default search paths, after we search our own set of paths.
find_path(CUDA_TOOLKIT_INCLUDE device_functions.h)
mark_as_advanced(CUDA_TOOLKIT_INCLUDE)

# Set the user list of include dir to nothing to initialize it.
set (CUDA_NVCC_INCLUDE_ARGS_USER "")
set (CUDA_INCLUDE_DIRS ${CUDA_TOOLKIT_INCLUDE})

macro(FIND_LIBRARY_LOCAL_FIRST _var _names _doc)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_cuda_64bit_lib_dir "${CUDA_TOOLKIT_ROOT_DIR}/lib64")
  endif()
  find_library(${_var}
    NAMES ${_names}
    PATHS ${_cuda_64bit_lib_dir}
          "${CUDA_TOOLKIT_ROOT_DIR}/lib"
    ENV CUDA_LIB_PATH
    DOC ${_doc}
    NO_DEFAULT_PATH
    )
  # Search default search paths, after we search our own set of paths.
  find_library(${_var} NAMES ${_names} DOC ${_doc})
endmacro()

# CUDA_LIBRARIES
find_library_local_first(CUDA_CUDART_LIBRARY cudart "\"cudart\" library")
if(CUDA_VERSION VERSION_EQUAL "3.0")
  # The cudartemu library only existed for the 3.0 version of CUDA.
  find_library_local_first(CUDA_CUDARTEMU_LIBRARY cudartemu "\"cudartemu\" library")
  mark_as_advanced(
    CUDA_CUDARTEMU_LIBRARY
    )
endif()
# If we are using emulation mode and we found the cudartemu library then use
# that one instead of cudart.
if(CUDA_BUILD_EMULATION AND CUDA_CUDARTEMU_LIBRARY)
  set(CUDA_LIBRARIES ${CUDA_CUDARTEMU_LIBRARY})
else()
  set(CUDA_LIBRARIES ${CUDA_CUDART_LIBRARY})
endif()
if(APPLE)
  # We need to add the path to cudart to the linker using rpath, since the
  # library name for the cuda libraries is prepended with @rpath.
  if(CUDA_BUILD_EMULATION AND CUDA_CUDARTEMU_LIBRARY)
    get_filename_component(_cuda_path_to_cudart "${CUDA_CUDARTEMU_LIBRARY}" PATH)
  else()
    get_filename_component(_cuda_path_to_cudart "${CUDA_CUDART_LIBRARY}" PATH)
  endif()
  if(_cuda_path_to_cudart)
    list(APPEND CUDA_LIBRARIES -Wl,-rpath "-Wl,${_cuda_path_to_cudart}")
  endif()
endif()

# 1.1 toolkit on linux doesn't appear to have a separate library on
# some platforms.
find_library_local_first(CUDA_CUDA_LIBRARY cuda "\"cuda\" library (older versions only).")

# Add cuda library to the link line only if it is found.
if (CUDA_CUDA_LIBRARY)
  set(CUDA_LIBRARIES ${CUDA_LIBRARIES} ${CUDA_CUDA_LIBRARY})
endif(CUDA_CUDA_LIBRARY)

mark_as_advanced(
  CUDA_CUDA_LIBRARY
  CUDA_CUDART_LIBRARY
  )

#######################
# Look for some of the toolkit helper libraries
macro(FIND_CUDA_HELPER_LIBS _name)
  find_library_local_first(CUDA_${_name}_LIBRARY ${_name} "\"${_name}\" library")
  mark_as_advanced(CUDA_${_name}_LIBRARY)
endmacro(FIND_CUDA_HELPER_LIBS)

# Search for cufft and cublas libraries.
find_cuda_helper_libs(cufftemu)
find_cuda_helper_libs(cublasemu)
find_cuda_helper_libs(cufft)
find_cuda_helper_libs(cublas)

if (CUDA_BUILD_EMULATION)
  set(CUDA_CUFFT_LIBRARIES ${CUDA_cufftemu_LIBRARY})
  set(CUDA_CUBLAS_LIBRARIES ${CUDA_cublasemu_LIBRARY})
else()
  set(CUDA_CUFFT_LIBRARIES ${CUDA_cufft_LIBRARY})
  set(CUDA_CUBLAS_LIBRARIES ${CUDA_cublas_LIBRARY})
endif()

########################
# Look for the SDK stuff
find_path(CUDA_SDK_ROOT_DIR common/inc/cutil.h
  "$ENV{NVSDKCUDA_ROOT}"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\NVIDIA Corporation\\Installed Products\\NVIDIA SDK 10\\Compute;InstallDir]"
  "/Developer/GPU\ Computing/C"
  )

# Keep the CUDA_SDK_ROOT_DIR first in order to be able to override the
# environment variables.
set(CUDA_SDK_SEARCH_PATH
  "${CUDA_SDK_ROOT_DIR}"
  "${CUDA_TOOLKIT_ROOT_DIR}/local/NVSDK0.2"
  "${CUDA_TOOLKIT_ROOT_DIR}/NVSDK0.2"
  "${CUDA_TOOLKIT_ROOT_DIR}/NV_CUDA_SDK"
  "$ENV{HOME}/NVIDIA_CUDA_SDK"
  "$ENV{HOME}/NVIDIA_CUDA_SDK_MACOSX"
  "/Developer/CUDA"
  )

# Example of how to find an include file from the CUDA_SDK_ROOT_DIR

# find_path(CUDA_CUT_INCLUDE_DIR
#   cutil.h
#   PATHS ${CUDA_SDK_SEARCH_PATH}
#   PATH_SUFFIXES "common/inc"
#   DOC "Location of cutil.h"
#   NO_DEFAULT_PATH
#   )
# # Now search system paths
# find_path(CUDA_CUT_INCLUDE_DIR cutil.h DOC "Location of cutil.h")

# mark_as_advanced(CUDA_CUT_INCLUDE_DIR)


# Example of how to find a library in the CUDA_SDK_ROOT_DIR

# # cutil library is called cutil64 for 64 bit builds on windows.  We don't want
# # to get these confused, so we are setting the name based on the word size of
# # the build.

# if(CMAKE_SIZEOF_VOID_P EQUAL 8)
#   set(cuda_cutil_name cutil64)
# else(CMAKE_SIZEOF_VOID_P EQUAL 8)
#   set(cuda_cutil_name cutil32)
# endif(CMAKE_SIZEOF_VOID_P EQUAL 8)

# find_library(CUDA_CUT_LIBRARY
#   NAMES cutil ${cuda_cutil_name}
#   PATHS ${CUDA_SDK_SEARCH_PATH}
#   # The new version of the sdk shows up in common/lib, but the old one is in lib
#   PATH_SUFFIXES "common/lib" "lib"
#   DOC "Location of cutil library"
#   NO_DEFAULT_PATH
#   )
# # Now search system paths
# find_library(CUDA_CUT_LIBRARY NAMES cutil ${cuda_cutil_name} DOC "Location of cutil library")
# mark_as_advanced(CUDA_CUT_LIBRARY)
# set(CUDA_CUT_LIBRARIES ${CUDA_CUT_LIBRARY})



#############################
# Check for required components
set(CUDA_FOUND TRUE)

set(CUDA_TOOLKIT_ROOT_DIR_INTERNAL "${CUDA_TOOLKIT_ROOT_DIR}" CACHE INTERNAL
  "This is the value of the last time CUDA_TOOLKIT_ROOT_DIR was set successfully." FORCE)
set(CUDA_SDK_ROOT_DIR_INTERNAL "${CUDA_SDK_ROOT_DIR}" CACHE INTERNAL
  "This is the value of the last time CUDA_SDK_ROOT_DIR was set successfully." FORCE)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CUDA DEFAULT_MSG
  CUDA_TOOLKIT_ROOT_DIR
  CUDA_NVCC_EXECUTABLE
  CUDA_INCLUDE_DIRS
  CUDA_CUDART_LIBRARY
  _cuda_version_acceptable
  )



###############################################################################
###############################################################################
# Macros
###############################################################################
###############################################################################

###############################################################################
# Add include directories to pass to the nvcc command.
macro(CUDA_INCLUDE_DIRECTORIES)
  foreach(dir ${ARGN})
    list(APPEND CUDA_NVCC_INCLUDE_ARGS_USER "-I${dir}")
  endforeach(dir ${ARGN})
endmacro(CUDA_INCLUDE_DIRECTORIES)


##############################################################################
cuda_find_helper_file(parse_cubin cmake)
cuda_find_helper_file(make2cmake cmake)
cuda_find_helper_file(run_nvcc cmake)

##############################################################################
# Separate the OPTIONS out from the sources
#
macro(CUDA_GET_SOURCES_AND_OPTIONS _sources _cmake_options _options)
  set( ${_sources} )
  set( ${_cmake_options} )
  set( ${_options} )
  set( _found_options FALSE )
  foreach(arg ${ARGN})
    if("${arg}" STREQUAL "OPTIONS")
      set( _found_options TRUE )
    elseif(
        "${arg}" STREQUAL "WIN32" OR
        "${arg}" STREQUAL "MACOSX_BUNDLE" OR
        "${arg}" STREQUAL "EXCLUDE_FROM_ALL" OR
        "${arg}" STREQUAL "STATIC" OR
        "${arg}" STREQUAL "SHARED" OR
        "${arg}" STREQUAL "MODULE"
        )
      list(APPEND ${_cmake_options} "${arg}")
    else()
      if ( _found_options )
        list(APPEND ${_options} "${arg}")
      else()
        # Assume this is a file
        list(APPEND ${_sources} "${arg}")
      endif()
    endif()
  endforeach()
endmacro()

##############################################################################
# Parse the OPTIONS from ARGN and set the variables prefixed by _option_prefix
#
macro(CUDA_PARSE_NVCC_OPTIONS _option_prefix)
  set( _found_config )
  foreach(arg ${ARGN})
    # Determine if we are dealing with a perconfiguration flag
    foreach(config ${CUDA_configuration_types})
      string(TOUPPER ${config} config_upper)
      if ("${arg}" STREQUAL "${config_upper}")
        set( _found_config _${arg})
        # Set arg to nothing to keep it from being processed further
        set( arg )
      endif()
    endforeach()

    if ( arg )
      list(APPEND ${_option_prefix}${_found_config} "${arg}")
    endif()
  endforeach()
endmacro()

##############################################################################
# Helper to add the include directory for CUDA only once
function(CUDA_ADD_CUDA_INCLUDE_ONCE)
  get_directory_property(_include_directories INCLUDE_DIRECTORIES)
  set(_add TRUE)
  if(_include_directories)
    foreach(dir ${_include_directories})
      if("${dir}" STREQUAL "${CUDA_INCLUDE_DIRS}")
        set(_add FALSE)
      endif()
    endforeach()
  endif()
  if(_add)
    include_directories(${CUDA_INCLUDE_DIRS})
  endif()
endfunction()

function(CUDA_BUILD_SHARED_LIBRARY shared_flag)
  set(cmake_args ${ARGN})
  # If SHARED, MODULE, or STATIC aren't already in the list of arguments, then
  # add SHARED or STATIC based on the value of BUILD_SHARED_LIBS.
  list(FIND cmake_args SHARED _cuda_found_SHARED)
  list(FIND cmake_args MODULE _cuda_found_MODULE)
  list(FIND cmake_args STATIC _cuda_found_STATIC)
  if( _cuda_found_SHARED GREATER -1 OR
      _cuda_found_MODULE GREATER -1 OR
      _cuda_found_STATIC GREATER -1)
    set(_cuda_build_shared_libs)
  else()
    if (BUILD_SHARED_LIBS)
      set(_cuda_build_shared_libs SHARED)
    else()
      set(_cuda_build_shared_libs STATIC)
    endif()
  endif()
  set(${shared_flag} ${_cuda_build_shared_libs} PARENT_SCOPE)
endfunction()

##############################################################################
# This helper macro populates the following variables and setups up custom
# commands and targets to invoke the nvcc compiler to generate C or PTX source
# dependant upon the format parameter.  The compiler is invoked once with -M
# to generate a dependency file and a second time with -cuda or -ptx to generate
# a .cpp or .ptx file.
# INPUT:
#   cuda_target         - Target name
#   format              - PTX or OBJ
#   FILE1 .. FILEN      - The remaining arguments are the sources to be wrapped.
#   OPTIONS             - Extra options to NVCC
# OUTPUT:
#   generated_files     - List of generated files
##############################################################################
##############################################################################

macro(CUDA_WRAP_SRCS cuda_target format generated_files)

  if( ${format} MATCHES "PTX" )
    set( compile_to_ptx ON )
  elseif( ${format} MATCHES "OBJ")
    set( compile_to_ptx OFF )
  else()
    message( FATAL_ERROR "Invalid format flag passed to CUDA_WRAP_SRCS: '${format}'.  Use OBJ or PTX.")
  endif()

  # Set up all the command line flags here, so that they can be overriden on a per target basis.

  set(nvcc_flags "")

  # Emulation if the card isn't present.
  if (CUDA_BUILD_EMULATION)
    # Emulation.
    set(nvcc_flags ${nvcc_flags} --device-emulation -D_DEVICEEMU -g)
  else(CUDA_BUILD_EMULATION)
    # Device mode.  No flags necessary.
  endif(CUDA_BUILD_EMULATION)

  if(CUDA_HOST_COMPILATION_CPP)
    set(CUDA_C_OR_CXX CXX)
  else(CUDA_HOST_COMPILATION_CPP)
    if(CUDA_VERSION VERSION_LESS "3.0")
      set(nvcc_flags ${nvcc_flags} --host-compilation C)
    else()
      message(WARNING "--host-compilation flag is deprecated in CUDA version >= 3.0.  Removing --host-compilation C flag" )
    endif()
    set(CUDA_C_OR_CXX C)
  endif(CUDA_HOST_COMPILATION_CPP)

  set(generated_extension ${CMAKE_${CUDA_C_OR_CXX}_OUTPUT_EXTENSION})

  if(CUDA_64_BIT_DEVICE_CODE)
    set(nvcc_flags ${nvcc_flags} -m64)
  else()
    set(nvcc_flags ${nvcc_flags} -m32)
  endif()

  # This needs to be passed in at this stage, because VS needs to fill out the
  # value of VCInstallDir from within VS.
  if(CMAKE_GENERATOR MATCHES "Visual Studio")
    if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
      # Add nvcc flag for 64b Windows
      set(ccbin_flags -D "\"CCBIN:PATH=$(VCInstallDir)bin\"" )
    endif()
  endif()

  # Figure out which configure we will use and pass that in as an argument to
  # the script.  We need to defer the decision until compilation time, because
  # for VS projects we won't know if we are making a debug or release build
  # until build time.
  if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set( CUDA_build_configuration "$(ConfigurationName)" )
  else()
    set( CUDA_build_configuration "${CMAKE_BUILD_TYPE}")
  endif()

  # Initialize our list of includes with the user ones followed by the CUDA system ones.
  set(CUDA_NVCC_INCLUDE_ARGS ${CUDA_NVCC_INCLUDE_ARGS_USER} "-I${CUDA_INCLUDE_DIRS}")
  # Get the include directories for this directory and use them for our nvcc command.
  get_directory_property(CUDA_NVCC_INCLUDE_DIRECTORIES INCLUDE_DIRECTORIES)
  if(CUDA_NVCC_INCLUDE_DIRECTORIES)
    foreach(dir ${CUDA_NVCC_INCLUDE_DIRECTORIES})
      list(APPEND CUDA_NVCC_INCLUDE_ARGS "-I${dir}")
    endforeach()
  endif()

  # Reset these variables
  set(CUDA_WRAP_OPTION_NVCC_FLAGS)
  foreach(config ${CUDA_configuration_types})
    string(TOUPPER ${config} config_upper)
    set(CUDA_WRAP_OPTION_NVCC_FLAGS_${config_upper})
  endforeach()

  CUDA_GET_SOURCES_AND_OPTIONS(_cuda_wrap_sources _cuda_wrap_cmake_options _cuda_wrap_options ${ARGN})
  CUDA_PARSE_NVCC_OPTIONS(CUDA_WRAP_OPTION_NVCC_FLAGS ${_cuda_wrap_options})

  # Figure out if we are building a shared library.  BUILD_SHARED_LIBS is
  # respected in CUDA_ADD_LIBRARY.
  set(_cuda_build_shared_libs FALSE)
  # SHARED, MODULE
  list(FIND _cuda_wrap_cmake_options SHARED _cuda_found_SHARED)
  list(FIND _cuda_wrap_cmake_options MODULE _cuda_found_MODULE)
  if(_cuda_found_SHARED GREATER -1 OR _cuda_found_MODULE GREATER -1)
    set(_cuda_build_shared_libs TRUE)
  endif()
  # STATIC
  list(FIND _cuda_wrap_cmake_options STATIC _cuda_found_STATIC)
  if(_cuda_found_STATIC GREATER -1)
    set(_cuda_build_shared_libs FALSE)
  endif()

  # CUDA_HOST_FLAGS
  if(_cuda_build_shared_libs)
    # If we are setting up code for a shared library, then we need to add extra flags for
    # compiling objects for shared libraries.
    set(CUDA_HOST_SHARED_FLAGS ${CMAKE_SHARED_LIBRARY_${CUDA_C_OR_CXX}_FLAGS})
  else()
    set(CUDA_HOST_SHARED_FLAGS)
  endif()
  # Only add the CMAKE_{C,CXX}_FLAGS if we are propagating host flags.  We
  # always need to set the SHARED_FLAGS, though.
  if(CUDA_PROPAGATE_HOST_FLAGS)
    set(CUDA_HOST_FLAGS "set(CMAKE_HOST_FLAGS ${CMAKE_${CUDA_C_OR_CXX}_FLAGS} ${CUDA_HOST_SHARED_FLAGS})")
  else()
    set(CUDA_HOST_FLAGS "set(CMAKE_HOST_FLAGS ${CUDA_HOST_SHARED_FLAGS})")
  endif()

  set(CUDA_NVCC_FLAGS_CONFIG "# Build specific configuration flags")
  # Loop over all the configuration types to generate appropriate flags for run_nvcc.cmake
  foreach(config ${CUDA_configuration_types})
    string(TOUPPER ${config} config_upper)
    # CMAKE_FLAGS are strings and not lists.  By not putting quotes around CMAKE_FLAGS
    # we convert the strings to lists (like we want).

    if(CUDA_PROPAGATE_HOST_FLAGS)
      # nvcc chokes on -g3, so replace it with -g
      if(CMAKE_COMPILER_IS_GNUCC)
        string(REPLACE "-g3" "-g" _cuda_C_FLAGS "${CMAKE_${CUDA_C_OR_CXX}_FLAGS_${config_upper}}")
      else()
        set(_cuda_C_FLAGS "${CMAKE_${CUDA_C_OR_CXX}_FLAGS_${config_upper}}")
      endif()

      set(CUDA_HOST_FLAGS "${CUDA_HOST_FLAGS}\nset(CMAKE_HOST_FLAGS_${config_upper} ${_cuda_C_FLAGS})")
    endif()

    # Note that if we ever want CUDA_NVCC_FLAGS_<CONFIG> to be string (instead of a list
    # like it is currently), we can remove the quotes around the
    # ${CUDA_NVCC_FLAGS_${config_upper}} variable like the CMAKE_HOST_FLAGS_<CONFIG> variable.
    set(CUDA_NVCC_FLAGS_CONFIG "${CUDA_NVCC_FLAGS_CONFIG}\nset(CUDA_NVCC_FLAGS_${config_upper} \"${CUDA_NVCC_FLAGS_${config_upper}};;${CUDA_WRAP_OPTION_NVCC_FLAGS_${config_upper}}\")")
  endforeach()

  if(compile_to_ptx)
    # Don't use any of the host compilation flags for PTX targets.
    set(CUDA_HOST_FLAGS)
    set(CUDA_NVCC_FLAGS_CONFIG)
  endif()

  # Get the list of definitions from the directory property
  get_directory_property(CUDA_NVCC_DEFINITIONS COMPILE_DEFINITIONS)
  if(CUDA_NVCC_DEFINITIONS)
    foreach(_definition ${CUDA_NVCC_DEFINITIONS})
      list(APPEND nvcc_flags "-D${_definition}")
    endforeach()
  endif()

  if(_cuda_build_shared_libs)
    list(APPEND nvcc_flags "-D${cuda_target}_EXPORTS")
  endif()

  # Determine output directory
  if(CUDA_GENERATED_OUTPUT_DIR)
    set(cuda_compile_output_dir "${CUDA_GENERATED_OUTPUT_DIR}")
  else()
    set(cuda_compile_output_dir "${CMAKE_CURRENT_BINARY_DIR}")
  endif()

  # Reset the output variable
  set(_cuda_wrap_generated_files "")

  # Iterate over the macro arguments and create custom
  # commands for all the .cu files.
  foreach(file ${ARGN})
    # Ignore any file marked as a HEADER_FILE_ONLY
    get_source_file_property(_is_header ${file} HEADER_FILE_ONLY)
    if(${file} MATCHES ".*\\.cu$" AND NOT _is_header)

      # Add a custom target to generate a c or ptx file. ######################

      get_filename_component( basename ${file} NAME )
      if( compile_to_ptx )
        set(generated_file_path "${cuda_compile_output_dir}")
        set(generated_file_basename "${cuda_target}_generated_${basename}.ptx")
        set(format_flag "-ptx")
        file(MAKE_DIRECTORY "${cuda_compile_output_dir}")
      else( compile_to_ptx )
        set(generated_file_path "${cuda_compile_output_dir}/${CMAKE_CFG_INTDIR}")
        set(generated_file_basename "${cuda_target}_generated_${basename}${generated_extension}")
        set(format_flag "-c")
      endif( compile_to_ptx )

      # Set all of our file names.  Make sure that whatever filenames that have
      # generated_file_path in them get passed in through as a command line
      # argument, so that the ${CMAKE_CFG_INTDIR} gets expanded at run time
      # instead of configure time.
      set(generated_file "${generated_file_path}/${generated_file_basename}")
      set(cmake_dependency_file "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${generated_file_basename}.depend")
      set(NVCC_generated_dependency_file "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${generated_file_basename}.NVCC-depend")
      set(generated_cubin_file "${generated_file_path}/${generated_file_basename}.cubin.txt")
      set(custom_target_script "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${generated_file_basename}.cmake")

      # Setup properties for obj files:
      if( NOT compile_to_ptx )
        set_source_files_properties("${generated_file}"
          PROPERTIES
          EXTERNAL_OBJECT true # This is an object file not to be compiled, but only be linked.
          )
      endif()

      # Don't add CMAKE_CURRENT_SOURCE_DIR if the path is already an absolute path.
      get_filename_component(file_path "${file}" PATH)
      if(IS_ABSOLUTE "${file_path}")
        set(source_file "${file}")
      else()
        set(source_file "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
      endif()

      # Bring in the dependencies.  Creates a variable CUDA_NVCC_DEPEND #######
      cuda_include_nvcc_dependencies(${cmake_dependency_file})

      # Convience string for output ###########################################
      if(CUDA_BUILD_EMULATION)
        set(cuda_build_type "Emulation")
      else(CUDA_BUILD_EMULATION)
        set(cuda_build_type "Device")
      endif(CUDA_BUILD_EMULATION)

      # Build the NVCC made dependency file ###################################
      set(build_cubin OFF)
      if ( NOT CUDA_BUILD_EMULATION AND CUDA_BUILD_CUBIN )
         if ( NOT compile_to_ptx )
           set ( build_cubin ON )
         endif( NOT compile_to_ptx )
      endif( NOT CUDA_BUILD_EMULATION AND CUDA_BUILD_CUBIN )

      # Configure the build script
      configure_file("${CUDA_run_nvcc}" "${custom_target_script}" @ONLY)

      # So if a user specifies the same cuda file as input more than once, you
      # can have bad things happen with dependencies.  Here we check an option
      # to see if this is the behavior they want.
      if(CUDA_ATTACH_VS_BUILD_RULE_TO_CUDA_FILE)
        set(main_dep MAIN_DEPENDENCY ${source_file})
      else()
        set(main_dep DEPENDS ${source_file})
      endif()

      if(CUDA_VERBOSE_BUILD)
        set(verbose_output ON)
      elseif(CMAKE_GENERATOR MATCHES "Makefiles")
        set(verbose_output "$(VERBOSE)")
      else()
        set(verbose_output OFF)
      endif()

      # Create up the comment string
      file(RELATIVE_PATH generated_file_relative_path "${CMAKE_BINARY_DIR}" "${generated_file}")
      if(compile_to_ptx)
        set(cuda_build_comment_string "Building NVCC ptx file ${generated_file_relative_path}")
      else()
        set(cuda_build_comment_string "Building NVCC (${cuda_build_type}) object ${generated_file_relative_path}")
      endif()

      # Build the generated file and dependency file ##########################
      add_custom_command(
        OUTPUT ${generated_file}
        # These output files depend on the source_file and the contents of cmake_dependency_file
        ${main_dep}
        DEPENDS ${CUDA_NVCC_DEPEND}
        DEPENDS ${custom_target_script}
        # Make sure the output directory exists before trying to write to it.
        COMMAND ${CMAKE_COMMAND} -E make_directory "${generated_file_path}"
        COMMAND ${CMAKE_COMMAND} ARGS
          -D verbose:BOOL=${verbose_output}
          ${ccbin_flags}
          -D build_configuration:STRING=${CUDA_build_configuration}
          -D "generated_file:STRING=${generated_file}"
          -D "generated_cubin_file:STRING=${generated_cubin_file}"
          -P "${custom_target_script}"
        COMMENT "${cuda_build_comment_string}"
        )

      # Make sure the build system knows the file is generated.
      set_source_files_properties(${generated_file} PROPERTIES GENERATED TRUE)

      # Don't add the object file to the list of generated files if we are using
      # visual studio and we are attaching the build rule to the cuda file.  VS
      # will add our object file to the linker automatically for us.
      set(cuda_add_generated_file TRUE)

      if(NOT compile_to_ptx AND CMAKE_GENERATOR MATCHES "Visual Studio" AND CUDA_ATTACH_VS_BUILD_RULE_TO_CUDA_FILE)
        # Visual Studio 8 crashes when you close the solution when you don't add the object file.
        if(NOT CMAKE_GENERATOR MATCHES "Visual Studio 8")
          #message("Not adding ${generated_file}")
          set(cuda_add_generated_file FALSE)
        endif()
      endif()

      if(cuda_add_generated_file)
        list(APPEND _cuda_wrap_generated_files ${generated_file})
      endif()

      # Add the other files that we want cmake to clean on a cleanup ##########
      list(APPEND CUDA_ADDITIONAL_CLEAN_FILES "${cmake_dependency_file}")
      list(REMOVE_DUPLICATES CUDA_ADDITIONAL_CLEAN_FILES)
      set(CUDA_ADDITIONAL_CLEAN_FILES ${CUDA_ADDITIONAL_CLEAN_FILES} CACHE INTERNAL "List of intermediate files that are part of the cuda dependency scanning.")

    endif(${file} MATCHES ".*\\.cu$" AND NOT _is_header)
  endforeach(file)

  # Set the return parameter
  set(${generated_files} ${_cuda_wrap_generated_files})
endmacro(CUDA_WRAP_SRCS)


###############################################################################
###############################################################################
# ADD LIBRARY
###############################################################################
###############################################################################
macro(CUDA_ADD_LIBRARY cuda_target)

  CUDA_ADD_CUDA_INCLUDE_ONCE()

  # Separate the sources from the options
  CUDA_GET_SOURCES_AND_OPTIONS(_sources _cmake_options _options ${ARGN})
  CUDA_BUILD_SHARED_LIBRARY(_cuda_shared_flag ${ARGN})
  # Create custom commands and targets for each file.
  CUDA_WRAP_SRCS( ${cuda_target} OBJ _generated_files ${_sources}
    ${_cmake_options} ${_cuda_shared_flag}
    OPTIONS ${_options} )

  # Add the library.
  add_library(${cuda_target} ${_cmake_options}
    ${_generated_files}
    ${_sources}
    )

  target_link_libraries(${cuda_target}
    ${CUDA_LIBRARIES}
    )

  # We need to set the linker language based on what the expected generated file
  # would be. CUDA_C_OR_CXX is computed based on CUDA_HOST_COMPILATION_CPP.
  set_target_properties(${cuda_target}
    PROPERTIES
    LINKER_LANGUAGE ${CUDA_C_OR_CXX}
    )

endmacro(CUDA_ADD_LIBRARY cuda_target)


###############################################################################
###############################################################################
# ADD EXECUTABLE
###############################################################################
###############################################################################
macro(CUDA_ADD_EXECUTABLE cuda_target)

  CUDA_ADD_CUDA_INCLUDE_ONCE()

  # Separate the sources from the options
  CUDA_GET_SOURCES_AND_OPTIONS(_sources _cmake_options _options ${ARGN})
  # Create custom commands and targets for each file.
  CUDA_WRAP_SRCS( ${cuda_target} OBJ _generated_files ${_sources} OPTIONS ${_options} )

  # Add the library.
  add_executable(${cuda_target} ${_cmake_options}
    ${_generated_files}
    ${_sources}
    )

  target_link_libraries(${cuda_target}
    ${CUDA_LIBRARIES}
    )

  # We need to set the linker language based on what the expected generated file
  # would be. CUDA_C_OR_CXX is computed based on CUDA_HOST_COMPILATION_CPP.
  set_target_properties(${cuda_target}
    PROPERTIES
    LINKER_LANGUAGE ${CUDA_C_OR_CXX}
    )

endmacro(CUDA_ADD_EXECUTABLE cuda_target)


###############################################################################
###############################################################################
# CUDA COMPILE
###############################################################################
###############################################################################
macro(CUDA_COMPILE generated_files)

  # Separate the sources from the options
  CUDA_GET_SOURCES_AND_OPTIONS(_sources _cmake_options _options ${ARGN})
  # Create custom commands and targets for each file.
  CUDA_WRAP_SRCS( cuda_compile OBJ _generated_files ${_sources} ${_cmake_options}
    OPTIONS ${_options} )

  set( ${generated_files} ${_generated_files})

endmacro(CUDA_COMPILE)


###############################################################################
###############################################################################
# CUDA COMPILE PTX
###############################################################################
###############################################################################
macro(CUDA_COMPILE_PTX generated_files)

  # Separate the sources from the options
  CUDA_GET_SOURCES_AND_OPTIONS(_sources _cmake_options _options ${ARGN})
  # Create custom commands and targets for each file.
  CUDA_WRAP_SRCS( cuda_compile_ptx PTX _generated_files ${_sources} ${_cmake_options}
    OPTIONS ${_options} )

  set( ${generated_files} ${_generated_files})

endmacro(CUDA_COMPILE_PTX)

###############################################################################
###############################################################################
# CUDA ADD CUFFT TO TARGET
###############################################################################
###############################################################################
macro(CUDA_ADD_CUFFT_TO_TARGET target)
  if (CUDA_BUILD_EMULATION)
    target_link_libraries(${target} ${CUDA_cufftemu_LIBRARY})
  else()
    target_link_libraries(${target} ${CUDA_cufft_LIBRARY})
  endif()
endmacro()

###############################################################################
###############################################################################
# CUDA ADD CUBLAS TO TARGET
###############################################################################
###############################################################################
macro(CUDA_ADD_CUBLAS_TO_TARGET target)
  if (CUDA_BUILD_EMULATION)
    target_link_libraries(${target} ${CUDA_cublasemu_LIBRARY})
  else()
    target_link_libraries(${target} ${CUDA_cublas_LIBRARY})
  endif()
endmacro()

###############################################################################
###############################################################################
# CUDA BUILD CLEAN TARGET
###############################################################################
###############################################################################
macro(CUDA_BUILD_CLEAN_TARGET)
  # Call this after you add all your CUDA targets, and you will get a convience
  # target.  You should also make clean after running this target to get the
  # build system to generate all the code again.

  set(cuda_clean_target_name clean_cuda_depends)
  if (CMAKE_GENERATOR MATCHES "Visual Studio")
    string(TOUPPER ${cuda_clean_target_name} cuda_clean_target_name)
  endif()
  add_custom_target(${cuda_clean_target_name}
    COMMAND ${CMAKE_COMMAND} -E remove ${CUDA_ADDITIONAL_CLEAN_FILES})

  # Clear out the variable, so the next time we configure it will be empty.
  # This is useful so that the files won't persist in the list after targets
  # have been removed.
  set(CUDA_ADDITIONAL_CLEAN_FILES "" CACHE INTERNAL "List of intermediate files that are part of the cuda dependency scanning.")
endmacro(CUDA_BUILD_CLEAN_TARGET)
