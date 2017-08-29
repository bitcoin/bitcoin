:man_page: mongoc_apm_command_started_get_server_id

mongoc_apm_command_started_get_server_id()
==========================================

Synopsis
--------

.. code-block:: c

  uint32_t
  mongoc_apm_command_started_get_server_id (
     const mongoc_apm_command_started_t *event);

Returns this event's server id.

Parameters
----------

* ``event``: A :symbol:`mongoc_apm_command_started_t`.

Returns
-------

The event's server id.

See Also
--------

:doc:`Introduction to Application Performance Monitoring <application-performance-monitoring>`

