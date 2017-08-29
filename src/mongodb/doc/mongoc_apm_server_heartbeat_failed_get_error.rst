:man_page: mongoc_apm_server_heartbeat_failed_get_error

mongoc_apm_server_heartbeat_failed_get_error()
==============================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_apm_server_heartbeat_failed_get_error (
     const mongoc_apm_server_heartbeat_failed_t *event, bson_error_t *error);

Copies this event's error info.

Parameters
----------

* ``event``: A :symbol:`mongoc_apm_server_heartbeat_failed_t`.
* ``error``: A :symbol:`bson:bson_error_t` to receive the event's error info.

See Also
--------

:doc:`Introduction to Application Performance Monitoring <application-performance-monitoring>`

