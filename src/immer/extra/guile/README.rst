
Guile bindings
==============

This library includes experimental bindings bring efficient immutable
vectors for the `GNU Guile`_ Scheme implementation.  The interface is
somewhat **incomplete**, but you can already do something interesting
things like:

.. literalinclude:: ../extra/guile/example.scm
   :language: scheme
   :start-after: intro/start
   :end-before:  intro/end
..

    **Do you want to help** making these bindings complete and production
    ready?  Drop a line at `immer@sinusoid.al
    <mailto:immer@sinusoid.al>`_ or `open an issue on Github
    <https://github.com/arximboldi/immer>`_

.. _GNU Guile: https://www.gnu.org/software/guile/

Installation
------------

.. highlight:: sh

To install the software, you need `GNU Guile 2.2
<https://www.gnu.org/software/guile/download/>`_.  Then you have to
`clone the repository <https://github.com/arximboldi/immer>`_ and
inside the repository do something like::

    mkdir build; cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DGUILE_EXTENSION_DIR="<somewhere...>"
    make guile
    cp extra/guile/libguile-immer.so "<...the GUILE_EXTENSION_DIR>"
    cp extra/guile/immer.scm "<somewhere in your GUILE_LOAD_PATH>"

Benchmarks
----------

The library includes some quick and dirty benchmarks that show how
these vectors perform compared to *mutable vectors*, *lists*, and
*v-lists*.  Once you have installed the library, you may run them by
executing the following in the project root::

    guile extra/guile/benchmark.scm

This is the output I get when running those:

.. code-block:: scheme
   :name: benchmark-output

   (define bench-size 1000000)
   (define bench-samples 10)
   ;;;; benchmarking creation...
   ; evaluating:
         (apply ivector (iota bench-size))
   ; average time: 0.0608697784 seconds
   ; evaluating:
         (apply ivector-u32 (iota bench-size))
   ; average time: 0.0567354933 seconds
   ; evaluating:
         (iota bench-size)
   ; average time: 0.032995402 seconds
   ; evaluating:
         (apply vector (iota bench-size))
   ; average time: 0.0513594425 seconds
   ; evaluating:
         (apply u32vector (iota bench-size))
   ; average time: 0.0939185315 seconds
   ; evaluating:
         (list->vlist (iota bench-size))
   ; average time: 0.2369570977 seconds
   ;;;; benchmarking iteration...
   (define bench-ivector (apply ivector (iota bench-size)))
   (define bench-ivector-u32 (apply ivector-u32 (iota bench-size)))
   (define bench-list (iota bench-size))
   (define bench-vector (apply vector (iota bench-size)))
   (define bench-u32vector (apply u32vector (iota bench-size)))
   (define bench-vlist (list->vlist (iota bench-size)))
   ; evaluating:
         (ivector-fold + 0 bench-ivector)
   ; average time: 0.035750341 seconds
   ; evaluating:
         (ivector-u32-fold + 0 bench-ivector-u32)
   ; average time: 0.0363843682 seconds
   ; evaluating:
         (fold + 0 bench-list)
   ; average time: 0.0271881423 seconds
   ; evaluating:
         (vector-fold + 0 bench-vector)
   ; average time: 0.0405022349 seconds
   ; evaluating:
         (vlist-fold + 0 bench-vlist)
   ; average time: 0.0424709098 seconds
   ;;;; benchmarking iteration by index...
   ; evaluating:
         (let iter ((i 0) (acc 0))
           (if (< i (ivector-length bench-ivector))
             (iter (+ i 1) (+ acc (ivector-ref bench-ivector i)))
             acc))
   ; average time: 0.2195658936 seconds
   ; evaluating:
         (let iter ((i 0) (acc 0))
           (if (< i (ivector-u32-length bench-ivector-u32))
             (iter (+ i 1) (+ acc (ivector-u32-ref bench-ivector-u32 i)))
             acc))
   ; average time: 0.2205486326 seconds
   ; evaluating:
         (let iter ((i 0) (acc 0))
           (if (< i (vector-length bench-vector))
             (iter (+ i 1) (+ acc (vector-ref bench-vector i)))
             acc))
   ; average time: 0.0097157637 seconds
   ; evaluating:
         (let iter ((i 0) (acc 0))
           (if (< i (u32vector-length bench-u32vector))
             (iter (+ i 1) (+ acc (u32vector-ref bench-u32vector i)))
             acc))
   ; average time: 0.0733736008 seconds
   ; evaluating:
         (let iter ((i 0) (acc 0))
           (if (< i (vlist-length bench-vlist))
             (iter (+ i 1) (+ acc (vlist-ref bench-vlist i)))
             acc))
   ; average time: 0.3220357243 seconds
   ;;;; benchmarking concatenation...
   ; evaluating:
         (ivector-append bench-ivector bench-ivector)
   ; average time: 1.63022e-5 seconds
   ; evaluating:
         (ivector-u32-append bench-ivector-u32 bench-ivector-u32)
   ; average time: 1.63754e-5 seconds
   ; evaluating:
         (append bench-list bench-list)
   ; average time: 0.0135592963 seconds
   ; evaluating:
         (vector-append bench-vector bench-vector)
   ; average time: 0.0044506586 seconds
   ; evaluating:
         (vlist-append bench-vlist bench-vlist)
   ; average time: 0.3227312512 seconds
