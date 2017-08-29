:man_page: mongoc_server_description_t

mongoc_server_description_t
===========================

Server description

Synopsis
--------

.. code-block:: c

  #include <mongoc.h>
  typedef struct _mongoc_server_description_t mongoc_server_description_t

``mongoc_server_description_t`` holds information about a mongod or mongos the driver is connected to.

See also :symbol:`mongoc_client_get_server_descriptions()`.

Lifecycle
---------

Clean up with :symbol:`mongoc_server_description_destroy()`.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_server_description_destroy
    mongoc_server_description_host
    mongoc_server_description_id
    mongoc_server_description_ismaster
    mongoc_server_description_new_copy
    mongoc_server_description_round_trip_time
    mongoc_server_description_type
    mongoc_server_descriptions_destroy_all

