:man_page: bson_iter_next

bson_iter_next()
================

Synopsis
--------

.. code-block:: c

  bool
  bson_iter_next (bson_iter_t *iter);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.

Description
-----------

Advances ``iter`` to the next element in the document.

Returns
-------

true if ``iter`` was advanced. Returns false if ``iter`` has passed the last element in the document or encountered invalid BSON.

It is a programming error to use ``iter`` after this function has returned false.

