#!/usr/bin/env bash
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
helm uninstall "$AGENT_NAME" -n "$NAMESPACE" || {
    echo "Warning: Failed to uninstall Helm release. It may have already been removed."
}

# Delete the namespace
echo "Deleting namespace..."
echo "Note: Namespace deletion may take time as resources are terminated."
kubectl delete namespace "$NAMESPACE" --timeout=60s || {
    echo "Warning: Namespace deletion timed out or failed."
    echo "Resources may still be terminating. Check with:"
    echo "  kubectl get namespace $NAMESPACE"
    echo "  kubectl get all -n $NAMESPACE"
    exit 1
}

echo "GitLab agent uninstallation complete!"
