#!/usr/bin/env bash
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

###   This script attempts to download the signature file SHA256SUMS.asc from
###   bitcoincore.org and bitcoin.org and compares them.
###   It first checks if the signature passes, and then downloads the files specified in
###   the file, and checks if the hashes of these files match those that are specified
###   in the signature file.
###   The script returns 0 if everything passes the checks. It returns 1 if either the
###   signature check or the hash check doesn't pass. If an error occurs the return value is 2

export LC_ALL=C
function clean_up {
   for file in "$@"
   do
      rm "$file" 2> /dev/null
   done
}

WORKINGDIR="/tmp/bitcoin_verify_binaries"
TMPFILE="hashes.tmp"

SIGNATUREFILENAME="SHA256SUMS.asc"
RCSUBDIR="test"
HOST1="https://bitcoincore.org"
HOST2="https://bitcoin.org"
BASEDIR="/bin/"
VERSIONPREFIX="bitcoin-core-"
RCVERSIONSTRING="rc"

if [ ! -d "$WORKINGDIR" ]; then
   mkdir "$WORKINGDIR"
fi

cd "$WORKINGDIR" || exit 1

#test if a version number has been passed as an argument
if [ -n "$1" ]; then
   #let's also check if the version number includes the prefix 'bitcoin-',
   #  and add this prefix if it doesn't
   if [[ $1 == "$VERSIONPREFIX"* ]]; then
      VERSION="$1"
   else
      VERSION="$VERSIONPREFIX$1"
   fi

   STRIPPEDLAST="${VERSION%-*}"

   #now let's see if the version string contains "rc" or a platform name (e.g. "osx")
   if [[ "$STRIPPEDLAST-" == "$VERSIONPREFIX" ]]; then
      BASEDIR="$BASEDIR$VERSION/"
   else
      # let's examine the last part to see if it's rc and/or platform name
      STRIPPEDNEXTTOLAST="${STRIPPEDLAST%-*}"
      if [[ "$STRIPPEDNEXTTOLAST-" == "$VERSIONPREFIX" ]]; then

         LASTSUFFIX="${VERSION##*-}"
         VERSION="$STRIPPEDLAST"

         if [[ $LASTSUFFIX == *"$RCVERSIONSTRING"* ]]; then
            RCVERSION="$LASTSUFFIX"
         else
            PLATFORM="$LASTSUFFIX"
         fi

      else
         RCVERSION="${STRIPPEDLAST##*-}"
         PLATFORM="${VERSION##*-}"

         VERSION="$STRIPPEDNEXTTOLAST"
      fi

      BASEDIR="$BASEDIR$VERSION/"
      if [[ $RCVERSION == *"$RCVERSIONSTRING"* ]]; then
         BASEDIR="$BASEDIR$RCSUBDIR.$RCVERSION/"
      fi
   fi
else
   echo "Error: need to specify a version on the command line"
   exit 2
fi

if ! WGETOUT=$(wget -N "$HOST1$BASEDIR$SIGNATUREFILENAME" 2>&1); then
   echo "Error: couldn't fetch signature file. Have you specified the version number in the following format?"
   # shellcheck disable=SC1087
   echo "[$VERSIONPREFIX]<version>-[$RCVERSIONSTRING[0-9]] (example: ${VERSIONPREFIX}0.10.4-${RCVERSIONSTRING}1)"
   echo "wget output:"
   # shellcheck disable=SC2001
   echo "$WGETOUT"|sed 's/^/\t/g'
   exit 2
fi

if ! WGETOUT=$(wget -N -O "$SIGNATUREFILENAME.2" "$HOST2$BASEDIR$SIGNATUREFILENAME" 2>&1); then
   echo "bitcoin.org failed to provide signature file, but bitcoincore.org did?"
   echo "wget output:"
   # shellcheck disable=SC2001
   echo "$WGETOUT"|sed 's/^/\t/g'
   clean_up $SIGNATUREFILENAME
   exit 3
fi

SIGFILEDIFFS="$(diff $SIGNATUREFILENAME $SIGNATUREFILENAME.2)"
if [ "$SIGFILEDIFFS" != "" ]; then
   echo "bitcoin.org and bitcoincore.org signature files were not equal?"
   clean_up $SIGNATUREFILENAME $SIGNATUREFILENAME.2
   exit 4
fi

#then we check it
GPGOUT=$(gpg --yes --decrypt --output "$TMPFILE" "$SIGNATUREFILENAME" 2>&1)

#return value 0: good signature
#return value 1: bad signature
#return value 2: gpg error

RET="$?"
if [ $RET -ne 0 ]; then
   if [ $RET -eq 1 ]; then
      #and notify the user if it's bad
      echo "Bad signature."
   elif [ $RET -eq 2 ]; then
      #or if a gpg error has occurred
      echo "gpg error. Do you have the Bitcoin Core binary release signing key installed?"
   fi

   echo "gpg output:"
   # shellcheck disable=SC2001
   echo "$GPGOUT"|sed 's/^/\t/g'
   clean_up $SIGNATUREFILENAME $SIGNATUREFILENAME.2 $TMPFILE
   exit "$RET"
fi

if [ -n "$PLATFORM" ]; then
   grep $PLATFORM $TMPFILE > "$TMPFILE-plat"
   TMPFILESIZE=$(stat -c%s "$TMPFILE-plat")
   if [ $TMPFILESIZE -eq 0 ]; then
      echo "error: no files matched the platform specified" && exit 3
   fi
   mv "$TMPFILE-plat" $TMPFILE
fi

#here we extract the filenames from the signature file
FILES=$(awk '{print $2}' "$TMPFILE")

#and download these one by one
for file in $FILES
do
   echo "Downloading $file"
   wget --quiet -N "$HOST1$BASEDIR$file"
done

#check hashes
DIFF=$(diff <(sha256sum $FILES) "$TMPFILE")

if [ $? -eq 1 ]; then
   echo "Hashes don't match."
   echo "Offending files:"
   echo "$DIFF"|grep "^<"|awk '{print "\t"$3}'
   exit 1
elif [ $? -gt 1 ]; then
   echo "Error executing 'diff'"
   exit 2
fi

if [ -n "$2" ]; then
   echo "Clean up the binaries"
   clean_up $FILES $SIGNATUREFILENAME $SIGNATUREFILENAME.2 $TMPFILE
else
   echo "Keep the binaries in $WORKINGDIR"
   clean_up $TMPFILE
fi

echo -e "Verified hashes of \n$FILES"

exit 0
