/**
 * @license
 * Copyright 2025 Kolosal Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import type { Config, ToolCallRequestInfo } from '@kolosal-ai/kolosal-ai-core';
import { executeToolCall, GeminiEventType, ApprovalMode, GeminiClient, AuthType } from '@kolosal-ai/kolosal-ai-core';
import type { Content, Part } from '@google/genai';
import { handleAtCommand } from '../utils/atCommandProcessor.js';
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
      model?: string;
      apiKey?: string;
      baseUrl?: string;
      workingDirectory?: string;
    } = {},
  ): Promise<GenerationResult> {
    const { onContentChunk, onEvent, conversationHistory, model, apiKey, baseUrl, workingDirectory } = options;

    // Set approval mode to YOLO for API requests to auto-approve tool calls
    const originalApprovalMode = this.config.getApprovalMode();
    this.config.setApprovalMode(ApprovalMode.YOLO);

    // Store original workspace context if we need to temporarily change it
    let originalWorkspaceContext: any = null;
    let tempWorkspaceContext: any = null;

    if (workingDirectory) {
      try {
        // Import the WorkspaceContext class and fs
        const { WorkspaceContext } = await import('@kolosal-ai/kolosal-ai-core');
        const fs = await import('node:fs/promises');
        const path = await import('node:path');
        
        // Resolve the working directory path
        const resolvedWorkingDirectory = path.resolve(workingDirectory);
        
        // Check if the directory exists, create it if it doesn't
        try {
          await fs.access(resolvedWorkingDirectory);
        } catch (error) {
          console.log(`[API] Creating working directory: ${resolvedWorkingDirectory}`);
          await fs.mkdir(resolvedWorkingDirectory, { recursive: true });
        }
        
        // Store the original workspace context
        originalWorkspaceContext = this.config.getWorkspaceContext();
        
        // Create a temporary workspace context with the specified working directory
        tempWorkspaceContext = new WorkspaceContext(resolvedWorkingDirectory, []);
        
        // Temporarily replace the workspace context in the config
        this.config.setWorkspaceContext(tempWorkspaceContext);
      } catch (error) {
        console.warn('[API] Failed to set working directory:', error);
        // Continue with the original workspace context
      }
    }

    try {
      let geminiClient = this.config.getGeminiClient();

      // Create a temporary client with custom model/API key/baseUrl if provided
      if (model || apiKey || baseUrl) {
        const currentConfig = this.config.getContentGeneratorConfig();
        const customConfig = {
          ...currentConfig,
          ...(model && { model }),
          ...(apiKey && { apiKey, authType: AuthType.USE_OPENAI }),
          ...(baseUrl && { baseUrl, authType: AuthType.USE_OPENAI }),
        };

        // Create a temporary client for this request
        geminiClient = new GeminiClient(this.config);
        await geminiClient.initialize(customConfig);
        
        // Ensure the client is properly initialized before setting history
        if (!geminiClient.isInitialized()) {
          throw new Error('Failed to initialize custom Gemini client');
        }
      } else {
        // No custom parameters provided - check if default client exists and is initialized
        if (!geminiClient) {
          throw new Error('No Gemini client available from configuration and no custom parameters provided');
        }
        
        if (!geminiClient.isInitialized()) {
          throw new Error('Default Gemini client is not properly initialized and no custom parameters provided');
        }
      }
      
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
      // Restore original workspace context if it was changed
      if (originalWorkspaceContext && workingDirectory) {
        this.config.setWorkspaceContext(originalWorkspaceContext);
      }
      
      // Restore original approval mode
      this.config.setApprovalMode(originalApprovalMode);
    }
  }

  private setupConversationHistory(geminiClient: any, conversationHistory?: Content[]): void {
    // Check if the client is properly initialized
    if (!geminiClient || !geminiClient.isInitialized()) {
      console.error('[API] Gemini client is not properly initialized');
      throw new Error('Gemini client is not properly initialized');
    }

    if (conversationHistory && conversationHistory.length > 0) {
      try {
        geminiClient.setHistory(conversationHistory);
      } catch (e) {
        console.error('[API] Failed to set provided conversation history:', e);
        // Fall back to clearing history to ensure statelessness
        try {
          geminiClient.setHistory([]);
        } catch (clearError) {
          console.error('[API] Failed to clear history as fallback:', clearError);
          throw new Error('Unable to initialize conversation history');
        }
      }
    } else {
      try {
        geminiClient.setHistory([]);
      } catch (e) {
        console.error('[API] Failed to clear conversation history:', e);
        throw new Error('Unable to initialize conversation history');
      }
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
      // Include full response details similar to non-streaming mode
      const fullResponse: any = {
        type: 'tool_result',
        name: requestInfo.name,
        ok: true,
      };

      // Add responseText if available
      if (typeof toolResponse.resultDisplay === 'string') {
        fullResponse.responseText = toolResponse.resultDisplay;
      }

      // Add full response details from responseParts if available
      if (toolResponse.responseParts && toolResponse.responseParts.length > 0) {
        const responsePart = toolResponse.responseParts[0];
        if (responsePart.functionResponse && responsePart.functionResponse.response) {
          fullResponse.response = responsePart.functionResponse.response;
        }
      }

      return fullResponse;
    }
  }
}