#!/bin/sh
set -o xtrace   # Write all commands first to stderr
set -o errexit  # Exit the script with error if any of the commands fail

echo '{ "results": [ { "status": "pass", "test_file": "Create Release Archive",' > test-results.json
start=$(date +%s)
echo '"start": ' $start ', ' >> test-results.json

# Use modern sphinx-build from venv.
. venv/bin/activate
which sphinx-build
sphinx-build --version

./autogen.sh --enable-html-docs --enable-man-pages && make distcheck
sphinx-build -b linkcheck ./doc doc/html

# Check that docs were included, but sphinx temp files weren't.
tarfile=libbson-*.tar.gz
docs='libbson-*/doc/html/index.html libbson-*/doc/man/bson_t.3'
tmpfiles='libbson-*/doc/html/.doctrees \
   libbson-*/doc/html/.buildinfo \
   libbson-*/doc/man/.doctrees \
   libbson-*/doc/man/.buildinfo'

echo "Checking for built docs"
for doc in $docs; do
   # Check this doc is in the archive.
   tar --wildcards -tzf $tarfile $doc
done

echo "Checking that temp files are not included in tarball"
for tmpfile in $tmpfiles; do
   # Check this temp file doesn't exist.
   if tar --wildcards -tzf $tarfile $tmpfile > /dev/null 2>&1; then
      echo "Found temp file in archive: $tmpfile"
      exit 1
   fi
done

echo "Checking that all C files are included in tarball"
# Check that all C files were included.
TAR_CFILES=`tar --wildcards -tf libbson-*.tar.gz 'libbson-*/src/bson/*.c' | cut -d / -f 4 | sort`
SRC_CFILES=`echo src/bson/*.c | xargs -n 1 | cut -d / -f 3 | sort`
if [ "$TAR_CFILES" != "$SRC_CFILES" ]; then
   echo "Not all C files are in the release archive"
   echo $TAR_CFILES > tar_cfiles.txt
   echo $SRC_CFILES | diff -y - tar_cfiles.txt
fi

end=$(date +%s)
echo '"end": ' $end ', ' >> test-results.json
sum=$(expr $end - $start)
echo '"elapsed": ' $sum ' } ] }' >> test-results.json
