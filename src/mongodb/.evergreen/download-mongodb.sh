#!/bin/sh
set -o xtrace   # Write all commands first to stderr
set -o errexit  # Exit the script with error if any of the commands fail

get_distro ()
{
   if [ -f /etc/os-release ]; then
      . /etc/os-release
      DISTRO="${ID}-${VERSION_ID}"
   elif command -v lsb_release 2>/dev/null; then
      name=$(lsb_release -s -i)
      if [ "$name" = "RedHatEnterpriseServer" ]; then # RHEL 6.2 at least
         name="rhel"
      fi
      version=$(lsb_release -s -r)
      DISTRO="${name}-${version}"
   elif [ -f /etc/redhat-release ]; then
      release=$(cat /etc/redhat-release)

      if [[ "$release" =~ "Red Hat" ]]; then
         name="rhel"
      elif [[ "$release" =~ "Fedora" ]]; then
         name="fedora"
      fi
      version=$(echo $release | sed 's/.*\([[:digit:]]\).*/\1/g')
      DISTRO="${name}-${version}"
   elif [ -f /etc/lsb-release ]; then
      . /etc/lsb-release
      DISTRO="${DISTRIB_ID}-${DISTRIB_RELEASE}"
   fi

   OS=$(uname -s)
   MARCH=$(uname -m)
   DISTRO=$(echo "$OS-$DISTRO-$MARCH" | tr '[:upper:]' '[:lower:]')

   echo $DISTRO
}

