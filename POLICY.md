# Repository Policy

## Overview

This document defines the governance, contribution policies, and operational guidelines for the kushmanmb-org/bitcoin repository.

## Repository Ownership

**Owner**: kushmanmb  
**ENS Identifiers**:
- `kushmanmb.eth` (Ethereum Mainnet)
- `Kushmanmb.base.eth` (Base Network)
- `yaketh.eth`

**Creator**: kushmanmb  
**Repository**: kushmanmb-org/bitcoin  
**License**: MIT License (see [COPYING](COPYING))

## Governance Model

### Repository Structure

This repository follows a **single-owner governance model** with community contributions:

1. **Owner/Maintainer**: Final decision-making authority on all changes
2. **Contributors**: Community members who submit pull requests
3. **Reviewers**: Trusted contributors with review access
4. **Automated Systems**: GitHub Actions workflows for CI/CD and automation

### Decision Making

- **Critical Changes**: Require owner approval (security, architecture, policy)
- **Feature Additions**: Reviewed by maintainer, community input welcomed
- **Bug Fixes**: Fast-tracked with appropriate testing
- **Documentation**: Community contributions encouraged, owner approval required

## Contribution Policy

### General Guidelines

1. **Read First**: Review [CONTRIBUTING.md](CONTRIBUTING.md) before contributing
2. **Clear Purpose**: All contributions must have clear rationale and benefit
3. **Code Quality**: Follow existing code style and conventions
4. **Testing**: Include appropriate tests for all code changes
5. **Documentation**: Update relevant documentation with your changes
6. **Security**: Never commit secrets, private keys, or sensitive data

### Contribution Process

1. **Fork** the repository
2. **Create** a feature branch from `master`
3. **Make** your changes with clear, focused commits
4. **Test** thoroughly (unit tests, integration tests, manual testing)
5. **Submit** a pull request with detailed description
6. **Respond** to review feedback promptly
7. **Iterate** until approved and merged

### Code Review Requirements

- **All Changes**: Require at least one review
- **Security Changes**: Require owner review
- **Breaking Changes**: Require discussion and owner approval
- **Documentation Only**: Can be fast-tracked with maintainer approval

### Pull Request Guidelines

- **Title**: Clear, concise description of changes
- **Description**: Detailed explanation of what, why, and how
- **Scope**: Keep PRs focused and minimal
- **Tests**: Include test coverage for changes
- **Documentation**: Update docs when behavior changes
- **Commits**: Use meaningful commit messages

### What We Accept

✅ **Welcomed Contributions**:
- Bug fixes with reproduction steps
- Test improvements and new test coverage
- Performance optimizations with benchmarks
- Security vulnerability fixes
- Documentation improvements
- Feature enhancements (after discussion)

❌ **Not Accepted**:
- Large refactoring without clear benefit
- Style-only changes without substantial improvement
- Features that conflict with project goals
- Changes that break existing functionality
- Contributions with inadequate testing

## Branch Policy

### Branch Structure

- **`master`**: Main development branch, should always be stable
- **Feature Branches**: `feature/description` for new features
- **Bug Fix Branches**: `fix/description` for bug fixes
- **Release Branches**: `release/version` for release preparation

### Branch Protection

The `master` branch is protected with the following rules:

1. **Require Pull Requests**: Direct pushes not allowed
2. **Require Reviews**: At least 1 approval required
3. **Status Checks**: CI must pass before merge
4. **Up-to-date**: Branch must be current with master
5. **Signed Commits**: Encouraged but not required

## Security Policy

### Reporting Vulnerabilities

**DO NOT** create public issues for security vulnerabilities.

Instead:
1. Email: security@bitcoincore.org (or repository owner)
2. Provide detailed description and reproduction steps
3. Wait for confirmation before public disclosure
4. Follow coordinated disclosure timeline

See [SECURITY.md](SECURITY.md) for complete security policy.

### Security Requirements

- **No Secrets in Code**: Use GitHub Secrets for sensitive data
- **Dependency Security**: Regular security updates required
- **Code Scanning**: CodeQL and security scanners enabled
- **Safe Practices**: Follow [SECURITY_PRACTICES.md](SECURITY_PRACTICES.md)

## Workflow and Automation Policy

### GitHub Actions

All workflows must:
1. Follow security best practices
2. Use minimal necessary permissions
3. Document their purpose clearly
4. Support both GitHub-hosted and self-hosted runners
5. Include ownership attribution in headers

