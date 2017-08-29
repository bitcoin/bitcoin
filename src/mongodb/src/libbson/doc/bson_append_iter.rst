:man_page: bson_append_iter

bson_append_iter()
==================

Synopsis
--------

.. code-block:: c

  bool
  bson_append_iter (bson_t *bson,
                    const char *key,
                    int key_length,
                    const bson_iter_t *iter);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: Optional field name. If NULL, uses :symbol:`bson_iter_key(iter) <bson_iter_key>`.
* ``key_length``: The length of ``key`` or -1 to use strlen().
* ``iter``: A :symbol:`bson_iter_t` located on the position of the element to append.

Description
-----------

Appends the value at the current position of ``iter`` to the document.

Returns
-------

Returns ``true`` if successful; ``false`` if the operation would overflow the maximum document size or another invalid state is detected.
