#!/usr/bin/env python3
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Lint format strings: This program checks that the number of arguments passed
# to a variadic format string function matches the number of format specifiers
# in the format string.

import argparse
import re
import sys

FALSE_POSITIVES = [
    ("src/dbwrapper.cpp", "vsnprintf(p, limit - p, format, backup_ap)"),
    ("src/index/base.cpp", "FatalError(const char* fmt, const Args&... args)"),
    ("src/netbase.cpp", "LogConnectFailure(bool manual_connection, const char* fmt, const Args&... args)"),
    ("src/clientversion.cpp", "strprintf(_(COPYRIGHT_HOLDERS).translated, COPYRIGHT_HOLDERS_SUBSTITUTION)"),
    ("src/test/translation_tests.cpp", "strprintf(format, arg)"),
    ("src/validationinterface.cpp", "LogPrint(BCLog::VALIDATION, fmt \"\\n\", __VA_ARGS__)"),
    ("src/wallet/wallet.h",  "WalletLogPrintf(std::string fmt, Params... parameters)"),
    ("src/wallet/wallet.h", "LogPrintf((\"%s \" + fmt).c_str(), GetDisplayName(), parameters...)"),
    ("src/wallet/scriptpubkeyman.h",  "WalletLogPrintf(std::string fmt, Params... parameters)"),
    ("src/wallet/scriptpubkeyman.h", "LogPrintf((\"%s \" + fmt).c_str(), m_storage.GetDisplayName(), parameters...)"),
    ("src/batchedlogger.h", "strprintf(fmt, args...)"),
]


def parse_function_calls(function_name, source_code):
    """Return an array with all calls to function function_name in string source_code.
    Preprocessor directives and C++ style comments ("//") in source_code are removed.

    >>> len(parse_function_calls("foo", "foo();bar();foo();bar();"))
    2
    >>> parse_function_calls("foo", "foo(1);bar(1);foo(2);bar(2);")[0].startswith("foo(1);")
    True
    >>> parse_function_calls("foo", "foo(1);bar(1);foo(2);bar(2);")[1].startswith("foo(2);")
    True
    >>> len(parse_function_calls("foo", "foo();bar();// foo();bar();"))
    1
    >>> len(parse_function_calls("foo", "#define FOO foo();"))
    0
    """
    assert type(function_name) is str and type(source_code) is str and function_name
    lines = [re.sub("// .*", " ", line).strip()
             for line in source_code.split("\n")
             if not line.strip().startswith("#")]
    return re.findall(r"[^a-zA-Z_](?=({}\(.*).*)".format(function_name), " " + " ".join(lines))


def normalize(s):
    """Return a normalized version of string s with newlines, tabs and C style comments ("/* ... */")
    replaced with spaces. Multiple spaces are replaced with a single space.

    >>> normalize("  /* nothing */   foo\tfoo  /* bar */  foo     ")
    'foo foo foo'
    """
    assert type(s) is str
    s = s.replace("\n", " ")
    s = s.replace("\t", " ")
    s = re.sub(r"/\*.*?\*/", " ", s)
    s = re.sub(" {2,}", " ", s)
    return s.strip()


ESCAPE_MAP = {
    r"\n": "[escaped-newline]",
    r"\t": "[escaped-tab]",
    r'\"': "[escaped-quote]",
}


def escape(s):
    """Return the escaped version of string s with "\\\"", "\\n" and "\\t" escaped as
    "[escaped-backslash]", "[escaped-newline]" and "[escaped-tab]".

    >>> unescape(escape("foo")) == "foo"
    True
    >>> escape(r'foo \\t foo \\n foo \\\\ foo \\ foo \\"bar\\"')
    'foo [escaped-tab] foo [escaped-newline] foo \\\\\\\\ foo \\\\ foo [escaped-quote]bar[escaped-quote]'
    """
    assert type(s) is str
    for raw_value, escaped_value in ESCAPE_MAP.items():
        s = s.replace(raw_value, escaped_value)
    return s


def unescape(s):
    """Return the unescaped version of escaped string s.
    Reverses the replacements made in function escape(s).

    >>> unescape(escape("bar"))
    'bar'
    >>> unescape("foo [escaped-tab] foo [escaped-newline] foo \\\\\\\\ foo \\\\ foo [escaped-quote]bar[escaped-quote]")
    'foo \\\\t foo \\\\n foo \\\\\\\\ foo \\\\ foo \\\\"bar\\\\"'
    """
    assert type(s) is str
    for raw_value, escaped_value in ESCAPE_MAP.items():
        s = s.replace(escaped_value, raw_value)
    return s


