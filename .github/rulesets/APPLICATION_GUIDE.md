# Rulesets Application Guide

## Overview

This guide provides complete instructions for applying the branch protection rulesets to the kushmanmb-org/bitcoin repository.

## Current Status

✅ **All ruleset configuration files are ready**
- Master branch protection
- Release branch protection  
- Development branches
- Release tags protection

## Prerequisites

Before applying rulesets, ensure you have:

1. **Admin access** to the kushmanmb-org/bitcoin repository
2. **GitHub CLI** installed and authenticated
3. **jq** installed for JSON processing
4. **Two-factor authentication** enabled on your GitHub account

## Installation Steps

### 1. Install Required Tools

#### GitHub CLI
```bash
# macOS
brew install gh

# Linux (Ubuntu/Debian)
curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg | sudo dd of=/usr/share/keyrings/githubcli-archive-keyring.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" | sudo tee /etc/apt/sources.list.d/github-cli.list > /dev/null
sudo apt update
sudo apt install gh

# Windows
winget install --id GitHub.cli
```

#### jq (JSON processor)
```bash
# macOS
brew install jq

# Linux
sudo apt-get install jq

# Windows
winget install jqlang.jq
```

### 2. Authenticate with GitHub

```bash
gh auth login
```

Follow the prompts to authenticate with your GitHub account.

### 3. Verify Authentication

```bash
gh auth status
```

Expected output should show you're logged in to github.com.

## Applying Rulesets

### Important: Choose Your Approach

There are two versions of the master branch protection ruleset:

1. **`master-branch-protection.json`** - Full protection with specific status checks
   - ⚠️ Requires status check names to match exactly
   - Read [STATUS_CHECK_NOTES.md](STATUS_CHECK_NOTES.md) before using

2. **`master-branch-protection-simple.json`** - Simplified protection without status checks (RECOMMENDED for initial setup)
   - ✅ Works immediately
   - Still requires PR approval from code owner
   - Blocks force pushes and deletions
   - Add status checks later after verification

### Quick Apply - Simplified Version (Recommended)

Apply the simplified version first to get protection active immediately:

```bash
cd /path/to/bitcoin

# Apply simplified master protection (no status checks)
gh api --method POST \
  -H "Accept: application/vnd.github+json" \
  repos/kushmanmb-org/bitcoin/rulesets \
  --input .github/rulesets/master-branch-protection-simple.json

# Apply other rulesets
gh api --method POST \
  -H "Accept: application/vnd.github+json" \
  repos/kushmanmb-org/bitcoin/rulesets \
  --input .github/rulesets/release-branch-protection.json

gh api --method POST \
  -H "Accept: application/vnd.github+json" \
  repos/kushmanmb-org/bitcoin/rulesets \
  --input .github/rulesets/development-branches.json

gh api --method POST \
  -H "Accept: application/vnd.github+json" \
  repos/kushmanmb-org/bitcoin/rulesets \
  --input .github/rulesets/release-tags-protection.json
```

### Quick Apply - Full Version with Script

Use the provided script to apply all rulesets at once (includes status checks):

```bash
cd /path/to/bitcoin
.github/rulesets/apply-rulesets.sh --create
```

⚠️ **Note:** The script will attempt to create `master-branch-protection.json` with status checks. If this fails or blocks legitimate PRs, delete it and apply the simplified version instead.

### Manual Apply (Alternative)

Apply each ruleset individually:

```bash
# Set repository variable
REPO="kushmanmb-org/bitcoin"

# Apply master branch protection
gh api --method POST \
  -H "Accept: application/vnd.github+json" \
  "repos/$REPO/rulesets" \
  --input .github/rulesets/master-branch-protection.json

# Apply release branch protection
gh api --method POST \
  -H "Accept: application/vnd.github+json" \
  "repos/$REPO/rulesets" \
  --input .github/rulesets/release-branch-protection.json

# Apply development branches
gh api --method POST \
  -H "Accept: application/vnd.github+json" \
  "repos/$REPO/rulesets" \
  --input .github/rulesets/development-branches.json

# Apply tag protection
gh api --method POST \
  -H "Accept: application/vnd.github+json" \
  "repos/$REPO/rulesets" \
  --input .github/rulesets/release-tags-protection.json
```

## Verification

### 1. List Active Rulesets

```bash
.github/rulesets/apply-rulesets.sh --list
```

Expected output should show 4 rulesets:
- master-branch-protection (active)
- release-branch-protection (active)
- development-branches (active)
- release-tags-protection (active)

### 2. Verify Ruleset Configuration

```bash
.github/rulesets/apply-rulesets.sh --verify
```

All 4 expected rulesets should show as active with ✓ marks.

### 3. Test Branch Protection

Test that the protection is working:

```bash
# This should fail (protected branch)
git checkout master
git commit --allow-empty -m "test protection"
git push origin master
```

