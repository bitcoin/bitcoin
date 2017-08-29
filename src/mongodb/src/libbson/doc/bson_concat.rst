:man_page: bson_concat

bson_concat()
=============

Synopsis
--------

.. code-block:: c

  bool
  bson_concat (bson_t *dst, const bson_t *src);

Parameters
----------

* ``dst``: A :symbol:`bson_t`.
* ``src``: A :symbol:`bson_t`.

Description
-----------

The :symbol:`bson_concat()` function shall append the contents of ``src`` to ``dst``.

Returns
-------

Returns ``true`` if successful; ``false`` if the operation would overflow the maximum document size or another invalid state is detected.

.. only:: html

  .. taglist:: See Also:
    :tags: create-bson
