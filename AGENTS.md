# AGENTS.md

The project name is "Ascend Forge."

Ascend Forge is a small, batteries-included rapid application development host written in plain C with SDL3. It gives you a cross-platform window, input, and scene system with almost no package dependencies and a static-link-friendly build, so you can drop in a plugin and start building the actual tool instead of wiring up infrastructure. Ascend Forge provides a robust, stable, and malleable surface from which to focus on user experience, business logic, and data model without being forced to follow a particular convention.

## Project Vision

This is a thin, portable host application for rapid application development. The host itself contains almost no domain logic. Nearly all behavior lives in plugins. The host will provide a small catalog of curated plugins that ship by default. Each plugin owns a **scene**: a self-contained interactive surface (canvas, tool, editor, utility, etc.) that the user activates and works inside.

The host's job is discovery, activation, context switching, and providing a stable, minimal API surface (window, input routing, menus, status, command palette). Once a plugin is active, it owns the main content area and its interactions.

Primary user-facing navigation:

- A launcher / home surface that shows available plugins as searchable, filterable cards, or a hierarchical list view with collapsible / expandable elements.
- A command prompt / fuzzy search for fast activation by name.
- A lightweight switcher (sidebar, recent list, or pinned favorites) so the user can move between open scenes without returning all the way to the launcher.
- Clear, low-cost ways to return to the launcher or close a scene.
- Global settings screen for host application configuration.
- Global settings should be provided from the CLI (highest precedence), a user-specified config file (next precedence), and the default config (favors usability and robustness by default).
- The data model and API contract should enforce correct, professional-grade plugins with semantic versions, complete metadata, behavioral tags, logical groups, dependencies, etc.
- Favor simplicity and terseness like Go in all things.

The long-term goal is that the host layer stabilizes early. Subsequent work is almost entirely additive: new plugins, plugin improvements, or removal of plugins. Changes that require modifying host code should be rare and deliberate.

## Technology Constraints

- Language: plain C (C20).
- Multimedia / platform abstraction: SDL3.
- Everything else is implemented as first-party project libraries. Minimize external dependencies and license obligations. The project owns its fate.
- Target high portability. Prefer SDL types and the SDL standard library wrappers so behavior stays consistent across platforms.
- No C++. No heavy frameworks. No dependency managers that pull in large transitive graphs.
- Plugin authors should be able to write (unsafe) native plugins in C, and safe / managed plugins in Lua (embedded).
- Unit tests are of critical importance. Everything should be testable, especially plugins.
- One of the default / host-provided plugins should be "New Plugin": a plugin bootstrap, either in Lua or in C. Users should fill out a menu whose inputs bootstrap the plugin following idiomatic convention from robust / professional-grade templates. Make build, test, and deploy cycles fast and easy.

## Architectural Principles

- **Open/Closed**: The host is closed for modification and open for extension via plugins. New capability arrives as a new plugin or an extension of an existing plugin, not as changes to host internals.
- **Single Responsibility**: Separate data model, logic, and user-experience presentation. A module should have one clear reason to change.
- **Fail fast**: Detect invariant violations as early as possible. Prefer explicit checks over silent recovery.
- **Data-oriented**: Favor arena allocation, contiguous storage, and simple data layouts. Avoid pointer-heavy graphs in hot paths when a simpler structure will do.
- **Additive evolution**: Pull requests should primarily add or modify plugins. Host changes require strong justification.
- **User experience agnostic**: The host application needs to work as a shared library, CLI, and GUI application. GUI is the default / most expected use case; however, Ascend Forge can also be used to rapidly create plugin-driven CLI tools. Users should be able to query and invoke plugins that could be one-shot CLI tools, long-lived daemons, visual GUI applications, or video games.

## Plugin Model

A plugin is a scene plus the behavior that occurs inside it.

- Plugins register themselves with the host (name, description, category, icon/thumbnail, entry points).
- The host never reaches into plugin internals. Communication happens through a narrow, stable interface the host defines.
- A plugin may be activated, deactivated, and destroyed. The host owns that lifetime. See Plugin roles. Deactivate and destroy end a run. They do not remove the plugin.
- Plugins must be able to run with the host's memory, error, and parallelism model (see below). Memory allocation failures should be treated as fatal to simplify error handling.
- Plugins submit jobs to the host scheduler. They do not create their own threads for host or scene data.
- File associations, "New X", or command-palette entries are how plugins advertise entry points; the host routes them.

## Plugin roles

Three roles handle every plugin. The steps in a role stay the same for a person, for CI, and for an agent.

### Author

The author writes the plugin and publishes it.

1. Bootstrap a workspace.
2. Create the plugin.
3. Change it.
4. Validate its contract.
5. Test, debug, and inspect it.
6. Add tests for missing behavior.
7. Set its semantic version.
8. Publish it to that plugin's own git remote.

