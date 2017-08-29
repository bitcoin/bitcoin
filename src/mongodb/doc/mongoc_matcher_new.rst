:man_page: mongoc_matcher_new

mongoc_matcher_new()
====================

Synopsis
--------

.. code-block:: c

  mongoc_matcher_t *
  mongoc_matcher_new (const bson_t *query, bson_error_t *error);

Create a new :symbol:`mongoc_matcher_t` using the query specification provided.

Deprecated
----------

.. warning::

  ``mongoc_matcher_t`` is deprecated and will be removed in version 2.0.

Parameters
----------

* ``query``: A :symbol:`bson:bson_t`.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

A newly allocated :symbol:`mongoc_matcher_t` that should be freed with :symbol:`mongoc_matcher_destroy()` when no longer in use. Upon failure, ``NULL`` is returned and ``error`` is set. This could happen if ``query`` contains an invalid query specification.

