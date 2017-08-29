#include <mongoc.h>


#ifdef MONGOC_ENABLE_SSL_OPENSSL
#include <openssl/err.h>
#endif

#include "ssl-test.h"
#include "TestSuite.h"
#include "test-libmongoc.h"

#if !defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL) && \
   !defined(MONGOC_ENABLE_SSL_LIBRESSL)

static void
test_mongoc_tls_no_certs (void)
{
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
   ssl_test_result_t sr;
   ssl_test_result_t cr;

   capture_logs (true);
   /* No server cert is not valid TLS at all */
   ssl_test (&copt, &sopt, "doesnt_matter", &cr, &sr);

   ASSERT_CMPINT (cr.result, !=, SSL_TEST_SUCCESS);
   ASSERT_CMPINT (sr.result, !=, SSL_TEST_SUCCESS);
}


#ifdef MONGOC_ENABLE_SSL_OPENSSL
static void
test_mongoc_tls_password (void)
{
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
   ssl_test_result_t sr;
   ssl_test_result_t cr;

   sopt.ca_file = CERT_CA;
   sopt.pem_file = CERT_SERVER;

   copt.ca_file = CERT_CA;
   copt.pem_file = CERT_PASSWORD_PROTECTED;
   copt.pem_pwd = CERT_PASSWORD;

   /* Password protected key */
   ssl_test (&copt, &sopt, "localhost", &cr, &sr);
   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SUCCESS);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SUCCESS);
}

static void
test_mongoc_tls_bad_password (void)
{
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
   ssl_test_result_t sr;
   ssl_test_result_t cr;

   sopt.ca_file = CERT_CA;
   sopt.pem_file = CERT_SERVER;

   copt.ca_file = CERT_CA;
   copt.pem_file = CERT_PASSWORD_PROTECTED;
   copt.pem_pwd = "incorrect password";


   capture_logs (true);
   /* Incorrect password cannot unlock the key */
   ssl_test (&copt, &sopt, "localhost", &cr, &sr);

   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SSL_HANDSHAKE);
   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SSL_INIT);

   /* Change it to the right password */
   copt.pem_pwd = CERT_PASSWORD;
   ssl_test (&copt, &sopt, "localhost", &cr, &sr);

   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SUCCESS);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SUCCESS);
}
#endif


static void
test_mongoc_tls_no_verify (void)
{
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
   ssl_test_result_t sr;
   ssl_test_result_t cr;

   sopt.ca_file = CERT_CA;
   sopt.pem_file = CERT_SERVER;

   copt.ca_file = CERT_CA;
   copt.pem_file = CERT_CLIENT;
   /* Weak cert validation allows never fails */
   copt.weak_cert_validation = 1;

   ssl_test (&copt, &sopt, "bad_domain.com", &cr, &sr);

   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SUCCESS);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SUCCESS);
}


static void
test_mongoc_tls_allow_invalid_hostname (void)
{
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
   ssl_test_result_t sr;
   ssl_test_result_t cr;

   sopt.ca_file = CERT_CA;
   sopt.pem_file = CERT_SERVER;

   copt.ca_file = CERT_CA;
   copt.pem_file = CERT_CLIENT;
   copt.allow_invalid_hostname = 1;

   /* Connect to a domain not listed in the cert */
   ssl_test (&copt, &sopt, "bad_domain.com", &cr, &sr);

   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SUCCESS);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SUCCESS);
}


static void
test_mongoc_tls_bad_verify (void)
{
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
   ssl_test_result_t sr;
   ssl_test_result_t cr;

   sopt.ca_file = CERT_CA;
   sopt.pem_file = CERT_SERVER;

   copt.ca_file = CERT_CA;
   copt.pem_file = CERT_CLIENT;

   capture_logs (true);
   ssl_test (&copt, &sopt, "bad_domain.com", &cr, &sr);

   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SSL_HANDSHAKE);
   ASSERT_CMPINT (sr.result, !=, SSL_TEST_SUCCESS);


   /* weak_cert_validation allows bad domains */
   copt.weak_cert_validation = 1;
   ssl_test (&copt, &sopt, "bad_domain.com", &cr, &sr);
   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SUCCESS);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SUCCESS);
}


