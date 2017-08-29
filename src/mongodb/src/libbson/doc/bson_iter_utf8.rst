:man_page: bson_iter_utf8

bson_iter_utf8()
================

Synopsis
--------

.. code-block:: c

  #define BSON_ITER_HOLDS_UTF8(iter) (bson_iter_type ((iter)) == BSON_TYPE_UTF8)

  const char *
  bson_iter_utf8 (const bson_iter_t *iter, uint32_t *length);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``length``: An optional location for the length of the resulting UTF-8 encoded string.

Description
-----------

The :symbol:`bson_iter_utf8()` function shall retrieve the contents of a BSON_TYPE_UTF8 element currently observed by ``iter``.

It is invalid to call this function while observing an element other than BSON_TYPE_UTF8.

Returns
-------

A UTF-8 encoded string that has not been modified or freed.

It is suggested that the caller validate the content is valid UTF-8 before using this in other places. That can be done by calling :symbol:`bson_utf8_validate()` or validating the underlying :symbol:`bson_t` before iterating it.

Note that not all drivers use multi-byte representation for ``\0`` in UTF-8 encodings (commonly referred to as modified-UTF8). You probably want to take a look at the length field when marshaling to other runtimes.

