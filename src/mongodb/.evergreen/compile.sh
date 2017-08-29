#!/bin/sh
set -o xtrace   # Write all commands first to stderr
set -o errexit  # Exit the script with error if any of the commands fail

OS=$(uname -s | tr '[:upper:]' '[:lower:]')

case "$OS" in
   cygwin*)
      sh ./.evergreen/compile-windows.sh
   ;;

   *)
      sh ./.evergreen/compile-unix.sh
   ;;
esac
