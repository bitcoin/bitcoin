**Testing**
--------------

Testing and code review is the bottleneck for development; we get more pull requests than we can review and test on short notice. Please be patient and help out by testing other people's pull requests, and remember this is a security-critical project where any mistake might cost people lots of money.

**Automated Testing**

Developers are strongly encouraged to write [**unit tests**](/src/test/README.md) for new code, and to submit new unit tests for old code. Unit tests can be compiled and run (assuming they weren't disabled in configure) with: make check. Further details on running and extending unit tests can be found in /src/test/README.md.

There are also [regression and integration tests](https://github.com/bitcoin/bitcoin/blob/master/test), written in Python. These tests can be run (if the test dependencies are installed) with: test/functional/test_runner.py

The CI (Continuous Integration) systems make sure that every pull request is built for Windows, Linux, and macOS, and that unit/sanity tests are run automatically.

**Manual Quality Assurance (QA) Testing**

Changes should be tested by somebody other than the developer who wrote the code. This is especially important for large or high-risk changes. It is useful to add a test plan to the pull request description if testing the changes is not straightforward.
