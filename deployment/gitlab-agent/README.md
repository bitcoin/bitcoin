# GitLab Agent for Kubernetes

This directory contains the configuration and scripts for deploying the GitLab agent to a Kubernetes cluster.

## Overview

The GitLab agent for Kubernetes enables GitLab to interact with your Kubernetes cluster for CI/CD operations, deployments, and cluster management.

## Prerequisites

- Kubernetes cluster with `kubectl` configured
- Helm 3.x installed
- Appropriate permissions to create namespaces and deploy applications
- GitLab agent token (obtain from your GitLab project settings)
  - Navigate to: Project Settings > Infrastructure > Kubernetes clusters > GitLab Agent
  - Create a new agent or use an existing one
  - Copy the agent token for use in installation

## Installation

### Quick Start

1. Set your GitLab agent token as an environment variable:
   ```bash
   export GITLAB_AGENT_TOKEN='<YOUR_AGENT_TOKEN>'
   ```
   
   Alternatively, you can copy the example environment file and source it:
   ```bash
   cp .env.example .env
   # Edit .env and add your token
   source .env
   ```

2. Run the installation script:
   ```bash
   ./install.sh
   ```

### Custom Configuration

You can override the default configuration using environment variables:

```bash
export GITLAB_AGENT_TOKEN='<YOUR_AGENT_TOKEN>'
export GITLAB_AGENT_NAME='kushbot801'                      # Default: kushbot801
export GITLAB_AGENT_NAMESPACE='gitlab-agent-kushbot801'    # Default: gitlab-agent-kushbot801
export GITLAB_KAS_ADDRESS='wss://kas.gitlab.com'           # Default: wss://kas.gitlab.com
./install.sh
```

### Manual Installation

If you prefer to run the commands manually:

```bash
# Add GitLab Helm repository
helm repo add gitlab https://charts.gitlab.io

# Update Helm repositories
helm repo update

# Install the GitLab agent
helm upgrade --install kushbot801 gitlab/gitlab-agent \
    --namespace gitlab-agent-kushbot801 \
    --create-namespace \
    --set config.token='<YOUR_AGENT_TOKEN>' \
    --set config.kasAddress=wss://kas.gitlab.com
```

**Important:** Replace `<YOUR_AGENT_TOKEN>` with your actual GitLab agent token.

## Configuration

The agent is configured with the following settings:

- **Agent Name**: `kushbot801`
- **Namespace**: `gitlab-agent-kushbot801`
- **KAS Address**: `wss://kas.gitlab.com`
- **Token**: Configured via Helm values (stored securely)

## Verification

After installation, verify the agent is running:

```bash
# Check the agent pod status
kubectl get pods -n gitlab-agent-kushbot801

# Check the agent logs
kubectl logs -n gitlab-agent-kushbot801 -l app=gitlab-agent
```

## Updating

To update the agent configuration or upgrade to a new version, simply run the installation script again or execute the `helm upgrade` command with updated values.

## Uninstallation

### Quick Start

Run the uninstallation script:

```bash
./uninstall.sh
```

### Manual Uninstallation

To remove the GitLab agent manually:

```bash
helm uninstall kushbot801 -n gitlab-agent-kushbot801
kubectl delete namespace gitlab-agent-kushbot801
```

## Security Considerations

- The agent token should be kept secure and rotated regularly
- Access to the namespace should be restricted using Kubernetes RBAC
- Review the agent's permissions and ensure they follow the principle of least privilege

## Additional Resources

- [GitLab Agent for Kubernetes Documentation](https://docs.gitlab.com/ee/user/clusters/agent/)
- [Helm Chart Repository](https://charts.gitlab.io)
