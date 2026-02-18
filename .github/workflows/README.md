# Automated Validators and Workflows

## Overview

This directory contains automated validator actions and workflows for ensuring code quality, security, and proper build processes in the Bitcoin Core repository. All workflows support both self-hosted and GitHub-hosted runners.

## Validator Actions

### 1. Code Quality Validator (`validate-code-quality`)

**Purpose**: Automated code quality validation with multiple checks

**Features**:
- Code formatting checks (clang-format)
- Common issues detection (trailing whitespace, tabs, debug statements)
- File permissions validation
- Customizable failure on warnings

**Usage**:
```yaml
- name: Run code quality validation
  uses: ./.github/actions/validate-code-quality
  with:
    path: '.'
    fail-on-warning: 'false'
```

### 2. Security Validator (`validate-security`)

**Purpose**: Automated security validation checks

**Features**:
- Hardcoded secrets detection
- File permissions and ownership checks
- Secure coding pattern validation
- Unsafe C functions detection
- SQL injection risk detection
- Dependency security checks

**Usage**:
```yaml
- name: Run security validation
  uses: ./.github/actions/validate-security
  with:
    check-secrets: 'true'
    check-dependencies: 'true'
```

### 3. Dependency Validator (`validate-dependencies`)

**Purpose**: Check dependencies for security vulnerabilities and compatibility

**Features**:
- Python dependency scanning (safety)
- Node.js dependency auditing (npm audit)
- Maven dependency analysis
- License compliance checking
- Outdated dependency detection

**Usage**:
```yaml
- name: Run dependency validation
  uses: ./.github/actions/validate-dependencies
  with:
    fail-on-vulnerability: 'true'
```

### 4. Commit Message Validator (`validate-commit-messages`)

**Purpose**: Validate commit messages follow conventional commit format

**Features**:
- Conventional commit format validation
- Subject line length checking
- Imperative mood suggestions
- Customizable pattern matching

**Usage**:
```yaml
- name: Validate commit messages
  uses: ./.github/actions/validate-commit-messages
  with:
    pattern: '^(feat|fix|docs|style|refactor|perf|test|build|ci|chore|revert)(\(.+\))?: .{1,72}'
```

### 5. Secure Environment Setup (`setup-secure-environment`)

**Purpose**: Configure secure environment with proper key management and secrets handling

**Features**:
- Environment security validation
- Secure temporary directory creation
- GPG configuration for secure operations
- SSH security setup
- Secure file handling functions
- Secret detection setup

**Usage**:
```yaml
- name: Setup secure environment
  uses: ./.github/actions/setup-secure-environment
  with:
    enable-gpg: 'true'
    cleanup-after: 'true'
```

## Workflows

### 1. Automated Validators (`automated-validators.yml`)

**Purpose**: Run all automated validators on code changes

**Triggers**:
- Push to main/master/develop branches
- Pull requests to main/master/develop
- Manual workflow dispatch

**Jobs**:
- `validate-code-quality`: Run code quality checks
- `validate-security`: Run security scans
- `validate-dependencies`: Check dependency vulnerabilities
- `validate-commits`: Validate commit message format
- `validation-summary`: Generate overall validation report

**Configuration**:
```yaml
# Enable self-hosted runners
vars.USE_SELF_HOSTED: 'true'
```

### 2. Test Suite (`test-suite.yml`)

**Purpose**: Comprehensive test execution with self-hosted runner support

**Triggers**:
- Push to main/master/develop
- Pull requests
- Manual workflow dispatch with test level selection

**Jobs**:
- `test-setup`: Determine test matrix
- `unit-tests`: Run C++ and Python unit tests
- `lint-tests`: Run code linting (flake8, pylint, shellcheck, codespell)
- `functional-tests`: Run functional test suite
- `test-summary`: Generate test report

**Manual Execution**:
```yaml
# Select test level: unit, integration, or all
workflow_dispatch:
  inputs:
    test-level: 'all'
```

### 3. Lint and Build (`lint-and-build.yml`)

**Purpose**: Comprehensive linting and build validation

**Triggers**:
- Push to main/master/develop
- Pull requests
- Manual workflow dispatch

**Jobs**:
- `lint-cpp`: C++ linting (clang-format, clang-tidy, cppcheck)
- `lint-python`: Python linting (black, isort, flake8, pylint)
- `lint-shell`: Shell script linting (shellcheck)
- `build-linux`: Linux build with CMake and ccache
- `build-documentation`: Build project documentation
- `lint-build-summary`: Generate summary report

**Features**:
- Parallel linting for different languages
- Ccache support for faster rebuilds
- Build artifact uploads
- Documentation generation

### 4. Runner Health Check (`runner-health-check.yml`)

**Purpose**: Monitor self-hosted runner infrastructure health

**Triggers**:
- Scheduled (every 6 hours)
- Manual workflow dispatch

**Jobs**:
- `check-self-hosted-runners`: Health checks for self-hosted runners
  - Connectivity verification
  - Disk space monitoring
  - Memory usage checking
  - CPU load monitoring
  - Docker availability check
  - Network connectivity tests
- `github-hosted-fallback-test`: Verify GitHub-hosted fallback
- `health-summary`: Overall health status

