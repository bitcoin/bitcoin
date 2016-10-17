Release Process
====================

* update translations (ping wumpus, Diapolo or tcatm on IRC)
<<<<<<< HEAD
* see https://github.com/dashpay/dash/blob/master/doc/translation_process.md#syncing-with-transifex
=======
* see https://github.com/crowncoin/crowncoin/blob/master/doc/translation_process.md#syncing-with-transifex
>>>>>>> origin/dirty-merge-dash-0.11.0

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

###update gitian

 In order to take advantage of the new caching features in gitian, be sure to update to a recent version (e9741525c or higher is recommended)

###perform gitian builds

<<<<<<< HEAD
 From a directory containing the bitcoin source, gitian-builder and gitian.sigs

	export SIGNER=(your gitian key, ie bluematt, sipa, etc)
	export VERSION=(new version, e.g. 0.8.0)
	pushd ./dash
=======
 From a directory containing the crowncoin source, gitian-builder and gitian.sigs
  
	export SIGNER=(your gitian key, ie bluematt, sipa, etc)
	export VERSION=(new version, e.g. 0.8.0)
	pushd ./crowncoin
>>>>>>> origin/dirty-merge-dash-0.11.0
	git checkout v${VERSION}
	popd
	pushd ./gitian-builder

###fetch and build inputs: (first time, or when dependency versions change)
 
<<<<<<< HEAD
	mkdir -p inputs

 Register and download the Apple SDK: (see OSX Readme for details)
 
 https://developer.apple.com/downloads/download.action?path=Developer_Tools/xcode_4.6.3/xcode4630916281a.dmg
 
 Using a Mac, create a tarball for the 10.7 SDK and copy it to the inputs directory:
 
	tar -C /Volumes/Xcode/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/ -czf MacOSX10.7.sdk.tar.gz MacOSX10.7.sdk
