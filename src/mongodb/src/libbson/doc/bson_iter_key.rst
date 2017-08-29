:man_page: bson_iter_key

bson_iter_key()
===============

Synopsis
--------

.. code-block:: c

  const char *
  bson_iter_key (const bson_iter_t *iter);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.

Description
-----------

Fetches the key for the current element observed by ``iter``.

Returns
-------

A string which should not be modified or freed.

