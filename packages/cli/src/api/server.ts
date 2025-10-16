/**
 * @license
 * Copyright 2025 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import http, { IncomingMessage, ServerResponse } from 'http';
import { URL } from 'url';
import type { Config, ToolCallRequestInfo } from '@kolosal-ai/kolosal-ai-core';
import { executeToolCall, GeminiEventType, ApprovalMode } from '@kolosal-ai/kolosal-ai-core';
import type { Content, Part } from '@google/genai';
import { handleAtCommand } from '../ui/hooks/atCommandProcessor.js';

export type ApiServerOptions = {
  port: number;
  host?: string; // default 127.0.0.1
  enableCors?: boolean;
  // If set, require Authorization: Bearer <token>
  authToken?: string;
};

export type ApiServer = {
  port: number;
  close: () => Promise<void>;
};

type TranscriptItem =
  | { type: 'assistant'; content: string }
  | { type: 'tool_call'; name: string; arguments?: unknown }
  | { type: 'tool_result'; name: string; ok: boolean; responseText?: string; error?: string };

function writeCors(res: ServerResponse, enableCors: boolean | undefined) {
  if (!enableCors) return;
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET,POST,OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization');
}

function sendJson(
  res: ServerResponse,
  status: number,
  body: Record<string, unknown>,
  enableCors?: boolean,
) {
  writeCors(res, enableCors);
  res.statusCode = status;
  res.setHeader('Content-Type', 'application/json; charset=utf-8');
  res.end(JSON.stringify(body));
}

async function readJsonBody<T = any>(req: IncomingMessage): Promise<T> {
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

function unauthorized(res: ServerResponse, enableCors?: boolean) {
  sendJson(res, 401, { error: 'Unauthorized' }, enableCors);
}

function checkAuth(
  req: IncomingMessage,
  res: ServerResponse,
  token: string | undefined,
  enableCors?: boolean,
): boolean {
  if (!token) return true;
  const authHeader = req.headers['authorization'];
  if (!authHeader || !authHeader.startsWith('Bearer ')) {
    unauthorized(res, enableCors);
    return false;
  }
  const provided = authHeader.slice('Bearer '.length);
  if (provided !== token) {
    unauthorized(res, enableCors);
    return false;
  }
  return true;
}

type StreamEventCallback = (event: TranscriptItem) => void;

async function handleGenerateOnce(
  config: Config,
  input: string,
  promptId: string,
  signal: AbortSignal,
  onChunk?: (text: string) => void,
  onEvent?: StreamEventCallback,
  conversationHistory?: Content[],
): Promise<{ finalText: string; transcript: TranscriptItem[]; history: Content[] }> {
  // Set approval mode to YOLO for API requests to auto-approve tool calls
  const originalApprovalMode = config.getApprovalMode();
  config.setApprovalMode(ApprovalMode.YOLO);

  try {
    const geminiClient = config.getGeminiClient();

  const { processedQuery, shouldProceed } = await handleAtCommand({
    query: input,
    config,
    addItem: (_item, _timestamp) => 0,
    onDebugMessage: () => {},
    messageId: Date.now(),
    signal,
  });

  if (!shouldProceed || !processedQuery) {
    throw new Error('Error processing @-command in input.');
  }

  // Start with conversation history if provided, otherwise start fresh
  let allMessages: Content[] = conversationHistory ? [...conversationHistory] : [];
  
  // Add the new user message
  const userMessage: Content = {
    role: 'user',
    parts: processedQuery as Part[],
  };
  allMessages.push(userMessage);

  let currentMessages: Content[] = [userMessage];

  let finalText = '';
  const transcript: TranscriptItem[] = [];

  while (true) {
    const toolCallRequests: ToolCallRequestInfo[] = [];
    let turnText = '';

    const responseStream = geminiClient.sendMessageStream(
      currentMessages[0]?.parts || [],
      signal,
      promptId,
    );

    for await (const event of responseStream) {
      if (signal.aborted) return { finalText, transcript, history: allMessages };
      if (event.type === GeminiEventType.Content) {
        finalText += event.value;
        turnText += event.value;
        onChunk?.(event.value);
      } else if (event.type === GeminiEventType.ToolCallRequest) {
        toolCallRequests.push(event.value);
      }
    }

    if (turnText) {
      const assistantEvent: TranscriptItem = { type: 'assistant', content: turnText };
      transcript.push(assistantEvent);
      // Only emit assistant event if we're NOT streaming content chunks
      // (to avoid duplication - content was already streamed via onChunk)
      if (!onChunk) {
        onEvent?.(assistantEvent);
      }
      
      // Add assistant message to conversation history
      allMessages.push({
        role: 'model',
        parts: [{ text: turnText }],
      });
    }

    if (toolCallRequests.length > 0) {
      const toolResponseParts: Part[] = [];
      for (const requestInfo of toolCallRequests) {
        // Record and stream the tool call
        let toolCallEvent: TranscriptItem;
        try {
          const args = (requestInfo as any)?.args ?? undefined;
          toolCallEvent = { type: 'tool_call', name: requestInfo.name, arguments: args };
        } catch {
          toolCallEvent = { type: 'tool_call', name: requestInfo.name };
        }
        transcript.push(toolCallEvent);
        onEvent?.(toolCallEvent);

        const toolResponse = await executeToolCall(
          config,
          requestInfo,
          signal,
        );

        // Record and stream result
        let toolResultEvent: TranscriptItem;
        if (toolResponse.error) {
          toolResultEvent = {
            type: 'tool_result',
            name: requestInfo.name,
            ok: false,
            error: toolResponse.error.message,
          };
        } else {
          toolResultEvent = {
            type: 'tool_result',
            name: requestInfo.name,
            ok: true,
            responseText:
              typeof toolResponse.resultDisplay === 'string'
                ? toolResponse.resultDisplay
                : undefined,
          };
        }
        transcript.push(toolResultEvent);
        onEvent?.(toolResultEvent);

        if (toolResponse.responseParts) {
          toolResponseParts.push(...toolResponse.responseParts);
        }
      }
      currentMessages = [{ role: 'user', parts: toolResponseParts }];
      // Add tool response to conversation history
      allMessages.push({ role: 'user', parts: toolResponseParts });
    } else {
      break;
    }
  }

  return { finalText, transcript, history: allMessages };
  } finally {
    // Restore original approval mode
    config.setApprovalMode(originalApprovalMode);
  }
}

export function startApiServer(
  config: Config,
  options: ApiServerOptions,
): Promise<ApiServer> {
  const enableCors = options.enableCors ?? true;
  const authToken = options.authToken ?? process.env['KOLOSAL_CLI_API_TOKEN'];

  const server = http.createServer(async (req, res) => {
    // Basic routing
    try {
      if (!req.url) {
        return sendJson(res, 400, { error: 'Missing URL' }, enableCors);
      }
      const url = new URL(req.url, 'http://localhost');

      if (req.method === 'OPTIONS') {
        writeCors(res, enableCors);
        res.statusCode = 204;
        return res.end();
      }

      if (url.pathname === '/healthz' && req.method === 'GET') {
        return sendJson(
          res,
          200,
          {
            status: 'ok',
            timestamp: new Date().toISOString(),
          },
          enableCors,
        );
      }

      if (url.pathname === '/v1/generate' && req.method === 'POST') {
        if (!checkAuth(req, res, authToken, enableCors)) return;
        let body: any;
        try {
          body = await readJsonBody(req);
        } catch (e) {
          return sendJson(
            res,
            400,
            { error: (e as Error).message },
            enableCors,
          );
        }

        const input = (body?.input ?? '').toString();
        const stream = Boolean(body?.stream);
        const promptId =
          (body?.prompt_id as string) || Math.random().toString(16).slice(2);
        const history = body?.history as Content[] | undefined;

        if (!input) {
          return sendJson(
            res,
            400,
            { error: 'Missing required field: input' },
            enableCors,
          );
        }

        const abortController = new AbortController();
        req.on('close', () => abortController.abort());

        if (stream) {
          writeCors(res, enableCors);
          res.statusCode = 200;
          res.setHeader('Content-Type', 'text/event-stream; charset=utf-8');
          res.setHeader('Cache-Control', 'no-cache');
          res.setHeader('Connection', 'keep-alive');

          const writeSse = (event: string, data: string) => {
            res.write(`event: ${event}\n`);
            res.write(`data: ${data.replace(/\n/g, '\\n')}\n\n`);
          };

          try {
            const { history: updatedHistory } = await handleGenerateOnce(
              config,
              input,
              promptId,
              abortController.signal,
              (chunk) => writeSse('content', chunk),
              (item) => {
                // Stream each transcript item as it happens
                writeSse(item.type, JSON.stringify(item));
              },
              history,
            );
            // Send the updated conversation history so client can maintain state
            writeSse('history', JSON.stringify(updatedHistory));
            writeSse('done', 'true');
            res.end();
          } catch (e) {
            writeSse(
              'error',
              JSON.stringify({ message: (e as Error).message }),
            );
            res.end();
          }
          return;
        } else {
          try {
            const { finalText, transcript, history: updatedHistory } = await handleGenerateOnce(
              config,
              input,
              promptId,
              abortController.signal,
              undefined, // No content streaming for non-streaming mode
              undefined, // No event callback for non-streaming mode
              history,
            );
            return sendJson(
              res,
              200,
              { 
                output: finalText, 
                prompt_id: promptId, 
                messages: transcript,
                history: updatedHistory,
              },
              enableCors,
            );
          } catch (e) {
            return sendJson(
              res,
              500,
              { error: (e as Error).message },
              enableCors,
            );
          }
        }
      }

      // Not found
      return sendJson(res, 404, { error: 'Not Found' }, enableCors);
    } catch (err) {
      // Last-ditch error
      try {
        return sendJson(
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
