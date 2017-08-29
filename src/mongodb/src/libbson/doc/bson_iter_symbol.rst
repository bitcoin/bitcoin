:man_page: bson_iter_symbol

bson_iter_symbol()
==================

Synopsis
--------

.. code-block:: c

  const char *
  bson_iter_symbol (const bson_iter_t *iter, uint32_t *length);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``length``: A uint32_t.

Description
-----------

The symbol element type is *DEPRECATED* in the bson specification at http://bsonspec.org.

The :symbol:`bson_iter_symbol()` function shall return the contents of a BSON_TYPE_SYMBOL element.

If non-NULL, ``length`` will be set to the length of the resulting string.

Returns
-------

The symbol and ``length`` is set.

