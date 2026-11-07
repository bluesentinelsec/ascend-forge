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
- The desktop entrypoint is `src/main.c`. The version API is the C header generated from `cmake/version.h.in`. Apple UI stays Objective-C. `tests/ios/test_main.mm` may stay Objective-C. The Android activity stays Java. SDL sources stay as SDL ships them.
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
- Prefer `Slice_` (`include/ascend_forge/slice.h`) for a dynamic collection. Stamp the element type with `Slice_Define` and keep the elements in the caller's arena.
- Use SDL family allocation functions (and SDL3 standard library wrapper functions in general).
- Memory allocation failure is fatal by default. The program logs the failure and exits. Do not attempt graceful recovery from OOM in normal application paths; the complexity is not worth it for this class of tool.
- Error handling is built around invariants and centralized cleanup. Use explicit error codes or result structs at module boundaries. Fail fast when an invariant is broken.
- Resource acquisition is paired with a single, obvious cleanup path (goto cleanup labels or equivalent structured approach). Avoid scattered free calls.

## Primitives

Primitives are the stable base the host and every plugin share. They hold no product behavior. A primitive is plain C, it composes with the other primitives, and a job can use it when the caller hands it data that job owns.

A primitive has three homes:

- Public contract: `include/ascend_forge/<name>.h`
- Implementation: `src/<name>/`, in the primitives static library
- Tests: `tests/<name>/`

SDL3 is the platform layer under the primitives. Link that library statically on desktop, Android, iOS, and web. It is the C runtime and the window, input, and audio layer. Each of those builds runs the primitive tests. A platform package that publishes a primitive header also publishes the static SDL it was linked with. The desktop host executable links that same static library, so a plugin uses the host's SDL and not a second copy. The installed core archive does not link it.

The arena (`Arena_` in `include/ascend_forge/arena.h`) is the memory primitive:

- One caller owns an arena. Jobs do not share one. The arena has no lock.
- The backing store is one `SDL_malloc`. Reset and rewind keep that block. Destroy frees it.
- Capacity must be greater than zero. A zero capacity returns a non-NULL sentinel and does not allocate.
- `Arena_Alloc` failure leaves the used count unchanged and returns a non-NULL byte that the caller does not write.
- A size of zero succeeds and consumes nothing. Alignment is a power of two.
- When `SDL_malloc` fails, log and exit. Callers do not recover from that failure.
- A job receives an arena that dies with the job or with the frame. One thread owns it for that lifetime.

The byte span and UTF-8 string (`Bytes_` and `Utf8_` in `include/ascend_forge/utf8.h`) borrow memory. They do not own it.

- A span is a pointer and a length. The empty span is a real pointer and a length of zero.
- Compare, prefix, suffix, and UTF-8 walks use SDL's string functions. A byte span compares with `SDL_memcmp`. UTF-8 text compares with `SDL_strncmp`.
- `Utf8_Next` walks with `SDL_StepUTF8`. A byte that is not UTF-8 is invalid. The real U+FFFD sequence stays valid.
- `Utf8Builder_` appends into one arena and has one owner. Jobs may read the borrowed text. They do not share the builder.
- A NUL terminator is written only by `Utf8Builder_CString`, for an SDL or C boundary.
- When the arena cannot hold the next append, the builder's text stays as it was.

The result (`Result_` in `include/ascend_forge/result.h`) is the one shape for a fallible call.

- A result says whether the call worked, a code, what was expected, what arrived, and what to do next.
- The strings borrow static text or the caller's arena. A result does not allocate.
- Out of memory stays fatal inside the arena. A result does not report it.
- Code zero is success. A failure uses a caller-defined code other than zero.
- A failure leaves the callee's output untouched when that callee documents the contract.
- Jobs may read a result. The borrowed strings stay valid for as long as the memory they point at.

File and path calls (`Io_` in `include/ascend_forge/io.h`) sit on SDL.

- Read a file into the caller's arena. Write a span to a path. Join paths as arena text.
- Use `SDL_IOStream`, `SDL_GetPathInfo`, `SDL_GetBasePath`, and `SDL_GetPrefPath`.
- One job owns a path while it writes that path. Two jobs do not write the same path.
- A missing file or a short transfer returns a `Result_` and does not exit. The output span stays as it was.
- A short read still consumes arena space for the attempt. A short write may leave a partial file.
- Out of memory stays fatal inside the arena.

JSON (`Json_` in `include/ascend_forge/json.h`) parses one immutable value into the caller's arena.

