#! /bin/sh

# Allow invocation from a separate build directory; in that case, we change
# to the source directory to run the auto*, then change back before running configure
srcdir=`dirname "$0"`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd "$srcdir" || exit 1

if [ ! -f README ]; then
    ln -s README.md README
fi

touch ChangeLog
touch AUTHORS

if test -z "$(which libtoolize)" && test -z "$(which glibtoolize)"; then
    echo "Error: libtoolize was not found on your system. Cannot continue."
    if test "$(uname)" = "Darwin"; then
        echo "On Darwin, this is named glibtoolize"
    fi
fi

if test -z `which autoreconf`; then
    echo "Error: autoreconf not found, please install it."
    exit 1
fi
autoreconf --force --verbose --install -I build/autotools $ACLOCAL_FLAGS|| exit $?
rm -rf autom4te.cache

cd "$ORIGDIR" || exit 1

if test -z "$NOCONFIGURE"; then
    "$srcdir"/configure $AUTOGEN_CONFIGURE_ARGS "$@" || exit $?
fi
