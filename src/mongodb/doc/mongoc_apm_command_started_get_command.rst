:man_page: mongoc_apm_command_started_get_command

mongoc_apm_command_started_get_command()
========================================

Synopsis
--------

.. code-block:: c

  const bson_t *
  mongoc_apm_command_started_get_command (
     const mongoc_apm_command_started_t *event);

Returns this event's command. The data is only valid in the scope of the callback that receives this event; copy it if it will be accessed after the callback returns.

Parameters
----------

* ``event``: A :symbol:`mongoc_apm_command_started_t`.

Returns
-------

A :symbol:`bson:bson_t` that should not be modified or freed.

See Also
--------

:doc:`Introduction to Application Performance Monitoring <application-performance-monitoring>`

