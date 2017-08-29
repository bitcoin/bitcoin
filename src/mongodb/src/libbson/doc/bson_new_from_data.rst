:man_page: bson_new_from_data

bson_new_from_data()
====================

Synopsis
--------

.. code-block:: c

  bson_t *
  bson_new_from_data (const uint8_t *data, size_t length);

Parameters
----------

* ``data``: A BSON encoded document buffer.
* ``length``: The length of ``data`` in bytes.

Description
-----------

The :symbol:`bson_new_from_data()` function shall create a new :symbol:`bson_t` on the heap and copy the contents of ``data``. This may be helpful when working with language bindings but is generally expected to be slower.

Returns
-------

A newly allocated :symbol:`bson_t` if successful, otherwise NULL.

.. only:: html

  .. taglist:: See Also:
    :tags: create-bson
