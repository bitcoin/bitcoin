# Ruleset Configuration Implementation Notes

## Overview

This document explains the implementation of GitHub repository rulesets in JSON configuration files and clarifies any differences from the theoretical documentation in [RULESETS.md](../../RULESETS.md).

## Implementation Summary

The following ruleset configuration files have been created in `.github/rulesets/`:

1. **master-branch-protection.json** - Protects the master branch
2. **release-branch-protection.json** - Protects release/* branches
3. **release-tags-protection.json** - Protects version tags (v*.*.*)
4. **development-branches.json** - Light protection for feature/fix/docs branches

## Rule Types Explained

### Branch Protection Rules

#### `pull_request` Rule Type
Enforces pull request requirements before merging:
- `required_approving_review_count`: Number of approvals needed
- `dismiss_stale_reviews_on_push`: Dismisses approvals when new commits are pushed
- `require_code_owner_review`: Requires approval from code owners (defined in CODEOWNERS)
- `require_last_push_approval`: Ensures the last committer is not the approver (security best practice)

#### `required_status_checks` Rule Type
Requires specific CI/CD checks to pass before merging:
- `required_status_checks`: Array of status check contexts that must pass
- `strict_required_status_checks_policy`: Requires branch to be up-to-date with base branch

#### `deletion` Rule Type
Prevents branch or tag deletion except by bypass actors (repository owners)

#### `non_fast_forward` Rule Type
Prevents force pushes to protected branches

### Tag Protection Rules

#### `creation` Rule Type
Restricts who can create tags (only bypass actors)

#### `update` Rule Type
Prevents modification of existing tags (makes them immutable)

## Bypass Actors

All rulesets include bypass configuration:
```json
"bypass_actors": [
  {
    "actor_id": 5,
    "actor_type": "RepositoryRole",
    "bypass_mode": "always"
  }
]
```

**Explanation**:
- `actor_id: 5` refers to repository administrators
- `actor_type: "RepositoryRole"` indicates this is a role-based bypass
- `bypass_mode: "always"` allows bypassing in emergency situations

This configuration allows repository owners to bypass rules when necessary (e.g., emergency hotfixes), while all bypass events are logged in the audit trail.

## Differences from RULESETS.md

### Signed Commits
**RULESETS.md**: Recommends signed commits but does not enforce them  
**Implementation**: Not included in JSON configurations (recommendation only)  
**Reason**: GitHub Rulesets API does not provide a direct parameter for requiring signed commits at the ruleset level. This is typically configured as a repository setting or encouraged through documentation.

### Signed Tags
**RULESETS.md**: Specifies "Require Signed Tags: Yes"  
**Implementation**: Tag protection rules restrict creation/deletion/modification to repository owners  
**Reason**: The GitHub Rulesets API controls **who** can create tags (via `creation` rule), rather than enforcing GPG signatures directly. Repository owners are expected to use signed tags as a best practice.

### Push Restrictions on Release Branches
**RULESETS.md**: Specifies "Who Can Push: Repository owner only"  
**Implementation**: Pull request requirement with code owner review effectively restricts pushes  
**Reason**: The GitHub Rulesets API enforces push restrictions through pull request requirements. Combined with CODEOWNERS file, this achieves the same effect.

### Additional Security: `require_last_push_approval`
**RULESETS.md**: Not explicitly mentioned  
**Implementation**: Added to master and release branch protection  
**Reason**: This is a security best practice that prevents the same person who pushed code from also approving it, reducing the risk of unauthorized changes.

## API Compatibility

These JSON files are compatible with GitHub's Rulesets API (REST API v3). They can be applied using:

### GitHub CLI
```bash
gh api \
  --method POST \
  -H "Accept: application/vnd.github+json" \
  repos/kushmanmb-org/bitcoin/rulesets \
  --input .github/rulesets/master-branch-protection.json
```

### Using the Management Script
```bash
.github/rulesets/apply-rulesets.sh --create
```

## Validation

All JSON files have been validated:
- ✅ Valid JSON syntax
- ✅ Correct rule types for GitHub Rulesets API
- ✅ Proper target patterns (refs/heads/*, refs/tags/*)
- ✅ Appropriate bypass actor configuration

## Testing Recommendations

After applying these rulesets, test the following scenarios:

1. **Protected Branch Push**: Try to push directly to master (should fail)
2. **Pull Request Requirements**: Create a PR without approval (merge should be blocked)
3. **Status Checks**: Create a PR that fails CI (merge should be blocked)
4. **Tag Protection**: Try to create/delete a tag (should require admin permissions)
5. **Development Branches**: Push to feature/* branch (should succeed)

## Maintenance

When updating rulesets:
1. Modify the JSON file in `.github/rulesets/`
2. Commit the change to version control
3. Apply the updated ruleset via GitHub CLI or API
4. Verify the change in GitHub web interface
5. Update this document if behavior changes

## References

- [GitHub Rulesets API Documentation](https://docs.github.com/en/rest/repos/rules)
- [RULESETS.md](../../RULESETS.md) - Conceptual ruleset documentation
- [RULESETS_SETUP_GUIDE.md](../../RULESETS_SETUP_GUIDE.md) - Setup instructions
- [.github/rulesets/README.md](README.md) - Usage instructions

## Support

For questions about these configurations:
- Check [README.md](README.md) in this directory
- Review [RULESETS.md](../../RULESETS.md) for policy details
- Contact repository owner: @kushmanmb

---

**Last Updated**: 2026-02-17  
**API Version**: GitHub REST API v3  
**Status**: Production Ready
