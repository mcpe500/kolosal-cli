# Kolosal Cli Go + Bubble Tea Reimplementation Plan

_Last updated: 2025-10-08_

## Objectives & Success Criteria
- Deliver a first-class terminal experience that mirrors current Kolosal Cli features while feeling native to Go and Bubble Tea.
- Preserve multi-provider AI capabilities (OpenAI-compatible, Hugging Face, local inference) with clear extension points.
- Maintain strong ergonomics for large-repo navigation, code editing assistance, and workflow automation.
- Achieve fast startup (<200ms cold start) and responsive interactions (<100ms UI updates for local actions).
- Ship with automated validation: unit, integration, snapshot, and performance benchmarks.

## Scope & Assumptions
- Target Go 1.23+ with module support and generics.
- Focus on macOS and Linux for the first milestone; Windows support follows once pseudo-terminal abstractions are validated.
- Bubble Tea paired with Lip Gloss, Bubbles, and docopt-style CLI parsing ensures a rich TUI.
- MCP (Model Context Protocol) compatibility remains a core requirement; Kolosal Server integration is out-of-process over gRPC/WebSocket.
- Existing Kolosal features—code search, intelligent editing, workflow automation—must be feature-parity or better; packaging and installers can trail behind the CLI MVP.

## Existing Capabilities Snapshot
- AI-powered command-line tool built atop TypeScript + Ink with robust prompts for code understanding and editing.
- Multi-provider AI abstraction supporting OpenAI-compatible APIs, Hugging Face models, and OAuth flows.
- Advanced parsing, session memory compression, and vision model routing.
- Workflow automation for git, changelog generation, refactoring, testing, and terminal control.
- Distribution scripts for macOS/Windows installers, telemetry, and sandbox execution support.

## High-Level Architecture

### System Overview
```
┌──────────┐    bubbletea msgs     ┌─────────────┐      typed requests      ┌────────────┐
│   User   │ ─────────────────────▶│   TUI App    │────────────────────────▶│  Domain     │
└──────────┘    keyboard events    │  (Model/View)│      async channels      │  Services   │
                                     ▲            │                         └────────────┘
                                     │            │                                │
                                     │  state diff│                                ▼
                                     │            │                         ┌────────────┐
                                     │            │◀────────────────────────│ AI Runtime │
                                     │            │  streaming responses     └────────────┘
                                     │            │                                │
                                     │ commands   │                                ▼
                                     │            │                         ┌────────────┐
                                     │            └────────────────────────▶│  Workspace │
                                     │                 file ops / git        │  Services │
                                     │                                       └────────────┘
                                     │
                                   telemetry
                                     │
                                     ▼
                               Observability
```

### Core Modules
| Module | Responsibility |
| --- | --- |
| `cmd/kolosal` | CLI entrypoint, configuration bootstrap, signals handling, profile flags. |
| `internal/tui` | Bubble Tea program with models, views, and component composition. |
| `internal/domain` | Orchestrates high-level user intents (code understanding, refactoring, automation). |
| `internal/providers` | AI provider clients, streaming response decoding, rate limiting. |
| `internal/tasks` | Workflow automation runners (git, tests, file ops) with cancellation support. |
| `internal/workspace` | File system traversal, indexing, ripgrep integration, caching. |
| `internal/config` | Loads profiles, secrets, session settings, and persists runtime choices. |
| `internal/telemetry` | Structured logging, metrics, privacy-safe traces. |
| `internal/serverlink` | Optional Kolosal Server/MCP bridge maintaining background session. |

### Bubble Tea Program Design
- **Model (`AppModel`)** tracks session state: active view, conversation history, current task, queued commands, provider status, and ephemeral notifications.
- **Messages** include user input, provider streaming tokens, task lifecycle events, file-system updates, and global hotkeys.
- **Commands** spawn goroutines to execute AI calls, run workflows, scan repositories, and publish incremental updates back to the Bubble Tea event loop.
- Views are composed with Lip Gloss layouts and Bubbles components (text inputs, list/table, status bars, progress meters).
- Global keymap uses Bubble Tea delegates to switch panes (chat, context browser, job monitor) and to trigger modal dialogs.

