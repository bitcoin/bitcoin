#!/bin/bash

###   This script attempts to download the signature file SHA256SUMS.asc from bitcoin.org
###   It first checks if the signature passes, and then downloads the files specified in
###   the file, and checks if the hashes of these files match those that are specified
###   in the signature file.
###   The script returns 0 if everything passes the checks. It returns 1 if either the
###   signature check or the hash check doesn't pass. If an error occurs the return value is 2

function clean_up {
   for file in $*
   do
      rm "$file" 2> /dev/null
   done
}

WORKINGDIR="/tmp/bitcoin"
TMPFILE="hashes.tmp"

#this URL is used if a version number is not specified as an argument to the script
SIGNATUREFILE="https://bitcoin.org/bin/0.9.2.1/SHA256SUMS.asc"

SIGNATUREFILENAME="SHA256SUMS.asc"
RCSUBDIR="test/"
BASEDIR="https://bitcoin.org/bin/"
VERSIONPREFIX="bitcoin-"
RCVERSIONSTRING="rc"

if [ ! -d "$WORKINGDIR" ]; then
   mkdir "$WORKINGDIR"
fi

cd "$WORKINGDIR"

#test if a version number has been passed as an argument
if [ -n "$1" ]; then
   #let's also check if the version number includes the prefix 'bitcoin-',
   #  and add this prefix if it doesn't
   if [[ $1 == "$VERSIONPREFIX"* ]]; then
      VERSION="$1"
   else
      VERSION="$VERSIONPREFIX$1"
   fi

   #now let's see if the version string contains "rc", and strip it off if it does
   #  and simultaneously add RCSUBDIR to BASEDIR, where we will look for SIGNATUREFILENAME
   if [[ $VERSION == *"$RCVERSIONSTRING"* ]]; then
      BASEDIR="$BASEDIR${VERSION/%-$RCVERSIONSTRING*}/"
      BASEDIR="$BASEDIR$RCSUBDIR"
   else
      BASEDIR="$BASEDIR$VERSION/"
   fi

   SIGNATUREFILE="$BASEDIR$SIGNATUREFILENAME"
else
   BASEDIR="${SIGNATUREFILE%/*}/"
fi

#first we fetch the file containing the signature
WGETOUT=$(wget -N "$BASEDIR$SIGNATUREFILENAME" 2>&1)

#and then see if wget completed successfully
if [ $? -ne 0 ]; then
   echo "Error: couldn't fetch signature file. Have you specified the version number in the following format?"
   echo "[bitcoin-]<version>-[rc[0-9]] (example: bitcoin-0.9.2-rc1)"
   echo "wget output:"
   echo "$WGETOUT"|sed 's/^/\t/g'
   exit 2
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
      echo "gpg error. Do you have Gavin's code signing key installed?"
   fi

   echo "gpg output:"
   echo "$GPGOUT"|sed 's/^/\t/g'
   clean_up $SIGNATUREFILENAME $TMPFILE
   exit "$RET"
fi

#here we extract the filenames from the signature file
FILES=$(awk '{print $2}' "$TMPFILE")

#and download these one by one
for file in in $FILES
do
   wget --quiet -N "$BASEDIR$file"
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

#everything matches! clean up the mess
clean_up $FILES $SIGNATUREFILENAME $TMPFILE

exit 0
