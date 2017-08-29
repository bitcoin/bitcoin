#include <bson.h>
#include <errno.h>

#include "mongoc-config.h"
#ifdef MONGOC_ENABLE_SSL_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#include "mongoc-stream-tls.h"

#include "ssl-test.h"
#include "TestSuite.h"

#define TIMEOUT 10 * 1000

#define NUM_IOVECS 2000

#define LOCALHOST "127.0.0.1"

/** this function is meant to be run from ssl_test as a child thread
 *
 * It:
 *    1. spins up
 *    2. binds and listens to a random port
 *    3. notifies the client of its port through a condvar
 *    4. accepts a request
 *    5. reads a 32 bit length
 *    6. reads a string of that length
 *    7. echoes it back to the client
 *    8. shuts down
 */
static void *
ssl_test_server (void *ptr)
{
   ssl_test_data_t *data = (ssl_test_data_t *) ptr;

   mongoc_stream_t *sock_stream;
   mongoc_stream_t *ssl_stream;
   mongoc_socket_t *listen_sock;
   mongoc_socket_t *conn_sock;
   mongoc_socklen_t sock_len;
   char buf[4 * NUM_IOVECS];
   ssize_t r;
   bson_error_t error;
   mongoc_iovec_t iov;
   struct sockaddr_in server_addr = {0};
   int len;

   iov.iov_base = buf;
   iov.iov_len = sizeof buf;

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
   ssl_stream =
      mongoc_stream_tls_new_with_hostname (sock_stream, NULL, data->server, 0);
   if (!ssl_stream) {
#ifdef MONGOC_ENABLE_SSL_OPENSSL
      unsigned long err = ERR_get_error ();
#else
      unsigned long err = 42;
#endif
      BSON_ASSERT (err);

      data->server_result->ssl_err = err;
      data->server_result->result = SSL_TEST_SSL_INIT;
#ifdef MONGOC_ENABLE_SSL_OPENSSL
      MONGOC_ERROR ("ERRORED (line: %d): %s\n",
                    __LINE__,
                    ERR_error_string (ERR_get_error (), NULL));
#endif
      mongoc_stream_destroy (sock_stream);
      mongoc_socket_destroy (listen_sock);

      return NULL;
   }
   BSON_ASSERT (ssl_stream);

   r = mongoc_stream_tls_handshake_block (
      ssl_stream, data->host, TIMEOUT, &error);

   if (!r) {
      unsigned long err = 43;

      MONGOC_ERROR ("ERRORED (line: %d): %s\n", __LINE__, error.message);
#ifdef MONGOC_ENABLE_SSL_OPENSSL
      MONGOC_ERROR ("msg: %s\n", ERR_error_string (ERR_get_error (), NULL));
#endif
      data->server_result->ssl_err = err;
      data->server_result->result = SSL_TEST_SSL_HANDSHAKE;

      mongoc_socket_destroy (listen_sock);
      mongoc_stream_destroy (ssl_stream);

      return NULL;
   }

   r = mongoc_stream_readv (ssl_stream, &iov, 1, 4, TIMEOUT);
   if (r < 0) {
      data->server_result->err = errno;
      data->server_result->result = SSL_TEST_TIMEOUT;
      MONGOC_ERROR ("ERRORED (line: %d): %s\n", __LINE__, error.message);

      mongoc_stream_destroy (ssl_stream);
      mongoc_socket_destroy (listen_sock);

      return NULL;
   }

   BSON_ASSERT (r == 4);
   memcpy (&len, iov.iov_base, r);

   r = mongoc_stream_readv (ssl_stream, &iov, 1, len, TIMEOUT);
   BSON_ASSERT (r == len);

   iov.iov_len = r;
   mongoc_stream_writev (ssl_stream, &iov, 1, TIMEOUT);

   mongoc_stream_destroy (ssl_stream);

   mongoc_socket_destroy (listen_sock);

   data->server_result->result = SSL_TEST_SUCCESS;

   return NULL;
}

/** this function is meant to be run from ssl_test as a child thread
 *
 * It:
 *    1. spins up
 *    2. waits on a condvar until the server is up
 *    3. connects to the server's port
 *    4. writes a 4 bytes length
 *    5. writes a string of length size
 *    6. reads a response back of the given length
 *    7. confirms that its the same as what was written
 *    8. shuts down
 */
