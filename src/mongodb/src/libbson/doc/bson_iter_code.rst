:man_page: bson_iter_code

bson_iter_code()
================

Synopsis
--------

.. code-block:: c

  #define BSON_ITER_HOLDS_CODE(iter) (bson_iter_type ((iter)) == BSON_TYPE_CODE)

  const char *
  bson_iter_code (const bson_iter_t *iter, uint32_t *length);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``length``: A location for the length of the UTF-8 encoded string or NULL.

Description
-----------

This function returns the contents of a BSON_TYPE_CODE field. The length of the string is stored in ``length`` if non-NULL.

It is invalid to call this function on a field that is not of type BSON_TYPE_CODE.

Returns
-------

A UTF-8 encoded string which should not be modified or freed.

