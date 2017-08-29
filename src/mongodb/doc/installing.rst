:man_page: mongoc_installing

Installing the MongoDB C Driver
===============================

The following guide will step you through the process of downloading, building, and installing the current release of the MongoDB C Driver.

Supported Platforms
-------------------

The MongoDB C Driver is `continuously tested <https://evergreen.mongodb.com/waterfall/mongo-c-driver>`_ on variety of platforms including:

- Archlinux
- Debian 8.1
- macOS 10.10
- Microsoft Windows Server 2008
- RHEL 7.0, 7.1, 7.2
- SUSE 12
- smartOS (sunos / Solaris)
- Ubuntu 12.04, 14.04, 16.04
- Clang 3.4, 3.5, 3.7, 3.8
- GCC 4.6, 4.8, 4.9, 5.3
- MinGW-W64
- Visual Studio 2010, 2013, 2015
- x86, x86_64, ARM (aarch64), Power8 (ppc64le), zSeries (s390x)


Install with a Package Manager
------------------------------

The libmongoc package is available on recent versions of Debian and Ubuntu.

.. code-block:: none

  $ apt-get install libmongoc-1.0-0

On Fedora, a mongo-c-driver package is available in the default repositories and can be installed with:

.. code-block:: none

  $ dnf install mongo-c-driver

On recent Red Hat systems, such as CentOS and RHEL 7, a mongo-c-driver package is available in the `EPEL <https://fedoraproject.org/wiki/EPEL>`_ repository. To check version available, see `https://apps.fedoraproject.org/packages/mongo-c-driver <https://apps.fedoraproject.org/packages/mongo-c-driver>`_. The package can be installed with:

.. code-block:: none

  $ yum install mongo-c-driver

Building on Unix
----------------

Prerequisites
^^^^^^^^^^^^^

OpenSSL is required for authentication or for SSL connections to MongoDB. Kerberos or LDAP support requires Cyrus SASL.

To install all optional dependencies on RedHat / Fedora:

.. code-block:: none

  $ sudo yum install pkg-config openssl-devel cyrus-sasl-devel

On Debian / Ubuntu:

.. code-block:: none

  $ sudo apt-get install pkg-config libssl-dev libsasl2-dev

On FreeBSD:

.. code-block:: none

  $ su -c 'pkg install pkgconf openssl cyrus-sasl'

Building from a release tarball
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Unless you intend on contributing to the mongo-c-driver, you will want to build from a release tarball.

The most recent release of libmongoc is |release| and can be :release:`downloaded here <>`. The following snippet will download and extract the driver, and configure it:

.. parsed-literal::

  $ wget |release_download|
  $ tar xzf mongo-c-driver-|release|.tar.gz
  $ cd mongo-c-driver-|release|
  $ ./configure --disable-automatic-init-and-cleanup

The ``--disable-automatic-init-and-cleanup`` option is recommended, see :doc:`init-cleanup`. For a list of all configure options, run ``./configure --help``.

If ``configure`` completed successfully, you'll see something like the following describing your build configuration.

.. parsed-literal::

  libmongoc |release| was configured with the following options:

  Build configuration:
    Enable debugging (slow)                          : no
    Compile with debug symbols (slow)                : no
    Enable GCC build optimization                    : yes
    Enable automatic init and cleanup                : no
    Enable maintainer flags                          : no
    Code coverage support                            : no
    Cross Compiling                                  : no
    Fast counters                                    : no
    Shared memory performance counters               : yes
    SASL                                             : sasl2
    SSL                                              : openssl
    Snappy Compression                               : bundled
    Zlib Compression                                 : bundled
    Libbson                                          : bundled

  Documentation:
    man                                              : no
    HTML                                             : no

mongo-c-driver contains a copy of libbson, in case your system does not already have libbson installed. The configure script will detect if libbson is not installed and use the bundled libbson.

.. code-block:: none

  $ make
  $ sudo make install
      

Building from git
^^^^^^^^^^^^^^^^^

To build an unreleased version of the driver from git requires additional dependencies.

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

  $ git clone https://github.com/mongodb/mongo-c-driver.git
  $ cd mongo-c-driver
  $ git checkout x.y.z  # To build a particular release
  $ ./autogen.sh --with-libbson=bundled
  $ make
  $ sudo make install
      

Generating the documentation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Install `Sphinx <http://www.sphinx-doc.org/>`_, then:

.. code-block:: none

  $ ./configure --enable-html-docs --enable-man-pages
  $ make man html

Building on Mac OS X
--------------------

Install the XCode Command Line Tools::

  $ xcode-select --install

The ``pkg-config`` utility is also required. First `install Homebrew according to its instructions <http://brew.sh/>`_, then::

  $ brew install pkgconfig

Download the latest release tarball:

.. parsed-literal::

  $ curl -LO |release_download|
  $ tar xzf mongo-c-driver-|release|.tar.gz
  $ cd mongo-c-driver-|release|

Build and install the driver:

.. code-block:: none

  $ ./configure
  $ make
  $ sudo make install

