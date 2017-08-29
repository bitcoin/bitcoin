:man_page: bson_writer_get_length

bson_writer_get_length()
========================

Synopsis
--------

.. code-block:: c

  size_t
  bson_writer_get_length (bson_writer_t *writer);

Parameters
----------

* ``writer``: A :symbol:`bson_writer_t`.

Description
-----------

Fetches the current length of the content written by the buffer (including the initial offset). This includes a partly written document currently being written.

This is useful if you want to check to see if you've passed a given memory boundary that cannot be sent in a packet. See :symbol:`bson_writer_rollback()` to abort the current document being written.

Returns
-------

The length of the underlying buffer.

