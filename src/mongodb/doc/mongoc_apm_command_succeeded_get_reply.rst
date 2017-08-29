:man_page: mongoc_apm_command_succeeded_get_reply

mongoc_apm_command_succeeded_get_reply()
========================================

Synopsis
--------

.. code-block:: c

  const bson_t *
  mongoc_apm_command_succeeded_get_reply (
     const mongoc_apm_command_succeeded_t *event);

Returns this event's reply. The data is only valid in the scope of the callback that receives this event; copy it if it will be accessed after the callback returns.

Parameters
----------

* ``event``: A :symbol:`mongoc_apm_command_succeeded_t`.

Returns
-------

A :symbol:`bson:bson_t` that should not be modified or freed.

See Also
--------

:doc:`Introduction to Application Performance Monitoring <application-performance-monitoring>`

