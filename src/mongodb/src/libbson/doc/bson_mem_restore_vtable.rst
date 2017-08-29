:man_page: bson_mem_restore_vtable

bson_mem_restore_vtable()
=========================

Synopsis
--------

.. code-block:: c

  void
  bson_mem_restore_vtable (void);

Description
-----------

This function shall restore the default memory allocator to be used by Libbson.

.. warning::

  This function *MUST* be called at the end of the process. Failure to do so will result in memory being freed by the wrong allocator.

