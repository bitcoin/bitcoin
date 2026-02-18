# GitHub Attestation Verification for kushmanmb.base.eth

## Overview

This repository implements GitHub's artifact attestation feature to provide cryptographic proof that build artifacts were produced by trusted GitHub Actions workflows. This ensures the integrity and provenance of all binaries and build outputs.

## What are Attestations?

Attestations are cryptographically signed statements about software artifacts. They provide:

- **Provenance**: Proof of where and how an artifact was built
- **Integrity**: Verification that artifacts haven't been tampered with
- **Trust**: Assurance that artifacts come from authorized workflows

## ENS Identity

This repository is owned and maintained by:
- **Primary ENS**: kushmanmb.eth (Ethereum Mainnet)
- **Base Network**: kushmanmb.base.eth
- **Repository Owner**: kushmanmb-org

## Attestation Implementation

### Build Artifacts with Attestations

The following artifacts have attestations generated automatically:

1. **bitcoind** - Bitcoin daemon binary
2. **bitcoin-cli** - Bitcoin command-line interface
3. **bitcoin-tx** - Bitcoin transaction utility

### How It Works

1. **Build Process**: When code is built via the "Lint and Build" workflow:
   - Artifacts are compiled from source
   - Cryptographic signatures are generated
   - Attestations are published to GitHub

2. **Verification**: Anyone can verify artifacts using:
   - The `gh attestation verify` command
   - Our custom verification script
   - The verification workflow

## Verifying Attestations

### Prerequisites

