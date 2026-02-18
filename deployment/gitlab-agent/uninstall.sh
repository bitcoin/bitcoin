#!/bin/bash
# GitLab Agent Uninstallation Script
# This script removes the GitLab agent from Kubernetes

set -e

echo "Uninstalling GitLab Agent from Kubernetes..."

# Uninstall the GitLab agent using Helm
echo "Removing Helm release..."
helm uninstall kushbot801 -n gitlab-agent-kushbot801

# Delete the namespace
echo "Deleting namespace..."
kubectl delete namespace gitlab-agent-kushbot801

echo "GitLab agent uninstallation complete!"
