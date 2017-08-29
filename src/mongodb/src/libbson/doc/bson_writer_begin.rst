:man_page: bson_writer_begin

bson_writer_begin()
===================

Synopsis
--------

.. code-block:: c

  bool
  bson_writer_begin (bson_writer_t *writer, bson_t **bson);

Parameters
----------

* ``writer``: A :symbol:`bson_writer_t`.
* ``bson``: A :symbol:`bson_t`.

Description
-----------

Begins writing a new document. The caller may use the bson structure to write out a new BSON document. When completed, the caller must call either :symbol:`bson_writer_end()` or :symbol:`bson_writer_rollback()`.

Returns
-------

true if there was space in the underlying buffers to begin the document.

