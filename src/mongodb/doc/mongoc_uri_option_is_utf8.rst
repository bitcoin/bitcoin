:man_page: mongoc_uri_option_is_utf8

mongoc_uri_option_is_utf8()
===========================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_uri_option_is_utf8 (const char *option);

Parameters
----------

* ``option``: The name of an option, case insensitive.

Description
-----------

Returns true if the option is a known MongoDB URI option of string type. For example, "replicaSet=my_rs" is a valid MongoDB URI option, so ``mongoc_uri_option_is_utf8 ("replicaSet")`` is true.

