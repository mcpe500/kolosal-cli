/**
 * @license
 * Copyright 2025 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import type { RouteHandler, HttpContext, GenerateRequest } from '../types/index.js';
import { HttpUtils } from '../utils/http.js';
import { GenerationService } from '../services/generation.service.js';

export class GenerateHandler implements RouteHandler {
  constructor(private generationService: GenerationService) {}

  async handle(context: HttpContext): Promise<void> {
    const { req, res, enableCors } = context;

    let body: GenerateRequest;
    try {
      body = await HttpUtils.readJsonBody<GenerateRequest>(req);
    } catch (e) {
      return HttpUtils.sendJson(
        res,
        400,
        { error: (e as Error).message },
        enableCors,
      );
    }

    const input = (body?.input ?? '').toString();
    const stream = Boolean(body?.stream);
    const promptId = body?.prompt_id || Math.random().toString(16).slice(2);
    const history = body?.history;

    if (!input) {
      return HttpUtils.sendJson(
        res,
        400,
        { error: 'Missing required field: input' },
        enableCors,
      );
    }

    const abortController = new AbortController();
    req.on('close', () => abortController.abort());

    try {
      if (stream) {
        await this.handleStreamingResponse(
          input,
          promptId,
          history,
          abortController.signal,
          res,
          enableCors,
        );
      } else {
        await this.handleNonStreamingResponse(
          input,
          promptId,
          history,
          abortController.signal,
          res,
          enableCors,
        );
      }
    } catch (e) {
      if (stream) {
        HttpUtils.writeSse(res, 'error', JSON.stringify({ message: (e as Error).message }));
        res.end();
      } else {
        HttpUtils.sendJson(res, 500, { error: (e as Error).message }, enableCors);
      }
    }
  }

  private async handleStreamingResponse(
    input: string,
    promptId: string,
    history: any,
    signal: AbortSignal,
    res: any,
    enableCors: boolean,
  ): Promise<void> {
    HttpUtils.setupSseHeaders(res, enableCors);

    const { history: updatedHistory } = await this.generationService.generateResponse(
      input,
      promptId,
      signal,
      {
        onContentChunk: (chunk) => HttpUtils.writeSse(res, 'content', chunk),
        onEvent: (item) => HttpUtils.writeSse(res, item.type, JSON.stringify(item)),
        conversationHistory: history,
      },
    );

    // Send the updated conversation history so client can maintain state
    HttpUtils.writeSse(res, 'history', JSON.stringify(updatedHistory));
    HttpUtils.writeSse(res, 'done', 'true');
    res.end();
  }

  private async handleNonStreamingResponse(
    input: string,
    promptId: string,
    history: any,
    signal: AbortSignal,
    res: any,
    enableCors: boolean,
  ): Promise<void> {
    const { finalText, transcript, history: updatedHistory } = 
      await this.generationService.generateResponse(
        input,
        promptId,
        signal,
        {
          conversationHistory: history,
        },
      );

    HttpUtils.sendJson(
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
  }
}