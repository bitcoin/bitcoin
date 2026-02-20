# Status Check Configuration Notes

## Overview

The `master-branch-protection.json` ruleset requires specific status checks to pass before merging. These status check names must **exactly match** the context names reported by GitHub Actions workflows.

## Current Configuration

The current `master-branch-protection.json` requires these status checks:

```json
{
  "context": "ci / ci (ubuntu-latest)"
},
{
  "context": "ci / lint"
},
{
  "context": "CodeQL"
},
{
  "context": "verify-no-secrets"
}
```

## Actual Workflow Status Checks

Based on the workflow analysis:

### CI Workflow (`.github/workflows/ci.yml`)

**Workflow Name:** `CI`

**Jobs:**
- `lint` → Status check: **`CI / lint`** ✅ (matches)
- `ci-matrix` with multiple matrix values:
  - `CI / iwyu`
  - `CI / 32 bit ARM`
  - `CI / ASan + LSan + UBSan + integer`
  - `CI / macOS-cross to arm64`
  - `CI / macOS-cross to x86_64`
  - `CI / No wallet`
  - `CI / i686, no IPC`
  - `CI / fuzzer,address,undefined,integer`
  - And more...

**Issue:** There is no job that produces `ci / ci (ubuntu-latest)` status check.

### CodeQL Workflow (`.github/workflows/codeql.yml`)

**Workflow Name:** `CodeQL Advanced`

**Expected Status Check:** `CodeQL Advanced / Analyze (language)` or similar

**Issue:** The workflow name is "CodeQL Advanced", not just "CodeQL"

### Secret Scanning

**Workflow:** `.github/workflows/bitcoin-ownership-announcement.yml`

**Job Name:** `verify-no-secrets` with display name `Verify No Secrets in Announcement`

**Expected Status Check:** `Bitcoin Ownership Announcement / Verify No Secrets in Announcement`

**Issue:** The status check context includes the full workflow name.

## Recommended Fixes

### Option 1: Update Ruleset to Match Specific CI Jobs (Recommended)

Update `master-branch-protection.json` to require specific, essential CI jobs:

```json
{
  "type": "required_status_checks",
  "parameters": {
    "required_status_checks": [
      {
        "context": "CI / lint"
      },
      {
        "context": "CI / ASan + LSan + UBSan + integer"
      },
      {
        "context": "CodeQL Advanced / Analyze"
      }
    ],
    "strict_required_status_checks_policy": true
  }
}
```

### Option 2: Create a Summary Status Check

Add a final job to the CI workflow that depends on all critical jobs and reports a single status:

```yaml
# Add to .github/workflows/ci.yml
jobs:
  # ... existing jobs ...
  
  ci-success:
    name: 'ci (ubuntu-latest)'
    needs: [lint, ci-matrix]
    runs-on: ubuntu-latest
    if: always()
    steps:
      - name: Check all jobs succeeded
        run: |
          if [ "${{ needs.lint.result }}" != "success" ] || \
             [ "${{ needs.ci-matrix.result }}" != "success" ]; then
            exit 1
          fi
```

This would create a status check `CI / ci (ubuntu-latest)` that summarizes other jobs.

### Option 3: Remove Status Check Requirements Temporarily

Simplify the ruleset to only require pull request approval without specific status checks:

```json
{
  "rules": [
    {
      "type": "pull_request",
      "parameters": {
        "required_approving_review_count": 1,
        "dismiss_stale_reviews_on_push": true,
        "require_code_owner_review": true,
        "require_last_push_approval": true
      }
    },
    {
      "type": "deletion"
    },
    {
      "type": "non_fast_forward"
    }
  ]
}
```

This keeps protection while allowing flexibility during initial setup.

## Verification Steps

After applying rulesets:

1. **Create a test PR** to trigger workflows
2. **Check actual status checks** reported:
   ```bash
   gh pr view <PR_NUMBER> --json statusCheckRollup --jq '.statusCheckRollup[].context'
   ```
3. **Update ruleset** with exact context names
4. **Reapply the ruleset**

## How to Update Status Checks

1. **Edit the JSON file:**
   ```bash
   vim .github/rulesets/master-branch-protection.json
   ```

2. **Update the required_status_checks array** with correct context names

3. **Get the ruleset ID:**
   ```bash
   gh api repos/kushmanmb-org/bitcoin/rulesets | \
     jq -r '.[] | select(.name=="master-branch-protection") | .id'
   ```

4. **Update the ruleset:**
   ```bash
   gh api --method PUT \
     -H "Accept: application/vnd.github+json" \
     repos/kushmanmb-org/bitcoin/rulesets/<RULESET_ID> \
     --input .github/rulesets/master-branch-protection.json
   ```

## Best Practices

1. **Start with minimal status checks** and add more as needed
2. **Use a summary/aggregator job** to simplify status check requirements
3. **Test in a draft PR** before enforcing on master
4. **Document why each status check is required**
5. **Review and update quarterly** as workflows evolve

## Current Recommendation

For immediate application, I recommend **Option 3** (simplified ruleset without specific status checks) or **Option 2** (add a summary status check job to the CI workflow).

This allows the branch protection to be activated immediately while you determine the exact status check names from actual PR runs.

---

**Note:** This is a common issue when setting up branch protection. GitHub requires exact matches for status check context names, which can be tricky with matrix strategies.

**Last Updated:** 2026-02-19
