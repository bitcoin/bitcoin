#include <mongoc.h>

#include "mongoc-client-private.h"
#include "mongoc-topology-private.h"
#include "mongoc-uri-private.h"
#include "mongoc-host-list-private.h"

#include "TestSuite.h"

#include "test-libmongoc.h"
#include "test-conveniences.h"


static void
test_mongoc_uri_new (void)
{
   const mongoc_host_list_t *hosts;
   const bson_t *options;
   const bson_t *credentials;
   const bson_t *read_prefs_tags;
   const mongoc_read_prefs_t *read_prefs;
   bson_t properties;
   mongoc_uri_t *uri;
   bson_iter_t iter;
   bson_iter_t child;

   capture_logs (true);

   /* bad uris */
   ASSERT (!mongoc_uri_new ("mongodb://"));
   ASSERT (!mongoc_uri_new ("mongodb://\x80"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost/\x80"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost:\x80/"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost/?ipv6=\x80"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost/?foo=\x80"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost/?\x80=bar"));
   ASSERT (!mongoc_uri_new ("mongodb://\x80:pass@localhost"));
   ASSERT (!mongoc_uri_new ("mongodb://user:\x80@localhost"));
   ASSERT (!mongoc_uri_new ("mongodb://user%40DOMAIN.COM:password@localhost/"
                            "?" MONGOC_URI_AUTHMECHANISM "=\x80"));
   ASSERT (!mongoc_uri_new ("mongodb://user%40DOMAIN.COM:password@localhost/"
                            "?" MONGOC_URI_AUTHMECHANISM
                            "=GSSAPI&" MONGOC_URI_AUTHMECHANISMPROPERTIES
                            "=SERVICE_NAME:\x80"));
   ASSERT (!mongoc_uri_new ("mongodb://user%40DOMAIN.COM:password@localhost/"
                            "?" MONGOC_URI_AUTHMECHANISM
                            "=GSSAPI&" MONGOC_URI_AUTHMECHANISMPROPERTIES
                            "=\x80:mongodb"));
   ASSERT (!mongoc_uri_new ("mongodb://::"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost::27017"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost,localhost::"));
   ASSERT (!mongoc_uri_new ("mongodb://local1,local2,local3/d?k"));
   ASSERT (!mongoc_uri_new (""));
   ASSERT (!mongoc_uri_new ("mongodb://,localhost:27017"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost:27017,,b"));
   ASSERT (!mongoc_uri_new ("mongo://localhost:27017"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost::27017"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost::27017/"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost::27017,abc"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost:-1"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost:65536"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost:foo"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost:65536/"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost:0/"));
   ASSERT (!mongoc_uri_new ("mongodb://[::1%lo0]"));
   ASSERT (!mongoc_uri_new ("mongodb://[::1]:-1"));
   ASSERT (!mongoc_uri_new ("mongodb://[::1]:foo"));
   ASSERT (!mongoc_uri_new ("mongodb://[::1]:65536"));
   ASSERT (!mongoc_uri_new ("mongodb://[::1]:65536/"));
   ASSERT (!mongoc_uri_new ("mongodb://[::1]:0/"));

   uri = mongoc_uri_new (
      "mongodb://[::1]:27888,[::2]:27999/?ipv6=true&" MONGOC_URI_SAFE "=true");
   BSON_ASSERT (uri);
   hosts = mongoc_uri_get_hosts (uri);
   BSON_ASSERT (hosts);
   ASSERT_CMPSTR (hosts->host, "::1");
   BSON_ASSERT (hosts->port == 27888);
   ASSERT_CMPSTR (hosts->host_and_port, "[::1]:27888");
   mongoc_uri_destroy (uri);

   /* should recognize IPv6 "scope" like "::1%lo0", with % escaped  */
   uri = mongoc_uri_new ("mongodb://[::1%25lo0]");
   BSON_ASSERT (uri);
   hosts = mongoc_uri_get_hosts (uri);
   BSON_ASSERT (hosts);
   ASSERT_CMPSTR (hosts->host, "::1%lo0");
   BSON_ASSERT (hosts->port == 27017);
   ASSERT_CMPSTR (hosts->host_and_port, "[::1%lo0]:27017");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://%2Ftmp%2Fmongodb-27017.sock/?");
   ASSERT (uri);
   mongoc_uri_destroy (uri);

   /* should normalize to lowercase */
   uri = mongoc_uri_new ("mongodb://cRaZyHoStNaMe");
   BSON_ASSERT (uri);
   hosts = mongoc_uri_get_hosts (uri);
   BSON_ASSERT (hosts);
   ASSERT_CMPSTR (hosts->host, "crazyhostname");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/?");
   ASSERT (uri);
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost:27017/test?replicaset=foo");
   ASSERT (uri);
   hosts = mongoc_uri_get_hosts (uri);
   ASSERT (hosts);
   ASSERT (!hosts->next);
   ASSERT_CMPSTR (hosts->host, "localhost");
   ASSERT_CMPINT (hosts->port, ==, 27017);
   ASSERT_CMPSTR (hosts->host_and_port, "localhost:27017");
   ASSERT_CMPSTR (mongoc_uri_get_database (uri), "test");
   options = mongoc_uri_get_options (uri);
   ASSERT (options);
   ASSERT (bson_iter_init_find (&iter, options, "replicaset"));
   ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), "foo");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://local1,local2:999,local3/?replicaset=foo");
   ASSERT (uri);
   hosts = mongoc_uri_get_hosts (uri);
   ASSERT (hosts);
   ASSERT (hosts->next);
   ASSERT (hosts->next->next);
   ASSERT (!hosts->next->next->next);
   ASSERT_CMPSTR (hosts->host, "local1");
   ASSERT_CMPINT (hosts->port, ==, 27017);
   ASSERT_CMPSTR (hosts->next->host, "local2");
   ASSERT_CMPINT (hosts->next->port, ==, 999);
   ASSERT_CMPSTR (hosts->next->next->host, "local3");
   ASSERT_CMPINT (hosts->next->next->port, ==, 27017);
   options = mongoc_uri_get_options (uri);
   ASSERT (options);
   ASSERT (bson_iter_init_find (&iter, options, "replicaset"));
   ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), "foo");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost:27017/"
                         "?" MONGOC_URI_READPREFERENCE
                         "=secondaryPreferred&" MONGOC_URI_READPREFERENCETAGS
                         "=dc:ny&" MONGOC_URI_READPREFERENCETAGS "=");
   ASSERT (uri);
   read_prefs = mongoc_uri_get_read_prefs_t (uri);
   ASSERT (mongoc_read_prefs_get_mode (read_prefs) ==
           MONGOC_READ_SECONDARY_PREFERRED);
   ASSERT (read_prefs);
   read_prefs_tags = mongoc_read_prefs_get_tags (read_prefs);
   ASSERT (read_prefs_tags);
   ASSERT_CMPINT (bson_count_keys (read_prefs_tags), ==, 2);
   ASSERT (bson_iter_init_find (&iter, read_prefs_tags, "0"));
   ASSERT (BSON_ITER_HOLDS_DOCUMENT (&iter));
   ASSERT (bson_iter_recurse (&iter, &child));
   ASSERT (bson_iter_next (&child));
   ASSERT_CMPSTR (bson_iter_key (&child), "dc");
   ASSERT_CMPSTR (bson_iter_utf8 (&child, NULL), "ny");
   ASSERT (!bson_iter_next (&child));
   ASSERT (bson_iter_next (&iter));
   ASSERT (BSON_ITER_HOLDS_DOCUMENT (&iter));
   ASSERT (bson_iter_recurse (&iter, &child));
   ASSERT (!bson_iter_next (&child));
   ASSERT (!bson_iter_next (&iter));
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/a?" MONGOC_URI_SLAVEOK
                         "=true&" MONGOC_URI_SSL "=false&" MONGOC_URI_JOURNAL
                         "=true");
   options = mongoc_uri_get_options (uri);
   ASSERT (options);
   ASSERT_CMPINT (bson_count_keys (options), ==, 3);
   ASSERT (bson_iter_init (&iter, options));
   ASSERT (bson_iter_find_case (&iter, "" MONGOC_URI_SLAVEOK ""));
   ASSERT (BSON_ITER_HOLDS_BOOL (&iter));
   ASSERT (bson_iter_bool (&iter));
   ASSERT (bson_iter_find_case (&iter, MONGOC_URI_SSL));
   ASSERT (BSON_ITER_HOLDS_BOOL (&iter));
   ASSERT (!bson_iter_bool (&iter));
   ASSERT (bson_iter_find_case (&iter, MONGOC_URI_JOURNAL));
   ASSERT (BSON_ITER_HOLDS_BOOL (&iter));
   ASSERT (bson_iter_bool (&iter));
   ASSERT (!bson_iter_next (&iter));
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_SAFE
                         "=false&" MONGOC_URI_JOURNAL "=false");
   options = mongoc_uri_get_options (uri);
   ASSERT (options);
   ASSERT_CMPINT (bson_count_keys (options), ==, 2);
   ASSERT (bson_iter_init (&iter, options));
   ASSERT (bson_iter_find_case (&iter, "" MONGOC_URI_SAFE ""));
   ASSERT (BSON_ITER_HOLDS_BOOL (&iter));
   ASSERT (!bson_iter_bool (&iter));
   ASSERT (bson_iter_find_case (&iter, MONGOC_URI_JOURNAL));
   ASSERT (BSON_ITER_HOLDS_BOOL (&iter));
   ASSERT (!bson_iter_bool (&iter));
   ASSERT (!bson_iter_next (&iter));
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new (
      "mongodb://%2Ftmp%2Fmongodb-27017.sock/?" MONGOC_URI_SSL "=false");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->host, "/tmp/mongodb-27017.sock");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new (
      "mongodb://%2Ftmp%2Fmongodb-27017.sock,localhost:27017/?" MONGOC_URI_SSL
      "=false");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->host, "/tmp/mongodb-27017.sock");
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->next->host_and_port,
                  "localhost:27017");
   ASSERT (!mongoc_uri_get_hosts (uri)->next->next);
   mongoc_uri_destroy (uri);

   /* should assign port numbers to correct hosts */
   uri = mongoc_uri_new ("mongodb://host1,host2:30000/foo");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->host_and_port, "host1:27017");
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->next->host_and_port,
                  "host2:30000");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new (
      "mongodb://localhost:27017,%2Ftmp%2Fmongodb-27017.sock/?" MONGOC_URI_SSL
      "=false");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->host_and_port, "localhost:27017");
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->next->host,
                  "/tmp/mongodb-27017.sock");
   ASSERT (!mongoc_uri_get_hosts (uri)->next->next);
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_HEARTBEATFREQUENCYMS
                         "=600");
   ASSERT (uri);
   ASSERT_CMPINT32 (
      600,
      ==,
      mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 0));

   mongoc_uri_destroy (uri);

   /* heartbeat frequency too short */
   ASSERT (!mongoc_uri_new (
      "mongodb://localhost/?" MONGOC_URI_HEARTBEATFREQUENCYMS "=499"));

   /* should use the " MONGOC_URI_AUTHSOURCE " over db when both are specified
    */
   uri = mongoc_uri_new (
      "mongodb://christian:secret@localhost:27017/foo?" MONGOC_URI_AUTHSOURCE
      "=abcd");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_username (uri), "christian");
   ASSERT_CMPSTR (mongoc_uri_get_password (uri), "secret");
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "abcd");
   mongoc_uri_destroy (uri);

   /* should use the default auth source and mechanism */
   uri = mongoc_uri_new ("mongodb://christian:secret@localhost:27017");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "admin");
   ASSERT (!mongoc_uri_get_auth_mechanism (uri));
   mongoc_uri_destroy (uri);

   /* should use the db when no " MONGOC_URI_AUTHSOURCE " is specified */
   uri = mongoc_uri_new ("mongodb://user:password@localhost/foo");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "foo");
   mongoc_uri_destroy (uri);

   /* should recognize an empty password */
   uri = mongoc_uri_new ("mongodb://samantha:@localhost");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_username (uri), "samantha");
   ASSERT_CMPSTR (mongoc_uri_get_password (uri), "");
   mongoc_uri_destroy (uri);

   /* should recognize no password */
   uri = mongoc_uri_new ("mongodb://christian@localhost:27017");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_username (uri), "christian");
   ASSERT (!mongoc_uri_get_password (uri));
   mongoc_uri_destroy (uri);

   /* should recognize a url escaped character in the username */
   uri = mongoc_uri_new ("mongodb://christian%40realm:pwd@localhost:27017");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_username (uri), "christian@realm");
   mongoc_uri_destroy (uri);

   /* should fail on invalid escaped characters */
   capture_logs (true);
   uri = mongoc_uri_new ("mongodb://u%ser:pwd@localhost:27017");
   ASSERT (!uri);
   ASSERT_CAPTURED_LOG (
      "uri", MONGOC_LOG_LEVEL_WARNING, "Invalid % escape sequence");

   uri = mongoc_uri_new ("mongodb://user:p%wd@localhost:27017");
   ASSERT (!uri);
   ASSERT_CAPTURED_LOG (
      "uri", MONGOC_LOG_LEVEL_WARNING, "Invalid % escape sequence");

   uri = mongoc_uri_new ("mongodb://user:pwd@local% host:27017");
   ASSERT (!uri);
   ASSERT_CAPTURED_LOG (
      "uri", MONGOC_LOG_LEVEL_WARNING, "Invalid % escape sequence");

   uri = mongoc_uri_new (
      "mongodb://christian%40realm@localhost:27017/?replicaset=%20");
   ASSERT (uri);
   options = mongoc_uri_get_options (uri);
   ASSERT (options);
   ASSERT (bson_iter_init_find (&iter, options, "replicaset"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
   ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), " ");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new (
      "mongodb://christian%40realm@[::6]:27017/?replicaset=%20");
   ASSERT (uri);
   options = mongoc_uri_get_options (uri);
   ASSERT (options);
   ASSERT (bson_iter_init_find (&iter, options, "replicaset"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
   ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), " ");
   mongoc_uri_destroy (uri);

   /* GSSAPI-specific options */

   /* should recognize the GSSAPI mechanism, and use $external as source */
   uri = mongoc_uri_new ("mongodb://user%40DOMAIN.COM:password@localhost/"
                         "?" MONGOC_URI_AUTHMECHANISM "=GSSAPI");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), "GSSAPI");
   /*ASSERT_CMPSTR(mongoc_uri_get_auth_source(uri), "$external");*/
   mongoc_uri_destroy (uri);

   /* use $external as source when db is specified */
   uri = mongoc_uri_new ("mongodb://user%40DOMAIN.COM:password@localhost/foo"
                         "?" MONGOC_URI_AUTHMECHANISM "=GSSAPI");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "$external");
   mongoc_uri_destroy (uri);

   /* should not accept " MONGOC_URI_AUTHSOURCE " other than $external */
   ASSERT (!mongoc_uri_new ("mongodb://user%40DOMAIN.COM:password@localhost/"
                            "foo?" MONGOC_URI_AUTHMECHANISM
                            "=GSSAPI&" MONGOC_URI_AUTHSOURCE "=bar"));

   /* should accept MONGOC_URI_AUTHMECHANISMPROPERTIES */
   uri = mongoc_uri_new ("mongodb://user%40DOMAIN.COM:password@localhost/"
                         "?" MONGOC_URI_AUTHMECHANISM
                         "=GSSAPI&" MONGOC_URI_AUTHMECHANISMPROPERTIES
                         "=SERVICE_NAME:other,CANONICALIZE_HOST_NAME:"
                         "true");
   ASSERT (uri);
   credentials = mongoc_uri_get_credentials (uri);
   ASSERT (credentials);
   ASSERT (mongoc_uri_get_mechanism_properties (uri, &properties));
   BSON_ASSERT (bson_iter_init_find_case (&iter, &properties, "SERVICE_NAME") &&
                BSON_ITER_HOLDS_UTF8 (&iter) &&
                (0 == strcmp (bson_iter_utf8 (&iter, NULL), "other")));
   BSON_ASSERT (
      bson_iter_init_find_case (&iter, &properties, "CANONICALIZE_HOST_NAME") &&
      BSON_ITER_HOLDS_UTF8 (&iter) &&
      (0 == strcmp (bson_iter_utf8 (&iter, NULL), "true")));
   mongoc_uri_destroy (uri);

   /* reverse order of arguments to ensure parsing still succeeds */
   uri = mongoc_uri_new (
      "mongodb://user@localhost/?" MONGOC_URI_AUTHMECHANISMPROPERTIES
      "=SERVICE_NAME:other&" MONGOC_URI_AUTHMECHANISM "=GSSAPI");
   ASSERT (uri);
   mongoc_uri_destroy (uri);

   /* deprecated gssapiServiceName option */
   uri = mongoc_uri_new ("mongodb://christian%40realm.cc@localhost:27017/"
                         "?" MONGOC_URI_AUTHMECHANISM
                         "=GSSAPI&" MONGOC_URI_GSSAPISERVICENAME "=blah");
   ASSERT (uri);
   options = mongoc_uri_get_options (uri);
   ASSERT (options);
   BSON_ASSERT (0 == strcmp (mongoc_uri_get_auth_mechanism (uri), "GSSAPI"));
   BSON_ASSERT (0 ==
                strcmp (mongoc_uri_get_username (uri), "christian@realm.cc"));
   BSON_ASSERT (
      bson_iter_init_find_case (&iter, options, MONGOC_URI_GSSAPISERVICENAME) &&
      BSON_ITER_HOLDS_UTF8 (&iter) &&
      (0 == strcmp (bson_iter_utf8 (&iter, NULL), "blah")));
   mongoc_uri_destroy (uri);

   /* MONGODB-CR */

   /* should recognize this mechanism */
   uri = mongoc_uri_new ("mongodb://user@localhost/?" MONGOC_URI_AUTHMECHANISM
                         "=MONGODB-CR");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), "MONGODB-CR");
   mongoc_uri_destroy (uri);

   /* X509 */

   /* should recognize this mechanism, and use $external as the source */
   uri = mongoc_uri_new ("mongodb://user@localhost/?" MONGOC_URI_AUTHMECHANISM
                         "=MONGODB-X509");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), "MONGODB-X509");
   /*ASSERT_CMPSTR(mongoc_uri_get_auth_source(uri), "$external");*/
   mongoc_uri_destroy (uri);

   /* use $external as source when db is specified */
   uri = mongoc_uri_new (
      "mongodb://CN%3DmyName%2COU%3DmyOrgUnit%2CO%3DmyOrg%2CL%3DmyLocality"
      "%2CST%3DmyState%2CC%3DmyCountry@localhost/foo"
      "?" MONGOC_URI_AUTHMECHANISM "=MONGODB-X509");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "$external");
   mongoc_uri_destroy (uri);

   /* should not accept " MONGOC_URI_AUTHSOURCE " other than $external */
   ASSERT (!mongoc_uri_new (
      "mongodb://CN%3DmyName%2COU%3DmyOrgUnit%2CO%3DmyOrg%2CL%3DmyLocality"
      "%2CST%3DmyState%2CC%3DmyCountry@localhost/foo"
      "?" MONGOC_URI_AUTHMECHANISM "=MONGODB-X509&" MONGOC_URI_AUTHSOURCE
      "=bar"));

   /* should recognize the encoded username */
   uri = mongoc_uri_new (
      "mongodb://CN%3DmyName%2COU%3DmyOrgUnit%2CO%3DmyOrg%2CL%3DmyLocality"
      "%2CST%3DmyState%2CC%3DmyCountry@localhost/?" MONGOC_URI_AUTHMECHANISM
      "=MONGODB-X509");
   ASSERT (uri);
   ASSERT_CMPSTR (
      mongoc_uri_get_username (uri),
      "CN=myName,OU=myOrgUnit,O=myOrg,L=myLocality,ST=myState,C=myCountry");
   mongoc_uri_destroy (uri);

   /* PLAIN */

   /* should recognize this mechanism */
   uri = mongoc_uri_new ("mongodb://user@localhost/?" MONGOC_URI_AUTHMECHANISM
                         "=PLAIN");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), "PLAIN");
   mongoc_uri_destroy (uri);

   /* SCRAM-SHA1 */

   /* should recognize this mechanism */
   uri = mongoc_uri_new ("mongodb://user@localhost/?" MONGOC_URI_AUTHMECHANISM
                         "=SCRAM-SHA1");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), "SCRAM-SHA1");
   mongoc_uri_destroy (uri);
}


