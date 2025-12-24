/**
 * @license
 * Copyright 2025 Kolosal
 * SPDX-License-Identifier: Apache-2.0
 */

import { describe, expect, it, vi, beforeEach } from 'vitest';
import { modelDeleteCommand } from './modelDeleteCommand.js';
import { AuthType } from '@kolosal-ai/kolosal-ai-core';
import type { CommandContext } from './types.js';
import type { SavedModelEntry } from '../../config/savedModels.js';
import { createMockCommandContext } from '../../test-utils/mockCommandContext.js';

describe('modelDeleteCommand', () => {
  let mockContext: CommandContext;
  const mockGetModel = vi.fn(() => 'current-model');

  beforeEach(() => {
    mockContext = createMockCommandContext({
      services: {
        config: {
          getModel: mockGetModel,
          getContentGeneratorConfig: vi.fn(() => ({
            authType: AuthType.USE_OPENAI,
            model: 'current-model',
          })),
        } as any,
        settings: {
          merged: {
            model: {
              savedModels: [] as SavedModelEntry[],
            },
          },
        } as any,
      },
    });
  });

  it('has correct name and description', () => {
    expect(modelDeleteCommand.name).toBe('model-delete');
    expect(modelDeleteCommand.altNames).toContain('delete-model');
    expect(modelDeleteCommand.description).toBe('Delete a saved custom model');
  });

  it('returns error when config is not available', async () => {
    const contextWithoutConfig = {
      ...mockContext,
      services: {
        ...mockContext.services,
        config: null,
      },
    };

    const result = await modelDeleteCommand.action!(contextWithoutConfig, '');

    expect(result).toEqual({
      type: 'message',
      messageType: 'error',
      content: 'Configuration not available.',
    });
  });

  it('returns error when no saved models exist', async () => {
    const result = await modelDeleteCommand.action!(mockContext, '');

    expect(result).toEqual({
      type: 'message',
      messageType: 'error',
      content: 'No saved models found. Nothing to delete.',
    });
  });

  it('returns error when only current model exists', async () => {
    if (mockContext.services.settings) {
      mockContext.services.settings.merged.model = {
        savedModels: [
          {
            id: 'current-model',
            provider: 'openai-compatible',
          },
        ] as SavedModelEntry[],
      };
    }

    const result = await modelDeleteCommand.action!(mockContext, '');

    expect(result).toEqual({
      type: 'message',
      messageType: 'error',
      content:
        'No deletable models found. The currently active model cannot be deleted. Switch to a different model first.',
    });
  });

  it('opens model_delete dialog when deletable models exist', async () => {
    if (mockContext.services.settings) {
      mockContext.services.settings.merged.model = {
        savedModels: [
          {
            id: 'current-model',
            provider: 'openai-compatible',
          },
          {
            id: 'deletable-model',
            provider: 'openai-compatible',
          },
        ] as SavedModelEntry[],
      };
    }

    const result = await modelDeleteCommand.action!(mockContext, '');

    expect(result).toEqual({
      type: 'dialog',
      dialog: 'model_delete',
    });
  });

  it('handles models with runtimeModelId', async () => {
    mockGetModel.mockReturnValue('runtime-model-id');
    if (mockContext.services.settings) {
      mockContext.services.settings.merged.model = {
        savedModels: [
          {
            id: 'saved-model-id',
            provider: 'openai-compatible',
            runtimeModelId: 'runtime-model-id',
          },
          {
            id: 'deletable-model',
            provider: 'openai-compatible',
          },
        ] as SavedModelEntry[],
      };
    }

    const result = await modelDeleteCommand.action!(mockContext, '');

    expect(result).toEqual({
      type: 'dialog',
      dialog: 'model_delete',
    });
  });
});

