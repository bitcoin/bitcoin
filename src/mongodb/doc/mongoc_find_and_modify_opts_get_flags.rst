:man_page: mongoc_find_and_modify_opts_get_flags

mongoc_find_and_modify_opts_get_flags()
=======================================

Synopsis
--------

.. code-block:: c

  mongoc_find_and_modify_flags_t
  mongoc_find_and_modify_opts_get_flags (
     const mongoc_find_and_modify_opts_t *opts);

Parameters
----------

* ``opts``: A :symbol:`mongoc_find_and_modify_opts_t`.

Returns
-------

Returns the flags set with :symbol:`mongoc_find_and_modify_opts_set_flags`.

