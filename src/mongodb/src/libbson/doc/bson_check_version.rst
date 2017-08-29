:man_page: bson_check_version

bson_check_version()
====================

Synopsis
--------

.. code-block:: c

  bool
  bson_check_version (int required_major, int required_minor, int required_micro);

Parameters
----------

* ``required_major``: The minimum major version required.
* ``required_minor``: The minimum minor version required.
* ``required_micro``: The minimum micro version required.

Description
-----------

Check at runtime if this release of libbson meets a required version.

Returns
-------

True if libbson's version is greater than or equal to the required version.

