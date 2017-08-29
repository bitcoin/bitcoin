/*
 * Copyright 2017 MongoDB, Inc.
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

#ifndef MONGOC_MACROS_H
#define MONGOC_MACROS_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

/* Decorate public functions:
 * - if MONGOC_STATIC, we're compiling a program that uses libmongoc as
 *   a static library, don't decorate functions
 * - else if MONGOC_COMPILATION, we're compiling a static or shared libmongoc,
 *   mark public functions for export from the shared lib (which has no effect
 *   on the static lib)
 * - else, we're compiling a program that uses libmongoc as a shared library,
 *   mark public functions as DLL imports for Microsoft Visual C.
 */

#ifdef _MSC_VER
/*
 * Microsoft Visual C
 */
#ifdef MONGOC_STATIC
#define MONGOC_API
#elif defined(MONGOC_COMPILATION)
#define MONGOC_API __declspec(dllexport)
#else
#define MONGOC_API __declspec(dllimport)
#endif
#define MONGOC_CALL __cdecl

#elif defined(__GNUC__)
/*
 * GCC
 */
#ifdef MONGOC_STATIC
#define MONGOC_API
#elif defined(MONGOC_COMPILATION)
#define MONGOC_API __attribute__ ((visibility ("default")))
#else
#define MONGOC_API
#endif
#define MONGOC_CALL

#else
/*
 * Other compilers
 */
#define MONGOC_API
#define MONGOC_CALL

#endif

#define MONGOC_EXPORT(type) MONGOC_API type MONGOC_CALL

#endif /* MONGOC_MACROS_H */
