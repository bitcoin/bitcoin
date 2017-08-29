:man_page: mongoc_apm_command_started_get_command_name

mongoc_apm_command_started_get_command_name()
=============================================

Synopsis
--------

.. code-block:: c

  const char *
  mongoc_apm_command_started_get_command_name (
     const mongoc_apm_command_started_t *event);

Returns this event's command name. The data is only valid in the scope of the callback that receives this event; copy it if it will be accessed after the callback returns.

Parameters
----------

* ``event``: A :symbol:`mongoc_apm_command_started_t`.

Returns
-------

A string that should not be modified or freed.

See Also
--------

:doc:`Introduction to Application Performance Monitoring <application-performance-monitoring>`

