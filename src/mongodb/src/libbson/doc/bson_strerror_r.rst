:man_page: bson_strerror_r

bson_strerror_r()
=================

Synopsis
--------

.. code-block:: c

  char *
  bson_strerror_r (int err_code, char *buf, size_t buflen);

Parameters
----------

* ``err_code``: An errno.
* ``buf``: A location to store the string.
* ``buflen``: The size of ``buf``.

Description
-----------

This is a portability wrapper around ``strerror()``.

Returns
-------

The passed in ``buf`` parameter or an internal string.

