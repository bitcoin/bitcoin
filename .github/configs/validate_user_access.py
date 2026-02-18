#!/usr/bin/env python3
"""
Validate user_access.yml configuration file

This script validates the structure and content of the user access configuration.
It checks:
- YAML syntax
- Required fields
- Valid access levels
- Consistent project IDs
- Rate limit values
"""

import yaml
import sys
from pathlib import Path

def validate_access_role(role_name, role_data):
    """Validate an access role configuration"""
    errors = []
    
    # Check required fields
    required_fields = ['tier', 'description', 'permissions', 'rate_limits']
    for field in required_fields:
        if field not in role_data:
            errors.append(f"Role '{role_name}' missing required field: {field}")
    
    # Validate tier values
    if 'tier' in role_data:
        valid_tiers = ['free', 'premium+']
        if role_data['tier'] not in valid_tiers:
            errors.append(f"Role '{role_name}' has invalid tier: {role_data['tier']}")
    
    # Validate permissions is a list
    if 'permissions' in role_data:
        if not isinstance(role_data['permissions'], list):
            errors.append(f"Role '{role_name}' permissions must be a list")
        else:
            # Validate permission values against expected permissions
            valid_permissions = [
                'read', 'write', 'basic_queries', 'advanced_queries',
                'public_data_access', 'private_data_access', 'api_access', 'analytics'
            ]
            for permission in role_data['permissions']:
                if permission not in valid_permissions:
                    errors.append(f"Role '{role_name}' has invalid permission: {permission}")
    
    # Validate rate_limits structure
    if 'rate_limits' in role_data:
        rate_limits = role_data['rate_limits']
        if not isinstance(rate_limits, dict):
            errors.append(f"Role '{role_name}' rate_limits must be a dict")
        else:
            required_limits = ['requests_per_hour', 'concurrent_requests']
            for limit in required_limits:
                if limit not in rate_limits:
                    errors.append(f"Role '{role_name}' rate_limits missing: {limit}")
                elif not isinstance(rate_limits[limit], int) or rate_limits[limit] <= 0:
                    errors.append(f"Role '{role_name}' rate_limits.{limit} must be a positive integer")
    
    return errors

def validate_project(project, index):
    """Validate a project configuration"""
    errors = []
    
    # Check required fields
    required_fields = ['id', 'name', 'description', 'access_level']
    for field in required_fields:
        if field not in project:
            errors.append(f"Project {index} missing required field: {field}")
    
    # Validate access_level
    if 'access_level' in project:
        valid_levels = ['all', 'premium']
        if project['access_level'] not in valid_levels:
            errors.append(f"Project {index} ({project.get('id', 'unknown')}) has invalid access_level: {project['access_level']}")
    
    # Validate id format (basic check)
    if 'id' in project:
        project_id = project['id']
        if not isinstance(project_id, str) or not project_id:
            errors.append(f"Project {index} has invalid id: {project_id}")
        elif not project_id.replace('-', '').replace('_', '').isalnum():
            errors.append(f"Project {index} id should only contain alphanumeric characters, hyphens, and underscores: {project_id}")
    
    return errors

def validate_config(config_path):
    """Validate the entire configuration file"""
    errors = []
    
    # Check file exists
    if not config_path.exists():
        return [f"Configuration file not found: {config_path}"]
    
    # Parse YAML
    try:
        with open(config_path, 'r') as f:
            config = yaml.safe_load(f)
    except yaml.YAMLError as e:
        return [f"YAML parsing error: {e}"]
    except Exception as e:
        return [f"Error reading file: {e}"]
    
    # Check root structure
    if not isinstance(config, dict):
        return ["Configuration must be a dictionary"]
    
    if 'user_access' not in config:
        return ["Configuration missing 'user_access' root key"]
    
    user_access = config['user_access']
    
    # Validate access_as section
    if 'access_as' not in user_access:
        errors.append("Missing 'access_as' section")
    else:
        access_as = user_access['access_as']
        
        # Check required roles
        required_roles = ['agent', 'user']
        for role_name in required_roles:
            if role_name not in access_as:
                errors.append(f"Missing required role: {role_name}")
            else:
                errors.extend(validate_access_role(role_name, access_as[role_name]))
    
    # Validate projects section
    if 'projects' not in user_access:
        errors.append("Missing 'projects' section")
    else:
        projects = user_access['projects']
        
        if not isinstance(projects, list):
            errors.append("'projects' must be a list")
        else:
            # Validate each project
            project_ids = set()
            for i, project in enumerate(projects):
                errors.extend(validate_project(project, i))
                
                # Check for duplicate IDs
                if 'id' in project:
                    if project['id'] in project_ids:
                        errors.append(f"Duplicate project ID: {project['id']}")
                    project_ids.add(project['id'])
    
    return errors

def main():
    """Main validation function"""
    # Find config file
    repo_root = Path(__file__).parent.parent.parent
    config_path = repo_root / '.github' / 'configs' / 'user_access.yml'
    
    print(f"Validating: {config_path}")
    print("-" * 60)
    
    # Run validation
    errors = validate_config(config_path)
    
    if errors:
        print(f"❌ Validation failed with {len(errors)} error(s):\n")
        for i, error in enumerate(errors, 1):
            print(f"{i}. {error}")
        return 1
    else:
        print("✅ Validation successful!")
        print("\nConfiguration summary:")
        
        # Print summary
        with open(config_path, 'r') as f:
            config = yaml.safe_load(f)
            access_as = config['user_access']['access_as']
            projects = config['user_access']['projects']
            
            print(f"  - Roles defined: {', '.join(access_as.keys())}")
            print(f"  - Projects: {len(projects)}")
            print(f"  - Public projects: {sum(1 for p in projects if p['access_level'] == 'all')}")
            print(f"  - Premium projects: {sum(1 for p in projects if p['access_level'] == 'premium')}")
        
        return 0

if __name__ == '__main__':
    sys.exit(main())
