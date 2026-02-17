Bitcoin Core integration/staging tree
=====================================

https://bitcoincore.org

For an immediately usable, binary version of the Bitcoin Core software, see
https://bitcoincore.org/en/download/.

## Repository Ownership

**Owner**: kushmanmb  
**Creator**: kushmanmb  
**ENS**: kushmanmb.eth, Kushmanmb.base.eth, yaketh.eth  
**Organization**: kushmanmb-org

For complete ownership documentation, see [OWNERSHIP.md](OWNERSHIP.md).

## What is Bitcoin Core?

Bitcoin Core connects to the Bitcoin peer-to-peer network to download and fully
validate blocks and transactions. It also includes a wallet and graphical user
interface, which can be optionally built.

Further information about Bitcoin Core is available in the [doc folder](/doc).

## Repository Policies

This repository operates under documented policies and governance:

- **[POLICY.md](POLICY.md)** - Repository governance, contribution policies, and guidelines
- **[OWNERSHIP.md](OWNERSHIP.md)** - Creator and ownership documentation
- **[RULESETS.md](RULESETS.md)** - GitHub repository rulesets and branch protection
- **[CONTRIBUTING.md](CONTRIBUTING.md)** - How to contribute to this project
- **[.github/CODEOWNERS](.github/CODEOWNERS)** - Code ownership and review requirements

## License

Bitcoin Core is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/license/MIT.

Development Process
-------------------

The `master` branch is regularly built (see `doc/build-*.md` for instructions) and tested, but it is not guaranteed to be
completely stable. [Tags](https://github.com/bitcoin/bitcoin/tags) are created
regularly from release branches to indicate new official, stable release versions of Bitcoin Core.

The https://github.com/bitcoin-core/gui repository is used exclusively for the
development of the GUI. Its master branch is identical in all monotree
repositories. Release branches and tags do not exist, so please do not fork
that repository unless it is for development reasons.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md)
and useful hints for developers can be found in [doc/developer-notes.md](doc/developer-notes.md).

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write [unit tests](src/test/README.md) for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run
(assuming they weren't disabled during the generation of the build system) with: `ctest`. Further details on running
and extending unit tests can be found in [/src/test/README.md](/src/test/README.md).

There are also [regression and integration tests](/test), written
in Python.
These tests can be run (if the [test dependencies](/test) are installed) with: `build/test/functional/test_runner.py`
(assuming `build` is your build directory).

The CI (Continuous Integration) systems make sure that every pull request is tested on Windows, Linux, and macOS.
The CI must pass on all commits before merge to avoid unrelated CI failures on new pull requests.

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.

Translations
------------

Changes to translations as well as new translations can be submitted to
[Bitcoin Core's Transifex page](https://explore.transifex.com/bitcoin/bitcoin/).

Translations are periodically pulled from Transifex and merged into the git repository. See the
[translation process](doc/translation_process.md) for details on how this works.

**Important**: We do not accept translation changes as GitHub pull requests because the next
pull from Transifex would automatically overwrite them again.

Security and Safe Development Practices
----------------------------------------

Bitcoin Core is a security-critical project. Please review our security policies and best practices:

- **[SECURITY.md](SECURITY.md)** - Security policy, supported versions, and how to report vulnerabilities
- **[SECURITY_PRACTICES.md](SECURITY_PRACTICES.md)** - Comprehensive security and privacy practices for development
- **[SELF_HOSTED_RUNNER_SETUP.md](SELF_HOSTED_RUNNER_SETUP.md)** - Self-hosted GitHub Actions runner configuration with security best practices
- **[ENS_CONFIGURATION.md](ENS_CONFIGURATION.md)** - kushmanmb.eth ENS domain configuration and blockchain integration

### Key Security Reminders

- **Never commit private keys, wallet files, or sensitive data** - Review the [.gitignore](.gitignore) file for protected patterns
- **Use environment variables for secrets** - Never hardcode API keys, tokens, or credentials
- **Keep dependencies updated** - Regularly check for security updates
- **Review code carefully** - This is a security-critical project where mistakes can be costly
- **Report vulnerabilities privately** - Email security@bitcoincore.org for security issues

### ENS Integration

This project uses the **kushmanmb.eth** ENS (Ethereum Name Service) domain for blockchain identity:
- Provides decentralized identity on Ethereum
- Links to website and other resources
- Used in GitHub Actions workflows for Etherscan API integration
- See [ENS_CONFIGURATION.md](ENS_CONFIGURATION.md) for details

### Coinbase Developer Platform (CDP) API Integration

This repository includes tools for interacting with the Coinbase Developer Platform API:
- **JWT-based authentication** with ES256 algorithm
- **EVM blockchain data queries** (token balances, transactions, blocks)
- **Multi-network support** (Ethereum, Base, Polygon, Optimism, Arbitrum, etc.)
- **Secure credential management** via environment variables

**Quick Start:**
```bash
export KEY_ID="your-key-id"
export KEY_SECRET="your-key-secret"
export REQUEST_PATH="/platform/v2/evm/token-balances/base-sepolia/0x..."
node contrib/devtools/fetch-cdp-api.js
```

**Documentation:**
- Quick Start Guide: [CDP_API_QUICKSTART.md](CDP_API_QUICKSTART.md)
- Full Documentation: [contrib/devtools/CDP_API_README.md](contrib/devtools/CDP_API_README.md)
- Demo Script: `./contrib/devtools/demo-cdp-api.sh`

### Safe Git Practices

When contributing to Bitcoin Core:

1. **Review changes before committing**: Use `git diff` to verify you're not committing sensitive data
2. **Check your gitignore**: Ensure local configuration files are properly excluded
3. **Use meaningful commit messages**: Clearly describe what and why you're changing
4. **Keep commits focused**: Make small, atomic commits that address one concern at a time
5. **Test before pushing**: Run tests and verify your changes work as expected
6. **Sign your commits**: Consider using GPG to sign commits for authenticity
7. **Follow the PR workflow**: Never push directly to master; always use pull requests

For detailed Git workflow, branch management, and contribution process, see [GIT_WORKFLOW_GUIDE.md](GIT_WORKFLOW_GUIDE.md).

For detailed authentication and publishing guidance, especially for Maven/GitHub Packages, see [SECURITY_PRACTICES.md](SECURITY_PRACTICES.md).
