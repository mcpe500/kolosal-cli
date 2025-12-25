/**
 * @license
 * Copyright 2025 Kolosal
 * SPDX-License-Identifier: Apache-2.0
 */

import OpenAI from 'openai';

/**
 * Normalizes a base URL by removing common endpoint paths
 * OpenAI client automatically appends /chat/completions, so we should strip it
 */
export function normalizeBaseUrl(urlString: string): string {
  if (!urlString || urlString.trim().length === 0) {
    return urlString;
  }

  let normalized = urlString.trim();
  
  // Remove trailing slashes
  normalized = normalized.replace(/\/+$/, '');
  
  // Remove common endpoint paths that the client will add automatically
  const endpointsToRemove = [
    '/chat/completions',
    '/v1/chat/completions',
    '/api/v1/chat/completions',
    '/completions',
    '/v1/completions',
  ];
  
  for (const endpoint of endpointsToRemove) {
    if (normalized.endsWith(endpoint)) {
      normalized = normalized.slice(0, -endpoint.length);
      break;
    }
  }
  
  return normalized;
}

/**
 * Validates if a string is a valid URL format
 */
export function isValidUrl(urlString: string): boolean {
  if (!urlString || urlString.trim().length === 0) {
    return false;
  }

  try {
    const url = new URL(urlString);
    return url.protocol === 'http:' || url.protocol === 'https:';
  } catch {
    return false;
  }
}

/**
 * Tests an OpenAI-compatible API connection
 */
export async function testOpenAIConnection(
  baseUrl: string,
  apiKey: string,
  model: string,
  timeout: number = 10000,
): Promise<{ success: boolean; error?: string; normalizedUrl?: string }> {
  // Normalize the base URL (remove endpoint paths)
  const normalizedBaseUrl = normalizeBaseUrl(baseUrl);
  
  try {
    const client = new OpenAI({
      apiKey,
      baseURL: normalizedBaseUrl,
      timeout,
      maxRetries: 0,
    });

    // Try models.list() first (lighter)
    try {
      await Promise.race([
        client.models.list(),
        new Promise((_, reject) =>
          setTimeout(() => reject(new Error('Request timeout')), timeout),
        ),
      ]);
      return { success: true, normalizedUrl: normalizedBaseUrl };
    } catch (_listError) {
      // Fallback to minimal chat completion
      try {
        await Promise.race([
          client.chat.completions.create({
            model,
            messages: [{ role: 'user', content: 'test' }],
            max_tokens: 1,
          }),
          new Promise((_, reject) =>
            setTimeout(() => reject(new Error('Request timeout')), timeout),
          ),
        ]);
        return { success: true, normalizedUrl: normalizedBaseUrl };
      } catch (completionError) {
        const error =
          completionError instanceof Error
            ? completionError.message
            : 'Unknown error';
        if (error.includes('401') || error.includes('Unauthorized')) {
          return {
            success: false,
            error: 'Invalid API key. Please check your API key and try again.',
            normalizedUrl: normalizedBaseUrl,
          };
        }
        if (error.includes('404') || error.includes('Not Found')) {
          // Check if the original URL had endpoint paths that might cause issues
          if (baseUrl !== normalizedBaseUrl) {
            return {
              success: false,
              error: `Model not found. The base URL was normalized from "${baseUrl}" to "${normalizedBaseUrl}". Please check the model name and try again.`,
              normalizedUrl: normalizedBaseUrl,
            };
          }
          return {
            success: false,
            error: 'Model not found. Please check the model name and try again.',
            normalizedUrl: normalizedBaseUrl,
          };
        }
        if (error.includes('timeout') || error.includes('Timeout')) {
          return {
            success: false,
            error: 'Connection timeout. Please check your base URL and network connection.',
          };
        }
        return {
          success: false,
          error: `API test failed: ${error}`,
        };
      }
    }
  } catch (error) {
    const errorMessage =
      error instanceof Error ? error.message : 'Unknown error';
    if (errorMessage.includes('Invalid URL')) {
      return {
        success: false,
        error: 'Invalid base URL format. Please enter a valid HTTP or HTTPS URL.',
      };
    }
    return {
      success: false,
      error: `Connection failed: ${errorMessage}`,
    };
  }
}
