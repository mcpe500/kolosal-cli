/**
 * @license
 * Copyright 2025 Kolosal Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import type { Config, ToolCallRequestInfo } from '@kolosal-ai/kolosal-ai-core';
import { executeToolCall, GeminiEventType, ApprovalMode } from '@kolosal-ai/kolosal-ai-core';
import type { Content, Part } from '@google/genai';
import { handleAtCommand } from '../../ui/hooks/atCommandProcessor.js';
import type {
  TranscriptItem,
  GenerationResult,
  StreamEventCallback,
  ContentStreamCallback,
} from '../types/index.js';

export class GenerationService {
  constructor(private config: Config) {}

  async generateResponse(
    input: string,
    promptId: string,
    signal: AbortSignal,
    options: {
      onContentChunk?: ContentStreamCallback;
      onEvent?: StreamEventCallback;
      conversationHistory?: Content[];
    } = {},
  ): Promise<GenerationResult> {
    const { onContentChunk, onEvent, conversationHistory } = options;

    // Set approval mode to YOLO for API requests to auto-approve tool calls
    const originalApprovalMode = this.config.getApprovalMode();
    this.config.setApprovalMode(ApprovalMode.YOLO);

    try {
      const geminiClient = this.config.getGeminiClient();
      
      this.setupConversationHistory(geminiClient, conversationHistory);
      this.logDebugInfo();

      const processedQuery = await this.processAtCommand(input, signal);
      
      return await this.runGenerationLoop(
        geminiClient,
        processedQuery,
        promptId,
        signal,
        conversationHistory || [],
        onContentChunk,
        onEvent,
      );
    } finally {
      // Restore original approval mode
      this.config.setApprovalMode(originalApprovalMode);
    }
  }

  private setupConversationHistory(geminiClient: any, conversationHistory?: Content[]): void {
    if (conversationHistory && conversationHistory.length > 0) {
      try {
        geminiClient.setHistory(conversationHistory);
      } catch (e) {
        console.error('[API] Failed to set provided conversation history:', e);
        // Fall back to clearing history to ensure statelessness
        geminiClient.setHistory([]);
      }
    } else {
      geminiClient.setHistory([]);
    }
  }

  private logDebugInfo(): void {
    console.error('[API] Available tools:', this.config.getExcludeTools());
    console.error('[API] Approval mode:', this.config.getApprovalMode());
  }

  private async processAtCommand(input: string, signal: AbortSignal): Promise<Part[]> {
    const { processedQuery, shouldProceed } = await handleAtCommand({
      query: input,
      config: this.config,
      addItem: (_item, _timestamp) => 0,
      onDebugMessage: () => {},
      messageId: Date.now(),
      signal,
    });

    if (!shouldProceed || !processedQuery) {
      throw new Error('Error processing @-command in input.');
    }

    return processedQuery as Part[];
  }

  private async runGenerationLoop(
    geminiClient: any,
    processedQuery: Part[],
    promptId: string,
    signal: AbortSignal,
    conversationHistory: Content[],
    onContentChunk?: ContentStreamCallback,
    onEvent?: StreamEventCallback,
  ): Promise<GenerationResult> {
    let allMessages: Content[] = [...conversationHistory];
    
    // Add the new user message
    const userMessage: Content = {
      role: 'user',
      parts: processedQuery,
    };
    allMessages.push(userMessage);

    let currentMessages: Content[] = [userMessage];
    let finalText = '';
    const transcript: TranscriptItem[] = [];

    while (true) {
      const result = await this.processGenerationTurn(
        geminiClient,
        currentMessages,
        promptId,
        signal,
        onContentChunk,
        onEvent,
      );

      finalText += result.turnText;
      transcript.push(...result.transcriptItems);
      allMessages.push(...result.newMessages);

      if (result.toolRequests.length > 0) {
        const { toolResponseParts, toolMessages } = await this.processToolCalls(
          result.toolRequests,
          signal,
          transcript,
          onEvent,
        );
        
        currentMessages = [{ role: 'user', parts: toolResponseParts }];
        allMessages.push(...toolMessages);
      } else {
        break;
      }
    }

    return { finalText, transcript, history: allMessages };
  }

  private async processGenerationTurn(
    geminiClient: any,
    currentMessages: Content[],
    promptId: string,
    signal: AbortSignal,
    onContentChunk?: ContentStreamCallback,
    onEvent?: StreamEventCallback,
  ): Promise<{
    turnText: string;
    transcriptItems: TranscriptItem[];
    newMessages: Content[];
    toolRequests: ToolCallRequestInfo[];
  }> {
    const toolCallRequests: ToolCallRequestInfo[] = [];
    let turnText = '';
    const transcriptItems: TranscriptItem[] = [];
    const newMessages: Content[] = [];

    const responseStream = geminiClient.sendMessageStream(
      currentMessages[0]?.parts || [],
      signal,
      promptId,
    );

    for await (const event of responseStream) {
      if (signal.aborted) break;
      
      if (event.type === GeminiEventType.Content) {
        turnText += event.value;
        onContentChunk?.(event.value);
      } else if (event.type === GeminiEventType.ToolCallRequest) {
        toolCallRequests.push(event.value);
      }
    }

    if (turnText) {
      const assistantEvent: TranscriptItem = { type: 'assistant', content: turnText };
      transcriptItems.push(assistantEvent);
      
      // Only emit assistant event if we're NOT streaming content chunks
      if (!onContentChunk) {
        onEvent?.(assistantEvent);
      }
      
      // Add assistant message to conversation history
      newMessages.push({
        role: 'model',
        parts: [{ text: turnText }],
      });
    }

    return { turnText, transcriptItems, newMessages, toolRequests: toolCallRequests };
  }

  private async processToolCalls(
    toolRequests: ToolCallRequestInfo[],
    signal: AbortSignal,
    transcript: TranscriptItem[],
    onEvent?: StreamEventCallback,
  ): Promise<{ toolResponseParts: Part[]; toolMessages: Content[] }> {
    const toolResponseParts: Part[] = [];
    const toolMessages: Content[] = [];

    for (const requestInfo of toolRequests) {
      // Record and stream the tool call
      const toolCallEvent = this.createToolCallEvent(requestInfo);
      transcript.push(toolCallEvent);
      onEvent?.(toolCallEvent);

      const toolResponse = await executeToolCall(this.config, requestInfo, signal);

      // Record and stream result
      const toolResultEvent = this.createToolResultEvent(requestInfo, toolResponse);
      transcript.push(toolResultEvent);
      onEvent?.(toolResultEvent);

      if (toolResponse.responseParts) {
        toolResponseParts.push(...toolResponse.responseParts);
      }
    }

    // Add tool response to conversation history
    if (toolResponseParts.length > 0) {
      toolMessages.push({ role: 'user', parts: toolResponseParts });
    }

    return { toolResponseParts, toolMessages };
  }

  private createToolCallEvent(requestInfo: ToolCallRequestInfo): TranscriptItem {
    try {
      const args = (requestInfo as any)?.args ?? undefined;
      return { type: 'tool_call', name: requestInfo.name, arguments: args };
    } catch {
      return { type: 'tool_call', name: requestInfo.name };
    }
  }

  private createToolResultEvent(requestInfo: ToolCallRequestInfo, toolResponse: any): TranscriptItem {
    if (toolResponse.error) {
      return {
        type: 'tool_result',
        name: requestInfo.name,
        ok: false,
        error: toolResponse.error.message,
      };
    } else {
      return {
        type: 'tool_result',
        name: requestInfo.name,
        ok: true,
        responseText:
          typeof toolResponse.resultDisplay === 'string'
            ? toolResponse.resultDisplay
            : undefined,
      };
    }
  }
}