Create and publish include the plugin's media. A plugin reads media from disk, from one packed archive, or embedded in the binary. Publish yields one self-contained binary, or one packed archive when the data is large. Embedded media may be encrypted with a symmetric key.

### Caller

The caller finds plugins and invokes them. The caller may also be the author.

1. Query the catalog. The catalog includes plugins shipped with the host, plugins in a directory, and plugins in a git repo.
2. Get a version.
3. Run one or more plugins and pass their arguments.
4. Get a newer version when one is published.
5. Remove a plugin. Remove deletes its source and its registration.

### Host

The host owns the plugin for the length of a run.

1. Register it, so the catalog and its entry points can see it.
2. Activate it.
3. Deactivate it.
4. Destroy it.

Create registers a local plugin. Get registers a fetched one. Deactivate ends a run and leaves the plugin in place. Destroy drops that run's scene state. Remove is a caller step.

## Coding Standards

### General

- Write clean, simple, human-readable C. Avoid syntactic sugar and clever tricks.
- Correctness and defensive programming take priority over brevity.
- Every variable is initialized at the point of declaration.
- Types are explicit. Prefer SDL types (`Sint32`, `Uint64`, `SDL_bool`, etc.) and project typedefs over raw C types for consistency.
- Public functions require Doxygen-style comments that state purpose, parameters, return value, and any important preconditions or side effects.
- Function and variable names must reveal intent. Prefer longer, clear names over abbreviations.

### Naming and Scope (Go-style)

- Exported (public) symbols: `namespace_PascalCase` or `PluginName_FunctionName` style as appropriate to the module.
- Unexported (file-local) symbols: `namespace_pascalCase` with a clear private convention; pick one and stay consistent within a module.
- Use namespaces (prefixes) aggressively to avoid symbol collisions: `Host_`, `Scene_`, `PluginPng_`, `Arena_`, etc.
- Header files expose only what other modules need. Implementation details stay in `.c` files.
- Favor const correctness and private by default for most symbols.

### Memory and Errors

- NULL is not permitted in high-level code. It may appear only in low-level unsafe or platform boundary code, and must be confined and documented.
- Prefer arena allocation (or other bulk/data-oriented allocators) for scene and plugin state. Individual `malloc`/`free` pairs should be rare outside the arena implementation itself.
- Use SDL family allocation functions (and SDL3 standard library wrapper functions in general).
- Memory allocation failure is fatal by default. The program logs the failure and exits. Do not attempt graceful recovery from OOM in normal application paths; the complexity is not worth it for this class of tool.
- Error handling is built around invariants and centralized cleanup. Use explicit error codes or result structs at module boundaries. Fail fast when an invariant is broken.
- Resource acquisition is paired with a single, obvious cleanup path (goto cleanup labels or equivalent structured approach). Avoid scattered free calls.

### Structure

- Keep data structures, pure logic, and UI / presentation code in separate modules or clearly delineated sections.
- The host provides services (window, input, timing, command palette, scene switching, job scheduling). Plugins consume those services; they do not reimplement them.
- Global mutable state is minimized and explicitly owned.

## Parallelism

One scheduler, owned by the host. Declare dependencies. Give each byte one writer for the duration of a phase. Join before the next phase reads the result. Do not coordinate with a web of locks.

The shape is a main thread plus a fork-join worker pool. A task graph (jobs with prerequisites, nested jobs that complete with their parent) may sit on that same pool when a plugin has real dependencies beyond "split this array, then continue." Fibers and a Verse-style `sync` / `race` API are not the first scheduler. If a structured-concurrency API arrives later, implement it as jobs and joins.

### Main thread and workers

- The main thread initializes SDL, pumps events, owns the window and renderer, activates plugins, and runs the frame.
- `SDL_PollEvent` stays on the thread that initialized video. Rendering stays on that thread. Workers never call SDL window, event, or renderer APIs.
- Create about one worker per core, minus the main thread. Do not oversubscribe.
- Workers run host jobs to completion. Plugin code does not call `SDL_CreateThread` for host or scene data.

### Jobs and phases

- A job receives a read-only input span, a write span that no other job aliases, and an arena that dies with the job or with the frame. The usual job is a half-open index range over a contiguous array. Use a structure of arrays when a system is hot. The worker writes only its own range.
- A frame is a fixed sequence of phases:
  1. The main thread polls input and publishes an immutable input snapshot for this frame.
  2. Fork: systems that do not depend on each other run as parallel-for jobs.
  3. Join: the main thread waits until that phase's completion count hits zero.
  4. The next phase may read the previous phase's buffers.
  5. The main thread presents, then swaps phase buffers.
