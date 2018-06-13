#!/bin/bash

export LC_ALL=C
KNOWN_VIOLATIONS=(
    "src/base58.cpp:.*isspace"
    "src/bitcoin-tx.cpp.*stoul"
    "src/bitcoin-tx.cpp.*trim_right"
    "src/bitcoin-tx.cpp:.*atoi"
    "src/core_read.cpp.*is_digit"
    "src/dbwrapper.cpp.*stoul"
    "src/dbwrapper.cpp:.*vsnprintf"
    "src/httprpc.cpp.*trim"
    "src/init.cpp:.*atoi"
    "src/netbase.cpp.*to_lower"
    "src/qt/rpcconsole.cpp:.*atoi"
    "src/qt/rpcconsole.cpp:.*isdigit"
    "src/rest.cpp:.*strtol"
    "src/rpc/server.cpp.*to_upper"
    "src/test/dbwrapper_tests.cpp:.*snprintf"
    "src/test/getarg_tests.cpp.*split"
    "src/torcontrol.cpp:.*atoi"
    "src/torcontrol.cpp:.*strtol"
    "src/uint256.cpp:.*isspace"
    "src/uint256.cpp:.*tolower"
    "src/util.cpp:.*atoi"
    "src/util.cpp:.*fprintf"
    "src/util.cpp:.*tolower"
    "src/utilmoneystr.cpp:.*isdigit"
    "src/utilmoneystr.cpp:.*isspace"
    "src/utilstrencodings.cpp:.*atoi"
    "src/utilstrencodings.cpp:.*isspace"
    "src/utilstrencodings.cpp:.*strtol"
    "src/utilstrencodings.cpp:.*strtoll"
    "src/utilstrencodings.cpp:.*strtoul"
    "src/utilstrencodings.cpp:.*strtoull"
    "src/utilstrencodings.h:.*atoi"
)

REGEXP_IGNORE_EXTERNAL_DEPENDENCIES="^src/(crypto/ctaes/|leveldb/|secp256k1/|tinyformat.h|univalue/)"

LOCALE_DEPENDENT_FUNCTIONS=(
    alphasort    # LC_COLLATE (via strcoll)
    asctime      # LC_TIME (directly)
    asprintf     # (via vasprintf)
    atof         # LC_NUMERIC (via strtod)
    atoi         # LC_NUMERIC (via strtol)
    atol         # LC_NUMERIC (via strtol)
    atoll        # (via strtoll)
    atoq
    btowc        # LC_CTYPE (directly)
    ctime        # (via asctime or localtime)
    dprintf      # (via vdprintf)
    fgetwc
    fgetws
    fold_case    # boost::locale::fold_case
    fprintf      # (via vfprintf)
    fputwc
    fputws
    fscanf       # (via __vfscanf)
    fwprintf     # (via __vfwprintf)
    getdate      # via __getdate_r => isspace // __localtime_r
    getwc
    getwchar
    is_digit     # boost::algorithm::is_digit
    is_space     # boost::algorithm::is_space
    isalnum      # LC_CTYPE
    isalpha      # LC_CTYPE
    isblank      # LC_CTYPE
    iscntrl      # LC_CTYPE
    isctype      # LC_CTYPE
    isdigit      # LC_CTYPE
    isgraph      # LC_CTYPE
    islower      # LC_CTYPE
    isprint      # LC_CTYPE
    ispunct      # LC_CTYPE
    isspace      # LC_CTYPE
    isupper      # LC_CTYPE
    iswalnum     # LC_CTYPE
    iswalpha     # LC_CTYPE
    iswblank     # LC_CTYPE
    iswcntrl     # LC_CTYPE
    iswctype     # LC_CTYPE
    iswdigit     # LC_CTYPE
    iswgraph     # LC_CTYPE
    iswlower     # LC_CTYPE
    iswprint     # LC_CTYPE
    iswpunct     # LC_CTYPE
    iswspace     # LC_CTYPE
    iswupper     # LC_CTYPE
    iswxdigit    # LC_CTYPE
    isxdigit     # LC_CTYPE
    localeconv   # LC_NUMERIC + LC_MONETARY
    mblen        # LC_CTYPE
    mbrlen
    mbrtowc
    mbsinit
    mbsnrtowcs
    mbsrtowcs
    mbstowcs     # LC_CTYPE
    mbtowc       # LC_CTYPE
    mktime
    normalize    # boost::locale::normalize
#   printf       # LC_NUMERIC
    putwc
    putwchar
    scanf        # LC_NUMERIC
    setlocale
    snprintf
    sprintf
    sscanf
    stod
    stof
    stoi
    stol
    stold
    stoll
    stoul
    stoull
    strcasecmp
    strcasestr
    strcoll      # LC_COLLATE
#   strerror
    strfmon
    strftime     # LC_TIME
    strncasecmp
    strptime
    strtod       # LC_NUMERIC
    strtof
    strtoimax
    strtol       # LC_NUMERIC
    strtold
    strtoll
    strtoq
    strtoul      # LC_NUMERIC
    strtoull
    strtoumax
    strtouq
    strxfrm      # LC_COLLATE
    swprintf
    to_lower     # boost::locale::to_lower
    to_title     # boost::locale::to_title
    to_upper     # boost::locale::to_upper
    tolower      # LC_CTYPE
    toupper      # LC_CTYPE
    towctrans
    towlower     # LC_CTYPE
    towupper     # LC_CTYPE
    trim         # boost::algorithm::trim
    trim_left    # boost::algorithm::trim_left
    trim_right   # boost::algorithm::trim_right
    ungetwc
    vasprintf
    vdprintf
    versionsort
    vfprintf
    vfscanf
    vfwprintf
    vprintf
    vscanf
    vsnprintf
    vsprintf
    vsscanf
    vswprintf
    vwprintf
    wcrtomb
    wcscasecmp
    wcscoll      # LC_COLLATE
    wcsftime     # LC_TIME
    wcsncasecmp
    wcsnrtombs
    wcsrtombs
    wcstod       # LC_NUMERIC
    wcstof
    wcstoimax
    wcstol       # LC_NUMERIC
    wcstold
    wcstoll
    wcstombs     # LC_CTYPE
    wcstoul      # LC_NUMERIC
    wcstoull
    wcstoumax
    wcswidth
    wcsxfrm      # LC_COLLATE
    wctob
    wctomb       # LC_CTYPE
    wctrans
    wctype
    wcwidth
    wprintf
)

