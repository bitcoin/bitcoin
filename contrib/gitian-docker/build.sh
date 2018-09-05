#!/bin/bash

VERSION=$1

if [[ -n "$VERSION" ]]; then
	echo ${VERSION}
else
    echo "argument error, provide commit, branch or tag"
	exit
fi

cd /gitian-builder/

./bin/gbuild --commit peercoin=${VERSION} ../peercoin/contrib/gitian-descriptors/gitian-linux.yml
pushd build/out
zip -r peercoin-${VERSION}-linux-gitian.zip *
mv peercoin-${VERSION}-linux-gitian.zip /peercoin/
popd

./bin/gbuild --commit peercoin=${VERSION} ../peercoin/contrib/gitian-descriptors/gitian-win.yml
pushd build/out
zip -r peercoin-${VERSION}-win-gitian.zip *
mv peercoin-${VERSION}-win-gitian.zip /peercoin/
popd

./bin/gbuild --commit peercoin=${VERSION} ../peercoin/contrib/gitian-descriptors/gitian-osx.yml
pushd build/out
zip -r peercoin-${VERSION}-osx-gitian.zip *
mv peercoin-${VERSION}-osx-gitian.zip /peercoin/
popd

echo "build complete, files are in /peercoin/ directory"
