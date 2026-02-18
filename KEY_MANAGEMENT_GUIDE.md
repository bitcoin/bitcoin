# Key Management and Secrets Configuration Guide

## Overview

This guide provides comprehensive instructions for managing secrets, keys, and sensitive data in the Bitcoin Core repository with a focus on privacy, security, and automated workflows.

## Table of Contents

1. [Secret Management Strategy](#secret-management-strategy)
2. [GitHub Secrets Configuration](#github-secrets-configuration)
3. [Key Rotation Procedures](#key-rotation-procedures)
4. [Encrypted Secrets](#encrypted-secrets)
5. [Environment Variables](#environment-variables)
6. [Best Practices](#best-practices)
7. [Audit and Compliance](#audit-and-compliance)

## Secret Management Strategy

### Principles

1. **Never commit secrets to the repository**
2. **Use GitHub Secrets for sensitive data**
3. **Implement least privilege access**
4. **Rotate secrets regularly**
5. **Audit secret access**
6. **Use encrypted storage when possible**

### Secret Types

#### 1. GitHub Secrets (Repository Level)

Stored in: `Settings → Secrets and variables → Actions → Secrets`

**When to use:**
- API keys for external services
- Authentication tokens
- Deployment credentials
- Service account keys

**Example secrets to configure:**

```yaml
# API Keys
ETHERSCAN_API_KEY          # Etherscan API access
CDP_API_KEY                # Coinbase Developer Platform
GITHUB_TOKEN               # Auto-provided by GitHub Actions

# Deployment Keys
DEPLOY_SSH_KEY             # SSH key for deployment
GPG_PRIVATE_KEY            # GPG key for signing
SIGNING_KEY                # Code signing key

# Service Credentials
NPM_TOKEN                  # NPM registry authentication
DOCKER_USERNAME            # Docker Hub username
DOCKER_PASSWORD            # Docker Hub password
```

#### 2. GitHub Variables (Repository Level)

Stored in: `Settings → Secrets and variables → Actions → Variables`

**When to use:**
- Non-sensitive configuration
- Feature flags
- Runner configuration

**Example variables:**

```yaml
USE_SELF_HOSTED           # Enable self-hosted runners (true/false)
ENVIRONMENT               # Environment name (dev/staging/prod)
BUILD_CONFIG              # Build configuration options
ENABLE_TELEMETRY          # Enable telemetry (true/false)
```

#### 3. Environment Secrets

Stored in: `Settings → Environments → [environment name] → Secrets`

**When to use:**
- Environment-specific credentials
- Deployment-specific keys
- Stage-gated secrets

**Example environments:**

```yaml
# Production Environment
production:
  secrets:
    - PRODUCTION_API_KEY
    - PRODUCTION_DATABASE_URL
    - PRODUCTION_SIGNING_KEY

# Staging Environment
staging:
  secrets:
    - STAGING_API_KEY
    - STAGING_DATABASE_URL
```

## GitHub Secrets Configuration

### Setting Up Secrets

#### Via GitHub Web Interface

1. Navigate to repository settings
2. Go to `Secrets and variables → Actions`
3. Click `New repository secret`
4. Enter name and value
5. Click `Add secret`

#### Via GitHub CLI

```bash
# Install GitHub CLI
# See: https://cli.github.com/

# Authenticate
gh auth login

# Add a secret
gh secret set SECRET_NAME

# Add a secret from file
gh secret set SECRET_NAME < secret.txt

# Add a secret with value
echo "secret-value" | gh secret set SECRET_NAME

# List secrets
gh secret list

# Remove a secret
gh secret remove SECRET_NAME
```

### Using Secrets in Workflows

```yaml
name: Example Workflow with Secrets

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - name: Use secrets
        env:
          API_KEY: ${{ secrets.API_KEY }}
          DEPLOY_KEY: ${{ secrets.DEPLOY_SSH_KEY }}
        run: |
          # Secrets are available as environment variables
          # NEVER echo or print secrets
          echo "Deploying with API key..."
          # Use the secret in your commands
          ./deploy.sh
```

## Key Rotation Procedures

### Rotation Schedule

| Secret Type | Rotation Frequency | Priority |
|-------------|-------------------|----------|
| Production API Keys | Every 90 days | Critical |
| Staging API Keys | Every 180 days | High |
| Development Tokens | Every 365 days | Medium |
| SSH Keys | Every 365 days | High |
| GPG Keys | Every 730 days | Medium |

### Rotation Procedure

#### 1. Generate New Secret

```bash
# Generate SSH key
ssh-keygen -t ed25519 -C "github-actions@bitcoin-project" -f new_deploy_key

# Generate GPG key
gpg --full-generate-key

# Generate API token
# (Use service provider's interface)
```

#### 2. Update GitHub Secrets

```bash
# Using GitHub CLI
gh secret set DEPLOY_SSH_KEY < new_deploy_key

# Or via web interface
# Settings → Secrets → Edit secret → Update value
```

#### 3. Test New Secret

```yaml
# Create a test workflow run
name: Test New Secret

on: workflow_dispatch

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - name: Test secret
        env:
          NEW_SECRET: ${{ secrets.DEPLOY_SSH_KEY }}
        run: |
          echo "Testing new secret configuration..."
          # Test command here
```

#### 4. Revoke Old Secret

```bash
# Revoke old SSH key from target servers
ssh-keygen -R old_key_fingerprint

# Revoke old API token from service provider
# (Use service provider's interface)

# Remove old GPG key
gpg --delete-secret-key OLD_KEY_ID
```

#### 5. Document Rotation

```bash
# Create rotation record
cat >> SECURITY_LOG.md << EOF
## Secret Rotation - $(date -u +"%Y-%m-%d")

- **Secret**: DEPLOY_SSH_KEY
- **Rotated by**: [Your Name]
- **Reason**: Scheduled rotation
- **Old key revoked**: Yes
- **New key tested**: Yes
EOF
```

## Encrypted Secrets

### Using GPG for Secret Encryption

#### Setup

```bash
# Install GPG
sudo apt-get install gnupg

# Generate GPG key for the project
gpg --full-generate-key
# Select: (1) RSA and RSA
# Key size: 4096
# Expiration: 2y
# Name: Bitcoin Project Secrets
# Email: secrets@bitcoin-project
```

#### Encrypting Secrets

```bash
# Encrypt a secret file
gpg --encrypt --recipient secrets@bitcoin-project secret.json

# Encrypt with symmetric encryption (password)
gpg --symmetric --cipher-algo AES256 secret.json

# Output: secret.json.gpg
```

#### Decrypting in Workflows

```yaml
name: Use Encrypted Secrets

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Import GPG key
        run: |
          echo "${{ secrets.GPG_PRIVATE_KEY }}" | gpg --import

      - name: Decrypt secrets
        run: |
          gpg --decrypt --output secrets.json secrets.json.gpg

      - name: Use secrets
        run: |
          # Read and use secrets from decrypted file
          API_KEY=$(jq -r '.api_key' secrets.json)
          # Use API_KEY...

      - name: Clean up
        if: always()
        run: |
          shred -vfz -n 3 secrets.json
```

### Using Age for Encryption

```bash
# Install age
sudo apt-get install age

# Generate key
age-keygen -o key.txt

# Encrypt
age -r $(cat key.txt.pub) -o secret.age secret.txt

# Decrypt
age -d -i key.txt secret.age > secret.txt
```

## Environment Variables

### Secure Environment Setup

```yaml
name: Secure Environment

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - name: Setup secure environment
        uses: ./.github/actions/setup-secure-environment
        with:
          enable-gpg: 'true'
          cleanup-after: 'true'

      - name: Configure environment
        env:
          API_KEY: ${{ secrets.API_KEY }}
          SECRET_TOKEN: ${{ secrets.SECRET_TOKEN }}
        run: |
          # Environment variables are only available in this step
          ./configure --with-api-key="$API_KEY"
```

### Masking Secrets in Logs

```yaml
- name: Use secrets safely
  run: |
    # Register secret for masking (if not from GitHub Secrets)
    echo "::add-mask::$CUSTOM_SECRET"
    
    # Secrets from GitHub Secrets are automatically masked
    echo "API Key: ${{ secrets.API_KEY }}"  # Will appear as ***
```

## Best Practices

### DO's ✅

1. **Use GitHub Secrets for all sensitive data**
   ```yaml
   env:
     API_KEY: ${{ secrets.API_KEY }}
   ```

2. **Use environment-specific secrets**
   ```yaml
   environment: production
   ```

3. **Rotate secrets regularly**
   - Set calendar reminders
   - Document rotation in security log

4. **Use least privilege**
   - Only grant necessary permissions
   - Limit secret access to required workflows

5. **Audit secret usage**
   ```yaml
   - name: Audit secret access
     run: |
       echo "Secret accessed by: ${{ github.actor }}"
       echo "Workflow: ${{ github.workflow }}"
       echo "Time: $(date -u)"
   ```

6. **Clean up after use**
   ```yaml
   - name: Clean up secrets
     if: always()
     run: |
       unset API_KEY
       shred -vfz -n 3 credentials.json
   ```

### DON'Ts ❌

1. **Never hardcode secrets**
   ```bash
   # ❌ BAD
   API_KEY="sk_live_123456789"
   
   # ✅ GOOD
   API_KEY="${{ secrets.API_KEY }}"
   ```

2. **Never commit secrets**
   ```bash
   # Check before committing
   git diff | grep -E "(password|api[_-]?key|secret|token)"
   ```

3. **Never log secrets**
   ```bash
   # ❌ BAD
   echo "API Key: $API_KEY"
   
   # ✅ GOOD
   echo "API Key configured"
   ```

4. **Never use secrets in pull requests from forks**
   ```yaml
   # Secrets are not available in PR from forks by default
   if: github.event.pull_request.head.repo.full_name == github.repository
   ```

5. **Never store secrets in code comments**
   ```python
   # ❌ BAD
   # API_KEY = "sk_live_123456789"
   
   # ✅ GOOD
   # API_KEY loaded from environment variable
   ```

## Audit and Compliance

### Secret Access Logging

```yaml
name: Audit Secret Access

on:
  workflow_run:
    workflows: ["*"]
    types: [completed]

jobs:
  audit:
    runs-on: ubuntu-latest
    steps:
      - name: Log secret usage
        run: |
          cat >> AUDIT_LOG.md << EOF
          ## Workflow Run - $(date -u +"%Y-%m-%d %H:%M:%S")
          
          - **Workflow**: ${{ github.workflow }}
          - **Actor**: ${{ github.actor }}
          - **Event**: ${{ github.event_name }}
          - **Status**: ${{ github.event.workflow_run.conclusion }}
          - **Run ID**: ${{ github.run_id }}
          EOF
```

### Secret Scanning

Enable GitHub's secret scanning:

1. Go to `Settings → Code security and analysis`
2. Enable `Secret scanning`
3. Enable `Push protection`

### Compliance Checklist

- [ ] All secrets stored in GitHub Secrets
- [ ] No secrets in repository files
- [ ] Secret rotation schedule documented
- [ ] Access audit logging enabled
- [ ] Secret scanning enabled
- [ ] Push protection enabled
- [ ] Least privilege access configured
- [ ] Environment protection rules set
- [ ] Backup of encryption keys (offline)
- [ ] Incident response plan documented

## Emergency Procedures

### Secret Compromise Response

If a secret is compromised:

1. **Immediate Actions** (within 1 hour)
   - Revoke the compromised secret immediately
   - Generate and deploy new secret
   - Review access logs for unauthorized use

2. **Investigation** (within 24 hours)
   - Determine scope of compromise
   - Identify affected systems
   - Document timeline of events

3. **Remediation** (within 48 hours)
   - Rotate all related secrets
   - Update access controls
   - Notify affected parties

4. **Post-Incident** (within 1 week)
   - Conduct post-mortem analysis
   - Update security procedures
   - Implement preventive measures

### Emergency Contacts

```yaml
# Configure emergency notification
- name: Emergency notification
  if: failure()
  run: |
    # Send notification to security team
    curl -X POST "${{ secrets.EMERGENCY_WEBHOOK }}" \
      -H "Content-Type: application/json" \
      -d '{"text":"Security incident detected in workflow"}'
```

## Additional Resources

- [GitHub Secrets Documentation](https://docs.github.com/en/actions/security-guides/encrypted-secrets)
- [GitHub Security Best Practices](https://docs.github.com/en/actions/security-guides/security-hardening-for-github-actions)
- [Self-Hosted Runner Setup](SELF_HOSTED_RUNNER_SETUP.md)
- [Security Practices](SECURITY_PRACTICES.md)

---

**Last Updated**: 2026-02-18  
**Version**: 1.0.0  
**Maintained by**: kushmanmb.eth (Kushman MB)
