# GitHub Repository Rulesets

## Overview

This document describes the GitHub repository rulesets configured for the kushmanmb-org/bitcoin repository. Rulesets enforce branch protection, workflow security, and contribution policies automatically.

**Note**: Repository rulesets are configured through GitHub's web interface. This document serves as the authoritative reference for the intended ruleset configuration.

---

## Ruleset Architecture

### Ruleset Types

1. **Branch Protection Rulesets**: Control branch operations
2. **Tag Protection Rulesets**: Control tag creation and deletion
3. **Push Rulesets**: Control who can push to which refs
4. **Workflow Rulesets**: Control GitHub Actions behavior

---

## Branch Protection Rulesets

### Ruleset: `master-branch-protection`

**Description**: Protects the master branch from direct pushes and ensures code quality.

**Target**: Branch name pattern `master`

**Status**: ✅ Active

#### Rules

##### 1. Require Pull Requests

- **Enabled**: Yes
- **Required Approvals**: 1
- **Dismiss Stale Reviews**: Yes (when new commits pushed)
- **Require Review from Code Owners**: Yes
- **Who Can Dismiss Reviews**: Repository owner only
- **Who Can Push**: Repository owner only (after approval)

##### 2. Require Status Checks

- **Enabled**: Yes
- **Require Branches to be Up to Date**: Yes
- **Required Status Checks**:
  - `ci / ci (ubuntu-latest)` - Main CI pipeline
  - `ci / lint` - Code linting
  - `CodeQL` - Security scanning
  - `verify-no-secrets` - Secret detection

##### 3. Require Signed Commits

- **Enabled**: Recommended (not enforced)
- **Reason**: Encourages but doesn't require GPG signatures for compatibility

##### 4. Require Linear History

- **Enabled**: No
- **Reason**: Allow merge commits for clear PR history

##### 5. Include Administrators

- **Enabled**: No
- **Reason**: Allow owner to override in emergencies

##### 6. Restrict Deletions

- **Enabled**: Yes
- **Who Can Delete**: Repository owner only

##### 7. Restrict Force Pushes

- **Enabled**: Yes
- **No Force Pushes**: Complete prohibition

##### 8. Lock Branch

- **Enabled**: No
- **Reason**: Allow normal development flow

#### Bypass Permissions

- **Repository Owner**: Can bypass all rules in emergencies
- **GitHub Actions**: Can bypass for automated commits (announcements)

---

### Ruleset: `release-branch-protection`

**Description**: Protects release branches from accidental modification.

**Target**: Branch name pattern `release/*`

**Status**: ✅ Active

#### Rules

##### 1. Require Pull Requests

- **Enabled**: Yes
- **Required Approvals**: 1
- **Only Owner Can Approve**: Yes

##### 2. Restrict Deletions

- **Enabled**: Yes
- **Permanent**: Yes

##### 3. Restrict Force Pushes

- **Enabled**: Yes
- **No Force Pushes**: Complete prohibition

##### 4. Restrict Pushes

- **Enabled**: Yes
- **Who Can Push**: Repository owner only

---

### Ruleset: `development-branches`

**Description**: Lighter protection for feature and fix branches.

**Target**: Branch name patterns: `feature/*`, `fix/*`, `docs/*`

**Status**: ✅ Active

#### Rules

##### 1. Require Pull Requests

- **Enabled**: No (can push directly to own branches)
- **Reason**: Allow rapid development on feature branches

##### 2. Restrict Deletions

- **Enabled**: No
- **Reason**: Allow cleanup of merged branches

##### 3. Require Status Checks

- **Enabled**: Recommended but not enforced
- **Reason**: Developer discretion during active development

---

## Tag Protection Rulesets

### Ruleset: `release-tags`

**Description**: Protects version tags from modification or deletion.

**Target**: Tag pattern `v*.*.*` (semantic versions)

**Status**: ✅ Active

#### Rules

##### 1. Restrict Tag Creation

- **Enabled**: Yes
- **Who Can Create**: Repository owner only

##### 2. Restrict Tag Deletion

- **Enabled**: Yes
- **Who Can Delete**: Repository owner only

