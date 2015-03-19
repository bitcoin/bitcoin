Release Process
====================

* update translations (ping wumpus, Diapolo or tcatm on IRC)
* see https://github.com/bitcoin/bitcoin/blob/master/doc/translation_process.md#syncing-with-transifex

* * *

###update (commit) version in sources

	contrib/verifysfbinaries/verify.sh
	doc/README*
	share/setup.nsi
	src/clientversion.h (change CLIENT_VERSION_IS_RELEASE to true)

###tag version in git

	git tag -s v(new version, e.g. 0.8.0)

###write release notes. git shortlog helps a lot, for example:

	git shortlog --no-merges v(current version, e.g. 0.7.2)..v(new version, e.g. 0.8.0)

* * *

##perform gitian builds

 From a directory containing the bitcoin source, gitian-builder and gitian.sigs

	export SIGNER=(your gitian key, ie bluematt, sipa, etc)
	export VERSION=(new version, e.g. 0.8.0)
	pushd ./bitcoin
	git checkout v${VERSION}
	popd
	pushd ./gitian-builder
        mkdir -p inputs; cd inputs/

 Register and download the Apple SDK (see OSX Readme for details)
	visit https://developer.apple.com/downloads/download.action?path=Developer_Tools/xcode_4.6.3/xcode4630916281a.dmg

 Using a Mac, create a tarball for the 10.7 SDK
	tar -C /Volumes/Xcode/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/ -czf MacOSX10.7.sdk.tar.gz MacOSX10.7.sdk

 Fetch and build inputs: (first time, or when dependency versions change)

	wget 'http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.9.20140701.tar.gz' -O miniupnpc-1.9.20140701.tar.gz
	wget 'https://www.openssl.org/source/openssl-1.0.1k.tar.gz'
	wget 'http://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz'
	wget 'http://zlib.net/zlib-1.2.8.tar.gz'
	wget 'ftp://ftp.simplesystems.org/pub/png/src/history/libpng16/libpng-1.6.8.tar.gz'
	wget 'https://fukuchi.org/works/qrencode/qrencode-3.4.3.tar.bz2'
	wget 'https://downloads.sourceforge.net/project/boost/boost/1.55.0/boost_1_55_0.tar.bz2'
	wget 'https://svn.boost.org/trac/boost/raw-attachment/ticket/7262/boost-mingw.patch' -O \
	     boost-mingw-gas-cross-compile-2013-03-03.patch
	wget 'https://download.qt-project.org/official_releases/qt/5.2/5.2.0/single/qt-everywhere-opensource-src-5.2.0.tar.gz'
	wget 'https://download.qt-project.org/archive/qt/4.6/qt-everywhere-opensource-src-4.6.4.tar.gz'
	wget 'https://protobuf.googlecode.com/files/protobuf-2.5.0.tar.bz2'
	wget 'https://github.com/mingwandroid/toolchain4/archive/10cc648683617cca8bcbeae507888099b41b530c.tar.gz'
	wget 'http://www.opensource.apple.com/tarballs/cctools/cctools-809.tar.gz'
	wget 'http://www.opensource.apple.com/tarballs/dyld/dyld-195.5.tar.gz'
	wget 'http://www.opensource.apple.com/tarballs/ld64/ld64-127.2.tar.gz'
	wget 'http://pkgs.fedoraproject.org/repo/pkgs/cdrkit/cdrkit-1.1.11.tar.gz/efe08e2f3ca478486037b053acd512e9/cdrkit-1.1.11.tar.gz'
	wget 'https://github.com/theuni/libdmg-hfsplus/archive/libdmg-hfsplus-v0.1.tar.gz'
	wget 'http://llvm.org/releases/3.2/clang+llvm-3.2-x86-linux-ubuntu-12.04.tar.gz' -O \
	     clang-llvm-3.2-x86-linux-ubuntu-12.04.tar.gz
        wget 'https://raw.githubusercontent.com/theuni/osx-cross-depends/master/patches/cdrtools/genisoimage.diff' -O \
	     cdrkit-deterministic.patch
	cd ..
	./bin/gbuild ../bitcoin/contrib/gitian-descriptors/boost-linux.yml
	mv build/out/boost-*.zip inputs/
	./bin/gbuild ../bitcoin/contrib/gitian-descriptors/deps-linux.yml
	mv build/out/bitcoin-deps-*.zip inputs/
	./bin/gbuild ../bitcoin/contrib/gitian-descriptors/qt-linux.yml
	mv build/out/qt-*.tar.gz inputs/
	./bin/gbuild ../bitcoin/contrib/gitian-descriptors/boost-win.yml
	mv build/out/boost-*.zip inputs/
	./bin/gbuild ../bitcoin/contrib/gitian-descriptors/deps-win.yml
	mv build/out/bitcoin-deps-*.zip inputs/
	./bin/gbuild ../bitcoin/contrib/gitian-descriptors/qt-win.yml
	mv build/out/qt-*.zip inputs/
	./bin/gbuild ../bitcoin/contrib/gitian-descriptors/protobuf-win.yml
	mv build/out/protobuf-*.zip inputs/
	./bin/gbuild ../bitcoin/contrib/gitian-descriptors/gitian-osx-native.yml
	mv build/out/osx-*.tar.gz inputs/
	./bin/gbuild ../bitcoin/contrib/gitian-descriptors/gitian-osx-depends.yml
	mv build/out/osx-*.tar.gz inputs/
	./bin/gbuild ../bitcoin/contrib/gitian-descriptors/gitian-osx-qt.yml
	mv build/out/osx-*.tar.gz inputs/

 The expected SHA256 hashes of the intermediate inputs are:

    b1f6f10148d4c4a1a69a58e703427578dc5a4de86eefd6b925e3abf3c8fbe542  bitcoin-deps-linux32-gitian-r9.zip
    71e03e434af269dcbf3cb685cd1a5d51b8d2c904b67035eb4e5c1a2623b9f0df  bitcoin-deps-linux64-gitian-r9.zip
    f29b7d9577417333fb56e023c2977f5726a7c297f320b175a4108cf7cd4c2d29  boost-linux32-1.55.0-gitian-r1.zip
    88232451c4104f7eb16e469ac6474fd1231bd485687253f7b2bdf46c0781d535  boost-linux64-1.55.0-gitian-r1.zip
    57e57dbdadc818cd270e7e00500a5e1085b3bcbdef69a885f0fb7573a8d987e1  qt-linux32-4.6.4-gitian-r1.tar.gz
    60eb4b9c5779580b7d66529efa5b2836ba1a70edde2a0f3f696d647906a826be  qt-linux64-4.6.4-gitian-r1.tar.gz
    60dc2d3b61e9c7d5dbe2f90d5955772ad748a47918ff2d8b74e8db9b1b91c909  boost-win32-1.55.0-gitian-r6.zip
    f65fcaf346bc7b73bc8db3a8614f4f6bee2f61fcbe495e9881133a7c2612a167  boost-win64-1.55.0-gitian-r6.zip
    2af17b1968bd7d46b260c8d16474e1f339cde1b9e96265c80f6626ea0c2785a9  bitcoin-deps-win32-gitian-r16.zip
    7608bdf7848101d48ba8a296cb9c29ac68193405f11d8075fb46154ff3476233  bitcoin-deps-win64-gitian-r16.zip
    963e3e5e85879010a91143c90a711a5d1d5aba992e38672cdf7b54e42c56b2f1  qt-win32-5.2.0-gitian-r3.zip
    751c579830d173ef3e6f194e83d18b92ebef6df03289db13ab77a52b6bc86ef0  qt-win64-5.2.0-gitian-r3.zip
    e2e403e1a08869c7eed4d4293bce13d51ec6a63592918b90ae215a0eceb44cb4  protobuf-win32-2.5.0-gitian-r4.zip
    a0999037e8b0ef9ade13efd88fee261ba401f5ca910068b7e0cd3262ba667db0  protobuf-win64-2.5.0-gitian-r4.zip

 Build dashd and dash-qt on Linux32, Linux64, and Win32:

	./bin/gbuild --commit dash=v${VERSION} ../dash/contrib/gitian-descriptors/gitian-linux.yml
	./bin/gsign --signer $SIGNER --release ${VERSION} --destination ../gitian.sigs/ ../dash/contrib/gitian-descriptors/gitian-linux.yml
	pushd build/out
	zip -r dash-${VERSION}-linux-gitian.zip *
	mv dash-${VERSION}-linux-gitian.zip ../../../
	popd
	./bin/gbuild --commit dash=v${VERSION} ../dash/contrib/gitian-descriptors/gitian-win.yml
	./bin/gsign --signer $SIGNER --release ${VERSION}-win --destination ../gitian.sigs/ ../dash/contrib/gitian-descriptors/gitian-win.yml
	pushd build/out
	zip -r dash-${VERSION}-win-gitian.zip *
	mv dash-${VERSION}-win-gitian.zip ../../../
	popd
    ./bin/gbuild --commit dash=v${VERSION} ../dash/contrib/gitian-descriptors/gitian-osx-bitcoin.yml
    ./bin/gsign --signer $SIGNER --release ${VERSION}-osx --destination ../gitian.sigs/ ../bitcoin/contrib/gitian-descriptors/gitian-osx-bitcoin.yml
	pushd build/out
	mv Dash-Qt.dmg ../../../
	popd
	popd

  Build output expected:

  1. linux 32-bit and 64-bit binaries + source (bitcoin-${VERSION}-linux-gitian.zip)
  2. windows 32-bit and 64-bit binaries + installer + source (bitcoin-${VERSION}-win-gitian.zip)
  3. OSX installer (Bitcoin-Qt.dmg)
  4. Gitian signatures (in gitian.sigs/${VERSION}[-win|-osx]/(your gitian key)/

repackage gitian builds for release as stand-alone zip/tar/installer exe

**Linux .tar.gz:**

	unzip bitcoin-${VERSION}-linux-gitian.zip -d bitcoin-${VERSION}-linux
	tar czvf bitcoin-${VERSION}-linux.tar.gz bitcoin-${VERSION}-linux
	rm -rf bitcoin-${VERSION}-linux

**Windows .zip and setup.exe:**

	unzip bitcoin-${VERSION}-win-gitian.zip -d bitcoin-${VERSION}-win
	mv bitcoin-${VERSION}-win/bitcoin-*-setup.exe .
	zip -r bitcoin-${VERSION}-win.zip bitcoin-${VERSION}-win
	rm -rf bitcoin-${VERSION}-win

**Mac OS X .dmg:**

	mv Bitcoin-Qt.dmg bitcoin-${VERSION}-osx.dmg

###Next steps:

Commit your signature to gitian.sigs:

	pushd gitian.sigs
	git add ${VERSION}-linux/${SIGNER}
	git add ${VERSION}-win/${SIGNER}
	git add ${VERSION}-osx/${SIGNER}
	git commit -a
	git push  # Assuming you can push to the gitian.sigs tree
	popd

-------------------------------------------------------------------------

### After 3 or more people have gitian-built and their results match:

- Perform code-signing.

    - Code-sign Windows -setup.exe (in a Windows virtual machine using signtool)

    - Code-sign MacOSX .dmg

  Note: only Gavin has the code-signing keys currently.

- Create `SHA256SUMS.asc` for builds, and PGP-sign it. This is done manually.
  Include all the files to be uploaded. The file has `sha256sum` format with a
  simple header at the top:

```
Hash: SHA256

0060f7d38b98113ab912d4c184000291d7f026eaf77ca5830deec15059678f54  bitcoin-x.y.z-linux.tar.gz
...
```

- Upload zips and installers, as well as `SHA256SUMS.asc` from last step, to the bitcoin.org server

- Update bitcoin.org version

  - Make a pull request to add a file named `YYYY-MM-DD-vX.Y.Z.md` with the release notes
  to https://github.com/bitcoin/bitcoin.org/tree/master/_releases
   ([Example for 0.9.2.1](https://raw.githubusercontent.com/bitcoin/bitcoin.org/master/_releases/2014-06-19-v0.9.2.1.md)).

  - After the pull request is merged, the website will automatically show the newest version, as well
    as update the OS download links. Ping Saivann in case anything goes wrong

- Announce the release:

  - Release sticky on bitcointalk: https://bitcointalk.org/index.php?board=1.0

  - Bitcoin-development mailing list

  - Update title of #bitcoin on Freenode IRC

  - Optionally reddit /r/Bitcoin, ... but this will usually sort out itself

- Notify BlueMatt so that he can start building [https://launchpad.net/~bitcoin/+archive/ubuntu/bitcoin](the PPAs)

- Add release notes for the new version to the directory `doc/release-notes` in git master

- Celebrate
