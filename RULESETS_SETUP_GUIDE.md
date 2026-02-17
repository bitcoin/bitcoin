# GitHub Rulesets Configuration Guide

## Overview

This guide provides step-by-step instructions for configuring GitHub repository rulesets as documented in [RULESETS.md](RULESETS.md).

**Repository**: kushmanmb-org/bitcoin  
**Owner**: kushmanmb  
**Purpose**: Enforce branch protection, workflow security, and contribution policies

---

## Prerequisites

### Required Access

- **Repository Admin Access**: Required to create/modify rulesets
- **Organization Owner** (if applicable): For organization-level rulesets
- **Two-Factor Authentication**: Enabled and configured

### Knowledge Requirements

- Familiarity with GitHub repository settings
- Understanding of git branches and workflows
- Knowledge of pull request processes

---

## Quick Start

### 1. Access Repository Settings

```
https://github.com/kushmanmb-org/bitcoin/settings
```

Or navigate:
```
Repository → Settings → Rules → Rulesets
```

### 2. Create First Ruleset

1. Click **"New branch ruleset"** or **"New tag ruleset"**
2. Follow the configuration steps below
3. Save and activate

---

## Ruleset Configurations

### Configuration 1: Master Branch Protection

**Purpose**: Protect the master branch from direct pushes and ensure code quality.

#### Step-by-Step Configuration

1. **Create Ruleset**
   - Go to: `Settings → Rules → Rulesets`
   - Click: `New branch ruleset`
   - Name: `master-branch-protection`

2. **Target Branches**
   - Enforcement status: `Active`
   - Target: `Include by pattern`
   - Pattern: `master`

3. **Bypass List**
   - Add: Repository administrator (yourself)
   - Reason: Emergency hotfixes only

4. **Rules - Pull Requests**
   - ✅ Enable: `Require a pull request before merging`
   - Required approvals: `1`
   - ✅ Dismiss stale reviews on push
   - ✅ Require review from code owners
   - ✅ Require approval of the most recent reviewable push

5. **Rules - Status Checks**
   - ✅ Enable: `Require status checks to pass`
   - ✅ Require branches to be up to date before merging
   - Add required checks:
     - `ci / ci (ubuntu-latest)`
     - `ci / lint`
     - `CodeQL`

6. **Rules - History**
   - ⬜ Require linear history (Optional - allows merge commits)
   - ⬜ Require signed commits (Recommended but not enforced)

7. **Rules - Protection**
   - ✅ Block force pushes
   - ✅ Block deletions

8. **Save Ruleset**
   - Click: `Create`
   - Verify: Ruleset shows as `Active`

---

### Configuration 2: Release Branch Protection

**Purpose**: Protect release branches from accidental modification.

#### Step-by-Step Configuration

1. **Create Ruleset**
   - Name: `release-branch-protection`
   - Enforcement status: `Active`

2. **Target Branches**
   - Pattern: `release/*`
   - Include all release branches

3. **Rules**
   - ✅ Require pull request (1 approval)
   - ✅ Restrict who can push: Repository admin only
   - ✅ Block force pushes
   - ✅ Block deletions

4. **Save Ruleset**

---

### Configuration 3: Tag Protection

**Purpose**: Protect version tags from modification or deletion.

#### Step-by-Step Configuration

1. **Create Ruleset**
   - Go to: `Settings → Rules → Rulesets`
   - Click: `New tag ruleset`
   - Name: `release-tags-protection`

2. **Target Tags**
   - Enforcement status: `Active`
   - Pattern: `v*.*.*` (semantic versioning)

3. **Rules**
   - ✅ Restrict tag creation: Repository admin only
   - ✅ Restrict tag deletions: Repository admin only
   - ✅ Require signed tags (Optional)

4. **Save Ruleset**

---

### Configuration 4: Development Branch Rules

**Purpose**: Lighter rules for feature branches.

#### Step-by-Step Configuration

1. **Create Ruleset**
   - Name: `development-branches`
   - Enforcement status: `Active`

2. **Target Branches**
   - Patterns: 
     - `feature/*`
     - `fix/*`
     - `docs/*`

