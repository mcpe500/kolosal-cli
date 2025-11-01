## Kolosal CLI API Server

This document describes the HTTP API exposed by the Kolosal CLI’s built‑in API server. It’s a lightweight local server intended for embedding Kolosal in other tools, scripts, or UIs.

The implementation lives in `packages/cli/src/api/server.ts`.

### Summary

- Host: defaults to 127.0.0.1
- Port: configurable by the CLI/launcher
- CORS: enabled by default (Access-Control-Allow-Origin: *)
- Streaming: Server-Sent Events (SSE) for token streaming and tool-call events
- Conversation history: Optional, using Google GenAI `Content[]` shape

## CORS

When CORS is enabled (default), the server sets:

- `Access-Control-Allow-Origin: *`
- `Access-Control-Allow-Methods: GET,POST,OPTIONS`
- `Access-Control-Allow-Headers: Content-Type, Authorization`

The server responds to `OPTIONS` preflight with `204 No Content`.

---

## Endpoints

### GET /healthz

Health check.

Response example:

```json
{
  "status": "ok",
  "timestamp": "2025-01-01T00:00:00.000Z",
  "mode": "server"
}
```

### GET /status

Status and capability check.

Response example:

```json
{
  "status": "ready",
  "timestamp": "2025-01-01T00:00:00.000Z",
  "version": "1.0.0",
  "mode": "server-only",
  "endpoints": {
    "generate": "/v1/generate",
    "health": "/healthz",
    "status": "/status"
  },
  "features": {
    "streaming": true,
    "conversationHistory": true,
    "toolExecution": true
  }
}
```

Notes:
- The `version` field is informational and may be updated by the CLI.

### POST /v1/generate

Generates model output for a single prompt, optionally streaming tokens and tool-call events. 

Request body:

```json
{
  "input": "Your prompt text…",               // required
  "stream": true,                             // optional (default: false)
  "prompt_id": "client-generated-id",         // optional; echoed back; auto-generated if missing
  "history": [                                // optional; Google GenAI Content[]
    { "role": "user",  "parts": [{ "text": "Hello" }] },
    { "role": "model", "parts": [{ "text": "Hi!" }] }
  ]
}
```

The `history` field follows the Google GenAI `Content[]` schema:

- `role`: `"user" | "model"`
- `parts`: array of `Part` objects (e.g., `{ "text": "…" }`).

#### Non-streaming response (stream=false)

```json
{
  "output": "final concatenated text",
  "prompt_id": "client-generated-id-or-random",
  "messages": [
    // transcript items emitted during the run (assistant/tool events)
  ],
  "history": [
    // updated conversation history in Google GenAI Content[] shape
  ]
}
```

`messages` is an array of transcript items, each in one of these shapes:

```ts
type TranscriptItem =
  | { type: 'assistant';  content: string }
  | { type: 'tool_call';  name: string; arguments?: unknown }
  | { type: 'tool_result'; name: string; ok: boolean; responseText?: string; error?: string };
```

`history` contains the prior `history` plus the new turn(s) (assistant text and tool responses as applicable).

#### Streaming response (stream=true)

Content is streamed via Server‑Sent Events (SSE). The connection is `Content-Type: text/event-stream` and remains open until a `done` event is sent or an error occurs.

Events:

- `content`: a chunk of assistant text (string). Multiple `content` events are emitted as tokens arrive.
- `assistant`: final assistant text for the turn, as a JSON TranscriptItem `{ type: 'assistant', content: string }`.
- `tool_call`: a JSON TranscriptItem `{ type: 'tool_call', name, arguments? }` emitted when a tool call is requested.
- `tool_result`: a JSON TranscriptItem `{ type: 'tool_result', name, ok, responseText?, error? }` after tool execution.
- `history`: the full updated conversation history as JSON (`Content[]`). Sent once the turn completes.
- `done`: the string `"true"` signaling the end of the stream.
- `error`: a JSON object `{ message: string }` if an error occurs.

