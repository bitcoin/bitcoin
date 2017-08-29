:man_page: mongoc_find_and_modify_opts_get_update

mongoc_find_and_modify_opts_get_update()
========================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_find_and_modify_opts_get_update (
     const mongoc_find_and_modify_opts_t *opts, bson_t *update);

Parameters
----------

* ``opts``: A :symbol:`mongoc_find_and_modify_opts_t`.
* ``update``: An uninitialized :symbol:`bson:bson_t`.

Description
-----------

Copies the update document set with :symbol:`mongoc_find_and_modify_opts_set_update`, or initializes ``update`` with an empty BSON document.

