:man_page: mongoc_server_description_round_trip_time

mongoc_server_description_round_trip_time()
===========================================

Synopsis
--------

.. code-block:: c

  int64_t
  mongoc_server_description_round_trip_time (
     const mongoc_server_description_t *description);

Parameters
----------

* ``description``: A :symbol:`mongoc_server_description_t`.

Description
-----------

Get the server's round trip time in milliseconds. This is the client's measurement of the duration of an "ismaster" command.

