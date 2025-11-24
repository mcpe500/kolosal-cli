/**
 * @license
 * Copyright 2025 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import type React from 'react';
import { useRef, useState, useCallback } from 'react';
import { Box, Text, useInput } from 'ink';
import { Colors } from '../colors.js';
import { LeftBorderPanel } from './shared/LeftBorderPanel.js';
import { getPanelBackgroundColor } from './shared/panelStyles.js';
import { isValidUrl, testOpenAIConnection, normalizeBaseUrl } from '../../utils/validateOpenAIConfig.js';

interface OpenAIKeyPromptProps {
  onSubmit: (apiKey: string, baseUrl: string, model: string) => void;
  onCancel: () => void;
}

export function OpenAIKeyPrompt({
  onSubmit,
  onCancel,
}: OpenAIKeyPromptProps): React.JSX.Element {
  const [currentField, setCurrentField] = useState<'baseUrl' | 'model' | 'apiKey'>('baseUrl');
  const [baseUrl, setBaseUrl] = useState('');
  const [model, setModel] = useState('');
  const [apiKey, setApiKey] = useState('');
  const [baseUrlError, setBaseUrlError] = useState<string | null>(null);
  const [isTesting, setIsTesting] = useState(false);
  const [testError, setTestError] = useState<string | null>(null);
  
  const baseUrlRef = useRef(baseUrl);
  const modelRef = useRef(model);
  const apiKeyRef = useRef(apiKey);

  useInput((input, key) => {
    // Handle arrow keys for navigation
    if (key.upArrow) {
      if (currentField === 'model') {
        setCurrentField('baseUrl');
      } else if (currentField === 'apiKey') {
        setCurrentField('model');
      }
      return;
    }

    if (key.downArrow) {
      if (currentField === 'baseUrl') {
        setCurrentField('model');
      } else if (currentField === 'model') {
        setCurrentField('apiKey');
      }
      return;
    }

    // Filter paste-related control sequences
    let cleanInput = (input || '')
      .replace(/\u001b\[[0-9;]*[a-zA-Z]/g, '') // eslint-disable-line no-control-regex
      .replace(/\[200~/g, '')
      .replace(/\[201~/g, '')
      .replace(/^\[|~$/g, '');

    // Filter invisible characters (ASCII < 32, except newline)
    cleanInput = cleanInput
      .split('')
      .filter((ch) => ch.charCodeAt(0) >= 32)
      .join('');

    if (cleanInput.length > 0) {
      if (currentField === 'baseUrl') {
        baseUrlRef.current = `${baseUrlRef.current}${cleanInput}`;
        const newBaseUrl = baseUrlRef.current;
        setBaseUrl(newBaseUrl);
        if (newBaseUrl.trim().length > 0 && !isValidUrl(newBaseUrl.trim())) {
          setBaseUrlError('Invalid URL format. Please enter a valid HTTP or HTTPS URL.');
        } else {
          setBaseUrlError(null);
        }
      } else if (currentField === 'model') {
        modelRef.current = `${modelRef.current}${cleanInput}`;
        setModel(modelRef.current);
      } else if (currentField === 'apiKey') {
        apiKeyRef.current = `${apiKeyRef.current}${cleanInput}`;
        setApiKey(apiKeyRef.current);
      }
      return;
    }

    // Check if Enter key pressed
    if (input.includes('\n') || input.includes('\r')) {
      if (currentField === 'baseUrl') {
        const trimmedBaseUrl = baseUrlRef.current.trim();
        if (trimmedBaseUrl.length > 0 && !isValidUrl(trimmedBaseUrl)) {
          setBaseUrlError('Invalid URL format. Please enter a valid HTTP or HTTPS URL.');
          return;
        }
        setBaseUrlError(null);
        setCurrentField('model');
      } else if (currentField === 'model') {
        setCurrentField('apiKey');
      } else if (currentField === 'apiKey') {
        void handleSubmit();
      }
      return;
    }

    if (key.escape) {
      onCancel();
      return;
    }

    // Handle backspace
    if (key.backspace || key.delete) {
      if (currentField === 'baseUrl') {
        baseUrlRef.current = baseUrlRef.current.slice(0, -1);
        const newBaseUrl = baseUrlRef.current;
        setBaseUrl(newBaseUrl);
        if (newBaseUrl.trim().length > 0 && !isValidUrl(newBaseUrl.trim())) {
          setBaseUrlError('Invalid URL format. Please enter a valid HTTP or HTTPS URL.');
        } else {
          setBaseUrlError(null);
        }
      } else if (currentField === 'model') {
        modelRef.current = modelRef.current.slice(0, -1);
        setModel(modelRef.current);
      } else if (currentField === 'apiKey') {
        apiKeyRef.current = apiKeyRef.current.slice(0, -1);
        setApiKey(apiKeyRef.current);
      }
      return;
    }
  });

  const handleSubmit = useCallback(async () => {
    const trimmedApiKey = apiKeyRef.current.trim();
    const trimmedBaseUrl = baseUrlRef.current.trim();
    const trimmedModel = modelRef.current.trim();

    if (!trimmedApiKey || !trimmedBaseUrl || !trimmedModel) {
      setTestError('All fields are required.');
      return;
    }

    if (!isValidUrl(trimmedBaseUrl)) {
      setTestError('Invalid base URL format. Please enter a valid HTTP or HTTPS URL.');
      setBaseUrlError('Invalid URL format. Please enter a valid HTTP or HTTPS URL.');
      return;
    }

    // Normalize the base URL (remove endpoint paths)
    const normalizedBaseUrl = normalizeBaseUrl(trimmedBaseUrl);
    if (normalizedBaseUrl !== trimmedBaseUrl) {
      // Update the displayed URL to show the normalized version
      baseUrlRef.current = normalizedBaseUrl;
      setBaseUrl(normalizedBaseUrl);
    }

    setIsTesting(true);
    setTestError(null);

    try {
      const result = await testOpenAIConnection(trimmedBaseUrl, trimmedApiKey, trimmedModel);
      const finalBaseUrl = result.normalizedUrl || normalizedBaseUrl;
      
      if (result.success) {
        setIsTesting(false);
        // Submit with normalized URL
        onSubmit(trimmedApiKey, finalBaseUrl, trimmedModel);
      } else {
        setIsTesting(false);
        setTestError(result.error || 'API test failed. Please check your credentials.');
        // Update displayed URL if it was normalized
        if (result.normalizedUrl && result.normalizedUrl !== trimmedBaseUrl) {
          baseUrlRef.current = result.normalizedUrl;
          setBaseUrl(result.normalizedUrl);
        }
      }
    } catch (error) {
      setIsTesting(false);
      setTestError(error instanceof Error ? error.message : 'Failed to test API connection.');
    }
  }, [onSubmit]);

  return (
    <LeftBorderPanel
      accentColor="yellow"
      backgroundColor={getPanelBackgroundColor()}
      width="100%"
      marginLeft={1}
      marginTop={1}
      marginBottom={1}
      contentProps={{
        flexDirection: 'column',
        padding: 1,
      }}
    >
      <Text bold color={Colors.AccentBlue}>
        OpenAI Compatible API Configuration
      </Text>
      <Box marginTop={1}>
        <Text>
          Please enter your OpenAI-compatible API configuration to continue.
        </Text>
      </Box>
      <Box marginTop={1} flexDirection="row">
        <Box width={12}>
          <Text color={currentField === 'baseUrl' ? Colors.AccentBlue : Colors.Gray}>
            Base URL:
          </Text>
        </Box>
        <Box flexGrow={1}>
          <Text color={currentField === 'baseUrl' ? 'white' : Colors.Gray}>
            {currentField === 'baseUrl' ? `> ${baseUrl || ' '}` : baseUrl || '(not set)'}
          </Text>
        </Box>
      </Box>
      {baseUrlError && (
        <Box marginTop={1} marginLeft={13}>
          <Text color={Colors.AccentRed}>{baseUrlError}</Text>
        </Box>
      )}
      <Box marginTop={1} flexDirection="row">
        <Box width={12}>
          <Text color={currentField === 'model' ? Colors.AccentBlue : Colors.Gray}>
            Model:
          </Text>
        </Box>
        <Box flexGrow={1}>
          <Text color={currentField === 'model' ? 'white' : Colors.Gray}>
            {currentField === 'model' ? `> ${model || ' '}` : model || '(not set)'}
          </Text>
        </Box>
      </Box>
      <Box marginTop={1} flexDirection="row">
        <Box width={12}>
          <Text color={currentField === 'apiKey' ? Colors.AccentBlue : Colors.Gray}>
            API Key:
          </Text>
        </Box>
        <Box flexGrow={1}>
          <Text color={currentField === 'apiKey' ? 'white' : Colors.Gray}>
            {currentField === 'apiKey' ? `> ${apiKey || ' '}` : apiKey || '(not set)'}
          </Text>
        </Box>
      </Box>
      {isTesting && (
        <Box marginTop={1}>
          <Text color={Colors.AccentYellow}>Testing API connection...</Text>
        </Box>
      )}
      {testError && (
        <Box marginTop={1}>
          <Text color={Colors.AccentRed}>{testError}</Text>
        </Box>
      )}
      <Box marginTop={1}>
        <Text color={Colors.Gray}>
          Use ↑/↓ to navigate, Enter to continue, Esc to cancel
        </Text>
      </Box>
    </LeftBorderPanel>
  );
}