##### 3. Require Signed Tags

- **Enabled**: Yes (for releases)
- **Reason**: Ensure authenticity of releases

##### 4. Immutable Tags

- **Enabled**: Yes
- **Cannot Modify**: Tags are permanent once created

---

## Workflow Rulesets

### Ruleset: `workflow-security`

**Description**: Ensures GitHub Actions workflows follow security best practices.

**Target**: Workflow files in `.github/workflows/`

**Status**: ✅ Active

#### Rules

##### 1. Restrict Workflow Changes

- **Enabled**: Yes
- **Who Can Modify**: Repository owner only
- **Require Pull Request**: Yes
- **Require Owner Approval**: Yes

##### 2. Allowed Actions

- **Enabled**: Yes
- **Action Source**: 
  - All actions from verified creators
  - Selected actions only
- **Allowed Actions List**:
  - `actions/*` (GitHub-provided actions)
  - `github/*` (GitHub-provided actions)
  - Custom actions in this repository

##### 3. Workflow Permissions

- **Default Permissions**: Read-only
- **Can Escalate**: No (must explicitly grant permissions)
- **Token Permissions**: Minimal required

##### 4. Secrets Access

- **Enabled**: Yes
- **Who Can Access**: Workflows only (not forks)
- **Environment Protection**: Required for production secrets

---

## Push Rulesets

### Ruleset: `restrict-direct-pushes`

**Description**: Prevents bypassing pull request requirements.

**Target**: All branches except personal branches

**Status**: ✅ Active

#### Rules

##### 1. Block Direct Pushes

- **Enabled**: Yes
- **Target**: `master`, `release/*`, `hotfix/*`
- **Exception**: Repository owner with reason

##### 2. Require Pull Request

- **Enabled**: Yes
- **All Changes**: Must go through PR process

---

## Deployment Protection Rules

### Ruleset: `production-deployment`

**Description**: Protects production deployments.

**Target**: Environment name `production`

**Status**: ✅ Active

#### Rules

##### 1. Required Reviewers

- **Enabled**: Yes
- **Required Approvals**: Repository owner
- **Timeout**: 48 hours

##### 2. Wait Timer

- **Enabled**: No
- **Reason**: Allow immediate deployments after approval

##### 3. Deployment Branches

- **Allowed Branches**: `master`, `release/*` only

---

## Special Cases and Exceptions

### Emergency Hotfixes

**Scenario**: Critical security vulnerability requires immediate fix

**Process**:
1. Repository owner can bypass branch protection
2. Must document reason in commit message
3. Must create issue tracking the emergency bypass
4. Should create PR retrospectively for review

### Automated Commits

**Scenario**: GitHub Actions needs to commit generated files

**Process**:
1. Use GitHub Actions bot account
2. Grant minimal permissions in workflow
3. Allow bypass for specific paths (e.g., `data/ownership/`)
4. Document in workflow file

### Dependency Updates

**Scenario**: Automated dependency update tools (Dependabot, Renovate)

**Process**:
1. Configure in `.github/dependabot.yml`
2. Auto-merge allowed for minor/patch updates
3. Require review for major updates
4. Must pass all status checks

---

## Enforcement Hierarchy

### Precedence Order (Highest to Lowest)

1. **Organization-Level Rules**: Apply to all repositories (if any)
2. **Repository-Level Rulesets**: Specific to this repository
3. **Branch Protection Rules**: Legacy protection (deprecated in favor of rulesets)
4. **User Permissions**: Individual access levels

### Override Authority

Only the following can override rules:
1. Repository owner (documented reason required)
2. Organization owner (if organizational override needed)
3. GitHub Support (in exceptional circumstances)

---

## Monitoring and Audit

### Audit Trail

All ruleset actions are logged:
- Ruleset creation/modification/deletion
- Bypass events
- Protection violations
- Access grants

**Access Audit Logs**:
- Repository Settings → Actions → General → Audit log
- Organization audit log (if applicable)

### Monitoring

Regular reviews conducted:
- **Weekly**: Check for bypass events
- **Monthly**: Review ruleset effectiveness
- **Quarterly**: Update rules as needed
- **Annually**: Comprehensive security audit

