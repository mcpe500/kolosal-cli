# Ollama Integration Documentation

## Overview

This document explains how to use the Ollama integration in Kolosal CLI. The integration allows you to browse, select, and use models from your local Ollama installation directly within the Kolosal interface.

## Prerequisites

1. **Ollama Installation**: You must have Ollama installed and running on your system.
   - Download Ollama from: https://ollama.com/
   - Follow the installation instructions for your operating system.

2. **Models**: You need to have at least one model pulled in Ollama.
   - Pull a model using: `ollama pull llama3:8b`
   - List available models: `ollama list`

## Usage

### 1. Starting Kolosal CLI

Launch the Kolosal CLI as usual:

```bash
./kolosal
```

### 2. Selecting Ollama Models

When the main menu appears, you'll see several options:

1. Hugging Face Models (default)
2. Ollama Models (local)
3. Direct GGUF File URL
4. Local GGUF File
5. Exit

Select option **2** to browse Ollama models.

### 3. Browsing Ollama Models

The interface will show all models currently available in your local Ollama installation, including their sizes.

### 4. Using an Ollama Model

Select a model from the list. Kolosal will:
1. Register the model with the Kolosal server
2. Start the chat interface with the selected model

### 5. Chatting with Ollama Models

Once selected, you can chat with the Ollama model just like any other model in Kolosal. The model will be executed using Ollama's inference engine.

## Technical Details

### How It Works

1. **Model Discovery**: Kolosal queries the Ollama API (`http://localhost:11434/api/tags`) to list local models.
2. **Model Registration**: When you select a model, Kolosal registers it with the server using a special URL scheme (`ollama://model-name`).
3. **Inference**: The Kolosal server communicates with Ollama to perform inference on the selected model.

### API Endpoints Used

- `GET /api/tags` - List local models
- The Kolosal server handles the actual chat completion requests to Ollama.

### Model File Representation

Ollama models are represented in Kolosal as special `ModelFile` objects with:
- `downloadUrl`: `ollama://model-name` (special scheme indicating Ollama model)
- `filename`: `model-name.gguf`
- `modelId`: `ollama/model-name`

## Troubleshooting

### Ollama Server Not Detected

**Problem**: "Ollama server is not running" message appears.

**Solution**: 
1. Ensure Ollama is installed and running
2. Check if the Ollama service is active: `ollama list`
3. Verify the API endpoint is accessible: `curl http://localhost:11434/api/tags`

### No Models Found

**Problem**: "No Ollama models found locally" message appears.

**Solution**:
1. Pull a model: `ollama pull llama3:8b`
2. Verify models exist: `ollama list`

### Model Not Responding

**Problem**: Chat interface doesn't respond or returns errors.

**Solution**:
1. Check Ollama logs: `ollama logs`
2. Restart Ollama service
3. Ensure sufficient system resources (RAM/CPU) for the model

## Limitations

1. **No Model Pulling**: Kolosal does not currently support pulling new models from within the interface. You must use `ollama pull` command first.
2. **Local Only**: Only local Ollama models are supported, not remote registry browsing.
3. **No Model Management**: You cannot delete or manage Ollama models from within Kolosal.

## Future Enhancements

Planned improvements include:
- Direct model pulling from Ollama registry
- Model search functionality
- Enhanced model information display
- Better error handling and user feedback

## Support

For issues with the Ollama integration, please:
1. Check that Ollama is properly installed and running
2. Verify your models are correctly pulled
3. Review the Kolosal server logs for detailed error information
4. Report issues to the Kolosal GitHub repository