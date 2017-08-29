:man_page: bson_new_from_buffer

bson_new_from_buffer()
======================

Synopsis
--------

.. code-block:: c

  bson_t *
  bson_new_from_buffer (uint8_t **buf,
                        size_t *buf_len,
                        bson_realloc_func realloc_func,
                        void *realloc_func_ctx);

Parameters
----------

* ``buf``: An out-pointer to a buffer containing a serialized BSON document, or to NULL.
* ``buf_len``: An out-pointer to the length of the buffer in bytes.
* ``realloc_func``: Optional :symbol:`bson_realloc_func` for reallocating the buffer.
* ``realloc_func_ctx``: Optional pointer that will be passed as ``ctx`` to ``realloc_func``.

Description
-----------

Creates a new :symbol:`bson_t` using the data provided.

The ``realloc_func``, if provided, is called to resize ``buf`` if the document is later expanded, for example by a call to one of the ``bson_append`` functions.

If ``*buf`` is initially NULL then it is allocated, using ``realloc_func`` or the default allocator, and initialized with an empty BSON document, and ``*buf_len`` is set to 5, the size of an empty document.

Returns
-------

A newly-allocated :symbol:`bson_t` on success, or NULL.

.. only:: html

  .. taglist:: See Also:
    :tags: create-bson