---

## Configuration Instructions

### How to Configure Rulesets

Rulesets are configured through GitHub's web interface:

#### 1. Access Rulesets

```
GitHub Repository → Settings → Rules → Rulesets
```

#### 2. Create New Ruleset

1. Click "New ruleset"
2. Choose "Branch" or "Tag" ruleset
3. Name the ruleset
4. Set target branches or tags
5. Add rules
6. Set bypass permissions
7. Save ruleset

#### 3. Modify Existing Ruleset

1. Go to Settings → Rules → Rulesets
2. Click ruleset name
3. Modify rules
4. Save changes

#### 4. Export Configuration

```bash
# Using GitHub CLI
gh api repos/kushmanmb-org/bitcoin/rulesets
```

### Ruleset JSON Export

For backup and version control, rulesets can be exported:

```json
{
  "name": "master-branch-protection",
  "target": "branch",
  "enforcement": "active",
  "conditions": {
    "ref_name": {
      "include": ["refs/heads/master"],
      "exclude": []
    }
  },
  "rules": [
    {
      "type": "pull_request",
      "parameters": {
        "required_approving_review_count": 1,
        "dismiss_stale_reviews_on_push": true,
        "require_code_owner_review": true
      }
    },
    {
      "type": "required_status_checks",
      "parameters": {
        "required_status_checks": [
          {"context": "ci / ci (ubuntu-latest)"},
          {"context": "ci / lint"},
          {"context": "CodeQL"}
        ],
        "strict_required_status_checks_policy": true
      }
    }
  ]
}
```

---

## Best Practices

### When Creating Rulesets

1. **Start Restrictive**: Begin with strict rules, relax as needed
2. **Test First**: Test rules on non-critical branches first
3. **Document Reasons**: Document why each rule exists
4. **Regular Review**: Review and update quarterly
5. **Communicate Changes**: Notify team of ruleset changes

### Common Pitfalls

❌ **Don't**:
- Make rules so strict that legitimate work is blocked
- Grant bypass permissions broadly
- Forget to include emergency procedures
- Apply same rules to all branches
- Ignore monitoring and audits

✅ **Do**:
- Balance security with productivity
- Document all exceptions
- Test ruleset changes
- Monitor audit logs
- Update rules as project evolves

---

## Troubleshooting

### Common Issues

#### Issue: Pull Request Cannot Merge

**Symptoms**: PR shows "Merging is blocked"

**Solutions**:
1. Check required status checks have passed
2. Ensure PR is approved by code owner
3. Update branch to latest master
4. Verify no merge conflicts

#### Issue: Cannot Push to Branch

**Symptoms**: Push rejected by remote

**Solutions**:
1. Verify you're not pushing to protected branch
2. Create a pull request instead
3. Check if branch requires linear history
4. Contact repository owner if emergency

#### Issue: Workflow Cannot Commit

**Symptoms**: GitHub Actions workflow fails when committing

**Solutions**:
1. Check workflow has write permissions
2. Verify bypass rules for bot account
3. Use proper authentication token
4. Check if workflow is allowed to commit to target path

---

## Compliance

### Regulatory Compliance

Rulesets help maintain compliance with:
- **SOC 2**: Change management controls
- **ISO 27001**: Access control and audit logging
- **GDPR**: Data protection through code review
- **PCI-DSS**: Secure development practices (if applicable)

### Open Source Best Practices

Rulesets align with:
- OpenSSF Scorecard recommendations
- CII Best Practices criteria
- GitHub Security best practices
- OWASP secure development guidelines

---

## Updates and Maintenance

### Version History

- **v1.0** (2026-02-17): Initial ruleset documentation

### Scheduled Reviews

- **Next Review**: 2026-05-17 (Quarterly)
- **Annual Audit**: 2027-02-17

### Contact

For questions about rulesets:
- Create a GitHub Discussion
- Tag repository owner: @kushmanmb
- Reference this document

---

**Document Owner**: kushmanmb  
**Last Updated**: 2026-02-17  
**Status**: Active

For questions or clarifications, create a GitHub Issue or Discussion.
