:man_page: bson_strfreev

bson_strfreev()
===============

Synopsis
--------

.. code-block:: c

  void
  bson_strfreev (char **strv);

Parameters
----------

* ``strv``: A NULL-terminated array of strings to free, including the array.

Description
-----------

This will free each string in a NULL-terminated array of strings and then the array itself.

