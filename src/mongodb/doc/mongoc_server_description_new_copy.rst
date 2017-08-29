:man_page: mongoc_server_description_new_copy

mongoc_server_description_new_copy()
====================================

Synopsis
--------

.. code-block:: c

  mongoc_server_description_t *
  mongoc_server_description_new_copy (
     const mongoc_server_description_t *description);

Parameters
----------

* ``description``: A :symbol:`mongoc_server_description_t`.

Description
-----------

This function copies the given server description and returns a new server description object.  The caller is responsible for destroying the new copy.

Returns
-------

A copy of the original server description.

