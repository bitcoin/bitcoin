:man_page: mongoc_apm_command_failed_get_host

mongoc_apm_command_failed_get_host()
====================================

Synopsis
--------

.. code-block:: c

  const mongoc_host_list_t *
  mongoc_apm_command_failed_get_host (const mongoc_apm_command_failed_t *event);

Returns this event's host. This :symbol:`mongoc_host_list_t` is *not* part of a linked list, it is solely the server for this event. The data is only valid in the scope of the callback that receives this event; copy it if it will be accessed after the callback returns.

Parameters
----------

* ``event``: A :symbol:`mongoc_apm_command_failed_t`.

Returns
-------

A :symbol:`mongoc_host_list_t` that should not be modified or freed.

See Also
--------

:doc:`Introduction to Application Performance Monitoring <application-performance-monitoring>`

