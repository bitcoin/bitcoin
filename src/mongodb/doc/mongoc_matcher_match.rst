:man_page: mongoc_matcher_match

mongoc_matcher_match()
======================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_matcher_match (const mongoc_matcher_t *matcher, const bson_t *document);

This function will check to see if the query compiled in ``matcher`` matches ``document``.

Deprecated
----------

.. warning::

  ``mongoc_matcher_t`` is deprecated and will be removed in version 2.0.

Parameters
----------

* ``matcher``: A :symbol:`mongoc_matcher_t`.
* ``query``: A :symbol:`bson:bson_t` that contains the query.

Returns
-------

``true`` if ``document`` matches the query specification provided to :symbol:`mongoc_matcher_new()`. Otherwise, ``false``.

