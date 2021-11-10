/*
Copyright Rene Rivera 2011-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/
#include <boost/predef.h>

#define BOOST_PREDEF_INTERNAL_GENERATE_TESTS

void * add_predef_entry(const char * name, const char * description, unsigned value);
#undef BOOST_PREDEF_DECLARE_TEST
#define BOOST_PREDEF_DECLARE_TEST(x,s) void predef_entry_##x() { add_predef_entry(#x, s, x); }
#include <boost/predef.h>

#undef BOOST_PREDEF_DECLARE_TEST
#define BOOST_PREDEF_DECLARE_TEST(x,s) predef_entry_##x();
void create_predef_entries()
{
#include <boost/predef.h>
}

#ifdef __cplusplus
#include <cstring>
#include <cstdio>
#include <cstdlib>
using namespace std;
#else
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#endif

typedef struct predef_info
{
    const char * name;
    const char * description;
    unsigned value;
} predef_info;

#ifdef __cplusplus
using namespace std;
#endif

unsigned generated_predef_info_count = 0;
predef_info* generated_predef_info = 0;
void * add_predef_entry(const char * name, const char * description, unsigned value)
{
    if (0 == generated_predef_info_count)
    {
        generated_predef_info_count = 1;
        generated_predef_info = (predef_info*)malloc(sizeof(predef_info));
    }
    else
    {
        generated_predef_info_count += 1;
        generated_predef_info = (predef_info*)realloc(generated_predef_info,
            generated_predef_info_count*sizeof(predef_info));
    }
    generated_predef_info[generated_predef_info_count-1].name = name;
    generated_predef_info[generated_predef_info_count-1].description = description;
    generated_predef_info[generated_predef_info_count-1].value = value;
    return 0;
}

int predef_info_compare(const void * a, const void * b)
{
    const predef_info * i = (const predef_info *)a;
    const predef_info * j = (const predef_info *)b;
    return strcmp(i->name,j->name);
}
