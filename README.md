Bitcoin Core Integration/Staging Tree

Overview
Bitcoin Core is a full implementation of the Bitcoin protocol. It connects to the Bitcoin peer-to-peer network to download and fully validate blocks and transactions. It includes a wallet for managing your Bitcoin and a graphical user interface (GUI) for ease of use.

For a pre-built, binary version of Bitcoin Core, visit the download page.

Documentation
Detailed information about Bitcoin Core can be found in the doc folder.

License
Bitcoin Core is released under the MIT license. For more information, see the COPYING file or visit opensource.org/licenses/MIT.

Development Process
The master branch is regularly built and tested, although it is not guaranteed to be completely stable. Stable releases are indicated by tags created from release branches.

The GUI development is managed in a separate repository: bitcoin-core/gui. The master branch is kept in sync across all monotree repositories. Please do not fork this repository unless for development purposes.

For contribution guidelines, see CONTRIBUTING.md. Helpful information for developers is available in doc/developer-notes.md.

Testing
Testing and code review are crucial for the security and stability of Bitcoin Core. We encourage you to test other people's pull requests and be patient as your own pull requests undergo review.

Automated Testing
Developers are encouraged to write unit tests for new code and submit tests for existing code. To compile and run unit tests, use make check. For more details, see /src/test/README.md.

We also have regression and integration tests written in Python. Run these tests using test/functional/test_runner.py after installing the test dependencies.

Our CI (Continuous Integration) systems build every pull request for Windows, Linux, and macOS, and run unit/sanity tests automatically.

Manual Quality Assurance (QA) Testing
Code changes should be tested by someone other than the original developer, especially for large or high-risk changes. Including a test plan in the pull request description is helpful for others reviewing your changes.

Translations
Translation updates and new translations can be submitted via Bitcoin Core's Transifex page. Translations are periodically pulled from Transifex and merged into the Git repository. For more details, see the translation process documentation.

Important: Translation changes should not be submitted as GitHub pull requests because they will be overwritten by the next pull from Transifex.
