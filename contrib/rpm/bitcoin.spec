%define _hardened_build 1
%global selinux_variants mls strict targeted

Name:     bitcoin
Version:  1.14.3
Release:  1%{?prerelease}%{?dist}
Summary:  Peer to Peer Cryptographic Currency

Group:    Applications/System
License:  MIT
URL:      http://bitcoin.org/
Source0:  https://github.com/btc1/%{name}/releases/download/v%{version}/%{name}-%{version}.tar.gz

BuildRoot:  %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  miniupnpc-devel protobuf-devel openssl-devel
BuildRequires:  autoconf automake
BuildRequires:  checkpolicy selinux-policy-devel selinux-policy-doc
BuildRequires:  boost-devel libdb4-cxx-devel libevent-devel
BuildRequires:  libtool java

BuildRequires:  python2

BuildRequires:  python3-zmq zeromq-devel

BuildRequires:  zeromq-devel python3-zmq


%description
Bitcoin is a digital cryptographic currency that uses peer-to-peer technology to
operate with no central authority or banks; managing transactions and the
issuing of bitcoins is carried out collectively by the network.


%package libs
Summary:    Peer-to-peer digital currency


%package devel
Summary:   Peer-to-peer digital currency
Requires:  bitcoin-libs%{?_isa} = %{version}-%{release}


%package utils
Summary:    Peer-to-peer digital currency
Obsoletes:  bitcoin-cli <= 0.9.3


%package server
Summary:          Peer-to-peer digital currency
Requires(post):   systemd
Requires(preun):  systemd
Requires(postun): systemd
BuildRequires:    systemd
Requires(pre):    shadow-utils
Requires(post):   /usr/sbin/semodule, /sbin/restorecon, /sbin/fixfiles
Requires(postun): /usr/sbin/semodule, /sbin/restorecon, /sbin/fixfiles
Requires:   selinux-policy
Requires:   policycoreutils-python
Requires:   openssl-libs
Requires:	  bitcoin-utils%{_isa} = %{version}


%description libs
This package provides the bitcoinconsensus shared libraries. These libraries
may be used by third party software to provide consensus verification
functionality.

Unless you know need this package, you probably do not.


%description devel
This package contains the header files and static library for the
bitcoinconsensus shared library. If you are developing or compiling software
that wants to link against that library, then you need this package installed.

Most people do not need this package installed.


%description utils
Bitcoin is an experimental new digital currency that enables instant
payments to anyone, anywhere in the world. Bitcoin uses peer-to-peer
technology to operate with no central authority: managing transactions
and issuing money are carried out collectively by the network.

This package provides bitcoin-cli, a utility to communicate with and
control a Bitcoin server via its RPC protocol, and bitcoin-tx, a utility
to create custom Bitcoin transactions.


%description server
This package provides a stand-alone bitcoin-core daemon. For most users, this
package is only needed if they need a full-node without the graphical client.

Some third party wallet software will want this package to provide the actual
bitcoin-core node they use to connect to the network.

If you use the graphical bitcoin-core client then you almost certainly do not
need this package.


%prep
%setup -q

# Prep SELinux policy
mkdir SELinux
cp -p contrib/rpm/{bitcoin.te,bitcoin.fc,bitcoin.if} SELinux


%build
# Build Bitcoin
./autogen.sh
%configure --enable-reduce-exports --enable-glibc-back-compat --without-gui

make %{?_smp_mflags}

# Build SELinux policy
pushd SELinux
for selinuxvariant in %{selinux_variants}
do
  make NAME=${selinuxvariant} -f %{_datadir}/selinux/devel/Makefile
  mv bitcoin.pp bitcoin.pp.${selinuxvariant}
  make NAME=${selinuxvariant} -f %{_datadir}/selinux/devel/Makefile clean
done
popd


%check
# Run all the tests
make check
# Run all the other tests
pushd src
srcdir=. test/bitcoin-util-test.py
popd
LD_LIBRARY_PATH=/opt/openssl-compat-bitcoin/lib PYTHONUNBUFFERED=1 qa/pull-tester/rpc-tests.py


%install
rm -rf %{buildroot}
mkdir %{buildroot}
cp contrib/debian/examples/bitcoin.conf bitcoin.conf.example

make INSTALL="install -p" CP="cp -p" DESTDIR=%{buildroot} install

# TODO: Upstream puts bitcoind in the wrong directory. Need to fix the
# upstream Makefiles to relocate it.
mkdir -p -m 755 %{buildroot}%{_sbindir}
mv %{buildroot}%{_bindir}/bitcoind %{buildroot}%{_sbindir}/bitcoind

# Install ancillary files
install -D -m644 -p contrib/debian/examples/bitcoin.conf %{buildroot}%{_tmpfilesdir}/bitcoin.conf
install -D -m600 -p contrib/rpm/bitcoin.sysconfig %{buildroot}%{_sysconfdir}/sysconfig/bitcoin
install -D -m644 -p contrib/rpm/bitcoin.service %{buildroot}%{_unitdir}/bitcoin.service
install -d -m750 -p %{buildroot}%{_localstatedir}/lib/bitcoin
install -d -m750 -p %{buildroot}%{_sysconfdir}/bitcoin
install -D -m644 -p doc/man/bitcoind.1 %{buildroot}%{_mandir}/man1/bitcoind.1
install -D -m644 -p doc/man/bitcoin-cli.1 %{buildroot}%{_mandir}/man1/bitcoin-cli.1
gzip %{buildroot}%{_mandir}/man1/bitcoind.1
gzip %{buildroot}%{_mandir}/man1/bitcoin-cli.1