static void
test_mongoc_uri_authmechanismproperties (void)
{
   mongoc_uri_t *uri;
   bson_t props;

   uri = mongoc_uri_new ("mongodb://user@localhost/?" MONGOC_URI_AUTHMECHANISM
                         "=SCRAM-SHA1"
                         "&" MONGOC_URI_AUTHMECHANISMPROPERTIES "=a:one,b:two");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), "SCRAM-SHA1");
   ASSERT (mongoc_uri_get_mechanism_properties (uri, &props));
   ASSERT_MATCH (&props, "{'a': 'one', 'b': 'two'}");

   ASSERT (mongoc_uri_set_auth_mechanism (uri, "MONGODB-CR"));
   ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), "MONGODB-CR");

   /* prohibited */
   ASSERT (!mongoc_uri_set_option_as_utf8 (
      uri, MONGOC_URI_AUTHMECHANISM, "SCRAM-SHA1"));

   ASSERT (!mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_AUTHMECHANISM, 1));

   ASSERT (!mongoc_uri_set_option_as_utf8 (
      uri, MONGOC_URI_AUTHMECHANISMPROPERTIES, "a:three"));

   ASSERT (
      mongoc_uri_set_mechanism_properties (uri, tmp_bson ("{'a': 'four'}")));

   ASSERT (mongoc_uri_get_mechanism_properties (uri, &props));
   ASSERT_MATCH (&props, "{'a': 'four', 'b': {'$exists': false}}");

   mongoc_uri_destroy (uri);
}


