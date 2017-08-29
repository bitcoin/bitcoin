:man_page: bson_append_code_with_scope

bson_append_code_with_scope()
=============================

Synopsis
--------

.. code-block:: c

  #define BSON_APPEND_CODE_WITH_SCOPE(b, key, val, scope) \
     bson_append_code_with_scope (b, key, (int) strlen (key), val, scope)

  bool
  bson_append_code_with_scope (bson_t *bson,
                               const char *key,
                               int key_length,
                               const char *javascript,
                               const bson_t *scope);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: An ASCII C string containing the name of the field.
* ``key_length``: The length of ``key`` in bytes, or -1 to determine the length with ``strlen()``.
* ``javascript``: A NULL-terminated UTF-8 encoded string containing the javascript fragment.
* ``scope``: Optional :symbol:`bson_t` containing the scope for ``javascript``.

Description
-----------

The :symbol:`bson_append_code_with_scope()` function shall perform like :symbol:`bson_append_code()` except it allows providing a scope to the javascript function in the form of a bson document.

If ``scope`` is NULL, this function appends an element with BSON type "code", otherwise with BSON type "code with scope".

Returns
-------

Returns ``true`` if the operation was applied successfully. The function will fail if appending ``javascript`` and ``scope`` grows ``bson`` larger than INT32_MAX.
