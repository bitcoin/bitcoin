:man_page: bson_json_reader_read

bson_json_reader_read()
=======================

Synopsis
--------

.. code-block:: c

  int
  bson_json_reader_read (bson_json_reader_t *reader,
                         bson_t *bson,
                         bson_error_t *error);

Parameters
----------

* ``reader``: A :symbol:`bson_json_reader_t`.
* ``bson``: A :symbol:`bson_t`.
* ``error``: A :symbol:`bson_error_t`.

Description
-----------

Reads the next BSON document from the underlying JSON source.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

1 if successful and data was read. 0 if successful and no data was read. -1 if there was an error.

.. only:: html

  .. taglist:: See Also:
    :tags: json
