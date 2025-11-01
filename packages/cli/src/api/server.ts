/**
 * @license
 * Copyright 2025 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import type { Config } from '@kolosal-ai/kolosal-ai-core';
import type { ApiServerOptions, ApiServer } from './types/index.js';
import { ApiServerFactory } from './server.factory.js';

// Re-export types for backward compatibility
export type { ApiServerOptions, ApiServer } from './types/index.js';

/**
 * Creates and starts an API server instance.
 */
export function startApiServer(
  config: Config,
  options: ApiServerOptions,
): Promise<ApiServer> {
  return ApiServerFactory.create(config, options);
}