- Two jobs in the same phase may both read a buffer. Only one may write a given range. The next phase sees those writes only after the join.
- Work that need not finish this frame (load, import, search) completes later through the main-thread command queue.
- The host API is three operations: submit a range job in the current phase, submit a job that runs after this phase, and enqueue a main-thread command for anything that touches SDL or another plugin's public state.

### Races

- Each byte has one writer per phase. Readers in that phase see the previous phase's buffer.
- Double-buffer anything a later phase or the renderer consumes. Phase N writes one buffer while the presenter still reads the other. Swap only at the join.
- Per-worker counters, freelists, and scratch arenas are private. Pad them to a cache line so adjacent workers do not false-share.
- Publish completed work with one atomic release store. Consumers acquire-load. Do not hand out a pointer to a half-written struct.
- A job takes no writable pointer to host state, the plugin registry, or another job's range.
- Use atomics for completion counts, and single-producer single-consumer queues from workers back to the main thread. A multi-producer multi-consumer queue belongs only inside the scheduler.

### Deadlocks

- A job does not wait on another job, take a lock, or call back into the host. If more work appears, it enqueues a child and returns. The parent completes when its children complete.
- The main thread is the only thread that waits, and it waits at phase boundaries.
- If a lock exists, it has one total order, it is not held while enqueueing or joining, and workers never take it. Prefer that this list stay empty.
- Main-thread plugin code must not join a phase while a job in that phase needs the main thread. Jobs that need SDL or the UI enqueue a command; the main thread runs it after the join.

### Bottlenecks

- Keep the serial fraction small. Input, scene switch, present, and plugin activation stay on the main thread. Bulk data work does not.
- Batch indexes so a job hides enqueue cost (on the order of a tenth of a millisecond or more) and the last wave does not leave cores idle.
- Parallelize flat arrays of ranges. Flatten pointer-heavy graphs before trying to run them in parallel.
- Measure the join. A phase that is mostly one long job is still serial. Split that job, or move it off the frame.

## Testing

Test defensively. The point of a test is to prove correctness, not to show that the author tried the happy path once.

- Unit tests are self-hosted. Do not introduce a third-party test framework.
- Use a minimal minunit-style pattern: simple assertion macros, a test runner that can be compiled into the same binary or a separate test target, and clear pass/fail reporting.
- Tests live close to the code they exercise. Prefer testing pure logic and data-model functions over full UI scenes.
- Every public module should have corresponding tests for its core invariants.
- Follow Go conventions for testing. Discovery, selection, filtering, and verbosity follow `go test`.
- Cover the happy path and the error paths. Add cases for empty input, single-element input, boundary values, rejected input, and broken invariants. Each case should state what was given and what must be true afterward.
- One behavior per test. Keep tests clean and focused: a short setup, one action, and assertions that check the contract. Do not reuse a test as a second scenario by stacking unrelated checks.
- Assert the result and the state that must not change. When a function returns an error, check the error and check that outputs were left untouched.
- Name tests for the behavior they prove (`Plugin_RejectsMissingVersion`, `Arena_ResetKeepsCapacity`), not for the function under test alone.
- Fix a bug by adding the failing case first, then make that case pass.

## Performance

Code is not fast because it looks data-oriented. Measure the impact before and after a change that claims to be faster, and before accepting a change that sits on a hot path.

- Measure the thing the user feels: frame time, phase join time, load time, or bytes touched. Record the setup (build type, machine, input size) with the number.
- Optimize only after a measurement shows a real cost. State the before and after figures in the change.
- Prefer a layout change (contiguous ranges, fewer passes, less copying) over a clever micro-optimization. Re-measure after the change.
- Debug builds keep symbols and disable optimization. Time performance in a release build. Do not compare a debug binary to a release binary.
- A parallel phase still needs a measurement of the join. If the serial remainder dominates, more workers will not help.
- Do not keep a more complex implementation whose measured gain is noise.

## Refactoring

Get the code working and tested, then refactor. Correctness comes first. Cleanliness follows while the tests still pass.

- Refactor only with tests that already cover the behavior you intend to preserve. Add missing cases before moving code.
- Improve clarity of intent: names that say what the data is, functions that do one thing, and control flow a reader can follow without comments that narrate the code.
- Remove duplication, dead paths, and speculative abstraction. A shorter function is not automatically clearer.
- Keep the public contract stable while restructuring the inside of a plugin. If a signature must change, update every caller and every test in the same change.
- After the refactor, widen coverage where the new shape exposed an untested branch. Re-run the tests. A refactor that needs the tests to be weakened is a behavior change and should be called one.
- Leave the code more reliable than you found it: the same invariants, stricter checks, and no new shared mutable state.

## Build and Quality Gates

