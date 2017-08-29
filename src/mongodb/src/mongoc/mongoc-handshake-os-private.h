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

#ifndef MONGOC_HANDSHAKE_OS_PRIVATE
#define MONGOC_HANDSHAKE_OS_PRIVATE

/* Based on tables from
 * http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system
 */

#if defined(_WIN32) || defined(__CYGWIN__)
#define MONGOC_OS_TYPE "Windows"
#if defined(__CYGWIN__)
#define MONGOC_OS_NAME "Cygwin"
#else
#define MONGOC_OS_NAME "Windows"
#endif

/* osx and iphone defines __APPLE__ and __MACH__, but not __unix__ */
#elif defined(__APPLE__) && defined(__MACH__) && !defined(__unix__)
#define MONGOC_OS_TYPE "Darwin"
#include <TargetConditionals.h>
#if defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR == 1
#define MONGOC_OS_NAME "iOS Simulator"
#elif defined(TARGET_OS_IOS) && TARGET_OS_IOS == 1
#define MONGOC_OS_NAME "iOS"
#elif defined(TARGET_OS_MAC) && TARGET_OS_MAC == 1
#define MONGOC_OS_NAME "macOS"
#elif defined(TARGET_OS_TV) && TARGET_OS_TV == 1
#define MONGOC_OS_NAME "tvOS"
#elif defined(TARGET_OS_WATCH) && TARGET_OS_WATCH == 1
#define MONGOC_OS_NAME "watchOS"
#else
/*     Fall back to uname () */
#endif

/* Need to check if __unix is defined since sun and hpux always have __unix,
 * but not necessarily __unix__ defined. */
#elif defined(__unix__) || defined(__unix)
#include <sys/param.h>
#if defined(__linux__)
#define MONGOC_OS_IS_LINUX
#if defined(__ANDROID__)
#define MONGOC_OS_TYPE "Linux (Android)"
#else
#define MONGOC_OS_TYPE "Linux"
#endif
/*     Don't define OS_NAME. We'll scan the file system to determine distro. */
#elif defined(BSD)
#define MONGOC_OS_TYPE "BSD"
#if defined(__FreeBSD__)
#define MONGOC_OS_NAME "FreeBSD"
#elif defined(__NetBSD__)
#define MONGOC_OS_NAME "NetBSD"
#elif defined(__OpenBSD__)
#define MONGOC_OS_NAME "OpenBSD"
#elif defined(__DragonFly__)
#define MONGOC_OS_NAME "DragonFlyBSD"
#else
/*        Don't define OS_NAME. We'll use uname to figure it out. */
#endif
#else
#define MONGOC_OS_TYPE "Unix"
#if defined(_AIX)
#define MONGOC_OS_NAME "AIX"
#elif defined(__sun) && defined(__SVR4)
#define MONGOC_OS_NAME "Solaris"
#elif defined(__hpux)
#define MONGOC_OS_NAME "HP-UX"
#else
/*        Don't set OS name. We'll just fall back to uname. */
#endif
#endif

#endif

#endif
