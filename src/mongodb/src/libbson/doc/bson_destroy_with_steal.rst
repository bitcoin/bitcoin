:man_page: bson_destroy_with_steal

bson_destroy_with_steal()
=========================

Synopsis
--------

.. code-block:: c

  uint8_t *
  bson_destroy_with_steal (bson_t *bson, bool steal, uint32_t *length);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``steal``: A bool indicating if the underlying buffer should be stolen.
* ``length``: A location for storing the resulting buffer length.

Description
-----------

The :symbol:`bson_destroy_with_steal()` function shall destroy a :symbol:`bson_t` structure but return the underlying buffer instead of freeing it. If steal is false, this is equivalent to calling bson_destroy(). It is a programming error to call this function on a :symbol:`bson_t` that is not a top-level :symbol:`bson_t`, such as those initialized with :symbol:`bson_append_document_begin()`, :symbol:`bson_append_array_begin()`, and :symbol:`bson_writer_begin()`.

See also :symbol:`bson_steal`, a higher-level function that efficiently transfers the contents of one :symbol:`bson_t` to another.

Returns
-------

:symbol:`bson_destroy_with_steal()` shall return a buffer containing the contents of the :symbol:`bson_t` if ``steal`` is non-zero. This should be freed with :symbol:`bson_free()` when no longer in use. ``length`` will be set to the length of the bson document if non-NULL.