# Remove test files so that they aren't shipped. Tests have already been run.
rm -f %{buildroot}%{_bindir}/test_*

# We don't ship bench_bitcoin right now
rm -f %{buildroot}%{_bindir}/bench_bitcoin

# Install SELinux policy
for selinuxvariant in %{selinux_variants}
do
	install -d %{buildroot}%{_datadir}/selinux/${selinuxvariant}
	install -p -m 644 SELinux/bitcoin.pp.${selinuxvariant} \
		%{buildroot}%{_datadir}/selinux/${selinuxvariant}/bitcoin.pp
done


%clean
rm -rf %{buildroot}


%pre server
getent group bitcoin >/dev/null || groupadd -r bitcoin
getent passwd bitcoin >/dev/null ||
	useradd -r -g bitcoin -d /var/lib/bitcoin -s /sbin/nologin \
	-c "Bitcoin wallet server" bitcoin
exit 0


%post server
%systemd_post bitcoin.service
for selinuxvariant in %{selinux_variants}
do
	/usr/sbin/semodule -s ${selinuxvariant} -i \
		%{_datadir}/selinux/${selinuxvariant}/bitcoin.pp \
		&> /dev/null || :
done
# FIXME This is less than ideal, but until dwalsh gives me a better way...
/usr/sbin/semanage port -a -t bitcoin_port_t -p tcp 8332
/usr/sbin/semanage port -a -t bitcoin_port_t -p tcp 8333
/usr/sbin/semanage port -a -t bitcoin_port_t -p tcp 18332
/usr/sbin/semanage port -a -t bitcoin_port_t -p tcp 18333
/sbin/fixfiles -R bitcoin-server restore &> /dev/null || :
/sbin/restorecon -R %{_localstatedir}/lib/bitcoin || :


%posttrans server
/usr/bin/systemd-tmpfiles --create


%preun server
%systemd_preun bitcoin.service


%postun server
%systemd_postun bitcoin.service
if [ $1 -eq 0 ] ; then
	# FIXME This is less than ideal, but until dwalsh gives me a better way...
	/usr/sbin/semanage port -d -p tcp 8332
	/usr/sbin/semanage port -d -p tcp 8333
	/usr/sbin/semanage port -d -p tcp 18332
	/usr/sbin/semanage port -d -p tcp 18333
	for selinuxvariant in %{selinux_variants}
	do
		/usr/sbin/semodule -s ${selinuxvariant} -r bitcoin \
		&> /dev/null || :
	done
	/sbin/fixfiles -R bitcoin-server restore &> /dev/null || :
	[ -d %{_localstatedir}/lib/bitcoin ] && \
		/sbin/restorecon -R %{_localstatedir}/lib/bitcoin \
		&> /dev/null || :
fi


%files libs
%defattr(-,root,root,-)
%license COPYING
%doc doc/README.md doc/shared-libraries.md
%{_libdir}/libbitcoinconsensus.so*


%files devel
%defattr(-,root,root,-)
%license COPYING
%doc doc/README.md doc/developer-notes.md doc/shared-libraries.md
%{_includedir}/bitcoinconsensus.h
%{_libdir}/libbitcoinconsensus.a
%{_libdir}/libbitcoinconsensus.la
%{_libdir}/pkgconfig/libbitcoinconsensus.pc


%files utils
%defattr(-,root,root,-)
%license COPYING
%doc bitcoin.conf.example doc/README.md
%{_bindir}/bitcoin-cli
%{_bindir}/bitcoin-tx
%{_mandir}/man1/bitcoin-cli.1*
%{_mandir}/man1/bitcoin-tx.1*


%files server
%defattr(-,root,root,-)
%license COPYING
%doc doc/README.md doc/REST-interface.md doc/bips.md doc/dnsseed-policy.md doc/files.md doc/reduce-traffic.md doc/release-notes.md doc/tor.md doc/zmq.md
%dir %attr(750,bitcoin,bitcoin) %{_localstatedir}/lib/bitcoin
%dir %attr(750,bitcoin,bitcoin) %{_sysconfdir}/bitcoin
%config(noreplace) %attr(600,root,root) %{_sysconfdir}/sysconfig/bitcoin
%doc SELinux/*
%{_sbindir}/bitcoind
%{_unitdir}/bitcoin.service
%{_tmpfilesdir}/bitcoin.conf
%{_mandir}/man1/bitcoind.1*
%{_datadir}/selinux/*/bitcoin.pp


%changelog
* Mon Jul 10 2017 Ismael Bejarano <ismael.bejarano@coinfabrik.com> 1.14.3-1
- Packages for btc1 1.14.3 based on packages from https://www.ringingliberty.com/bitcoin/
