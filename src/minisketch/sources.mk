# - All variables are namespaced with MINISKETCH_ to avoid colliding with
#     downstream makefiles.
# - All Variables ending in _HEADERS or _SOURCES confuse automake, so the
#     _INT postfix is applied.
# - Convenience variables, for example a MINISKETCH_FIELDS_DIR should not be used
#     as they interfere with automatic dependency generation
# - The %reldir% is the relative path from the Makefile.am. This allows
#   downstreams to use these variables without having to manually account for
#   the path change.

MINISKETCH_INCLUDE_DIR_INT = %reldir%/include

MINISKETCH_DIST_HEADERS_INT =
MINISKETCH_DIST_HEADERS_INT += %reldir%/include/minisketch.h

MINISKETCH_LIB_HEADERS_INT =
MINISKETCH_LIB_HEADERS_INT += %reldir%/src/false_positives.h
MINISKETCH_LIB_HEADERS_INT += %reldir%/src/fielddefines.h
MINISKETCH_LIB_HEADERS_INT += %reldir%/src/int_utils.h
MINISKETCH_LIB_HEADERS_INT += %reldir%/src/lintrans.h
MINISKETCH_LIB_HEADERS_INT += %reldir%/src/sketch.h
MINISKETCH_LIB_HEADERS_INT += %reldir%/src/sketch_impl.h
MINISKETCH_LIB_HEADERS_INT += %reldir%/src/util.h

MINISKETCH_LIB_SOURCES_INT =
MINISKETCH_LIB_SOURCES_INT += %reldir%/src/minisketch.cpp

MINISKETCH_FIELD_GENERIC_HEADERS_INT =
MINISKETCH_FIELD_GENERIC_HEADERS_INT += %reldir%/src/fields/generic_common_impl.h

MINISKETCH_FIELD_GENERIC_SOURCES_INT =
MINISKETCH_FIELD_GENERIC_SOURCES_INT += %reldir%/src/fields/generic_1byte.cpp
MINISKETCH_FIELD_GENERIC_SOURCES_INT += %reldir%/src/fields/generic_2bytes.cpp
MINISKETCH_FIELD_GENERIC_SOURCES_INT += %reldir%/src/fields/generic_3bytes.cpp
MINISKETCH_FIELD_GENERIC_SOURCES_INT += %reldir%/src/fields/generic_4bytes.cpp
MINISKETCH_FIELD_GENERIC_SOURCES_INT += %reldir%/src/fields/generic_5bytes.cpp
MINISKETCH_FIELD_GENERIC_SOURCES_INT += %reldir%/src/fields/generic_6bytes.cpp
MINISKETCH_FIELD_GENERIC_SOURCES_INT += %reldir%/src/fields/generic_7bytes.cpp
MINISKETCH_FIELD_GENERIC_SOURCES_INT += %reldir%/src/fields/generic_8bytes.cpp

MINISKETCH_FIELD_CLMUL_HEADERS_INT =
MINISKETCH_FIELD_CLMUL_HEADERS_INT += %reldir%/src/fields/clmul_common_impl.h

MINISKETCH_FIELD_CLMUL_SOURCES_INT =
MINISKETCH_FIELD_CLMUL_SOURCES_INT += %reldir%/src/fields/clmul_1byte.cpp
MINISKETCH_FIELD_CLMUL_SOURCES_INT += %reldir%/src/fields/clmul_2bytes.cpp
MINISKETCH_FIELD_CLMUL_SOURCES_INT += %reldir%/src/fields/clmul_3bytes.cpp
MINISKETCH_FIELD_CLMUL_SOURCES_INT += %reldir%/src/fields/clmul_4bytes.cpp
MINISKETCH_FIELD_CLMUL_SOURCES_INT += %reldir%/src/fields/clmul_5bytes.cpp
MINISKETCH_FIELD_CLMUL_SOURCES_INT += %reldir%/src/fields/clmul_6bytes.cpp
MINISKETCH_FIELD_CLMUL_SOURCES_INT += %reldir%/src/fields/clmul_7bytes.cpp
MINISKETCH_FIELD_CLMUL_SOURCES_INT += %reldir%/src/fields/clmul_8bytes.cpp

MINISKETCH_BENCH_SOURCES_INT =
MINISKETCH_BENCH_SOURCES_INT += %reldir%/src/bench.cpp

MINISKETCH_TEST_SOURCES_INT =
MINISKETCH_TEST_SOURCES_INT += %reldir%/src/test.cpp
