:man_page: mongoc_check_version

mongoc_check_version()
======================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_check_version (int required_major,
                        int required_minor,
                        int required_micro);

Parameters
----------

* ``required_major``: The minimum major version required.
* ``required_minor``: The minimum minor version required.
* ``required_micro``: The minimum micro version required.

Returns
-------

True if libmongoc's version is greater than or equal to the required version.

