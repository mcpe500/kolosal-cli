# Unified Model Selection Interface

## Overview

The Kolosal CLI now features a unified model selection interface that makes it easier to browse and select models from multiple sources including Hugging Face and Ollama. This interface provides a consistent experience across all model sources with clear visual indicators to distinguish between them.

## Features

### Visual Source Indicators
- **[HF]** - Hugging Face models
- **[OL]** - Ollama models
- **[LOCAL]** - Local models from config
- **[URL]** - Direct GGUF file URLs

### Unified Interface
The new interface groups models by source with clear section headers:
```
=== Hugging Face Models ===
[HF] TheBloke/Mistral-7B-v0.1-GGUF 7B (4.1GB) [Q4_K_M]
[HF] TheBloke/LLaMA3-8B-GGUF 8B (4.9GB) [Q8_0]

=== Ollama Models ===
[OL] llama3:8b 8B (2.3GB)
[OL] mistral:7b 7B (4.1GB)

=== Local Config Models ===
[LOCAL] my-custom-model
```

### Easy Navigation
- Use arrow keys to navigate through the model list
- Press Enter to select a model
- Press Esc to cancel and return to the main menu

## Usage

### Running the Unified Interface
Simply run the Kolosal CLI without any arguments:
```bash
./kolosal
```

This will automatically show the unified model selection interface with models from all available sources.

### Source Status Indicators
- If Ollama is running, you'll see your local Ollama models
- If Ollama is not running, you'll see a message indicating that Ollama needs to be started
- Hugging Face models are fetched directly from the Hugging Face API

## Model Selection Workflow

1. **Browse Models**: The interface displays all available models grouped by source
2. **Select Model**: Navigate to your desired model and press Enter
3. **Model Processing**: 
   - For Hugging Face models, you'll be prompted to select a specific quantization
   - For Ollama models, the model is used directly
   - For local models, the existing configuration is used

## Requirements

### Ollama Integration
To use Ollama models:
1. Install and start Ollama: https://ollama.com/
2. Pull any models you want to use: `ollama pull llama3`
3. Run Kolosal CLI and the Ollama models will automatically appear in the interface

### Hugging Face Integration
Hugging Face models are available by default through the Hugging Face API.

## Benefits

### Improved User Experience
- No need to manually distinguish between model sources
- Clear visual indicators for each model type
- Consistent information display across all sources
- Single interface for all model selection needs

### Better Organization
- Models grouped by source for easy scanning
- Clear section headers separate different model types
- Visual tags make it easy to identify model sources at a glance

### Enhanced Discoverability
- All available models shown in one place
- Easy to compare models from different sources
- Parameter counts and file sizes displayed consistently