def parse_function_call_and_arguments(function_name, function_call):
    """Split string function_call into an array of strings consisting of:
    * the string function_call followed by "("
    * the function call argument #1
    * ...
    * the function call argument #n
    * a trailing ");"

    The strings returned are in escaped form. See escape(...).

    >>> parse_function_call_and_arguments("foo", 'foo("%s", "foo");')
    ['foo(', '"%s",', ' "foo"', ')']
    >>> parse_function_call_and_arguments("foo", 'foo("%s", "foo");')
    ['foo(', '"%s",', ' "foo"', ')']
    >>> parse_function_call_and_arguments("foo", 'foo("%s %s", "foo", "bar");')
    ['foo(', '"%s %s",', ' "foo",', ' "bar"', ')']
    >>> parse_function_call_and_arguments("fooprintf", 'fooprintf("%050d", i);')
    ['fooprintf(', '"%050d",', ' i', ')']
    >>> parse_function_call_and_arguments("foo", 'foo(bar(foobar(barfoo("foo"))), foobar); barfoo')
    ['foo(', 'bar(foobar(barfoo("foo"))),', ' foobar', ')']
    >>> parse_function_call_and_arguments("foo", "foo()")
    ['foo(', '', ')']
    >>> parse_function_call_and_arguments("foo", "foo(123)")
    ['foo(', '123', ')']
    >>> parse_function_call_and_arguments("foo", 'foo("foo")')
    ['foo(', '"foo"', ')']
    >>> parse_function_call_and_arguments("strprintf", 'strprintf("%s (%d)", std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>,wchar_t>().to_bytes(buf), err);')
    ['strprintf(', '"%s (%d)",', ' std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>,wchar_t>().to_bytes(buf),', ' err', ')']
    >>> parse_function_call_and_arguments("strprintf", 'strprintf("%s (%d)", foo<wchar_t>().to_bytes(buf), err);')
    ['strprintf(', '"%s (%d)",', ' foo<wchar_t>().to_bytes(buf),', ' err', ')']
    >>> parse_function_call_and_arguments("strprintf", 'strprintf("%s (%d)", foo().to_bytes(buf), err);')
    ['strprintf(', '"%s (%d)",', ' foo().to_bytes(buf),', ' err', ')']
    >>> parse_function_call_and_arguments("strprintf", 'strprintf("%s (%d)", foo << 1, err);')
    ['strprintf(', '"%s (%d)",', ' foo << 1,', ' err', ')']
    >>> parse_function_call_and_arguments("strprintf", 'strprintf("%s (%d)", foo<bar>() >> 1, err);')
    ['strprintf(', '"%s (%d)",', ' foo<bar>() >> 1,', ' err', ')']
    >>> parse_function_call_and_arguments("strprintf", 'strprintf("%s (%d)", foo < 1 ? bar : foobar, err);')
    ['strprintf(', '"%s (%d)",', ' foo < 1 ? bar : foobar,', ' err', ')']
    >>> parse_function_call_and_arguments("strprintf", 'strprintf("%s (%d)", foo < 1, err);')
    ['strprintf(', '"%s (%d)",', ' foo < 1,', ' err', ')']
    >>> parse_function_call_and_arguments("strprintf", 'strprintf("%s (%d)", foo > 1 ? bar : foobar, err);')
    ['strprintf(', '"%s (%d)",', ' foo > 1 ? bar : foobar,', ' err', ')']
    >>> parse_function_call_and_arguments("strprintf", 'strprintf("%s (%d)", foo > 1, err);')
    ['strprintf(', '"%s (%d)",', ' foo > 1,', ' err', ')']
    >>> parse_function_call_and_arguments("strprintf", 'strprintf("%s (%d)", foo <= 1, err);')
    ['strprintf(', '"%s (%d)",', ' foo <= 1,', ' err', ')']
    >>> parse_function_call_and_arguments("strprintf", 'strprintf("%s (%d)", foo <= bar<1, 2>(1, 2), err);')
    ['strprintf(', '"%s (%d)",', ' foo <= bar<1, 2>(1, 2),', ' err', ')']
    >>> parse_function_call_and_arguments("strprintf", 'strprintf("%s (%d)", foo>foo<1,2>(1,2)?bar:foobar,err)');
    ['strprintf(', '"%s (%d)",', ' foo>foo<1,2>(1,2)?bar:foobar,', 'err', ')']
    >>> parse_function_call_and_arguments("strprintf", 'strprintf("%s (%d)", foo>foo<1,2>(1,2),err)');
    ['strprintf(', '"%s (%d)",', ' foo>foo<1,2>(1,2),', 'err', ')']
    """
    assert type(function_name) is str and type(function_call) is str and function_name
    remaining = normalize(escape(function_call))
    expected_function_call = "{}(".format(function_name)
    assert remaining.startswith(expected_function_call)
    parts = [expected_function_call]
    remaining = remaining[len(expected_function_call):]
    open_parentheses = 1
    open_template_arguments = 0
    in_string = False
    parts.append("")
    for i, char in enumerate(remaining):
        parts.append(parts.pop() + char)
        if char == "\"":
            in_string = not in_string
            continue
        if in_string:
            continue
        if char == "(":
            open_parentheses += 1
            continue
        if char == ")":
            open_parentheses -= 1
        if open_parentheses > 1:
            continue
        if open_parentheses == 0:
            parts.append(parts.pop()[:-1])
            parts.append(char)
            break
        prev_char = remaining[i - 1] if i - 1 >= 0 else None
        next_char = remaining[i + 1] if i + 1 <= len(remaining) - 1 else None
        if char == "<" and next_char not in [" ", "<", "="] and prev_char not in [" ", "<"]:
            open_template_arguments += 1
            continue
        if char == ">" and next_char not in [" ", ">", "="] and prev_char not in [" ", ">"] and open_template_arguments > 0:
            open_template_arguments -= 1
        if open_template_arguments > 0:
            continue
        if char == ",":
            parts.append("")
    return parts