static void
test_mongoc_tls_basic (void)
{
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
   ssl_test_result_t sr;
   ssl_test_result_t cr;

   sopt.ca_file = CERT_CA;
   sopt.pem_file = CERT_SERVER;

   copt.ca_file = CERT_CA;
   copt.pem_file = CERT_CLIENT;

   ssl_test (&copt, &sopt, "localhost", &cr, &sr);

   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SUCCESS);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SUCCESS);
}


#ifdef MONGOC_ENABLE_SSL_OPENSSL
static void
test_mongoc_tls_weak_cert_validation (void)
{
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
   ssl_test_result_t sr;
   ssl_test_result_t cr;

   sopt.ca_file = CERT_CA;
   sopt.pem_file = CERT_SERVER;

   copt.ca_file = CERT_CA;
   copt.crl_file = CERT_CRL;
   copt.pem_file = CERT_CLIENT;

   capture_logs (true);
   /* Certificate has has been revoked, this should fail */
   ssl_test (&copt, &sopt, "localhost", &cr, &sr);

   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SSL_HANDSHAKE);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SSL_HANDSHAKE);


   /* weak_cert_validation allows revoked certs */
   copt.weak_cert_validation = 1;
   ssl_test (&copt, &sopt, "bad_domain.com", &cr, &sr);

   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SUCCESS);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SUCCESS);
}


static void
test_mongoc_tls_crl (void)
{
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
   ssl_test_result_t sr;
   ssl_test_result_t cr;

   sopt.ca_file = CERT_CA;
   sopt.pem_file = CERT_SERVER;

   copt.ca_file = CERT_CA;
   copt.pem_file = CERT_CLIENT;

   ssl_test (&copt, &sopt, "localhost", &cr, &sr);

   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SUCCESS);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SUCCESS);

   copt.crl_file = CERT_CRL;
   capture_logs (true);
   ssl_test (&copt, &sopt, "localhost", &cr, &sr);

   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SSL_HANDSHAKE);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SSL_HANDSHAKE);

   /* Weak cert validation allows revoked */
   copt.weak_cert_validation = true;
   ssl_test (&copt, &sopt, "localhost", &cr, &sr);
   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SUCCESS);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SUCCESS);
}
#endif


static void
test_mongoc_tls_expired (void)
{
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
   ssl_test_result_t sr;
   ssl_test_result_t cr;

   sopt.ca_file = CERT_CA;
   sopt.pem_file = CERT_EXPIRED;

   copt.ca_file = CERT_CA;
   copt.pem_file = CERT_CLIENT;

   capture_logs (true);
   ssl_test (&copt, &sopt, "localhost", &cr, &sr);

   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SSL_HANDSHAKE);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SSL_HANDSHAKE);

   /* Weak cert validation allows expired certs */
   copt.weak_cert_validation = true;
   ssl_test (&copt, &sopt, "localhost", &cr, &sr);
   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SUCCESS);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SUCCESS);
}


static void
test_mongoc_tls_common_name (void)
{
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
   ssl_test_result_t sr;
   ssl_test_result_t cr;

   sopt.ca_file = CERT_CA;
   sopt.pem_file = CERT_COMMONNAME;

   copt.ca_file = CERT_CA;
   copt.pem_file = CERT_CLIENT;

   /* Match against commonName */
   ssl_test (&copt, &sopt, "commonName.mongodb.org", &cr, &sr);

   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SUCCESS);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SUCCESS);
}


static void
test_mongoc_tls_altname (void)
{
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
   ssl_test_result_t sr;
   ssl_test_result_t cr;

   sopt.ca_file = CERT_CA;
   sopt.pem_file = CERT_ALTNAME;

   copt.ca_file = CERT_CA;
   copt.pem_file = CERT_CLIENT;

   /* Match against secondary Subject Alternative Name (SAN) */
   ssl_test (&copt, &sopt, "alternative.mongodb.com", &cr, &sr);

   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SUCCESS);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SUCCESS);
}


