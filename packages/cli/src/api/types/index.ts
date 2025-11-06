/**
 * @license
 * Copyright 2025 Kolosal Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import type { IncomingMessage, ServerResponse } from 'http';
import type { Config } from '@kolosal-ai/kolosal-ai-core';
import type { Content } from '@google/genai';

export interface ApiServerOptions {
  port: number;
  host?: string;
  enableCors?: boolean;
}

export interface ApiServer {
  port: number;
  close: () => Promise<void>;
}

export interface HttpContext {
  req: IncomingMessage;
  res: ServerResponse;
  config: Config;
  enableCors: boolean;
}

export interface RouteHandler {
  handle(context: HttpContext): Promise<void>;
}

export interface Middleware {
  process(context: HttpContext, next: () => Promise<void>): Promise<void>;
}

export type TranscriptItem =
  | { type: 'assistant'; content: string }
  | { type: 'tool_call'; name: string; arguments?: unknown }
  | { type: 'tool_result'; name: string; ok: boolean; responseText?: string; error?: string };

export interface GenerateRequest {
  input: string;
  stream?: boolean;
  prompt_id?: string;
  history?: Content[];
  model?: string;
  api_key?: string;
  base_url?: string;
  working_directory?: string;
}

export interface GenerateResponse {
  output: string;
  prompt_id: string;
  messages: TranscriptItem[];
  history: Content[];
}

export interface GenerationResult {
  finalText: string;
  transcript: TranscriptItem[];
  history: Content[];
}

export type StreamEventCallback = (event: TranscriptItem) => void;
export type ContentStreamCallback = (text: string) => void;