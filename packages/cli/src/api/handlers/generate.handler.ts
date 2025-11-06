/**
 * @license
 * Copyright 2025 Kolosal Inc.
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
    const model = body?.model;
    const apiKey = body?.api_key;
    const baseUrl = body?.base_url;

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
          model,
          apiKey,
          baseUrl,
        );
      } else {
        await this.handleNonStreamingResponse(
          input,
          promptId,
          history,
          abortController.signal,
          res,
          enableCors,
          model,
          apiKey,
          baseUrl,
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
    model?: string,
    apiKey?: string,
    baseUrl?: string,
  ): Promise<void> {
    HttpUtils.setupSseHeaders(res, enableCors);

    // Track state to filter unwanted newline chunks
    let lastEventType: string | null = null;
    let previousContentEmpty = true;

    const { history: updatedHistory } = await this.generationService.generateResponse(
      input,
      promptId,
      signal,
      {
        onContentChunk: (chunk) => {
          // Filter out leading newlines if previous content is empty or after tool results
          let filteredChunk = chunk;
          if (previousContentEmpty || lastEventType === 'tool_result') {
            filteredChunk = chunk.replace(/^\n+/, '');
          }
          
          // Limit consecutive newlines to maximum of 2
          filteredChunk = filteredChunk.replace(/\n{3,}/g, '\n\n');
          
          // Skip if chunk becomes empty after filtering
          if (filteredChunk === '') {
            return;
          }
          
          HttpUtils.writeSse(res, 'content', filteredChunk);
          previousContentEmpty = filteredChunk.trim() === '';
          lastEventType = 'content';
        },
        onEvent: (item) => {
          HttpUtils.writeSse(res, item.type, JSON.stringify(item));
          lastEventType = item.type;
          // Reset content state after tool events
          if (item.type === 'tool_call' || item.type === 'tool_result') {
            previousContentEmpty = true;
          }
        },
        conversationHistory: history,
        model,
        apiKey,
        baseUrl,
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
    model?: string,
    apiKey?: string,
    baseUrl?: string,
  ): Promise<void> {
    const { finalText, transcript, history: updatedHistory } = 
      await this.generationService.generateResponse(
        input,
        promptId,
        signal,
        {
          conversationHistory: history,
          model,
          apiKey,
          baseUrl,
        },
      );

    // Apply similar filtering logic as streaming to clean up final text
    const cleanedFinalText = this.cleanFinalText(finalText, transcript);

    HttpUtils.sendJson(
      res,
      200,
      { 
        output: cleanedFinalText, 
        prompt_id: promptId, 
        messages: transcript,
        history: updatedHistory,
      },
      enableCors,
    );
  }

  private cleanFinalText(finalText: string, transcript: any[]): string {
    if (!finalText) return finalText;

    let cleanedText = finalText;

    // Check if the last events in transcript were tool-related
    const lastEvent = transcript[transcript.length - 1];
    const secondLastEvent = transcript.length > 1 ? transcript[transcript.length - 2] : null;
    
    // If final text starts with newlines and follows tool results, clean them up
    if (cleanedText.startsWith('\n') && 
        (lastEvent?.type === 'tool_result' || secondLastEvent?.type === 'tool_result')) {
      cleanedText = cleanedText.replace(/^\n+/, '');
    }

    // Limit consecutive newlines to maximum of 2
    cleanedText = cleanedText.replace(/\n{3,}/g, '\n\n');

    // Also clean up cases where the text is just newlines
    if (cleanedText.trim() === '') {
      return '';
    }

    return cleanedText;
  }
}