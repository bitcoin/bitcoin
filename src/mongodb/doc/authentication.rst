:man_page: mongoc_authentication

Authentication
==============

This guide covers the use of authentication options with the MongoDB C Driver. Ensure that the MongoDB server is also properly configured for authentication before making a connection. For more information, see the `MongoDB security documentation <https://docs.mongodb.org/manual/administration/security/>`_.

The MongoDB C driver supports several authentication mechanisms through the use of MongoDB connection URIs.

By default, if a username and password are provided as part of the connection string (and an optional authentication database), they are used to connect via the default authentication mechanism of the server.

To select a specific authentication mechanism other than the default, see the list of supported mechanism below.


.. code-block:: none

  mongoc_client_t *client = mongoc_client_new ("mongodb://user:password@localhost/?authSource=mydb");


Currently supported values for the authMechanism connection string option are:

* :ref:`SCRAM-SHA-1 <authentication_scram_sha_1>`
* :ref:`MONGODB-CR <authentication_mongodbcr>`
* :ref:`GSSAPI <authentication_kerberos>`
* :ref:`PLAIN <authentication_plain>`
* :ref:`X509 <authentication_x509>`

.. _authentication_scram_sha_1:

Basic Authentication (SCRAM-SHA-1)
----------------------------------

The default authentication mechanism when talking to MongoDB 3.0 and later is ``SCRAM-SHA-1`` (`RFC 5802 <http://tools.ietf.org/html/rfc5802>`_). Using this authentication mechnism means that the password is never actually sent over the wire when authenticating, but rather a computed proof that the client password is the same as the password the server knows.


.. code-block:: none

  mongoc_client_t *client = mongoc_client_new ("mongodb://user:password@localhost/?authMechanism=SCRAM-SHA-1&authSource=mydb");


.. note::

  ``SCRAM-SHA-1`` authenticates against the ``admin`` database by default. If the user is created in another database, then specifying the authSource is required. 


.. _authentication_mongodbcr:

Legacy Authentication (MONGODB-CR)
----------------------------------

The MONGODB-CR authMechanism is a challenge response authentication mechanism. It was the default mechanism until MongoDB 3.0 and is being phased out. It is strongly suggested that users upgrade to SCRAM-SHA-1.


.. note::

  ``MONGODB-CR`` authenticates against the ``admin`` database by default. If the user is created in another database, then specifying the authSource is required. 


.. _authentication_kerberos:

GSSAPI (Kerberos) Authentication
--------------------------------

.. note::

  Kerberos support requires compiling the driver against ``cyrus-sasl`` on UNIX-like environments. On Windows, configure the driver to build against the Windows Native SSPI.

``GSSAPI`` (Kerberos) authentication is available in the Enterprise Edition of MongoDB, version 2.4 and newer. To authenticate using ``GSSAPI``, the MongoDB C driver must be installed with SASL support. 

On UNIX-like environments, run the ``kinit`` command before using the following authentication methods:

.. code-block:: none

  $ kinit mongodbuser@EXAMPLE.COMmongodbuser@EXAMPLE.COM's Password:
  $ klistCredentials cache: FILE:/tmp/krb5cc_1000
          Principal: mongodbuser@EXAMPLE.COM

    Issued                Expires               Principal
  Feb  9 13:48:51 2013  Feb  9 23:48:51 2013  krbtgt/EXAMPLE.COM@EXAMPLE.COM

Now authenticate using the MongoDB URI. ``GSSAPI`` authenticates against the ``$external`` virtual database, so a database does not need to be specified in the URI. Note that the Kerberos principal *must* be URL-encoded:

.. code-block:: none

  mongoc_client_t *client;

  client = mongoc_client_new ("mongodb://mongodbuser%40EXAMPLE.COM@example.com/?authMechanism=GSSAPI");

.. note::

  ``GSSAPI`` authenticates against the ``$external`` database, so specifying the authSource database is not required. 

The driver supports these GSSAPI properties:

* ``CANONICALIZE_HOST_NAME``: This might be required when the hosts report different hostnames than what is used in the kerberos database. The default is "false".
* ``SERVICE_NAME``: Use a different service name than the default, "mongodb".

Set properties in the URL:

.. code-block:: none

  mongoc_client_t *client;

  client = mongoc_client_new ("mongodb://mongodbuser%40EXAMPLE.COM@example.com/?authMechanism=GSSAPI&"
                              "authMechanismProperties=SERVICE_NAME:other,CANONICALIZE_HOST_NAME:true");

If you encounter errors such as ``Invalid net address``, check if the application is behind a NAT (Network Address Translation) firewall. If so, create a ticket that uses ``forwardable`` and ``addressless`` Kerberos tickets. This can be done by passing ``-f -A`` to ``kinit``.

.. code-block:: none

  $ kinit -f -A mongodbuser@EXAMPLE.COM


.. _authentication_plain:

SASL Plain Authentication
-------------------------

.. note::

  The MongoDB C Driver must be compiled with SASL support in order to use ``SASL PLAIN`` authentication.

MongoDB Enterprise Edition versions 2.6.0 and newer support the ``SASL PLAIN`` authentication mechanism, initially intended for delegating authentication to an LDAP server. Using the ``SASL PLAIN`` mechanism is very similar to the challenge response mechanism with usernames and passwords. This authentication mechanism uses the ``$external`` virtual database for ``LDAP`` support:

.. note::

  ``SASL PLAIN`` is a clear-text authentication mechanism. It is strongly recommended to connect to MongoDB using SSL with certificate validation when using the ``PLAIN`` mechanism.

.. code-block:: none

  mongoc_client_t *client;

  client = mongoc_client_new ("mongodb://user:password@example.com/?authMechanism=PLAIN");

``PLAIN`` authenticates against the ``$external`` database, so specifying the authSource database is not required.


.. _authentication_x509:

X.509 Certificate Authentication
--------------------------------

.. note::

  The MongoDB C Driver must be compiled with SSL support for X.509 authentication support. Once this is done, start a server with the following options: 

  .. code-block:: none

    $ mongod --sslMode requireSSL --sslPEMKeyFile server.pem --sslCAFile ca.pem

The ``MONGODB-X509`` mechanism authenticates a username derived from the distinguished subject name of the X.509 certificate presented by the driver during SSL negotiation. This authentication method requires the use of SSL connections with certificate validation and is available in MongoDB 2.6.0 and newer:

.. code-block:: none

  mongoc_client_t *client;
  mongoc_ssl_opt_t ssl_opts = { 0 };

  ssl_opts.pem_file = "mycert.pem";
  ssl_opts.pem_pwd = "mycertpassword";
  ssl_opts.ca_file = "myca.pem";
  ssl_opts.ca_dir = "trust_dir";
  ssl_opts.weak_cert_validation = false;

  client = mongoc_client_new ("mongodb://x509_derived_username@localhost/?authMechanism=MONGODB-X509");
  mongoc_client_set_ssl_opts (client, &ssl_opts);

``MONGODB-X509`` authenticates against the ``$external`` database, so specifying the authSource database is not required. For more information on the x509_derived_username, see the MongoDB server `x.509 tutorial <https://docs.mongodb.com/manual/tutorial/configure-x509-client-authentication/#add-x-509-certificate-subject-as-a-user>`_.


.. note::

  The MongoDB C Driver will attempt to determine the x509 derived username when none is provided, and as of MongoDB 3.4 providing the username is not required at all.

.. only:: html

  .. taglist:: See Also:
    :tags: authmechanism
