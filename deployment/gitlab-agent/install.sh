#!/bin/bash
# GitLab Agent Installation Script
# This script installs the GitLab agent for Kubernetes using Helm

set -e

echo "Installing GitLab Agent for Kubernetes..."

# Add GitLab Helm repository
echo "Adding GitLab Helm repository..."
helm repo add gitlab https://charts.gitlab.io

# Update Helm repositories
echo "Updating Helm repositories..."
helm repo update

# Install or upgrade the GitLab agent
echo "Installing/Upgrading GitLab agent..."
helm upgrade --install kushbot801 gitlab/gitlab-agent \
    --namespace gitlab-agent-kushbot801 \
    --create-namespace \
    --set config.token=glagent-7Z5SUNjeFVdmGzhx9kTe4m86MQpwOjFiZGo4dww.01.130dzsxzk \
    --set config.kasAddress=wss://kas.gitlab.com

echo "GitLab agent installation complete!"
echo "Namespace: gitlab-agent-kushbot801"
echo "Agent name: kushbot801"
