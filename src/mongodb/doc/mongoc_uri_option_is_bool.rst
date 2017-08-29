:man_page: mongoc_uri_option_is_bool

mongoc_uri_option_is_bool()
===========================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_uri_option_is_bool (const char *option);

Parameters
----------

* ``option``: The name of an option, case insensitive.

Description
-----------

Returns true if the option is a known MongoDB URI option of boolean type. For example, "ssl=false" is a valid MongoDB URI option, so ``mongoc_uri_option_is_bool ("ssl")`` is true.

