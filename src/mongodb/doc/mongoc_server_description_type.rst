:man_page: mongoc_server_description_type

mongoc_server_description_type()
================================

Synopsis
--------

.. code-block:: c

  const char *
  mongoc_server_description_type (const mongoc_server_description_t *description);

Parameters
----------

* ``description``: A :symbol:`mongoc_server_description_t`.

Description
-----------

This function returns a string, one of the server types defined in the Server Discovery And Monitoring Spec:

* Standalone
* Mongos
* PossiblePrimary
* RSPrimary
* RSSecondary
* RSArbiter
* RSOther
* RSGhost
* Unknown