Notes:
- Newlines in `data:` lines are escaped as `\n` per the SSE writer in the server.
- When streaming, you’ll receive `content` events for incremental text; the final `assistant` event contains the same content for the turn. If you’re already using `content` events, you may ignore the `assistant` event to avoid duplication.

---

## Tool Calls and Approval Mode

During a `/v1/generate` request, the server sets approval mode to YOLO (auto‑approve tool calls) so that tool execution can proceed without user prompts. The original approval mode is restored after the request finishes.

For each tool request emitted by the model, the server:

1. Streams a `tool_call` event (streaming mode) and records it in `messages` (non‑streaming mode).
2. Executes the tool via Kolosal’s tool runtime.
3. Streams a `tool_result` event (streaming mode) and records it (non‑streaming mode), including `ok`, `responseText` (if available), or `error` on failure.
4. Sends the tool responses back to the model as the next input turn when additional reasoning is required.

---

## Error Handling

- `400 Bad Request` — invalid JSON body or missing fields (e.g., `input`).
- `404 Not Found` — unrecognized path.
- `500 Internal Server Error` — unexpected exceptions.

In streaming mode, errors are emitted as an `error` SSE event with `{ "message": string }`, followed by connection closure.

---

## Examples

### Health check

```sh
curl -sS http://127.0.0.1:8787/healthz | jq .
```

### Status

```sh
curl -sS http://127.0.0.1:8787/status | jq .
```

### Non‑streaming generation

```sh
curl -sS -H "Content-Type: application/json" \
  -X POST http://127.0.0.1:8787/v1/generate \
  -d '{
    "input": "Summarize the benefits of local AI.",
    "stream": false
  }' | jq .
```

### Streaming generation (SSE)

```sh
curl -N -sS -H "Content-Type: application/json" \
  -X POST http://127.0.0.1:8787/v1/generate \
  -d '{
    "input": "List three CLI productivity tips.",
    "stream": true
  }'
```

Typical output (abridged):

```
event: content
data: Tip 1: …

event: content
data: Tip 2: …

event: assistant
data: {"type":"assistant","content":"Tip 1…\nTip 2…\nTip 3…"}

event: history
data: [{"role":"user","parts":[{"text":"List three CLI productivity tips."}]},{"role":"model","parts":[{"text":"Tip 1…\nTip 2…\nTip 3…"}]}]

event: done
data: true
```

### Passing conversation history

```sh
curl -sS -H "Content-Type: application/json" \
  -X POST http://127.0.0.1:8787/v1/generate \
  -d '{
    "input": "And add one more.",
    "stream": false,
    "history": [
      { "role": "user",  "parts": [{ "text": "List three CLI productivity tips." }] },
      { "role": "model", "parts": [{ "text": "Tip 1…\nTip 2…\nTip 3…" }] }
    ]
  }' | jq .
```

---

## Programmatic use

If you need to embed the server in your own Node.js process, you can import and start it programmatically:

```ts
import { startApiServer } from "@kolosal-ai/kolosal-cli"; // path depends on your setup
import { createConfig } from "@kolosal-ai/kolosal-ai-core"; // hypothetical

const config = createConfig(/* … */);

await startApiServer(config, {
  port: 8787,
  host: "127.0.0.1",      // default
  enableCors: true,       // default
});
```

Options:

- `port: number` — required
- `host?: string` — default `"127.0.0.1"`
- `enableCors?: boolean` — default `true`

---

## Compatibility and notes

- This API uses Google GenAI `Content[]`/`Part` shapes for history and internal message passing. If you’re not using Google’s types directly, treat them as the minimal schema shown above.
- SSE data lines escape newlines (`\n`) to keep each event on a single data line.
- Tool execution is performed within the Kolosal tool runtime; see the CLI and core package docs for configuring available tools and permissions.