static void
test_mongoc_uri_functions (void)
{
   mongoc_client_t *client;
   mongoc_uri_t *uri;
   mongoc_database_t *db;
   int32_t i;

   uri = mongoc_uri_new (
      "mongodb://foo:bar@localhost:27017/baz?" MONGOC_URI_AUTHSOURCE "=source");

   ASSERT_CMPSTR (mongoc_uri_get_username (uri), "foo");
   ASSERT_CMPSTR (mongoc_uri_get_password (uri), "bar");
   ASSERT_CMPSTR (mongoc_uri_get_database (uri), "baz");
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "source");

   mongoc_uri_set_username (uri, "longer username that should work");
   ASSERT_CMPSTR (mongoc_uri_get_username (uri),
                  "longer username that should work");

   mongoc_uri_set_password (uri, "longer password that should also work");
   ASSERT_CMPSTR (mongoc_uri_get_password (uri),
                  "longer password that should also work");

   mongoc_uri_set_database (uri, "longer database that should work");
   ASSERT_CMPSTR (mongoc_uri_get_database (uri),
                  "longer database that should work");
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "source");

   mongoc_uri_set_auth_source (uri, "longer authsource that should work");
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri),
                  "longer authsource that should work");
   ASSERT_CMPSTR (mongoc_uri_get_database (uri),
                  "longer database that should work");

   client = mongoc_client_new_from_uri (uri);
   mongoc_uri_destroy (uri);

   ASSERT_CMPSTR (mongoc_uri_get_username (client->uri),
                  "longer username that should work");
   ASSERT_CMPSTR (mongoc_uri_get_password (client->uri),
                  "longer password that should also work");
   ASSERT_CMPSTR (mongoc_uri_get_database (client->uri),
                  "longer database that should work");
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (client->uri),
                  "longer authsource that should work");
   mongoc_client_destroy (client);


   uri = mongoc_uri_new (
      "mongodb://localhost/?" MONGOC_URI_SERVERSELECTIONTIMEOUTMS "=3"
      "&" MONGOC_URI_JOURNAL "=true"
      "&" MONGOC_URI_WTIMEOUTMS "=42"
      "&" MONGOC_URI_CANONICALIZEHOSTNAME "=false");

   ASSERT_CMPINT (
      mongoc_uri_get_option_as_int32 (uri, "serverselectiontimeoutms", 18),
      ==,
      3);
   ASSERT (
      mongoc_uri_set_option_as_int32 (uri, "serverselectiontimeoutms", 18));
   ASSERT_CMPINT (
      mongoc_uri_get_option_as_int32 (uri, "serverselectiontimeoutms", 19),
      ==,
      18);

   ASSERT_CMPINT (
      mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_WTIMEOUTMS, 18), ==, 42);
   ASSERT (mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_WTIMEOUTMS, 18));
   ASSERT_CMPINT (
      mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_WTIMEOUTMS, 19), ==, 18);

   ASSERT (mongoc_uri_set_option_as_int32 (
      uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 500));

   i = mongoc_uri_get_option_as_int32 (
      uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 1000);

   ASSERT_CMPINT32 (i, ==, 500);

   capture_logs (true);

   /* Server Discovery and Monitoring Spec: "the driver MUST NOT permit users to
    * configure it less than minHeartbeatFrequencyMS (500ms)." */
   ASSERT (!mongoc_uri_set_option_as_int32 (
      uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 499));

   ASSERT_CAPTURED_LOG (
      "mongoc_uri_set_option_as_int32",
      MONGOC_LOG_LEVEL_WARNING,
      "Invalid \"heartbeatfrequencyms\" of 499: must be at least 500");

   /* socketcheckintervalms isn't set, return our fallback */
   ASSERT_CMPINT (mongoc_uri_get_option_as_int32 (
                     uri, MONGOC_URI_SOCKETCHECKINTERVALMS, 123),
                  ==,
                  123);
   ASSERT (mongoc_uri_set_option_as_int32 (
      uri, MONGOC_URI_SOCKETCHECKINTERVALMS, 18));
   ASSERT_CMPINT (mongoc_uri_get_option_as_int32 (
                     uri, MONGOC_URI_SOCKETCHECKINTERVALMS, 19),
                  ==,
                  18);

   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_JOURNAL, false));
   ASSERT (!mongoc_uri_get_option_as_bool (
      uri, MONGOC_URI_CANONICALIZEHOSTNAME, true));
   /* ssl isn't set, return out fallback */
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_SSL, true));

   client = mongoc_client_new_from_uri (uri);
   mongoc_uri_destroy (uri);

   ASSERT (
      mongoc_uri_get_option_as_bool (client->uri, MONGOC_URI_JOURNAL, false));
   ASSERT (!mongoc_uri_get_option_as_bool (
      client->uri, MONGOC_URI_CANONICALIZEHOSTNAME, true));
   /* ssl isn't set, return out fallback */
   ASSERT (mongoc_uri_get_option_as_bool (client->uri, MONGOC_URI_SSL, true));
   mongoc_client_destroy (client);

   uri = mongoc_uri_new ("mongodb://localhost/");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, "replicaset", "default"),
                  "default");
   ASSERT (mongoc_uri_set_option_as_utf8 (uri, "replicaset", "value"));
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, "replicaset", "default"),
                  "value");

   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_SOCKETTIMEOUTMS
                         "=1&" MONGOC_URI_SOCKETCHECKINTERVALMS "=200");
   ASSERT_CMPINT (
      1,
      ==,
      mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_SOCKETTIMEOUTMS, 0));
   ASSERT_CMPINT (200,
                  ==,
                  mongoc_uri_get_option_as_int32 (
                     uri, MONGOC_URI_SOCKETCHECKINTERVALMS, 0));

   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_SOCKETTIMEOUTMS, 2);
   ASSERT_CMPINT (
      2,
      ==,
      mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_SOCKETTIMEOUTMS, 0));

   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_SOCKETCHECKINTERVALMS, 202);
   ASSERT_CMPINT (202,
                  ==,
                  mongoc_uri_get_option_as_int32 (
                     uri, MONGOC_URI_SOCKETCHECKINTERVALMS, 0));


   client = mongoc_client_new_from_uri (uri);
   ASSERT_CMPINT (2, ==, client->cluster.sockettimeoutms);
   ASSERT_CMPINT (202, ==, client->cluster.socketcheckintervalms);

   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new ("mongodb://host/dbname0");
   ASSERT_CMPSTR (mongoc_uri_get_database (uri), "dbname0");
   mongoc_uri_set_database (uri, "dbname1");
   client = mongoc_client_new_from_uri (uri);
   db = mongoc_client_get_default_database (client);
   ASSERT_CMPSTR (mongoc_database_get_name (db), "dbname1");

   mongoc_database_destroy (db);
   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://%2Ftmp%2FMongoDB-27017.sock/");
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->host, "/tmp/MongoDB-27017.sock");
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->host_and_port,
                  "/tmp/MongoDB-27017.sock");

   mongoc_uri_destroy (uri);
}

