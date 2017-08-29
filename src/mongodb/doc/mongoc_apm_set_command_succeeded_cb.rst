:man_page: mongoc_apm_set_command_succeeded_cb

mongoc_apm_set_command_succeeded_cb()
=====================================

Synopsis
--------

.. code-block:: c

  typedef void (*mongoc_apm_command_succeeded_cb_t) (
     const mongoc_apm_command_succeeded_t *event);

  void
  mongoc_apm_set_command_succeeded_cb (mongoc_apm_callbacks_t *callbacks,
                                       mongoc_apm_command_succeeded_cb_t cb);

Receive an event notification whenever the driver completes a MongoDB operation.

Parameters
----------

* ``callbacks``: A :symbol:`mongoc_apm_callbacks_t`.
* ``cb``: A function to call with a :symbol:`mongoc_apm_command_succeeded_t` whenever the driver completes a MongoDB operation.

See Also
--------

:doc:`Introduction to Application Performance Monitoring <application-performance-monitoring>`

