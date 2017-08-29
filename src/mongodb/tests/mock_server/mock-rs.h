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

#ifndef MOCK_RS_H
#define MOCK_RS_H

#include "mongoc.h"

#include "mock-server.h"

typedef struct _mock_rs_t mock_rs_t;

mock_rs_t *
mock_rs_with_autoismaster (int32_t max_wire_version,
                           bool has_primary,
                           int n_secondaries,
                           int n_arbiters);

int64_t
mock_rs_get_request_timeout_msec (mock_rs_t *rs);

void
mock_rs_set_request_timeout_msec (mock_rs_t *rs, int64_t request_timeout_msec);

void
mock_rs_run (mock_rs_t *rs);

const mongoc_uri_t *
mock_rs_get_uri (mock_rs_t *rs);

request_t *
mock_rs_receives_request (mock_rs_t *rs);

request_t *
mock_rs_receives_query (mock_rs_t *rs,
                        const char *ns,
                        mongoc_query_flags_t flags,
                        uint32_t skip,
                        int32_t n_return,
                        const char *query_json,
                        const char *fields_json);

request_t *
mock_rs_receives_command (mock_rs_t *rs,
                          const char *database_name,
                          mongoc_query_flags_t flags,
                          const char *command_json,
                          ...);

request_t *
mock_rs_receives_insert (mock_rs_t *rs,
                         const char *ns,
                         mongoc_insert_flags_t flags,
                         const char *doc_json);

request_t *
mock_rs_receives_getmore (mock_rs_t *rs,
                          const char *ns,
                          int32_t n_return,
                          int64_t cursor_id);

request_t *
mock_rs_receives_kill_cursors (mock_rs_t *rs, int64_t cursor_id);

void
mock_rs_replies (request_t *request,
                 uint32_t flags,
                 int64_t cursor_id,
                 int32_t starting_from,
                 int32_t number_returned,
                 const char *docs_json);

void
mock_rs_replies_simple (request_t *request, const char *docs_json);

void
mock_rs_replies_to_find (request_t *request,
                         mongoc_query_flags_t flags,
                         int64_t cursor_id,
                         int32_t number_returned,
                         const char *ns,
                         const char *reply_json,
                         bool is_command);

void
mock_rs_hangs_up (request_t *request);


bool
mock_rs_request_is_to_primary (mock_rs_t *rs, request_t *request);

bool
mock_rs_request_is_to_secondary (mock_rs_t *rs, request_t *request);

void
mock_rs_destroy (mock_rs_t *rs);

#endif /* MOCK_RS_H */
