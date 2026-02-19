# GitGuardian Secret Scanning Setup

## Overview

This repository uses [GitGuardian](https://www.gitguardian.com/) for automated secret scanning to prevent accidental exposure of sensitive information such as API keys, credentials, and private keys.

## What is GitGuardian?

GitGuardian is a security tool that scans your codebase for:
- API keys and tokens
- Database credentials
- Private keys and certificates
- OAuth tokens
- AWS credentials
- And 350+ other types of secrets

## Workflow Integration

The GitGuardian workflow (`.github/workflows/gitguardian.yml`) automatically runs on:
- **Pull Requests**: Scans all changes before they are merged
- **Push Events**: Scans commits pushed to any branch
- **Scheduled Scans**: Daily scans at 2 AM UTC to catch any historical secrets

## Setup Instructions

### For Repository Administrators

To enable GitGuardian scanning, you need to:

1. **Create a GitGuardian Account**
   - Go to [GitGuardian](https://www.gitguardian.com/)
   - Sign up for an account (free tier available for public repositories)
   - Navigate to your dashboard

2. **Generate an API Key**
   - In your GitGuardian dashboard, go to **API** → **Personal Access Tokens**
   - Click **Create token**
   - Give it a descriptive name (e.g., "kushmanmb-org/bitcoin")
   - Copy the generated API key (you won't be able to see it again)

3. **Add the API Key to GitHub Secrets**
   - Go to your GitHub repository settings
   - Navigate to **Settings** → **Secrets and variables** → **Actions**
   - Click **New repository secret**
   - Name: `GITGUARDIAN_API_KEY`
   - Value: Paste your GitGuardian API key
   - Click **Add secret**

4. **Verify the Setup**
   - The workflow will now run automatically on the next push or PR
   - Check the **Actions** tab to see the workflow results
   - Any detected secrets will appear as workflow failures with details

## Workflow Status

Once configured, you can view the GitGuardian scan results:
- Go to the **Actions** tab in the repository
- Look for "GitGuardian Secret Scanning" workflow runs
- Click on any run to see detailed results

## What to Do If Secrets Are Found

If GitGuardian detects secrets in your code:

1. **DO NOT** ignore the alert
2. **Immediately** revoke/rotate the exposed credential
3. **Remove** the secret from the code
4. **Replace** with environment variables or GitHub Secrets
5. **Rewrite** git history if the secret was committed (use `git filter-branch` or `BFG Repo-Cleaner`)

### Example: Removing a Secret

```bash
# Option 1: Amend the last commit
git commit --amend
git push --force

# Option 2: Use git filter-branch for older commits
git filter-branch --force --index-filter \
  'git rm --cached --ignore-unmatch path/to/file/with/secret' \
  --prune-empty --tag-name-filter cat -- --all

# Option 3: Use BFG Repo-Cleaner (faster for large repos)
bfg --replace-text passwords.txt
git reflog expire --expire=now --all && git gc --prune=now --aggressive
```

## Configuration

The GitGuardian workflow can be customized by editing `.github/workflows/gitguardian.yml`:

- **Scan Frequency**: Modify the `schedule` cron expression
- **Branches**: Adjust which branches trigger scans
- **Permissions**: Modify to match your security requirements

## Benefits

✅ **Prevents Credential Leaks**: Catches secrets before they're merged  
✅ **Automated Security**: No manual secret scanning needed  
✅ **Compliance**: Helps meet security compliance requirements  
✅ **Historical Scanning**: Finds secrets in existing codebase  
✅ **CI/CD Integration**: Blocks merges if secrets are detected  

## Troubleshooting

### Workflow Shows "Action Required"

This typically means the `GITGUARDIAN_API_KEY` secret is not configured. Follow the setup instructions above.

### False Positives

If GitGuardian detects false positives (e.g., example credentials, test data):
1. Review the detection to confirm it's truly a false positive
2. Add the file or pattern to `.gitguardian.yaml` ignore list
3. Document why the detection is being ignored

### Getting Help

- **GitGuardian Documentation**: https://docs.gitguardian.com/
- **GitHub Actions**: Check the Actions tab for detailed logs
- **Repository Issues**: Open an issue if you encounter problems with the workflow

## Related Security Documentation

- [SECURITY.md](SECURITY.md) - Security policy and vulnerability reporting
- [SECURITY_PRACTICES.md](SECURITY_PRACTICES.md) - General security practices
- [SECURITY_AUDIT.md](SECURITY_AUDIT.md) - Security audit information

## License

This GitGuardian integration is part of the Bitcoin Core project and follows the MIT License.