static void
test_mongoc_uri_new_with_error (void)
{
   bson_error_t error = {0};
   mongoc_uri_t *uri;

   capture_logs (true);
   ASSERT (!mongoc_uri_new_with_error ("mongodb://", NULL));
   uri = mongoc_uri_new_with_error ("mongodb://localhost", NULL);
   ASSERT (uri);
   mongoc_uri_destroy (uri);

   ASSERT (!mongoc_uri_new_with_error ("mongodb://", &error));
   ASSERT (error.domain == MONGOC_ERROR_COMMAND);

   error.domain = 0;
   ASSERT (!mongoc_uri_new_with_error ("mongo://localhost", &error));
   ASSERT (error.domain == MONGOC_ERROR_COMMAND);

   error.domain = 0;
   ASSERT (!mongoc_uri_new_with_error (
      "mongodb://localhost/?readPreference=unknown", &error));
   ASSERT (error.domain == MONGOC_ERROR_COMMAND);

   error.domain = 0;
   ASSERT (!mongoc_uri_new_with_error (
      "mongodb://localhost/"
      "?appname="
      "WayTooLongAppnameToBeValidSoThisShouldResultInAnErrorWayToLongAppnameToB"
      "eValidSoThisShouldResultInAnErrorWayToLongAppnameToBeValidSoThisShouldRe"
      "sultInAnError",
      &error));
   ASSERT (error.domain == MONGOC_ERROR_COMMAND);

   uri = mongoc_uri_new ("mongodb://localhost");
   ASSERT (!mongoc_uri_set_option_as_utf8 (
      uri,
      MONGOC_URI_APPNAME,
      "WayTooLongAppnameToBeValidSoThisShouldResultInAnErrorWayToLongAppnameToB"
      "eValidSoThisShouldResultInAnErrorWayToLongAppnameToBeValidSoThisShouldRe"
      "sultInAnError"));
   mongoc_uri_destroy (uri);

   error.domain = 0;
   ASSERT (
      !mongoc_uri_new_with_error ("mongodb://user%p:pass@localhost/", &error));
   ASSERT (error.domain == MONGOC_ERROR_COMMAND);

   error.domain = 0;
   ASSERT (!mongoc_uri_new_with_error ("mongodb://l%oc, alhost/", &error));
   ASSERT (error.domain == MONGOC_ERROR_COMMAND);

   error.domain = 0;
   ASSERT (!mongoc_uri_new_with_error ("mongodb:///tmp/mongodb.sock", &error));
   ASSERT (error.domain == MONGOC_ERROR_COMMAND);

   error.domain = 0;
   ASSERT (!mongoc_uri_new_with_error ("mongodb://localhost/db.na%me", &error));
   ASSERT (error.domain == MONGOC_ERROR_COMMAND);
}


#undef ASSERT_SUPPRESS


static void
test_mongoc_uri_compound_setters (void)
{
   mongoc_uri_t *uri;
   mongoc_read_prefs_t *prefs;
   const mongoc_read_prefs_t *prefs_result;
   mongoc_read_concern_t *rc;
   const mongoc_read_concern_t *rc_result;
   mongoc_write_concern_t *wc;
   const mongoc_write_concern_t *wc_result;

   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_READPREFERENCE
                         "=nearest&" MONGOC_URI_READPREFERENCETAGS
                         "=dc:ny&" MONGOC_URI_READCONCERNLEVEL
                         "=majority&" MONGOC_URI_W "=3");

   prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   mongoc_uri_set_read_prefs_t (uri, prefs);
   prefs_result = mongoc_uri_get_read_prefs_t (uri);
   ASSERT_CMPINT (
      mongoc_read_prefs_get_mode (prefs_result), ==, MONGOC_READ_SECONDARY);
   ASSERT (bson_empty (mongoc_read_prefs_get_tags (prefs_result)));

   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, "whatever");
   mongoc_uri_set_read_concern (uri, rc);
   rc_result = mongoc_uri_get_read_concern (uri);
   ASSERT_CMPSTR (mongoc_read_concern_get_level (rc_result), "whatever");

   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, 2);
   mongoc_uri_set_write_concern (uri, wc);
   wc_result = mongoc_uri_get_write_concern (uri);
   ASSERT_CMPINT32 (mongoc_write_concern_get_w (wc_result), ==, (int32_t) 2);

   mongoc_read_prefs_destroy (prefs);
   mongoc_read_concern_destroy (rc);
   mongoc_write_concern_destroy (wc);
   mongoc_uri_destroy (uri);
}


