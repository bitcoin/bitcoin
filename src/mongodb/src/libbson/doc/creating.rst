:man_page: bson_creating

Creating a BSON Document
========================

The bson_t structure
--------------------

BSON documents are created using the :symbol:`bson_t` structure. This structure encapsulates the necessary logic for encoding using the `BSON Specification <http://bsonspec.org>`_. At the core, :symbol:`bson_t` is a buffer manager and set of encoding routines.

.. tip::

  BSON documents can live on the stack or the heap based on the performance needs or preference of the consumer.

Let's start by creating a new BSON document on the stack. Whenever using libbson, make sure you ``#include <bson.h>``.

.. code-block:: c

  bson_t b;

  bson_init (&b);

This creates an empty document. In JSON, this would be the same as ``{}``.

We can now proceed to adding items to the BSON document. A variety of functions prefixed with ``bson_append_`` can be used based on the type of field you want to append. Let's append a UTF-8 encoded string.

.. code-block:: c

  bson_append_utf8 (&b, "key", -1, "value", -1);

Notice the two ``-1`` parameters. The first indicates that the length of ``key`` in bytes should be determined with ``strlen()``. Alternatively, we could have passed the number ``3``. The same goes for the second ``-1``, but for ``value``.

Libbson provides macros to make this less tedious when using string literals. The following two appends are identical.

.. code-block:: c

  bson_append_utf8 (&b, "key", -1, "value", -1);
  BSON_APPEND_UTF8 (&b, "key", "value");

Now let's take a look at an example that adds a few different field types to a BSON document.

.. code-block:: c

  bson_t b = BSON_INITIALIZER;

  BSON_APPEND_INT32 (&b, "a", 1);
  BSON_APPEND_UTF8 (&b, "hello", "world");
  BSON_APPEND_BOOL (&b, "bool", true);

Notice that we omitted the call to :symbol:`bson_init()`. By specifying ``BSON_INITIALIZER`` we can remove the need to initialize the structure to a base state.

Sub-Documents and Sub-Arrays
----------------------------

To simplify the creation of sub-documents and arrays, :symbol:`bson_append_document_begin()` and :symbol:`bson_append_array_begin()` exist. These can be used to build a sub-document using the parent documents memory region as the destination buffer.

.. code-block:: c

  bson_t parent;
  bson_t child;
  char *str;

  bson_init (&parent);
  bson_append_document_begin (&parent, "foo", 3, &child);
  bson_append_int32 (&child, "baz", 3, 1);
  bson_append_document_end (&parent, &child);

  str = bson_as_canonical_extended_json (&parent, NULL);
  printf ("%s\n", str);
  bson_free (str);

  bson_destroy (&parent);

.. code-block:: none

  { "foo" : { "baz" : 1 } }

Simplified BSON C Object Notation
---------------------------------

Creating BSON documents by hand can be tedious and time consuming. BCON, or BSON C Object Notation, was added to allow for the creation of BSON documents in a format that looks closer to the destination format.

The following example shows the use of BCON. Notice that values for fields are wrapped in the ``BCON_*`` macros. These are required for the variadic processor to determine the parameter type.

.. code-block:: c

  bson_t *doc;

  doc = BCON_NEW ("foo",
                  "{",
                  "int",
                  BCON_INT32 (1),
                  "array",
                  "[",
                  BCON_INT32 (100),
                  "{",
                  "sub",
                  BCON_UTF8 ("value"),
                  "}",
                  "]",
                  "}");

Creates the following document

.. code-block:: none

  { "foo" : { "int" : 1, "array" : [ 100, { "sub" : "value" } ] } }