=======
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
	./bin/gbuild ../crowncoin/contrib/gitian-descriptors/boost-linux.yml
	mv build/out/boost-*.zip inputs/
	./bin/gbuild ../crowncoin/contrib/gitian-descriptors/deps-linux.yml
	mv build/out/crowncoin-deps-*.zip inputs/
	./bin/gbuild ../crowncoin/contrib/gitian-descriptors/qt-linux.yml
	mv build/out/qt-*.tar.gz inputs/
	./bin/gbuild ../crowncoin/contrib/gitian-descriptors/boost-win.yml
	mv build/out/boost-*.zip inputs/
	./bin/gbuild ../crowncoin/contrib/gitian-descriptors/deps-win.yml
	mv build/out/crowncoin-deps-*.zip inputs/
	./bin/gbuild ../crowncoin/contrib/gitian-descriptors/qt-win.yml
	mv build/out/qt-*.zip inputs/
	./bin/gbuild ../crowncoin/contrib/gitian-descriptors/protobuf-win.yml
	mv build/out/protobuf-*.zip inputs/
	./bin/gbuild ../crowncoin/contrib/gitian-descriptors/gitian-osx-native.yml
	mv build/out/osx-*.tar.gz inputs/
	./bin/gbuild ../crowncoin/contrib/gitian-descriptors/gitian-osx-depends.yml
	mv build/out/osx-*.tar.gz inputs/
	./bin/gbuild ../crowncoin/contrib/gitian-descriptors/gitian-osx-qt.yml
	mv build/out/osx-*.tar.gz inputs/

 The expected SHA256 hashes of the intermediate inputs are:

    46710f673467e367738d8806e45b4cb5931aaeea61f4b6b55a68eea56d5006c5  crowncoin-deps-linux32-gitian-r6.zip
    f03be39fb26670243d3a659e64d18e19d03dec5c11e9912011107768390b5268  crowncoin-deps-linux64-gitian-r6.zip
    f29b7d9577417333fb56e023c2977f5726a7c297f320b175a4108cf7cd4c2d29  boost-linux32-1.55.0-gitian-r1.zip
    88232451c4104f7eb16e469ac6474fd1231bd485687253f7b2bdf46c0781d535  boost-linux64-1.55.0-gitian-r1.zip
    57e57dbdadc818cd270e7e00500a5e1085b3bcbdef69a885f0fb7573a8d987e1  qt-linux32-4.6.4-gitian-r1.tar.gz
    60eb4b9c5779580b7d66529efa5b2836ba1a70edde2a0f3f696d647906a826be  qt-linux64-4.6.4-gitian-r1.tar.gz
    60dc2d3b61e9c7d5dbe2f90d5955772ad748a47918ff2d8b74e8db9b1b91c909  boost-win32-1.55.0-gitian-r6.zip
    f65fcaf346bc7b73bc8db3a8614f4f6bee2f61fcbe495e9881133a7c2612a167  boost-win64-1.55.0-gitian-r6.zip
    70de248cd0dd7e7476194129e818402e974ca9c5751cbf591644dc9f332d3b59  crowncoin-deps-win32-gitian-r13.zip
    9eace4c76f639f4f3580a478eee4f50246e1bbb5ccdcf37a158261a5a3fa3e65  crowncoin-deps-win64-gitian-r13.zip
    963e3e5e85879010a91143c90a711a5d1d5aba992e38672cdf7b54e42c56b2f1  qt-win32-5.2.0-gitian-r3.zip
    751c579830d173ef3e6f194e83d18b92ebef6df03289db13ab77a52b6bc86ef0  qt-win64-5.2.0-gitian-r3.zip
    e2e403e1a08869c7eed4d4293bce13d51ec6a63592918b90ae215a0eceb44cb4  protobuf-win32-2.5.0-gitian-r4.zip
    a0999037e8b0ef9ade13efd88fee261ba401f5ca910068b7e0cd3262ba667db0  protobuf-win64-2.5.0-gitian-r4.zip

 Build crowncoind and crowncoin-qt on Linux32, Linux64, and Win32:
  
	./bin/gbuild --commit crowncoin=v${VERSION} ../crowncoin/contrib/gitian-descriptors/gitian-linux.yml
	./bin/gsign --signer $SIGNER --release ${VERSION} --destination ../gitian.sigs/ ../crowncoin/contrib/gitian-descriptors/gitian-linux.yml
	pushd build/out
	zip -r crowncoin-${VERSION}-linux-gitian.zip *
	mv crowncoin-${VERSION}-linux-gitian.zip ../../../
	popd
	./bin/gbuild --commit crowncoin=v${VERSION} ../crowncoin/contrib/gitian-descriptors/gitian-win.yml
	./bin/gsign --signer $SIGNER --release ${VERSION}-win --destination ../gitian.sigs/ ../crowncoin/contrib/gitian-descriptors/gitian-win.yml
	pushd build/out
	zip -r crowncoin-${VERSION}-win-gitian.zip *
	mv crowncoin-${VERSION}-win-gitian.zip ../../../
	popd
        ./bin/gbuild --commit crowncoin=v${VERSION} ../crowncoin/contrib/gitian-descriptors/gitian-osx-crowncoin.yml
        ./bin/gsign --signer $SIGNER --release ${VERSION}-osx --destination ../gitian.sigs/ ../crowncoin/contrib/gitian-descriptors/gitian-osx-crowncoin.yml
	pushd build/out
	mv Crowncoin-Qt.dmg ../../../
	popd
	popd

  Build output expected:

  1. linux 32-bit and 64-bit binaries + source (crowncoin-${VERSION}-linux-gitian.zip)
  2. windows 32-bit and 64-bit binaries + installer + source (crowncoin-${VERSION}-win-gitian.zip)
  3. OSX installer (Crowncoin-Qt.dmg)
  4. Gitian signatures (in gitian.sigs/${VERSION}[-win|-osx]/(your gitian key)/
>>>>>>> origin/dirty-merge-dash-0.11.0

###Optional: Seed the Gitian sources cache

  By default, gitian will fetch source files as needed. For offline builds, they can be fetched ahead of time:

<<<<<<< HEAD
	make -C ../dash/depends download SOURCES_PATH=`pwd`/cache/common
=======
	unzip crowncoin-${VERSION}-linux-gitian.zip -d crowncoin-${VERSION}-linux
	tar czvf crowncoin-${VERSION}-linux.tar.gz crowncoin-${VERSION}-linux
	rm -rf crowncoin-${VERSION}-linux
>>>>>>> origin/dirty-merge-dash-0.11.0

  Only missing files will be fetched, so this is safe to re-run for each build.

<<<<<<< HEAD
###Build Dash Core for Linux, Windows, and OS X:
=======
	unzip crowncoin-${VERSION}-win-gitian.zip -d crowncoin-${VERSION}-win
	mv crowncoin-${VERSION}-win/crowncoin-*-setup.exe .
	zip -r crowncoin-${VERSION}-win.zip crowncoin-${VERSION}-win
	rm -rf crowncoin-${VERSION}-win
>>>>>>> origin/dirty-merge-dash-0.11.0

	./bin/gbuild --commit dash=v${VERSION} ../dash/contrib/gitian-descriptors/gitian-linux.yml
	./bin/gsign --signer $SIGNER --release ${VERSION}-linux --destination ../gitian.sigs/ ../dash/contrib/gitian-descriptors/gitian-linux.yml
	mv build/out/dash-*.tar.gz build/out/src/dash-*.tar.gz ../
	./bin/gbuild --commit dash=v${VERSION} ../dash/contrib/gitian-descriptors/gitian-win.yml
	./bin/gsign --signer $SIGNER --release ${VERSION}-win --destination ../gitian.sigs/ ../dash/contrib/gitian-descriptors/gitian-win.yml
	mv build/out/dash-*.zip build/out/dash-*.exe ../
	./bin/gbuild --commit bitcoin=v${VERSION} ../dash/contrib/gitian-descriptors/gitian-osx.yml
	./bin/gsign --signer $SIGNER --release ${VERSION}-osx-unsigned --destination ../gitian.sigs/ ../dash/contrib/gitian-descriptors/gitian-osx.yml
	mv build/out/dash-*-unsigned.tar.gz inputs/dash-osx-unsigned.tar.gz
	mv build/out/dash-*.tar.gz build/out/dash-*.dmg ../
	popd
  Build output expected:

  1. source tarball (dash-${VERSION}.tar.gz)
  2. linux 32-bit and 64-bit binaries dist tarballs (dash-${VERSION}-linux[32|64].tar.gz)
  3. windows 32-bit and 64-bit installers and dist zips (dash-${VERSION}-win[32|64]-setup.exe, dash-${VERSION}-win[32|64].zip)
  4. OSX unsigned installer (dash-${VERSION}-osx-unsigned.dmg)
  5. Gitian signatures (in gitian.sigs/${VERSION}-<linux|win|osx-unsigned>/(your gitian key)/

* update wiki changelog: [https://en.crowncoin.it/wiki/Changelog](https://en.crowncoin.it/wiki/Changelog)

Commit your signature to gitian.sigs:

	pushd gitian.sigs
	git add ${VERSION}-linux/${SIGNER}
	git add ${VERSION}-win/${SIGNER}
	git add ${VERSION}-osx-unsigned/${SIGNER}
	git commit -a
	git push  # Assuming you can push to the gitian.sigs tree
	popd

  Wait for OSX detached signature:
	Once the OSX build has 3 matching signatures, Evan(?) ***TODO*** will sign it with the apple App-Store key.
	He will then upload a detached signature to be combined with the unsigned app to create a signed binary.

  Create the signed OSX binary:

	pushd ./gitian-builder
	# Fetch the signature as instructed by Evan
	cp signature.tar.gz inputs/
	./bin/gbuild -i ../dash/contrib/gitian-descriptors/gitian-osx-signer.yml
	./bin/gsign --signer $SIGNER --release ${VERSION}-osx-signed --destination ../gitian.sigs/ ../dash/contrib/gitian-descriptors/gitian-osx-signer.yml
	mv build/out/dash-osx-signed.dmg ../dash-${VERSION}-osx.dmg
	popd

Commit your signature for the signed OSX binary:

	pushd gitian.sigs
	git add ${VERSION}-osx-signed/${SIGNER}
	git commit -a
	git push  # Assuming you can push to the gitian.sigs tree
	popd

-------------------------------------------------------------------------

### After 3 or more people have gitian-built and their results match:

From a directory containing crowncoin source, gitian.sigs and gitian zips

	export VERSION=(new version, e.g. 0.8.0)
	mkdir crowncoin-${VERSION}-linux-gitian
	pushd crowncoin-${VERSION}-linux-gitian
	unzip ../crowncoin-${VERSION}-linux-gitian.zip
	mkdir gitian
	cp ../crowncoin/contrib/gitian-downloader/*.pgp ./gitian/
	for signer in $(ls ../gitian.sigs/${VERSION}/); do
	 cp ../gitian.sigs/${VERSION}/${signer}/crowncoin-build.assert ./gitian/${signer}-build.assert
	 cp ../gitian.sigs/${VERSION}/${signer}/crowncoin-build.assert.sig ./gitian/${signer}-build.assert.sig
	done
	zip -r crowncoin-${VERSION}-linux-gitian.zip *
	cp crowncoin-${VERSION}-linux-gitian.zip ../
	popd
	mkdir crowncoin-${VERSION}-win-gitian
	pushd crowncoin-${VERSION}-win-gitian
	unzip ../crowncoin-${VERSION}-win-gitian.zip
	mkdir gitian
	cp ../crowncoin/contrib/gitian-downloader/*.pgp ./gitian/
	for signer in $(ls ../gitian.sigs/${VERSION}-win/); do
	 cp ../gitian.sigs/${VERSION}-win/${signer}/crowncoin-build.assert ./gitian/${signer}-build.assert
	 cp ../gitian.sigs/${VERSION}-win/${signer}/crowncoin-build.assert.sig ./gitian/${signer}-build.assert.sig
	done
	zip -r crowncoin-${VERSION}-win-gitian.zip *
	cp crowncoin-${VERSION}-win-gitian.zip ../
	popd

  Note: only Evan has the code-signing keys currently.

- Create `SHA256SUMS.asc` for the builds, and GPG-sign it:
```bash
sha256sum * > SHA256SUMS
gpg --digest-algo sha256 --clearsign SHA256SUMS # outputs SHA256SUMS.asc
rm SHA256SUMS
```
(the digest algorithm is forced to sha256 to avoid confusion of the `Hash:` header that GPG adds with the SHA256 used for the files)

- Upload zips and installers, as well as `SHA256SUMS.asc` from last step, to the bitcoin.org server
  into `/var/www/bin/bitcoin-core-${VERSION}`

- Update dashpay.io version ***TODO***

  - First, check to see if the dashpay.io maintainers have prepared a
    release: https://github.com/bitcoin/bitcoin.org/labels/Releases

      - If they have, it will have previously failed their Travis CI
        checks because the final release files weren't uploaded.
        Trigger a Travis CI rebuild---if it passes, merge.

<<<<<<< HEAD
  - If they have not prepared a release, follow the Bitcoin.org release
    instructions: https://github.com/bitcoin/bitcoin.org#release-notes

  - After the pull request is merged, the website will automatically show the newest version within 15 minutes, as well
    as update the OS download links. Ping @saivann/@harding (saivann/harding on Freenode) in case anything goes wrong

- Announce the release:

  - Release sticky on dashtalk: https://dashtalk.org/index.php?board=1.0 ***TODO***

  - Dash-development mailing list

  - Update title of #dashpay on Freenode IRC

  - Optionally reddit /r/Dashpay, ... but this will usually sort out itself

- Notify Flare (?) ***TODO*** so that he can start building [https://launchpad.net/~dashpay/+archive/ubuntu/dash](the PPAs) ***TODO***
=======
  - Add the release to crowncoin.org: https://github.com/crowncoin/crowncoin.org/tree/master/_releases

  - Release sticky on crowncointalk: https://crowncointalk.org/index.php?board=1.0

  - Crowncoin-development mailing list
>>>>>>> origin/dirty-merge-dash-0.11.0

  - Optionally reddit /r/Crowncoin, ...

- Celebrate
