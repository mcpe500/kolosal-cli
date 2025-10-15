#!/usr/bin/env node

/**
 * @license
 * Copyright 2025 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import { exec } from 'node:child_process';
import { promisify } from 'node:util';
import { existsSync } from 'node:fs';

const execAsync = promisify(exec);

export interface GPUInfo {
  vendor: string;
  name: string;
  driver?: string;
  memory?: string;
  vulkanSupported: boolean;
  dedicated: boolean;
}

export interface GPUDetectionResult {
  hasGPU: boolean;
  hasDedicatedGPU: boolean;
  hasVulkanSupport: boolean;
  gpus: GPUInfo[];
  recommendedEngine: 'llama-cpu' | 'llama-vulkan';
}

/**
 * Detect available GPUs and their capabilities
 */
export async function detectGPUs(): Promise<GPUDetectionResult> {
  const result: GPUDetectionResult = {
    hasGPU: false,
    hasDedicatedGPU: false,
    hasVulkanSupport: false,
    gpus: [],
    recommendedEngine: 'llama-cpu'
  };

  try {
    if (process.platform === 'linux') {
      await detectLinuxGPUs(result);
    } else if (process.platform === 'darwin') {
      await detectMacGPUs(result);
    } else if (process.platform === 'win32') {
      await detectWindowsGPUs(result);
    }

    // Determine recommended engine
    if (result.hasDedicatedGPU && result.hasVulkanSupport) {
      result.recommendedEngine = 'llama-vulkan';
    }

    return result;
  } catch (error) {
    console.warn(`GPU detection failed: ${error}`);
    return result;
  }
}

/**
 * Detect GPUs on Linux systems
 */
async function detectLinuxGPUs(result: GPUDetectionResult): Promise<void> {
  // Check for Vulkan support first
  result.hasVulkanSupport = await checkVulkanSupport();

  // Use lspci to detect GPUs
  try {
    const { stdout } = await execAsync('lspci | grep -i "vga\\|3d\\|display"');
    const lines = stdout.trim().split('\n').filter(line => line.trim());

    for (const line of lines) {
      const gpu = parseLinuxGPULine(line);
      if (gpu) {
        result.gpus.push(gpu);
        result.hasGPU = true;
        if (gpu.dedicated) {
          result.hasDedicatedGPU = true;
        }
      }
    }
  } catch (error) {
    console.warn('Failed to detect GPUs via lspci:', error);
  }

  // Also check /proc/driver/nvidia/version for NVIDIA
  if (existsSync('/proc/driver/nvidia/version')) {
    try {
      const { stdout } = await execAsync('nvidia-smi --query-gpu=name,memory.total --format=csv,noheader,nounits 2>/dev/null || echo ""');
      if (stdout.trim()) {
        const lines = stdout.trim().split('\n');
        for (const line of lines) {
          const [name, memory] = line.split(',').map(s => s.trim());
          if (name && memory) {
            const existing = result.gpus.find(g => g.name.includes(name) || name.includes(g.name));
            if (existing) {
              existing.memory = `${memory} MB`;
              existing.driver = 'nvidia';
            } else {
              result.gpus.push({
                vendor: 'NVIDIA',
                name,
                memory: `${memory} MB`,
                driver: 'nvidia',
                vulkanSupported: result.hasVulkanSupport,
                dedicated: true
              });
              result.hasGPU = true;
              result.hasDedicatedGPU = true;
            }
          }
        }
      }
    } catch (error) {
      // nvidia-smi not available or failed
    }
  }
}

/**
 * Detect GPUs on macOS systems
 */
async function detectMacGPUs(result: GPUDetectionResult): Promise<void> {
  try {
    const { stdout } = await execAsync('system_profiler SPDisplaysDataType -json');
    const data = JSON.parse(stdout);
    
    if (data.SPDisplaysDataType) {
      for (const display of data.SPDisplaysDataType) {
        if (display.sppci_model || display.spdisplays_renderer) {
          const name = display.sppci_model || display.spdisplays_renderer;
          const memory = display.spdisplays_vram || display.spdisplays_ram;
          
          const gpu: GPUInfo = {
            vendor: detectVendorFromName(name),
            name,
            memory: memory ? `${memory}` : undefined,
            vulkanSupported: await checkVulkanSupport(),
            dedicated: !name.toLowerCase().includes('intel') // Assume non-Intel GPUs are dedicated
          };
          
          result.gpus.push(gpu);
          result.hasGPU = true;
          if (gpu.dedicated) {
            result.hasDedicatedGPU = true;
          }
        }
      }
    }

    result.hasVulkanSupport = await checkVulkanSupport();
  } catch (error) {
    console.warn('Failed to detect GPUs on macOS:', error);
  }
}

/**
 * Detect GPUs on Windows systems
 */