static void
test_mongoc_host_list_from_string (void)
{
   mongoc_host_list_t host_list = {0};

   /* shouldn't be parsable */
   ASSERT (!_mongoc_host_list_from_string (&host_list, ":27017"));
   ASSERT (!_mongoc_host_list_from_string (&host_list, "example.com:"));
   ASSERT (!_mongoc_host_list_from_string (&host_list, "localhost:999999999"));

   /* normal parsing, host and port are split, host is downcased */
   ASSERT (_mongoc_host_list_from_string (&host_list, "localHOST:27019"));
   ASSERT_CMPSTR (host_list.host_and_port, "localhost:27019");
   ASSERT_CMPSTR (host_list.host, "localhost");
   ASSERT (host_list.port == 27019);
   ASSERT (!host_list.next);

   ASSERT (_mongoc_host_list_from_string (&host_list, "localhost"));
   ASSERT_CMPSTR (host_list.host_and_port, "localhost:27017");
   ASSERT_CMPSTR (host_list.host, "localhost");
   ASSERT (host_list.port == 27017);

   ASSERT (_mongoc_host_list_from_string (&host_list, "[::1]"));
   ASSERT_CMPSTR (host_list.host_and_port, "[::1]:27017");
   ASSERT_CMPSTR (host_list.host, "::1"); /* no "[" or "]" */
   ASSERT (host_list.port == 27017);

   ASSERT (_mongoc_host_list_from_string (&host_list, "[Fe80::1]:1234"));
   ASSERT_CMPSTR (host_list.host_and_port, "[fe80::1]:1234");
   ASSERT_CMPSTR (host_list.host, "fe80::1");
   ASSERT (host_list.port == 1234);

   ASSERT (_mongoc_host_list_from_string (&host_list, "[fe80::1%lo0]:1234"));
   ASSERT_CMPSTR (host_list.host_and_port, "[fe80::1%lo0]:1234");
   ASSERT_CMPSTR (host_list.host, "fe80::1%lo0");
   ASSERT (host_list.port == 1234);

   ASSERT (_mongoc_host_list_from_string (&host_list, "[fe80::1%lo0]:1234"));
   ASSERT_CMPSTR (host_list.host_and_port, "[fe80::1%lo0]:1234");
   ASSERT_CMPSTR (host_list.host, "fe80::1%lo0");
   ASSERT (host_list.port == 1234);

   /* preserves case */
   ASSERT (_mongoc_host_list_from_string (&host_list, "/Path/to/file.sock"));
   ASSERT_CMPSTR (host_list.host_and_port, "/Path/to/file.sock");
   ASSERT_CMPSTR (host_list.host, "/Path/to/file.sock");

   /* weird cases that should still parse, without crashing */
   ASSERT (_mongoc_host_list_from_string (&host_list, "/Path/to/file.sock:1"));
   ASSERT_CMPSTR (host_list.host, "/Path/to/file.sock");
   ASSERT (host_list.family == AF_UNIX);

   ASSERT (_mongoc_host_list_from_string (&host_list, " :1234"));
   ASSERT_CMPSTR (host_list.host_and_port, " :1234");
   ASSERT_CMPSTR (host_list.host, " ");
   ASSERT (host_list.port == 1234);

   ASSERT (_mongoc_host_list_from_string (&host_list, "::1234"));
   ASSERT_CMPSTR (host_list.host_and_port, "::1234");
   ASSERT_CMPSTR (host_list.host, ":");
   ASSERT (host_list.port == 1234);

   ASSERT (_mongoc_host_list_from_string (&host_list, "]:1234"));
   ASSERT_CMPSTR (host_list.host_and_port, "]:1234");
   ASSERT_CMPSTR (host_list.host, "]");
   ASSERT (host_list.port == 1234);

   ASSERT (_mongoc_host_list_from_string (&host_list, "[:1234"));
   ASSERT_CMPSTR (host_list.host_and_port, "[:1234");
   ASSERT_CMPSTR (host_list.host, "[");
   ASSERT (host_list.port == 1234);

   ASSERT (_mongoc_host_list_from_string (&host_list, "[]:1234"));
   ASSERT_CMPSTR (host_list.host_and_port, "[]:1234");
   ASSERT_CMPSTR (host_list.host, "");
   ASSERT (host_list.port == 1234);

   ASSERT (_mongoc_host_list_from_string (&host_list, "[:]"));
   ASSERT_CMPSTR (host_list.host_and_port, "[:]:27017");
   ASSERT_CMPSTR (host_list.host, ":");
   ASSERT (host_list.port == 27017);

   ASSERT (_mongoc_host_list_from_string (&host_list, "[::1] foo"));
   ASSERT_CMPSTR (host_list.host_and_port, "[::1] foo:27017");
   ASSERT_CMPSTR (host_list.host, "[::1] foo");
   ASSERT (host_list.port == 27017);
   ASSERT (host_list.family == AF_INET);
}


static void
test_mongoc_uri_new_for_host_port (void)
{
   mongoc_uri_t *uri;

   uri = mongoc_uri_new_for_host_port ("uber", 555);
   ASSERT (uri);
   ASSERT (!strcmp ("uber", mongoc_uri_get_hosts (uri)->host));
   ASSERT (!strcmp ("uber:555", mongoc_uri_get_hosts (uri)->host_and_port));
   ASSERT (555 == mongoc_uri_get_hosts (uri)->port);
   mongoc_uri_destroy (uri);
}

static void
test_mongoc_uri_compressors (void)
{
   mongoc_uri_t *uri;

   uri = mongoc_uri_new ("mongodb://localhost/");

#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
   capture_logs (true);
   mongoc_uri_set_compressors (uri, "snappy,unknown");
   ASSERT (bson_has_field (mongoc_uri_get_compressors (uri), "snappy"));
   ASSERT (!bson_has_field (mongoc_uri_get_compressors (uri), "unknown"));
   ASSERT_CAPTURED_LOG ("mongoc_uri_set_compressors",
                        MONGOC_LOG_LEVEL_WARNING,
                        "Unsupported compressor: 'unknown'");
#endif


#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
   capture_logs (true);
   mongoc_uri_set_compressors (uri, "snappy");
   ASSERT (bson_has_field (mongoc_uri_get_compressors (uri), "snappy"));
   ASSERT (!bson_has_field (mongoc_uri_get_compressors (uri), "unknown"));
   ASSERT_NO_CAPTURED_LOGS ("snappy uri");

   /* Overwrite the previous URI, effectively disabling snappy */
   capture_logs (true);
   mongoc_uri_set_compressors (uri, "unknown");
   ASSERT (!bson_has_field (mongoc_uri_get_compressors (uri), "snappy"));
   ASSERT (!bson_has_field (mongoc_uri_get_compressors (uri), "unknown"));
   ASSERT_CAPTURED_LOG ("mongoc_uri_set_compressors",
                        MONGOC_LOG_LEVEL_WARNING,
                        "Unsupported compressor: 'unknown'");
#endif

   capture_logs (true);
   mongoc_uri_set_compressors (uri, "");
   ASSERT (bson_empty (mongoc_uri_get_compressors (uri)));
   ASSERT_CAPTURED_LOG ("mongoc_uri_set_compressors",
                        MONGOC_LOG_LEVEL_WARNING,
                        "Unsupported compressor: ''");


   /* Disable compression */
   capture_logs (true);
   mongoc_uri_set_compressors (uri, NULL);
   ASSERT (bson_empty (mongoc_uri_get_compressors (uri)));
   ASSERT_NO_CAPTURED_LOGS ("Disable compression");


   mongoc_uri_destroy (uri);


#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
   uri = mongoc_uri_new ("mongodb://localhost/?compressors=snappy");
   ASSERT (bson_has_field (mongoc_uri_get_compressors (uri), "snappy"));
   mongoc_uri_destroy (uri);

   capture_logs (true);
   uri =
      mongoc_uri_new ("mongodb://localhost/?compressors=snappy,somethingElse");
   ASSERT (bson_has_field (mongoc_uri_get_compressors (uri), "snappy"));
   ASSERT (!bson_has_field (mongoc_uri_get_compressors (uri), "somethingElse"));
   ASSERT_CAPTURED_LOG ("mongoc_uri_set_compressors",
                        MONGOC_LOG_LEVEL_WARNING,
                        "Unsupported compressor: 'somethingElse'");
   mongoc_uri_destroy (uri);
#endif


#ifdef MONGOC_ENABLE_COMPRESSION_ZLIB

#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
   uri = mongoc_uri_new ("mongodb://localhost/?compressors=snappy,zlib");
   ASSERT (bson_has_field (mongoc_uri_get_compressors (uri), "snappy"));
   ASSERT (bson_has_field (mongoc_uri_get_compressors (uri), "zlib"));
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/");
   ASSERT (mongoc_uri_set_compressors (uri, "snappy,zlib"));
   ASSERT (bson_has_field (mongoc_uri_get_compressors (uri), "snappy"));
   ASSERT (bson_has_field (mongoc_uri_get_compressors (uri), "zlib"));
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/");
   ASSERT (mongoc_uri_set_compressors (uri, "zlib"));
   ASSERT (mongoc_uri_set_compressors (uri, "snappy"));
   ASSERT (bson_has_field (mongoc_uri_get_compressors (uri), "snappy"));
   ASSERT (!bson_has_field (mongoc_uri_get_compressors (uri), "zlib"));
   mongoc_uri_destroy (uri);
#endif

   uri = mongoc_uri_new ("mongodb://localhost/?compressors=zlib");
   ASSERT (bson_has_field (mongoc_uri_get_compressors (uri), "zlib"));
   mongoc_uri_destroy (uri);

   capture_logs (true);
   uri = mongoc_uri_new ("mongodb://localhost/?compressors=zlib,somethingElse");
   ASSERT (bson_has_field (mongoc_uri_get_compressors (uri), "zlib"));
   ASSERT (!bson_has_field (mongoc_uri_get_compressors (uri), "somethingElse"));
   ASSERT_CAPTURED_LOG ("mongoc_uri_set_compressors",
                        MONGOC_LOG_LEVEL_WARNING,
                        "Unsupported compressor: 'somethingElse'");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new (
      "mongodb://localhost/?compressors=zlib&zlibCompressionLevel=-1");
   ASSERT (bson_has_field (mongoc_uri_get_compressors (uri), "zlib"));
   ASSERT_CMPINT32 (
      mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_ZLIBCOMPRESSIONLEVEL, 1),
      ==,
      -1);
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new (
      "mongodb://localhost/?compressors=zlib&zlibCompressionLevel=9");
   ASSERT_CMPINT32 (
      mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_ZLIBCOMPRESSIONLEVEL, 1),
      ==,
      9);
   mongoc_uri_destroy (uri);

   capture_logs (true);
   uri = mongoc_uri_new (
      "mongodb://localhost/?compressors=zlib&zlibCompressionLevel=-2");
   ASSERT_CAPTURED_LOG (
      "mongoc_uri_set_compressors",
      MONGOC_LOG_LEVEL_WARNING,
      "Invalid \"zlibcompressionlevel\" of -2: must be between -1 and 9");
   mongoc_uri_destroy (uri);

   capture_logs (true);
   uri = mongoc_uri_new (
      "mongodb://localhost/?compressors=zlib&zlibCompressionLevel=10");
   ASSERT_CAPTURED_LOG (
      "mongoc_uri_set_compressors",
      MONGOC_LOG_LEVEL_WARNING,
      "Invalid \"zlibcompressionlevel\" of 10: must be between -1 and 9");
   mongoc_uri_destroy (uri);

