#!/bin/sh
set -o xtrace   # Write all commands first to stderr
set -o errexit  # Exit the script with error if any of the commands fail

OS=$(uname -s | tr '[:upper:]' '[:lower:]')

echo '{ "results": [ { "status": "pass", "test_file": "Create Release Archive",' > test-results.json
start=$(date +%s)
echo '"start": ' $start ', ' >> test-results.json

case "$OS" in
   cygwin*)
      sh ./.evergreen/compile-windows.sh
   ;;

   *)
      sh ./.evergreen/compile-unix.sh
   ;;
esac

end=$(date +%s)
echo '"end": ' $end ', ' >> test-results.json
sum=$(expr $end - $start || true)
echo '"elapsed": ' $sum ' } ] }' >> test-results.json

