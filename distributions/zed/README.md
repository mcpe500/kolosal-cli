# Kolosal for Zed

This directory contains the Zed editor extension for Kolosal AI.

## Structure

```
distributions/zed/
├── extension.toml    # Zed extension manifest
├── icons/
│   └── kolosal.svg   # Extension icon
├── LICENSE           # Apache 2.0 (symlink to root)
└── README.md         # This file
```

## Building ACP Binaries

The Zed extension requires ACP (Agent Control Protocol) binaries for each platform.

### Build for Current Platform

```bash
node scripts/build-zed-acp.js
```

### Build for All Platforms

```bash
node scripts/build-zed-acp.js --all
```

Output will be in `dist/zed/`:
- `kolosal-acp-darwin-aarch64-{version}.zip`
- `kolosal-acp-darwin-x86_64-{version}.zip`
- `kolosal-acp-linux-aarch64-{version}.zip`
- `kolosal-acp-linux-x86_64-{version}.zip`
- `kolosal-acp-windows-aarch64-{version}.zip`
- `kolosal-acp-windows-x86_64-{version}.zip`

## Publishing

1. Build ACP binaries for all platforms
2. Create a GitHub release with the version tag (e.g., `v0.1.3`)
3. Upload all zip files to the release
4. Update `extension.toml` with the correct version and URLs
5. Submit the extension to the [Zed Extension Gallery](https://zed.dev/extensions)

## Local Development

To test the extension locally with Zed:

1. Build the ACP binary for your platform:
   ```bash
   node scripts/build-zed-acp.js
   ```

2. Run Kolosal in ACP mode manually:
   ```bash
   kolosal --experimental-acp
   ```

## How It Works

The Kolosal ACP binary communicates with Zed using JSON-RPC over stdin/stdout. The ACP protocol enables:

- Session management
- Tool execution with permission requests
- File operations
- Streaming responses

See `packages/cli/src/zed-integration/` for implementation details.
