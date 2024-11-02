.. image:: https://github.com/arximboldi/immer/workflows/test/badge.svg
   :target: https://github.com/arximboldi/immer/actions?query=workflow%3Atest+branch%3Amaster
   :alt: GitHub Actions Badge

.. image:: https://codecov.io/gh/arximboldi/immer/branch/master/graph/badge.svg
   :target: https://codecov.io/gh/arximboldi/immer
   :alt: CodeCov Badge

.. image:: https://cdn.rawgit.com/arximboldi/immer/355a113782aedc2ea22463444014809269c2376d/doc/_static/sinusoidal-badge.svg
   :target: https://sinusoid.al
   :alt: Sinusoidal Engineering badge
   :align: right

.. raw:: html

   <picture>
     <source media="(prefers-color-scheme: dark)" srcset="https://cdn.rawgit.com/arximboldi/immer/3888170d247359cc0905eed548cd46897caef0f4/doc/_static/logo-black.svg">
     <img width="100%" src="https://cdn.rawgit.com/arximboldi/immer/3888170d247359cc0905eed548cd46897caef0f4/doc/_static/logo-front.svg" alt="Logotype">
   </picture>

.. include:introduction/start

**immer** is a library of persistent_ and immutable_ data structures
written in C++.  These enable whole new kinds of architectures for
interactive and concurrent programs of striking simplicity,
correctness, and performance.

.. _persistent: https://en.wikipedia.org/wiki/Persistent_data_structure
.. _immutable:  https://en.wikipedia.org/wiki/Immutable_object

* **Documentation** (Contents_)
* **Code** (GitHub_)
* **CppCon'17 Talk**: *Postmodern Immutable Data Structures* (YouTube_, Slides_)
* **ICFP'17 Paper**: *Persistence for the masses* (Preprint_)

.. _contents: https://sinusoid.es/immer/#contents
.. _github: https://github.com/arximboldi/immer
.. _youtube: https://www.youtube.com/watch?v=sPhpelUfu8Q
.. _slides: https://sinusoid.es/talks/immer-cppcon17
.. _preprint: https://public.sinusoid.es/misc/immer/immer-icfp17.pdf


  .. raw:: html

     <a href="https://www.patreon.com/sinusoidal">
         <img align="right" src="https://cdn.rawgit.com/arximboldi/immer/master/doc/_static/patreon.svg">
     </a>

  This library has full months of *pro bono* research and development
  invested in it.  This is just the first step in a long-term vision
  of making interactive and concurrent C++ programs easier to
  write. **Put your logo here and help this project's long term
  sustainability by buying a sponsorship package:** immer@sinusoid.al

.. include:index/end

Example
-------

.. github does not support the ``literalinclude`` directive.  This
   example is copy pasted from ``example/vector/intro.cpp``

.. code-block:: c++

   #include <immer/vector.hpp>
   int main()
   {
       const auto v0 = immer::vector<int>{};
       const auto v1 = v0.push_back(13);
       assert(v0.size() == 0 && v1.size() == 1 && v1[0] == 13);

       const auto v2 = v1.set(0, 42);
       assert(v1[0] == 13 && v2[0] == 42);
   }
..

  For a **complete example** check `Ewig, a simple didactic
  text-editor <https://github.com/arximboldi/ewig>`_ built with this
  library.  You may also wanna check `Lager, a Redux-like library
  <https://github.com/arximboldi/lager>`_ for writing interactive
  software in C++ using a value-oriented design.


Why?
----

In the last few years, there has been a growing interest in immutable
data structures, motivated by the horizontal scaling of our processing
power and the ubiquity of highly interactive systems.  Languages like
Clojure_ and Scala_ provide them by default, and implementations
for JavaScript like Mori_ and Immutable.js_ are widely used,
specially in combination with modern UI frameworks like React_.

Interactivity
    Thanks to *persistence* and *structural sharing*, new values can
    be efficiently compared with old ones.  This enables simpler ways of
    *reasoning about change* that sit at the core of modern
    interactive systems programming paradigms like `reactive
    programming`_.