### Services & Background Workers
- Dedicated worker pool (`internal/runtime/worker`) handles I/O-heavy tasks (git, ripgrep, formatting) with context-aware cancellation.
- AI calls execute in isolated goroutines sharing a circuit-breaker and retry budget; streaming responses propagate through a message broker to avoid UI blocking.
- Long-running operations surface progress events over channels to update the TUI status views.

### AI Provider Abstraction
- `Provider` interface exposes `Complete`, `Chat`, `Embed`, and metadata queries with context deadlines and streaming callbacks.
- Each provider module encapsulates auth (API keys, OAuth), request shaping, response normalization, and rate limiting.
- A `Router` supports dynamic switching based on task type (vision, embedding, code) and session preferences.

### Configuration & Persistence
- Config loader resolves defaults from `$XDG_CONFIG_HOME/kolosal/config.yaml`, environment overrides, and per-project `.kolosalrc` files.
- Secrets managed via OS keychain/backing file with AES-GCM encryption; CLI prompts for missing credentials.
- Session transcripts stored in SQLite/bolt DB for resuming context; history compression performed offline via background job.

### Telemetry & Observability
- Structured log events emitted via `zap` or `slog` with redaction utilities.
- Metrics exported over OpenTelemetry OTLP when enabled; local logging disabled by default for privacy.
- Health pings capture latency, throughput, and failure counts for AI and workspace operations.

### Error Handling & Resilience
- Central `fault` package classifies errors (user, provider, system) and renders friendly TUI notifications.
- Automatic retries with exponential backoff for transient provider failures.
- Graceful degradation: if AI providers are unavailable, offline tooling (search, static analysis) remains accessible.

### Security & Privacy Considerations
- Explicit consent gates before sending code outside the workstation.
- Provider-specific data retention disclaimers shown inline; session scrubber removes PII before persistence.
- Sandboxed execution path for risky automation tasks, leveraging `kolosal-server` when available.

## Go Project Structure
```
kolosal-go/
├── cmd/
│   └── kolosal/           # CLI entrypoint (cobra/docopt hybrid)
├── internal/
│   ├── tui/               # Bubble Tea models, views, components
│   ├── components/        # Reusable UI widgets (progress, tabs, file-tree)
│   ├── domain/            # High-level use-case orchestration
│   ├── providers/         # AI provider clients and routing
│   ├── tasks/             # Automation runners (git, tests, workflows)
│   ├── workspace/         # Repo indexing, file mutation helpers
│   ├── config/            # Config loader, secrets, schema validation
│   ├── runtime/           # Worker pools, job scheduling, cancellation
│   ├── telemetry/         # Logging, metrics, tracing emitters
│   └── util/              # Shared helpers (path, prompts, error glue)
├── pkg/
│   ├── kolosalserver/     # Optional SDK for kolosal-server interactions
│   └── providers/         # Public provider adapters for downstream use
├── assets/                # Static prompts, templates, theme definitions
├── configs/               # Sample config files and schema definitions
├── scripts/               # Developer tooling (lint, fmt, release)
├── testdata/              # Golden files and fixture repositories
├── docs/                  # User and developer documentation
├── Makefile               # Developer convenience targets
├── go.mod / go.sum        # Module definition and dependencies
└── README.md              # Project overview and quick start
```

