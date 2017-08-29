:man_page: mongoc_write_concern_set_wtag

mongoc_write_concern_set_wtag()
===============================

Synopsis
--------

.. code-block:: c

  void
  mongoc_write_concern_set_wtag (mongoc_write_concern_t *write_concern,
                                 const char *tag);

Parameters
----------

* ``write_concern``: A :symbol:`mongoc_write_concern_t`.
* ``tag``: A string containing the write tag.

Description
-----------

Sets the write tag that must be satistified for the write to indicate success. Write tags are preset write concerns configured on your MongoDB server. See :symbol:`mongoc_write_concern_t` for more information on this setting.