#endif
}

static void
test_mongoc_uri_unescape (void)
{
#define ASSERT_URIDECODE_STR(_s, _e)        \
   do {                                     \
      char *str = mongoc_uri_unescape (_s); \
      ASSERT (!strcmp (str, _e));           \
      bson_free (str);                      \
   } while (0)
#define ASSERT_URIDECODE_FAIL(_s)                                       \
   do {                                                                 \
      char *str;                                                        \
      capture_logs (true);                                              \
      str = mongoc_uri_unescape (_s);                                   \
      ASSERT (!str);                                                    \
      ASSERT_CAPTURED_LOG (                                             \
         "uri", MONGOC_LOG_LEVEL_WARNING, "Invalid % escape sequence"); \
   } while (0)

   ASSERT_URIDECODE_STR ("", "");
   ASSERT_URIDECODE_STR ("%40", "@");
   ASSERT_URIDECODE_STR ("me%40localhost@localhost", "me@localhost@localhost");
   ASSERT_URIDECODE_STR ("%20", " ");
   ASSERT_URIDECODE_STR ("%24%21%40%2A%26%5E%21%40%2A%23%26%5E%21%40%23%2A%26"
                         "%5E%21%40%2A%23%26%5E%21%40%2A%26%23%5E%7D%7B%7D%7B"
                         "%22%22%27%7D%7B%5B%5D%3C%3E%3F",
                         "$!@*&^!@*#&^!@#*&^!@*#&^!@*&#^}{}{\"\"'}{[]<>?");

   ASSERT_URIDECODE_FAIL ("%");
   ASSERT_URIDECODE_FAIL ("%%");
   ASSERT_URIDECODE_FAIL ("%%%");
   ASSERT_URIDECODE_FAIL ("%FF");
   ASSERT_URIDECODE_FAIL ("%CC");
   ASSERT_URIDECODE_FAIL ("%00");

#undef ASSERT_URIDECODE_STR
#undef ASSERT_URIDECODE_FAIL
}


typedef struct {
   const char *uri;
   bool parses;
   mongoc_read_mode_t mode;
   bson_t *tags;
   const char *log_msg;
} read_prefs_test;


static void
test_mongoc_uri_read_prefs (void)
{
   const mongoc_read_prefs_t *rp;
   mongoc_uri_t *uri;
   const read_prefs_test *t;
   int i;

   bson_t *tags_dcny = BCON_NEW ("0", "{", "dc", "ny", "}");
   bson_t *tags_dcny_empty =
      BCON_NEW ("0", "{", "dc", "ny", "}", "1", "{", "}");
   bson_t *tags_dcnyusessd_dcsf_empty = BCON_NEW ("0",
                                                  "{",
                                                  "dc",
                                                  "ny",
                                                  "use",
                                                  "ssd",
                                                  "}",
                                                  "1",
                                                  "{",
                                                  "dc",
                                                  "sf",
                                                  "}",
                                                  "2",
                                                  "{",
                                                  "}");
   bson_t *tags_empty = BCON_NEW ("0", "{", "}");

   const char *conflicts = "Invalid readPreferences";

   const read_prefs_test tests[] = {
      {"mongodb://localhost/", true, MONGOC_READ_PRIMARY, NULL},
      {"mongodb://localhost/?" MONGOC_URI_SLAVEOK "=false",
       true,
       MONGOC_READ_PRIMARY,
       NULL},
      {"mongodb://localhost/?" MONGOC_URI_SLAVEOK "=true",
       true,
       MONGOC_READ_SECONDARY_PREFERRED,
       NULL},
      {"mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=primary",
       true,
       MONGOC_READ_PRIMARY,
       NULL},
      {"mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=primaryPreferred",
       true,
       MONGOC_READ_PRIMARY_PREFERRED,
       NULL},
      {"mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=secondary",
       true,
       MONGOC_READ_SECONDARY,
       NULL},
      {"mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=secondaryPreferred",
       true,
       MONGOC_READ_SECONDARY_PREFERRED,
       NULL},
      {"mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=nearest",
       true,
       MONGOC_READ_NEAREST,
       NULL},
      /* MONGOC_URI_READPREFERENCE should take priority over "
         MONGOC_URI_SLAVEOK " */
      {"mongodb://localhost/?" MONGOC_URI_SLAVEOK
       "=false&" MONGOC_URI_READPREFERENCE "=secondary",
       true,
       MONGOC_READ_SECONDARY,
       NULL},
      /* MONGOC_URI_READPREFERENCETAGS conflict with primary mode */
      {"mongodb://localhost/?" MONGOC_URI_READPREFERENCETAGS "=",
       false,
       MONGOC_READ_PRIMARY,
       NULL,
       conflicts},
      {"mongodb://localhost/?" MONGOC_URI_READPREFERENCE
       "=primary&" MONGOC_URI_READPREFERENCETAGS "=",
       false,
       MONGOC_READ_PRIMARY,
       NULL,
       conflicts},
      {"mongodb://localhost/?" MONGOC_URI_SLAVEOK
       "=false&" MONGOC_URI_READPREFERENCETAGS "=",
       false,
       MONGOC_READ_PRIMARY,
       NULL,
       conflicts},
      {"mongodb://localhost/"
       "?" MONGOC_URI_READPREFERENCE
       "=secondaryPreferred&" MONGOC_URI_READPREFERENCETAGS "=",
       true,
       MONGOC_READ_SECONDARY_PREFERRED,
       tags_empty},
      {"mongodb://localhost/"
       "?" MONGOC_URI_READPREFERENCE
       "=secondaryPreferred&" MONGOC_URI_READPREFERENCETAGS "=dc:ny",
       true,
       MONGOC_READ_SECONDARY_PREFERRED,
       tags_dcny},
      {"mongodb://localhost/"
       "?" MONGOC_URI_READPREFERENCE "=nearest&" MONGOC_URI_READPREFERENCETAGS
       "=dc:ny&" MONGOC_URI_READPREFERENCETAGS "=",
       true,
       MONGOC_READ_NEAREST,
       tags_dcny_empty},
      {"mongodb://localhost/"
       "?" MONGOC_URI_READPREFERENCE "=nearest&" MONGOC_URI_READPREFERENCETAGS
       "=dc:ny,use:ssd&" MONGOC_URI_READPREFERENCETAGS
       "=dc:sf&" MONGOC_URI_READPREFERENCETAGS "=",
       true,
       MONGOC_READ_NEAREST,
       tags_dcnyusessd_dcsf_empty},
      {"mongodb://localhost/?" MONGOC_URI_READPREFERENCE
       "=nearest&" MONGOC_URI_READPREFERENCETAGS "=foo",
       false,
       MONGOC_READ_NEAREST,
       NULL,
       "Unsupported value for \"" MONGOC_URI_READPREFERENCETAGS "\": \"foo\""},
      {"mongodb://localhost/?" MONGOC_URI_READPREFERENCE
       "=nearest&" MONGOC_URI_READPREFERENCETAGS "=foo,bar",
       false,
       MONGOC_READ_NEAREST,
       NULL,
       "Unsupported value for \"" MONGOC_URI_READPREFERENCETAGS
       "\": \"foo,bar\""},
      {"mongodb://localhost/?" MONGOC_URI_READPREFERENCE
       "=nearest&" MONGOC_URI_READPREFERENCETAGS "=1",
       false,
       MONGOC_READ_NEAREST,
       NULL,
       "Unsupported value for \"" MONGOC_URI_READPREFERENCETAGS "\": \"1\""},
      {NULL}};

   for (i = 0; tests[i].uri; i++) {
      t = &tests[i];

      capture_logs (true);
      uri = mongoc_uri_new (t->uri);
      if (t->parses) {
         BSON_ASSERT (uri);
         ASSERT_NO_CAPTURED_LOGS (t->uri);
      } else {
         BSON_ASSERT (!uri);
         if (t->log_msg) {
            ASSERT_CAPTURED_LOG (t->uri, MONGOC_LOG_LEVEL_WARNING, t->log_msg);
         }

         continue;
      }

      rp = mongoc_uri_get_read_prefs_t (uri);
      BSON_ASSERT (rp);

      BSON_ASSERT (t->mode == mongoc_read_prefs_get_mode (rp));

      if (t->tags) {
         BSON_ASSERT (bson_equal (t->tags, mongoc_read_prefs_get_tags (rp)));
      }

      mongoc_uri_destroy (uri);
   }

   bson_destroy (tags_dcny);
   bson_destroy (tags_dcny_empty);
   bson_destroy (tags_dcnyusessd_dcsf_empty);
   bson_destroy (tags_empty);
}


