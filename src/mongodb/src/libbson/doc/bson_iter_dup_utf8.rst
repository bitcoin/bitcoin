:man_page: bson_iter_dup_utf8

bson_iter_dup_utf8()
====================

Synopsis
--------

.. code-block:: c

  char *
  bson_iter_dup_utf8 (const bson_iter_t *iter, uint32_t *length);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``length``: An optional location for the length of the UTF-8 encoded string.

Description
-----------

This function is similar to :symbol:`bson_iter_utf8()` except that it calls :symbol:`bson_strndup()` on the result.

Returns
-------

A newly allocated string that should be freed with :symbol:`bson_free()`.