### Directory Highlights
- **`cmd/kolosal`** wires configuration, logging, and orchestrates the Bubble Tea program.
- **`internal/tui`** exposes modular views: chat, context browser, job monitor, settings. Each subcomponent keeps its own `tea.Model` for isolation.
- **`internal/domain`** maps high-level intents ("explain file", "refactor selection") into workflows that compose providers, workspace, and tasks.
- **`internal/providers`** registers OpenAI, Hugging Face, local llm drivers, and features a plugin loader for user-supplied providers.
- **`internal/workspace`** surfaces file queries (ripgrep, tree-sitter parsing), edit application, and diff generation.
- **`internal/runtime`** owns worker pools, rate limiters, and cancellation semantics shared across services.
- **`pkg/kolosalserver`** packages optional client APIs so other Go programs can integrate with Kolosal’s backend server without reusing internals.
- **`scripts/`** collects cross-platform shell scripts plus small Go utilities invoked via `go run ./scripts/...` for release automation.
- **`testdata/`** holds canonical fixtures enabling deterministic unit and integration tests.

### Packages Folder (Detailed File Map)
The Go reimplementation preserves a `packages/` workspace to host shareable Go modules mirroring the current TypeScript monorepo layout. Each subpackage compiles to an importable library with its own `go.mod` file, enabling isolated versioning and reuse.

#### `packages/cli`
| File | Responsibility |
| --- | --- |
| `main.go` | Binary entrypoint that binds Cobra commands to the Bubble Tea program and config bootstrap. |
| `root.go` | Declares CLI flags, persistent pre-run hooks, and profile selection logic. |
| `bootstrap/config.go` | Loads user config, secrets, and environment overrides before the TUI starts. |
| `bootstrap/telemetry.go` | Initializes logging, metrics exporters, and crash reporting hooks. |
| `tui/app.go` | Owns the primary `tea.Program`, routing lifecycle events across views. |
| `tui/keymap.go` | Centralized keybindings with accessibility-friendly defaults and overrides. |
| `tui/views/chat.go` | Chat pane rendering and message reconciliation for AI conversations. |
| `tui/views/context.go` | Context browser showing file tree, search results, and diff previews. |
| `tui/views/jobs.go` | Monitors background automation jobs with progress bars and logs. |
| `tui/components/statusbar.go` | Reusable status bar summarizing provider status, latency, and workspace hints. |

#### `packages/core`
| File | Responsibility |
| --- | --- |
| `go.mod` | Defines the shared core module consumed by CLI and server integrations. |
| `session/session_manager.go` | Manages active sessions, transcript storage, and compression policies. |
| `session/history_store.go` | Persists conversation history to SQLite/bolt DB with pruning strategies. |
| `providers/router.go` | Dispatches requests to the appropriate provider based on task type and policy. |
| `providers/openai/client.go` | Implements OpenAI-compatible chat/embedding calls with streaming support. |
| `providers/hf/client.go` | Hugging Face inference client with model-specific adapters. |
| `providers/local/runtime.go` | Bridges to kolosal-server or other local inference runtimes over gRPC/WebSocket. |
| `prompts/catalog.go` | Manages prompt templates, variables, and versioning across workflows. |
| `tasks/executor.go` | Schedules workflow automation jobs with concurrency limits and cancellation. |
| `tasks/git_workflow.go` | Encapsulates git automation (branch prep, rebase assist, changelog generation). |
| `tasks/test_runner.go` | Executes project tests/linters and parses structured results. |
| `workspace/indexer/indexer.go` | Builds and maintains repo indexes (ripgrep, ctags/tree-sitter). |
| `workspace/mutator/apply_patch.go` | Applies AI-generated patches with conflict detection and rollback. |
| `workspace/snippets/snippets.go` | Extracts relevant code/context from files for prompt construction. |
| `config/loader.go` | Resolves layered configuration: defaults, env, project overrides. |
| `config/schema.go` | Validates config with JSON schema-like constraints and helpful errors. |
| `telemetry/logger.go` | Provides structured logging utilities with redaction helpers. |
| `telemetry/metrics.go` | Exposes metrics emitters and OpenTelemetry integration hooks. |
| `fault/errors.go` | Error taxonomy helpers for consistent user messaging and retry decisions. |

