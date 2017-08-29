:man_page: mongoc_ssl_opt_t

mongoc_ssl_opt_t
================

Synopsis
--------

.. code-block:: c

  typedef struct {
     const char *pem_file;
     const char *pem_pwd;
     const char *ca_file;
     const char *ca_dir;
     const char *crl_file;
     bool weak_cert_validation;
     bool allow_invalid_hostname;
     void *padding[7];
  } mongoc_ssl_opt_t;

Description
-----------

This structure is used to set the SSL options for a :symbol:`mongoc_client_t` or :symbol:`mongoc_client_pool_t`.

Beginning in version 1.2.0, once a pool or client has any SSL options set, all connections use SSL, even if ``ssl=true`` is omitted from the MongoDB URI. Before, SSL options were ignored unless ``ssl=true`` was included in the URI.

As of 1.4.0, the :symbol:`mongoc_client_pool_set_ssl_opts` and :symbol:`mongoc_client_set_ssl_opts` will not only shallow copy the struct, but will also copy the ``const char*``. It is therefore no longer needed to make sure the values remain valid after setting them.


Configuration through URI Options
---------------------------------

Most of the configurable options can be using the Connection URI.

===============================  ===============================
**mongoc_ssl_opt_t key**         **URI key**
===============================  ===============================
pem_file                         sslClientCertificateKeyFile
pem_pwd                          sslClientCertificateKeyPassword
ca_file                          sslCertificateAuthorityFile
weak_cert_validation             sslAllowInvalidCertificates
allow_invalid_hostname           sslAllowInvalidHostnames
===============================  ===============================

Client Authentication
---------------------

When MongoDB is started with SSL enabled, it will by default require the client to provide a client certificate issued by a certificate authority specified by ``--sslCAFile``, or an authority trusted by the native certificate store in use on the server.

To provide the client certificate, the user must configure the ``pem_file`` to point at a PEM armored certificate.

.. code-block:: c

  mongoc_ssl_opt_t ssl_opts = {0};

  ssl_opts.pem_file = "/path/to/client-certificate.pem"

  /* Then set the client ssl_opts, when using a single client mongoc_client_t */
  mongoc_client_pool_set_ssl_opts (pool, &ssl_opts);

  /* or, set the pool ssl_opts, when using a the thread safe mongoc_client_pool_t */
  mongoc_client_set_ssl_opts (client, &ssl_opts);

Server Certificate Verification
-------------------------------

The MongoDB C Driver will automatically verify the validity of the server certificate, such as issued by configured Certificate Authority, hostname validation, and expiration.

To overwrite this behaviour, it is possible to disable hostname validation, and/or allow otherwise invalid certificates. This behaviour is controlled using the ``allow_invalid_hostname`` and ``weak_cert_validation`` fields. By default, both are set to ``false``. It is not recommended to change these defaults as it exposes the client to *Man In The Middle* attacks (when ``allow_invalid_hostname`` is set) and otherwise invalid certificates when ``weak_cert_validation`` is set to ``true``.

Native TLS Support on Linux (OpenSSL)
-------------------------------------

The MongoDB C Driver supports the dominating TLS library (OpenSSL) and crypto libraries (OpenSSL's libcrypto) on Linux and Unix platforms.

Support for OpenSSL 1.1 and later was added in 1.4.0.

When compiled against OpenSSL, the driver will attempt to load the system default certificate store, as configured by the distribution, if the ``ca_file`` and ``ca_dir`` are not set.

LibreSSL / libtls
-----------------

The MongoDB C Driver supports LibreSSL through the use of OpenSSL compatibility checks when configured to compile against ``openssl``. It also supports the new ``libtls`` library when configured to build against ``libressl``.

Native TLS Support on Windows (Secure Channel)
----------------------------------------------

The MongoDB C Driver supports the Windows native TLS library (Secure Channel, or SChannel), and its native crypto library (Cryptography API: Next Generation, or CNG).

When compiled against the Windows native libraries, the ``ca_dir`` option is not supported, and will issue an error if used.

Encrypted PEM files (e.g., requiring ``pem_pwd``) are also not supported, and will result in error when attempting to load them.

When ``ca_file`` is provided, the driver will only allow server certificates issued by the authority (or authorities) provided. When no ``ca_file`` is provided, the driver will look up the Certificate Authority using the ``System Local Machine Root`` certificate store to confirm the provided certificate.

When ``crl_file`` is provided, the driver will import the revocation list to the ``System Local Machine Root`` certificate store.

Native TLS Support on Mac OS X / Darwin (Secure Transport)
----------------------------------------------------------

The MongoDB C Driver supports the Darwin (OS X, macOS, iOS, etc.) native TLS library (Secure Transport), and its native crypto library (Common Crypto, or CC).

When compiled against Secure Transport, the ``ca_dir`` option is not supported, and will issue an error if used.

When ``ca_file`` is provided, the driver will only allow server certificates issued by the authority (or authorities) provided. When no ``ca_file`` is provided, the driver will use the Certificate Authorities in the currently unlocked keychains.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_ssl_opt_get_default

See Also
--------

* :doc:`mongoc_client_set_ssl_opts`
* :doc:`mongoc_client_pool_set_ssl_opts`

