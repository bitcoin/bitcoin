#!/bin/sh
# Based on Marco Mornati script https://blog.mornati.net/build-rpms-for-a-git-github-project-with-jenkins/

VERSION=1.14.3
JOB_NAME=bitcoin-${VERSION}
WORKSPACE=`pwd`

rm -rf rpmbuild ${JOB_NAME}.tar.gz
mkdir -p rpmbuild/{BUILD,RPMS,SOURCES/${JOB_NAME},SPECS,SRPMS}
tar --exclude-vcs --exclude='rpmbuild' -cp * | (cd rpmbuild/SOURCES/${JOB_NAME} ; tar xp)
cd ${WORKSPACE}/rpmbuild/SOURCES
tar -czf ${JOB_NAME}.tar.gz ${JOB_NAME}
cd ${WORKSPACE}

cp contrib/rpm/*.spec rpmbuild/SPECS/
sed -i "s/^[\t ]*Source0:.*/Source0: ${JOB_NAME}.tar.gz/g" rpmbuild/SPECS/*.spec
sed -i "s/^[\t ]*%setup[\t ]\+-n[\t ]\+.*/%setup -n ${JOB_NAME}/g" rpmbuild/SPECS/*.spec

rpmbuild --define "_topdir %(pwd)/rpmbuild" -ba rpmbuild/SPECS/*.spec
