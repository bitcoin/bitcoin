:man_page: mongoc_apm_callbacks_new

mongoc_apm_callbacks_new()
==========================

Synopsis
--------

.. code-block:: c

  mongoc_apm_callbacks_t *
  mongoc_apm_callbacks_new (void);

Create a struct to hold event-notification callbacks.

Returns
-------

A new ``mongoc_apm_callbacks_t`` you must free with :symbol:`mongoc_apm_callbacks_destroy`.

See Also
--------

:doc:`Introduction to Application Performance Monitoring <application-performance-monitoring>`

