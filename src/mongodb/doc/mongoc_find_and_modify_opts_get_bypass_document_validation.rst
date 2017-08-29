:man_page: mongoc_find_and_modify_opts_get_bypass_document_validation

mongoc_find_and_modify_opts_get_bypass_document_validation()
============================================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_find_and_modify_opts_get_bypass_document_validation (
     const mongoc_find_and_modify_opts_t *opts);

Parameters
----------

* ``opts``: A :symbol:`mongoc_find_and_modify_opts_t`.

Returns
-------

Returns true if :symbol:`mongoc_find_and_modify_opts_set_bypass_document_validation` was called previously on ``opts``.

