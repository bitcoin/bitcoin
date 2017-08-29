:man_page: mongoc_matcher_destroy

mongoc_matcher_destroy()
========================

Synopsis
--------

.. code-block:: c

  void
  mongoc_matcher_destroy (mongoc_matcher_t *matcher);

Release all resources associated with ``matcher`` including freeing the structure.

Deprecated
----------

.. warning::

  ``mongoc_matcher_t`` is deprecated and will be removed in version 2.0.

Parameters
----------

* ``matcher``: A :symbol:`mongoc_matcher_t`.

