:man_page: mongoc_find_and_modify_opts_new

mongoc_find_and_modify_opts_new()
=================================

Synopsis
--------

.. code-block:: c

  mongoc_find_and_modify_opts_t *
  mongoc_find_and_modify_opts_new (void);

Returns
-------

Creates a newly allocated find and modify builder structure that is used to create a findAndModify command. This should be freed with :symbol:`mongoc_find_and_modify_opts_destroy()` when no longer in use.