Expected result: Push should be rejected with a protection error.

## Status Check Configuration

The master branch protection requires these status checks to pass:

1. **CI / ci (ubuntu-latest)** - Main CI pipeline
2. **CI / lint** - Code linting
3. **CodeQL** - Security scanning  
4. **verify-no-secrets** - Secret detection

### Important Notes on Status Checks

⚠️ **Status Check Names Must Match Exactly**

The status check contexts in `master-branch-protection.json` must match the exact names reported by GitHub Actions workflows. The format is typically:

```
[Workflow Name] / [Job Name] ([Matrix Value if applicable])
```

**Common Issues:**
- Workflow name is "CI" not "ci"
- Job names from matrix strategies include the matrix value
- CodeQL workflow name might be "CodeQL Advanced" not "CodeQL"

### Verifying Status Check Names

To see actual status check names from a recent pull request:

```bash
# Get a recent PR number
PR_NUMBER=$(gh pr list --limit 1 --json number --jq '.[0].number')

# Check status checks for that PR
gh pr view $PR_NUMBER --json statusCheckRollup --jq '.statusCheckRollup[].context'
```

This will show the exact status check context names as reported by GitHub.

### Updating Status Check Names

If status check names don't match, update the JSON file:

1. Edit `.github/rulesets/master-branch-protection.json`
2. Update the `required_status_checks` array with correct context names
3. Re-apply the ruleset:
   ```bash
   # Get the ruleset ID
   RULESET_ID=$(gh api repos/kushmanmb-org/bitcoin/rulesets | jq -r '.[] | select(.name=="master-branch-protection") | .id')
   
   # Update the ruleset
   gh api --method PUT \
     -H "Accept: application/vnd.github+json" \
     "repos/kushmanmb-org/bitcoin/rulesets/$RULESET_ID" \
     --input .github/rulesets/master-branch-protection.json
   ```

## Troubleshooting

### Issue: "Ruleset already exists"

If you see errors about rulesets already existing:

```bash
# List existing rulesets
.github/rulesets/apply-rulesets.sh --list

# Option 1: Delete and recreate
.github/rulesets/apply-rulesets.sh --delete
.github/rulesets/apply-rulesets.sh --create

# Option 2: Update existing rulesets
# Get ruleset IDs and update each one
gh api repos/kushmanmb-org/bitcoin/rulesets | jq -r '.[] | "\(.id) \(.name)"'
```

### Issue: "Authentication failed"

```bash
# Re-authenticate
gh auth logout
gh auth login
```

### Issue: "You don't have permission"

Ensure you have:
- Admin access to the repository
- Two-factor authentication enabled
- Proper GitHub authentication via `gh auth login`

### Issue: "Status checks not enforcing"

1. Verify status check names match exactly (case-sensitive)
2. Ensure workflows are configured to run on pull requests
3. Wait a few minutes for GitHub to propagate changes
4. Check workflow files trigger on `pull_request` events

## Web Interface Alternative

If CLI application fails, you can configure rulesets via GitHub's web interface:

1. Go to https://github.com/kushmanmb-org/bitcoin/settings/rules
2. Click "New branch ruleset" or "New tag ruleset"
3. Configure according to the JSON files in this directory
4. Save and activate

See [RULESETS_SETUP_GUIDE.md](../../RULESETS_SETUP_GUIDE.md) for detailed web interface instructions.

## Post-Application Checklist

After applying rulesets:

- [ ] Verify all 4 rulesets show as active
- [ ] Test direct push to master (should fail)
- [ ] Create a test PR and verify approval requirements
- [ ] Verify status checks are required before merge
- [ ] Update team about new protection rules
- [ ] Document any bypass events in audit log

## Backup and Maintenance

### Backup Current Configuration

Before making changes, backup current rulesets:

```bash
gh api repos/kushmanmb-org/bitcoin/rulesets > rulesets-backup-$(date +%Y%m%d).json
```

### Regular Maintenance

- **Weekly**: Check audit log for bypass events
- **Monthly**: Verify rulesets are still appropriate
- **Quarterly**: Review and update as needed
- **Annually**: Comprehensive security audit

## Additional Resources

- **Complete Documentation**: [RULESETS.md](../../RULESETS.md)
- **Setup Guide**: [RULESETS_SETUP_GUIDE.md](../../RULESETS_SETUP_GUIDE.md)
- **Quick Start**: [QUICKSTART.md](QUICKSTART.md)
- **GitHub Documentation**: https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-rulesets

## Support

For questions or issues:
- Create a GitHub Discussion
- Open a GitHub Issue
- Contact repository owner: @kushmanmb
- Reference this guide and include error messages

---

**Document Version**: 1.0  
**Last Updated**: 2026-02-19  
**Maintained By**: kushmanmb
