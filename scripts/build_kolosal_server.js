#!/usr/bin/env node

/**
 * Build script for compiling kolosal-server and integrating it into the kolosal-code package
 * This script builds the kolosal-server C++ component and places the executable and libraries
 * in the dist/mac/kolosal-app/bin/ directory for macOS distribution
 */

import { exec, spawn } from 'child_process';
import { promisify } from 'util';
import fs from 'fs/promises';
import path from 'path';
import { fileURLToPath } from 'url';
import { createRequire } from 'module';

const execAsync = promisify(exec);
const require = createRequire(import.meta.url);
const __dirname = path.dirname(fileURLToPath(import.meta.url));
const rootDir = path.resolve(__dirname, '..');
const kolosalServerDir = path.join(rootDir, 'kolosal-server');
const isMac = process.platform === 'darwin';
const inferenceLibName = isMac ? 'libllama-metal.dylib' : 'libllama-cpu.dylib';

async function buildKolosalServer() {
  console.log('🔨 Building kolosal-server...\n');
  
  const buildDir = path.join(kolosalServerDir, 'build');
  const releaseDir = path.join(buildDir, 'Release');
  
  try {
    // Check if kolosal-server directory exists
    await fs.access(kolosalServerDir);
    console.log('✅ Found kolosal-server directory');
  } catch (error) {
    throw new Error('❌ kolosal-server directory not found. Make sure the kolosal-server subproject exists.');
  }
  
  // Create build directory if it doesn't exist
  try {
    await fs.mkdir(buildDir, { recursive: true });
    console.log('📁 Created build directory');
  } catch (error) {
    console.log('📁 Build directory already exists');
  }
  
  // Configure CMake
  console.log('⚙️  Configuring CMake...');
  try {
    const cmakeArgs = ['cmake', '..', '-DCMAKE_BUILD_TYPE=Release'];
    if (isMac) {
      cmakeArgs.push('-DUSE_METAL=ON', '-DUSE_VULKAN=OFF', '-DUSE_CUDA=OFF');
    }

    await execAsync(cmakeArgs.join(' '), {
      cwd: buildDir,
      stdio: 'inherit'
    });
    console.log('✅ CMake configuration completed');
  } catch (error) {
    throw new Error(`❌ CMake configuration failed: ${error.message}`);
  }
  
  // Build the project
  console.log('🔧 Building kolosal-server...');
  try {
    await execAsync('make -j4', {
      cwd: buildDir,
      stdio: 'inherit'
    });
    console.log('✅ kolosal-server build completed');
  } catch (error) {
    throw new Error(`❌ Build failed: ${error.message}`);
  }
  
  // Verify build artifacts exist
  const expectedFiles = [
    'kolosal-server',
    'libkolosal_server.dylib',
    inferenceLibName
  ];
  
  console.log('🔍 Verifying build artifacts...');
  for (const file of expectedFiles) {
    const filePath = path.join(releaseDir, file);
    try {
      await fs.access(filePath);
      console.log(`   ✓ ${file}`);
    } catch (error) {
      throw new Error(`❌ Missing build artifact: ${file}`);
    }
  }
  
  return releaseDir;
}

async function copyToDistribution(sourceDir) {
  console.log('\n📦 Copying kolosal-server to distribution directory...\n');
  
  const distDir = path.join(rootDir, 'dist', 'mac', 'kolosal-app');
  const binDir = path.join(distDir, 'bin');
  const libDir = path.join(distDir, 'lib');
  const resourcesDir = path.join(distDir, 'Resources');
  
  // Create directories if they don't exist
  await fs.mkdir(binDir, { recursive: true });
  await fs.mkdir(libDir, { recursive: true });
  await fs.mkdir(resourcesDir, { recursive: true });
  
  // Files to copy to bin directory
  const executableFiles = [
    'kolosal-server'
  ];
  
  // Files to copy to lib directory  
  const libraryFiles = [
    'libkolosal_server.dylib',
    inferenceLibName
  ];
  
  // Copy executable files
  console.log('📋 Copying executables to bin/...');
  for (const file of executableFiles) {
    const sourcePath = path.join(sourceDir, file);
    const destPath = path.join(binDir, file);
    
    try {
      await fs.copyFile(sourcePath, destPath);
      // Make executable
      await fs.chmod(destPath, 0o755);
      console.log(`   ✓ ${file} → bin/`);
    } catch (error) {
      throw new Error(`❌ Failed to copy ${file}: ${error.message}`);
    }
  }
  
  // Copy library files
  console.log('📚 Copying libraries to lib/...');
  for (const file of libraryFiles) {
    const sourcePath = path.join(sourceDir, file);
    const destPath = path.join(libDir, file);
    
    try {
      await fs.copyFile(sourcePath, destPath);
      console.log(`   ✓ ${file} → lib/`);
    } catch (error) {
      throw new Error(`❌ Failed to copy ${file}: ${error.message}`);
    }
  }

  // Copy default configuration for macOS distributions
  if (isMac) {
    const macConfigTemplate = path.join(rootDir, 'kolosal-server', 'configs', 'config.macos.yaml');
    const configDest = path.join(resourcesDir, 'config.yaml');

    try {
      await fs.copyFile(macConfigTemplate, configDest);
      console.log('   ✓ config.macos.yaml → Resources/config.yaml');
    } catch (error) {
      console.warn(`   ⚠️  Unable to copy default config: ${error.message}`);
    }
  }
  
  // Verify copied files
  console.log('\n🔍 Verifying copied files...');
  
  // Check executables
  for (const file of executableFiles) {
    const filePath = path.join(binDir, file);
    try {
      await fs.access(filePath);
      const stats = await fs.stat(filePath);
      if (stats.mode & 0o111) {
        console.log(`   ✓ ${file} (executable)`);
      } else {
        console.log(`   ⚠️  ${file} (not executable)`);
      }
    } catch (error) {
      throw new Error(`❌ Missing file in bin/: ${file}`);
    }
  }
  
  // Check libraries
  for (const file of libraryFiles) {
    const filePath = path.join(libDir, file);
    try {
      await fs.access(filePath);
      console.log(`   ✓ ${file}`);
    } catch (error) {
      throw new Error(`❌ Missing file in lib/: ${file}`);
    }
  }
  
  // Check config template
  if (isMac) {
    const configPath = path.join(resourcesDir, 'config.yaml');
    try {
      await fs.access(configPath);
      console.log(`   ✓ ${path.relative(distDir, configPath)}`);
    } catch {
      console.warn(`   ⚠️  Missing default config at ${configPath}`);
    }
  }
  
  console.log('\n✅ kolosal-server successfully integrated into distribution package!');
  console.log(`📍 Files located at: ${distDir}`);
  
  return { binDir, libDir, resourcesDir };
}

