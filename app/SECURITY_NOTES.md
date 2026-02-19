# Security Notes - Bitcoin Core Frontend

## Known Development Dependency Vulnerabilities

As of the last audit, the following vulnerabilities exist in development dependencies:

### Summary
- **Total**: 10 vulnerabilities (1 moderate, 9 high)
- **Impact**: Development dependencies only (not runtime)
- **Risk Level**: Low (affects development environment, not production)

### Vulnerabilities

1. **ajv <8.18.0** (Moderate)
   - Issue: ReDoS when using `$data` option
   - Advisory: GHSA-2g4f-4pwh-qvx6
   - Status: Transitive dependency of ESLint 8.x
   - Mitigation: Does not affect production build; only impacts linting process

2. **minimatch <10.2.1** (High)
   - Issue: ReDoS via repeated wildcards with non-matching literal in pattern
   - Advisory: GHSA-3ppc-4f35-3m26
   - Status: Transitive dependency of ESLint 8.x and related tools
   - Mitigation: Does not affect production build; only impacts linting and build tools

3. **esbuild <=0.24.2** (Moderate)
   - Issue: Development server can receive requests from any website
   - Advisory: GHSA-67mh-4wv8-2f99
   - Status: Transitive dependency of Vite
   - Mitigation: Only affects development server; not used in production

### Why Not Fixed?

- **ESLint 9 Migration**: Upgrading to ESLint 9 would require migrating from `.eslintrc.cjs` to `eslint.config.js`, which is a breaking change beyond the scope of minimal error fixes.
- **Plugin Compatibility**: Some ESLint plugins (e.g., `eslint-plugin-react-hooks@4.6.2`) don't support ESLint 9 yet.
- **Low Risk**: These vulnerabilities only affect the development environment and build tools, not the runtime application code.
- **Development Only**: All vulnerable packages are devDependencies and are not included in the production build.

### Recommendations

1. **Monitor Updates**: Regularly check for updates to ESLint and related plugins that fix these vulnerabilities.
2. **Migrate to ESLint 9**: When all required plugins support ESLint 9, migrate to the new configuration format.
3. **Secure Development Environment**: Ensure development machines are secure and only run `npm install` from trusted networks.
4. **Production Build**: Always verify that production builds do not include any vulnerable dependencies using `npm audit --production`.

### Production Security

Run the following to verify no vulnerabilities in production dependencies:
```bash
npm audit --production
```

### Last Updated
2026-02-19

### References
- ESLint v9 Migration Guide: https://eslint.org/docs/latest/use/configure/migration-guide
- npm audit documentation: https://docs.npmjs.com/cli/v9/commands/npm-audit
