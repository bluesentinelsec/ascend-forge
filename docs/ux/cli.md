# CLI

The `ascend-forge` command is one way to invoke the host. The host can also be invoked as a shared library, over the network, and later from a TUI, a GUI, or a REST API. Each way names one or more plugins, passes their arguments, and gets results back.

A plugin is named the way Go names a module: a local path, or the URL of the git repo that holds it. Add `@v1.2.3` to pin a version. A local path with no version is the working tree. A repo URL with no version is the latest semver tag. A repo may hold many plugins; the path after the repo names one. The git remote is the source of truth. A registry is optional.

```text
./plugins/png-viewer
./plugins/png-viewer@v0.1.0
github.com/acme/forge-plugins/png-viewer@v0.3.1
https://github.com/acme/forge-plugins.git@v0.3.1
git@github.com:acme/forge-plugins.git@v0.3.1
```

Three roles use these names. The [author](#author) writes and publishes a plugin. The [caller](#caller) finds and runs plugins. The [host](#host) registers, activates, deactivates, and destroys a plugin for the length of a run. CI runs the same commands without a prompt.

## Command shape

```text
ascend-forge [flags] <command> [plugin...] [-- plugin-args...]
```

- Flags come before the command. Plugin arguments come after `--`.
- Flags override the config file. The config file overrides defaults.
- `--json` prints one JSON document on stdout. Logs stay on stderr.
- Exit `0` means success. Exit `1` means a bad invocation or a broken contract. Exit `2` means the plugin ran and failed.

```bash
ascend-forge --config ./forge.json --json test ./plugins/png-viewer
ascend-forge run --entry thumb ./plugins/png-viewer -- sample.png
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

## Author

The author writes the plugin and publishes it. The steps below are the whole path.

### Bootstrap

`init` writes a workspace whose first test already passes.

```bash
ascend-forge init png-tools
cd png-tools
ascend-forge test
ascend-forge run ./plugins/hello
```

### Create

`plugin new` writes the plugin, registers it, and leaves a passing test. `plugin validate` checks version, description, tags, group, dependencies, and entry points.

```bash
ascend-forge plugin new png-viewer --lang lua
ascend-forge plugin new png-viewer --lang c
ascend-forge plugin validate ./plugins/png-viewer
```

Media ships with the plugin. A running plugin reads it from disk, from an archive, or embedded in the binary. Publish embeds it.

### Change

Edit the plugin, then test, run, inspect, and debug. Add a test when a behavior is missing.

```bash
ascend-forge test ./plugins/png-viewer
ascend-forge run ./plugins/png-viewer -- sample.png
ascend-forge inspect ./plugins/png-viewer
ascend-forge debug ./plugins/png-viewer -- sample.png
ascend-forge test new ./plugins/png-viewer RejectsEmptyInput
ascend-forge test ./plugins/png-viewer
```

- `inspect` prints name, version, entry points, tags, group, dependencies, and source.
- `debug` runs the same plugin as `run`, with symbols, and stops on the first broken invariant.
- `test new` adds one named test next to the plugin.

### Version and publish

Set the version, then publish. Publish pushes that semver tag to the plugin's own git remote. It refuses a dirty tree, a version that is not semver, and a tag that already exists.

```bash
ascend-forge plugin version ./plugins/png-viewer 0.1.0
ascend-forge plugin validate ./plugins/png-viewer
ascend-forge test ./plugins/png-viewer
ascend-forge publish ./plugins/png-viewer
```

## Caller

The caller finds plugins and invokes them. The author is a caller while trying the local plugin above.

### Query

`query` reads a catalog. With no path, it lists plugins shipped with the host. A path lists that directory or that git repo. No filter lists everything there. Filters are `name:`, `tag:`, `group:`, and `version:`. Several filters are AND.

```bash
ascend-forge query
ascend-forge query ./plugins
ascend-forge query ./plugins 'tag:image group:viewers'
ascend-forge query --json ./plugins 'name:png*'
ascend-forge query https://github.com/acme/forge-plugins.git
ascend-forge query git@github.com:acme/forge-plugins.git@v0.1.0 'group:viewers'
ascend-forge query github.com/acme/forge-plugins/png-viewer@v0.1.0
```

### Get

`get` fetches a version and registers it.

```bash
ascend-forge get github.com/acme/forge-plugins/png-viewer@v0.1.0
```

### Run

`run` passes arbitrary plugin arguments after `--`. One `run` can name several plugins. A file is the precise form when each plugin has its own arguments.

```bash
ascend-forge run ./plugins/png-viewer -- sample.png --out thumb.png
ascend-forge run github.com/acme/forge-plugins/png-viewer@v0.1.0 -- sample.png
ascend-forge run --file request.json
ascend-forge run --json --file request.json
```

```json
{
  "plugins": [
    {"plugin": "./plugins/decode", "entry": "main", "args": ["sample.png"]},
    {"plugin": "./plugins/view", "entry": "main", "args": []}
  ]
}
```

A plugin may be a one-shot tool, a long-running process, a scene, or a game. `run` is the same command. The plugin's contract says which.

### Upgrade

Get the newer version. The newer get replaces the registered version.

```bash
ascend-forge get github.com/acme/forge-plugins/png-viewer@v0.2.0
ascend-forge run github.com/acme/forge-plugins/png-viewer@v0.2.0 -- sample.png
```

### Remove

Remove deletes the plugin source and its registration. It does not deactivate a run. See [Host](#host).

```bash
ascend-forge plugin remove ./plugins/png-viewer
ascend-forge plugin remove --json ./plugins/png-viewer
```

## Host

The host owns the plugin for the length of a run. Create and get register it. `run` activates it. When `run` returns, the host deactivates it and destroys that run's scene state. The plugin stays registered. Remove is the caller's delete, and it is a different step.

```bash
ascend-forge plugin new png-viewer --lang lua
ascend-forge get github.com/acme/forge-plugins/png-viewer@v0.1.0
ascend-forge run ./plugins/png-viewer -- sample.png
ascend-forge plugin remove ./plugins/png-viewer
```

## CI

CI uses the author and caller commands as they are. Pass `--json`. Do not prompt. A non-zero exit fails the job.

```bash
ascend-forge plugin validate --json ./plugins/png-viewer
ascend-forge test --json ./plugins/png-viewer
ascend-forge run --json --file request.json
ascend-forge query --json ./plugins 'tag:image'
```

```yaml
- run: ascend-forge plugin validate --json ./plugins/png-viewer
- run: ascend-forge test --json ./plugins/png-viewer
- run: ascend-forge run --json --file request.json
```

## Other ways to invoke the host

These carry the same plugin names, arguments, and results.

- Shared library: a program links the host and calls the author, caller, and host steps in process.
- Network: a remote caller sends the same request the `--file` body carries.
- TUI: an interactive catalog, run, and inspect in the terminal.
- GUI: the launcher, scene switcher, and New Plugin menu call these same steps.
- REST: `POST /run` takes the request body. `GET /query` takes the same path and filter.
