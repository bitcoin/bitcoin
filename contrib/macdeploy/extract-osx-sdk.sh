#!/bin/bash
set -e

INPUTFILE="Xcode_7.3.1.dmg"
HFSFILENAME="5.hfs"
SDKDIR="Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk"

7z x "${INPUTFILE}" "${HFSFILENAME}"
SDKNAME="$(basename "${SDKDIR}")"
SDKDIRINODE=$(ifind -n "${SDKDIR}" "${HFSFILENAME}")
fls "${HFSFILENAME}" -rpF ${SDKDIRINODE} |
 while read type inode filename; do
	inode="${inode::-1}"
	if [ "${filename:0:14}" = "usr/share/man/" ]; then
		continue
	fi
	filename="${SDKNAME}/$filename"
	echo "Extracting $filename ..."
	mkdir -p "$(dirname "$filename")"
	if [ "$type" = "l/l" ]; then
		ln -s "$(icat "${HFSFILENAME}" $inode)" "$filename"
	else
		icat "${HFSFILENAME}" $inode >"$filename"
	fi
done
echo "Building ${SDKNAME}.tar.gz ..."
MTIME="$(istat "${HFSFILENAME}" "${SDKDIRINODE}" | perl -nle 'm/Content Modified:\s+(.*?)\s\(/ && print $1')"
find "${SDKNAME}" | sort | tar --no-recursion --mtime="${MTIME}" --mode='u+rw,go+r-w,a+X' --owner=0 --group=0 -c -T - | gzip -9n > "${SDKNAME}.tar.gz"
echo 'All done!'
