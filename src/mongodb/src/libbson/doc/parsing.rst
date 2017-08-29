:man_page: bson_parsing

Parsing and Iterating BSON Documents
====================================

Parsing
-------

BSON documents are lazily parsed as necessary. To begin parsing a BSON document, use one of the provided Libbson functions to create a new :symbol:`bson_t` from existing data such as :symbol:`bson_new_from_data()`. This will make a copy of the data so that additional mutations may occur to the BSON document.

.. tip::

  If you only want to parse a BSON document and have no need to mutate it, you may use :symbol:`bson_init_static()` to avoid making a copy of the data.

.. code-block:: c

  bson_t *b;

  b = bson_new_from_data (my_data, my_data_len);
  if (!b) {
     fprintf (stderr, "The specified length embedded in <my_data> did not match "
                      "<my_data_len>\n");
     return;
  }

  bson_destroy (b);

Only two checks are performed when creating a new :symbol:`bson_t` from an existing buffer. First, the document must begin with the buffer length, matching what was expected by the caller. Second, the document must end with the expected trailing ``\0`` byte.

To parse the document further we use a :symbol:`bson_iter_t` to iterate the elements within the document. Let's print all of the field names in the document.

.. code-block:: c

  bson_t *b;
  bson_iter_t iter;

  if ((b = bson_new_from_data (my_data, my_data_len))) {
     if (bson_iter_init (&iter, b)) {
        while (bson_iter_next (&iter)) {
           printf ("Found element key: \"%s\"\n", bson_iter_key (&iter));
        }
     }
     bson_destroy (b);
  }

Converting a document to JSON uses a :symbol:`bson_iter_t` and :symbol:`bson_visitor_t` to iterate all fields of a BSON document recursively and generate a UTF-8 encoded JSON string.

.. code-block:: c

  bson_t *b;
  char *json;

  if ((b = bson_new_from_data (my_data, my_data_len))) {
     if ((json = bson_as_canonical_extended_json (b, NULL))) {
        printf ("%s\n", json);
        bson_free (json);
     }
     bson_destroy (b);
  }

Recursing into Sub-Documents
----------------------------

Libbson provides convenient sub-iterators to dive down into a sub-document or sub-array. Below is an example that will dive into a sub-document named "foo" and print it's field names.

.. code-block:: c

  bson_iter_t iter;
  bson_iter_t *child;
  char *json;

  if (bson_iter_init_find (&iter, doc, "foo") &&
      BSON_ITER_HOLDS_DOCUMENT (&iter) && bson_iter_recurse (&iter, &child)) {
     while (bson_iter_next (&child)) {
        printf ("Found sub-key of \"foo\" named \"%s\"\n",
                bson_iter_key (&child));
     }
  }

Finding Fields using Dot Notation
---------------------------------

Using the :symbol:`bson_iter_recurse()` function exemplified above, :symbol:`bson_iter_find_descendant()` can find a field for you using the MongoDB style path notation such as "foo.bar.0.baz".

Let's create a document like ``{"foo": {"bar": [{"baz: 1}]}}`` and locate the ``"baz"`` field.

.. code-block:: c

  bson_t *b;
  bson_iter_t iter;
  bson_iter_t baz;

  b =
     BCON_NEW ("foo", "{", "bar", "[", "{", "baz", BCON_INT32 (1), "}", "]", "}");

  if (bson_iter_init (&iter, b) &&
      bson_iter_find_descendant (&iter, "foo.bar.0.baz", &baz) &&
      BSON_ITER_HOLDS_INT32 (&baz)) {
     printf ("baz = %d\n", bson_iter_int32 (&baz));
  }

  bson_destroy (b);

Validating a BSON Document
--------------------------

If all you want to do is validate that a BSON document is valid, you can use :symbol:`bson_validate()`.

.. code-block:: c

  size_t err_offset;

  if (!bson_validate (doc, BSON_VALIDATE_NONE, &err_offset)) {
     fprintf (stderr,
              "The document failed to validate at offset: %u\n",
              (unsigned) err_offset);
  }

See the :symbol:`bson_validate()` documentation for more information and examples.

