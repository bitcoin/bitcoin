:man_page: bson_iter_codewscope

bson_iter_codewscope()
======================

Synopsis
--------

.. code-block:: c

  #define BSON_ITER_HOLDS_CODEWSCOPE(iter) \
     (bson_iter_type ((iter)) == BSON_TYPE_CODEWSCOPE)

  const char *
  bson_iter_codewscope (const bson_iter_t *iter,
                        uint32_t *length,
                        uint32_t *scope_len,
                        const uint8_t **scope);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``length``: An optional location for the length of the resulting UTF-8 encoded string.
* ``scope_len``: A optional location for the length of ``scope``.
* ``scope``: An optional location to store the immutable raw scope BSON document.

Description
-----------

The ``bson_iter_codewscope()`` function acts similar to :symbol:`bson_iter_code()` except for BSON_TYPE_CODEWSCOPE elements. It also will provide a pointer to the buffer for scope, which can be loaded into a :symbol:`bson_t` using :symbol:`bson_init_static()`.

Returns
-------

An UTF-8 encoded string containing the JavaScript code which should not be modified or freed.

