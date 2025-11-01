/**
 * @license
 * Copyright 2025 Kolosal Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import type { IncomingMessage, ServerResponse } from 'http';

export class HttpUtils {
  static writeCors(res: ServerResponse, enableCors: boolean): void {
    if (!enableCors) return;
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET,POST,OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization');
  }

  static sendJson(
    res: ServerResponse,
    status: number,
    body: Record<string, unknown>,
    enableCors = false,
  ): void {
    this.writeCors(res, enableCors);
    res.statusCode = status;
    res.setHeader('Content-Type', 'application/json; charset=utf-8');
    res.end(JSON.stringify(body));
  }

  static async readJsonBody<T = any>(req: IncomingMessage): Promise<T> {
    const chunks: Buffer[] = [];
    for await (const chunk of req) {
      chunks.push(Buffer.isBuffer(chunk) ? chunk : Buffer.from(chunk));
    }
    const text = Buffer.concat(chunks).toString('utf8');
    if (!text) return {} as T;
    try {
      return JSON.parse(text) as T;
    } catch (e) {
      const err = e as Error;
      throw new Error(`Invalid JSON body: ${err.message}`);
    }
  }

  static writeSse(res: ServerResponse, event: string, data: string): void {
    res.write(`event: ${event}\n`);
    res.write(`data: ${data.replace(/\n/g, '\\n')}\n\n`);
  }

  static setupSseHeaders(res: ServerResponse, enableCors: boolean): void {
    this.writeCors(res, enableCors);
    res.statusCode = 200;
    res.setHeader('Content-Type', 'text/event-stream; charset=utf-8');
    res.setHeader('Cache-Control', 'no-cache');
    res.setHeader('Connection', 'keep-alive');
  }
}