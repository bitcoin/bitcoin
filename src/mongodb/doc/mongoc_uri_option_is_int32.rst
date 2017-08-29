:man_page: mongoc_uri_option_is_int32

mongoc_uri_option_is_int32()
============================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_uri_option_is_int32 (const char *option);

Parameters
----------

* ``option``: The name of an option, case insensitive.

Description
-----------

Returns true if the option is a known MongoDB URI option of integer type. For example, "socketTimeoutMS=100" is a valid MongoDB URI option, so ``mongoc_uri_option_is_int32 ("connectTimeoutMS")`` is true.

