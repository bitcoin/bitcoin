:man_page: bson_iter_document

bson_iter_document()
====================

Synopsis
--------

.. code-block:: c

  #define BSON_ITER_HOLDS_DOCUMENT(iter) \
     (bson_iter_type ((iter)) == BSON_TYPE_DOCUMENT)

  void
  bson_iter_document (const bson_iter_t *iter,
                      uint32_t *document_len,
                      const uint8_t **document);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``document_len``: A location for the length of the document in bytes.
* ``document``: A location for the document buffer.

Description
-----------

The ``bson_iter_document()`` function shall retrieve the raw buffer of a sub-document from ``iter``. ``iter`` *MUST* be on an element that is of type BSON_TYPE_DOCUMENT. This can be verified with :symbol:`bson_iter_type()` or the ``BSON_ITER_HOLDS_DOCUMENT()`` macro.

