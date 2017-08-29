:man_page: bson_init_static

bson_init_static()
==================

Synopsis
--------

.. code-block:: c

  bool
  bson_init_static (bson_t *b, const uint8_t *data, size_t length);

Parameters
----------

* ``b``: A :symbol:`bson_t`.
* ``data``: A buffer to initialize with.
* ``length``: The length of ``data`` in bytes.

Description
-----------

The :symbol:`bson_init_static()` function shall shall initialize a read-only :symbol:`bson_t` on the stack using the data provided. No copies of the data will be made and therefore must remain valid for the lifetime of the :symbol:`bson_t`.

The resulting `bson_t` has internal references and therefore must not be copied to avoid dangling pointers in the copy.

Returns
-------

Returns ``true`` if :symbol:`bson_t` was successfully initialized, otherwise ``false``. The function can fail if ``data`` or ``length`` are invalid.

.. only:: html

  .. taglist:: See Also:
    :tags: create-bson
