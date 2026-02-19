#!/usr/bin/env node

import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import prompts from 'prompts';
import chalk from 'chalk';
import ora from 'ora';
import { execSync } from 'child_process';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// Banner
console.log(chalk.blue.bold('\n🚀 Create Onchain Agent\n'));

// Main function
async function main() {
  try {
    // Prompt for project configuration
    const responses = await prompts([
      {
        type: 'text',
        name: 'projectName',
        message: 'Project name:',
        initial: 'my-onchain-agent',
        validate: (value) => {
          if (!value) return 'Project name is required';
          if (!/^[a-z0-9-]+$/.test(value)) {
            return 'Project name must only contain lowercase letters, numbers, and hyphens';
          }
          return true;
        }
      },
      {
        type: 'text',
        name: 'apiKey',
        message: 'Enter your Coinbase Developer Platform Client API Key: (optional)',
        initial: ''
      },
      {
        type: 'confirm',
        name: 'shareUsageData',
        message: 'Share anonymous usage data to help improve create-onchain-agent?',
        initial: false
      }
    ]);

    if (!responses.projectName) {
      console.log(chalk.yellow('\nSetup cancelled.'));
      process.exit(1);
    }

    const { projectName, apiKey, shareUsageData } = responses;
    const projectPath = path.join(process.cwd(), projectName);

    // Check if directory already exists
    if (fs.existsSync(projectPath)) {
      console.log(chalk.red(`\n❌ Error: Directory "${projectName}" already exists.`));
      process.exit(1);
    }

    // Create project directory
    const spinner = ora('Creating project...').start();
    fs.mkdirSync(projectPath, { recursive: true });

    // Copy template files
    const templatePath = path.join(__dirname, 'templates', 'default');
    copyRecursive(templatePath, projectPath);

    // Generate .env file
    const envContent = `# Coinbase Developer Platform API Key
NEXT_PUBLIC_ONCHAINKIT_API_KEY="${apiKey || ''}"

# Project Configuration
NEXT_PUBLIC_PROJECT_NAME="${projectName}"

# Network Configuration (optional)
# NEXT_PUBLIC_CHAIN_ID=8453  # Base Mainnet
# NEXT_PUBLIC_CHAIN_ID=84532 # Base Sepolia
`;
    fs.writeFileSync(path.join(projectPath, '.env'), envContent);

    // Update package.json with project name
    const packageJsonPath = path.join(projectPath, 'package.json');
    const packageJson = JSON.parse(fs.readFileSync(packageJsonPath, 'utf-8'));
    packageJson.name = projectName;
    fs.writeFileSync(packageJsonPath, JSON.stringify(packageJson, null, 2) + '\n');

    spinner.succeed('Project created successfully!');

    // Install dependencies
    console.log(chalk.cyan('\n📦 Installing dependencies...\n'));
    try {
      execSync('npm install', {
        cwd: projectPath,
        stdio: 'inherit'
      });
    } catch (error) {
      console.log(chalk.yellow('\n⚠️  Failed to install dependencies automatically.'));
      console.log(chalk.yellow('Please run "npm install" manually in the project directory.\n'));
    }

    // Success message
    console.log(chalk.green.bold('\n✅ All done!\n'));
    console.log(chalk.white('To get started:\n'));
    console.log(chalk.cyan(`  cd ${projectName}`));
    console.log(chalk.cyan('  npm run dev\n'));
    console.log(chalk.white('Then open http://localhost:3000 in your browser.\n'));
    console.log(chalk.gray('Learn more at https://docs.base.org/onchainkit\n'));

    // Handle usage data sharing (placeholder)
    if (shareUsageData) {
      // In a real implementation, you would send anonymous usage data here
      // For now, we'll just acknowledge the user's choice
    }

  } catch (error) {
    if (error.message === 'canceled') {
      console.log(chalk.yellow('\nSetup cancelled.'));
      process.exit(1);
    }
    console.error(chalk.red('\n❌ Error:'), error.message);
    process.exit(1);
  }
}

// Helper function to copy files recursively
function copyRecursive(src, dest) {
  const exists = fs.existsSync(src);
  const stats = exists && fs.statSync(src);
  const isDirectory = exists && stats.isDirectory();

  if (isDirectory) {
    if (!fs.existsSync(dest)) {
      fs.mkdirSync(dest, { recursive: true });
    }
    fs.readdirSync(src).forEach((childItemName) => {
      copyRecursive(
        path.join(src, childItemName),
        path.join(dest, childItemName)
      );
    });
  } else {
    fs.copyFileSync(src, dest);
  }
}

// Run the CLI
main().catch((error) => {
  console.error(chalk.red('Unexpected error:'), error);
  process.exit(1);
});
