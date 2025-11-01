/**
 * @license
 * Copyright 2025 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import type { Middleware, HttpContext } from '../types/index.js';
import { HttpUtils } from '../utils/http.js';

export class CorsMiddleware implements Middleware {
  async process(context: HttpContext, next: () => Promise<void>): Promise<void> {
    const { req, res, enableCors } = context;

    if (req.method === 'OPTIONS') {
      HttpUtils.writeCors(res, enableCors);
      res.statusCode = 204;
      res.end();
      return;
    }

    await next();
  }
}