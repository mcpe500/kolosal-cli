/**
 * @license
 * Copyright 2025 Kolosal Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { URL } from 'url';
import type { RouteHandler, HttpContext, Middleware } from './types/index.js';
import { HttpUtils } from './utils/http.js';

export interface Route {
  method: string;
  path: string;
  handler: RouteHandler;
}

export class Router {
  private routes: Route[] = [];
  private middlewares: Middleware[] = [];

  addRoute(method: string, path: string, handler: RouteHandler): void {
    this.routes.push({ method, path, handler });
  }

  addMiddleware(middleware: Middleware): void {
    this.middlewares.push(middleware);
  }

  async handle(context: HttpContext): Promise<void> {
    const { req, res, enableCors } = context;

    if (!req.url) {
      return HttpUtils.sendJson(res, 400, { error: 'Missing URL' }, enableCors);
    }

    const url = new URL(req.url, 'http://localhost');
    const method = req.method || 'GET';

    // Find matching route
    const route = this.routes.find(r => r.method === method && r.path === url.pathname);

    if (!route) {
      return HttpUtils.sendJson(res, 404, { error: 'Not Found' }, enableCors);
    }

    // Execute middlewares and then the route handler
    await this.executeMiddlewareChain(context, () => route.handler.handle(context));
  }

  private async executeMiddlewareChain(
    context: HttpContext,
    finalHandler: () => Promise<void>,
  ): Promise<void> {
    let index = 0;

    const next = async (): Promise<void> => {
      if (index >= this.middlewares.length) {
        await finalHandler();
        return;
      }

      const middleware = this.middlewares[index++];
      await middleware.process(context, next);
    };

    await next();
  }
}