:man_page: bson_string_free

bson_string_free()
==================

Synopsis
--------

.. code-block:: c

  char *
  bson_string_free (bson_string_t *string, bool free_segment);

Parameters
----------

* ``string``: A :symbol:`bson_string_t`.
* ``free_segment``: A bool indicating if ``str->str`` should be returned.

Description
-----------

Frees the bson_string_t structure and optionally the string itself.

Returns
-------

``str->str`` if ``free_segment`` is true, otherwise NULL.

