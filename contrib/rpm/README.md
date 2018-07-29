RPM Spec File Notes
-------------------

The RPM spec file provided here is for Bitcoin-Core 0.12.0 and builds on CentOS
7 with either the CentOS provided OpenSSL library or with LibreSSL as packaged
at [LibreLAMP.com](https://librelamp.com/). It should hopefully not be too
difficult to port the RPM spec file to most RPM based Linux distributions.

When porting the spec file to build for a particular distribution, there are
some important notes.

## Sources

It is considered good form for all sources to reference a URL where the source
can be downloaded.

Sources 0-9 should be reserved for source code tarballs. `Source0` should
reference the release tarball available from https://bitcoin.org/bin/ and
`Source1` should reference the BerkeleyDB source.

Sources 10-99 are for source files that are maintained in the
[Bitcoin git repository](https://github.com/bitcoin/bitcoin) but are not part of
the release tarball. Most of these will reside in the `contrib` sub-directory.

Sources 10-19 should be reserved for miscellaneous configuration files.
Currently only `Source10` is used, for the example `bitcoin.conf` file.

Sources 20-29 should be reserved for man pages. Currently only `Source20`
through `Source23` are used.

Sources 30-39 should be reserved for SELinux related files. Currently only
`Source30` through `Source32` are used. Until those files are in a tagged
release, the full URL specified in the RPM spec file will not work. You can get
them from the git ropository where you retrieved this file.

Sources 100+ are for files that are not source tarballs and are not maintained
in the bitcoin git repository. At present only an SVG version of the Bitcoin
icon is used.

## Patches

In general, patches should be avoided. When a packager feels a patch is
necessary, the packager should bring the problem to the attention of the bitcoin
developers so that an official fix to the issue can make it into the next
release.

### Patch0 bitcoin-0.12.0-libressl.patch

This patch is only needed if building against LibreSSL. LibreSSL is not the
standard TLS library on most Linux distributions. The patch will likely not be
needed when 0.12.1 is released, a proper fix is already in the Bitcoin git
master branch.

## BuildRequires

The packages specified in the `BuildRequires` are specified according to the
package naming convention currently used in CentOS 7 and EPEL for CentOS 7. You
may need to change some of the package names for other distributions. This is
most likely to be the case with the Qt packages.

## BerkeleyDB

The `build-unix.md` file recommends building against BerkeleyDB 4.8.30. Even if
that is the version your Linux distribution ships with, it probably is a good
idea to build Bitcoin Core against a static version of that library compiled
according to the instructions in the `build-unix.md` file so that any changes
the distribution may make in the future will not result in a problem for users.

The problem that can exist, clients built against different versions of
BerkeleyDB may not be able read each other's `wallet.dat` file which can make it
difficult for a user to recover from backup in the event of a system failure.

## Graphical User Interface and Qt Version

The RPM spec file will by default build the GUI client linked against the Qt5
libraries. If you wish instead to link against the Qt4 libraries you need to
pass the switch `-D '_use_qt4 1'` at build time to the `rpmbuild` or `mock`
command used to build the packages.

If you would prefer not to build the GUI at all, you can pass the switch
`-D '_no_gui 1'` to the `rpmbuild` or `mock` build command.

## Desktop and KDE Files

The desktop and KDE meta files are created in the spec file itself with the
`cat` command. This is done to allow easy distribution specific changes without
needing to use any patches. A specific time stamp is given to the files so that
it does not they do not appear to have been updated every time the package is
built. If you do make changes to them, you probably should update time stamp
assigned to them in the `touch` command that specifies the time stamp.

## SVG, PNG, and XPM Icons

The `bitcoin.svg` file is from the source listed as `Source100`. It is used as
the source for the PNG and XPM files. The generated PNG and XPM files are given
the same time stamp as the source SVG file as a means of indicating they are
derived from it.

## Systemd

This spec file assumes the target distribution uses systemd. That really only
matters for the `bitcoin-server` package. At this point, most RPM based
distributions that still receive vendor updates do in fact use systemd.

The files to control the service are created in the RPM spec file itself using
the `cat` command. This is done to make it easy to modify for other
distributions that may implement things differently without needing to patch
source. A specific time stamp is given to the files so that they do not appear
to have been updated every time the package is built. If you do make changes to
them, you probably should update the time stamp assigned to them in the `touch`
command that specifies the time stamp.

## SELinux

The `bitcoin-server` package should have SELinux support. How to properly do
that *may* vary by distribution and version of distribution.

The SELinux stuff in this RPM spec file *should* be correct for CentOS, RHEL,
and Fedora but it would be a good idea to review it before building the package
on other distributions.

## Tests

The `%check` section takes a very long time to run. If your build system has a
time limit for package build, you may need to make an exception for this
package. On CentOS 7 the `%check` section completes successfully with both
OpenSSL and LibreSSL, a failure really does mean something is wrong.

## LibreSSL Build Notes

To build against LibreSSL you will need to pass the switch
`-D '_use_libressl 1'` to the `rpmbuild` or `mock` command or the spec file will
want the OpenSSL development files.

### LibreSSL and Boost

LibreSSL (and some newer builds of OpenSSL) do not have support for SSLv3. This
can cause issues with the Boost package if the Boost package has not been
patched accordingly. On those distributions, you will either need to build
Bitcoin-Core against OpenSSL or use a patched version of Boost in the build
system.

As SSLv3 is no longer safe, distributions that have not patched Boost to work
with TLS libraries that do not support SSLv3 should have bug reports filed
against the Boost package. This bug report has already been filed for RHEL 7 but
it may need to be filed for other distributions.

A patch for Boost: https://github.com/boostorg/asio/pull/23/files

## ZeroMQ

At this time, this RPM spec file does not support the ZeroMQ build options. A
suitable version of ZeroMQ is not available for the platform this spec file was
developed on (CentOS 7).

## Legacy Credit

This RPM spec file is largely based upon the work of Michael Hampton at
[Ringing Liberty](https://www.ringingliberty.com/bitcoin/). He has been
packaging Bitcoin for Fedora at least since 2012.

Most of the differences between his packaging and this package are stylistic in
nature. The major differences:

1. He builds from a github tagged release rather than a release tarball. This
should not result in different source code.

2. He does not build BerkeleyDB but instead uses the BerkeleyDB provided by the
Linux distribution. For the distributions he packages for, they currently all
use the same version of BerkeleyDB so that difference is *probably* just
academic.

3. As of his 10.11.2 package he did not allow for building against LibreSSL,
specifying a build without the Qt GUI, or specifying which version of the Qt
libraries to use.

4. I renamed the `bitcoin` package that contains the Qt GUI to `bitcoin-core` as
that appears to be how the general population refers to it, in contrast to
`bitcoin-xt` or `bitcoin-classic`. I wanted to make sure the general population
knows what they are getting when installing the GUI package.

As far as minor differences, I generally prefer to assign the file permissions
in the `%files` portion of an RPM spec file rather than specifying the
permissions of a file during `%install` and other minor things like that that
are largely just cosmetic.
