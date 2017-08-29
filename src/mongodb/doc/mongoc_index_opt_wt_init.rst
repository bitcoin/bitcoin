:man_page: mongoc_index_opt_wt_init

mongoc_index_opt_wt_init()
==========================

Synopsis
--------

.. code-block:: c

  void
  mongoc_index_opt_wt_init (mongoc_index_opt_wt_t *opt);

Parameters
----------

* ``opt``: A :symbol:`mongoc_index_opt_wt_t`.

Description
-----------

This function will initialize ``opt`` to the default values. It should be called before modifying any fields within the structure.

