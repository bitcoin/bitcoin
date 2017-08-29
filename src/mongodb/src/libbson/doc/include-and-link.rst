:man_page: bson_include_and_link

Using libbson In Your C Program
===============================

Include bson.h
--------------

All libbson's functions and types are available in one header file. Simply include ``bson.h``:

.. literalinclude:: ../examples/hello_bson.c
  :caption: hello_bson.c
  :start-after: -- sphinx-include-start --

CMake
-----

The libbson installation includes a `CMake config-file package`_, so you can use CMake's `find_package`_ command to find libbson's header and library paths and link to libbson:

.. literalinclude:: ../examples/cmake/find_package/CMakeLists.txt
  :caption: CMakeLists.txt
  :start-after: -- sphinx-include-start --

By default, libbson is dynamically linked. You can use libbson as a static library instead: Use the included ``libbson-static-1.0`` config-file package and (on Unix) link to ``pthread``:

.. literalinclude:: ../examples/cmake/find_package_static/CMakeLists.txt
  :start-after: -- sphinx-include-start --
  :emphasize-lines: 1, 6

.. _CMake config-file package: https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html#config-file-packages
.. _find_package: https://cmake.org/cmake/help/latest/command/find_package.html

pkg-config
----------

If you're not using CMake, use `pkg-config`_ on the command line to set header and library paths:

.. literalinclude:: ../examples/compile-with-pkg-config.sh
  :start-after: -- sphinx-include-start --

Or to statically link to libbson:

.. literalinclude:: ../examples/compile-with-pkg-config-static.sh
  :start-after: -- sphinx-include-start --

.. _pkg-config: https://www.freedesktop.org/wiki/Software/pkg-config/