typedef struct {
   const char *uri;
   bool parses;
   int32_t w;
   const char *wtag;
   int32_t wtimeoutms;
   const char *log_msg;
} write_concern_test;


static void
test_mongoc_uri_write_concern (void)
{
   const mongoc_write_concern_t *wr;
   mongoc_uri_t *uri;
   const write_concern_test *t;
   int i;
   static const write_concern_test tests[] = {
      {"mongodb://localhost/?" MONGOC_URI_SAFE "=false",
       true,
       MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED},
      {"mongodb://localhost/?" MONGOC_URI_SAFE "=true", true, 1},
      {"mongodb://localhost/?" MONGOC_URI_W "=-1",
       true,
       MONGOC_WRITE_CONCERN_W_ERRORS_IGNORED},
      {"mongodb://localhost/?" MONGOC_URI_W "=0",
       true,
       MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED},
      {"mongodb://localhost/?" MONGOC_URI_W "=1", true, 1},
      {"mongodb://localhost/?" MONGOC_URI_W "=2", true, 2},
      {"mongodb://localhost/?" MONGOC_URI_W "=majority",
       true,
       MONGOC_WRITE_CONCERN_W_MAJORITY},
      {"mongodb://localhost/?" MONGOC_URI_W "=10", true, 10},
      {"mongodb://localhost/?" MONGOC_URI_W "=",
       true,
       MONGOC_WRITE_CONCERN_W_DEFAULT},
      {"mongodb://localhost/?" MONGOC_URI_W "=mytag",
       true,
       MONGOC_WRITE_CONCERN_W_TAG,
       "mytag"},
      {"mongodb://localhost/?" MONGOC_URI_W "=mytag&" MONGOC_URI_SAFE "=false",
       true,
       MONGOC_WRITE_CONCERN_W_TAG,
       "mytag"},
      {"mongodb://localhost/?" MONGOC_URI_W "=1&" MONGOC_URI_SAFE "=false",
       true,
       1},
      {"mongodb://localhost/?" MONGOC_URI_JOURNAL "=true",
       true,
       MONGOC_WRITE_CONCERN_W_DEFAULT},
      {"mongodb://localhost/?" MONGOC_URI_W "=1&" MONGOC_URI_JOURNAL "=true",
       true,
       1},
      {"mongodb://localhost/?" MONGOC_URI_W "=2&" MONGOC_URI_WTIMEOUTMS "=1000",
       true,
       2,
       NULL,
       1000},
      {"mongodb://localhost/?" MONGOC_URI_W "=majority&" MONGOC_URI_WTIMEOUTMS
       "=1000",
       true,
       MONGOC_WRITE_CONCERN_W_MAJORITY,
       NULL,
       1000},
      {"mongodb://localhost/?" MONGOC_URI_W "=mytag&" MONGOC_URI_WTIMEOUTMS
       "=1000",
       true,
       MONGOC_WRITE_CONCERN_W_TAG,
       "mytag",
       1000},
      {"mongodb://localhost/?" MONGOC_URI_W "=0&" MONGOC_URI_JOURNAL "=true",
       false,
       MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED,
       NULL,
       0,
       "Journal conflicts with w value [" MONGOC_URI_W "=0]"},
      {"mongodb://localhost/?" MONGOC_URI_W "=-1&" MONGOC_URI_JOURNAL "=true",
       false,
       MONGOC_WRITE_CONCERN_W_ERRORS_IGNORED,
       NULL,
       0,
       "Journal conflicts with w value [" MONGOC_URI_W "=-1]"},
      {NULL}};

   for (i = 0; tests[i].uri; i++) {
      t = &tests[i];

      capture_logs (true);
      uri = mongoc_uri_new (t->uri);

      if (tests[i].log_msg) {
         ASSERT_CAPTURED_LOG (
            tests[i].uri, MONGOC_LOG_LEVEL_WARNING, tests[i].log_msg);
      } else {
         ASSERT_NO_CAPTURED_LOGS (tests[i].uri);
      }

      capture_logs (false); /* clear captured logs */

      if (t->parses) {
         BSON_ASSERT (uri);
      } else {
         BSON_ASSERT (!uri);
         continue;
      }

      wr = mongoc_uri_get_write_concern (uri);
      BSON_ASSERT (wr);

      BSON_ASSERT (t->w == mongoc_write_concern_get_w (wr));

      if (t->wtag) {
         BSON_ASSERT (0 ==
                      strcmp (t->wtag, mongoc_write_concern_get_wtag (wr)));
      }

      if (t->wtimeoutms) {
         BSON_ASSERT (t->wtimeoutms == mongoc_write_concern_get_wtimeout (wr));
      }

      mongoc_uri_destroy (uri);
   }
}

static void
test_mongoc_uri_read_concern (void)
{
   const mongoc_read_concern_t *rc;
   mongoc_uri_t *uri;

   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_READCONCERNLEVEL
                         "=majority");
   rc = mongoc_uri_get_read_concern (uri);
   ASSERT_CMPSTR (mongoc_read_concern_get_level (rc), "majority");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/"
                         "?" MONGOC_URI_READCONCERNLEVEL
                         "=" MONGOC_READ_CONCERN_LEVEL_MAJORITY);
   rc = mongoc_uri_get_read_concern (uri);
   ASSERT_CMPSTR (mongoc_read_concern_get_level (rc), "majority");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/"
                         "?" MONGOC_URI_READCONCERNLEVEL
                         "=" MONGOC_READ_CONCERN_LEVEL_LINEARIZABLE);
   rc = mongoc_uri_get_read_concern (uri);
   ASSERT_CMPSTR (mongoc_read_concern_get_level (rc), "linearizable");
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_READCONCERNLEVEL
                         "=local");
   rc = mongoc_uri_get_read_concern (uri);
   ASSERT_CMPSTR (mongoc_read_concern_get_level (rc), "local");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_READCONCERNLEVEL
                         "=" MONGOC_READ_CONCERN_LEVEL_LOCAL);
   rc = mongoc_uri_get_read_concern (uri);
   ASSERT_CMPSTR (mongoc_read_concern_get_level (rc), "local");
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_READCONCERNLEVEL
                         "=randomstuff");
   rc = mongoc_uri_get_read_concern (uri);
   ASSERT_CMPSTR (mongoc_read_concern_get_level (rc), "randomstuff");
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new ("mongodb://localhost/");
   rc = mongoc_uri_get_read_concern (uri);
   ASSERT (mongoc_read_concern_get_level (rc) == NULL);
   mongoc_uri_destroy (uri);


   uri =
      mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_READCONCERNLEVEL "=");
   rc = mongoc_uri_get_read_concern (uri);
   ASSERT_CMPSTR (mongoc_read_concern_get_level (rc), "");
   mongoc_uri_destroy (uri);
}

static void
test_mongoc_uri_long_hostname (void)
{
   char *host;
   char *host_and_port;
   size_t len = BSON_HOST_NAME_MAX;
   char *uri_str;
   mongoc_uri_t *uri;

   /* hostname of exactly maximum length */
   host = bson_malloc (len + 1);
   memset (host, 'a', len);
   host[len] = '\0';
   host_and_port = bson_strdup_printf ("%s:12345", host);
   uri_str = bson_strdup_printf ("mongodb://%s", host_and_port);
   uri = mongoc_uri_new (uri_str);
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->host_and_port, host_and_port);

   mongoc_uri_destroy (uri);
   uri = mongoc_uri_new_for_host_port (host, 12345);
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->host_and_port, host_and_port);

   mongoc_uri_destroy (uri);
   bson_free (uri_str);
   bson_free (host_and_port);
   bson_free (host);

   /* hostname length exceeds maximum by one */
   len++;
   host = bson_malloc (len + 1);
   memset (host, 'a', len);
   host[len] = '\0';
   host_and_port = bson_strdup_printf ("%s:12345", host);
   uri_str = bson_strdup_printf ("mongodb://%s", host_and_port);

   capture_logs (true);
   ASSERT (!mongoc_uri_new (uri_str));
   ASSERT_CAPTURED_LOG ("mongoc_uri_new", MONGOC_LOG_LEVEL_ERROR, "too long");

   clear_captured_logs ();
   ASSERT (!mongoc_uri_new_for_host_port (host, 12345));
   ASSERT_CAPTURED_LOG ("mongoc_uri_new", MONGOC_LOG_LEVEL_ERROR, "too long");

   bson_free (uri_str);
   bson_free (host_and_port);
   bson_free (host);
}