- The reader is yyjson 0.13.0, vendored as ANSI C under `third_party/yyjson/` and compiled into the primitives library. It is not a public header, and its symbols stay hidden. No C++ JSON library is part of this primitive.
- Emscripten Debug compiles `third_party/yyjson/yyjson.c` at `-O1`. At `-O0`, `yyjson_read_opts` has more Wasm locals than the browser allows. The rest of that debug build stays unoptimized. Release keeps the toolchain optimization.
- yyjson is built freestanding. Its memory and string calls are `SDL_memcpy`, `SDL_memmove`, `SDL_memset`, `SDL_memcmp`, and `SDL_strlen`. It does not open files. `Json_` passes an arena allocator: growth copies into a new block, and free waits for the arena.
- A string is a `Utf8` span. A number keeps its source span. Arrays and objects are contiguous ranges.
- `Json_Stringify` appends to a `Utf8Builder`. A value can live in a private arena while the builder lives in another.
- Invalid JSON returns a `Result_` and leaves the output value untouched. Arena space used by the attempt stays consumed. A short arena returns a result. `SDL_malloc` failure stays fatal inside the arena.
- One writer runs during the parse. After it returns, any job may read the tree. The borrowed text stays valid for as long as that arena does.

The slice (`Slice_` in `include/ascend_forge/slice.h`) is the preferred dynamic collection. It is a growable contiguous range.

- `Slice_Define` stamps one element type. Call sites use `Slice_<Name>_Init`, `Slice_<Name>_Append`, and `Slice_<Name>_Remove`, and see a typed pointer, a count, and a capacity.
- The bytes are one range in the caller's arena. The slice does not call `SDL_malloc`.
- One owner. Jobs may read a finished slice. They do not share a slice that can still grow.
- The empty slice is a real pointer and a length of zero. Init does not allocate.
- Append copies into a new arena block when the current block cannot hold the next element. An append that does not fit leaves the slice unchanged. `SDL_malloc` failure stays fatal inside the arena.
- Element alignment is the type's alignment, and that alignment is a power of two.
- Remove compacts the range. It does not free. The arena owns the bytes.

The atomic (`Atomic_` in `include/ascend_forge/atomic.h`) publishes a finished value and counts completed jobs.

- `AtomicU32` wraps `SDL_AtomicU32`. Init happens before any other caller can reach it. Load, store, add, and compare-and-swap are the operations. Add returns the previous value.
- Callers share the address of a counter. They do not copy it.
- `AtomicPointer_Init` stores the unpublished null. `Atomic_Publish` applies a release barrier and then stores a pointer. `Atomic_Acquire` loads that pointer and applies an acquire barrier before the caller reads the value.
- The primitive does not allocate and does not start a thread.
- A null counter or slot is logged and rejected. A failed call that writes an output leaves that output untouched.

The thread (`Thread_` in `include/ascend_forge/thread.h`) starts one worker and joins it.

- `Thread_Init` marks the thread as not started. `Thread_Run` starts one worker with `SDL_CreateThread`. `Thread_Join` waits with `SDL_WaitThread` and reports the worker's return value.
- One owner joins. The caller chooses how many workers to start. This primitive does not size a pool.
- The worker does not call SDL window, event, or renderer functions. This primitive does not call them either. The SDL thread handle stays inside the primitive.
- A failed create is logged and returns false. The thread stays as it was. Joining a thread that is not running is rejected and leaves the status untouched.
- Web stays single-threaded. The primitive compiles there, and the rejection checks run there. Spawn checks run on desktop and Android. The iOS workflow stays manual.
- Plugins submit jobs to the host. They do not call `SDL_CreateThread` for host or scene data.

The time (`Time_` in `include/ascend_forge/time.h`) is the monotonic nanosecond clock.

- `Time_Now` returns `SDL_GetTicksNS`. The value does not go backward for the life of the process. It is not a wall clock.
- The call does not allocate and does not start a thread.
- A phase join reads this clock when it needs a duration. Higher layers do not call the SDL timer themselves.

The index range (`IndexRange_` in `include/ascend_forge/index.h`) is the usual job shape. It is a half-open `[begin, end)` over a contiguous array.

- Init rejects an end that is before begin and leaves the range untouched. An empty range has `begin == end`.
- `IndexRange_ForEach` walks that range on the calling thread. An empty range calls the function zero times. The worker writes only indexes inside its range.
- `IndexRange_Split` covers the parent with contiguous parts, no gap and no overlap. The first parts receive one extra index when the count does not divide evenly. A part past the last index is empty.
- The caller owns the input span, the write span, and the arena. This primitive does not look at them, does not allocate, and does not start a thread.
- A null argument is logged and rejected. A failed call that writes an output leaves that output untouched.

The fork-join (`ForkJoin_` in `include/ascend_forge/fork.h`) runs one index-range job on a caller-chosen number of workers.