### Self-Hosted Runners

- Configuration: See [SELF_HOSTED_RUNNER_SETUP.md](SELF_HOSTED_RUNNER_SETUP.md)
- Security: Isolated environments, minimal permissions
- Management: Owner-controlled, documented processes
- Fallback: GitHub-hosted runners available as backup

### Workflow Management

- **Owner Control**: Only owner can modify critical workflows
- **Review Required**: Changes to workflows require review
- **Testing**: Test workflow changes in feature branches
- **Documentation**: Keep workflow documentation current

## Code Ownership

Code ownership is defined in [CODEOWNERS](.github/CODEOWNERS) file.

### Ownership Rules

1. **Default**: All files owned by repository owner
2. **Specific Paths**: May have designated owners
3. **Review Requirements**: Owners must approve changes to their areas
4. **Documentation**: Ownership documented in CODEOWNERS

## License Policy

### Repository License

This repository is licensed under the **MIT License**.

- See [COPYING](COPYING) for full license text
- All contributions must be compatible with MIT License
- Contributors retain copyright, license to repository

### Dependency Licenses

- **Allowed**: MIT, BSD, Apache 2.0, ISC
- **Case-by-Case**: Other permissive licenses
- **Not Allowed**: GPL, AGPL (license compatibility issues)

### Third-Party Code

When incorporating third-party code:
1. Verify license compatibility
2. Maintain license headers
3. Document in dependencies.md
4. Credit original authors

## Communication Policy

### Channels

- **GitHub Issues**: Bug reports, feature requests
- **Pull Requests**: Code contributions and discussions
- **Discussions**: General questions and community discussion
- **Email**: Security issues, private matters

### Code of Conduct

All participants must:
1. **Be Respectful**: Professional, courteous communication
2. **Be Constructive**: Focus on improving the project
3. **Be Collaborative**: Work together, help others
4. **Be Patient**: Recognize volunteer nature of project

### Unacceptable Behavior

- Harassment or discrimination
- Trolling or inflammatory comments
- Spam or off-topic content
- Publishing others' private information
- Dishonest or malicious conduct

## Maintenance Policy

### Update Frequency

- **Security Updates**: Immediate (as vulnerabilities discovered)
- **Dependency Updates**: Monthly review cycle
- **Feature Releases**: As needed, typically quarterly
- **Documentation**: Continuous updates

### Support

- **Current Release**: Full support
- **Previous Release**: Security fixes only
- **Older Releases**: Unsupported, upgrade recommended

### Deprecation

When deprecating features:
1. Announce in release notes
2. Provide migration guide
3. Maintain for at least one major version
4. Remove in subsequent major version

## Rulesets

GitHub repository rulesets enforce these policies:

### Branch Rulesets

1. **Master Branch Protection**
   - Require pull request reviews
   - Require status checks to pass
   - Require branches to be up to date
   - Require signed commits (recommended)

2. **Tag Protection**
   - Only owner can create/delete tags
   - Signed tags required for releases

### Workflow Rulesets

1. **Workflow Files**: Require owner review for changes
2. **Actions**: Only approved actions allowed
3. **Secrets**: Strict access controls

## Compliance

### Legal Compliance

This project complies with:
- Open source license terms (MIT)
- Export control regulations (where applicable)
- GitHub Terms of Service
- Applicable laws and regulations

### Audit Trail

- All changes tracked in git history
- Pull requests preserve discussion
- Releases tagged and documented
- Security incidents documented privately

## Policy Updates

This policy may be updated by the repository owner.

### Update Process

1. **Propose**: Create pull request with policy changes
2. **Discuss**: Allow community feedback period
3. **Approve**: Owner final approval
4. **Communicate**: Announce policy changes
5. **Effective Date**: Typically immediate unless specified

### Version History

- **v1.0** (2026-02-17): Initial policy document

## Contact

### Repository Owner

- **GitHub**: @kushmanmb (via repository owner)
- **ENS**: kushmanmb.eth
- **Email**: Via GitHub or repository contact

### Questions

For policy questions or clarifications:
1. Create a GitHub Discussion
2. Reference this policy document
3. Wait for owner or maintainer response

---

**Last Updated**: 2026-02-17  
**Effective Date**: 2026-02-17  
**Owner**: kushmanmb
