find_library(LIBICONV_LIBRARY iconv)
get_filename_component(LIBICONV_DIR "${LIBICONV_LIBRARY}" DIRECTORY)

list(APPEND B2_OPTIONS
    boost.locale.iconv=on
    boost.locale.posix=on
    /boost/locale//boost_locale
    boost.locale.icu=off
    -sICONV_PATH=${LIBICONV_LIBRARY}
)