static void
test_mongoc_uri_ssl (void)
{
   mongoc_uri_t *uri;

   uri = mongoc_uri_new (
      "mongodb://CN=client,OU=kerneluser,O=10Gen,L=New York City,ST=New "
      "York,C=US@ldaptest.10gen.cc/"
      "?ssl=true&" MONGOC_URI_AUTHMECHANISM
      "=MONGODB-X509&" MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE "=tests/"
      "x509gen/legacy-x509.pem&" MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE
      "=tests/x509gen/"
      "legacy-ca.crt&" MONGOC_URI_SSLALLOWINVALIDHOSTNAMES "=true");

   ASSERT_CMPSTR (
      mongoc_uri_get_username (uri),
      "CN=client,OU=kerneluser,O=10Gen,L=New York City,ST=New York,C=US");
   ASSERT (!mongoc_uri_get_password (uri));
   ASSERT_CMPSTR (mongoc_uri_get_database (uri), "");
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "$external");
   ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), "MONGODB-X509");

   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (
                     uri, MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE, "none"),
                  "tests/x509gen/legacy-x509.pem");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (
                     uri, MONGOC_URI_SSLCLIENTCERTIFICATEKEYPASSWORD, "none"),
                  "none");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (
                     uri, MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE, "none"),
                  "tests/x509gen/legacy-ca.crt");
   ASSERT (!mongoc_uri_get_option_as_bool (
      uri, MONGOC_URI_SSLALLOWINVALIDCERTIFICATES, false));
   ASSERT (mongoc_uri_get_option_as_bool (
      uri, MONGOC_URI_SSLALLOWINVALIDHOSTNAMES, false));
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new ("mongodb://localhost/"
                         "?ssl=true&" MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE
                         "=key.pem&"
                         "" MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE "=ca.pem");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (
                     uri, MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE, "none"),
                  "key.pem");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (
                     uri, MONGOC_URI_SSLCLIENTCERTIFICATEKEYPASSWORD, "none"),
                  "none");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (
                     uri, MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE, "none"),
                  "ca.pem");
   ASSERT (!mongoc_uri_get_option_as_bool (
      uri, MONGOC_URI_SSLALLOWINVALIDCERTIFICATES, false));
   ASSERT (!mongoc_uri_get_option_as_bool (
      uri, MONGOC_URI_SSLALLOWINVALIDHOSTNAMES, false));
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new ("mongodb://localhost/?ssl=true");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (
                     uri, MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE, "none"),
                  "none");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (
                     uri, MONGOC_URI_SSLCLIENTCERTIFICATEKEYPASSWORD, "none"),
                  "none");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (
                     uri, MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE, "none"),
                  "none");
   ASSERT (!mongoc_uri_get_option_as_bool (
      uri, MONGOC_URI_SSLALLOWINVALIDCERTIFICATES, false));
   ASSERT (!mongoc_uri_get_option_as_bool (
      uri, MONGOC_URI_SSLALLOWINVALIDHOSTNAMES, false));
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new (
      "mongodb://localhost/"
      "?ssl=true&" MONGOC_URI_SSLCLIENTCERTIFICATEKEYPASSWORD "=pa$$word!&"
      "" MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE "=encrypted.pem");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (
                     uri, MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE, "none"),
                  "encrypted.pem");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (
                     uri, MONGOC_URI_SSLCLIENTCERTIFICATEKEYPASSWORD, "none"),
                  "pa$$word!");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (
                     uri, MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE, "none"),
                  "none");
   ASSERT (!mongoc_uri_get_option_as_bool (
      uri, MONGOC_URI_SSLALLOWINVALIDCERTIFICATES, false));
   ASSERT (!mongoc_uri_get_option_as_bool (
      uri, MONGOC_URI_SSLALLOWINVALIDHOSTNAMES, false));
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new (
      "mongodb://localhost/?ssl=true&" MONGOC_URI_SSLALLOWINVALIDCERTIFICATES
      "=true");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (
                     uri, MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE, "none"),
                  "none");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (
                     uri, MONGOC_URI_SSLCLIENTCERTIFICATEKEYPASSWORD, "none"),
                  "none");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (
                     uri, MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE, "none"),
                  "none");
   ASSERT (mongoc_uri_get_option_as_bool (
      uri, MONGOC_URI_SSLALLOWINVALIDCERTIFICATES, false));
   ASSERT (!mongoc_uri_get_option_as_bool (
      uri, MONGOC_URI_SSLALLOWINVALIDHOSTNAMES, false));
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new (
      "mongodb://localhost/?" MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE
      "=foo.pem");
   ASSERT (mongoc_uri_get_ssl (uri));
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new (
      "mongodb://localhost/?" MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE
      "=foo.pem");
   ASSERT (mongoc_uri_get_ssl (uri));
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new (
      "mongodb://localhost/?" MONGOC_URI_SSLALLOWINVALIDCERTIFICATES "=true");
   ASSERT (mongoc_uri_get_ssl (uri));
   ASSERT (mongoc_uri_get_option_as_bool (
      uri, MONGOC_URI_SSLALLOWINVALIDCERTIFICATES, false));
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new (
      "mongodb://localhost/?" MONGOC_URI_SSLALLOWINVALIDHOSTNAMES "=false");
   ASSERT (mongoc_uri_get_ssl (uri));
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new (
      "mongodb://localhost/?ssl=false&" MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE
      "=foo.pem");
   ASSERT (!mongoc_uri_get_ssl (uri));
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new (
      "mongodb://localhost/?ssl=false&" MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE
      "=foo.pem");
   ASSERT (!mongoc_uri_get_ssl (uri));
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new (
      "mongodb://localhost/?ssl=false&" MONGOC_URI_SSLALLOWINVALIDCERTIFICATES
      "=true");
   ASSERT (!mongoc_uri_get_ssl (uri));
   ASSERT (mongoc_uri_get_option_as_bool (
      uri, MONGOC_URI_SSLALLOWINVALIDCERTIFICATES, false));
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new (
      "mongodb://localhost/?ssl=false&" MONGOC_URI_SSLALLOWINVALIDHOSTNAMES
      "=false");
   ASSERT (!mongoc_uri_get_ssl (uri));
   mongoc_uri_destroy (uri);
}


static void
test_mongoc_uri_local_threshold_ms (void)
{
   mongoc_uri_t *uri;

   uri = mongoc_uri_new ("mongodb://localhost/");

   /* localthresholdms isn't set, return the default */
   ASSERT_CMPINT (mongoc_uri_get_local_threshold_option (uri),
                  ==,
                  MONGOC_TOPOLOGY_LOCAL_THRESHOLD_MS);
   ASSERT (
      mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_LOCALTHRESHOLDMS, 99));
   ASSERT_CMPINT (mongoc_uri_get_local_threshold_option (uri), ==, 99);

   mongoc_uri_destroy (uri);

   uri =
      mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_LOCALTHRESHOLDMS "=0");

   ASSERT_CMPINT (mongoc_uri_get_local_threshold_option (uri), ==, 0);
   ASSERT (
      mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_LOCALTHRESHOLDMS, 99));
   ASSERT_CMPINT (mongoc_uri_get_local_threshold_option (uri), ==, 99);

   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_LOCALTHRESHOLDMS
                         "=-1");

   /* localthresholdms is invalid, return the default */
   capture_logs (true);
   ASSERT_CMPINT (mongoc_uri_get_local_threshold_option (uri),
                  ==,
                  MONGOC_TOPOLOGY_LOCAL_THRESHOLD_MS);
   ASSERT_CAPTURED_LOG ("mongoc_uri_get_local_threshold_option",
                        MONGOC_LOG_LEVEL_WARNING,
                        "Invalid localThresholdMS: -1");

   mongoc_uri_destroy (uri);
}


void
test_uri_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/Uri/new", test_mongoc_uri_new);
   TestSuite_Add (suite, "/Uri/new_with_error", test_mongoc_uri_new_with_error);
   TestSuite_Add (
      suite, "/Uri/new_for_host_port", test_mongoc_uri_new_for_host_port);
   TestSuite_Add (suite, "/Uri/compressors", test_mongoc_uri_compressors);
   TestSuite_Add (suite, "/Uri/unescape", test_mongoc_uri_unescape);
   TestSuite_Add (suite, "/Uri/read_prefs", test_mongoc_uri_read_prefs);
   TestSuite_Add (suite, "/Uri/read_concern", test_mongoc_uri_read_concern);
   TestSuite_Add (suite, "/Uri/write_concern", test_mongoc_uri_write_concern);
   TestSuite_Add (
      suite, "/HostList/from_string", test_mongoc_host_list_from_string);
   TestSuite_Add (suite,
                  "/Uri/auth_mechanism_properties",
                  test_mongoc_uri_authmechanismproperties);
   TestSuite_Add (suite, "/Uri/functions", test_mongoc_uri_functions);
   TestSuite_Add (suite, "/Uri/ssl", test_mongoc_uri_ssl);
   TestSuite_Add (
      suite, "/Uri/compound_setters", test_mongoc_uri_compound_setters);
   TestSuite_Add (suite, "/Uri/long_hostname", test_mongoc_uri_long_hostname);
   TestSuite_Add (
      suite, "/Uri/local_threshold_ms", test_mongoc_uri_local_threshold_ms);
}