- The caller passes zeroed `Thread` storage and the count. This primitive does not size a pool.
- `IndexRange_Split` cuts the parent into that many parts. Each worker walks its part with `IndexRange_ForEach` and writes only that part. An empty part still completes.
- The completion count is an `AtomicU32` that starts at the worker count. The call returns after that count is zero.
- Workers start with `Thread_Run` and the owner waits with `Thread_Join`. Workers do not call SDL window, event, or renderer functions.
- On web, where threads do not start, the same call walks each part on the caller and the count still reaches zero.
- A null argument, a worker count of zero, or a range whose end is before its begin is rejected and the function is not called. The temporary worker records are one SDL allocation freed before return. When that allocation fails, the call logs and exits.

Adding a primitive:

- Add the public header, the source in the primitives static library, and self-hosted tests for the contract.
- Use SDL types. Allocate caller data from an arena the caller owns.
- Keep the C warning set, and treat warnings as errors.
- Run the new tests on desktop, Android, iOS, and web. A check that needs a window runs where a window exists. Every other check runs on all four.
- Ship the header with the platform packages that link the primitives.

Changing a primitive is a host change:

- State why the host must change.
- Add the failing test first, then make it pass.
- Keep the public contract stable. When a signature has to change, update every caller and every platform test in the same change.
- Re-run the primitive tests on desktop, Android, iOS, and web.

### Structure

- Keep data structures, pure logic, and UI / presentation code in separate modules or clearly delineated sections.
- The host provides services (window, input, timing, command palette, scene switching, job scheduling). Plugins consume those services; they do not reimplement them.
- Global mutable state is minimized and explicitly owned.

## Parallelism

One scheduler, owned by the host. Declare dependencies. Give each byte one writer for the duration of a phase. Join before the next phase reads the result. Do not coordinate with a web of locks.

The shape is a main thread plus a fork-join worker pool. A task graph (jobs with prerequisites, nested jobs that complete with their parent) may sit on that same pool when a plugin has real dependencies beyond "split this array, then continue." Fibers and a Verse-style `sync` / `race` API are not the first scheduler. If a structured-concurrency API arrives later, implement it as jobs and joins.

### Main thread and workers

- The main thread initializes SDL, pumps events, owns the window and renderer, activates plugins, and runs the frame. The first caller that opens a window, pumps events, or renders does that work on the thread that initialized video.
- `SDL_PollEvent` stays on the thread that initialized video. Rendering stays on that thread. Workers, including a job started with `Thread_`, never call SDL window, event, or renderer APIs. Primitive tests do not open a window.
- Create about one worker per core, minus the main thread. Do not oversubscribe. `Thread_` does not size that pool. The caller chooses how many workers to start.
- Workers run host jobs to completion. A worker starts with `Thread_Run` and the owner waits with `Thread_Join`. Plugin code does not call `SDL_CreateThread` for host or scene data.

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
- Desktop, Android, iOS, and web link SDL3 statically and run the primitive tests with that platform's other tests.
- The iOS workflow runs only when someone starts it. Pull requests do not wait on that job.
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

## UX documents

Docs under `docs/ux/` are living documents, one per way of invoking the host. `docs/ux/cli.md` is the CLI.

While a surface is still being built, its UX doc is the roadmap. Implement the feature so the behavior matches the doc. The CLI is in this phase. The commands in `docs/ux/cli.md` are the goal, and the host does not implement them yet.

When the goals in a UX doc are met, that doc switches to maintenance. It then describes what the implementation does. A behavior change updates the doc in the same change. A doc that disagrees with the implementation is wrong.

A pull request that only changes these docs does not need to wait for CI.

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
- Treat `docs/ux/` as living. Follow the UX doc while that surface is unfinished. After its goals are met, update the doc in the same change as the behavior.
- Write plain, explicit, initialized C. Use SDL types. Avoid NULL in high-level code.
- The desktop entrypoint is `src/main.c`. Do not add `src/main.cpp`.
- The desktop host executable links `SDL3-static`. Do not link that library into the installed core archive.
- Allocate from arenas. Treat allocation failure as fatal.
- Prefer `Slice_` for a dynamic collection. Keep its elements in the caller's arena.
- Publish a finished value with `Atomic_Publish`. Readers use `Atomic_Acquire`.
- Start a worker with `Thread_Run` and wait with `Thread_Join`. That worker does not call SDL window, event, or renderer APIs.
- Read a duration with `Time_Now`. The clock is monotonic. It is not a wall clock.
- Split work with `IndexRange_Split`. A job walks its half-open range with `IndexRange_ForEach` and writes only that range.
- Run that split with `ForkJoin_Run`. The caller chooses the worker count. The call returns when the completion count is zero.
- Keep primitives in the shared C library, statically linked to SDL3 on desktop, Android, iOS, and web, and run their tests on each of those platforms.
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
