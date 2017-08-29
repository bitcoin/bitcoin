/*
 * Copyright 2016 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef MONGOC_HANDSHAKE_COMPILER_PRIVATE_H
#define MONGOC_HANDSHAKE_COMPILER_PRIVATE_H

#include "mongoc-config.h"
#include "mongoc-util-private.h"

/*
 * Thanks to:
 * http://nadeausoftware.com/articles/2012/10/c_c_tip_how_detect_compiler_name_and_version_using_compiler_predefined_macros
 */

#if defined(__clang__)
#define MONGOC_COMPILER "clang"
#define MONGOC_COMPILER_VERSION __clang_version__
#elif defined(__ICC) || defined(__INTEL_COMPILER)
#define MONGOC_COMPILER "ICC"
#define MONGOC_COMPILER_VERSION __VERSION__
#elif defined(__GNUC__) || defined(__GNUG__)
#define MONGOC_COMPILER "GCC"
#define MONGOC_COMPILER_VERSION __VERSION__
#elif defined(__HP_cc) || defined(__HP_aCC)
#define MONGOC_COMPILER "aCC"
#define MONGOC_COMPILER_VERSION MONGOC_EVALUATE_STR (__HP_cc)
#elif defined(__IBMC__) || defined(__IBMCPP__)
#define MONGOC_COMPILER "xlc"
#define MONGOC_COMPILER_VERSION __xlc__
#elif defined(_MSC_VER)
#define MONGOC_COMPILER "MSVC"
#define MONGOC_COMPILER_VERSION MONGOC_EVALUATE_STR (_MSC_VER)
#elif defined(__PGI)
#define MONGOC_COMPILER "Portland PGCC"
#define MONGOC_COMPILER_VERSION                                     \
   MONGOC_EVALUATE_STR (__PGIC__)                                   \
   "." MONGOC_EVALUATE_STR (__PGIC_MINOR) "." MONGOC_EVALUATE_STR ( \
      __PGIC_PATCHLEVEL__)
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#define MONGOC_COMPILER "Solaris Studio"
#define MONGOC_COMPILER_VERSION MONGOC_EVALUATE_STR (__SUNPRO_C)
#elif defined(__PCC__)
/* Portable C Compiler. Version may not be available */
#define MONGOC_COMPILER "PCC"
#else
#define MONGOC_COMPILER MONGOC_EVALUATE_STR (MONGOC_CC)
/* Not defining COMPILER_VERSION. We'll fall back to values set at
 * configure-time */
#endif

#endif
