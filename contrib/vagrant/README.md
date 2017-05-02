Automated Gitian builds with Vagrant
====================================

This directory contains platform-independent scripts for building bitcoind and
Bitcoin-QT using the deterministic Gitian build process.

Dependencies
------------

These build scripts depend on a UNIX-like build environment, the freely
available open-source release of VirtualBox, and the Vagrant ruby gem. The
makefile will complain if any of the required tools are not found. Use the
makefile to find out which requirements are missing:

    $ cd contrib/vagrant
    $ make check-requirements

Instructions
------------

    $ cd contrib/vagrant && make

It really is that simple.

Read the remainder of this document for some platform-specific instructions
for setting up an appropriate build environment.

Mac OS X
--------

Install the latest versions of VirtualBox and Vagrant. The scripts are known
to work with VirtualBox 4.1.16 r78094 and Vagrant 1.0.3 on Mac OS X 10.7 Lion
with XCode 4.3.2 and the command-line developer tools installed.

VirtualBox binaries are available from the VirtualBox website:

    https://www.virtualbox.org/wiki/Downloads

The Vagrant installer for Mac OS X works:

    http://vagrantup.com/

Use MacPorts or homebrew to install any missing dependencies, for example:

    $ sudo port install openssl wget xz

Then use GNU make to initiate the build:

    $ cd contrib/vagrant
    $ make

Linux
-----

Existing binaries for VirtualBox, Vagrant, git, and the various UNIX
dependencies provided by your distribution should work. Use the makefile to
find out which requirements are missing:

    $ cd contrib/vagrant
    $ make check-requirements

If you cannot find `vagrant` in your distribution's package repositories, you
can install it using ruby gems:

    $ sudo gem install vagrant

Once the dependencies are met, use GNU make to initiate the build:

    $ cd contrib/vagrant
    $ make

Windows
-------

A UNIX-like build environment is required, but due to the peculiarities of the
required dependencies, the exact combination you have installed probably won't
work. Here are build instructions that are known to work from an updated fresh
install of Windows 7:

Install Git for Windows (if you need git):

    http://msysgit.github.com/

You don't need the msys version--we will be installing msys separately.

Install ruby using the one-click ruby installer:

    http://rubyinstaller.org/downloads/

Install ruby development environment, from the same download page.
Instructions:

    https://github.com/oneclick/rubyinstaller/wiki/Development-Kit

Install Virtualbox (extension pack not required):

    https://www.virtualbox.org/wiki/Downloads

Install mingw + msys environment:

    http://www.mingw.org/wiki/Getting_Started

Install Vagrant via gem:

    $ gem install vagrant

Use mingw-get to install other required packages.

    $ mingw-get install wget

Once the dependencies are met, use GNU make to initiate the build:

    $ cd contrib/vagrant
    $ make
