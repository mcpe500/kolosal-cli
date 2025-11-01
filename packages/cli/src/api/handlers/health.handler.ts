/**
 * @license
 * Copyright 2025 Kolosal Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import type { RouteHandler, HttpContext } from '../types/index.js';
import { HttpUtils } from '../utils/http.js';

export class HealthHandler implements RouteHandler {
  async handle(context: HttpContext): Promise<void> {
    const { res, enableCors } = context;
    
    HttpUtils.sendJson(
      res,
      200,
      {
        status: 'ok',
        timestamp: new Date().toISOString(),
        mode: 'server',
      },
      enableCors,
    );
  }
}