static void
test_mongoc_tls_wild (void)
{
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
   ssl_test_result_t sr;
   ssl_test_result_t cr;

   sopt.ca_file = CERT_CA;
   sopt.pem_file = CERT_WILD;

   copt.ca_file = CERT_CA;
   copt.pem_file = CERT_CLIENT;

   ssl_test (&copt, &sopt, "anything.mongodb.org", &cr, &sr);

   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SUCCESS);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SUCCESS);
}


#ifdef MONGOC_ENABLE_SSL_OPENSSL
static void
test_mongoc_tls_ip (void)
{
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
   ssl_test_result_t sr;
   ssl_test_result_t cr;

   sopt.ca_file = CERT_CA;
   sopt.pem_file = CERT_SERVER;

   copt.ca_file = CERT_CA;

   ssl_test (&copt, &sopt, "127.0.0.1", &cr, &sr);

   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SUCCESS);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SUCCESS);
}
#endif


#if !defined(__APPLE__) && !defined(_WIN32) && \
   defined(MONGOC_ENABLE_SSL_OPENSSL) && OPENSSL_VERSION_NUMBER >= 0x10000000L
static void
test_mongoc_tls_trust_dir (void)
{
   mongoc_ssl_opt_t sopt = {0};
   mongoc_ssl_opt_t copt = {0};
   ssl_test_result_t sr;
   ssl_test_result_t cr;

   sopt.pem_file = CERT_SERVER;
   sopt.ca_file = CERT_CA;

   copt.ca_dir = CERT_TEST_DIR;

   ssl_test (&copt, &sopt, "localhost", &cr, &sr);

   ASSERT_CMPINT (cr.result, ==, SSL_TEST_SUCCESS);
   ASSERT_CMPINT (sr.result, ==, SSL_TEST_SUCCESS);
}
#endif

#endif /* !MONGOC_ENABLE_SSL_SECURE_CHANNEL && !MONGOC_ENABLE_SSL_LIBRESSL */

void
test_stream_tls_install (TestSuite *suite)
{
#if !defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL) && \
   !defined(MONGOC_ENABLE_SSL_LIBRESSL)
   TestSuite_Add (suite, "/TLS/commonName", test_mongoc_tls_common_name);
   TestSuite_Add (suite, "/TLS/altname", test_mongoc_tls_altname);
   TestSuite_Add (suite, "/TLS/basic", test_mongoc_tls_basic);
   TestSuite_Add (suite,
                  "/TLS/allow_invalid_hostname",
                  test_mongoc_tls_allow_invalid_hostname);
   TestSuite_Add (suite, "/TLS/wild", test_mongoc_tls_wild);
   TestSuite_Add (suite, "/TLS/no_verify", test_mongoc_tls_no_verify);
   TestSuite_Add (suite, "/TLS/bad_verify", test_mongoc_tls_bad_verify);
   TestSuite_Add (suite, "/TLS/no_certs", test_mongoc_tls_no_certs);

   TestSuite_Add (suite, "/TLS/expired", test_mongoc_tls_expired);

#ifdef MONGOC_ENABLE_SSL_OPENSSL
   TestSuite_Add (suite, "/TLS/ip", test_mongoc_tls_ip);
   TestSuite_Add (suite, "/TLS/password", test_mongoc_tls_password);
   TestSuite_Add (suite, "/TLS/bad_password", test_mongoc_tls_bad_password);
   TestSuite_Add (
      suite, "/TLS/weak_cert_validation", test_mongoc_tls_weak_cert_validation);
   TestSuite_Add (suite, "/TLS/crl", test_mongoc_tls_crl);
#endif

#if !defined(__APPLE__) && !defined(_WIN32) && \
   defined(MONGOC_ENABLE_SSL_OPENSSL) && OPENSSL_VERSION_NUMBER >= 0x10000000L
   TestSuite_Add (suite, "/TLS/trust_dir", test_mongoc_tls_trust_dir);
#endif
#endif
}
