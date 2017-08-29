:man_page: mongoc_client_set_apm_callbacks

mongoc_client_set_apm_callbacks()
=================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_client_set_apm_callbacks (mongoc_client_t *client,
                                   mongoc_apm_callbacks_t *callbacks,
                                   void *context);

Register a set of callbacks to receive Application Performance Monitoring events.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``callbacks``: Optional :symbol:`mongoc_apm_callbacks_t`. Pass NULL to clear all callbacks.
* ``context``: Optional pointer to include with each event notification.

Returns
-------

Returns true on success. If any arguments are invalid, returns false and logs an error.

See Also
--------

:doc:`Introduction to Application Performance Monitoring <application-performance-monitoring>`

