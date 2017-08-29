:man_page: bson_json_data_reader_ingest

bson_json_data_reader_ingest()
==============================

Synopsis
--------

.. code-block:: c

  void
  bson_json_data_reader_ingest (bson_json_reader_t *reader,
                                const uint8_t *data,
                                size_t len);

Parameters
----------

* ``reader``: A :symbol:`bson_json_reader_t`.
* ``data``: A uint8_t containing data to feed.
* ``len``: A size_t containing the length of ``data``.

Description
-----------

Feed data to a memory based json reader.