- All compiler warnings are enabled and treated as errors (`-Wall -Wextra -Werror` or equivalent, plus any additional useful flags).
- The build must be clean on the supported platforms. No warnings are acceptable in committed code.
- Static analysis and sanitizers (ASan, UBSan) are encouraged during development; the code should remain clean under them.
- Leverage CI for testing platforms that cannot be tested locally.

## Contribution and Maintenance Guidance

- Default expectation: a pull request adds a plugin, removes a plugin, or changes logic inside one or more plugins.
- Changes to the host layer (scene management, launcher, command palette, plugin interface, core services) require an explicit rationale and exemplify "invent and simplify".
- When adding a plugin, follow the author role. The plugin appears in the launcher and command palette, the build stays warning-free, and defaults look finished.
- Static resources such as images are embedded in the host, so the application does not ship them as loose files. A plugin reads media from disk, from one packed archive, or from the binary. Publish yields one self-contained binary, or one packed archive when the data is large. Embedded media may be encrypted with a symmetric key.
- When removing a plugin, follow the caller role. Delete its registration and its source. Do not leave dead code or a commented-out registration.
- Keep the plugin interface stable. Extending it is preferable to breaking it. If a breaking change is unavoidable, document the migration clearly.
- Prefer data-oriented changes inside plugins over introducing new abstraction layers in the host.

## Invocation Surface

The host has one invocation surface. A caller names one or more plugins and supplies their arguments. The host resolves the plugins, checks the arguments against each plugin's contract, runs them, and returns results.

CLI, GUI, shared library, network, and RPC are transports for that surface. They differ in how the request arrives. They agree on which plugins run, which arguments those plugins see, and what a result means. Arguments follow the plugin contract, so a caller can pass the inputs that the plugin defines.

A request carries the plugins, the entry points, and the arguments for that call. Ambient configuration keeps its existing precedence: CLI, then a user config file, then defaults. A caller can supply the overrides for that request so the run can be repeated.

Discovery is part of the surface. A caller can read the catalog: names, versions, entry points, argument contracts, and dependencies, and can validate a request before running it.

Results are data. A failure names the plugin, what the contract expected, what arrived, and what would make the request valid. The caller that started the work can read the result, the status, and the log.

A caller follows the caller role. Every way of invoking the host follows the host role for the length of the request.

## Agentic Callers

Agentic programs such as Grok, Codex, and Claude are callers of this surface. The host contract is product-neutral. A particular agent product connects through an adapter, and the adapter carries the same request and the same result as every other transport.

The surface serves those callers well when:

- Discovery, validation, invocation, and results are available with no person operating the GUI.
- One request can name several plugins and can pass arguments among them when the caller asks for that.
- Plugin contracts are explicit and stable, so a caller can remember what a plugin accepts and can tell when the contract changes.
- Validation happens before expensive work.
- The result is sufficient for the caller to choose the next request.

## Summary for Agents

When generating or modifying code:

- Stay inside the plugin model. Do not expand the host unless the task explicitly requires it.
- Invoke plugins through the shared request surface. A request names plugins and arguments; the response returns results.
- Keep CLI, GUI, library, network, and RPC as transports for that same request.
- Expose discovery, validation, and results as data so an agent can choose the next call.
- Connect a specific agent product through an adapter. Leave the host contract product-neutral.
- Follow the author role when writing a plugin: version it, include its media, and publish it to that plugin's git remote.
- Follow the caller role when discovering or invoking a plugin: query the built-in catalog, a directory, or a git URL, then get, run, upgrade, and remove.
- Leave register, activate, deactivate, and destroy to the host. Remove is a caller step.
- Write plain, explicit, initialized C. Use SDL types. Avoid NULL in high-level code.
- Allocate from arenas. Treat allocation failure as fatal.
- Submit work to the host scheduler. One writer per byte per phase. Join before the next phase reads the result. Workers do not call SDL or take locks.
- Separate data, logic, and presentation.
- Name things so intent is obvious. Prefix for namespacing.
- Use good console logs and error messages so the user can understand where issues occurred, why they occurred (what was expected versus what was received), and what to do about it, if anything.
- Add Doxygen comments on public functions.
- Test defensively: many focused cases for the happy path and the error paths. Prove the contract.
- Measure performance on the hot path in a release build. Keep a change that claims a speedup only when the numbers show it.
- After the code works, refactor for clarity of intent, then re-run and extend the tests.
- Ensure the result compiles with warnings-as-errors.
- Prefer additive plugin changes over host modifications.
- Build the project in cross-platform-friendly CMake, with wrappers via GNU Make and `.bat` files for Windows.
- Configure release builds with professional metadata and packaging; debug builds should have debug symbols and no optimization.

This document is the source of truth for style, architecture, and evolution rules. When in doubt, choose the approach that keeps the host smaller and the plugins more self-contained.
