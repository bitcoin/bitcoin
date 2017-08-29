:man_page: mongoc_client_set_stream_initiator

mongoc_client_set_stream_initiator()
====================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_client_set_stream_initiator (mongoc_client_t *client,
                                      mongoc_stream_initiator_t initiator,
                                      void *user_data);

The :symbol:`mongoc_client_set_stream_initiator()` function shall associate a given :symbol:`mongoc_client_t` with a new stream initiator. This will completely replace the default transport (buffered TCP, possibly with TLS). The ``initiator`` should fulfill the :symbol:`mongoc_stream_t` contract. ``user_data`` is passed through to the ``initiator`` callback and may be used for whatever run time customization is necessary.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``initiator``: A :symbol:`mongoc_stream_initiator_t <mongoc_client_t>`.
* ``user_data``: User supplied pointer for callback function.