#### `packages/providers`
| File | Responsibility |
| --- | --- |
| `registry.go` | Public extension point where third-party providers register capabilities. |
| `manifest.go` | Defines provider metadata (auth requirements, supported modalities, limits). |
| `loader.go` | Discovers provider plugins on disk and wires them into the router. |
| `plugin/example/main.go` | Template demonstrating how to implement a custom provider. |
| `auth/keyring.go` | Shared helpers for securely storing and retrieving provider credentials. |

#### `packages/workspace`
| File | Responsibility |
| --- | --- |
| `filesystem/fs.go` | Abstracts file operations with safe writes, backups, and change notifications. |
| `search/search.go` | High-level query API combining ripgrep, AST search, and heuristics. |
| `diff/diff.go` | Generates human-friendly diffs, patch hunks, and unified output for TUI rendering. |
| `selectors/path_filters.go` | Implements include/exclude logic for large repos and monorepos. |
| `cache/cache.go` | Maintains on-disk caches for indexes and provider metadata. |

#### `packages/testkit`
| File | Responsibility |
| --- | --- |
| `fixtures/fixtures.go` | Loads standardized repositories and transcripts for deterministic tests. |
| `golden/golden.go` | Helpers for managing golden files and snapshot comparisons. |
| `mocks/providers.go` | Mock provider implementations for unit and integration testing. |
| `mocks/workspace.go` | Fake workspace services to simulate large repo operations quickly. |

Each package adheres to Go workspace conventions: modules in `packages/` export stable APIs, while the `internal/` tree retains implementation details specific to the binary. This split lets other binaries (e.g., future VS Code companion tooling) reuse core logic without depending on CLI-specific code.

### Parity Checklist vs. Current TypeScript Core
| Source Component | Behaviors That Must Be Reimplemented | Gap in Current Plan | Go Implementation Notes |
| --- | --- | --- | --- |
| `GeminiClient` (`client.ts`) | Session turn limits, loop detection, IDE context memoization, flash model fallback, compression thresholds (`COMPRESSION_TOKEN_THRESHOLD`, `COMPRESSION_PRESERVE_THRESHOLD`), forced full-context refresh. | Plan references session management but omits concrete policies for compression, fallback, and loop detection. | Add `packages/core/session/loop_detector.go`, `session/compression.go`, and `providers/fallback.go` to codify thresholds, last-prompt tracking, and dynamic model switching.
| `GeminiChat` (`geminiChat.ts`) | Stream validation, retry policies, curated history extraction, empty-stream error handling. | TUI flow does not specify stream sanitization or automatic retries. | Introduce `packages/core/providers/gemini/chat.go` with retry backoff constants, `isValidResponse` equivalent, and typed stream events surfaced to Bubble Tea messages.
| `ContentGenerator` & `LoggingContentGenerator` | Unified interface for sync/stream calls, token counting, embeddings, telemetry-decorated wrapper. | Plan lacks explicit factory + decorator stack for provider logging. | Design `packages/core/providers/factory.go` returning a `ContentGenerator` plus optional decorators (logging, tracing). Implement `logging/content_generator.go` with structured telemetry hooks.
| OpenAI pipeline (`openaiContentGenerator/*`) | Provider selection, message normalization, streaming tool-call parsing, enhanced error handling, cache-control metadata. | Provider section in plan mentions adapters but not pipeline mechanics or streaming parser. | Add `packages/providers/openai/pipeline.go`, `message_normalizer.go`, `streaming_tool_parser.go`, and provider-specific request builders. Mirror DashScope/OpenRouter behaviors and error suppression strategies.
| `CoreToolScheduler` & `nonInteractiveToolExecutor` | Rich tool-call state machine (validating, awaiting approval, executing, success/error), approval handlers, editor callbacks. | Workflow automation section lacks details about interactive tool orchestration and approval gating. | Plan new package `packages/core/tools/scheduler.go` with explicit states, observer callbacks, and helper `tools/non_interactive_executor.go`.
| Prompts library (`prompts.ts`) | Core system prompt generation, compression prompt, project summary prompt, subagent reminders, URL normalization. | Plan lists `prompts/catalog.go` but doesn’t address specific prompt categories or personalization rules. | Expand to `prompts/system.go`, `prompts/compression.go`, `prompts/subagents.go`, `prompts/summary.go`; ensure user memory and URL normalization utilities are ported.
| Token limits (`tokenLimits.ts`) | Model normalization, limit lookup for input/output windows, defaults for unknown models. | No roadmap mention of token budgeting logic feeding compression/job planner. | Introduce `packages/core/models/token_limits.go` consumed by session compression and provider router.
| Telemetry/logger (`logger.ts`) | JSON log rotation, filename-safe tagging, corrupted-log backups, checkpointing. | Telemetry section focuses on metrics but omits durable log capture semantics. | Implement `packages/core/telemetry/logger.go` mirroring encode/decode helpers, checkpointing, and recovery flows.
| `turn.ts` events | Structured event enums for streaming thoughts, tool-call requests, compression status, session token overruns. | Data flow diagram references messages but not structured event model. | Define `packages/core/protocol/events.go` with enums/structs and ensure TUI uses strongly typed channels.
| `GeminiRequest` utilities | Verbose logging helpers for Gemini request parts. | Missing mention of lower-level debugging utilities. | Provide `packages/core/providers/gemini/request.go` with pretty-printer used by logging decorator.

