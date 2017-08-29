#include <mongoc.h>
#include <mongoc-thread-private.h>
#include <mongoc-util-private.h>
#include <mongoc-stream-tls.h>

#ifdef MONGOC_ENABLE_SSL_OPENSSL
#include <openssl/err.h>
#endif

#include "ssl-test.h"
#include "TestSuite.h"

#define TIMEOUT 10000 /* milliseconds */

#if !defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL) && \
   !defined(MONGOC_ENABLE_SSL_LIBRESSL)
/** run as a child thread by test_mongoc_tls_hangup
 *
 * It:
 *    1. spins up
 *    2. binds and listens to a random port
 *    3. notifies the client of its port through a condvar
 *    4. accepts a request
 *    5. reads a byte
 *    7. hangs up
 */
static void *
ssl_error_server (void *ptr)
{
   ssl_test_data_t *data = (ssl_test_data_t *) ptr;

   mongoc_stream_t *sock_stream;
   mongoc_stream_t *ssl_stream;
   mongoc_socket_t *listen_sock;
   mongoc_socket_t *conn_sock;
   mongoc_socklen_t sock_len;
   char buf;
   ssize_t r;
   mongoc_iovec_t iov;
   struct sockaddr_in server_addr = {0};
   bson_error_t error;

   iov.iov_base = &buf;
   iov.iov_len = 1;

   listen_sock = mongoc_socket_new (AF_INET, SOCK_STREAM, 0);
   BSON_ASSERT (listen_sock);

   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
   server_addr.sin_port = htons (0);

   r = mongoc_socket_bind (
      listen_sock, (struct sockaddr *) &server_addr, sizeof server_addr);
   BSON_ASSERT (r == 0);

   sock_len = sizeof (server_addr);
   r = mongoc_socket_getsockname (
      listen_sock, (struct sockaddr *) &server_addr, &sock_len);
   BSON_ASSERT (r == 0);

   r = mongoc_socket_listen (listen_sock, 10);
   BSON_ASSERT (r == 0);

   mongoc_mutex_lock (&data->cond_mutex);
   data->server_port = ntohs (server_addr.sin_port);
   mongoc_cond_signal (&data->cond);
   mongoc_mutex_unlock (&data->cond_mutex);

   conn_sock = mongoc_socket_accept (listen_sock, -1);
   BSON_ASSERT (conn_sock);

   sock_stream = mongoc_stream_socket_new (conn_sock);
   BSON_ASSERT (sock_stream);

   ssl_stream = mongoc_stream_tls_new_with_hostname (
      sock_stream, data->host, data->server, 0);
   BSON_ASSERT (ssl_stream);

   switch (data->behavior) {
   case SSL_TEST_BEHAVIOR_STALL_BEFORE_HANDSHAKE:
      _mongoc_usleep (data->handshake_stall_ms * 1000);
      break;
   case SSL_TEST_BEHAVIOR_HANGUP_AFTER_HANDSHAKE:
      r = mongoc_stream_tls_handshake_block (
         ssl_stream, data->host, TIMEOUT, &error);
      BSON_ASSERT (r);

      r = mongoc_stream_readv (ssl_stream, &iov, 1, 1, TIMEOUT);
      BSON_ASSERT (r == 1);
      break;
   case SSL_TEST_BEHAVIOR_NORMAL:
   default:
      fprintf (stderr, "unimplemented ssl_test_behavior_t\n");
      abort ();
   }

   data->server_result->result = SSL_TEST_SUCCESS;

   mongoc_stream_close (ssl_stream);
   mongoc_stream_destroy (ssl_stream);
   mongoc_socket_destroy (listen_sock);

   return NULL;
}


#if !defined(__sun) && !defined(__APPLE__)
/** run as a child thread by test_mongoc_tls_hangup
 *
 * It:
 *    1. spins up
 *    2. waits on a condvar until the server is up
 *    3. connects to the server's port
 *    4. writes a byte
 *    5. confirms that the server hangs up promptly
 *    6. shuts down
 */
