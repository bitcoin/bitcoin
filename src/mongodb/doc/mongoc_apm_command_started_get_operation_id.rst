:man_page: mongoc_apm_command_started_get_operation_id

mongoc_apm_command_started_get_operation_id()
=============================================

Synopsis
--------

.. code-block:: c

  int64_t
  mongoc_apm_command_started_get_operation_id (
     const mongoc_apm_command_started_t *event);

Returns this event's operation id. This number correlates all the commands in a bulk operation, or all the "find" and "getMore" commands required to stream a large query result.

Parameters
----------

* ``event``: A :symbol:`mongoc_apm_command_started_t`.

Returns
-------

The event's operation id.

See Also
--------

:doc:`Introduction to Application Performance Monitoring <application-performance-monitoring>`