async function detectWindowsGPUs(result: GPUDetectionResult): Promise<void> {
  try {
    const { stdout } = await execAsync('wmic path win32_VideoController get Name,AdapterRAM,DriverVersion /format:csv');
    const lines = stdout.trim().split('\n').slice(1); // Skip header
    
    for (const line of lines) {
      const parts = line.split(',').map(s => s.trim()).filter(s => s);
      if (parts.length >= 2) {
        const name = parts[parts.length - 1]; // Name is usually last
        const memory = parts[1]; // AdapterRAM
        
        if (name && !name.toLowerCase().includes('node')) {
          const gpu: GPUInfo = {
            vendor: detectVendorFromName(name),
            name,
            memory: memory && memory !== '' ? `${Math.round(parseInt(memory) / 1024 / 1024)} MB` : undefined,
            vulkanSupported: await checkVulkanSupport(),
            dedicated: !name.toLowerCase().includes('intel') || name.toLowerCase().includes('arc')
          };
          
          result.gpus.push(gpu);
          result.hasGPU = true;
          if (gpu.dedicated) {
            result.hasDedicatedGPU = true;
          }
        }
      }
    }

    result.hasVulkanSupport = await checkVulkanSupport();
  } catch (error) {
    console.warn('Failed to detect GPUs on Windows:', error);
  }
}

/**
 * Parse a Linux lspci GPU line
 */
function parseLinuxGPULine(line: string): GPUInfo | null {
  // Example: 01:00.0 VGA compatible controller: NVIDIA Corporation TU102 [GeForce RTX 2080 Ti] (rev a1)
  const match = line.match(/.*:\s*(.*?)(?:\s*\[.*?\])?\s*(?:\(.*?\))?$/i);
  if (!match) return null;

  const fullName = match[1].trim();
  const vendor = detectVendorFromName(fullName);
  
  // Determine if it's dedicated (assume NVIDIA/AMD are dedicated, Intel integrated unless Arc)
  const isDedicated = vendor === 'NVIDIA' || 
                     vendor === 'AMD' || 
                     fullName.toLowerCase().includes('arc') ||
                     (!fullName.toLowerCase().includes('intel') && !fullName.toLowerCase().includes('integrated'));

  return {
    vendor,
    name: fullName,
    vulkanSupported: false, // Will be updated later
    dedicated: isDedicated
  };
}

/**
 * Detect GPU vendor from name
 */
function detectVendorFromName(name: string): string {
  const lowerName = name.toLowerCase();
  
  if (lowerName.includes('nvidia') || lowerName.includes('geforce') || lowerName.includes('quadro') || lowerName.includes('tesla')) {
    return 'NVIDIA';
  } else if (lowerName.includes('amd') || lowerName.includes('radeon') || lowerName.includes('ati')) {
    return 'AMD';
  } else if (lowerName.includes('intel')) {
    return 'Intel';
  } else if (lowerName.includes('apple')) {
    return 'Apple';
  }
  
  return 'Unknown';
}

/**
 * Check if Vulkan is supported on the system
 */
async function checkVulkanSupport(): Promise<boolean> {
  try {
    // Try vulkaninfo command
    const { stdout } = await execAsync('vulkaninfo --summary 2>/dev/null || echo ""');
    if (stdout.includes('Vulkan API Version') || stdout.includes('apiVersion')) {
      return true;
    }
  } catch (error) {
    // vulkaninfo not available
  }

  // Check for Vulkan libraries
  const vulkanLibPaths = [
    '/usr/lib/x86_64-linux-gnu/libvulkan.so.1',
    '/usr/lib/libvulkan.so.1',
    '/usr/lib64/libvulkan.so.1',
    '/lib/x86_64-linux-gnu/libvulkan.so.1',
  ];

  for (const path of vulkanLibPaths) {
    if (existsSync(path)) {
      return true;
    }
  }

  // On macOS, check for MoltenVK or system Vulkan
  if (process.platform === 'darwin') {
    try {
      const { stdout } = await execAsync('find /usr/local -name "*vulkan*" -o -name "*molten*" 2>/dev/null | head -1');
      return stdout.trim().length > 0;
    } catch (error) {
      return false;
    }
  }

  // On Windows, check for vulkan-1.dll
  if (process.platform === 'win32') {
    try {
      const { stdout } = await execAsync('where vulkan-1.dll 2>nul || echo ""');
      return stdout.trim().length > 0;
    } catch (error) {
      return false;
    }
  }

  return false;
}

/**
 * Get a summary of GPU detection results for logging
 */
export function getGPUSummary(result: GPUDetectionResult): string {
  if (!result.hasGPU) {
    return 'No GPUs detected - using CPU inference';
  }

  const dedicatedGPUs = result.gpus.filter(g => g.dedicated);
  const integratedGPUs = result.gpus.filter(g => !g.dedicated);

  let summary = '';
  
  if (dedicatedGPUs.length > 0) {
    summary += `Dedicated GPU(s): ${dedicatedGPUs.map(g => `${g.vendor} ${g.name}`).join(', ')}`;
  }
  
  if (integratedGPUs.length > 0) {
    if (summary) summary += '; ';
    summary += `Integrated GPU(s): ${integratedGPUs.map(g => `${g.vendor} ${g.name}`).join(', ')}`;
  }

  summary += `; Vulkan: ${result.hasVulkanSupport ? 'supported' : 'not available'}`;
  summary += `; Recommended engine: ${result.recommendedEngine}`;

  return summary;
}