#include <mongoc.h>

#include "mongoc-async-private.h"
#include "mongoc-async-cmd-private.h"
#include "TestSuite.h"
#include "mock_server/mock-server.h"
#include "mongoc-errno-private.h"

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "async-test"

#define TIMEOUT 10000 /* milliseconds */
#define NSERVERS 10

struct result {
   int32_t max_wire_version;
   bool finished;
};


static void
test_ismaster_helper (mongoc_async_cmd_result_t result,
                      const bson_t *bson,
                      int64_t rtt_msec,
                      void *data,
                      bson_error_t *error)
{
   struct result *r = (struct result *) data;
   bson_iter_t iter;

   if (result != MONGOC_ASYNC_CMD_SUCCESS) {
      fprintf (stderr, "error: %s\n", error->message);
   }
   ASSERT_CMPINT (result, ==, MONGOC_ASYNC_CMD_SUCCESS);

   BSON_ASSERT (bson_iter_init_find (&iter, bson, "maxWireVersion"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT32 (&iter));
   r->max_wire_version = bson_iter_int32 (&iter);
   r->finished = true;
}


static void
test_ismaster_impl (bool with_ssl)
{
   mock_server_t *servers[NSERVERS];
   mongoc_async_t *async;
   mongoc_stream_t *sock_streams[NSERVERS];
   mongoc_socket_t *conn_sock;
   mongoc_async_cmd_setup_t setup = NULL;
   void *setup_ctx = NULL;
   struct sockaddr_in server_addr = {0};
   uint16_t ports[NSERVERS];
   struct result results[NSERVERS];
   int r;
   int i;
   int errcode;
   bson_t q = BSON_INITIALIZER;

#ifdef MONGOC_ENABLE_SSL
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
#endif

   if (!TestSuite_CheckMockServerAllowed ()) {
      return;
   }

   BSON_ASSERT (bson_append_int32 (&q, "isMaster", 8, 1));

   for (i = 0; i < NSERVERS; i++) {
      /* use max wire versions just to distinguish among responses */
      servers[i] = mock_server_with_autoismaster (i);

#ifdef MONGOC_ENABLE_SSL
      if (with_ssl) {
         sopt.weak_cert_validation = true;
         sopt.pem_file = CERT_SERVER;
         sopt.ca_file = CERT_CA;

         mock_server_set_ssl_opts (servers[i], &sopt);
      }
#endif

      ports[i] = mock_server_run (servers[i]);
   }

   async = mongoc_async_new ();

   for (i = 0; i < NSERVERS; i++) {
      conn_sock = mongoc_socket_new (AF_INET, SOCK_STREAM, 0);
      BSON_ASSERT (conn_sock);

      server_addr.sin_family = AF_INET;
      server_addr.sin_port = htons (ports[i]);
      server_addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
      r = mongoc_socket_connect (
         conn_sock, (struct sockaddr *) &server_addr, sizeof (server_addr), 0);

      errcode = mongoc_socket_errno (conn_sock);
      if (!(r == 0 || MONGOC_ERRNO_IS_AGAIN (errcode))) {
         fprintf (stderr,
                  "mongoc_socket_connect unexpected return: "
                  "%d (errno: %d)\n",
                  r,
                  errcode);
         fflush (stderr);
         abort ();
      }

      sock_streams[i] = mongoc_stream_socket_new (conn_sock);

#ifdef MONGOC_ENABLE_SSL
      if (with_ssl) {
         copt.ca_file = CERT_CA;
         copt.weak_cert_validation = 1;

         sock_streams[i] = mongoc_stream_tls_new_with_hostname (
            sock_streams[i], NULL, &copt, 1);
         setup = mongoc_async_cmd_tls_setup;
         setup_ctx = (void *) "127.0.0.1";
      }
#endif

      results[i].finished = false;

      mongoc_async_cmd_new (async,
                            sock_streams[i],
                            setup,
                            setup_ctx,
                            "admin",
                            &q,
                            &test_ismaster_helper,
                            (void *) &results[i],
                            TIMEOUT);
   }

   mongoc_async_run (async);

   for (i = 0; i < NSERVERS; i++) {
      if (!results[i].finished) {
         fprintf (stderr, "command %d not finished\n", i);
         abort ();
      }

      /* received the maxWireVersion configured above */
      ASSERT_CMPINT (i, ==, results[i].max_wire_version);
   }

   mongoc_async_destroy (async);

   bson_destroy (&q);

   for (i = 0; i < NSERVERS; i++) {
      mock_server_destroy (servers[i]);
      mongoc_stream_destroy (sock_streams[i]);
   }
}


static void
test_ismaster (void)
{
   test_ismaster_impl (false);
}


#if defined(MONGOC_ENABLE_SSL_OPENSSL)
static void
test_ismaster_ssl (void)
{
   test_ismaster_impl (true);
}
#endif


void
test_async_install (TestSuite *suite)
{
   TestSuite_AddMockServerTest (suite, "/Async/ismaster", test_ismaster);
#if defined(MONGOC_ENABLE_SSL_OPENSSL)
   TestSuite_AddMockServerTest (
      suite, "/Async/ismaster_ssl", test_ismaster_ssl);
#endif
}
