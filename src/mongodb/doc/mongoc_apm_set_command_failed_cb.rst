:man_page: mongoc_apm_set_command_failed_cb

mongoc_apm_set_command_failed_cb()
==================================

Synopsis
--------

.. code-block:: c

  typedef void (*mongoc_apm_command_failed_cb_t) (
     const mongoc_apm_command_failed_t *event);

  void
  mongoc_apm_set_command_failed_cb (mongoc_apm_callbacks_t *callbacks,
                                    mongoc_apm_command_failed_cb_t cb);

Receive an event notification whenever the driver fails to execute a MongoDB operation.

Parameters
----------

* ``callbacks``: A :symbol:`mongoc_apm_callbacks_t`.
* ``cb``: A function to call with a :symbol:`mongoc_apm_command_failed_t` whenever a MongoDB operation fails.

See Also
--------

:doc:`Introduction to Application Performance Monitoring <application-performance-monitoring>`