### Additional Coverage: Tools, Utilities, MCP, and Prompts
| Area | TS Source Highlights | Status in Current Plan | Required Go Additions |
| --- | --- | --- | --- |
| Declarative tools (`packages/core/src/tools/tools.ts`) | Tool builder/invocation pattern, confirmation flows, tool result displays, schema cycle detection. | Plan references `internal/tasks` but lacks explicit tooling infrastructure. | Introduce `packages/tools/` module with `builder.go`, `invocation.go`, `result.go`, and JSON schema utilities. Ensure scheduler integrates with tool confirmation payloads. |
| File utilities (`utils/fileUtils.ts`) | MIME detection, binary heuristics, truncation logic, processed file result struct, line range metadata. | Workspace section mentions indexing but not detailed file processing. | Add `packages/workspace/files/reader.go`, `mime.go`, and `binary_check.go` replicating heuristics and return types for TUI display. |
| File system & Git services (`services/fileSystemService.ts`, `gitService.ts`) | Abstracted filesystem operations, shadow git repo for checkpointing, snapshot/restore flows. | Runtime/workspace notes mention backups but missing Git checkpoint strategy. | Implement `packages/services/filesystem` (interfaces, std impl) and `packages/services/git` with shadow repo setup and snapshot APIs. |
| Loop detection (`services/loopDetectionService.ts`) | Tool/content loop heuristics, adaptive LLM checks, chunk hashing. | Plan notes loop detection but lacks module names. | Create `packages/services/loopdetection` with Go equivalents for thresholds, chunk analysis, and LLM-based backup checks. |
| Shell execution (`services/shellExecutionService.ts`) | Pty vs child_process fallback, structured output events, abort handling. | Workflow automation mentions command runner but not specifics. | Add `packages/services/shell` with `executor.go` supporting pty (via `creack/pty` or `weaver`), structured events, binary detection, and abort semantics.
| Chat recording (`services/chatRecordingService.ts`) | Session transcript JSON writer, token summaries, tool call logging. | Session persistence referenced but no detailed recorder. | Implement `packages/services/chatrecording` with JSON writer, resume logic, and integration hooks for TUI notifications.
| Prompt registry (`prompts/prompt-registry.ts`, `prompts/mcp-prompts.ts`) | MCP prompt registry with dedupe, server scoping. | Plan lists `prompts/catalog.go` but omits registry + MCP integration. | Add `packages/prompts/registry.go` plus `mcp_prompts.go` to expose server-specific prompts and dedupe naming. |
| Subagents (`subagents/subagent.ts`) | Context state, template interpolation, statistics, tool usage tracking, function-call processing. | Go plan does not mention subagents or nested orchestration. | Define `packages/subagents/` containing context state, execution scope, template helpers, statistics, and hooks for tool usage analytics. |
| MCP OAuth utilities (`mcp/oauth-utils.ts`) | OAuth metadata discovery, WWW-Authenticate parsing, SSE detection. | Plan lacks OAuth discovery story for MCP. | Add `packages/mcp/oauth.go` with metadata fetch, discovery, parsing, and SSE helpers; ensure `internal/serverlink` reuses it. |

