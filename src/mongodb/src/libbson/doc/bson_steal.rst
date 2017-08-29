:man_page: bson_steal

bson_steal()
============

Synopsis
--------

.. code-block:: c

  bool
  bson_steal (bson_t *dst, bson_t *src);

Parameters
----------

* ``dst``: An uninitialized :symbol:`bson_t`.
* ``src``: A :symbol:`bson_t`.

Description
-----------

Efficiently transfer the contents of ``src`` to ``dst`` and destroy ``src``.

Before calling this function, ``src`` must be initialized and ``dst`` must be uninitialized. After this function returns successfully, ``src`` is destroyed, and ``dst`` is initialized and must be freed with :symbol:`bson_destroy`.

For example, if you have a higher-level structure that wraps a :symbol:`bson_t`, use ``bson_steal`` to transfer BSON data into it:

.. code-block:: c

  typedef struct {
     bson_t bson;
  } bson_wrapper_t;


  bson_wrapper_t *
  wrap_bson (bson_t *b)
  {
     bson_wrapper_t *wrapper = bson_malloc (sizeof (bson_wrapper_t));

     if (bson_steal (&wrapper->bson, b)) {
        return wrapper;
     }

     bson_free (wrapper);
     return NULL;
  }


  void
  bson_wrapper_destroy (bson_wrapper_t *wrapper)
  {
     bson_destroy (&wrapper->bson);
     bson_free (wrapper);
  }


  int
  main (int argc, char *argv[])
  {
     bson_t bson = BSON_INITIALIZER;
     bson_wrapper_t *wrapper;

     BSON_APPEND_UTF8 (&bson, "key", "value");

     /* now "bson" is destroyed */
     wrapper = wrap_bson (&bson);

     /* clean up */
     bson_wrapper_destroy (wrapper);
  }

See also :symbol:`bson_destroy_with_steal`, a lower-level function that returns the raw contents of a :symbol:`bson_t`.

Returns
-------

Returns ``true`` if ``src`` was successfully moved to ``dst``, ``false`` if ``src`` is invalid, or was statically initialized, or another error occurred.

