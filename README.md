# Kolosal CLI

<div align="center">

[![npm version](https://img.shields.io/npm/v/@kolosal-ai/kolosal-ai.svg)](https://www.npmjs.com/package/@kolosal-ai/kolosal-ai)
[![License](https://img.shields.io/badge/license-Apache_2.0-blue.svg)](./LICENSE)
[![Node.js Version](https://img.shields.io/badge/node-%3E%3D20.0.0-brightgreen.svg)](https://nodejs.org/)

**AI-powered command-line workflow tool for developers**

[Installation](#installation) • [Quick Start](#quick-start) • [Features](#features) • [Documentation](./docs/) • [Contributing](./CONTRIBUTING.md)

</div>

Kolosal CLI is a powerful command-line AI workflow tool that enhances your development workflow with advanced code understanding, automated tasks, and intelligent assistance. Based on Qwen Code models, it provides seamless integration with your existing development environment.

## Features

- **Code Understanding & Editing** - Query and edit large codebases beyond traditional context window limits
- **Workflow Automation** - Automate operational tasks like handling pull requests and complex rebases
- **Enhanced Parser** - Optimized specifically for code-oriented models
- **Vision Model Support** - Automatic detection and multimodal analysis of images in your input
- **Flexible Model Support** - Works with OpenAI-compatible APIs and Hugging Face models

## Installation

### Prerequisites

Ensure you have [Node.js version 20](https://nodejs.org/en/download) or higher installed.

### Install from npm

```bash
npm install -g @kolosal-ai/kolosal-ai@latest
kolosal --version
```

### Install from source

```bash
git clone <your repo url>
cd kolosal-code
npm install
npm install -g .
```

### Homebrew (Coming Soon)

```bash
brew install kolosal-ai
```

## Quick Start

```bash
# Start Kolosal CLI
kolosal

# Example commands
> Explain this codebase structure
> Help me refactor this function
> Generate unit tests for this module
```

### Authentication Options

#### 1. OpenAI Compatible API

Set environment variables or create a `.env` file:

```bash
export OPENAI_API_KEY="your_api_key_here"
export OPENAI_BASE_URL="your_api_endpoint" 
export OPENAI_MODEL="your_model_choice"
```

#### 2. Hugging Face LLM Models

Configure Hugging Face models for local or API-based inference:

```bash
export HF_TOKEN="your_huggingface_token"
export HF_MODEL="your_model_name"
```

> **Note**: Kolosal CLI may issue multiple API calls per cycle, which can result in higher token usage. We're actively optimizing API efficiency.

### Configuration

#### Session Management

Configure token limits in `.kolosal/settings.json`:

```json
{
  "sessionTokenLimit": 32000
}
```

#### Vision Models

Kolosal CLI automatically detects images and switches to vision-capable models. Configure the behavior:

```json
{
  "experimental": {
    "vlmSwitchMode": "once",  // "once", "session", "persist"
    "visionModelPreview": true
  }
}
```

## Usage Examples

### Code Analysis & Understanding
```bash
> Explain this codebase architecture
> What are the key dependencies and how do they interact?
> Find all API endpoints and their authentication methods
> Generate a dependency graph for this module
```

### Code Development & Refactoring
```bash
> Refactor this function to improve readability and performance
> Convert this class to use dependency injection
> Generate unit tests for the authentication module
> Add error handling to all database operations
```

### Workflow Automation
```bash
> Analyze git commits from the last 7 days, grouped by feature
> Create a changelog from recent commits
> Find and remove all console.log statements
> Check for potential SQL injection vulnerabilities
```

## Commands & Shortcuts

### Session Commands
- `/help` - Display available commands
- `/clear` - Clear conversation history  
- `/compress` - Compress history to save tokens
- `/stats` - Show current session information
- `/exit` or `/quit` - Exit Kolosal CLI

### Keyboard Shortcuts
- `Ctrl+C` - Cancel current operation
- `Ctrl+D` - Exit (on empty line)
- `Up/Down` - Navigate command history

## Development & Contributing

See [CONTRIBUTING.md](./CONTRIBUTING.md) to learn how to contribute to the project.

## Distribution

### Windows Package

Build a single executable and installer:

```bash
npm run bundle               # builds bundle/gemini.js
npm run build:win:exe        # creates dist/win/kolosal.exe
iscc installer/kolosal.iss   # creates installer
```

### macOS Package

Build a standalone `.pkg` installer with bundled Node.js:

```bash
npm run build:mac:pkg
```

Creates `dist/mac/KolosalCode-macos-signed.pkg` - a self-contained package that requires no Node.js installation.

## Acknowledgments

Kolosal CLI is built upon and adapted from the Qwen Code project, which is licensed under the Apache License, Version 2.0. We acknowledge and appreciate the excellent work of the Qwen development team.

This project also incorporates concepts and approaches from [Google Gemini CLI](https://github.com/google-gemini/gemini-cli). We appreciate the foundational work that made this project possible.

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](./LICENSE) for the full license text.

This product includes software developed from the Qwen Code project, which is also licensed under the Apache License, Version 2.0.


