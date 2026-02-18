# Configuration Files

This directory contains configuration files for repository-wide settings and access control.

## Available Configurations

### user_access.yml

Defines user access levels and project permissions for the repository.

#### Structure

```yaml
user_access:
  access_as:
    agent:    # Free tier access
    user:     # Premium+ tier access
  projects:   # Project-specific access controls
    - id: ... # Project identifier
```

#### Access Roles

**Agent Role (Free Tier)**
- **Tier**: `free`
- **Description**: Free tier access for automated agents and basic users
- **Permissions**: 
  - `read`: Read access to public data
  - `basic_queries`: Execute basic queries
  - `public_data_access`: Access to public data only
- **Rate Limits**:
  - 100 requests per hour
  - 5 concurrent requests

**User Role (Premium+ Tier)**
- **Tier**: `premium+`
- **Description**: Premium tier access for authenticated users with full features
- **Permissions**:
  - `read`: Read access
  - `write`: Write access
  - `advanced_queries`: Execute advanced queries
  - `private_data_access`: Access to private data
  - `api_access`: Full API access
  - `analytics`: Access to analytics features
- **Rate Limits**:
  - 1000 requests per hour
  - 20 concurrent requests

#### Project Access Levels

Projects can have one of the following access levels:
- **`all`**: Available to all users (free and premium)
- **`premium`**: Available only to premium+ users

#### Current Projects

1. **bitcoin-core**: Main Bitcoin Core project (all users)
2. **onchain-agent**: AI-powered blockchain agent scaffolding tool (all users)
3. **cdp-api-integration**: Coinbase Developer Platform API integration (premium only)
4. **ens-integration**: Ethereum Name Service integration (premium only)
5. **private-workflows**: Private CI/CD workflows and automation (premium only)

## Usage

### Loading Configuration (Python)

```python
import yaml

with open('.github/configs/user_access.yml', 'r') as f:
    config = yaml.safe_load(f)
    
# Access agent permissions
agent_perms = config['user_access']['access_as']['agent']['permissions']

# Get all premium projects
premium_projects = [
    p for p in config['user_access']['projects'] 
    if p['access_level'] == 'premium'
]
```

### Loading Configuration (Node.js/TypeScript)

```typescript
import * as yaml from 'js-yaml';
import * as fs from 'fs';

interface UserAccessConfig {
  user_access: {
    access_as: {
      agent: AccessRole;
      user: AccessRole;
    };
    projects: Project[];
  };
}

interface AccessRole {
  tier: string;
  description: string;
  permissions: string[];
  rate_limits: {
    requests_per_hour: number;
    concurrent_requests: number;
  };
}

interface Project {
  id: string;
  name: string;
  description: string;
  access_level: 'all' | 'premium';
}

const config = yaml.load(
  fs.readFileSync('.github/configs/user_access.yml', 'utf8')
) as UserAccessConfig;
```

### Validating Configuration

```bash
# Validate YAML syntax
python3 -c "import yaml; yaml.safe_load(open('.github/configs/user_access.yml'))"

# Or using yq (if installed)
yq eval '.github/configs/user_access.yml'
```

## Modifying Configuration

To add new projects or modify access levels:

1. Edit `user_access.yml`
2. Ensure YAML syntax is valid
3. Update this README if adding new project types or access levels
4. Commit changes and create a pull request

## Security Considerations

- This configuration file defines access controls and should be carefully reviewed
- Changes to access levels should be reviewed by repository maintainers
- Rate limits help prevent abuse and ensure fair resource usage
- Premium features require proper authentication and authorization mechanisms
