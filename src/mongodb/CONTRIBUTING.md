# Contributing to mongo-c-driver

Thanks for considering contributing to the mongo-c-driver!

This document intends to be a short guide to helping you contribute to the codebase.
It expects a familiarity with the C programming language and writing portable software.
Whenever in doubt, feel free to ask others that have contributed or look at the existing body of code.


## Guidelines

The mongo-c-driver has a few guidelines that help direct the process.


### Portability

mongo-c-driver is portable software. It needs to run on a multitude of
operating systems and architectures.

 * Linux (RHEL 5 and newer)
 * FreeBSD (10 and newer)
 * Windows (Vista and newer)
 * macOS (10.8 and newer)
 * Solaris x86_64/SPARC (11 and newer)
 * ARM/SPARC/x86/x86_64


### Licensing

Some of the mongo-c-driver users embed the library statically in their
products.  Therefore, the driver and all contributions must be liberally
licensed.  As a policy, we have chosen Apache 2.0 as the license for the
project.


### Coding Style

We try not to be pedantic with taking contributions that are not properly
formatted, but we will likely perform a followup commit that cleans things up.
The basics are, in vim:

```
 : set ts=3 sw=3 et
```

3 space tabs, insert spaces instead of tabs.

For all the gory details, see [.clang-format](.clang-format)

### Adding a new error code or domain                                              
                                                                                   
When adding a new error code or domain, you must do the following. This is most
applicable if you are adding a new symbol with a bson_error_t as a parameter,
and the existing codes or domains are inappropriate.                               
                                                                                   
 - Add the domain to `mongoc_error_domain_t` in `src/mongoc/mongoc-error.h`        
 - Add the code to `mongoc_error_code_t` in `src/mongoc/mongoc-error.h`            
 - Add documentation for the domain or code to the table in `doc/mongoc_errors.rst`
                              
### Adding a new symbol

This should be done rarely but there are several things that you need to do
when adding a new symbol.

 - Add documentation for the new symbol in `doc/mongoc_your_new_symbol_name.rst`

### Documentation

We strive to document all symbols. See doc/ for documentation examples. If you
add a new function, add a new .txt file describing the function so that we can
generate man pages and HTML for it.


### Testing

To run the entire test suite, including authentication tests,
start `mongod` with auth enabled:

```
$ mongod --auth
```

In another terminal, use the `mongo` shell to create a user:

```
$ mongo --eval "db.createUser({user: 'admin', pwd: 'pass', roles: ['root']})" admin
```

Authentication in MongoDB 3.0 and later uses SCRAM-SHA-1, which in turn
requires a driver built with SSL:

```
$ ./configure --enable-ssl`
```

Set the user and password environment variables, then build and run the tests:

```
$ export MONGOC_TEST_USER=admin
$ export MONGOC_TEST_PASSWORD=pass
$ make test
```

Additional environment variables:

* `MONGOC_TEST_HOST`: default `localhost`, the host running MongoDB.
* `MONGOC_TEST_PORT`: default 27017, MongoDB's listening port.
* `MONGOC_TEST_URI`: override both host and port with a full connection string,
  like "mongodb://server1,server2".
* `MONGOC_TEST_SERVER_LOG`: set to `stdout` or `stderr` for wire protocol 
  logging from tests that use `mock_server_t`. Set to `json` to include these
  logs in the test framework's JSON output, in a format compatible with
  [Evergreen](https://github.com/evergreen-ci/evergreen).
* `MONGOC_TEST_MONITORING_VERBOSE`: set to `on` for verbose output from
  Application Performance Monitoring tests.
* `MONGOC_TEST_COMPRESSORS=snappy,zlib`: wire protocol compressors to use

If you start `mongod` with SSL, set these variables to configure how
`make test` connects to it:

* `MONGOC_TEST_SSL`: set to `on` to connect to the server with SSL.
* `MONGOC_TEST_SSL_PEM_FILE`: path to a client PEM file.
* `MONGOC_TEST_SSL_PEM_PWD`: the PEM file's password.
* `MONGOC_TEST_SSL_CA_FILE`: path to a certificate authority file.
* `MONGOC_TEST_SSL_CA_DIR`: path to a certificate authority directory.
* `MONGOC_TEST_SSL_CRL_FILE`: path to a certificate revocation list.
* `MONGOC_TEST_SSL_WEAK_CERT_VALIDATION`: set to `on` to relax the client's
  validation of the server's certificate.

The SASL / GSSAPI / Kerberos tests are skipped by default. To run them, set up a
separate `mongod` with Kerberos and set its host and Kerberos principal name
as environment variables:

* `MONGOC_TEST_GSSAPI_HOST` 
* `MONGOC_TEST_GSSAPI_USER` 

URI-escape the username, for example write "user@realm" as "user%40realm".
The user must be authorized to query `kerberos.test`.

MongoDB 3.2 adds support for readConcern, but does not enable support for
read concern majority by default. mongod must be launched using
`--enableMajorityReadConcern`.
The test framework does not (and can't) automatically discover if this option was
provided to MongoDB, so an additional variable must be set to enable these tests:

* `MONGOC_ENABLE_MAJORITY_READ_CONCERN`

Set this environment variable to `on` if MongoDB has enabled majority read concern.

Some tests require Internet access, e.g. to check the error message when failing
to open a MongoDB connection to example.com. Skip them with:

* `MONGOC_TEST_OFFLINE=on`

Some tests require a running MongoDB server. Skip them with:

* `MONGOC_TEST_SKIP_LIVE=on`

For quick checks during development, disable long-running tests:

* `MONGOC_TEST_SKIP_SLOW=on`

Some tests run against a local mock server, these can be skipped with:

* `MONGOC_TEST_SKIP_MOCK=on`

If you have started with MongoDB with `--ipv6`, you can test IPv6 with:

* `MONGOC_CHECK_IPV6=on`

All tests should pass before submitting a patch.

## Configuring the test runner

The test runner can be configured by declaring the `TEST_ARGS` environment
variable. The following options can be provided:

```
    -h, --help    Show this help menu.
    -f, --no-fork Do not spawn a process per test (abort on first error).
    -l NAME       Run test by name, e.g. "/Client/command" or "/Client/*".
    -s, --silent  Suppress all output.
    -F FILENAME   Write test results (JSON) to FILENAME.
    -d            Print debug output (useful if a test hangs).
    -t, --trace   Enable mongoc tracing (useful to debug tests).
```

`TEST_ARGS` is set to "--no-fork" by default, meaning that the suite aborts on
the first test failure. Use "--fork" to continue after failures.

To run just a specific portion of the test suite use the -l option like so:

```
$ make test TEST_ARGS="-l /server_selection/*"
```

The full list of tests is shown in the help.

## Debugging failed tests

The easiest way to debug a failed tests is to use the `debug` make target:

```
$ make debug TEST_ARGS="-l /WriteConcern/bson_omits_defaults"
```

This will build all dependencies and leave you in a debugger ready to run the test.
