:man_page: mongoc_find_and_modify_opts_set_max_time_ms

mongoc_find_and_modify_opts_set_max_time_ms()
=============================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_find_and_modify_opts_set_max_time_ms (
     mongoc_find_and_modify_opts_t *opts, uint32_t max_time_ms);

Parameters
----------

* ``opts``: A :symbol:`mongoc_find_and_modify_opts_t`.
* ``max_time_ms``: The maximum server-side execution time permitted, in milliseconds, or 0 to specify no maximum time (the default setting).

Description
-----------

Adds a maxTimeMS argument to the builder.

Returns
-------

Returns ``true`` if it successfully added the option to the builder, otherwise ``false`` and logs an error.

Setting maxTimeMS
-----------------

.. literalinclude:: ../examples/find_and_modify_with_opts/opts.c
   :language: c
   :caption: opts.c

