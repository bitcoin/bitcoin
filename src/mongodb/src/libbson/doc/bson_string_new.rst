:man_page: bson_string_new

bson_string_new()
=================

Synopsis
--------

.. code-block:: c

  bson_string_t *
  bson_string_new (const char *str);

Parameters
----------

* ``str``: A string to be copied or NULL.

Description
-----------

Creates a new string builder, which uses power-of-two growth of buffers. Use the various bson_string_append*() functions to append to the string.

Returns
-------

A newly allocated bson_string_t that should be freed with bson_string_free() when no longer in use.

