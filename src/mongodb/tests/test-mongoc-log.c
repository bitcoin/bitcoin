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

#include <mongoc.h>

#include "mongoc-log-private.h"
#include "mongoc-trace-private.h"
#include "TestSuite.h"


struct log_state {
   mongoc_log_func_t handler;
   void *data;
   bool trace_enabled;
};


static void
save_state (struct log_state *state)
{
   _mongoc_log_get_handler (&state->handler, &state->data);
   state->trace_enabled = _mongoc_log_trace_is_enabled ();
}


static void
restore_state (const struct log_state *state)
{
   mongoc_log_set_handler (state->handler, state->data);

   if (state->trace_enabled) {
      mongoc_log_trace_enable ();
   } else {
      mongoc_log_trace_disable ();
   }
}


struct log_func_data {
   mongoc_log_level_t log_level;
   char *log_domain;
   char *message;
};


void
log_func (mongoc_log_level_t log_level,
          const char *log_domain,
          const char *message,
          void *user_data)
{
   struct log_func_data *data = (struct log_func_data *) user_data;

   data->log_level = log_level;
   data->log_domain = bson_strdup (log_domain);
   data->message = bson_strdup (message);
}


static void
test_mongoc_log_handler (void)
{
   struct log_state old_state;
   struct log_func_data data;

   save_state (&old_state);
   mongoc_log_set_handler (log_func, &data);

#pragma push_macro("MONGOC_LOG_DOMAIN")
#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "my-custom-domain"

   MONGOC_WARNING ("warning!");

#pragma pop_macro("MONGOC_LOG_DOMAIN")

   ASSERT_CMPINT (data.log_level, ==, MONGOC_LOG_LEVEL_WARNING);
   ASSERT_CMPSTR (data.log_domain, "my-custom-domain");
   ASSERT_CMPSTR (data.message, "warning!");

   restore_state (&old_state);

   bson_free (data.log_domain);
   bson_free (data.message);
}


static void
test_mongoc_log_null (void)
{
   struct log_state old_state;

   save_state (&old_state);
   mongoc_log_set_handler (NULL, NULL);

   /* doesn't seg fault */
   MONGOC_ERROR ("error!");
   MONGOC_DEBUG ("debug!");

   restore_state (&old_state);
}

static int
should_run_trace_tests (void)
{
#ifdef MONGOC_TRACE
   return 1;
#else
   return 0;
#endif
}
static int
should_not_run_trace_tests (void)
{
   return !should_run_trace_tests ();
}

static void
test_mongoc_log_trace_enabled (void *context)
{
   struct log_state old_state;
   struct log_func_data data;

   save_state (&old_state);
   mongoc_log_set_handler (log_func, &data);

   mongoc_log_trace_enable ();
   TRACE ("%s", "Conscript reporting!");
   ASSERT_CMPINT (data.log_level, ==, MONGOC_LOG_LEVEL_TRACE);
   ASSERT_CONTAINS (data.message, " Conscript reporting!");
   bson_free (data.log_domain);
   bson_free (data.message);

   TRACE ("%s", "Awaiting orders");
   ASSERT_CMPINT (data.log_level, ==, MONGOC_LOG_LEVEL_TRACE);
   ASSERT_CONTAINS (data.message, "Awaiting orders");

   mongoc_log_trace_disable ();
   TRACE ("%s", "For the Union");
   ASSERT_CMPINT (data.log_level, ==, MONGOC_LOG_LEVEL_TRACE);
   ASSERT_CONTAINS (data.message, "Awaiting orders");
   bson_free (data.log_domain);
   bson_free (data.message);


   mongoc_log_trace_enable ();
   TRACE ("%s", "For home country");
   ASSERT_CMPINT (data.log_level, ==, MONGOC_LOG_LEVEL_TRACE);
   ASSERT_CONTAINS (data.message, "For home country");

   restore_state (&old_state);

   bson_free (data.log_domain);
   bson_free (data.message);
}

static void
test_mongoc_log_trace_disabled (void *context)
{
   struct log_state old_state;
   struct log_func_data data = {(mongoc_log_level_t) -1, 0, NULL};

   save_state (&old_state);
   mongoc_log_set_handler (log_func, &data);

   TRACE ("%s", "Conscript reporting!");
   ASSERT_CMPINT (data.log_level, ==, (mongoc_log_level_t) -1);

   restore_state (&old_state);
}

void
test_log_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/Log/basic", test_mongoc_log_handler);
   TestSuite_AddFull (suite,
                      "/Log/trace/enabled",
                      test_mongoc_log_trace_enabled,
                      NULL,
                      NULL,
                      should_run_trace_tests);
   TestSuite_AddFull (suite,
                      "/Log/trace/disabled",
                      test_mongoc_log_trace_disabled,
                      NULL,
                      NULL,
                      should_not_run_trace_tests);
   TestSuite_Add (suite, "/Log/null", test_mongoc_log_null);
}