Concurrency
    Passing immutable data structures by value does not need to copy
    any data. In the absence of mutation, data can be safely read
    from multiple concurrent processes, and enable concurrency
    patterns like `share by communicating`_ efficiently.

Parallelism
   Some recent immutable data structures have interesting properties
   like :math:`O(log(n))` concatenation, which enable new kinds of
   `parallelization algorithms`_.

.. _clojure: http://clojure.org/reference/data_structures
.. _scala: http://docs.scala-lang.org/overviews/collections/overview.html

.. _mori: https://swannodette.github.io/mori/
.. _immutable.js: https://github.com/facebook/immutable-js
.. _react: https://facebook.github.io/react/

.. _reactive programming: https://en.wikipedia.org/wiki/Reactive_programming
.. _share by communicating: https://blog.golang.org/share-memory-by-communicating
.. _parallelization algorithms: http://docs.scala-lang.org/overviews/parallel-collections/overview.html

Features
--------

Idiomatic
    This library doesn't pretend that it is written in Haskell.  It
    leverages features from recent standards to provide an API that is
    both efficient and natural for a C++ developer.

Performant
    You use C++ because you need this.  *Immer* implements state of
    the art data structures with efficient cache utilization and have
    been proven production ready in other languages.  It also includes
    our own improvements over that are only possible because of the
    C++'s ability to abstract over memory layout.  We monitor the
    performance impact of every change by collecting `benchmark
    results`_ directly from CI.

.. _benchmark results: https://public.sinusoid.es/misc/immer/reports/

Customizable
    We leverage templates and `policy-based design`_ to build
    data-structures that can be adapted to work efficiently for
    various purposes and architectures, for example, by choosing among
    various `memory management strategies`.  This turns
    *immer* into a good foundation to provide immutable data
    structures to higher level languages with a C runtime, like
    Python_ or Guile_.

.. _python: https://www.python.org/
.. _guile: https://www.gnu.org/software/guile/
.. _policy-based design: https://en.wikipedia.org/wiki/Policy-based_design
.. _memory management strategies: https://sinusoid.es/immer/memory.html

Dependencies
------------

This library is written in **C++14** and a compliant compiler is
necessary.  It is `continuously tested`_ with Clang 3.8 and GCC 6, but
it might work with other compilers and versions.

No external library is necessary and there are no other requirements.

.. _continuously tested: https://travis-ci.org/arximboldi/immer

Usage
-----

This is a **header only** library.  You can just copy the ``immer``
subfolder somewhere in your *include path*.

If you are using the `Nix package manager`_ (we strongly recommend it)
you can just::

    nix-env -if https://github.com/arximboldi/immer/archive/master.tar.gz

Alternatively, you can use `CMake`_ to install the library in your
system once you have manually cloned the repository::

    mkdir -p build && cd build
    cmake .. && sudo make install

.. _nix package manager: https://nixos.org/nix
.. _cmake: https://cmake.org/

Development
-----------

In order to develop the library, you will need to compile and run the
examples, tests and benchmarks.  These require some additional tools.
The easiest way to install them is by using the `Nix package
manager`_.  At the root of the repository just type::

    nix-shell

This will download all required dependencies and create an isolated
environment in which you can use these dependencies, without polluting
your system.

Then you can proceed to generate a development project using `CMake`_::

    mkdir build && cd build
    cmake ..

From then on, one may build and run all tests by doing::

    make check

In order to build and run all benchmarks when running ``make check``,
run ``cmake`` again with the option ``-DCHECK_BENCHMARKS=1``.  The
results of running the benchmarks will be saved to a folder
``reports/`` in the project root.

License
-------

**This software is licensed under the Boost Software License 1.0**.

.. image:: https://upload.wikimedia.org/wikipedia/commons/c/cd/Boost.png
   :alt: Boost logo
   :target: http://boost.org/LICENSE_1_0.txt
   :align: right

The full text of the license is can be accessed `via this link
<http://boost.org/LICENSE_1_0.txt>`_ and is also included
in the ``LICENSE`` file of this software package.
