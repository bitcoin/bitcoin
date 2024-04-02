// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

pub mod doc;
pub mod includes;
pub mod locale;
pub mod spelling;
pub mod std_filesystem;
pub mod subtree;
pub mod whitespace;

use crate::{run_all_python_linters, LintFn};

pub struct Linter {
    pub description: &'static str,
    pub name: &'static str,
    pub lint_fn: LintFn,
}

pub fn get_linter_list() -> Vec<&'static Linter> {
    vec![
        &Linter {
            description: "Check that all command line arguments are documented.",
            name: "doc",
            lint_fn: doc::doc
        },
        &Linter {
            description: "Check for duplicate includes, includes of .cpp files, new boost includes, and quote syntax includes.",
            name: "includes",
            lint_fn: includes::includes
        },
        &Linter {
            description: "Check that no symbol from bitcoin-config.h is used without the header being included",
            name: "includes_build_config",
            lint_fn: includes::includes_build_config
        },
        &Linter {
            description: "Check that header files have include guards",
            name: "include_guards",
            lint_fn: includes::include_guards
        },
        &Linter {
            description: "Check that locale dependent functions are not used",
            name: "locale_dependent",
            lint_fn: locale::locale_dependent
        },
        &Linter {
            description: "Print warnings for spelling errors. (These will not cause lint check to fail)",
            name: "spelling",
            lint_fn: spelling::spelling
        },
        &Linter {
            description: "Check that subtrees are pure subtrees",
            name: "subtree",
            lint_fn: subtree::subtree},
        &Linter {
            description: "Check that std::filesystem is not used directly",
            name: "std_filesystem",
            lint_fn: std_filesystem::std_filesystem
        },
        &Linter {
            description: "Check that tabs are not used as whitespace",
            name: "tabs_whitespace",
            lint_fn: whitespace::tabs_whitespace
        },
        &Linter {
            description: "Check for trailing whitespace",
            name: "trailing_whitespace",
            lint_fn: whitespace::trailing_whitespace
        },
        &Linter {
            description: "Run all the linters of the form: test/lint/lint-*.py",
            name: "all_python_linters",
            lint_fn: run_all_python_linters
        },
    ]
}