static void *
ssl_test_client (void *ptr)
{
   ssl_test_data_t *data = (ssl_test_data_t *) ptr;
   mongoc_stream_t *sock_stream;
   mongoc_stream_t *ssl_stream;
   mongoc_socket_t *conn_sock;
   int i;
   char buf[1024];
   ssize_t r;
   mongoc_iovec_t riov;
   mongoc_iovec_t wiov;
   mongoc_iovec_t wiov_many[NUM_IOVECS];
   struct sockaddr_in server_addr = {0};
   int len;
   bson_error_t error;

   riov.iov_base = buf;
   riov.iov_len = sizeof buf;

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
   if (r != 0) {
      fprintf (stderr,
               "mongoc_socket_connect returned %zd: \"%s\"",
               r,
               strerror (errno));
      abort ();
   }

   sock_stream = mongoc_stream_socket_new (conn_sock);
   BSON_ASSERT (sock_stream);
   ssl_stream = mongoc_stream_tls_new_with_hostname (
      sock_stream, data->host, data->client, 1);
   if (!ssl_stream) {
#ifdef MONGOC_ENABLE_SSL_OPENSSL
      unsigned long err = ERR_get_error ();
#else
      unsigned long err = 44;
#endif
      BSON_ASSERT (err);

      data->client_result->ssl_err = err;
      data->client_result->result = SSL_TEST_SSL_INIT;
      MONGOC_ERROR ("ERRORED (line: %d): %s\n", __LINE__, error.message);

      mongoc_stream_destroy (sock_stream);

      return NULL;
   }
   BSON_ASSERT (ssl_stream);

   r = mongoc_stream_tls_handshake_block (
      ssl_stream, data->host, TIMEOUT, &error);

   if (!r) {
      unsigned long err = 45;

      data->client_result->ssl_err = err;
      data->client_result->result = SSL_TEST_SSL_HANDSHAKE;
      MONGOC_ERROR ("ERRORED (line: %d): %s\n", __LINE__, error.message);

      mongoc_stream_destroy (ssl_stream);
      return NULL;
   }

   len = 4 * NUM_IOVECS;

   wiov.iov_base = (void *) &len;
   wiov.iov_len = 4;
   r = mongoc_stream_writev (ssl_stream, &wiov, 1, TIMEOUT);

   BSON_ASSERT (r == wiov.iov_len);

   for (i = 0; i < NUM_IOVECS; i++) {
      wiov_many[i].iov_base = (void *) "foo";
      wiov_many[i].iov_len = 4;
   }

   r = mongoc_stream_writev (ssl_stream, wiov_many, NUM_IOVECS, TIMEOUT);
   BSON_ASSERT (r == wiov_many[0].iov_len * NUM_IOVECS);

   riov.iov_len = 1;

   r = mongoc_stream_readv (ssl_stream, &riov, 1, 1, TIMEOUT);
   BSON_ASSERT (r == 1);
   BSON_ASSERT (memcmp (riov.iov_base, "f", 1) == 0);

   riov.iov_len = 3;

   r = mongoc_stream_readv (ssl_stream, &riov, 1, 3, TIMEOUT);
   BSON_ASSERT (r == 3);
   BSON_ASSERT (memcmp (riov.iov_base, "oo", 3) == 0);

   mongoc_stream_destroy (ssl_stream);

   data->client_result->result = SSL_TEST_SUCCESS;

   return NULL;
}


/** This is the testing function for the ssl-test lib
 *
 * The basic idea is that you spin up a client and server, which will
 * communicate over a mongoc-stream-tls, with varying mongoc_ssl_opt's.  The
 * client and server speak a simple echo protocol, so all we're really testing
 * here is that any given configuration succeeds or fails as it should
 */
void
ssl_test (mongoc_ssl_opt_t *client,
          mongoc_ssl_opt_t *server,
          const char *host,
          ssl_test_result_t *client_result,
          ssl_test_result_t *server_result)
{
   ssl_test_data_t data = {0};
   mongoc_thread_t threads[2];
   int i, r;

   data.server = server;
   data.client = client;
   data.client_result = client_result;
   data.server_result = server_result;
   data.host = host;

   mongoc_mutex_init (&data.cond_mutex);
   mongoc_cond_init (&data.cond);

   r = mongoc_thread_create (threads, &ssl_test_server, &data);
   BSON_ASSERT (r == 0);

   r = mongoc_thread_create (threads + 1, &ssl_test_client, &data);
   BSON_ASSERT (r == 0);

   for (i = 0; i < 2; i++) {
      r = mongoc_thread_join (threads[i]);
      BSON_ASSERT (r == 0);
   }

   mongoc_mutex_destroy (&data.cond_mutex);
   mongoc_cond_destroy (&data.cond);
}
