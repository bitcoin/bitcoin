:man_page: bson_iter_init_from_data

bson_iter_init_from_data()
==========================

Synopsis
--------

.. code-block:: c

  bool
  bson_iter_init_from_data (bson_iter_t *iter, const uint8_t *data, size_t length);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``data``: A buffer to initialize with.
* ``length``: The length of ``data`` in bytes.

Description
-----------

The ``bson_iter_init_from_data()`` function shall initialize ``iter`` to iterate upon the buffer ``data``, which must contain a BSON document. Upon initialization, ``iter`` is placed before the first element. Callers must call :symbol:`bson_iter_next()`, :symbol:`bson_iter_find()`, or :symbol:`bson_iter_find_case()` to advance to an element.

Returns
-------

Returns true if the iter was successfully initialized.

Example
-------

.. code-block:: c

  static void
  print_doc_id (const uint8_t *data, size_t length)
  {
     bson_iter_t iter;
     bson_oid_t oid;
     char oidstr[25];

     if (bson_iter_init_from_data (&iter, data, length) && bson_iter_find (&iter, "_id") &&
         BSON_ITER_HOLDS_OID (&iter)) {
        bson_iter_oid (&iter, &oid);
        bson_oid_to_string (&oid, oidstr);
        printf ("%s\n", oidstr);
     } else {
        printf ("Document is missing _id.\n");
     }
  }

.. only:: html

  .. taglist:: See Also:
    :tags: iter-init
