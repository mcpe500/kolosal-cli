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
const isLinux = process.platform === 'linux';
const inferenceLibName = isMac ? 'libllama-metal.dylib' : 'libllama-cpu.so';
const libExtension = isMac ? '.dylib' : '.so';

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

  if (isLinux) {
    console.log('🐧 Linux detected: Building both CPU and Vulkan inference engines...\n');
    return await buildDualInferenceEngines();
  } else {
    return await buildSingleInferenceEngine();
  }
}

async function buildSingleInferenceEngine() {
  console.log('🔨 Building single inference engine...\n');
  
  const buildDir = path.join(kolosalServerDir, 'build');
  const releaseDir = path.join(buildDir, 'Release');
  
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
    `libkolosal_server${libExtension}`,
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

async function buildDualInferenceEngines() {
  console.log('🔥 Building dual inference engines for Linux...\n');
  
  const buildDirCpu = path.join(kolosalServerDir, 'build-cpu');
  const buildDirVulkan = path.join(kolosalServerDir, 'build-vulkan');
  const releaseDirCpu = path.join(buildDirCpu, 'Release');
  const releaseDirVulkan = path.join(buildDirVulkan, 'Release');
  
  // Clean and create build directories
  await fs.rm(buildDirCpu, { recursive: true, force: true });
  await fs.rm(buildDirVulkan, { recursive: true, force: true });
  await fs.mkdir(buildDirCpu, { recursive: true });
  await fs.mkdir(buildDirVulkan, { recursive: true });
  
  console.log('📁 Created separate build directories for CPU and Vulkan');

  // Build CPU version
  console.log('\n🖥️  Building CPU inference engine...');
  try {
    await execAsync('cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_CUDA=OFF -DUSE_VULKAN=OFF -DUSE_METAL=OFF', {
      cwd: buildDirCpu,
      stdio: 'inherit'
    });
    console.log('✅ CPU CMake configuration completed');
    
    await execAsync('make -j4', {
      cwd: buildDirCpu,
      stdio: 'inherit'
    });
    console.log('✅ CPU build completed');
  } catch (error) {
    throw new Error(`❌ CPU build failed: ${error.message}`);
  }

  // Build Vulkan version
  console.log('\n🎮 Building Vulkan inference engine...');
  try {
    await execAsync('cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_CUDA=OFF -DUSE_VULKAN=ON -DUSE_METAL=OFF', {
      cwd: buildDirVulkan,
      stdio: 'inherit'
    });
    console.log('✅ Vulkan CMake configuration completed');
    
    await execAsync('make -j4', {
      cwd: buildDirVulkan,
      stdio: 'inherit'
    });
    console.log('✅ Vulkan build completed');
  } catch (error) {
    console.warn(`⚠️  Vulkan build failed: ${error.message}`);
    console.warn('   Continuing with CPU-only build...');
    
    // Return CPU build only if Vulkan fails
    return { 
      cpu: releaseDirCpu,
      vulkan: null,
      primary: releaseDirCpu
    };
  }

  // Verify build artifacts
  console.log('\n🔍 Verifying build artifacts...');
  
  const cpuFiles = [
    'kolosal-server',
    'libkolosal_server.so',
    'libllama-cpu.so'
  ];
  
  const vulkanFiles = [
    'libllama-vulkan.so'
  ];

  console.log('   CPU build artifacts:');
  for (const file of cpuFiles) {
    const filePath = path.join(releaseDirCpu, file);
    try {
      await fs.access(filePath);
      console.log(`     ✓ ${file}`);
    } catch (error) {
      throw new Error(`❌ Missing CPU build artifact: ${file}`);
    }
  }

  console.log('   Vulkan build artifacts:');
  for (const file of vulkanFiles) {
    const filePath = path.join(releaseDirVulkan, file);
    try {
      await fs.access(filePath);
      console.log(`     ✓ ${file}`);
    } catch (error) {
      console.warn(`⚠️  Missing Vulkan build artifact: ${file}`);
    }
  }
  
  return {
    cpu: releaseDirCpu,
    vulkan: releaseDirVulkan,
    primary: releaseDirCpu
  };
}

async function copyToDistribution(sourceData) {
  console.log('\n📦 Copying kolosal-server to distribution directory...\n');
  
  // Detect platform and set appropriate dist directory
  const platformDir = isMac ? 'mac' : isLinux ? 'linux' : 'unknown';
  if (platformDir === 'unknown') {
    throw new Error(`Unsupported platform: ${process.platform}`);
  }
  
  const distDir = path.join(rootDir, 'dist', platformDir, 'kolosal-app');
  const binDir = path.join(distDir, 'bin');
  const libDir = path.join(distDir, 'lib');
  const resourcesDir = path.join(distDir, 'Resources');
  
  console.log(`   Platform: ${platformDir}`);
  console.log(`   Target: ${distDir}`);
  
  // Create directories if they don't exist
  await fs.mkdir(binDir, { recursive: true });
  await fs.mkdir(libDir, { recursive: true });
  await fs.mkdir(resourcesDir, { recursive: true });

  // Handle dual build (Linux) or single build (other platforms)
  const isDualBuild = typeof sourceData === 'object' && sourceData.cpu;
  const primarySourceDir = isDualBuild ? sourceData.primary : sourceData;
  
  console.log(isDualBuild ? '🔥 Processing dual-build artifacts...' : '📋 Processing single-build artifacts...');
  
  // Files to copy to bin directory
  const executableFiles = [
    'kolosal-server'
  ];
  
  // Copy executable files from primary build
  console.log('📋 Copying executables to bin/...');
  for (const file of executableFiles) {
    const sourcePath = path.join(primarySourceDir, file);
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
  
  if (isDualBuild) {
    // Copy libraries from both CPU and Vulkan builds
    const cpuLibraries = [
      `libkolosal_server${libExtension}`,
      'libllama-cpu.so'
    ];
    
    console.log('   From CPU build:');
    for (const file of cpuLibraries) {
      const sourcePath = path.join(sourceData.cpu, file);
      const destPath = path.join(libDir, file);
      
      try {
        await fs.copyFile(sourcePath, destPath);
        console.log(`     ✓ ${file} → lib/`);
      } catch (error) {
        throw new Error(`❌ Failed to copy CPU library ${file}: ${error.message}`);
      }
    }
    
    // Copy Vulkan library if available
    if (sourceData.vulkan) {
      console.log('   From Vulkan build:');
      const vulkanLibraries = ['libllama-vulkan.so'];
      
      for (const file of vulkanLibraries) {
        const sourcePath = path.join(sourceData.vulkan, file);
        const destPath = path.join(libDir, file);
        
        try {
          await fs.copyFile(sourcePath, destPath);
          console.log(`     ✓ ${file} → lib/`);
        } catch (error) {
          console.warn(`⚠️  Failed to copy Vulkan library ${file}: ${error.message}`);
        }
      }
    } else {
      console.warn('   ⚠️  Vulkan build not available, skipping Vulkan libraries');
    }
  } else {
    // Single build - copy libraries as before
    const libraryFiles = [
      `libkolosal_server${libExtension}`,
      inferenceLibName
    ];
    
    for (const file of libraryFiles) {
      const sourcePath = path.join(primarySourceDir, file);
      const destPath = path.join(libDir, file);
      
      try {
        await fs.copyFile(sourcePath, destPath);
        console.log(`   ✓ ${file} → lib/`);
      } catch (_error) {
        throw new Error(`❌ Failed to copy ${file}: ${_error.message}`);
      }
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
  } else if (isLinux) {
    const linuxConfigTemplate = path.join(rootDir, 'kolosal-server', 'configs', 'config.linux.yaml');
    const configDest = path.join(resourcesDir, 'config.yaml');

    try {
      await fs.copyFile(linuxConfigTemplate, configDest);
      console.log('   ✓ config.linux.yaml → Resources/config.yaml');
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
    } catch (_error) {
      throw new Error(`❌ Missing file in bin/: ${file}`);
    }
  }
  
  // Check libraries - different approach for dual vs single builds
  if (isDualBuild) {
    console.log('   Libraries (dual build):');
    const expectedLibraries = [
      `libkolosal_server${libExtension}`,
      'libllama-cpu.so',
      'libllama-vulkan.so'  // Optional
    ];
    
    for (const file of expectedLibraries) {
      const filePath = path.join(libDir, file);
      try {
        await fs.access(filePath);
        console.log(`     ✓ ${file}`);
      } catch (_error) {
        if (file === 'libllama-vulkan.so') {
          console.log(`     ⚠️  ${file} (Vulkan build failed)`);
        } else {
          throw new Error(`❌ Missing file in lib/: ${file}`);
        }
      }
    }
  } else {
    // Single build verification
    const libraryFiles = [
      `libkolosal_server${libExtension}`,
      inferenceLibName
    ];
    
    console.log('   Libraries (single build):');
    for (const file of libraryFiles) {
      const filePath = path.join(libDir, file);
      try {
        await fs.access(filePath);
        console.log(`     ✓ ${file}`);
      } catch (_error) {
        throw new Error(`❌ Missing file in lib/: ${file}`);
      }
    }
  }
  
  // Check config template
  const configPath = path.join(resourcesDir, 'config.yaml');
  try {
    await fs.access(configPath);
    console.log(`   ✓ ${path.relative(distDir, configPath)}`);
  } catch {
    console.warn(`   ⚠️  Missing default config at ${configPath}`);
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
  const libEnvKey = isMac ? 'DYLD_LIBRARY_PATH' : process.platform === 'win32' ? 'PATH' : 'LD_LIBRARY_PATH';
  if (libDir) {
    env[libEnvKey] = libDir + (env[libEnvKey] ? `${delimiter}${env[libEnvKey]}` : '');
  }

  const bundledConfig = path.join(resourcesDir, 'config.yaml');
  try {
    await fs.access(bundledConfig);
    console.log(`   Found bundled config at ${bundledConfig}`);
  } catch {
    // ignore missing config
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
    
    // Analyze shared library dependencies (for Linux)
    if (isLinux) {
      console.log('\n📚 Analyzing shared library dependencies...');
      try {
        await execAsync('node scripts/bundle_shared_libs.js', { cwd: rootDir });
        console.log('✅ Dependency analysis complete');
      } catch (error) {
        console.warn(`⚠️  Dependency analysis failed: ${error.message}`);
      }
    }
    
    console.log('\n🎉 Build and integration completed successfully!');
    console.log('\n📋 Summary:');
    console.log(`   • Executable: ${path.join(binDir, 'kolosal-server')}`);
    
    if (isLinux) {
      console.log(`   • Libraries:`);
      console.log(`     - ${path.join(libDir, `libkolosal_server${libExtension}`)}`);
      console.log(`     - ${path.join(libDir, 'libllama-cpu.so')}`);
      
      // Check if Vulkan library exists
      try {
        await fs.access(path.join(libDir, 'libllama-vulkan.so'));
        console.log(`     - ${path.join(libDir, 'libllama-vulkan.so')}`);
        console.log(`   • CPU and Vulkan acceleration available`);
      } catch {
        console.log(`   • CPU-only acceleration (Vulkan build failed)`);
      }
    } else {
      console.log(`   • Libraries: ${path.join(libDir, `libkolosal_server${libExtension}`)}, ${path.join(libDir, inferenceLibName)}`);
    }
    
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