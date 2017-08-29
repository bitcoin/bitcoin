:man_page: mongoc_client_pool_set_apm_callbacks

mongoc_client_pool_set_apm_callbacks()
======================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_client_pool_set_apm_callbacks (mongoc_client_pool_t *pool,
                                        mongoc_apm_callbacks_t *callbacks,
                                        void *context);

Register a set of callbacks to receive Application Performance Monitoring events.

Parameters
----------

* ``pool``: A :symbol:`mongoc_client_pool_t`.
* ``callbacks``: A :symbol:`mongoc_apm_callbacks_t`.
* ``context``: Optional pointer to include with each event notification.

Returns
-------

Returns true on success. If any arguments are invalid, returns false and logs an error.

.. include:: includes/mongoc_client_pool_call_once.txt

See Also
--------

:doc:`Introduction to Application Performance Monitoring <application-performance-monitoring>`

