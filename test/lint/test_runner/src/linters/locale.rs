// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

// Be aware that bitcoind and bitcoin-qt differ in terms of localization: Qt
// opts in to POSIX localization by running setlocale(LC_ALL, "") on startup,
// whereas no such call is made in bitcoind.
//
// Qt runs setlocale(LC_ALL, "") on initialization. This installs the locale
// specified by the user's LC_ALL (or LC_*) environment variable as the new
// C locale.
//
// In contrast, bitcoind does not opt in to localization -- no call to
// setlocale(LC_ALL, "") is made and the environment variables LC_* are
// thus ignored.
//
// This results in situations where bitcoind is guaranteed to be running
// with the classic locale ("C") whereas the locale of bitcoin-qt will vary
// depending on the user's environment variables.
//
// An example: Assuming the environment variable LC_ALL=de_DE then the
// call std::to_string(1.23) will return "1.230000" in bitcoind but
// "1,230000" in bitcoin-qt.
//
// From the Qt documentation:
// "On Unix/Linux Qt is configured to use the system locale settings by default.
//  This can cause a conflict when using POSIX functions, for instance, when
//  converting between data types such as floats and strings, since the notation
//  may differ between locales. To get around this problem, call the POSIX function
//  setlocale(LC_NUMERIC,"C") right after initializing QApplication, QGuiApplication
//  or QCoreApplication to reset the locale that is used for number formatting to
//  "C"-locale."
//
// See https://doc.qt.io/qt-5/qcoreapplication.html#locale-settings and
// https://stackoverflow.com/a/34878283 for more details.

use crate::{exclude, utils, LintResult};

pub fn locale_dependent() -> LintResult {
    let mut locale_error = false;
    let locale_dependent_lines = String::from_utf8(
        utils::git()
            .args(["grep", "-P", &get_locale_dependence_regexp()])
            .args(["--", "*.cpp", "*.h"])
            .args(exclude::get_pathspecs_exclude_locale_dependence())
            .output()
            .expect("command error")
            .stdout,
    )
    .expect("error reading stdout");

    // Filter out expected violations
    let new_locale_dependent_lines = utils::system_grep(
        &get_expected_violations_regexp(),
        &locale_dependent_lines,
        &vec!["-v"],
    );
    for locale_usage in new_locale_dependent_lines.lines() {
        locale_error = true;
        println!(
            r#"
A locale dependent function appears to be used:
{locale_usage}
"#
        )
    }

    if locale_error {
        Err(r#"
^^^
The lint check: 'locale_dependent' found one or more uses of locale dependent functions.

Unnecessary locale dependence can cause bugs that are very tricky to isolate and fix. Please avoid using locale-dependent functions if possible.

Advice not applicable in this specific case? Add an exception by updating the exceptions list.
        "#
        .to_string())
    } else {
        Ok(())
    }
}

fn get_expected_violations_regexp() -> String {
    [
        "src/dbwrapper.cpp:.*vsnprintf",
        "src/test/fuzz/locale.cpp:.*setlocale",
        "src/test/util_tests.cpp:.*strtoll",
        "src/wallet/bdb.cpp:.*DbEnv::strerror", // False positive
        "src/util/syserror.cpp:.*strerror",     // Outside this function use `SysErrorString`
    ]
    // 'grep' utility needs alternation operator to be escaped.
    .join("\\|")
}

fn get_locale_dependence_regexp() -> String {
    format!(
        r#"^(?!\s*//)(?!\s*\*)(?!\s*/\*).*[^a-zA-Z0-9_\\`'"<>]({})(_r|_s)?[^a-zA-Z0-9_\\`'"<>](?!.*\*/)"#,
        get_locale_dependent_functions()
    )
    .to_string()
}

