#!/bin/bash
# GitLab Agent Uninstallation Script
# This script removes the GitLab agent from Kubernetes

set -e

# Configuration
AGENT_NAME="${GITLAB_AGENT_NAME:-kushbot801}"
NAMESPACE="${GITLAB_AGENT_NAMESPACE:-gitlab-agent-kushbot801}"

echo "Uninstalling GitLab Agent from Kubernetes..."
echo "Agent name: $AGENT_NAME"
echo "Namespace: $NAMESPACE"

# Uninstall the GitLab agent using Helm
echo "Removing Helm release..."
helm uninstall "$AGENT_NAME" -n "$NAMESPACE"

# Delete the namespace
echo "Deleting namespace..."
kubectl delete namespace "$NAMESPACE"

echo "GitLab agent uninstallation complete!"
