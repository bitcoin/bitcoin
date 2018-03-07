Automated Gitian builds with Vagrant
====================================

This directory contains platform-independent scripts for building
bitcoind and Bitcoin-Qt using the deterministic Gitian build process.

Dependencies
------------

These build scripts depend on a UNIX-like build environment, the
freely available open-source release of Oracle VirtualBox, and
HashiCorp's vagrant and packer VM-creation utilities. The makefile
will complain if any of the required tools are not found. Use the
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

Install the latest versions of VirtualBox and Vagrant. The scripts are
known to work with VirtualBox 5.2.8 r121009, Vagrant 2.0.1, and Packer
1.1.3 on Mac OS X 10.13.3 High Sierra with XCode 9.2 (9C40b) and the
command-line developer tools installed.

VirtualBox binaries are available from the VirtualBox website:

    https://www.virtualbox.org/wiki/Downloads

The Vagrant installer for Mac OS X works:

    https://www.vagrantup.com/downloads.html

Be sure to install the VirtualBox plugin:

    $ vagrant plugin install vagrant-vbguest

And the Packer binary needs to be placed in your path:

    https://www.packer.io

Use MacPorts or homebrew to install any missing dependencies, for example:

    $ sudo port install openssl wget xz

Then use GNU make to initiate the build:

    $ cd contrib/vagrant
    $ make

Linux
-----

Existing binaries for VirtualBox, Vagrant, Packer, git, and the
various UNIX dependencies provided by your distribution should
work. Use the makefile to find out which requirements are missing:

    $ cd contrib/vagrant
    $ make check-requirements

If you cannot find `vagrant` in your distribution's package
repositories, you can install a binary release from here:

    https://www.vagrantup.com/downloads.html

Be sure to install the VirtualBox plugin:

    $ vagrant plugin install vagrant-vbguest

And for Packer:

    https://www.packer.io/downloads.html

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

Install the VirtualBox plugin for vagrant:

    $ vagrant plugin install vagrant-vbguest

Install mingw + msys environment:

    http://www.mingw.org/wiki/Getting_Started

Install Vagrant via the installer:

    https://www.vagrantup.com/downloads.html

Install Packer into your path:

    https://www.packer.io/downloads.html

Use mingw-get to install other required packages.

    $ mingw-get install wget

Once the dependencies are met, use GNU make to initiate the build:

    $ cd contrib/vagrant
    $ make
