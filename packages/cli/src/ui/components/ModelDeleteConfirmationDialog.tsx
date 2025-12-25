/**
 * @license
 * Copyright 2025 Kolosal
 * SPDX-License-Identifier: Apache-2.0
 */

import { Box, Text } from 'ink';
import type React from 'react';
import { Colors } from '../colors.js';
import {
  RadioButtonSelect,
  type RadioSelectItem,
} from './shared/RadioButtonSelect.js';
import { useKeypress } from '../hooks/useKeypress.js';
import { LeftBorderPanel } from './shared/LeftBorderPanel.js';
import { getPanelBackgroundColor } from './shared/panelStyles.js';
import type { AvailableModel } from '../models/availableModels.js';

export enum ModelDeleteChoice {
  CANCEL = 'cancel',
  DELETE = 'delete',
}

export interface ModelDeleteConfirmationDialogProps {
  model: AvailableModel;
  onSelect: (choice: ModelDeleteChoice) => void;
}

export const ModelDeleteConfirmationDialog: React.FC<
  ModelDeleteConfirmationDialogProps
> = ({ model, onSelect }) => {
  useKeypress(
    (key) => {
      if (key.name === 'escape') {
        onSelect(ModelDeleteChoice.CANCEL);
      }
    },
    { isActive: true },
  );

  const modelLabel = model.label ?? model.id;
  const options: Array<RadioSelectItem<ModelDeleteChoice>> = [
    {
      label: 'Yes, delete',
      value: ModelDeleteChoice.DELETE,
    },
    {
      label: 'Cancel (Esc)',
      value: ModelDeleteChoice.CANCEL,
    },
  ];

  return (
    <LeftBorderPanel
      accentColor={Colors.AccentRed}
      backgroundColor={getPanelBackgroundColor()}
      marginTop={1}
      marginBottom={1}
      marginLeft={1}
      contentProps={{
        flexDirection: 'column',
        padding: 1,
        width: '100%',
      }}
    >
      <Box flexDirection="column" marginBottom={1}>
        <Text bold>Delete Model</Text>
        <Text>
          Are you sure you want to delete the model "{modelLabel}"?
        </Text>
        <Box marginTop={1}>
          <Text color={Colors.Gray}>
            This action cannot be undone.
          </Text>
        </Box>
      </Box>

      <RadioButtonSelect items={options} onSelect={onSelect} isFocused />
    </LeftBorderPanel>
  );
};