const TEST_TIMEOUT_MS = 7000;

async function testKolosalServer(binDir, libDir, resourcesDir) {
  console.log('\n🧪 Testing kolosal-server executable...');
  
  const executablePath = path.join(binDir, 'kolosal-server');
  const env = { ...process.env };
  const delimiter = process.platform === 'win32' ? ';' : ':';
  const libEnvKey = process.platform === 'darwin' ? 'DYLD_LIBRARY_PATH' : process.platform === 'win32' ? 'PATH' : 'LD_LIBRARY_PATH';
  if (libDir) {
    env[libEnvKey] = libDir + (env[libEnvKey] ? `${delimiter}${env[libEnvKey]}` : '');
  }

  if (resourcesDir && process.platform === 'darwin') {
    const bundledConfig = path.join(resourcesDir, 'config.yaml');
    try {
      await fs.access(bundledConfig);
      console.log(`   Found bundled config at ${bundledConfig}`);
    } catch {
      // ignore missing config
    }
  }

  const args = ['--version'];
  
  if (process.env.KOLOSAL_SERVER_TEST_ARGS) {
    args.splice(0, args.length, ...process.env.KOLOSAL_SERVER_TEST_ARGS.split(' ').filter(Boolean));
    console.log(`   Using KOLOSAL_SERVER_TEST_ARGS override: ${args.join(' ')}`);
  }
  
  console.log(`   Command: ${[executablePath, ...args].join(' ')}`);
  
  try {
    const child = spawn(executablePath, args, { env, stdio: ['ignore', 'pipe', 'pipe'] });

    let stdout = '';
    let stderr = '';
    let timedOut = false;

    const timer = setTimeout(() => {
      timedOut = true;
      child.kill('SIGTERM');
    }, TEST_TIMEOUT_MS);

    child.stdout.on('data', (data) => {
      stdout += data.toString();
    });
    child.stderr.on('data', (data) => {
      stderr += data.toString();
    });

    await new Promise((resolve, reject) => {
      child.on('error', reject);
      child.on('close', (code, signal) => {
        clearTimeout(timer);
        if (timedOut) {
          return reject(new Error(`Timed out after ${TEST_TIMEOUT_MS}ms (signal: ${signal ?? 'N/A'})`));
        }
        if (code !== 0) {
          return reject(new Error(`Exited with code ${code ?? 'unknown'}; stderr: ${stderr.trim()}`));
        }
        resolve();
      });
    });

    console.log('✅ kolosal-server executable is working');
    if (stdout.trim()) {
      const preview = stdout.trim().substring(0, 160);
      console.log(`   Output: ${preview}${stdout.trim().length > 160 ? '…' : ''}`);
    }
  } catch (error) {
    console.warn(`⚠️  Could not test kolosal-server: ${error.message}`);
    console.warn('   This might be normal if the server requires specific arguments');
  }
}

async function main() {
  try {
    console.log('🚀 Starting kolosal-server build and integration process...\n');
    
    // Build kolosal-server
    const buildOutputDir = await buildKolosalServer();
    
    // Copy to distribution
  const { binDir, libDir, resourcesDir } = await copyToDistribution(buildOutputDir);
    
    // Test the executable
  await testKolosalServer(binDir, libDir, resourcesDir);
    
    console.log('\n🎉 Build and integration completed successfully!');
    console.log('\n📋 Summary:');
  console.log(`   • Executable: ${path.join(binDir, 'kolosal-server')}`);
  console.log(`   • Libraries: ${path.join(libDir, 'libkolosal_server.dylib')}, ${path.join(libDir, inferenceLibName)}`);
    console.log(`   • Ready for packaging and distribution`);
    
  } catch (error) {
    console.error('\n💥 Build failed:', error.message);
    process.exit(1);
  }
}

// Run the main function if this script is called directly
if (import.meta.url === `file://${process.argv[1]}`) {
  main();
}

export { buildKolosalServer, copyToDistribution, testKolosalServer };