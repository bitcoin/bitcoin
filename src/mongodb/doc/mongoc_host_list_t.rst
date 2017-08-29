:man_page: mongoc_host_list_t

mongoc_host_list_t
==================

Synopsis
--------

.. code-block:: c

  typedef struct {
     mongoc_host_list_t *next;
     char host[BSON_HOST_NAME_MAX + 1];
     char host_and_port[BSON_HOST_NAME_MAX + 7];
     uint16_t port;
     int family;
     void *padding[4];
  } mongoc_host_list_t;

Description
-----------

The host and port of a MongoDB server. Can be part of a linked list: for example the return value of :symbol:`mongoc_uri_get_hosts` when multiple hosts are provided in the MongoDB URI.

See Also
--------

:symbol:`mongoc_uri_get_hosts` and :symbol:`mongoc_cursor_get_host`.