**Monitoring**:
- Automatic alerts for resource issues
- Regular health reports
- Fallback verification

## Configuration

### Self-Hosted Runner Setup

To enable self-hosted runners for all workflows:

1. **Set Repository Variable**:
   - Navigate to: `Settings → Secrets and variables → Actions → Variables`
   - Create variable: `USE_SELF_HOSTED` with value `true`

2. **Configure Runners**:
   - Follow [SELF_HOSTED_RUNNER_SETUP.md](../../../SELF_HOSTED_RUNNER_SETUP.md)
   - Ensure runners have appropriate labels: `self-hosted`, `linux`, etc.

3. **Verify Setup**:
   - Run the `Runner Health Check` workflow manually
   - Check the health report in workflow summary

### Secrets Configuration

Configure required secrets in repository settings:

```yaml
# Required Secrets (Settings → Secrets and variables → Actions → Secrets)
GITHUB_TOKEN              # Auto-provided by GitHub
GPG_PRIVATE_KEY          # For signed commits (optional)
DEPLOY_SSH_KEY           # For deployments (optional)

# Optional Secrets for validators
NPM_TOKEN                # For npm package publishing
DOCKER_PASSWORD          # For Docker registry access
```

See [KEY_MANAGEMENT_GUIDE.md](../../../KEY_MANAGEMENT_GUIDE.md) for detailed secrets management.

### Environment Variables

```yaml
# Common environment variables used in workflows
PYTHON_VERSION: '3.11'
NODE_VERSION: '20'
CMAKE_BUILD_TYPE: 'Release'
CCACHE_DIR: '${{ github.workspace }}/.ccache'
```

## Usage Examples

### Running Validators Locally

```bash
# Code quality check
find . -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run

# Security check for hardcoded secrets
git ls-files | xargs grep -E "password|api[_-]?key|secret"

# Dependency check
npm audit
pip install safety && safety check
```

### Testing Workflows Locally

Using [act](https://github.com/nektos/act):

```bash
# Install act
curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash

# Run automated validators workflow
act push -W .github/workflows/automated-validators.yml

# Run specific job
act -j validate-security

# Run with secrets
act --secret-file .secrets
```

### Triggering Workflows Manually

```bash
# Using GitHub CLI
gh workflow run automated-validators.yml

# Run test suite with specific level
gh workflow run test-suite.yml -f test-level=all

# Run runner health check
gh workflow run runner-health-check.yml
```

## Integration with Existing CI

These workflows complement the existing CI system:

1. **Existing CI** (`ci.yml`): Main build and test pipeline
2. **CodeQL** (`codeql.yml`): Security analysis
3. **New Validators**: Additional quality and security checks

All workflows can run in parallel and provide comprehensive validation.

## Troubleshooting

### Common Issues

#### Validator Failing

1. Check validator output in workflow logs
2. Review specific validation errors
3. Fix issues or adjust validator settings

#### Self-Hosted Runner Not Found

1. Verify `USE_SELF_HOSTED` variable is set
2. Check runner status in Settings → Actions → Runners
3. Verify runner has correct labels
4. Check runner health with health check workflow

#### Secrets Not Available

1. Verify secrets are configured in repository settings
2. Check secret names match workflow references
3. Ensure workflow has access to secrets (not from fork)

### Debug Mode

Enable debug logging:

```yaml
# Add to workflow file
env:
  ACTIONS_STEP_DEBUG: true
  ACTIONS_RUNNER_DEBUG: true
```

Or enable via GitHub UI:
1. Re-run workflow
2. Check "Enable debug logging"

## Best Practices

### Validator Configuration

1. **Start with warnings**: Set `fail-on-warning: 'false'` initially
2. **Gradually tighten**: Move to `fail-on-warning: 'true'` as issues are fixed
3. **Customize patterns**: Adjust validation patterns for your project
4. **Monitor regularly**: Review validation reports weekly

### Workflow Optimization

1. **Use ccache**: Enable ccache for faster builds
2. **Parallel jobs**: Use matrix builds for parallel execution
3. **Conditional execution**: Skip unnecessary jobs based on changed files
4. **Artifact retention**: Set appropriate retention days (7-30 days)

### Security

1. **Never log secrets**: Always use masked secrets
2. **Rotate keys**: Follow KEY_MANAGEMENT_GUIDE.md rotation schedule
3. **Audit access**: Review secret access logs regularly
4. **Use environments**: Protect production secrets with environment rules

## Contributing

When adding new validators or workflows:

1. Follow existing patterns for consistency
2. Add comprehensive documentation
3. Include usage examples
4. Test thoroughly with both runner types
5. Update this README

## Additional Resources

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Self-Hosted Runner Setup Guide](../../../SELF_HOSTED_RUNNER_SETUP.md)
- [Key Management Guide](../../../KEY_MANAGEMENT_GUIDE.md)
- [Security Practices](../../../SECURITY_PRACTICES.md)
- [Contributing Guide](../../../CONTRIBUTING.md)

---

**Last Updated**: 2026-02-18  
**Version**: 1.0.0  
**Maintained by**: kushmanmb.eth (Kushman MB)
