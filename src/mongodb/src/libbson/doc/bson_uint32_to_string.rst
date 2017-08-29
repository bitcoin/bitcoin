:man_page: bson_uint32_to_string

bson_uint32_to_string()
=======================

Synopsis
--------

.. code-block:: c

  size_t
  bson_uint32_to_string (uint32_t value,
                         const char **strptr,
                         char *str,
                         size_t size);


Parameters
----------

* ``value``: A uint32_t.
* ``strptr``: A location for the resulting string pointer.
* ``str``: A location to buffer the string.
* ``size``: A size_t containing the size of ``str``.

Description
-----------

Converts ``value`` to a string.

If ``value`` is from 0 to 999, it will use a constant string in the data section of the library.

If not, a string will be formatted using ``str`` and ``snprintf()``.

``strptr`` will always be set. It will either point to ``str`` or a constant string. Use this as your key.

Array Element Key Building
--------------------------

Each element in a BSON array has a monotonic string key like ``"0"``, ``"1"``, etc. This function is optimized for generating such string keys.

.. code-block:: c

  char str[16];
  const char *key;
  uint32_t i;

  for (i = 0; i < 10; i++) {
     bson_uint32_to_string (i, &key, str, sizeof str);
     printf ("Key: %s\n", key);
  }

Returns
-------

The number of bytes in the resulting string.

