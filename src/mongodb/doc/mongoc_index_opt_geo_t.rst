:man_page: mongoc_index_opt_geo_t

mongoc_index_opt_geo_t
======================

Synopsis
--------

.. code-block:: c

  #include <mongoc.h>

  typedef struct {
     uint8_t twod_sphere_version;
     uint8_t twod_bits_precision;
     double twod_location_min;
     double twod_location_max;
     double haystack_bucket_size;
     uint8_t *padding[32];
  } mongoc_index_opt_geo_t;

Description
-----------

This structure contains the options that may be used for tuning a GEO index.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_index_opt_geo_get_default
    mongoc_index_opt_geo_init

See Also
--------

:doc:`mongoc_index_opt_t`

:doc:`mongoc_index_opt_wt_t`

