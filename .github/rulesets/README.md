# GitHub Rulesets Configuration

This directory contains JSON configuration files for GitHub repository rulesets. These files define branch protection, tag protection, and other repository rules that can be applied using the GitHub API or CLI.

## Overview

Repository rulesets provide a modern way to enforce branch protection and other policies across your repository. Unlike legacy branch protection rules, rulesets offer:

- More flexible targeting with patterns
- Better support for multiple protection rules
- Easier management through the API
- Version-controlled configuration

## Quick Links

- **[APPLICATION_GUIDE.md](APPLICATION_GUIDE.md)** - Complete step-by-step application instructions
- **[QUICKSTART.md](QUICKSTART.md)** - Quick 3-step setup guide
- **[verify-status-checks.sh](verify-status-checks.sh)** - Script to verify status check names
- **[apply-rulesets.sh](apply-rulesets.sh)** - Script to apply/manage rulesets

## Configuration Files

### Branch Protection Rulesets

#### `master-branch-protection.json`
Protects the `master` branch with the following rules:
- Requires 1 approval before merging
- Dismisses stale reviews when new commits are pushed
- Requires code owner review
- Requires approval of the most recent push
- Requires the following status checks to pass:
  - `ci / ci (ubuntu-latest)` - Main CI pipeline
  - `ci / lint` - Code linting
  - `CodeQL` - Security scanning
  - `verify-no-secrets` - Secret detection
- Requires branches to be up to date before merging
- Blocks deletions
- Blocks force pushes

#### `release-branch-protection.json`
Protects all `release/*` branches with:
- Requires 1 approval before merging
- Dismisses stale reviews when new commits are pushed
- Requires code owner review
- Requires approval of the most recent push
- Blocks deletions
- Blocks force pushes

#### `development-branches.json`
Light protection for `feature/*`, `fix/*`, and `docs/*` branches:
- No strict requirements
- Allows developers to push directly to their own branches
- Allows cleanup of merged branches

### Tag Protection Rulesets

#### `release-tags-protection.json`
Protects version tags matching `v*.*.*` pattern:
- Restricts tag creation to repository admins
- Restricts tag deletion to repository admins
- Restricts tag updates (makes tags immutable)

## Applying Rulesets

### Option 1: Using GitHub CLI

1. **Install GitHub CLI** (if not already installed):
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

2. **Authenticate with GitHub**:
   ```bash
   gh auth login
   ```

3. **Create rulesets from configuration files**:
   ```bash
   # Create master branch protection
   gh api \
     --method POST \
     -H "Accept: application/vnd.github+json" \
     repos/kushmanmb-org/bitcoin/rulesets \
     --input .github/rulesets/master-branch-protection.json
   
   # Create release branch protection
   gh api \
     --method POST \
     -H "Accept: application/vnd.github+json" \
     repos/kushmanmb-org/bitcoin/rulesets \
     --input .github/rulesets/release-branch-protection.json
   
   # Create development branches ruleset
   gh api \
     --method POST \
     -H "Accept: application/vnd.github+json" \
     repos/kushmanmb-org/bitcoin/rulesets \
     --input .github/rulesets/development-branches.json
   
   # Create tag protection
   gh api \
     --method POST \
     -H "Accept: application/vnd.github+json" \
     repos/kushmanmb-org/bitcoin/rulesets \
     --input .github/rulesets/release-tags-protection.json
   ```

### Option 2: Using GitHub Web Interface

1. Go to repository Settings → Rules → Rulesets
2. Click "New branch ruleset" or "New tag ruleset"
3. Manually configure rules as specified in the JSON files
4. Save the ruleset

See [RULESETS_SETUP_GUIDE.md](../../RULESETS_SETUP_GUIDE.md) for detailed web interface instructions.

## Verifying Rulesets

### List Active Rulesets

```bash
gh api repos/kushmanmb-org/bitcoin/rulesets | jq '.[] | {name: .name, enforcement: .enforcement, target: .target}'
```

### Get Specific Ruleset Details

```bash
gh api repos/kushmanmb-org/bitcoin/rulesets/[RULESET_ID] | jq '.'
```

### Test Branch Protection

Try pushing directly to the master branch (should fail):
```bash
git checkout master
git commit --allow-empty -m "test protection"
git push origin master
# Expected: Push rejected with protection error
```

## Updating Rulesets

### Update Existing Ruleset

1. Get the ruleset ID:
   ```bash
   gh api repos/kushmanmb-org/bitcoin/rulesets | jq '.[] | select(.name=="master-branch-protection") | .id'
   ```

2. Update the ruleset:
   ```bash
   gh api \
     --method PUT \
     -H "Accept: application/vnd.github+json" \
     repos/kushmanmb-org/bitcoin/rulesets/[RULESET_ID] \
     --input .github/rulesets/master-branch-protection.json
   ```

### Delete Ruleset

```bash
gh api \
  --method DELETE \
  -H "Accept: application/vnd.github+json" \
  repos/kushmanmb-org/bitcoin/rulesets/[RULESET_ID]
```

## Backup and Export

### Export Current Rulesets

To back up current rulesets:
```bash
# Export all rulesets
gh api repos/kushmanmb-org/bitcoin/rulesets | jq '.' > rulesets-backup.json

# Export specific ruleset
gh api repos/kushmanmb-org/bitcoin/rulesets/[RULESET_ID] | jq '.' > master-protection-backup.json
```

## Bypass Actors

The `bypass_actors` field in each ruleset specifies who can bypass the rules:

- `actor_id: 5` with `actor_type: "RepositoryRole"` refers to repository administrators
- `bypass_mode: "always"` allows bypassing in emergency situations

**Note**: Bypass events are logged in the repository audit log for security monitoring.

## Troubleshooting

### Issue: Cannot Create Ruleset

**Error**: "You don't have permission to create rulesets"

**Solutions**:
- Verify you have admin access to the repository
- Ensure 2FA is enabled on your account
- Check if organization policies restrict ruleset creation

### Issue: Status Checks Not Found

**Error**: Status checks specified in ruleset don't match workflow jobs

**Solutions**:
- Verify status check names match exactly (case-sensitive)
- Check `.github/workflows/` for correct job names
- Ensure workflows are running on PRs

### Issue: Ruleset Not Enforcing

**Error**: Can still push to protected branch

**Solutions**:
- Check ruleset enforcement is set to "active" (not "disabled" or "evaluate")
- Verify target pattern matches your branch name
- Wait a few minutes for GitHub to propagate changes
- Test with a non-admin account to verify protection

## Additional Resources

- [RULESETS.md](../../RULESETS.md) - Complete ruleset documentation
- [RULESETS_SETUP_GUIDE.md](../../RULESETS_SETUP_GUIDE.md) - Setup instructions
- [GitHub Rulesets Documentation](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-rulesets/about-rulesets)
- [GitHub API - Rulesets](https://docs.github.com/en/rest/repos/rules)

## Maintenance

### Regular Reviews

- **Weekly**: Check audit log for bypass events
- **Monthly**: Verify rulesets are still appropriate
- **Quarterly**: Review and update as needed
- **Annually**: Comprehensive security audit

### Version Control

When modifying rulesets:
1. Update the JSON file in this directory
2. Document changes in git commit message
3. Apply changes via GitHub CLI or API
4. Verify changes in GitHub web interface
5. Update [RULESETS.md](../../RULESETS.md) if needed

## Contact

For questions about rulesets:
- Create a GitHub Discussion
- Open a GitHub Issue
- Contact repository owner: @kushmanmb

---

**Last Updated**: 2026-02-17  
**Maintained By**: kushmanmb  
**Status**: Active