static void *
ssl_hangup_client (void *ptr)
{
   ssl_test_data_t *data = (ssl_test_data_t *) ptr;
   mongoc_stream_t *sock_stream;
   mongoc_stream_t *ssl_stream;
   mongoc_socket_t *conn_sock;
   char buf = 'b';
   ssize_t r;
   mongoc_iovec_t riov;
   mongoc_iovec_t wiov;
   struct sockaddr_in server_addr = {0};
   int64_t start_time;
   bson_error_t error;

   conn_sock = mongoc_socket_new (AF_INET, SOCK_STREAM, 0);
   BSON_ASSERT (conn_sock);

   mongoc_mutex_lock (&data->cond_mutex);
   while (!data->server_port) {
      mongoc_cond_wait (&data->cond, &data->cond_mutex);
   }
   mongoc_mutex_unlock (&data->cond_mutex);

   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons (data->server_port);
   server_addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

   r = mongoc_socket_connect (
      conn_sock, (struct sockaddr *) &server_addr, sizeof (server_addr), -1);
   BSON_ASSERT (r == 0);

   sock_stream = mongoc_stream_socket_new (conn_sock);
   BSON_ASSERT (sock_stream);

   ssl_stream = mongoc_stream_tls_new_with_hostname (
      sock_stream, data->host, data->client, 1);
   BSON_ASSERT (ssl_stream);

   r = mongoc_stream_tls_handshake_block (
      ssl_stream, data->host, TIMEOUT, &error);
   BSON_ASSERT (r);

   wiov.iov_base = (void *) &buf;
   wiov.iov_len = 1;
   r = mongoc_stream_writev (ssl_stream, &wiov, 1, TIMEOUT);
   BSON_ASSERT (r == 1);

   riov.iov_base = (void *) &buf;
   riov.iov_len = 1;

   /* we should notice promptly that the server hangs up */
   start_time = bson_get_monotonic_time ();
   r = mongoc_stream_readv (ssl_stream, &riov, 1, 1, TIMEOUT);
   /* time is in microseconds */
   BSON_ASSERT (bson_get_monotonic_time () - start_time < 1000 * 1000);
   BSON_ASSERT (r == -1);
   mongoc_stream_destroy (ssl_stream);
   data->client_result->result = SSL_TEST_SUCCESS;
   return NULL;
}

static void
test_mongoc_tls_hangup (void)
{
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
   ssl_test_result_t sr;
   ssl_test_result_t cr;
   ssl_test_data_t data = {0};
   mongoc_thread_t threads[2];
   int i, r;

   sopt.pem_file = CERT_SERVER;
   sopt.weak_cert_validation = 1;
   copt.weak_cert_validation = 1;

   data.server = &sopt;
   data.client = &copt;
   data.behavior = SSL_TEST_BEHAVIOR_HANGUP_AFTER_HANDSHAKE;
   data.server_result = &sr;
   data.client_result = &cr;
   data.host = "localhost";

   mongoc_mutex_init (&data.cond_mutex);
   mongoc_cond_init (&data.cond);

   r = mongoc_thread_create (threads, &ssl_error_server, &data);
   BSON_ASSERT (r == 0);

   r = mongoc_thread_create (threads + 1, &ssl_hangup_client, &data);
   BSON_ASSERT (r == 0);

   for (i = 0; i < 2; i++) {
      r = mongoc_thread_join (threads[i]);
      BSON_ASSERT (r == 0);
   }

   mongoc_mutex_destroy (&data.cond_mutex);
   mongoc_cond_destroy (&data.cond);

   ASSERT (cr.result == SSL_TEST_SUCCESS);
   ASSERT (sr.result == SSL_TEST_SUCCESS);
}
#endif


/** run as a child thread by test_mongoc_tls_handshake_stall
 *
 * It:
 *    1. spins up
 *    2. waits on a condvar until the server is up
 *    3. connects to the server's port
 *    4. attempts handshake
 *    5. confirms that it times out
 *    6. shuts down
 */
