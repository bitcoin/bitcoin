# Quick Reference: Ownership and Policy Documentation

**Repository**: kushmanmb-org/bitcoin  
**Owner**: kushmanmb  
**ENS**: kushmanmb.eth, Kushmanmb.base.eth, yaketh.eth

---

## 📚 Core Documents

| Document | Purpose | Link |
|----------|---------|------|
| **Ownership** | Who owns this repository | [OWNERSHIP.md](OWNERSHIP.md) |
| **Policy** | How the repository is governed | [POLICY.md](POLICY.md) |
| **Rulesets** | What rules are enforced | [RULESETS.md](RULESETS.md) |
| **Code Owners** | Who reviews code | [.github/CODEOWNERS](.github/CODEOWNERS) |
| **Documentation Index** | Complete documentation map | [DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md) |
| **Rulesets Setup** | How to configure rulesets | [RULESETS_SETUP_GUIDE.md](RULESETS_SETUP_GUIDE.md) |

---

## 🔑 Key Information

### Repository Owner
- **GitHub**: @kushmanmb
- **Organization**: kushmanmb-org
- **ENS Identities**: kushmanmb.eth, Kushmanmb.base.eth, yaketh.eth

### License
- **Type**: MIT License
- **File**: [COPYING](COPYING)

### Contribution
- **Guidelines**: [CONTRIBUTING.md](CONTRIBUTING.md)
- **Policy**: [POLICY.md](POLICY.md)
- **Security**: [SECURITY.md](SECURITY.md)

---

## 🚀 Quick Actions

### For Contributors

**Before Contributing**:
1. Read [CONTRIBUTING.md](CONTRIBUTING.md)
2. Review [POLICY.md](POLICY.md)
3. Check [SECURITY.md](SECURITY.md)

**Contribution Flow**:
```bash
# 1. Fork and clone
git clone https://github.com/YOUR_USERNAME/bitcoin.git

# 2. Create feature branch
git checkout -b feature/your-feature

# 3. Make changes and test
# ... make your changes ...
make test

# 4. Commit and push
git commit -am "Your descriptive message"
git push origin feature/your-feature

# 5. Create pull request via GitHub UI
```

### For Maintainers

**Review Checklist**:
- [ ] Code quality meets standards
- [ ] Tests included and passing
- [ ] Documentation updated
- [ ] Security review completed
- [ ] CODEOWNERS approval obtained
- [ ] CI/CD passing

**Merge Process**:
1. Approve pull request
2. Ensure CI passes
3. Squash and merge (or merge commit)
4. Delete feature branch

---

## 🔒 Security

### Reporting Vulnerabilities
**DO NOT** create public issues for security vulnerabilities.

**Contact**: security@bitcoincore.org or repository owner

**Process**: See [SECURITY.md](SECURITY.md)

### Security Documents
- [SECURITY.md](SECURITY.md) - Security policy
- [SECURITY_PRACTICES.md](SECURITY_PRACTICES.md) - Best practices
- [SELF_HOSTED_RUNNER_SETUP.md](SELF_HOSTED_RUNNER_SETUP.md) - Runner security

---

## ⚙️ Workflows

### Key Workflows

| Workflow | Purpose | Trigger |
|----------|---------|---------|
| **CI** | Build and test | Push, PR |
| **Ownership Announcement** | Publish ownership | Weekly, Manual |
| **CodeQL** | Security scanning | Push, PR |
| **Self-Hosted Runner Setup** | Runner configuration | Manual |
| **Website Deployment** | Deploy website | Push to master |

### Self-Hosted Runners

**Configuration**: [SELF_HOSTED_RUNNER_SETUP.md](SELF_HOSTED_RUNNER_SETUP.md)

**Usage**: Workflows use `vars.USE_SELF_HOSTED` to conditionally use self-hosted runners:
```yaml
runs-on: ${{ vars.USE_SELF_HOSTED == 'true' && 'self-hosted' || 'ubuntu-latest' }}
```

---

## 📋 Rulesets

### Branch Protection

| Branch | Protection Level | Required Approvals |
|--------|-----------------|-------------------|
| `master` | ⚠️ High | 1 (owner) |
| `release/*` | ⚠️ High | 1 (owner) |
| `feature/*` | ℹ️ Low | None |
| `fix/*` | ℹ️ Low | None |

### Tag Protection

| Pattern | Protected | Who Can Create |
|---------|-----------|----------------|
| `v*.*.*` | ✅ Yes | Owner only |

**Details**: See [RULESETS.md](RULESETS.md)  
**Setup Guide**: See [RULESETS_SETUP_GUIDE.md](RULESETS_SETUP_GUIDE.md)

---

## 🤝 Code Ownership

All code is owned by repository owner by default.

**Specific Areas**:
- **Security files**: `@kushmanmb`
- **Workflows**: `@kushmanmb`
- **Build system**: `@kushmanmb`
- **Core source**: `@kushmanmb`

**Full Details**: [.github/CODEOWNERS](.github/CODEOWNERS)

---

## 📞 Contact

### Questions?
- **Public Questions**: Create a [GitHub Discussion](../../discussions)
- **Bug Reports**: Create a [GitHub Issue](../../issues)
- **Security**: Email security@bitcoincore.org
- **Owner**: Via GitHub or repository contact

### Response Times
- **Critical Security**: 24-48 hours
- **Ownership Disputes**: 14 days
- **General Questions**: Best effort, ~7 days

