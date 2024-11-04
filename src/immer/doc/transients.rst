.. _transient:

Transients
==========

*Transients* is a concept borrowed `from Clojure
<clojure-transients>`_, with some twists to make it more idiomatic
in C++.  Essentially, they are a mutable interface built on top of the
same data structures that implement the immutable containers under the
hood.

These can be useful for :ref:`performing efficient batch
updates<batch-update>` or :ref:`interfacing with standard
algorithms<std-compat>`.

.. _clojure-transients: https://clojure.org/reference/transients

array_transient
---------------

.. doxygenclass:: immer::array_transient
    :members:
    :undoc-members:

vector_transient
----------------

.. doxygenclass:: immer::vector_transient
    :members:
    :undoc-members:

flex_vector_transient
---------------------

.. doxygenclass:: immer::flex_vector_transient
    :members:
    :undoc-members:

set_transient
-------------

.. doxygenclass:: immer::set_transient
    :members:
    :undoc-members:

map_transient
-------------

.. doxygenclass:: immer::map_transient
    :members:
    :undoc-members:

table_transient
---------------

.. doxygenclass:: immer::table_transient
    :members:
    :undoc-members:
