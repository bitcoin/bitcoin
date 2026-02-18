/**
 * TypeScript types for user access configuration
 * 
 * This file provides type definitions for the user_access.yml configuration file.
 * These types enable type-safe access to the configuration in TypeScript applications.
 * 
 * Configuration file location: .github/configs/user_access.yml
 */

/**
 * Rate limit settings for an access role
 */
export interface RateLimits {
  requests_per_hour: number;
  concurrent_requests: number;
}

/**
 * Access role definition (agent or user)
 */
export interface AccessRole {
  tier: 'free' | 'premium+';
  description: string;
  permissions: string[];
  rate_limits: RateLimits;
}

/**
 * Project definition with access control
 */
export interface Project {
  id: string;
  name: string;
  description: string;
  access_level: 'all' | 'premium';
}

/**
 * Access configuration structure
 */
export interface AccessConfig {
  agent: AccessRole;
  user: AccessRole;
}

/**
 * Root user access configuration
 */
export interface UserAccessConfig {
  user_access: {
    access_as: AccessConfig;
    projects: Project[];
  };
}

/**
 * Permission types available in the system
 * 
 * NOTE: These permission values must match the permissions defined in the YAML configuration.
 * If you add or modify permissions in .github/configs/user_access.yml, update this type accordingly.
 * The validation script (.github/configs/validate_user_access.py) enforces this consistency.
 */
export type Permission =
  | 'read'
  | 'write'
  | 'basic_queries'
  | 'advanced_queries'
  | 'public_data_access'
  | 'private_data_access'
  | 'api_access'
  | 'analytics';

/**
 * Access tier types
 */
export type AccessTier = 'free' | 'premium+';

/**
 * Project access level types
 */
export type ProjectAccessLevel = 'all' | 'premium';

/**
 * Helper function to check if a role has a specific permission
 */
export function hasPermission(role: AccessRole, permission: Permission): boolean {
  return role.permissions.includes(permission);
}

/**
 * Helper function to check if a user tier can access a project
 */
export function canAccessProject(tier: AccessTier, project: Project): boolean {
  if (project.access_level === 'all') {
    return true;
  }
  return tier === 'premium+';
}

/**
 * Helper function to get projects accessible by a tier
 */
export function getAccessibleProjects(
  tier: AccessTier,
  projects: Project[]
): Project[] {
  return projects.filter(project => canAccessProject(tier, project));
}

/**
 * Helper function to check rate limit compliance
 */
export function isWithinRateLimit(
  requestCount: number,
  rateLimits: RateLimits
): boolean {
  return requestCount <= rateLimits.requests_per_hour;
}

/**
 * Example usage:
 * 
 * ```typescript
 * import * as yaml from 'js-yaml';
 * import * as fs from 'fs';
 * import { UserAccessConfig, hasPermission, canAccessProject } from './userAccessTypes';
 * 
 * // Load configuration
 * const config = yaml.load(
 *   fs.readFileSync('.github/configs/user_access.yml', 'utf8')
 * ) as UserAccessConfig;
 * 
 * // Check permissions
 * const agentRole = config.user_access.access_as.agent;
 * const hasWrite = hasPermission(agentRole, 'write'); // false
 * 
 * // Check project access
 * const project = config.user_access.projects[0];
 * const canAccess = canAccessProject('free', project);
 * ```
 */
