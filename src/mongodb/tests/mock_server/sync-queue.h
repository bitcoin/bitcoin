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

#include <stddef.h>

#ifndef SYNC_QUEUE_H
#define SYNC_QUEUE_H


typedef struct _sync_queue_t sync_queue_t;

sync_queue_t *
q_new ();

void
q_put (sync_queue_t *q, void *item);

void *
q_get (sync_queue_t *q, int64_t timeout_msec);

void *
q_get_nowait (sync_queue_t *q);

void
q_destroy (sync_queue_t *q);

#endif /* SYNC_QUEUE_H */