static void *
handshake_stall_client (void *ptr)
{
   ssl_test_data_t *data = (ssl_test_data_t *) ptr;
   char *uri_str;
   mongoc_client_t *client;
   bson_t reply;
   bson_error_t error;
   int64_t connect_timeout_ms = data->handshake_stall_ms - 100;
   int64_t duration_ms;

   int64_t start_time;

   mongoc_mutex_lock (&data->cond_mutex);
   while (!data->server_port) {
      mongoc_cond_wait (&data->cond, &data->cond_mutex);
   }
   mongoc_mutex_unlock (&data->cond_mutex);

   uri_str = bson_strdup_printf ("mongodb://localhost:%u/"
                                 "?ssl=true&serverselectiontimeoutms=200&"
                                 "connecttimeoutms=%" PRId64,
                                 data->server_port,
                                 connect_timeout_ms);

   client = mongoc_client_new (uri_str);

   /* we should time out after about 200ms */
   start_time = bson_get_monotonic_time ();
   mongoc_client_get_server_status (client, NULL, &reply, &error);

   /* time is in microseconds */
   duration_ms = (bson_get_monotonic_time () - start_time) / 1000;

   if (llabs (duration_ms - connect_timeout_ms) > 100) {
      fprintf (stderr,
               "expected timeout after about 200ms, not %" PRId64 "\n",
               duration_ms);
      abort ();
   }

   data->client_result->result = SSL_TEST_SUCCESS;

   bson_destroy (&reply);
   mongoc_client_destroy (client);
   bson_free (uri_str);

   return NULL;
}


/* CDRIVER-2222 this should be reenabled for Apple Secure Channel too */
#if !defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
static void
test_mongoc_tls_handshake_stall (void)
{
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
   ssl_test_result_t sr;
   ssl_test_result_t cr;
   ssl_test_data_t data = {0};
   mongoc_thread_t threads[2];
   int i, r;

   sopt.pem_file = CERT_SERVER;
   sopt.weak_cert_validation = 1;
   copt.weak_cert_validation = 1;

   data.server = &sopt;
   data.client = &copt;
   data.behavior = SSL_TEST_BEHAVIOR_STALL_BEFORE_HANDSHAKE;
   data.handshake_stall_ms = 300;
   data.server_result = &sr;
   data.client_result = &cr;
   data.host = "localhost";

   mongoc_mutex_init (&data.cond_mutex);
   mongoc_cond_init (&data.cond);

   r = mongoc_thread_create (threads, &ssl_error_server, &data);
   BSON_ASSERT (r == 0);

   r = mongoc_thread_create (threads + 1, &handshake_stall_client, &data);
   BSON_ASSERT (r == 0);

   for (i = 0; i < 2; i++) {
      r = mongoc_thread_join (threads[i]);
      BSON_ASSERT (r == 0);
   }

   mongoc_mutex_destroy (&data.cond_mutex);
   mongoc_cond_destroy (&data.cond);

   ASSERT (cr.result == SSL_TEST_SUCCESS);
   ASSERT (sr.result == SSL_TEST_SUCCESS);
}

#endif /* !MONGOC_ENABLE_SSL_SECURE_CHANNEL */
#endif /* !MONGOC_ENABLE_SSL_SECURE_CHANNEL && !MONGOC_ENABLE_SSL_LIBRESSL */

void
test_stream_tls_error_install (TestSuite *suite)
{
/* TLS stream doesn't detect hangup promptly on Solaris for some reason */
#if !defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL) && \
   !defined(MONGOC_ENABLE_SSL_LIBRESSL)
#if !defined(__sun) && !defined(__APPLE__)
   TestSuite_Add (suite, "/TLS/hangup", test_mongoc_tls_hangup);
#endif

   /* CDRIVER-2222 this should be reenabled for Apple Secure Channel too */
#if !defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
   TestSuite_Add (
      suite, "/TLS/handshake_stall", test_mongoc_tls_handshake_stall);
#endif
#endif /* !MONGOC_ENABLE_SSL_SECURE_CHANNEL && !MONGOC_ENABLE_SSL_LIBRESSL */
}
