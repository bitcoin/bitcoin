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

#ifndef MOCK_SERVER_H
#define MOCK_SERVER_H

#include <bson.h>

#include "mongoc-uri.h"

#ifdef MONGOC_ENABLE_SSL
#include "mongoc-ssl.h"
#endif

#include "request.h"

typedef struct _mock_server_t mock_server_t;
typedef struct _autoresponder_handle_t autoresponder_handle_t;

typedef bool (*autoresponder_t) (request_t *request, void *data);

typedef void (*destructor_t) (void *data);

mock_server_t *
mock_server_new ();

mock_server_t *
mock_server_with_autoismaster (int32_t max_wire_version);

mock_server_t *
mock_mongos_new (int32_t max_wire_version);

mock_server_t *
mock_server_down (void);

int
mock_server_autoresponds (mock_server_t *server,
                          autoresponder_t responder,
                          void *data,
                          destructor_t destructor);

void
mock_server_remove_autoresponder (mock_server_t *server, int id);

int
mock_server_auto_ismaster (mock_server_t *server,
                           const char *response_json,
                           ...);

#ifdef MONGOC_ENABLE_SSL

void
mock_server_set_ssl_opts (mock_server_t *server, mongoc_ssl_opt_t *opts);

#endif

uint16_t
mock_server_run (mock_server_t *server);

const mongoc_uri_t *
mock_server_get_uri (mock_server_t *server);

const char *
mock_server_get_host_and_port (mock_server_t *server);

uint16_t
mock_server_get_port (mock_server_t *server);

int64_t
mock_server_get_request_timeout_msec (mock_server_t *server);

void
mock_server_set_request_timeout_msec (mock_server_t *server,
                                      int64_t request_timeout_msec);

bool
mock_server_get_rand_delay (mock_server_t *server);

void
mock_server_set_rand_delay (mock_server_t *server, bool rand_delay);

double
mock_server_get_uptime_sec (mock_server_t *server);

request_t *
mock_server_receives_request (mock_server_t *server);

request_t *
mock_server_receives_command (mock_server_t *server,
                              const char *database_name,
                              mongoc_query_flags_t flags,
                              const char *command_json,
                              ...);

request_t *
mock_server_receives_ismaster (mock_server_t *server);

request_t *
mock_server_receives_gle (mock_server_t *server, const char *database_name);

request_t *
mock_server_receives_query (mock_server_t *server,
                            const char *ns,
                            mongoc_query_flags_t flags,
                            uint32_t skip,
                            int32_t n_return,
                            const char *query_json,
                            const char *fields_json);

request_t *
mock_server_receives_insert (mock_server_t *server,
                             const char *ns,
                             mongoc_insert_flags_t flags,
                             const char *doc_json);

request_t *
mock_server_receives_bulk_insert (mock_server_t *server,
                                  const char *ns,
                                  mongoc_insert_flags_t flags,
                                  int n);

request_t *
mock_server_receives_update (mock_server_t *server,
                             const char *ns,
                             mongoc_update_flags_t flags,
                             const char *selector_json,
                             const char *update_json);

request_t *
mock_server_receives_delete (mock_server_t *server,
                             const char *ns,
                             mongoc_remove_flags_t flags,
                             const char *selector_json);

request_t *
mock_server_receives_getmore (mock_server_t *server,
                              const char *ns,
                              int32_t n_return,
                              int64_t cursor_id);

request_t *
mock_server_receives_kill_cursors (mock_server_t *server, int64_t cursor_id);

void
mock_server_hangs_up (request_t *request);

void
mock_server_resets (request_t *request);

void
mock_server_replies (request_t *request,
                     mongoc_reply_flags_t flags,
                     int64_t cursor_id,
                     int32_t starting_from,
                     int32_t number_returned,
                     const char *docs_json);

void
mock_server_replies_simple (request_t *request, const char *docs_json);

void
mock_server_replies_ok_and_destroys (request_t *request);

void
mock_server_replies_to_find (request_t *request,
                             mongoc_query_flags_t flags,
                             int64_t cursor_id,
                             int32_t number_returned,
                             const char *ns,
                             const char *reply_json,
                             bool is_command);

void
mock_server_reply_multi (request_t *request,
                         mongoc_reply_flags_t flags,
                         const bson_t *docs,
                         int n_docs,
                         int64_t cursor_id);

void
mock_server_destroy (mock_server_t *server);

#endif /* MOCK_SERVER_H */
