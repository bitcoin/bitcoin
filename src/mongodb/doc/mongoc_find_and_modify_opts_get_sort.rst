:man_page: mongoc_find_and_modify_opts_get_sort

mongoc_find_and_modify_opts_get_sort()
======================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_find_and_modify_opts_get_sort (const mongoc_find_and_modify_opts_t *opts,
                                        bson_t *sort);

Parameters
----------

* ``opts``: A :symbol:`mongoc_find_and_modify_opts_t`.
* ``sort``: An uninitialized :symbol:`bson:bson_t`.

Description
-----------

Copies the sort document set with :symbol:`mongoc_find_and_modify_opts_set_sort`, or initializes ``sort`` with an empty BSON document.