fn get_locale_dependent_functions() -> String {
    vec![
        "alphasort", // LC_COLLATE (via strcoll)
        "asctime",   // LC_TIME (directly)
        "asprintf",  // (via vasprintf)
        "atof",      // LC_NUMERIC (via strtod)
        "atoi",      // LC_NUMERIC (via strtol)
        "atol",      // LC_NUMERIC (via strtol)
        "atoll",     // (via strtoll)
        "atoq",
        "btowc",   // LC_CTYPE (directly)
        "ctime",   // (via asctime or localtime)
        "dprintf", // (via vdprintf)
        "fgetwc",
        "fgetws",
        "fold_case", // boost::locale::fold_case
        "fprintf",   // (via vfprintf)
        "fputwc",
        "fputws",
        "fscanf",   // (via __vfscanf)
        "fwprintf", // (via __vfwprintf)
        "getdate",  // via __getdate_r => isspace // __localtime_r
        "getwc",
        "getwchar",
        "is_digit",   // boost::algorithm::is_digit
        "is_space",   // boost::algorithm::is_space
        "isalnum",    // LC_CTYPE
        "isalpha",    // LC_CTYPE
        "isblank",    // LC_CTYPE
        "iscntrl",    // LC_CTYPE
        "isctype",    // LC_CTYPE
        "isdigit",    // LC_CTYPE
        "isgraph",    // LC_CTYPE
        "islower",    // LC_CTYPE
        "isprint",    // LC_CTYPE
        "ispunct",    // LC_CTYPE
        "isspace",    // LC_CTYPE
        "isupper",    // LC_CTYPE
        "iswalnum",   // LC_CTYPE
        "iswalpha",   // LC_CTYPE
        "iswblank",   // LC_CTYPE
        "iswcntrl",   // LC_CTYPE
        "iswctype",   // LC_CTYPE
        "iswdigit",   // LC_CTYPE
        "iswgraph",   // LC_CTYPE
        "iswlower",   // LC_CTYPE
        "iswprint",   // LC_CTYPE
        "iswpunct",   // LC_CTYPE
        "iswspace",   // LC_CTYPE
        "iswupper",   // LC_CTYPE
        "iswxdigit",  // LC_CTYPE
        "isxdigit",   // LC_CTYPE
        "localeconv", // LC_NUMERIC + LC_MONETARY
        "mblen",      // LC_CTYPE
        "mbrlen",
        "mbrtowc",
        "mbsinit",
        "mbsnrtowcs",
        "mbsrtowcs",
        "mbstowcs", // LC_CTYPE
        "mbtowc",   // LC_CTYPE
        "mktime",
        "normalize", // boost::locale::normalize
        "printf",    // LC_NUMERIC
        "putwc",
        "putwchar",
        "scanf", // LC_NUMERIC
        "setlocale",
        "snprintf",
        "sprintf",
        "sscanf",
        "std::locale::global",
        "std::to_string",
        "stod",
        "stof",
        "stoi",
        "stol",
        "stold",
        "stoll",
        "stoul",
        "stoull",
        "strcasecmp",
        "strcasestr",
        "strcoll", // LC_COLLATE
        "strerror",
        "strfmon",
        "strftime", // LC_TIME
        "strncasecmp",
        "strptime",
        "strtod", // LC_NUMERIC
        "strtof",
        "strtoimax",
        "strtol", // LC_NUMERIC
        "strtold",
        "strtoll",
        "strtoq",
        "strtoul", // LC_NUMERIC
        "strtoull",
        "strtoumax",
        "strtouq",
        "strxfrm", // LC_COLLATE
        "swprintf",
        "to_lower", // boost::locale::to_lower
        "to_title", // boost::locale::to_title
        "to_upper", // boost::locale::to_upper
        "tolower",  // LC_CTYPE
        "toupper",  // LC_CTYPE
        "towctrans",
        "towlower",   // LC_CTYPE
        "towupper",   // LC_CTYPE
        "trim",       // boost::algorithm::trim
        "trim_left",  // boost::algorithm::trim_left
        "trim_right", // boost::algorithm::trim_right
        "ungetwc",
        "vasprintf",
        "vdprintf",
        "versionsort",
        "vfprintf",
        "vfscanf",
        "vfwprintf",
        "vprintf",
        "vscanf",
        "vsnprintf",
        "vsprintf",
        "vsscanf",
        "vswprintf",
        "vwprintf",
        "wcrtomb",
        "wcscasecmp",
        "wcscoll",  // LC_COLLATE
        "wcsftime", // LC_TIME
        "wcsncasecmp",
        "wcsnrtombs",
        "wcsrtombs",
        "wcstod", // LC_NUMERIC
        "wcstof",
        "wcstoimax",
        "wcstol", // LC_NUMERIC
        "wcstold",
        "wcstoll",
        "wcstombs", // LC_CTYPE
        "wcstoul",  // LC_NUMERIC
        "wcstoull",
        "wcstoumax",
        "wcswidth",
        "wcsxfrm", // LC_COLLATE
        "wctob",
        "wctomb", // LC_CTYPE
        "wctrans",
        "wctype",
        "wcwidth",
        "wprintf",
    ]
    .join("|")
}
