/**
 * Example Node.js/TypeScript script demonstrating how to use the user_access.yml configuration
 * 
 * To run this example:
 * 1. Install dependencies: npm install js-yaml @types/js-yaml
 * 2. Run with ts-node: npx ts-node example_usage.ts
 * 3. Or compile and run: tsc example_usage.ts && node example_usage.js
 */

import * as yaml from 'js-yaml';
import * as fs from 'fs';
import * as path from 'path';
import {
  UserAccessConfig,
  AccessRole,
  Project,
  Permission,
  AccessTier,
  hasPermission,
  canAccessProject,
  getAccessibleProjects,
  isWithinRateLimit
} from '../../bitcoin-onchain-app/app/userAccessTypes';

/**
 * Load the user access configuration
 */
function loadConfig(): UserAccessConfig {
  const configPath = path.join(__dirname, 'user_access.yml');
  const fileContents = fs.readFileSync(configPath, 'utf8');
  return yaml.load(fileContents) as UserAccessConfig;
}

/**
 * Display role information
 */
function displayRole(roleName: string, role: AccessRole): void {
  console.log(`\n${roleName.toUpperCase()} (${role.tier})`);
  console.log(`  Description: ${role.description}`);
  console.log(`  Permissions: ${role.permissions.join(', ')}`);
  console.log(`  Rate Limits:`);
  console.log(`    - Requests/hour: ${role.rate_limits.requests_per_hour}`);
  console.log(`    - Concurrent: ${role.rate_limits.concurrent_requests}`);
}

/**
 * Display project information
 */
function displayProject(project: Project): void {
  const accessIcon = project.access_level === 'all' ? '🌍' : '💎';
  console.log(`${accessIcon} ${project.name} (${project.id})`);
  console.log(`  Access Level: ${project.access_level}`);
  console.log(`  Description: ${project.description}\n`);
}

/**
 * Main example function
 */
function main(): void {
  console.log('User Access Configuration Example (TypeScript)');
  console.log('='.repeat(60));

  // Load configuration
  const config = loadConfig();
  console.log('\n✅ Configuration loaded successfully\n');

  // Display roles
  console.log('Available Roles:');
  console.log('-'.repeat(60));
  const { agent, user } = config.user_access.access_as;
  displayRole('agent', agent);
  displayRole('user', user);

  // Display projects
  console.log('\n\nAvailable Projects:');
  console.log('-'.repeat(60));
  config.user_access.projects.forEach(displayProject);

  // Example permission checks
  console.log('\nPermission Checks:');
  console.log('-'.repeat(60));

  // Agent permissions
  console.log('\nAgent role:');
  console.log(`  - Can read: ${hasPermission(agent, 'read')}`);
  console.log(`  - Can write: ${hasPermission(agent, 'write')}`);
  console.log(`  - Can access analytics: ${hasPermission(agent, 'analytics')}`);

  // User permissions
  console.log('\nUser role:');
  console.log(`  - Can read: ${hasPermission(user, 'read')}`);
  console.log(`  - Can write: ${hasPermission(user, 'write')}`);
  console.log(`  - Can access analytics: ${hasPermission(user, 'analytics')}`);

  // Project access checks
  console.log('\n\nProject Access Checks:');
  console.log('-'.repeat(60));

  // Free tier (agent)
  console.log('\nFree tier can access:');
  const freeProjects = getAccessibleProjects('free', config.user_access.projects);
  freeProjects.forEach(project => {
    console.log(`  ✓ ${project.name}`);
  });

  // Premium tier (user)
  console.log('\nPremium+ tier can access:');
  const premiumProjects = getAccessibleProjects('premium+', config.user_access.projects);
  premiumProjects.forEach(project => {
    console.log(`  ✓ ${project.name}`);
  });

  // Rate limits
  console.log('\n\nRate Limits:');
  console.log('-'.repeat(60));

  console.log(`\nAgent: ${agent.rate_limits.requests_per_hour} req/hour, ` +
              `${agent.rate_limits.concurrent_requests} concurrent`);
  console.log(`User: ${user.rate_limits.requests_per_hour} req/hour, ` +
              `${user.rate_limits.concurrent_requests} concurrent`);

  // Rate limit compliance example
  console.log('\n\nRate Limit Compliance Examples:');
  console.log('-'.repeat(60));
  console.log(`\nAgent with 50 requests: ${isWithinRateLimit(50, agent.rate_limits) ? '✓' : '✗'} Within limit`);
  console.log(`Agent with 150 requests: ${isWithinRateLimit(150, agent.rate_limits) ? '✓' : '✗'} Exceeds limit`);
  console.log(`User with 500 requests: ${isWithinRateLimit(500, user.rate_limits) ? '✓' : '✗'} Within limit`);
  console.log(`User with 1500 requests: ${isWithinRateLimit(1500, user.rate_limits) ? '✓' : '✗'} Exceeds limit`);

  console.log('\n' + '='.repeat(60));
  console.log('Example complete!\n');
}

// Run if executed directly
if (require.main === module) {
  try {
    main();
  } catch (error) {
    console.error('Error:', error);
    process.exit(1);
  }
}

export { main };