function join_array {
    local IFS="$1"
    shift
    echo "$*"
}

REGEXP_IGNORE_KNOWN_VIOLATIONS=$(join_array "|" "${KNOWN_VIOLATIONS[@]}")

# Invoke "git grep" only once in order to minimize run-time
REGEXP_LOCALE_DEPENDENT_FUNCTIONS=$(join_array "|" "${LOCALE_DEPENDENT_FUNCTIONS[@]}")
GIT_GREP_OUTPUT=$(git grep -E "[^a-zA-Z0-9_\`'\"<>](${REGEXP_LOCALE_DEPENDENT_FUNCTIONS}(|_r|_s))[^a-zA-Z0-9_\`'\"<>]" -- "*.cpp" "*.h")

EXIT_CODE=0
for LOCALE_DEPENDENT_FUNCTION in "${LOCALE_DEPENDENT_FUNCTIONS[@]}"; do
    MATCHES=$(grep -E "[^a-zA-Z0-9_\`'\"<>]${LOCALE_DEPENDENT_FUNCTION}(|_r|_s)[^a-zA-Z0-9_\`'\"<>]" <<< "${GIT_GREP_OUTPUT}" | \
        grep -vE "\.(c|cpp|h):\s*(//|\*|/\*|\").*${LOCALE_DEPENDENT_FUNCTION}" | \
        grep -vE 'fprintf\(.*(stdout|stderr)')
    if [[ ${REGEXP_IGNORE_EXTERNAL_DEPENDENCIES} != "" ]]; then
        MATCHES=$(grep -vE "${REGEXP_IGNORE_EXTERNAL_DEPENDENCIES}" <<< "${MATCHES}")
    fi
    if [[ ${REGEXP_IGNORE_KNOWN_VIOLATIONS} != "" ]]; then
        MATCHES=$(grep -vE "${REGEXP_IGNORE_KNOWN_VIOLATIONS}" <<< "${MATCHES}")
    fi
    if [[ ${MATCHES} != "" ]]; then
        echo "The locale dependent function ${LOCALE_DEPENDENT_FUNCTION}(...) appears to be used:"
        echo "${MATCHES}"
        echo
        EXIT_CODE=1
    fi
done
if [[ ${EXIT_CODE} != 0 ]]; then
    echo "Unnecessary locale dependence can cause bugs that are very"
    echo "tricky to isolate and fix. Please avoid using locale dependent"
    echo "functions if possible."
    echo
    echo "Advice not applicable in this specific case? Add an exception"
    echo "by updating the ignore list in $0"
fi
exit ${EXIT_CODE}