---

## ✅ Verification

### Verify Ownership

**Via GitHub API**:
```bash
curl https://api.github.com/repos/kushmanmb-org/bitcoin | jq '.owner.login'
```

**Via Git History**:
```bash
git log --all --format='%an <%ae>' | sort -u
```

**Via ENS** (requires web3):
```bash
# Resolve kushmanmb.eth
npx @ensdomains/ensjs kushmanmb.eth
```

**Via Workflow Announcements**:
- Location: `data/ownership/`
- Frequency: Weekly
- Workflow: `.github/workflows/bitcoin-ownership-announcement.yml`

---

## 📊 Document Structure

```
kushmanmb-org/bitcoin/
├── OWNERSHIP.md                 # Ownership declaration
├── POLICY.md                    # Repository policies
├── RULESETS.md                  # Ruleset reference
├── RULESETS_SETUP_GUIDE.md      # Setup instructions
├── DOCUMENTATION_INDEX.md       # Complete documentation map
├── QUICKREF.md                  # This document
├── README.md                    # Project overview
├── CONTRIBUTING.md              # Contribution guide
├── SECURITY.md                  # Security policy
├── SECURITY_PRACTICES.md        # Security best practices
├── SELF_HOSTED_RUNNER_SETUP.md  # Runner setup
├── .github/
│   ├── CODEOWNERS              # Code ownership
│   └── workflows/              # CI/CD workflows
└── data/
    └── ownership/              # Ownership announcements
```

---

## 🔄 Regular Tasks

### Weekly
- [ ] Review pull requests
- [ ] Monitor CI/CD
- [ ] Check ownership announcements

### Monthly
- [ ] Update dependencies
- [ ] Review security advisories
- [ ] Check audit logs

### Quarterly
- [ ] Security audit
- [ ] Policy review
- [ ] Ruleset review
- [ ] Documentation update

---

## 🎯 Quick Commands

### Repository Management

```bash
# Clone repository
git clone https://github.com/kushmanmb-org/bitcoin.git

# Check status
git status

# View recent commits
git log --oneline --max-count=10

# Check remote
git remote -v
```

### Workflow Management

```bash
# List workflows
gh workflow list

# Run workflow
gh workflow run bitcoin-ownership-announcement.yml

# View workflow runs
gh run list --workflow=ci.yml

# View logs
gh run view [RUN_ID] --log
```

### Ruleset Management

```bash
# List rulesets (requires GitHub API)
gh api repos/kushmanmb-org/bitcoin/rulesets | jq '.[] | {name: .name, enforcement: .enforcement}'

# Export ruleset
gh api repos/kushmanmb-org/bitcoin/rulesets/[ID] > ruleset-backup.json
```

---

## 📖 Further Reading

### Essential Reading
1. [OWNERSHIP.md](OWNERSHIP.md) - Understand who owns this repository
2. [POLICY.md](POLICY.md) - Understand repository governance
3. [CONTRIBUTING.md](CONTRIBUTING.md) - Learn how to contribute

### Technical Documentation
- [RULESETS.md](RULESETS.md) - Branch protection and rules
- [RULESETS_SETUP_GUIDE.md](RULESETS_SETUP_GUIDE.md) - Configuration guide
- [SELF_HOSTED_RUNNER_SETUP.md](SELF_HOSTED_RUNNER_SETUP.md) - Runner setup

### Security Documentation
- [SECURITY.md](SECURITY.md) - Security policy
- [SECURITY_PRACTICES.md](SECURITY_PRACTICES.md) - Best practices
- [.github/CODEOWNERS](.github/CODEOWNERS) - Code review requirements

### Complete Index
- [DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md) - All documentation links

---

## 🏷️ Tags and Labels

### Issue Labels
- `announcement` - Ownership/policy announcements
- `bitcoin` - Bitcoin-related
- `ownership` - Ownership matters
- `automated` - Auto-generated content
- `security` - Security-related
- `documentation` - Documentation updates

### Workflow Tags
- `ownership-announcement` - Ownership workflows
- `ci` - Continuous integration
- `security-scan` - Security scanning
- `self-hosted` - Self-hosted runner jobs

---

## ⚡ Pro Tips

### For Contributors
1. Always create a feature branch
2. Write clear commit messages
3. Include tests with code changes
4. Update documentation
5. Sign your commits (recommended)

### For Reviewers
1. Check CODEOWNERS for required reviewers
2. Verify all CI checks pass
3. Review security implications
4. Test changes locally if possible
5. Provide constructive feedback

### For Maintainers
1. Keep POLICY.md updated
2. Review rulesets quarterly
3. Monitor audit logs weekly
4. Update dependencies monthly
5. Communicate changes clearly

---

## 🆘 Common Issues

### "Push to master rejected"
**Solution**: Create a pull request instead. Direct pushes to master are blocked.

### "PR cannot be merged"
**Solution**: 
1. Ensure CI passes
2. Get required approval
3. Update branch to latest master
4. Resolve any conflicts

### "Workflow permission denied"
**Solution**: Check workflow has required permissions in YAML file.

### "Cannot create tag"
**Solution**: Tags can only be created by repository owner. Contact owner.

---

**Version**: 1.0  
**Last Updated**: 2026-02-17  
**Maintained By**: kushmanmb

For questions, create a GitHub Discussion or Issue.
