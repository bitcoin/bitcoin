
Python bindings
===============

This library includes experimental bindings bring efficient immutable
vectors for the Python language.  They were developed as part of the
research for the `ICFP'17 paper`_.  The interface is quite
**incomplete**, yet you can already do some things like:

.. literalinclude:: ../extra/python/example.py
   :language: python
   :start-after: intro/start
   :end-before:  intro/end
..

    **Do you want to help** making these bindings complete and production
    ready?  Drop a line at `immer@sinusoid.al
    <mailto:immer@sinusoid.al>`_ or `open an issue on Github
    <https://github.com/arximboldi/immer>`_

Installation
------------
::

    pip install --user git+https://github.com/arximboldi/immer.git

Benchmarks
----------

The library includes a set of benchmarks that compare it to
`pyrsistent <https://github.com/tobgu/pyrsistent>`_.  You can see the
results in the `ICFP'17 paper`_.  If you want to run them yourself,
you need to install some dependencies::

     pip install --user pytest-benchmark pyrsistent

Then you need to clone the `project repository
<https://github.com/arximboldi/immer>`_ and from its root, run::

     pytest extra/python/benchmark

.. _ICFP'17 paper: https://public.sinusoid.es/misc/immer/immer-icfp17.pdf
