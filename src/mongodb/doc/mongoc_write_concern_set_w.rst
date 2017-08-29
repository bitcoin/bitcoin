:man_page: mongoc_write_concern_set_w

mongoc_write_concern_set_w()
============================

Synopsis
--------

.. code-block:: c

  void
  mongoc_write_concern_set_w (mongoc_write_concern_t *write_concern, int32_t w);

Parameters
----------

* ``write_concern``: A :symbol:`mongoc_write_concern_t`.
* ``w``: A positive ``int32_t`` or zero.

Description
-----------

Sets the ``w`` value for the write concern. See :symbol:`mongoc_write_concern_t` for more information on this setting.

