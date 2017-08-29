:man_page: bson_reader_set_destroy_func

bson_reader_set_destroy_func()
==============================

Synopsis
--------

.. code-block:: c

  void
  bson_reader_set_destroy_func (bson_reader_t *reader,
                                bson_reader_destroy_func_t func);

Parameters
----------

* ``reader``: A :symbol:`bson_reader_t`.
* ``func``: A :symbol:`bson_reader_destroy_func_t`.

Description
-----------

Allows for setting a callback to be executed when a reader is destroyed. This should only be used by implementations implementing their own read callbacks.

