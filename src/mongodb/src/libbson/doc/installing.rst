:man_page: bson_installing

Installing libbson
==================

The following guide will step you through the process of downloading, building, and installing the current release of libbson.

.. _installing_supported_platforms:

Supported Platforms
-------------------

The MongoDB C Driver is `continuously tested <https://evergreen.mongodb.com/waterfall/libbson>`_ on variety of platforms including:

- Archlinux
- Debian 8.1
- macOS 10.10
- Microsoft Windows Server 2008
- RHEL 5.5, 6.2, 7.0, 7.1, 7.2
- smartOS (sunos / Solaris)
- Ubuntu 12.04, 16.04
- Clang 3.5, 3.7, 3.8
- GCC 4.6, 4.8, 4.9, 5.3
- MinGW-W64
- Visual Studio 2010, 2013, 2015
- x86, x86_64, ARM (aarch64), Power8 (ppc64le), zSeries (s390x)


Install with a Package Manager
------------------------------

The libbson package is available on recent versions of Debian and Ubuntu.

.. code-block:: none

  $ apt-get install libbson-1.0

On Fedora, a libbson package is available in the default repositories and can be installed with:

.. code-block:: none

  $ dnf install libbson

On recent Red Hat systems, such as CentOS and RHEL 7, a libbson package
is available in the `EPEL <https://fedoraproject.org/wiki/EPEL>`_ repository. To check
version available, see `https://apps.fedoraproject.org/packages/libbson <https://apps.fedoraproject.org/packages/libbson>`_.
The package can be installed with:

.. code-block:: none

  $ yum install libbson

Building on Unix
----------------

Building from a release tarball
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Unless you intend on contributing to libbson, you will want to build from a release tarball.

The most recent release of libbson is |release| and can be :release:`downloaded here <>`. The following snippet will download and extract the current release of the driver.

.. parsed-literal::

  $ wget |release_download|
  $ tar -xzf libbson-|release|.tar.gz
  $ cd libbson-|release|/
  $ ./configure

For a list of all configure options, run ``./configure --help``.

If ``configure`` completed successfully, you'll see something like the following describing your build configuration.

.. parsed-literal::

  libbson |release| was configured with the following options:

  Build configuration:
    Enable debugging (slow)                          : no
    Enable extra alignment (required for 1.0 ABI)    : no
    Compile with debug symbols (slow)                : no
    Enable GCC build optimization                    : yes
    Code coverage support                            : no
    Cross Compiling                                  : no
    Big endian                                       : no
    Link Time Optimization (experimental)            : no

  Documentation:
    man                                              : no
    HTML                                             : no

We can now build libbson with the venerable ``make`` program.

.. code-block:: none

  $ make
  $ sudo make install

Building from git
^^^^^^^^^^^^^^^^^

To build an unreleased version of libbson from git requires additional dependencies.

RedHat / Fedora:

.. code-block:: none

  $ sudo yum install git gcc automake autoconf libtool

Debian / Ubuntu:

.. code-block:: none

  $ sudo apt-get install git gcc automake autoconf libtool

FreeBSD:

.. code-block:: none

  $ su -c 'pkg install git gcc automake autoconf libtool'

Once you have the dependencies installed, clone the repository and build the current master or a particular release tag:

.. code-block:: none

  $ git clone https://github.com/mongodb/libbson.git
  $ cd libbson
  $ git checkout x.y.z  # To build a particular release
  $ ./autogen.sh
  $ make
  $ sudo make install

Generating the documentation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Install `Sphinx <http://www.sphinx-doc.org/>`_, then:

.. code-block:: none

  $ ./configure --enable-html-docs --enable-man-pages
  $ make man html

.. _installing_building_on_windows:

Building on Mac OS X
--------------------

Install the XCode Command Line Tools::

  $ xcode-select --install

The ``pkg-config`` utility is also required. First `install Homebrew according to its instructions <https://brew.sh/>`_, then::

  $ brew install pkgconfig

Download the latest release tarball

.. parsed-literal::

  $ curl -LO |release_download|
  $ tar xzf libbson-|release|.tar.gz
  $ cd libbson-|release|

Build and install libbson:

.. code-block:: none

  $ ./configure
  $ make
  $ sudo make install

Building on Windows
-------------------

Building on Windows requires Windows Vista or newer and Visual Studio 2010 or newer. Additionally, ``cmake`` is required to generate Visual Studio project files.

Let's start by generating Visual Studio project files for libbson. The following assumes we are compiling for 64-bit Windows using Visual Studio 2010 Express which can be freely downloaded from Microsoft.

.. parsed-literal::

  > cd libbson-|release|
  > cmake -G "Visual Studio 14 2015 Win64" \\
    "-DCMAKE_INSTALL_PREFIX=C:\\libbson"
  > msbuild.exe ALL_BUILD.vcxproj
  > msbuild.exe INSTALL.vcxproj

You should now see libbson installed in ``C:\libbson``.
By default, this will create a debug build of libbson. To enable release build additional argument needs to be provided to both cmake and msbuild.exe:

.. parsed-literal::

  > cd libbson-|release|
  > cmake -G "Visual Studio 14 2015 Win64" \\
    "-DCMAKE_INSTALL_PREFIX=C:\\libbson" \\
    "-DCMAKE_BUILD_TYPE=Release"
  > msbuild.exe /p:Configuration=Release ALL_BUILD.vcxproj
  > msbuild.exe /p:Configuration=Release INSTALL.vcxproj

You can disable building the tests with:

.. code-block:: none

  > cmake -G "Visual Studio 14 2015 Win64" \
    "-DCMAKE_INSTALL_PREFIX=C:\libbson" \
    "-DENABLE_TESTS:BOOL=OFF"
