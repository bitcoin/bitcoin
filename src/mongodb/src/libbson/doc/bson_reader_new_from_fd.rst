:man_page: bson_reader_new_from_fd

bson_reader_new_from_fd()
=========================

Synopsis
--------

.. code-block:: c

  bson_reader_t *
  bson_reader_new_from_fd (int fd, bool close_on_destroy);

Parameters
----------

* ``fd``: A valid file-descriptor.
* ``close_on_destroy``: Whether ``close()`` should be called on ``fd`` when the reader is destroyed.

Description
-----------

The :symbol:`bson_reader_new_from_fd()` function shall create a new :symbol:`bson_reader_t` that will read from the provided file-descriptor.

fd *MUST* be in blocking mode.

If ``close_fd`` is true, then ``fd`` will be closed when the :symbol:`bson_reader_t` is destroyed with :symbol:`bson_reader_destroy()`.

Returns
-------

A newly allocated :symbol:`bson_reader_t` that should be freed with :symbol:`bson_reader_destroy`.

