
Algorithms
==========

This module provides overloads of standard algorithms that leverage
the internal structure of the immutable containers to provide faster
iteration. These are drop-in replacements of the respective STL
algorithms that can be a few times faster when applied on immutable
sequences.

For further convenience they can be passed in just a container where
the standard library algorithms require being passed in two iterators.

.. note::

   They are a similar idea to `structure-aware iterators`_
   but implemented using higher order functions in order to support
   structures of any depth.  The downside is that this sometimes
   causes the compiler to generate bigger executable files.

.. _structure-aware iterators: https://www.youtube.com/watch?v=T3oA3zAMH9M


-----

.. doxygengroup:: algorithm
   :project: immer
   :content-only:
