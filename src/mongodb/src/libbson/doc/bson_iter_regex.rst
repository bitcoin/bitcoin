:man_page: bson_iter_regex

bson_iter_regex()
=================

Synopsis
--------

.. code-block:: c

  const char *
  bson_iter_regex (const bson_iter_t *iter, const char **options);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``options``: A (null).

Description
-----------

The :symbol:`bson_iter_regex()` function shall retrieve the contents of a BSON_TYPE_REGEX element currently observed by ``iter``.

It is invalid to call this function when not observing an element of type BSON_TYPE_REGEX.

Returns
-------

A string containing the regex which should not be modified or freed. ``options`` is set to the options provided for the regex.

