:man_page: bson_iter_init

bson_iter_init()
================

Synopsis
--------

.. code-block:: c

  bool
  bson_iter_init (bson_iter_t *iter, const bson_t *bson);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``bson``: A :symbol:`bson_t`.

Description
-----------

The ``bson_iter_init()`` function shall initialize ``iter`` to iterate upon the BSON document ``bson``. Upon initialization, ``iter`` is placed before the first element. Callers must call :symbol:`bson_iter_next()`, :symbol:`bson_iter_find()`, or :symbol:`bson_iter_find_case()` to advance to an element.

Returns
-------

Returns true if the iter was successfully initialized.

Example
-------

.. code-block:: c

  static void
  print_doc_id (const bson_t *doc)
  {
     bson_iter_t iter;
     bson_oid_t oid;
     char oidstr[25];

     if (bson_iter_init (&iter, doc) && bson_iter_find (&iter, "_id") &&
         BSON_ITER_HOLDS_OID (&iter)) {
        bson_iter_oid (&iter, &oid);
        bson_oid_to_string (&oid, oidstr);
        printf ("%s\n", oidstr);
     } else {
        printf ("Document is missing _id.\n");
     }
  }

  /* alternatively */

  static void
  print_doc_id (const bson_t *doc)
  {
     bson_iter_t iter;
     bson_oid_t oid;
     char oidstr[25];

     if (bson_iter_init_find (&iter, doc, "_id") && BSON_ITER_HOLDS_OID (&iter)) {
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