3. **Rules**
   - ⬜ No strict requirements (developers can push directly)
   - ✅ Optional: Require status checks (not enforced)
   - ⬜ Allow force pushes (for rebasing)
   - ⬜ Allow deletions (for cleanup)

4. **Save Ruleset**

---

### Configuration 5: Workflow File Protection

**Purpose**: Ensure workflow file changes require owner approval.

#### Step-by-Step Configuration

**Note**: This uses path-based rules within branch rulesets.

1. **Edit Master Branch Ruleset**
   - Go to: `master-branch-protection` ruleset
   - Click: `Edit`

2. **Add Restricted Paths** (if available)
   - Path pattern: `.github/workflows/*`
   - Require additional review: Repository admin

3. **Alternative: Use CODEOWNERS**
   - Already configured in `.github/CODEOWNERS`
   - Automatically requires owner review for workflow changes

4. **Save Changes**

---

## Using GitHub CLI

### Install GitHub CLI

```bash
# macOS
brew install gh

# Linux
curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg | sudo dd of=/usr/share/keyrings/githubcli-archive-keyring.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" | sudo tee /etc/apt/sources.list.d/github-cli.list > /dev/null
sudo apt update
sudo apt install gh

# Windows
winget install --id GitHub.cli
```

### List Existing Rulesets

```bash
gh api repos/kushmanmb-org/bitcoin/rulesets | jq '.[] | {name: .name, enforcement: .enforcement, target: .target}'
```

### Export Ruleset Configuration

```bash
# Export specific ruleset
gh api repos/kushmanmb-org/bitcoin/rulesets/[RULESET_ID] > ruleset-backup.json

# Pretty print
gh api repos/kushmanmb-org/bitcoin/rulesets/[RULESET_ID] | jq '.' > ruleset-backup.json
```

### Create Ruleset via API (Advanced)

```bash
gh api \
  --method POST \
  -H "Accept: application/vnd.github+json" \
  repos/kushmanmb-org/bitcoin/rulesets \
  -f name='example-ruleset' \
  -f target='branch' \
  -f enforcement='active' \
  -F conditions='{"ref_name":{"include":["refs/heads/master"]}}' \
  -F rules='[{"type":"pull_request"}]'
```

---

## Testing Rulesets

### Test 1: Protected Branch Push

**Scenario**: Try to push directly to master

```bash
# Should fail
git checkout master
git commit --allow-empty -m "test"
git push origin master

# Expected: Push rejected
# Message: "remote: error: GH006: Protected branch update failed"
```

### Test 2: Pull Request Requirement

**Scenario**: Create PR and merge without approval

```bash
# Create feature branch and PR
git checkout -b test/ruleset-check
git commit --allow-empty -m "test ruleset"
git push origin test/ruleset-check

# Create PR via GitHub UI or CLI
gh pr create --title "Test Ruleset" --body "Testing branch protection"

# Try to merge without approval
gh pr merge

# Expected: Merge blocked until approved
```

### Test 3: Status Check Requirement

**Scenario**: Try to merge with failing CI

```bash
# Create PR that fails CI
git checkout -b test/failing-ci
# Make breaking change
git commit -am "intentionally failing change"
git push origin test/failing-ci

# Create PR
gh pr create --title "Test CI Check" --body "Testing status checks"

# Try to merge
gh pr merge

# Expected: Merge blocked until CI passes
```

---

## Monitoring and Maintenance

### View Ruleset Audit Log

1. **Access Audit Log**
   ```
   Repository → Settings → Security & analysis → Audit log
   ```

2. **Filter by Ruleset Events**
   - Filter: `action:repository.ruleset.*`
   - View: Creation, modification, deletion events

### Review Bypass Events

1. **Check Recent Bypasses**
   ```bash
   gh api repos/kushmanmb-org/bitcoin/events | jq '.[] | select(.type == "PushEvent") | select(.payload.forced == true)'
   ```

2. **Monitor Protection Violations**
   - Settings → Insights → Network
   - Check for unexpected direct pushes

### Update Rulesets

**Quarterly Review Process**:

1. Review RULESETS.md for current configuration
2. Check audit log for bypass events
3. Assess if rules are too strict or too lenient
4. Test proposed changes on test repository
5. Update production rulesets
6. Document changes in RULESETS.md
7. Announce changes to contributors

---

