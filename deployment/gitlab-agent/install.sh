#!/usr/bin/env bash
# GitLab Agent Installation Script
# This script installs the GitLab agent for Kubernetes using Helm

set -e

# Configuration
AGENT_NAME="${GITLAB_AGENT_NAME:-kushbot801}"
NAMESPACE="${GITLAB_AGENT_NAMESPACE:-gitlab-agent-kushbot801}"
KAS_ADDRESS="${GITLAB_KAS_ADDRESS:-wss://kas.gitlab.com}"

# Check if token is provided via environment variable
if [ -z "$GITLAB_AGENT_TOKEN" ]; then
    echo "Error: GITLAB_AGENT_TOKEN environment variable is not set"
    echo "Please set it before running this script:"
    echo "  export GITLAB_AGENT_TOKEN='your-token-here'"
    echo ""
    echo "Example:"
    echo "  export GITLAB_AGENT_TOKEN='glagent-EXAMPLE_TOKEN_REPLACE_WITH_YOUR_ACTUAL_TOKEN'"
    echo "  ./install.sh"
    exit 1
fi

echo "Installing GitLab Agent for Kubernetes..."
echo "Agent name: $AGENT_NAME"
echo "Namespace: $NAMESPACE"
echo "KAS address: $KAS_ADDRESS"

# Add GitLab Helm repository
echo "Adding GitLab Helm repository..."
helm repo add gitlab https://charts.gitlab.io

# Update Helm repositories
echo "Updating Helm repositories..."
helm repo update

# Install or upgrade the GitLab agent
echo "Installing/Upgrading GitLab agent..."
helm upgrade --install "$AGENT_NAME" gitlab/gitlab-agent \
    --namespace "$NAMESPACE" \
    --create-namespace \
    --set config.token="$GITLAB_AGENT_TOKEN" \
    --set config.kasAddress="$KAS_ADDRESS"

echo "GitLab agent installation complete!"
echo "Namespace: $NAMESPACE"
echo "Agent name: $AGENT_NAME"
