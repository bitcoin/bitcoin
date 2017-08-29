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


#ifndef MONGOC_LINUX_DISTRO_SCANNER_PRIVATE_H
#define MONGOC_LINUX_DISTRO_SCANNER_PRIVATE_H

#include "mongoc-handshake-os-private.h"

#ifdef MONGOC_OS_IS_LINUX

BSON_BEGIN_DECLS

bool
_mongoc_linux_distro_scanner_get_distro (char **name, char **version);

/* These functions are exposed so we can test them separately. */
void
_mongoc_linux_distro_scanner_read_key_value_file (const char *path,
                                                  const char *name_key,
                                                  ssize_t name_key_len,
                                                  char **name,
                                                  const char *version_key,
                                                  ssize_t version_key_len,
                                                  char **version);
void
_mongoc_linux_distro_scanner_read_generic_release_file (const char **paths,
                                                        char **name,
                                                        char **version);
void
_mongoc_linux_distro_scanner_split_line_by_release (const char *line,
                                                    ssize_t line_len,
                                                    char **name,
                                                    char **version);
BSON_END_DECLS

#endif

#endif
