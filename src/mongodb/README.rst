==============
mongo-c-driver
==============

About
=====

mongo-c-driver is a client library written in C for MongoDB.

mongo-c-driver depends on `Libbson <https://github.com/mongodb/libbson>`_.
Libbson will automatically be built if you do not have it installed on your system.

Documentation / Support / Feedback
==================================

The documentation is available at http://mongoc.org/.
For issues with, questions about, or feedback for libmongoc, please look into
our `support channels <http://www.mongodb.org/about/support>`_. Please
do not email any of the libmongoc developers directly with issues or
questions - you're more likely to get an answer on the `mongodb-user list`_
on Google Groups.

Bugs / Feature Requests
=======================

Think you’ve found a bug? Want to see a new feature in libmongoc? Please open a
case in our issue management tool, JIRA:

- `Create an account and login <https://jira.mongodb.org>`_.
- Navigate to `the CDRIVER project <https://jira.mongodb.org/browse/CDRIVER>`_.
- Click **Create Issue** - Please provide as much information as possible about the issue type and how to reproduce it.

Bug reports in JIRA for all driver projects (i.e. CDRIVER, CSHARP, JAVA) and the
Core Server (i.e. SERVER) project are **public**.

How To Ask For Help
-------------------

If you are having difficulty building the driver after reading the below instructions, please email
the `mongodb-user list`_ to ask for help. Please include in your email all of the following
information:

- The version of the driver you are trying to build (branch or tag).
    - Examples: master branch, 1.2.1 tag
- Host OS, version, and architecture.
    - Examples: Windows 8 64-bit x86, Ubuntu 12.04 32-bit x86, OS X Mavericks
- C Compiler and version.
    - Examples: GCC 4.8.2, MSVC 2013 Express, clang 3.4, XCode 5
- The output of ``./autogen.sh`` or ``./configure`` (depending on whether you are building from a
  repository checkout or from a tarball). The output starting from "libbson was configured with
  the following options" is sufficient.
- The text of the error you encountered.

Failure to include the relevant information will result in additional round-trip
communications to ascertain the necessary details, delaying a useful response.
Here is a made-up example of a help request that provides the relevant
information:

  Hello, I'm trying to build the C driver with SSL, from mongo-c-driver-1.2.1.tar.gz. I'm on Ubuntu
  14.04, 64-bit Intel, with gcc 4.8.2. I run configure like::

    $ ./configure --enable-sasl=yes
    checking for gcc... gcc
    checking whether the C compiler works... yes

    ... SNIPPED OUTPUT, but when you ask for help, include full output without any omissions ...

    checking for pkg-config... no
    checking for SASL... no
    checking for sasl_client_init in -lsasl2... no
    checking for sasl_client_init in -lsasl... no
    configure: error: You must install the Cyrus SASL libraries and development headers to enable SASL support.

  Can you tell me what I need to install? Thanks!

.. _mongodb-user list: http://groups.google.com/group/mongodb-user

Security Vulnerabilities
------------------------

If you’ve identified a security vulnerability in a driver or any other
MongoDB project, please report it according to the `instructions here
<http://docs.mongodb.org/manual/tutorial/create-a-vulnerability-report>`_.


Installation
============

Detailed installation instructions are in the manual:
http://mongoc.org/libmongoc/current/installing.html
