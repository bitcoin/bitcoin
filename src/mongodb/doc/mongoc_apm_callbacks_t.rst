:man_page: mongoc_apm_callbacks_t

mongoc_apm_callbacks_t
======================

Notification callbacks

Synopsis
--------

Used to receive notification of events, such as when a MongoDB command begins, succeeds, or fails.

Create a ``mongoc_apm_callbacks_t`` with :symbol:`mongoc_apm_callbacks_new`, set callbacks on it, then pass it to :symbol:`mongoc_client_set_apm_callbacks` or :symbol:`mongoc_client_pool_set_apm_callbacks`.

See Also
--------

:doc:`Introduction to Application Performance Monitoring <application-performance-monitoring>`

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_apm_callbacks_destroy
    mongoc_apm_callbacks_new
    mongoc_apm_set_command_failed_cb
    mongoc_apm_set_command_started_cb
    mongoc_apm_set_command_succeeded_cb

