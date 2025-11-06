#!/usr/bin/env node

/**
 * @license
 * Copyright 2025 Kolosal Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { startApiServer } from './index.js';
import { Config, AuthType } from '@kolosal-ai/kolosal-ai-core';

async function main() {
  try {
    // Parse command line arguments
    const args = process.argv.slice(2);
    const port = getArgValue(args, '--api-port') || process.env['KOLOSAL_CLI_API_PORT'] || 8080;
    const host = getArgValue(args, '--api-host') || process.env['KOLOSAL_CLI_API_HOST'] || '127.0.0.1';
    const corsEnabled = process.env['KOLOSAL_CLI_API_CORS'] ? 
      ['1', 'true', 'yes'].includes(String(process.env['KOLOSAL_CLI_API_CORS']).toLowerCase()) : 
      true;

    // Create a basic configuration
    // Note: For a fully functional server, you'll need to provide proper config
    const config = new Config({
      sessionId: `api-server-${Date.now()}`,
      targetDir: process.cwd(),
      cwd: process.cwd(),
      debugMode: process.env['DEBUG'] === '1' || args.includes('--debug'),
      model: 'z-ai/glm-4.6', // Default model - can be overridden by API requests
      excludeTools: [],
      // Add other config options as needed
    });

    console.log('Starting Kolosal AI API Server...');
    console.log(`Host: ${host}`);
    console.log(`Port: ${port}`);
    console.log(`CORS: ${corsEnabled ? 'enabled' : 'disabled'}`);

    // Initialize the config - this is critical for proper operation
    console.log('Initializing configuration...');
    await config.initialize();
    
    // Initialize auth - for standalone server, we use NO_AUTH as default
    // API requests can provide their own authentication
    console.log('Setting up authentication...');
    await config.refreshAuth(AuthType.NO_AUTH);

    const server = await startApiServer(config, {
      port: Number(port),
      host: String(host),
      enableCors: corsEnabled
    });

    console.log(`Server running on http://${host}:${server.port}`);
    console.log(`Health check: http://${host}:${server.port}/healthz`);
    console.log(`Status: http://${host}:${server.port}/status`);
    console.log(`Generate: POST http://${host}:${server.port}/v1/generate`);

    // Graceful shutdown
    process.on('SIGINT', async () => {
      console.log('\nShutting down server...');
      try {
        await server.close();
        console.log('Server closed successfully');
        process.exit(0);
      } catch (error) {
        console.error('Error during shutdown:', error);
        process.exit(1);
      }
    });

    process.on('SIGTERM', async () => {
      console.log('\nReceived SIGTERM, shutting down...');
      try {
        await server.close();
        process.exit(0);
      } catch (error) {
        console.error('Error during shutdown:', error);
        process.exit(1);
      }
    });

  } catch (error) {
    console.error('Failed to start server:', error);
    process.exit(1);
  }
}

function getArgValue(args: string[], flag: string): string | null {
  const index = args.indexOf(flag);
  if (index !== -1 && index + 1 < args.length) {
    return args[index + 1];
  }
  return null;
}

// Only run if this is the main module
if (import.meta.url === `file://${process.argv[1]}`) {
  main().catch((error) => {
    console.error('Unhandled error:', error);
    process.exit(1);
  });
}