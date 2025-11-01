/**
 * @license
 * Copyright 2025 Kolosal Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import http from 'http';
import type { Config } from '@kolosal-ai/kolosal-ai-core';
import type { ApiServerOptions, ApiServer, HttpContext } from './types/index.js';
import { Router } from './router.js';
import { CorsMiddleware } from './middleware/cors.middleware.js';
import { HealthHandler, StatusHandler, GenerateHandler } from './handlers/index.js';
import { GenerationService } from './services/generation.service.js';
import { HttpUtils } from './utils/http.js';

export class ApiServerFactory {
  static create(config: Config, options: ApiServerOptions): Promise<ApiServer> {
    const enableCors = options.enableCors ?? true;
    const router = this.setupRouter(config);

    const server = http.createServer(async (req, res) => {
      try {
        const context: HttpContext = {
          req,
          res,
          config,
          enableCors,
        };

        await router.handle(context);
      } catch (err) {
        // Last-ditch error handling
        try {
          HttpUtils.sendJson(
            res,
            500,
            { error: (err as Error).message || 'Internal Server Error' },
            enableCors,
          );
        } catch {
          res.statusCode = 500;
          res.end();
        }
      }
    });

    return new Promise<ApiServer>((resolve, reject) => {
      server.on('error', reject);
      const host = options.host ?? '127.0.0.1';
      
      server.listen(options.port, host, () => {
        resolve({
          port: options.port,
          close: () =>
            new Promise<void>((resClose) => server.close(() => resClose())),
        });
      });
    });
  }

  private static setupRouter(config: Config): Router {
    const router = new Router();
    const generationService = new GenerationService(config);

    // Add middleware
    router.addMiddleware(new CorsMiddleware());

    // Add routes
    router.addRoute('GET', '/healthz', new HealthHandler());
    router.addRoute('GET', '/status', new StatusHandler());
    router.addRoute('POST', '/v1/generate', new GenerateHandler(generationService));

    return router;
  }
}