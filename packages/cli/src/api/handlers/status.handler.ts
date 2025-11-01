/**
 * @license
 * Copyright 2025 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import type { RouteHandler, HttpContext } from '../types/index.js';
import { HttpUtils } from '../utils/http.js';

export class StatusHandler implements RouteHandler {
  async handle(context: HttpContext): Promise<void> {
    const { res, enableCors } = context;
    
    HttpUtils.sendJson(
      res,
      200,
      {
        status: 'ready',
        timestamp: new Date().toISOString(),
        version: '1.0.0', // TODO: Get from package.json
        mode: 'server-only',
        endpoints: {
          generate: '/v1/generate',
          health: '/healthz',
          status: '/status'
        },
        features: {
          streaming: true,
          conversationHistory: true,
          toolExecution: true
        }
      },
      enableCors,
    );
  }
}