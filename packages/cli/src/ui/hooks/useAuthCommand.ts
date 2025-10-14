/**
 * @license
 * Copyright 2025 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import { useState, useCallback, useEffect } from 'react';
import type { LoadedSettings, SettingScope } from '../../config/settings.js';
import type { AuthType, Config } from '@kolosal-ai/kolosal-ai-core';
import {
  clearCachedCredentialFile,
  getErrorMessage,
} from '@kolosal-ai/kolosal-ai-core';
import { 
  getCurrentModelAuthType,
  type SavedModelEntry 
} from '../../config/savedModels.js';

export const useAuthCommand = (
  settings: LoadedSettings,
  setAuthError: (error: string | null) => void,
  config: Config,
) => {
  // Get authType from current model's saved entry, not from global settings
  const currentModelName = settings.merged.model?.name;
  const savedModels = settings.merged.model?.savedModels as SavedModelEntry[] | undefined;
  const currentAuthType = getCurrentModelAuthType(currentModelName, savedModels);
  
  const [isAuthDialogOpen, setIsAuthDialogOpen] = useState(
    currentAuthType === undefined,
  );

  const openAuthDialog = useCallback(() => {
    setIsAuthDialogOpen(true);
  }, []);

  const [isAuthenticating, setIsAuthenticating] = useState(false);

  useEffect(() => {
    const authFlow = async () => {
      if (isAuthDialogOpen || !currentAuthType) {
        return;
      }

      try {
        setIsAuthenticating(true);
        await config.refreshAuth(currentAuthType);
        console.log(`Authenticated via "${currentAuthType}".`);
      } catch (e) {
        setAuthError(`Failed to login. Message: ${getErrorMessage(e)}`);
        openAuthDialog();
      } finally {
        setIsAuthenticating(false);
      }
    };

    void authFlow();
  }, [isAuthDialogOpen, currentAuthType, config, setAuthError, openAuthDialog]);

  const handleAuthSelect = useCallback(
    async (authType: AuthType | undefined, _scope: SettingScope) => {
      if (authType) {
        await clearCachedCredentialFile();

        // Note: We don't save selectedAuthType anymore - authType is per-model
      }
      setIsAuthDialogOpen(false);
      setAuthError(null);
    },
    [setAuthError],
  );

  const cancelAuthentication = useCallback(() => {
    setIsAuthenticating(false);
  }, []);

  return {
    isAuthDialogOpen,
    openAuthDialog,
    handleAuthSelect,
    isAuthenticating,
    cancelAuthentication,
  };
};