## Troubleshooting

### Issue: Cannot Create Ruleset

**Error**: "You don't have permission to create rulesets"

**Solutions**:
1. Verify you have admin access to repository
2. Check if organization policies restrict ruleset creation
3. Ensure 2FA is enabled
4. Contact organization owner if needed

### Issue: Ruleset Not Enforcing

**Error**: Can still push to protected branch

**Solutions**:
1. Check ruleset is `Active` (not `Disabled` or `Evaluate`)
2. Verify target pattern matches your branch name
3. Check if you have bypass permissions
4. Wait a few minutes for GitHub to propagate changes
5. Test with a different account without admin rights

### Issue: Status Checks Not Required

**Error**: Can merge PR without status checks

**Solutions**:
1. Verify status check names match exactly (case-sensitive)
2. Ensure status checks are running on PRs
3. Check "Require branches to be up to date" is enabled
4. Verify workflow is triggered for PR events

### Issue: Too Many Bypass Events

**Warning**: Audit log shows frequent bypasses

**Solutions**:
1. Review who has bypass permissions
2. Document all bypass events with reasons
3. Consider revoking bypass from non-essential accounts
4. Implement approval process for emergency bypasses
5. Update RULESETS.md if rules are too restrictive

---

## Best Practices

### Security Best Practices

1. **Minimal Bypass**: Grant bypass only to repository owner
2. **Document Bypasses**: Always document reason for bypassing
3. **Regular Audits**: Review audit logs monthly
4. **Test Changes**: Test ruleset changes in test environment first
5. **Backup Configuration**: Export rulesets regularly

### Operational Best Practices

1. **Start Strict**: Begin with strict rules, relax as needed
2. **Communicate Changes**: Announce ruleset updates to team
3. **Version Control**: Keep RULESETS.md updated
4. **Monitor Impact**: Track PR merge times and bottlenecks
5. **Balance Security and Productivity**: Adjust rules based on feedback

### Documentation Best Practices

1. **Keep Updated**: Update RULESETS.md when changing rules
2. **Include Examples**: Provide real examples of rule effects
3. **Explain Reasoning**: Document why each rule exists
4. **Link Resources**: Reference GitHub documentation
5. **Provide Contact**: Include owner contact for questions

---

## Rollback Plan

### If Rulesets Cause Issues

1. **Immediate Rollback**
   ```
   Settings → Rules → Rulesets → [Ruleset] → Disable
   ```

2. **Temporary Bypass**
   - Grant temporary bypass to affected users
   - Set expiration date for bypass
   - Document incident

3. **Evaluate and Fix**
   - Identify root cause
   - Test fix in sandbox
   - Re-enable with corrected configuration

4. **Post-Incident**
   - Document what went wrong
   - Update procedures
   - Communicate to team

---

## Resources

### GitHub Documentation

- [About Rulesets](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-rulesets/about-rulesets)
- [Creating Rulesets](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-rulesets/creating-rulesets)
- [Managing Rulesets](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-rulesets/managing-rulesets-for-a-repository)

### Repository Documentation

- [RULESETS.md](RULESETS.md) - Ruleset reference
- [POLICY.md](POLICY.md) - Repository policies
- [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines
- [.github/CODEOWNERS](.github/CODEOWNERS) - Code ownership

### Support

- **Questions**: Create GitHub Discussion
- **Issues**: Open GitHub Issue
- **Owner Contact**: @kushmanmb

---

## Checklist for Initial Setup

Use this checklist when setting up rulesets for the first time:

- [ ] Have repository admin access
- [ ] 2FA enabled
- [ ] Read RULESETS.md
- [ ] Create master branch protection ruleset
- [ ] Create release branch protection ruleset
- [ ] Create tag protection ruleset
- [ ] Configure CODEOWNERS file
- [ ] Test branch protection (try to push to master)
- [ ] Test PR requirements (create and merge test PR)
- [ ] Test status checks (wait for CI)
- [ ] Document configuration in RULESETS.md
- [ ] Communicate to team
- [ ] Schedule first quarterly review

---

**Document Version**: 1.0  
**Last Updated**: 2026-02-17  
**Maintained By**: kushmanmb

For questions about this guide, create a GitHub Discussion or Issue.