def parse_string_content(argument):
    """Return the text within quotes in string argument.

    >>> parse_string_content('1 "foo %d bar" 2')
    'foo %d bar'
    >>> parse_string_content('1 foobar 2')
    ''
    >>> parse_string_content('1 "bar" 2')
    'bar'
    >>> parse_string_content('1 "foo" 2 "bar" 3')
    'foobar'
    >>> parse_string_content('1 "foo" 2 " " "bar" 3')
    'foo bar'
    >>> parse_string_content('""')
    ''
    >>> parse_string_content('')
    ''
    >>> parse_string_content('1 2 3')
    ''
    """
    assert type(argument) is str
    string_content = ""
    in_string = False
    for char in normalize(escape(argument)):
        if char == "\"":
            in_string = not in_string
        elif in_string:
            string_content += char
    return string_content


def count_format_specifiers(format_string):
    """Return the number of format specifiers in string format_string.

    >>> count_format_specifiers("foo bar foo")
    0
    >>> count_format_specifiers("foo %d bar foo")
    1
    >>> count_format_specifiers("foo %d bar %i foo")
    2
    >>> count_format_specifiers("foo %d bar %i foo %% foo")
    2
    >>> count_format_specifiers("foo %d bar %i foo %% foo %d foo")
    3
    >>> count_format_specifiers("foo %d bar %i foo %% foo %*d foo")
    4
    """
    assert type(format_string) is str
    format_string = format_string.replace('%%', 'X')
    n = 0
    in_specifier = False
    for i, char in enumerate(format_string):
        if char == "%":
            in_specifier = True
            n += 1
        elif char in "aAcdeEfFgGinopsuxX":
            in_specifier = False
        elif in_specifier and char == "*":
            n += 1
    return n


def main():
    parser = argparse.ArgumentParser(description="This program checks that the number of arguments passed "
                                     "to a variadic format string function matches the number of format "
                                     "specifiers in the format string.")
    parser.add_argument("--skip-arguments", type=int, help="number of arguments before the format string "
                        "argument (e.g. 1 in the case of fprintf)", default=0)
    parser.add_argument("function_name", help="function name (e.g. fprintf)", default=None)
    parser.add_argument("file", nargs="*", help="C++ source code file (e.g. foo.cpp)")
    args = parser.parse_args()
    exit_code = 0
    for filename in args.file:
        with open(filename, "r", encoding="utf-8") as f:
            for function_call_str in parse_function_calls(args.function_name, f.read()):
                parts = parse_function_call_and_arguments(args.function_name, function_call_str)
                relevant_function_call_str = unescape("".join(parts))[:512]
                if (f.name, relevant_function_call_str) in FALSE_POSITIVES:
                    continue
                if len(parts) < 3 + args.skip_arguments:
                    exit_code = 1
                    print("{}: Could not parse function call string \"{}(...)\": {}".format(f.name, args.function_name, relevant_function_call_str))
                    continue
                argument_count = len(parts) - 3 - args.skip_arguments
                format_str = parse_string_content(parts[1 + args.skip_arguments])
                format_specifier_count = count_format_specifiers(format_str)
                if format_specifier_count != argument_count:
                    exit_code = 1
                    print("{}: Expected {} argument(s) after format string but found {} argument(s): {}".format(f.name, format_specifier_count, argument_count, relevant_function_call_str))
                    continue
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
