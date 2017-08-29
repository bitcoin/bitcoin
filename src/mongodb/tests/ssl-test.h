#include <mongoc.h>

#include <mongoc-thread-private.h>

typedef enum ssl_test_behavior {
   SSL_TEST_BEHAVIOR_NORMAL,
   SSL_TEST_BEHAVIOR_HANGUP_AFTER_HANDSHAKE,
   SSL_TEST_BEHAVIOR_STALL_BEFORE_HANDSHAKE,
} ssl_test_behavior_t;

typedef enum ssl_test_state {
   SSL_TEST_CRASH,
   SSL_TEST_SUCCESS,
   SSL_TEST_SSL_INIT,
   SSL_TEST_SSL_HANDSHAKE,
   SSL_TEST_TIMEOUT,
} ssl_test_state_t;

typedef struct ssl_test_result {
   ssl_test_state_t result;
   int err;
   unsigned long ssl_err;
} ssl_test_result_t;

typedef struct ssl_test_data {
   mongoc_ssl_opt_t *client;
   mongoc_ssl_opt_t *server;
   ssl_test_behavior_t behavior;
   int64_t handshake_stall_ms;
   const char *host;
   unsigned short server_port;
   mongoc_cond_t cond;
   mongoc_mutex_t cond_mutex;
   ssl_test_result_t *client_result;
   ssl_test_result_t *server_result;
} ssl_test_data_t;

void
ssl_test (mongoc_ssl_opt_t *client,
          mongoc_ssl_opt_t *server,
          const char *host,
          ssl_test_result_t *client_result,
          ssl_test_result_t *server_result);
