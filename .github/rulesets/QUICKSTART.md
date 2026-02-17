# GitHub Rulesets Quick Start Guide

## What Are Repository Rulesets?

Repository rulesets are GitHub's modern approach to branch protection and repository security. They provide:
- Automated enforcement of contribution policies
- Branch protection for critical branches
- Tag protection for releases
- Status check requirements before merging
- Audit trail of all protection events

## Files in This Directory

```
.github/rulesets/
├── README.md                          # Complete usage documentation
├── IMPLEMENTATION_NOTES.md            # Technical implementation details
├── QUICKSTART.md                      # This file
├── apply-rulesets.sh                  # Management script
├── master-branch-protection.json      # Master branch protection
├── release-branch-protection.json     # Release branches protection
├── development-branches.json          # Development branches (light protection)
└── release-tags-protection.json       # Version tag protection
```

## 🚀 Quick Start (3 Steps)

### Step 1: Prerequisites

Ensure you have:
```bash
# Install GitHub CLI (if needed)
# macOS: brew install gh
# Linux: See https://github.com/cli/cli#installation
# Windows: winget install --id GitHub.cli

# Verify installation
gh --version

# Authenticate
gh auth login

# Install jq (for JSON parsing)
# macOS: brew install jq
# Linux: sudo apt-get install jq
# Windows: winget install jqlang.jq
```

### Step 2: Apply Rulesets

From the repository root:
```bash
# Apply all rulesets at once
.github/rulesets/apply-rulesets.sh --create
```

**Output:**
```
Creating rulesets from JSON files...
Creating ruleset: master-branch-protection
✓ Successfully created: master-branch-protection
Creating ruleset: release-branch-protection
✓ Successfully created: release-branch-protection
Creating ruleset: development-branches
✓ Successfully created: development-branches
Creating ruleset: release-tags-protection
✓ Successfully created: release-tags-protection

Done! Run with --list to see all rulesets.
```

### Step 3: Verify

```bash
# List all rulesets
.github/rulesets/apply-rulesets.sh --list

# Verify expected rulesets are active
.github/rulesets/apply-rulesets.sh --verify
```

## What Each Ruleset Does

### 🔒 Master Branch Protection
**File:** `master-branch-protection.json`

Protects the `master` branch with:
- ✅ Requires 1 approval before merging
- ✅ Requires code owner review (from CODEOWNERS file)
- ✅ Requires these status checks to pass:
  - `ci / ci (ubuntu-latest)` - Main CI pipeline
  - `ci / lint` - Code linting
  - `CodeQL` - Security scanning
  - `verify-no-secrets` - Secret detection
- ✅ Blocks direct pushes (all changes via PR)
- ✅ Blocks force pushes
- ✅ Blocks branch deletion

### 📦 Release Branch Protection
**File:** `release-branch-protection.json`

Protects all `release/*` branches with:
- ✅ Requires 1 approval from repository owner
- ✅ Blocks force pushes
- ✅ Blocks branch deletion

### 🏷️ Version Tag Protection
**File:** `release-tags-protection.json`

Protects version tags (e.g., `v1.0.0`, `v2.1.3`) with:
- ✅ Only repository admins can create tags
- ✅ Only repository admins can delete tags
- ✅ Tags are immutable (cannot be modified)

### 🛠️ Development Branches
**File:** `development-branches.json`

Light protection for `feature/*`, `fix/*`, `docs/*` branches:
- ✅ No strict requirements
- ✅ Allows direct pushes for rapid development
- ✅ Allows force pushes for rebasing
- ✅ Allows branch deletion after merge

## Manual Application (Alternative)

If you prefer to apply rulesets individually:

```bash
# Master branch protection
gh api --method POST \
  -H "Accept: application/vnd.github+json" \
  repos/kushmanmb-org/bitcoin/rulesets \
  --input .github/rulesets/master-branch-protection.json

# Release branch protection
gh api --method POST \
  -H "Accept: application/vnd.github+json" \
  repos/kushmanmb-org/bitcoin/rulesets \
  --input .github/rulesets/release-branch-protection.json

# Tag protection
gh api --method POST \
  -H "Accept: application/vnd.github+json" \
  repos/kushmanmb-org/bitcoin/rulesets \
  --input .github/rulesets/release-tags-protection.json

# Development branches
gh api --method POST \
  -H "Accept: application/vnd.github+json" \
  repos/kushmanmb-org/bitcoin/rulesets \
  --input .github/rulesets/development-branches.json
```

## Testing Protection Rules

After applying rulesets, test them:

### Test 1: Protected Branch (should fail)
```bash
git checkout master
git commit --allow-empty -m "test protection"
git push origin master
# Expected: ❌ Push rejected - branch is protected
```

### Test 2: Pull Request Flow (should work)
```bash
# Create a feature branch
git checkout -b feature/test-rulesets
git commit --allow-empty -m "test PR flow"
git push origin feature/test-rulesets

# Create PR
gh pr create --title "Test Rulesets" --body "Testing branch protection"

# Try to merge without approval
gh pr merge
# Expected: ❌ Merge blocked - needs approval

# After getting approval and passing CI
gh pr merge --squash
# Expected: ✅ Merge succeeds
```

### Test 3: Tag Protection (should require admin)
```bash
# Try to create a tag
git tag v1.0.0
git push origin v1.0.0
# Expected: If you're not admin: ❌ Push rejected
# Expected: If you're admin: ✅ Tag created
```

## Troubleshooting

### "Cannot push to protected branch"
**Expected behavior** - All changes to `master` must go through pull requests.
**Solution:** Create a feature branch and open a PR.

### "Merge is blocked"
Check that:
- ✅ PR has required approvals (1 approval from code owner)
- ✅ All status checks passed (ci, lint, CodeQL)
- ✅ Branch is up to date with master

### "Ruleset already exists"
If you see errors when running `--create`, rulesets may already be configured.
**Solution:** Use `--list` to see existing rulesets, or delete and recreate.

## Common Commands

```bash
# List all rulesets
.github/rulesets/apply-rulesets.sh --list

# Verify rulesets are active
.github/rulesets/apply-rulesets.sh --verify

# View specific ruleset details
gh api repos/kushmanmb-org/bitcoin/rulesets | jq '.[] | select(.name=="master-branch-protection")'

# Export rulesets for backup
gh api repos/kushmanmb-org/bitcoin/rulesets > rulesets-backup.json
```

## Learn More

- **Complete Documentation:** [README.md](README.md)
- **Implementation Details:** [IMPLEMENTATION_NOTES.md](IMPLEMENTATION_NOTES.md)
- **Repository Policies:** [../../RULESETS.md](../../RULESETS.md)
- **Setup Guide:** [../../RULESETS_SETUP_GUIDE.md](../../RULESETS_SETUP_GUIDE.md)

## Need Help?

- 📖 Read the [README.md](README.md) for detailed instructions
- 🐛 Open a GitHub Issue if you encounter problems
- 💬 Start a GitHub Discussion for questions
- 👤 Contact repository owner: @kushmanmb

---

**Quick Reference:**
```bash
# Apply all rulesets
.github/rulesets/apply-rulesets.sh --create

# Verify they're active
.github/rulesets/apply-rulesets.sh --verify

# Test protection
git checkout master && git push  # Should fail ✅
```

**Status:** Production Ready  
**Last Updated:** 2026-02-17
