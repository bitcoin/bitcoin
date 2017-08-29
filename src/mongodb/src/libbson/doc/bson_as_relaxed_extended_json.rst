:man_page: bson_as_relaxed_extended_json

bson_as_relaxed_extended_json()
===============================

Synopsis
--------

.. code-block:: c

  char *
  bson_as_relaxed_extended_json (const bson_t *bson, size_t *length);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``length``: An optional location for the length of the resulting string.

Description
-----------

The :symbol:`bson_as_relaxed_extended_json()` encodes ``bson`` as a UTF-8 string in the relaxed `MongoDB Extended JSON format`_.

The caller is responsible for freeing the resulting UTF-8 encoded string by calling :symbol:`bson_free()` with the result.

If non-NULL, ``length`` will be set to the length of the result in bytes.

Returns
-------

If successful, a newly allocated UTF-8 encoded string and ``length`` is set.

Upon failure, NULL is returned.

Example
-------

.. code-block:: c

  char *str = bson_as_relaxed_extended_json (doc, NULL);
  printf ("%s\n", str);
  bson_free (str);


.. only:: html

  .. taglist:: See Also:
    :tags: bson-as-json

.. _MongoDB Extended JSON format: https://github.com/mongodb/specifications/blob/master/source/extended-json.rst
