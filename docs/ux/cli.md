# CLI

Status: sketch. Examples are the recommended shape. Bullets marked **TBD** are unsettled.

The CLI is one transport for the host request: name one or more plugins, pass their arguments, get results back. A local path and a git URL are the same kind of selector.

- Lifecycle: [bootstrap](#bootstrap), [create](#create), [dev loop](#dev-loop), [run](#run), [query](#query), [get and publish](#get-and-publish), [remove](#remove), [CI](#ci).
- TUI, GUI, and REST carry this same request later. They are out of scope here.

## Shape

```text
ascend-forge [host flags] <command> [selector...] [-- plugin-args...]
```

- Host flags sit before the command. Plugin args sit after `--`.
- Flags override the config file. The config file overrides defaults.
- The same command runs in a terminal and in CI. No prompt unless a command says it asks.

```bash
ascend-forge --config ./forge.json --json test ./plugins/png-viewer
ascend-forge run ./plugins/png-viewer -- sample.png --out thumb.png
```

- **TBD:** binary name alias (`forge` vs `ascend-forge`).
- **TBD:** where the user config file lives, and its format.
- **TBD:** Windows launcher (same binary vs `ascend-forge.bat`).

## Selectors

A selector is a source plus an optional version.

```text
selector = source ["@" version]
source   = local-path | module-path | git-url
```

```text
./plugins/png-viewer
./plugins/png-viewer@v0.1.0
github.com/acme/forge-plugins/png-viewer@v0.3.1
https://github.com/acme/forge-plugins.git@v0.3.1
git@github.com:acme/forge-plugins.git@v0.3.1
```

- A local path with no version is the working tree.
- A remote with no version is the latest semver tag.
- A repo may hold many plugins. The path after the repo selects one.
- No central registry. The git remote is the source of truth.

```bash
ascend-forge run --entry thumb ./plugins/png-viewer -- sample.png
ascend-forge run --entry thumb github.com/acme/forge-plugins/png-viewer@v0.3.1 -- sample.png
```

- **TBD:** default remote version (latest tag vs default branch vs a named channel).
- **TBD:** entry-point spelling (`--entry` vs a `path#entry` suffix).
- **TBD:** SSH vs HTTPS URL canonical form.
- **TBD:** how a subdirectory inside a multi-plugin repo is spelled on each URL form.

## Output

Human output is terse. `--json` prints one JSON document on stdout. Logs stay on stderr.

```bash
ascend-forge inspect ./plugins/png-viewer
ascend-forge inspect --json ./plugins/png-viewer
ascend-forge test --json ./plugins/png-viewer
```

```json
{
  "ok": false,
  "plugin": "png-viewer",
  "expected": "semantic version major.minor.patch",
  "received": "",
  "next": "set version to 0.1.0"
}
```

- Exit `0`: success.
- Exit `1`: usage or contract violation.
- Exit `2`: plugin ran and failed.
- **TBD:** remaining exit codes (not found, network, partial multi-plugin failure).
- **TBD:** which fields are required on every JSON result.
- **TBD:** streaming logs for long-running plugins while still ending on one JSON result.

## Bootstrap

`init` writes a workspace whose first test already passes, so the next step is a change.

```bash
ascend-forge init png-tools
cd png-tools
ascend-forge test
ascend-forge run ./plugins/hello
```

- Creates the plugin directory, host config, and one sample plugin.
- **TBD:** sample plugin language, and whether `init` can skip it.
- **TBD:** workspace layout (`plugins/<name>/` vs a flat module root).
- **TBD:** `init` inside an existing git repo vs creating one.

## Create

```bash
ascend-forge plugin new png-viewer --lang lua
ascend-forge plugin new png-viewer --lang c
ascend-forge plugin validate ./plugins/png-viewer
ascend-forge test ./plugins/png-viewer
```

- `plugin new` writes metadata, source, a registration, and a passing test.
- `plugin validate` checks version, description, tags, group, dependencies, and entry points before a build.
- **TBD:** default language.
- **TBD:** required metadata fields at create time vs later.
- **TBD:** flags for category, tags, group, and dependencies on `plugin new`.
- **TBD:** C plugin build integration vs Lua as a file drop-in.

## Dev loop

Edit the plugin, then repeat test, run, inspect, and debug. Add a test when a behavior is missing.

```bash
ascend-forge test ./plugins/png-viewer
ascend-forge run ./plugins/png-viewer -- sample.png
ascend-forge inspect ./plugins/png-viewer
ascend-forge debug ./plugins/png-viewer -- sample.png
ascend-forge test new ./plugins/png-viewer RejectsEmptyInput
ascend-forge test ./plugins/png-viewer
```

- `inspect` prints name, version, entry points, tags, group, dependencies, and source path.
- `debug` runs the same request as `run`, with symbols and a failure that stops on the first broken invariant.
- `test new` adds one named test next to the plugin.
- **TBD:** `test --watch` and what it rebuilds.
- **TBD:** what `debug` attaches to (host debugger, Lua break, log level).
- **TBD:** `test new` file layout and assertion template.
- **TBD:** format command (`ascend-forge fmt`) and whether it is host-owned.

## Run

One plugin, arbitrary plugin args after `--`.

```bash
ascend-forge run ./plugins/png-viewer -- sample.png --out thumb.png
ascend-forge run ./plugins/png-viewer@v0.1.0 -- sample.png
ascend-forge run github.com/acme/forge-plugins/png-viewer@v0.3.1 -- sample.png
```

Several plugins in one request. The request file is the precise form for CI and agents.

```bash
ascend-forge run --file request.json
```

```json
{
  "plugins": [
    {"selector": "./plugins/decode", "entry": "main", "args": ["sample.png"]},
    {"selector": "./plugins/view", "entry": "main", "args": []}
  ]
}
```

```bash
ascend-forge run --json --file request.json
ascend-forge run --foreground ./plugins/watcher -- --dir ./inbox
```

- A plugin may be a one-shot tool, a long-running process, a scene, or a game. `run` is the same command. The plugin contract says which.
- **TBD:** per-plugin args on the command line when `--file` is not used.
- **TBD:** passing one plugin's result into the next plugin's args.
- **TBD:** `--foreground`, detach, logs, and stop for long-running plugins.
- **TBD:** headless vs opening a window when the plugin is a scene.
- **TBD:** activation lifetime (one shot, until exit, until an explicit stop).

## Query

`query` reads a catalog. No expression lists everything. A source is a directory or a git URL.

```bash
ascend-forge query ./plugins
ascend-forge query ./plugins 'tag:image group:viewers'
ascend-forge query --json ./plugins 'name:png*'
ascend-forge query https://github.com/acme/forge-plugins.git
ascend-forge query git@github.com:acme/forge-plugins.git@v0.3.1 'group:viewers'
ascend-forge query github.com/acme/forge-plugins/png-viewer@v0.3.1
```

- Filters shown: `name:`, `tag:`, `group:`, `version:`. Multiple filters are AND.
- **TBD:** filter grammar (OR, quotes, missing fields, semver ranges).
- **TBD:** `list` as an alias of `query` with no expression.
- **TBD:** whether a query of a remote repo fetches or only reads a cached copy.
- **TBD:** result columns in text mode.

## Get and publish

Publish pushes a semver tag to the plugin's own git remote. Consumers fetch by selector. A registry is optional and not required.

```bash
ascend-forge plugin validate ./plugins/png-viewer
ascend-forge test ./plugins/png-viewer
ascend-forge publish ./plugins/png-viewer
ascend-forge get github.com/acme/forge-plugins/png-viewer@v0.3.1
ascend-forge run github.com/acme/forge-plugins/png-viewer@v0.3.1 -- sample.png
ascend-forge get github.com/acme/forge-plugins/png-viewer@v0.4.0
```

- `publish` refuses a dirty tree, a version that is not semver, and a tag that already exists.
- `get` records the selector and version so a later `run` can use the fetched plugin.
- **TBD:** tag format, and whether `publish` creates the tag or only pushes an existing one.
- **TBD:** one git repo per plugin vs many plugins in one repo.
- **TBD:** where `get` stores the fetched plugin, and how upgrades are pinned.
- **TBD:** checksum or signature check, and whether it is local-only.
- **TBD:** `--yes` and other non-interactive publish flags.
- **TBD:** retracting or yanking a published tag.

## Remove

```bash
ascend-forge plugin remove ./plugins/png-viewer
ascend-forge plugin remove --json ./plugins/png-viewer
```

- Removes the plugin source and its registration together.
- **TBD:** confirmation, `--yes`, and removal of a fetched upstream plugin vs a local one.
- **TBD:** what happens to dependents that still name the removed plugin.

## CI

Non-interactive. `--json` on, no prompts, non-zero exit on failure.

```bash
ascend-forge plugin validate --json ./plugins/png-viewer
ascend-forge test --json ./plugins/png-viewer
ascend-forge run --json --file request.json
ascend-forge query --json ./plugins 'tag:image'
```

```yaml
# .github/workflows/plugins.yml
- run: ascend-forge plugin validate --json ./plugins/png-viewer
- run: ascend-forge test --json ./plugins/png-viewer
- run: ascend-forge run --json --file request.json
```

- **TBD:** caching of fetched plugins in CI.
- **TBD:** a single `ascend-forge ci` command vs the explicit commands above.
- **TBD:** junit or other extra report formats beyond `--json`.

## Later transports

Same request, different carrier. Command sketches only.

- **TBD:** TUI. Interactive catalog, run, and inspect in the terminal.
- **TBD:** GUI. Launcher, scene switcher, and New Plugin menu call the same commands.
- **TBD:** REST. `POST /run` with the request-file body. `GET /query` with the same source and expression.
- **TBD:** which transport is allowed to prompt a person, and which must stay non-interactive.
