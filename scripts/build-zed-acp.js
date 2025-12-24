#!/usr/bin/env node

/**
 * @license
 * Copyright 2025 Kolosal AI
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Build script for creating Zed ACP (Agent Control Protocol) binaries
 * Creates platform-specific zip archives for Zed extension distribution
 */

import { exec } from 'child_process';
import { promisify } from 'util';
import fs from 'fs/promises';
import path from 'path';
import { fileURLToPath } from 'url';
import { createRequire } from 'module';

const execAsync = promisify(exec);
const require = createRequire(import.meta.url);
const __dirname = path.dirname(fileURLToPath(import.meta.url));
const rootDir = path.resolve(__dirname, '..');
const pkg = require(path.join(rootDir, 'package.json'));

const VERSION = pkg.version;

// Platform configurations
const PLATFORMS = {
  'darwin-aarch64': { nodeTarget: 'node20-macos-arm64', ext: '' },
  'darwin-x86_64': { nodeTarget: 'node20-macos-x64', ext: '' },
  'linux-aarch64': { nodeTarget: 'node20-linux-arm64', ext: '' },
  'linux-x86_64': { nodeTarget: 'node20-linux-x64', ext: '' },
  'windows-aarch64': { nodeTarget: 'node20-win-arm64', ext: '.exe' },
  'windows-x86_64': { nodeTarget: 'node20-win-x64', ext: '.exe' },
};

async function ensureBundle() {
  console.log('📦 Ensuring bundle is up to date...');
  await execAsync('npm run bundle', { cwd: rootDir });
  console.log('✅ Bundle ready\n');
}

async function buildAcpBinary(platform, config) {
  const { nodeTarget, ext } = config;
  const binaryName = `kolosal-acp${ext}`;
  const zipName = `kolosal-acp-${platform}-${VERSION}.zip`;

  const distDir = path.join(rootDir, 'dist', 'zed');
  const platformDir = path.join(distDir, platform);

  console.log(`🔨 Building ACP binary for ${platform}...`);

  // Create platform directory
  await fs.rm(platformDir, { recursive: true, force: true });
  await fs.mkdir(platformDir, { recursive: true });

  const outputPath = path.join(platformDir, binaryName);

  try {
    // Build with pkg
    const pkgCmd = `npx @yao-pkg/pkg bundle/gemini.js --target ${nodeTarget} --compress Brotli --public --output "${outputPath}"`;
    console.log(`   Running: ${pkgCmd}`);
    await execAsync(pkgCmd, { cwd: rootDir });

    // Make executable on Unix
    if (!ext) {
      await fs.chmod(outputPath, 0o755);
    }

    console.log(`✅ Built ${binaryName}`);

    // Create zip archive
    const zipPath = path.join(distDir, zipName);

    if (platform.startsWith('windows')) {
      // Use PowerShell for Windows-compatible zip
      await execAsync(`cd "${platformDir}" && zip -j "${zipPath}" "${binaryName}"`, { cwd: rootDir });
    } else {
      await execAsync(`cd "${platformDir}" && zip -j "${zipPath}" "${binaryName}"`, { cwd: rootDir });
    }

    console.log(`📦 Created ${zipName}\n`);

    return { success: true, zipPath, zipName };
  } catch (error) {
    console.error(`❌ Failed to build for ${platform}: ${error.message}\n`);
    return { success: false, error: error.message };
  }
}

async function buildAllPlatforms() {
  console.log('🚀 Building Kolosal ACP binaries for Zed\n');
  console.log(`   Version: ${VERSION}`);
  console.log(`   Platforms: ${Object.keys(PLATFORMS).join(', ')}\n`);

  await ensureBundle();

  const distDir = path.join(rootDir, 'dist', 'zed');
  await fs.mkdir(distDir, { recursive: true });

  const results = {};

  for (const [platform, config] of Object.entries(PLATFORMS)) {
    results[platform] = await buildAcpBinary(platform, config);
  }

  // Summary
  console.log('\n📊 Build Summary:');
  console.log('─'.repeat(60));

  const successful = [];
  const failed = [];

  for (const [platform, result] of Object.entries(results)) {
    if (result.success) {
      successful.push({ platform, zipName: result.zipName });
      console.log(`✅ ${platform}: ${result.zipName}`);
    } else {
      failed.push({ platform, error: result.error });
      console.log(`❌ ${platform}: ${result.error}`);
    }
  }

  console.log('─'.repeat(60));
  console.log(`\nTotal: ${successful.length} succeeded, ${failed.length} failed`);

  if (successful.length > 0) {
    console.log(`\n📁 Output directory: ${distDir}`);
    console.log('\n📝 Next steps:');
    console.log('   1. Upload zip files to GitHub release');
    console.log('   2. Update distributions/zed/extension.toml with correct URLs');
    console.log('   3. Submit extension to Zed marketplace');
  }

  return { successful, failed };
}

async function buildCurrentPlatform() {
  const arch = process.arch === 'arm64' ? 'aarch64' : 'x86_64';
  const os = process.platform === 'darwin' ? 'darwin' :
             process.platform === 'win32' ? 'windows' : 'linux';
  const platform = `${os}-${arch}`;

  if (!PLATFORMS[platform]) {
    console.error(`❌ Unsupported platform: ${platform}`);
    process.exit(1);
  }

  console.log(`🚀 Building Kolosal ACP for current platform: ${platform}\n`);

  await ensureBundle();

  const distDir = path.join(rootDir, 'dist', 'zed');
  await fs.mkdir(distDir, { recursive: true });

  const result = await buildAcpBinary(platform, PLATFORMS[platform]);

  if (result.success) {
    console.log(`\n✅ Build complete: ${result.zipPath}`);
  } else {
    console.error(`\n❌ Build failed: ${result.error}`);
    process.exit(1);
  }
}

// Main execution
const args = process.argv.slice(2);

if (args.includes('--all')) {
  buildAllPlatforms().catch((error) => {
    console.error('Build failed:', error);
    process.exit(1);
  });
} else if (args.includes('--help') || args.includes('-h')) {
  console.log(`
Kolosal ACP Build Script for Zed Integration

Usage:
  node scripts/build-zed-acp.js [options]

Options:
  --all     Build for all platforms (requires cross-compilation setup)
  --help    Show this help message

Without options, builds only for the current platform.

Output:
  dist/zed/kolosal-acp-{platform}-${VERSION}.zip
`);
} else {
  buildCurrentPlatform().catch((error) => {
    console.error('Build failed:', error);
    process.exit(1);
  });
}
