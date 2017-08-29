Using libmongoc in a Microsoft Visual Studio project
====================================================

:ref:`Download and install libmongoc on your system <build-on-windows>`, then open Visual Studio, select "File |rarrow| New |rarrow| Project...", and create a new Win32 Console Application.

.. image::
  static/msvc-create-project.png

Remember to switch the platform from 32-bit to 64-bit:

.. image::
  static/msvc-switch-architecture.png

Right-click on your console application in the Solution Explorer and select "Properties". Choose to edit properties for "All Configurations", expand the "C/C++" options and choose "General". Add to the "Additional Include Directories" these paths:

.. code-block:: text

  C:\mongo-c-driver\include\libbson-1.0
  C:\mongo-c-driver\include\libmongoc-1.0

.. image::
  static/msvc-add-include-directories.png

(If you chose a different ``CMAKE_INSTALL_PREFIX`` :ref:`when you ran CMake <build-on-windows>`, your include paths will be different.)

Also in the Properties dialog, expand the "Linker" options and choose "Input", and add to the "Additional Dependencies" these libraries:

.. code-block:: text

  C:\mongo-c-driver\lib\bson-1.0.lib
  C:\mongo-c-driver\lib\mongoc-1.0.lib

.. image::
  static/msvc-add-dependencies.png

Adding these libraries as dependencies provides linker symbols to build your application, but to actually run it, libbson's and libmongoc's DLLs must be in your executable path. Select "Debugging" in the Properties dialog, and set the "Environment" option to:

.. code-block:: text

  PATH=c:/mongo-c-driver/bin

.. image::
  static/msvc-set-path.png

Finally, include "mongoc.h" in your project's "stdafx.h":

.. code-block:: c

  #include <mongoc.h>

Static linking
--------------

Following the instructions above, you have dynamically linked your application to the libbson and libmongoc DLLs. This is usually the right choice. If you want to link statically instead, update your "Additional Dependencies" list by removing ``bson-1.0.lib`` and ``mongoc-1.0.lib`` and replacing them with these libraries:

.. code-block:: text

  C:\mongo-c-driver\lib\bson-static-1.0.lib
  C:\mongo-c-driver\lib\mongoc-static-1.0.lib
  ws2_32.lib
  Secur32.lib
  Crypt32.lib
  BCrypt.lib

.. image::
  static/msvc-add-dependencies-static.png

(To explain the purpose of each library: ``bson-static-1.0.lib`` and ``mongoc-static-1.0.lib`` are static archives of the driver code. The socket library ``ws2_32`` is required by libbson, which uses the socket routine ``gethostname`` to help guarantee ObjectId uniqueness. The ``BCrypt`` library is used by libmongoc for SSL connections to MongoDB, and ``Secur32`` and ``Crypt32`` are required for enterprise authentication methods like Kerberos.)

Finally, define two preprocessor symbols before including ``mongoc.h`` in your ``stdafx.h``:

.. code-block:: c

  #define BSON_STATIC
  #define MONGOC_STATIC
  #include <mongoc.h>

Making these changes to your project is only required for static linking; for most people, the dynamic-linking instructions above are preferred.

Next Steps
----------

Now you can build and debug applications in Visual Studio that use libbson and libmongoc. Proceed to :ref:`making-a-connection` in the tutorial to learn how connect to MongoDB and perform operations.

.. turn "rarrow" above into right-arrow with no spaces around it

.. |rarrow| unicode:: U+2192
  :trim:
