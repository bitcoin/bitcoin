#!/usr/bin/env python3
"""
Example script demonstrating how to use the user_access.yml configuration

This script shows various ways to interact with the user access configuration.
"""

import yaml
from pathlib import Path
from typing import Dict, List, Any

def load_config() -> Dict[str, Any]:
    """Load the user access configuration"""
    config_path = Path(__file__).parent / 'user_access.yml'
    with open(config_path, 'r') as f:
        return yaml.safe_load(f)

def check_permission(role_name: str, permission: str, config: Dict[str, Any]) -> bool:
    """Check if a role has a specific permission"""
    role = config['user_access']['access_as'].get(role_name)
    if not role:
        return False
    return permission in role['permissions']

def can_access_project(tier: str, project_id: str, config: Dict[str, Any]) -> bool:
    """Check if a tier can access a specific project"""
    projects = config['user_access']['projects']
    project = next((p for p in projects if p['id'] == project_id), None)
    
    if not project:
        return False
    
    if project['access_level'] == 'all':
        return True
    
    # Premium projects require premium+ tier
    return tier == 'premium+'

def get_accessible_projects(tier: str, config: Dict[str, Any]) -> List[Dict[str, Any]]:
    """Get all projects accessible by a given tier"""
    projects = config['user_access']['projects']
    accessible = []
    
    for project in projects:
        if project['access_level'] == 'all':
            accessible.append(project)
        elif tier == 'premium+':
            accessible.append(project)
    
    return accessible

def get_rate_limits(role_name: str, config: Dict[str, Any]) -> Dict[str, int]:
    """Get rate limits for a role"""
    role = config['user_access']['access_as'].get(role_name)
    if not role:
        return {}
    return role['rate_limits']

def main():
    """Demonstrate configuration usage"""
    print("User Access Configuration Example")
    print("=" * 60)
    
    # Load configuration
    config = load_config()
    print("\n✅ Configuration loaded successfully\n")
    
    # Display roles
    print("Available Roles:")
    print("-" * 60)
    for role_name, role_data in config['user_access']['access_as'].items():
        print(f"\n{role_name.upper()} ({role_data['tier']})")
        print(f"  Description: {role_data['description']}")
        print(f"  Permissions: {', '.join(role_data['permissions'])}")
        print(f"  Rate Limits:")
        print(f"    - Requests/hour: {role_data['rate_limits']['requests_per_hour']}")
        print(f"    - Concurrent: {role_data['rate_limits']['concurrent_requests']}")
    
    # Display projects
    print("\n\nAvailable Projects:")
    print("-" * 60)
    for project in config['user_access']['projects']:
        access_icon = "🌍" if project['access_level'] == 'all' else "💎"
        print(f"{access_icon} {project['name']} ({project['id']})")
        print(f"  Access Level: {project['access_level']}")
        print(f"  Description: {project['description']}\n")
    
    # Example permission checks
    print("\nPermission Checks:")
    print("-" * 60)
    
    # Agent permissions
    print("\nAgent role:")
    print(f"  - Can read: {check_permission('agent', 'read', config)}")
    print(f"  - Can write: {check_permission('agent', 'write', config)}")
    print(f"  - Can access analytics: {check_permission('agent', 'analytics', config)}")
    
    # User permissions
    print("\nUser role:")
    print(f"  - Can read: {check_permission('user', 'read', config)}")
    print(f"  - Can write: {check_permission('user', 'write', config)}")
    print(f"  - Can access analytics: {check_permission('user', 'analytics', config)}")
    
    # Project access checks
    print("\n\nProject Access Checks:")
    print("-" * 60)
    
    # Free tier (agent)
    print("\nFree tier can access:")
    free_projects = get_accessible_projects('free', config)
    for project in free_projects:
        print(f"  ✓ {project['name']}")
    
    # Premium tier (user)
    print("\nPremium+ tier can access:")
    premium_projects = get_accessible_projects('premium+', config)
    for project in premium_projects:
        print(f"  ✓ {project['name']}")
    
    # Rate limits
    print("\n\nRate Limits:")
    print("-" * 60)
    agent_limits = get_rate_limits('agent', config)
    user_limits = get_rate_limits('user', config)
    
    print(f"\nAgent: {agent_limits['requests_per_hour']} req/hour, "
          f"{agent_limits['concurrent_requests']} concurrent")
    print(f"User: {user_limits['requests_per_hour']} req/hour, "
          f"{user_limits['concurrent_requests']} concurrent")
    
    print("\n" + "=" * 60)
    print("Example complete!\n")

if __name__ == '__main__':
    main()
