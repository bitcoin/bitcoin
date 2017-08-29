/*
 * Copyright 2015 MongoDB, Inc.
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

#ifndef JSON_TEST_H
#define JSON_TEST_H

#include "TestSuite.h"

#include <bson.h>
#include <mongoc.h>

#include "mongoc-server-description-private.h"
#include "mongoc-topology-description-private.h"

#include "test-conveniences.h"

#define MAX_NUM_TESTS 100

typedef void (*test_hook) (bson_t *test);

bson_t *
get_bson_from_json_file (char *filename);

int
collect_tests_from_dir (char (*paths)[MAX_TEST_NAME_LENGTH] /* OUT */,
                        const char *dir_path,
                        int paths_index,
                        int max_paths);

void
assemble_path (const char *parent_path,
               const char *child_name,
               char *dst /* OUT */);

void
install_json_test_suite (TestSuite *suite,
                         const char *dir_path,
                         test_hook callback);

mongoc_topology_description_type_t
topology_type_from_test (const char *type);

mongoc_server_description_t *
server_description_by_hostname (mongoc_topology_description_t *topology,
                                const char *address);

void
process_sdam_test_ismaster_responses (bson_t *phase,
                                      mongoc_topology_description_t *td);

void
check_json_apm_events (const bson_t *events, const bson_t *expectations);

void
test_server_selection_logic_cb (bson_t *test);

mongoc_server_description_type_t
server_type_from_test (const char *type);

#endif
