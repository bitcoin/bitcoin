:man_page: mongoc_index_opt_wt_t

mongoc_index_opt_wt_t
=====================

Synopsis
--------

.. code-block:: c

  #include <mongoc.h>

  typedef struct {
     mongoc_index_opt_storage_t base;
     const char *config_str;
     void *padding[8];
  } mongoc_index_opt_wt_t;

Description
-----------

This structure contains the options that may be used for tuning a WiredTiger specific index.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_index_opt_wt_get_default
    mongoc_index_opt_wt_init

See Also
--------

:doc:`mongoc_index_opt_t`

:doc:`mongoc_index_opt_geo_t`

