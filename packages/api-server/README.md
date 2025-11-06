# @kolosal-ai/api-server

A standalone HTTP API server for Kolosal AI generation services.

## Installation

```bash
npm install @kolosal-ai/api-server
```

## Usage

### Basic Usage

```typescript
import { startApiServer } from '@kolosal-ai/api-server';
import { Config } from '@kolosal-ai/kolosal-ai-core';

const config = new Config(/* your config options */);

const server = await startApiServer(config, {
  port: 8080,
  host: '127.0.0.1',
  enableCors: true
});

console.log(`Server running on http://127.0.0.1:${server.port}`);

// Later, to close the server
await server.close();
```

### API Endpoints

#### Health Check
- **GET** `/healthz` - Returns server health status

#### Status
- **GET** `/status` - Returns server status and configuration info

#### Generate
- **POST** `/v1/generate` - Generate AI responses

**Request Body:**
```json
{
  "input": "Your prompt here",
  "stream": false,
  "prompt_id": "optional-prompt-id",
  "history": [],
  "model": "optional-custom-model",
  "api_key": "optional-custom-api-key", 
  "base_url": "optional-custom-base-url",
  "working_directory": "/optional/working/directory"
}
```

**Response:**
```json
{
  "output": "AI generated response",
  "prompt_id": "prompt-id",
  "messages": [],
  "history": []
}
```

### Advanced Usage

You can also use the lower-level components for custom setups:

```typescript
import { ApiServerFactory, Router } from '@kolosal-ai/api-server';

// Create a custom server with additional routes
const config = /* your config */;
const router = new Router();

// Add custom routes
router.addRoute('GET', '/custom', customHandler);

// Create server with custom router
const server = await ApiServerFactory.create(config, options);
```

## Features

- **File Reference Support**: Use `@filename` syntax in prompts to include file contents
- **Streaming Support**: Real-time response streaming
- **CORS Support**: Configurable cross-origin resource sharing
- **Custom Models**: Support for custom model configurations
- **Working Directory**: Set custom working directories for file operations (automatically created if it doesn't exist)
- **History Management**: Conversation history support

## Dependencies

This package requires:
- `@kolosal-ai/kolosal-ai-core` - Core Kolosal AI functionality
- `@google/genai` - Google Generative AI client

## License

Apache-2.0