Native TLS Support on Mac OS X / Darwin (Secure Transport)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The MongoDB C Driver supports the Darwin native TLS and crypto libraries.
Using the native libraries there is no need to install OpenSSL. By
default however, the driver will compile against OpenSSL if it
detects it being available. If OpenSSL is not available, it will
fallback on the native libraries.

To compile against the Darwin native TLS and crypto libraries, even when
OpenSSL is available, configure the driver like so:

.. code-block:: none

  $ ./configure --enable-ssl=darwin

OpenSSL support on El Capitan
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Beginning in OS X 10.11 El Capitan, OS X no longer includes the OpenSSL headers. To build the driver with SSL on El Capitan and later:

.. code-block:: none

  $ brew install openssl
  $ export LDFLAGS="-L/usr/local/opt/openssl/lib"
  $ export CPPFLAGS="-I/usr/local/opt/openssl/include"

.. _build-on-windows:

Building on Windows
-------------------

Building on Windows requires Windows Vista or newer and Visual Studio 2010 or newer. Additionally, ``cmake`` is required to generate Visual Studio project files.

Let's start by generating Visual Studio project files for libbson, a dependency of the C driver. The following assumes we are compiling for 64-bit Windows using Visual Studio 2015 Express, which can be freely downloaded from Microsoft.

.. parsed-literal::

  cd mongo-c-driver-|release|\\src\\libbson
  cmake -G "Visual Studio 14 2015 Win64" \\
    "-DCMAKE_INSTALL_PREFIX=C:\\mongo-c-driver" \\
    "-DCMAKE_BUILD_TYPE=Release" # Defaults to debug builds

(Run ``cmake -LH .`` for a list of other options.)

Now that we have project files generated, we can either open the project in Visual Studio or compile from the command line. Let's build using the command line program ``msbuild.exe``

.. code-block:: none

  msbuild.exe /p:Configuration=Release ALL_BUILD.vcxproj

Now that libbson is compiled, let's install it using msbuild. It will be installed to the path specified by ``CMAKE_INSTALL_PREFIX``.

.. code-block:: none

  msbuild.exe /p:Configuration=Release INSTALL.vcxproj

You should now see libbson installed in ``C:\mongo-c-driver``

Now let's do the same for the MongoDB C driver.

.. parsed-literal::

  cd mongo-c-driver-|release|
  cmake -G "Visual Studio 14 2015 Win64" \\
    "-DENABLE_SSL=WINDOWS" \\
    "-DENABLE_SASL=SSPI" \\
    "-DCMAKE_INSTALL_PREFIX=C:\\mongo-c-driver" \\
    "-DCMAKE_PREFIX_PATH=C:\\mongo-c-driver" \\
    "-DCMAKE_BUILD_TYPE=Release" # Defaults to debug builds

  msbuild.exe /p:Configuration=Release ALL_BUILD.vcxproj
  msbuild.exe /p:Configuration=Release INSTALL.vcxproj

All of the MongoDB C Driver's components will now have been build in release
mode and can be found in ``C:\mongo-c-driver``.
To build and install debug binaries, remove the
``"-DCMAKE_BUILD_TYPE=Release"`` argument to ``cmake`` and
``/p:Configuration=Release`` to ``msbuild.exe``.

To use the driver libraries in your program, see :doc:`visual-studio-guide`.

Native TLS Support on Windows (Secure Channel)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The MongoDB C Driver supports the Windows native TLS and crypto libraries.
Using the native libraries there is no need to install OpenSSL. By
default however, the driver will compile against OpenSSL if it
detects it being available. If OpenSSL is not available, it will
fallback on the native libraries.


To compile against the Windows native TLS and crypto libraries, even when
OpenSSL is available, configure the driver like so:

.. code-block:: none

  cmake -G "Visual Studio 14 2015 Win64" \
    "-DENABLE_SSL=WINDOWS" \
    "-DCMAKE_INSTALL_PREFIX=C:\\mongo-c-driver" \
    "-DCMAKE_PREFIX_PATH=C:\\mongo-c-driver"


Native SASL Support on Windows (SSPI)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The MongoDB C Driver supports the Windows native Kerberos and Active Directory
interface, SSPI. Using the native libraries there is no need to install any
dependencies, such as cyrus-sasl.  By default however, the driver will compile
against cyrus-sasl.


To compile against the Windows native SSPI, configure the driver like so:

.. code-block:: none

  cmake -G "Visual Studio 14 2015 Win64" \
    "-DENABLE_SASL=SSPI" \
    "-DCMAKE_INSTALL_PREFIX=C:\\mongo-c-driver" \
    "-DCMAKE_PREFIX_PATH=C:\\mongo-c-driver"

OpenSSL support on Windows
^^^^^^^^^^^^^^^^^^^^^^^^^^

For backwards compatibility CMake will default to OpenSSL support.
If not found, it will fallback to native TLS support provided by the platform.

OpenSSL 1.1.0 support requires CMake 3.7 or later on Windows.