# get_mongodb_download_url_for "linux-distro-version-architecture" "latest|34|32|30|28|26|24"
# Sets EXTRACT to aproprate extract command
# Sets MONGODB_DOWNLOAD_URL to the aproprate download url
get_mongodb_download_url_for ()
{
   _DISTRO=$1
   _VERSION=$2

   VERSION_34="3.4.0"
   VERSION_32="3.2.11"
   VERSION_30="3.0.14"
   VERSION_26="2.6.12"
   VERSION_24="2.4.12"

   EXTRACT="tar zxf"
   # getdata matrix on:
   # https://evergreen.mongodb.com/version/5797f0493ff12235e5001f05
   case "$_DISTRO" in
      darwin*)
         MONGODB_LATEST="http://downloads.10gen.com/osx/mongodb-osx-x86_64-enterprise-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/osx/mongodb-osx-x86_64-enterprise-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/osx/mongodb-osx-x86_64-enterprise-${VERSION_32}.tgz"
             MONGODB_30="https://fastdl.mongodb.org/osx/mongodb-osx-x86_64-${VERSION_30}.tgz"
             MONGODB_26="https://fastdl.mongodb.org/osx/mongodb-osx-x86_64-${VERSION_26}.tgz"
             MONGODB_24="https://fastdl.mongodb.org/osx/mongodb-osx-x86_64-${VERSION_24}.tgz"
      ;;
      sunos*i86pc)
         MONGODB_LATEST="https://fastdl.mongodb.org/sunos5/mongodb-sunos5-x86_64-latest.tgz"
             MONGODB_34="https://fastdl.mongodb.org/sunos5/mongodb-sunos5-x86_64-${VERSION_34}.tgz"
             MONGODB_32="https://fastdl.mongodb.org/sunos5/mongodb-sunos5-x86_64-${VERSION_32}.tgz"
             MONGODB_30="https://fastdl.mongodb.org/sunos5/mongodb-sunos5-x86_64-${VERSION_30}.tgz"
             MONGODB_26="https://fastdl.mongodb.org/sunos5/mongodb-sunos5-x86_64-${VERSION_26}.tgz"
             MONGODB_24="https://fastdl.mongodb.org/sunos5/mongodb-sunos5-x86_64-${VERSION_24}.tgz"
      ;;
      linux-rhel-7.2-s390x)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-s390x-enterprise-rhel72-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-s390x-enterprise-rhel72-${VERSION_34}.tgz"
             MONGODB_32=""
             MONGODB_30=""
             MONGODB_26=""
             MONGODB_24=""
      ;;
      linux-rhel-7.1-ppc64le)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-ppc64le-enterprise-rhel71-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-ppc64le-enterprise-rhel71-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/linux/mongodb-linux-ppc64le-enterprise-rhel71-${VERSION_32}.tgz"
             MONGODB_30=""
             MONGODB_26=""
             MONGODB_24=""
      ;;
      linux-rhel-7.0*)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel70-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel70-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel70-${VERSION_32}.tgz"
             MONGODB_30="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel70-${VERSION_30}.tgz"
             MONGODB_26="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel70-${VERSION_26}.tgz"
             MONGODB_24=""
      ;;
      linux-rhel-6.2*)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel62-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel62-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel62-${VERSION_32}.tgz"
             MONGODB_30="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel62-${VERSION_30}.tgz"
             MONGODB_26="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel62-${VERSION_26}.tgz"
             MONGODB_24="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel62-${VERSION_24}.tgz"
      ;;
      linux-rhel-5.5*)
         MONGODB_LATEST="http://downloads.mongodb.org/linux/mongodb-linux-x86_64-rhel55-latest.tgz"
             MONGODB_34="http://downloads.mongodb.org/linux/mongodb-linux-x86_64-rhel55-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.mongodb.org/linux/mongodb-linux-x86_64-rhel55-${VERSION_32}.tgz"
             MONGODB_30="http://downloads.mongodb.org/linux/mongodb-linux-x86_64-rhel55-${VERSION_30}.tgz"
             MONGODB_26=""
             MONGODB_24=""
      ;;
      linux-sles-12.1-s390x)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-s390x-enterprise-suse12-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-s390x-enterprise-suse12-${VERSION_34}.tgz"
             MONGODB_32=""
             MONGODB_30=""
             MONGODB_26=""
             MONGODB_24=""
      ;;
      linux-debian-8*)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-debian81-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-debian81-${VERSION_34}.tgz"
             MONGODB_32=""
             MONGODB_30=""
             MONGODB_26=""
             MONGODB_24=""
      ;;
      linux-ubuntu-16.04-s390x)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-s390x-enterprise-ubuntu1604-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-s390x-enterprise-ubuntu1604-${VERSION_34}.tgz"
             MONGODB_32=""
             MONGODB_30=""
             MONGODB_26=""
             MONGODB_24=""
      ;;
      linux-ubuntu-16.04-ppc64le)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-ppc64le-enterprise-ubuntu1604-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-ppc64le-enterprise-ubuntu1604-${VERSION_34}.tgz"
             MONGODB_32=""
             MONGODB_30=""
             MONGODB_26=""
             MONGODB_24=""
      ;;
      linux-ubuntu-16.04-aarch64)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-arm64-enterprise-ubuntu1604-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-arm64-enterprise-ubuntu1604-${VERSION_34}.tgz"
             MONGODB_32=""
             MONGODB_30=""
             MONGODB_26=""
             MONGODB_24=""
      ;;
      linux-ubuntu-16.04*)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1604-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1604-${VERSION_34}.tgz"
             MONGODB_32=""
             MONGODB_30=""
             MONGODB_26=""
             MONGODB_24=""
      ;;
      linux-ubuntu-14.04*)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1404-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1404-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1404-${VERSION_32}.tgz"
             MONGODB_30="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1404-${VERSION_30}.tgz"
             MONGODB_26="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1404-${VERSION_26}.tgz"
             MONGODB_24=""
      ;;
      linux-ubuntu-12.04*)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1204-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1204-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1204-${VERSION_32}.tgz"
             MONGODB_30="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1204-${VERSION_30}.tgz"
             MONGODB_26="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1204-${VERSION_26}.tgz"
             MONGODB_24="http://downloads.10gen.com/linux/mongodb-linux-x86_64-subscription-ubuntu1204-${VERSION_24}.tgz"
      ;;
      windows32*)
         EXTRACT="/cygdrive/c/Progra~2/7-Zip/7z.exe x"
         MONGODB_LATEST="https://fastdl.mongodb.org/win32/mongodb-win32-i386-latest.zip"
             MONGODB_34="https://fastdl.mongodb.org/win32/mongodb-win32-i386-${VERSION_34}.zip"
             MONGODB_32="https://fastdl.mongodb.org/win32/mongodb-win32-i386-${VERSION_32}.zip"
             MONGODB_30="https://fastdl.mongodb.org/win32/mongodb-win32-i386-${VERSION_30}.zip"
             MONGODB_26="https://fastdl.mongodb.org/win32/mongodb-win32-i386-${VERSION_26}.zip"
             MONGODB_24="https://fastdl.mongodb.org/win32/mongodb-win32-i386-${VERSION_24}.zip"
      ;;
      windows64*)
         EXTRACT="/cygdrive/c/Progra~2/7-Zip/7z.exe x"
         MONGODB_LATEST="https://fastdl.mongodb.org/win32/mongodb-win32-x86_64-2008plus-latest.zip"
             MONGODB_34="https://fastdl.mongodb.org/win32/mongodb-win32-x86_64-2008plus-${VERSION_34}.zip"
             MONGODB_32="https://fastdl.mongodb.org/win32/mongodb-win32-x86_64-2008plus-${VERSION_32}.zip"
             MONGODB_30="https://fastdl.mongodb.org/win32/mongodb-win32-x86_64-2008plus-${VERSION_30}.zip"
             MONGODB_26="https://fastdl.mongodb.org/win32/mongodb-win32-x86_64-2008plus-${VERSION_26}.zip"
             MONGODB_24="https://fastdl.mongodb.org/win32/mongodb-win32-x86_64-2008plus-${VERSION_24}.zip"
      ;;
      cygwin*)
         EXTRACT="/cygdrive/c/Progra~2/7-Zip/7z.exe x"
         MONGODB_LATEST="http://downloads.10gen.com/win32/mongodb-win32-x86_64-enterprise-windows-64-latest.zip"
             MONGODB_34="http://downloads.10gen.com/win32/mongodb-win32-x86_64-enterprise-windows-64-${VERSION_34}.zip"
             MONGODB_32="http://downloads.10gen.com/win32/mongodb-win32-x86_64-enterprise-windows-64-${VERSION_32}.zip"
             MONGODB_30="http://downloads.10gen.com/win32/mongodb-win32-x86_64-enterprise-windows-64-${VERSION_30}.zip"
             MONGODB_26="http://downloads.10gen.com/win32/mongodb-win32-x86_64-enterprise-windows-64-${VERSION_26}.zip"
             MONGODB_24=""
      ;;
      *linux*x86_64)
         MONGODB_LATEST="http://downloads.mongodb.org/linux/mongodb-linux-x86_64-latest.tgz"
             MONGODB_34="http://downloads.mongodb.org/linux/mongodb-linux-x86_64-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.mongodb.org/linux/mongodb-linux-x86_64-${VERSION_32}.tgz"
             MONGODB_30="http://downloads.mongodb.org/linux/mongodb-linux-x86_64-${VERSION_30}.tgz"
             MONGODB_26="http://downloads.mongodb.org/linux/mongodb-linux-x86_64-${VERSION_26}.tgz"
             MONGODB_24="http://downloads.mongodb.org/linux/mongodb-linux-x86_64-${VERSION_24}.tgz"
      ;;
   esac

   MONGODB_DOWNLOAD_URL=""
   case "$_VERSION" in
      latest) MONGODB_DOWNLOAD_URL=$MONGODB_LATEST ;;
      3.4) MONGODB_DOWNLOAD_URL=$MONGODB_34 ;;
      3.2) MONGODB_DOWNLOAD_URL=$MONGODB_32 ;;
      3.0) MONGODB_DOWNLOAD_URL=$MONGODB_30 ;;
      2.6) MONGODB_DOWNLOAD_URL=$MONGODB_26 ;;
      2.4) MONGODB_DOWNLOAD_URL=$MONGODB_24 ;;
   esac

   [ -z "$MONGODB_DOWNLOAD_URL" ] && MONGODB_DOWNLOAD_URL="Unknown version: $_VERSION for $_DISTRO"

   echo $MONGODB_DOWNLOAD_URL
}

download_and_extract ()
{
   MONGODB_DOWNLOAD_URL=$1
   EXTRACT=$2

   curl --retry 5 $MONGODB_DOWNLOAD_URL --silent --max-time 120 --fail --output mongodb-binaries.tgz

   $EXTRACT mongodb-binaries.tgz

   rm -rf mongodb*tgz mongodb/
   mv mongodb* mongodb
   chmod -R +x mongodb
   find . -name vcredist_x64.exe -exec {} /install /quiet \;
   ./mongodb/bin/mongod --version
}