### Supporting Tooling
- **Build:** `make build` executes `go build ./cmd/kolosal`, while `make bundle` performs static asset embedding with `go:embed` and `mage` tasks.
- **Testing:** `go test ./...`, snapshot comparisons via `termdash`, and integration suites orchestrated with `gotestsum`.
- **Linting/Format:** `golangci-lint` with custom linters for prompt hygiene; `go fmt` and `goimports` enforced pre-commit.
- **Release:** Cross-compilation using `goreleaser` with brew tap formula generation and optional pkg/msi packaging stubs.

## Data Flow & Interaction Contracts
- **Prompt Workflow**
  1. User input captured in the chat view triggers a `SubmitPromptMsg`.
  2. `internal/domain` validates context, gathers repository snippets, and dispatches a `ProviderRequest` via the routing layer.
  3. Provider streams `TokenChunkMsg` events that update the transcript and optionally trigger speculative edits.
  4. Completed responses may enqueue automation jobs (e.g., apply patch) executed by `internal/tasks`.
  5. Result summaries and diffs are emitted back to the TUI and optionally persisted in session history.
- **Workspace Mutation Contract**
  - Inputs: `{ path string, original string, patch string, metadata map[string]string }`.
  - Outputs: `{ status enum, applied bool, conflicts []Conflict, followUpActions []Action }`.
  - Errors escalate through `fault.Wrap` to convey recoverable vs fatal states.
- **Provider Interface Contract**

  | Method | Inputs | Outputs | Error Modes |
  | --- | --- | --- | --- |
  | `Chat(ctx, prompt Prompt)` | Context with deadline, prompt payload, stream sink | Streamed tokens + metadata | Timeout, auth failure, rate limit, provider-specific |
  | `Embed(ctx, docs []Document)` | Token budget, batch hints | Vectors, usage metrics | Model unsupported, payload too large |
  | `Capabilities()` | None | Feature flags (vision, functions, streaming) | n/a |

## Implementation Roadmap
1. **Foundation (Weeks 1-3)**
  - Initialize module, CI scaffolding, Makefile, pre-commit hooks.
  - Implement minimal Bubble Tea app with placeholder panes and configuration bootstrap.
  - Establish logging, telemetry toggles, and error taxonomy.
2. **Core Interactions (Weeks 4-6)**
  - Integrate workspace indexing, ripgrep bindings, and baseline provider (OpenRouter / OpenAI-compatible).
  - Implement streaming chat pane, context browser, job monitor, and notifications.
  - Add session persistence and config profiles.
3. **Workflow Automation (Weeks 7-9)**
  - Port git automation, test execution, diff application, and patch preview flows.
  - Introduce task scheduler with cancellation and concurrency limits.
  - Harden workspace mutations with backup, rollback, and conflict resolution.
4. **Provider Ecosystem (Weeks 10-12)**
  - Add Hugging Face, local inference (via kolosal-server), and vision model support.
  - Implement provider marketplace plugin loader and dynamic routing rules.
5. **Polish & Distribution (Weeks 13-15)**
  - Complete end-to-end smoke tests, telemetry dashboards, and performance regression suite.
  - Package cross-platform binaries via `goreleaser` and craft onboarding docs.
  - Run closed beta, gather feedback, iterate on UX polish.

