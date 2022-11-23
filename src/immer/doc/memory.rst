Memory management
=================

Memory management is specially important for immutable data
structures.  This is mainly due to:

#. In order to preserve the old value, new memory has to be allocated
   to store the new data whenever a container is manipulated.  Thus,
   more allocations are performed *when changing* than with traditional
   mutable data structures.

#. In order to support *structural sharing* transparently, some kind
   of garbage collection mechanism is required.  Passing immutable
   data structures around is, internally, just passing references,
   thus the system needs to figure out somehow when old values are not
   referenced anymore and should be deallocated.

Thus, most containers in this library can be customized via policies_
in order to use different *allocation* and *garbage collection*
strategies.

.. doxygentypedef:: immer::default_memory_policy
.. doxygentypedef:: immer::default_heap_policy
.. doxygentypedef:: immer::default_refcount_policy

.. _policies: https://en.wikipedia.org/wiki/Policy-based_design

.. _memory policy:

Memory policy
-------------

.. doxygenstruct:: immer::memory_policy
    :members:
    :undoc-members:

.. _gc:

Example: tracing garbage collection
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

It is note worthy that all aspects of a
:cpp:class:`immer::memory_policy` are not completely orthogonal.

Let's say you want to use a `tracing garbage collector`_. Actually, we
already provide :cpp:class:`a heap <immer::gc_heap>` that interfaces
with the `Boehm's conservative garbage collector`_.  Chunks of memory
allocated with this heap do not need to be deallocated, instead, after
a certain number of allocations, the heap itself will scan the stack
and all allocated memory to find references to other blocks of memory.
The memory that is not referenced anymore is automatically *freed*.
Thus, no reference counting mechanism is needed, and it makes no sense
to use this heap with anything else than the
:cpp:class:`immer::no_refcount_policy`.  Also, a big object can be
separated in parts that contain pointers and parts that do not, which
make scanning references faster.  So it makes most sense to use
``prefer_fewer_bigger_objects = false``.

.. note:: There are few considerations to note when using
          :cpp:class:`gc_heap` with containers.  Please make sure to
          read :cpp:class:`its documentation section <immer::gc_heap>`
          before using it.

.. literalinclude:: ../example/vector/gc.cpp
   :language: c++
   :start-after: example/start
   :end-before:  example/end


.. _boehm's conservative garbage collector: https://github.com/ivmai/bdwgc
.. _tracing garbage collector: https://en.wikipedia.org/wiki/Tracing_garbage_collection

Heaps
-----

A **heap policy** is a `metafunction class`_ that given the sizes of
the objects that we want to allocate, it *returns* a heap that can
allocate objects of those sizes.

.. _metafunction class: http://www.boost.org/doc/libs/1_62_0/libs/mpl/doc/refmanual/metafunction-class.html

A **heap** is a type with a methods ``void* allocate(std::size_t)``
and ``void deallocate(void*)`` that return and release raw memory.
For a canonical model of this concept check the
:cpp:class:`immer::cpp_heap`.

.. note:: Currently, *heaps* can only have **global state**.  Having
          internal state poses conceptual problems for immutable data
          structures: should a `const` method of a container modify
          its internal allocator state?  Should every immutable
          container object have its own internal state, or new objects
          made from another one just keep a reference to the allocator
          of the parent?

          On the other hand, having some **scoped state** does make
          sense for some use-cases of immutable data structures.  For
          example, we might want to support variations of `region
          based allocation`_.  This interface might evolve to evolve
          to support some kind of non-global state to accommodate
          these use cases.

.. _region based allocation: https://en.wikipedia.org/wiki/Region-based_memory_management

.. admonition:: Why not use the standard allocator interface?

   The standard allocator API was not designed to support different
   allocation strategies, but to abstract over the underlying memory
   model instead.  In C++11 the situation improves, but the new API is
   *stateful*, posing various challenges as described in the previous
   note.  So far it was easier to provide our own allocation
   interface.  In the future, we will provide adaptors so
   standard-compatible allocators can also be used with ``immer``.

Heap policies
~~~~~~~~~~~~~

.. doxygenstruct:: immer::heap_policy
   :members:
   :undoc-members:

.. doxygenstruct:: immer::free_list_heap_policy

Standard heap
~~~~~~~~~~~~~

.. doxygenstruct:: immer::cpp_heap
   :members:

Malloc heap
~~~~~~~~~~~

.. doxygenstruct:: immer::malloc_heap
   :members:

Garbage collected heap
~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: immer::gc_heap

Heap adaptors
~~~~~~~~~~~~~

Inspired by `Andrei Alexandrescu's talk on allocators <allocation
vexation>`_ and `Emery Berger's heap layers <heap layers>`_ we provide
allocator adaptors that can be combined using C++ mixins.  These
enable building more complex allocator out of simpler strategies, or
provide application specific optimizations on top of general
allocators.

.. _allocation vexation: https://www.youtube.com/watch?v=LIb3L4vKZ7U
.. _heap layers: https://github.com/emeryberger/Heap-Layers

.. doxygenstruct:: immer::with_data

.. doxygenstruct:: immer::free_list_heap

.. doxygenstruct:: immer::thread_local_free_list_heap

.. doxygenstruct:: immer::unsafe_free_list_heap

.. doxygenstruct:: immer::identity_heap

.. doxygenstruct:: immer::debug_size_heap

.. doxygenstruct:: immer::split_heap

.. _rc:

Reference counting
------------------

`Reference counting`_ is the most commonly used garbage collection
strategy for C++.  It can be implemented non-intrusively, in a way
orthogonal to the allocation strategy. It is deterministic, playing
well with RAII_.

A `memory policy`_ can provide a reference counting strategy that
containers can use to track their contents.

.. _reference counting: https://en.wikipedia.org/wiki/Reference_counting
.. _raii: https://en.wikipedia.org/wiki/Resource_acquisition_is_initialization

.. doxygenstruct:: immer::refcount_policy

.. doxygenstruct:: immer::unsafe_refcount_policy

.. doxygenstruct:: immer::no_refcount_policy

Transience
----------

In order to support `transients`, it is needed to provide a mechanism
to track the ownership of the data allocated inside the container.
This concept is encapsulated in *transience policies*.

Note that when :ref:`reference counting <rc>` is available, no such mechanism is
needed.  However, when :ref:`tracing garbage collection<gc>` is used instead,
a special policy has to be provided.  Otherwise, the transient API is
still available, but it will perform poorly, since it won't be able to
mutate any data in place.

.. doxygenstruct:: immer::no_transience_policy

.. doxygenstruct:: immer::gc_transience_policy
