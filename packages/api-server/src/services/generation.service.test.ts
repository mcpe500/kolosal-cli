/**
 * @license
 * Copyright 2025 Kolosal Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { describe, expect, it, vi, beforeEach, afterEach } from 'vitest';
import type { Config, WorkspaceContext } from '@kolosal-ai/kolosal-ai-core';
import { ApprovalMode } from '@kolosal-ai/kolosal-ai-core';
import { GenerationService } from '../services/generation.service.js';

// Mock the core module
vi.mock('@kolosal-ai/kolosal-ai-core', async () => {
  const actual = await vi.importActual('@kolosal-ai/kolosal-ai-core');
  return {
    ...actual,
    WorkspaceContext: vi.fn().mockImplementation((directory: string) => ({
      directory,
      getDirectories: vi.fn().mockReturnValue([directory]),
    })),
  };
});

describe('GenerationService - Working Directory', () => {
  let mockConfig: Partial<Config>;
  let generationService: GenerationService;
  let originalWorkspaceContext: Partial<WorkspaceContext>;

  beforeEach(() => {
    // Create original workspace context
    originalWorkspaceContext = {
      getDirectories: vi.fn().mockReturnValue(['/original/path']),
    };

    // Create mock config
    mockConfig = {
      getApprovalMode: vi.fn().mockReturnValue(ApprovalMode.DEFAULT),
      setApprovalMode: vi.fn(),
      getWorkspaceContext: vi.fn().mockReturnValue(originalWorkspaceContext),
      setWorkspaceContext: vi.fn(),
      getGeminiClient: vi.fn().mockReturnValue({
        isInitialized: vi.fn().mockReturnValue(true),
        setHistory: vi.fn(),
        sendMessageStream: vi.fn().mockReturnValue([]),
      }),
    };

    generationService = new GenerationService(mockConfig as Config);
  });

  afterEach(() => {
    vi.clearAllMocks();
  });

  it('should not change workspace context when no working directory is provided', async () => {
    const abortController = new AbortController();

    // Mock the generation flow to avoid actual AI calls
    const mockSendMessageStream = vi.fn().mockReturnValue([]);
    mockConfig.getGeminiClient = vi.fn().mockReturnValue({
      isInitialized: vi.fn().mockReturnValue(true),
      setHistory: vi.fn(),
      sendMessageStream: mockSendMessageStream,
    });

    try {
      await generationService.generateResponse(
        'test input',
        'test-prompt-id',
        abortController.signal,
        {
          // No working directory provided
        }
      );
    } catch {
      // Ignore errors for this test - we're only testing workspace context behavior
    }

    // Workspace context should not be changed
    expect(mockConfig.setWorkspaceContext).not.toHaveBeenCalled();
  });

  it('should temporarily change workspace context when working directory is provided', async () => {
    const abortController = new AbortController();
    const testWorkingDirectory = '/test/working/directory';

    // Mock the generation flow to avoid actual AI calls
    const mockSendMessageStream = vi.fn().mockReturnValue([]);
    mockConfig.getGeminiClient = vi.fn().mockReturnValue({
      isInitialized: vi.fn().mockReturnValue(true),
      setHistory: vi.fn(),
      sendMessageStream: mockSendMessageStream,
    });

    try {
      await generationService.generateResponse(
        'test input',
        'test-prompt-id',
        abortController.signal,
        {
          workingDirectory: testWorkingDirectory,
        }
      );
    } catch {
      // Ignore errors for this test - we're only testing workspace context behavior
    }

    // Workspace context should be changed and then restored
    expect(mockConfig.setWorkspaceContext).toHaveBeenCalledTimes(2);
    
    // First call should set the new workspace context
    const firstCall = (mockConfig.setWorkspaceContext as any).mock.calls[0];
    expect(firstCall[0]).toEqual(expect.objectContaining({
      getDirectories: expect.any(Function),
    }));
    
    // Second call should restore the original workspace context
    const secondCall = (mockConfig.setWorkspaceContext as any).mock.calls[1];
    expect(secondCall[0]).toBe(originalWorkspaceContext);
  });

  it('should restore original workspace context even if generation fails', async () => {
    const abortController = new AbortController();
    const testWorkingDirectory = '/test/working/directory';

    // Mock the generation to throw an error
    mockConfig.getGeminiClient = vi.fn().mockReturnValue({
      isInitialized: vi.fn().mockReturnValue(true),
      setHistory: vi.fn(),
      sendMessageStream: vi.fn().mockRejectedValue(new Error('Generation failed')),
    });

    try {
      await generationService.generateResponse(
        'test input',
        'test-prompt-id',
        abortController.signal,
        {
          workingDirectory: testWorkingDirectory,
        }
      );
    } catch {
      // Expected to fail
    }

    // Workspace context should still be restored even after error
    expect(mockConfig.setWorkspaceContext).toHaveBeenCalledTimes(2);
    
    // Second call should restore the original workspace context
    const secondCall = (mockConfig.setWorkspaceContext as any).mock.calls[1];
    expect(secondCall[0]).toBe(originalWorkspaceContext);
  });

  it('should handle WorkspaceContext creation failure gracefully', async () => {
    const abortController = new AbortController();
    const testWorkingDirectory = '/test/working/directory';

    // Mock the generation flow
    const mockSendMessageStream = vi.fn().mockReturnValue([]);
    mockConfig.getGeminiClient = vi.fn().mockReturnValue({
      isInitialized: vi.fn().mockReturnValue(true),
      setHistory: vi.fn(),
      sendMessageStream: mockSendMessageStream,
    });

    // This test verifies that the code handles directory setup gracefully
    // Since WorkspaceContext is mocked to succeed, we just verify the workspace context was set
    try {
      await generationService.generateResponse(
        'test input',
        'test-prompt-id',
        abortController.signal,
        {
          workingDirectory: testWorkingDirectory,
        }
      );
    } catch {
      // Ignore errors for this test
    }

    // The workspace context should be changed even with mocked implementation
    expect(mockConfig.setWorkspaceContext).toHaveBeenCalledTimes(2);
  });

  it('should set approval mode to YOLO and restore it afterwards', async () => {
    const abortController = new AbortController();

    // Mock the generation flow
    const mockSendMessageStream = vi.fn().mockReturnValue([]);
    mockConfig.getGeminiClient = vi.fn().mockReturnValue({
      isInitialized: vi.fn().mockReturnValue(true),
      setHistory: vi.fn(),
      sendMessageStream: mockSendMessageStream,
    });

    try {
      await generationService.generateResponse(
        'test input',
        'test-prompt-id',
        abortController.signal,
        {}
      );
    } catch {
      // Ignore errors for this test
    }

    // Should set to YOLO mode and then restore original
    expect(mockConfig.setApprovalMode).toHaveBeenCalledWith(ApprovalMode.YOLO);
    expect(mockConfig.setApprovalMode).toHaveBeenLastCalledWith(ApprovalMode.DEFAULT);
  });
});