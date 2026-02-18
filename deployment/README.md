# Deployment Documentation

This directory contains deployment configurations and scripts for the Bitcoin Core project infrastructure.

## Directory Structure

```
deployment/
├── gitlab-agent/       # GitLab Agent for Kubernetes installation
│   ├── README.md      # Detailed GitLab agent documentation
│   └── install.sh     # Automated installation script
```

## GitLab Agent

The GitLab agent enables integration between GitLab and Kubernetes clusters, allowing for:
- CI/CD pipeline execution in Kubernetes
- GitOps deployments
- Cluster management and monitoring
- Security scanning integration

See [gitlab-agent/README.md](gitlab-agent/README.md) for detailed installation instructions.

## Quick Start

### GitLab Agent Installation

```bash
cd deployment/gitlab-agent
./install.sh
```

## Prerequisites

Before deploying any components, ensure you have:
- Access to a Kubernetes cluster
- `kubectl` configured with appropriate credentials
- Helm 3.x installed
- Necessary permissions to create namespaces and deploy applications

## Security Notes

- All sensitive data (tokens, credentials) should be managed through Kubernetes secrets or secure credential management systems
- Review and understand security implications before deploying to production
- Follow the principle of least privilege when configuring service accounts and RBAC

## Contributing

When adding new deployment configurations:
1. Create a new subdirectory with a descriptive name
2. Include a comprehensive README.md
3. Provide installation/deployment scripts where applicable
4. Document all configuration options and prerequisites
5. Include uninstallation instructions