- [GitHub CLI](https://cli.github.com/) installed
- GitHub token with appropriate permissions (for private repos)

### Method 1: Using the Verification Script

We provide a convenient script for verifying attestations:

```bash
# Verify bitcoind binary
./contrib/verify-attestation.sh build/src/bitcoind

# Verify with explicit owner
./contrib/verify-attestation.sh build/src/bitcoind kushmanmb-org

# Verify bitcoin-cli
./contrib/verify-attestation.sh build/src/bitcoin-cli
```

The script will:
- Verify the attestation signature
- Display certificate information
- Show build provenance details
- Confirm the artifact came from a trusted workflow

### Method 2: Using GitHub CLI Directly

```bash
# Basic verification
gh attestation verify build/src/bitcoind --owner kushmanmb-org

# With detailed JSON output
gh attestation verify build/src/bitcoind --owner kushmanmb-org --format json

# Verify specific repository
gh attestation verify build/src/bitcoind --repo kushmanmb-org/bitcoin
```

### Method 3: Automated Verification Workflow

The repository includes an automated verification workflow that runs:
- After successful builds (automatically)
- On manual trigger (workflow_dispatch)

To run manually:

```bash
# Using GitHub CLI
gh workflow run verify-attestation.yml

# With custom parameters
gh workflow run verify-attestation.yml \
  -f artifact_path=build/src/bitcoind \
  -f owner=kushmanmb-org
```

Or via the GitHub UI:
1. Go to the "Actions" tab
2. Select "Verify Attestations" workflow
3. Click "Run workflow"
4. Enter parameters (or use defaults)

## Understanding Verification Output

### Successful Verification

When an attestation is successfully verified, you'll see:

```
✓ Attestation verified successfully!

Certificate Information:
  Issuer: https://token.actions.githubusercontent.com
  SAN: https://github.com/kushmanmb-org/bitcoin/.github/workflows/lint-and-build.yml@refs/heads/main
  Source Repository: kushmanmb-org/bitcoin
  Source Repository Owner: kushmanmb-org

Subject (Artifact):
  Name: build/src/bitcoind
  Digest: sha256:abc123...

Predicate Type:
  https://slsa.dev/provenance/v1

Build Info:
  Build Type: https://slsa.dev/provenance/v1
  Repository: https://github.com/kushmanmb-org/bitcoin
  Ref: refs/heads/main
```

This confirms:
- ✅ The artifact was built by our GitHub Actions workflow
- ✅ The build occurred in the kushmanmb-org/bitcoin repository
- ✅ The artifact has not been modified since building
- ✅ The build is linked to kushmanmb.base.eth identity

### Failed Verification

If verification fails, possible causes include:

1. **No Attestation Available**
   - The artifact hasn't been built by a workflow with attestation enabled
   - Solution: Wait for the next automated build or trigger one manually

2. **Artifact Modified**
   - The file has been changed after building
   - Solution: Re-download or rebuild the artifact

3. **Wrong Owner/Repository**
   - Verification is being attempted with incorrect parameters
   - Solution: Use `--owner kushmanmb-org` or `--repo kushmanmb-org/bitcoin`

4. **Network Issues**
   - Cannot reach GitHub API
   - Solution: Check internet connection and GitHub status

## Security Implications

### What Attestations Guarantee

✅ **Verified**:
- Artifact was built by our GitHub Actions workflow
- Build occurred in our authorized repository
- Artifact hash matches the original build output
- Build environment and parameters

❌ **Not Verified** (user-controllable):
- Custom metadata in the predicate
- Workflow inputs provided by users
- Environment variables set in the workflow

### Best Practices

1. **Always Verify Before Use**
   ```bash
   gh attestation verify <artifact> --owner kushmanmb-org
   ```

2. **Check Certificate Details**
   - Verify the source repository matches: `kushmanmb-org/bitcoin`
   - Verify the workflow path is expected
   - Check the git ref is from an official branch

3. **Use Specific Verification**
   ```bash
   # More secure: verify specific workflow
   gh attestation verify build/src/bitcoind \
     --owner kushmanmb-org \
     --signer-workflow kushmanmb-org/bitcoin/.github/workflows/lint-and-build.yml
   ```

4. **Verify Multiple Artifacts**
   ```bash
   for artifact in build/src/bitcoin*; do
     echo "Verifying $artifact..."
     gh attestation verify "$artifact" --owner kushmanmb-org
   done
   ```

## Advanced Usage

### Offline Verification

Download attestation bundles for offline verification:

```bash
# Download attestation
gh attestation download build/src/bitcoind --owner kushmanmb-org

# Verify offline
gh attestation verify build/src/bitcoind --bundle attestation.jsonl
```

### Policy Enforcement

Use JSON output for custom policy enforcement:

```bash
gh attestation verify build/src/bitcoind \
  --owner kushmanmb-org \
  --format json | jq '.[] | {
    repository: .verificationResult.signature.certificate.extensions.sourceRepository,
    workflow: .verificationResult.signature.certificate.subjectAlternativeName,
    timestamp: .verificationResult.verifiedTimestamps[0].timestamp
  }'
```

### OCI Container Images

For Docker images (future implementation):

```bash
# Verify container image
gh attestation verify oci://ghcr.io/kushmanmb-org/bitcoin:latest \
  --owner kushmanmb-org

# Fetch attestations from OCI registry
gh attestation verify oci://ghcr.io/kushmanmb-org/bitcoin:latest \
  --owner kushmanmb-org \
  --bundle-from-oci
```

## Integration with CI/CD

### In GitHub Actions

```yaml
- name: Verify artifact attestation
  run: |
    gh attestation verify build/src/bitcoind \
      --owner kushmanmb-org \
      --format json
  env:
    GH_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

### In Local Development

```bash
# Add to your build script
if command -v gh &> /dev/null; then
  echo "Verifying build artifacts..."
  ./contrib/verify-attestation.sh build/src/bitcoind
fi
```

## Troubleshooting

### "attestation not found" Error

This is expected if:
- The artifact was built locally (not via GitHub Actions)
- The build workflow hasn't run yet with attestations enabled
- The workflow run was too old (attestations have a retention period)

**Solution**: Trigger a new build via GitHub Actions.

### Permission Denied

If you get permission errors:

```bash
# Authenticate with GitHub CLI
gh auth login

# Or set token explicitly
export GH_TOKEN=ghp_your_token_here
```

### Script Not Found

If the verification script isn't found:

```bash
# Make sure you're in the repository root
cd /path/to/bitcoin/repository

# Make script executable
chmod +x contrib/verify-attestation.sh

# Run script
./contrib/verify-attestation.sh build/src/bitcoind
```

## Additional Resources

- [GitHub Attestations Documentation](https://docs.github.com/en/actions/security-guides/using-artifact-attestations-to-establish-provenance-for-builds)
- [SLSA Framework](https://slsa.dev/)
- [Sigstore Project](https://www.sigstore.dev/)
- [GitHub CLI Documentation](https://cli.github.com/manual/)

## Repository Information

- **Repository**: kushmanmb-org/bitcoin
- **Owner**: kushmanmb.eth (Primary), kushmanmb.base.eth (Base Network)
- **Workflow**: `.github/workflows/lint-and-build.yml`
- **Verification Workflow**: `.github/workflows/verify-attestation.yml`
- **Verification Script**: `contrib/verify-attestation.sh`

---

**Last Updated**: 2026-02-18  
**Maintained by**: kushmanmb.base.eth  
**Version**: 1.0.0
