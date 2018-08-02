#!/bin/bash

# Script to test how much bloating a large project will suffer when using
# tinyformat, vs alternatives.  Call as
#
# C99 printf            :  bloat_test.sh $CXX [-O3]
# tinyformat            :  bloat_test.sh $CXX [-O3] -DUSE_TINYFORMAT
# tinyformat, no inlines:  bloat_test.sh $CXX [-O3] -DUSE_TINYFORMAT -DUSE_TINYFORMAT_NOINLINE
# boost::format         :  bloat_test.sh $CXX [-O3] -DUSE_BOOST
# std::iostream         :  bloat_test.sh $CXX [-O3] -DUSE_IOSTREAMS
#
# Note: to test the NOINLINE version of tinyformat, you need to remove the few
# inline functions in the tinyformat::detail namespace, and put them into a
# file tinyformat.cpp.  Then rename that version of tinyformat.h into
# tinyformat_noinline.h


prefix=_bloat_test_tmp_
numTranslationUnits=100

rm -f $prefix??.cpp ${prefix}main.cpp ${prefix}all.h

template='
#ifdef USE_BOOST

#include <boost/format.hpp>
#include <iostream>

void doFormat_a()
{
    std::cout << boost::format("%s\n") % "somefile.cpp";
    std::cout << boost::format("%s:%d\n") % "somefile.cpp"% 42;
    std::cout << boost::format("%s:%d:%s\n") % "somefile.cpp"% 42% "asdf";
    std::cout << boost::format("%s:%d:%d:%s\n") % "somefile.cpp"% 42% 1% "asdf";
    std::cout << boost::format("%s:%d:%d:%d:%s\n") % "somefile.cpp"% 42% 1% 2% "asdf";
}

#elif defined(USE_IOSTREAMS)

#include <iostream>

void doFormat_a()
{
    const char* str1 = "somefile.cpp";
    const char* str2 = "asdf";
    std::cout << str1 << "\n";
    std::cout << str1 << ":" << 42 << "\n";
    std::cout << str1 << ":" << 42 << ":" << str2 << "\n";
    std::cout << str1 << ":" << 42 << ":" << 1 << ":" << str2 << "\n";
    std::cout << str1 << ":" << 42 << ":" << 1 << ":" << 2 << ":" << str2 << "\n";
}

#else
#ifdef USE_TINYFORMAT
#   ifdef USE_TINYFORMAT_NOINLINE
#       include "tinyformat_noinline.h"
#   else
#       include "tinyformat.h"
#   endif
#   define PRINTF tfm::printf
#else
#   include <stdio.h>
#   define PRINTF ::printf
#endif

void doFormat_a()
{
    const char* str1 = "somefile.cpp";
    const char* str2 = "asdf";
    PRINTF("%s\n", str1);
    PRINTF("%s:%d\n", str1, 42);
    PRINTF("%s:%d:%s\n", str1, 42, str2);
    PRINTF("%s:%d:%d:%s\n", str1, 42, 1, str2);
    PRINTF("%s:%d:%d:%d:%s\n", str1, 42, 1, 2, str2);
}
#endif
'

# Generate all the files
echo "#include \"${prefix}all.h\"" >> ${prefix}main.cpp
echo '
#ifdef USE_TINYFORMAT_NOINLINE
#include "tinyformat.cpp"
#endif

int main()
{' >> ${prefix}main.cpp

for ((i=0;i<$numTranslationUnits;i++)) ; do
    n=$(printf "%03d" $i)
    f=${prefix}$n.cpp
    echo "$template" | sed -e "s/doFormat_a/doFormat_a$n/" -e "s/42/$i/" > $f
    echo "doFormat_a$n();" >> ${prefix}main.cpp
    echo "void doFormat_a$n();" >> ${prefix}all.h
done

echo "return 0; }" >> ${prefix}main.cpp


# Compile
time "$@" ${prefix}???.cpp ${prefix}main.cpp -o ${prefix}.out
ls -sh ${prefix}.out
cp ${prefix}.out ${prefix}stripped.out
strip ${prefix}stripped.out
ls -sh ${prefix}stripped.out