## Testing & Quality Strategy
- **Unit Tests:** `go test` with table-driven cases for domain logic, provider adapters, and config parsing.
- **Snapshot & Golden Tests:** Capture TUI render states using `bubbles/testing` harness and golden files in `testdata/`.
- **Integration Tests:** Spin up fixtures of repositories, execute scripted prompts, and assert file mutations + AI transcripts.
- **Performance Benchmarks:** Measure end-to-end latency for prompt handling, workspace scans, and diff application using Go benchmarks.
- **Static Analysis:** Enforce `golangci-lint` (vet, gosimple, staticcheck) and run `go test -race` for data races.
- **Security Reviews:** Dependency scanning with `govulncheck`, secrets detection, and sandbox validation when executing automation tasks.

## Deployment & Distribution
- Primary artifacts: statically linked binaries for macOS (arm64, amd64) and Linux (amd64, arm64), built via `goreleaser`.
- Optional packaging: Homebrew formula, AUR PKGBUILD, Docker container with pre-configured providers.
- Configuration directories follow XDG base spec; Windows installer (MSIX) planned post-MVP.
- Auto-update strategy: integrate with `selfupdate` to fetch signed releases, with manual approval inside the TUI.

## Risks & Mitigations
- **Terminal Rendering Complexity:** Mitigated with snapshot testing and visual regression suite.
- **Provider Rate Limits:** Implement budget-aware request scheduler and backpressure in Bubble Tea loop.
- **Filesystem Mutations:** Use transactional writes with backup/rollback to avoid data loss.
- **Concurrency Bugs:** Adopt structured concurrency patterns (`errgroup`, context deadlines) and race detector in CI.
- **Large Repository Performance:** Cache indexes, lazy-load file trees, and provide progressive disclosure of results.

## Success Metrics
- Median prompt round-trip < 3s against remote providers.
- P0 crash rate < 0.5% across beta cohort.
- ≥90% positive developer feedback on usability surveys.
- Coverage ≥80% for domain and provider packages; snapshot coverage for critical TUI components.

## Dependencies & External Integrations
- `github.com/charmbracelet/bubbletea`, `bubbles`, `lipgloss` for TUI.
- `github.com/spf13/cobra` or `urfave/cli` for CLI flag parsing (final choice TBD).
- `github.com/go-git/go-git/v5` for git automation, `github.com/BurntSushi/toml` / `gopkg.in/yaml.v3` for config.
- Optional: `github.com/charmbracelet/wish` for future SSH-driven UX, `github.com/sashabaranov/go-openai` for provider baseline.

## Open Questions
- Should kolosal-server communication stay gRPC or shift to WebSocket-over-stdin to simplify packaging?
- Is a plugin marketplace necessary at MVP, or can we rely on configuration-driven providers?
- How much of the existing prompt library should be auto-migrated vs. authored specifically for Go runtime?
- What’s the acceptable compromise between offline capability and provider parity for early adopters?

## Next Steps
1. Validate design with stakeholders and align on MVP scope vs stretch goals.
2. Spike Bubble Tea prototype to confirm layout feasibility and performance.
3. Finalize provider interface contract and select first-party SDKs.
4. Schedule architecture review and resource allocation for the 15-week roadmap.

## Proactive Enhancements
- **Theme & Accessibility Toolkit:** Offer high-contrast palettes, screen-reader friendly transcripts, and user-defined keymaps exported/imported as JSON.
- **Prompt Library Sync:** Build a shared prompt registry backed by `kolosal-server` so multiple team members can reuse curated automations.
- **Plugin Playground:** Provide a `kolosal plugin init` command scaffolding Go plugins with examples, documentation, and automated lint checks.
- **Observability Dashboard:** Ship a lightweight web UI (built with `bubbletea` `huh` forms or a small Go HTTP server) to inspect sessions, token usage, and provider health in real time.
- **Config Migration Tool:** Auto-convert existing TypeScript/Ink settings into the new Go format to smooth adoption for current users.
