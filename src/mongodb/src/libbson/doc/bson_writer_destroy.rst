:man_page: bson_writer_destroy

bson_writer_destroy()
=====================

Synopsis
--------

.. code-block:: c

  void
  bson_writer_destroy (bson_writer_t *writer);

Parameters
----------

* ``writer``: A :symbol:`bson_writer_t`.

Description
-----------

Cleanup after ``writer`` and release any allocated memory. Note that the buffer supplied to :symbol:`bson_writer_new()` is *NOT* freed from this method.  The caller is responsible for that.

