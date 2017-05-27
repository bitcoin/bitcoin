# - Try to find the OpenSSL encryption library
# Once done this will define
#
#  OPENSSL_ROOT_DIR - Set this variable to the root installation of OpenSSL
#
# Read-Only variables:
#  OPENSSL_FOUND - system has the OpenSSL library
#  OPENSSL_INCLUDE_DIR - the OpenSSL include directory
#  OPENSSL_LIBRARIES - The libraries needed to use OpenSSL

#=============================================================================
# Copyright 2006-2009 Kitware, Inc.
# Copyright 2006 Alexander Neundorf <neundorf@kde.org>
# Copyright 2009-2010 Mathieu Malaterre <mathieu.malaterre@gmail.com>
# Minor fixes and bundled with Bitcoin build system by Dimitri Kaparis
# <kaparis.dimitri at gmail dot com>
#

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:

# * Redistributions of source code must retain the above copyright
  # notice, this list of conditions and the following disclaimer.

# * Redistributions in binary form must reproduce the above copyright
  # notice, this list of conditions and the following disclaimer in the
  # documentation and/or other materials provided with the distribution.

# * Neither the names of Kitware, Inc., the Insight Software Consortium,
  # nor the names of their contributors may be used to endorse or promote
  # products derived from this software without specific prior written
  # permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

# http://www.slproweb.com/products/Win32OpenSSL.html
SET(_OPENSSL_ROOT_HINTS
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\OpenSSL (32-bit)_is1;Inno Setup: App Path]"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\OpenSSL (64-bit)_is1;Inno Setup: App Path]"
  )
SET(_OPENSSL_ROOT_PATHS
  "C:/OpenSSL/"
  )
FIND_PATH(OPENSSL_ROOT_DIR
  NAMES include/openssl/ssl.h
  HINTS ${_OPENSSL_ROOT_HINTS}
  PATHS ${_OPENSSL_ROOT_PATHS}
)
MARK_AS_ADVANCED(OPENSSL_ROOT_DIR)

# Re-use the previous path:
FIND_PATH(OPENSSL_INCLUDE_DIR openssl/ssl.h
  ${OPENSSL_ROOT_DIR}/include
)

IF(WIN32 AND NOT CYGWIN)
  # MINGW should go here too
  IF(MSVC)
    # /MD and /MDd are the standard values - if someone wants to use
    # others, the libnames have to change here too
    # use also ssl and ssleay32 in debug as fallback for openssl < 0.9.8b
    # TODO: handle /MT and static lib
    # In Visual C++ naming convention each of these four kinds of Windows libraries has it's standard suffix:
    #   * MD for dynamic-release
    #   * MDd for dynamic-debug
    #   * MT for static-release
    #   * MTd for static-debug

    # Implementation details:
    # We are using the libraries located in the VC subdir instead of the parent directory eventhough :
    # libeay32MD.lib is identical to ../libeay32.lib, and
    # ssleay32MD.lib is identical to ../ssleay32.lib
    FIND_LIBRARY(LIB_EAY_DEBUG NAMES libeay32MDd libeay32
      PATHS ${OPENSSL_ROOT_DIR}/lib/VC
      )
    FIND_LIBRARY(LIB_EAY_RELEASE NAMES libeay32MD libeay32
      PATHS ${OPENSSL_ROOT_DIR}/lib/VC
      )
    FIND_LIBRARY(SSL_EAY_DEBUG NAMES ssleay32MDd ssleay32 ssl
      PATHS ${OPENSSL_ROOT_DIR}/lib/VC
      )
    FIND_LIBRARY(SSL_EAY_RELEASE NAMES ssleay32MD ssleay32 ssl
      PATHS ${OPENSSL_ROOT_DIR}/lib/VC
      )
    if( CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE )
      set( OPENSSL_LIBRARIES
        optimized ${SSL_EAY_RELEASE} debug ${SSL_EAY_DEBUG}
        optimized ${LIB_EAY_RELEASE} debug ${LIB_EAY_DEBUG}
        )
    else()
      set( OPENSSL_LIBRARIES ${SSL_EAY_RELEASE} ${LIB_EAY_RELEASE} )
    endif()
    MARK_AS_ADVANCED(SSL_EAY_DEBUG SSL_EAY_RELEASE)
    MARK_AS_ADVANCED(LIB_EAY_DEBUG LIB_EAY_RELEASE)
  ELSEIF(MINGW)
    # same player, for MingW
    FIND_LIBRARY(LIB_EAY NAMES libeay32
      PATHS ${OPENSSL_ROOT_DIR}/lib/MinGW
      )
    FIND_LIBRARY(SSL_EAY NAMES ssleay32
      PATHS ${OPENSSL_ROOT_DIR}/lib/MinGW
      )
    MARK_AS_ADVANCED(SSL_EAY LIB_EAY)
    set( OPENSSL_LIBRARIES ${SSL_EAY} ${LIB_EAY} )
  ELSE(MSVC)
    # Not sure what to pick for -say- intel, let's use the toplevel ones and hope someone report issues:
    FIND_LIBRARY(LIB_EAY NAMES libeay32
      ${OPENSSL_ROOT_DIR}/lib
      )
    FIND_LIBRARY(SSL_EAY NAMES ssleay32
      ${OPENSSL_ROOT_DIR}/lib
      )
    MARK_AS_ADVANCED(SSL_EAY LIB_EAY)
    set( OPENSSL_LIBRARIES ${SSL_EAY} ${LIB_EAY} )
  ENDIF(MSVC)
ELSE(WIN32 AND NOT CYGWIN)

  FIND_LIBRARY(OPENSSL_SSL_LIBRARIES NAMES ssl ssleay32 ssleay32MD)
  FIND_LIBRARY(OPENSSL_CRYPTO_LIBRARIES NAMES crypto)
  MARK_AS_ADVANCED(OPENSSL_CRYPTO_LIBRARIES OPENSSL_SSL_LIBRARIES)

  SET(OPENSSL_LIBRARIES ${OPENSSL_SSL_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARIES})

ENDIF(WIN32 AND NOT CYGWIN)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenSSL DEFAULT_MSG
  OPENSSL_LIBRARIES 
  OPENSSL_INCLUDE_DIR
)

MARK_AS_ADVANCED(OPENSSL_INCLUDE_DIR OPENSSL_LIBRARIES)

