# CLI

This is the terminal interface for Ascend Forge. You use it to write a plugin, to find and run a plugin, and to see what the host does while a plugin runs. The same actions are available later as a shared library, over the network, from a TUI, from a GUI, and from a REST API. This page is only the terminal.

In a terminal, a command with no required arguments uses professional defaults. It asks a question only when a choice has no safe default, and every question shows that default so you can press enter. Pass flags when you already know the answers. CI and agents pass flags. They are never asked a question.

A plugin is named the way Go names a module.

```text
./plugins/png-viewer
./plugins/png-viewer@v0.1.0
github.com/acme/forge-plugins/png-viewer@v0.3.1
https://github.com/acme/forge-plugins.git@v0.3.1
git@github.com:acme/forge-plugins.git@v0.3.1
```

A local path with no version is the working tree. A repo URL with no version is the latest semver tag. One repo may hold many plugins. The path after the repo names one. The git remote is the source of truth.

Each command below shows its pattern, then the invocations you will actually type. The [author](#author) path is next.

## Shared commands

These commands apply before any plugin work.

### See the application version

You want the version of the `ascend-forge` program itself, the same way `go version` reports the Go toolchain.

```text
ascend-forge version
```

```bash
ascend-forge version
```

### Read a result

Human output is short. `--json` prints one JSON document on stdout. Logs stay on stderr. Exit `0` is success. Exit `1` is a bad invocation or a broken plugin contract. Exit `2` means the plugin ran and failed. Flags override the config file. The config file overrides defaults.

```text
ascend-forge --json <command> [arguments]
ascend-forge --config <file> <command> [arguments]
```

```bash
ascend-forge --json test ./plugins/png-viewer
ascend-forge --config ./forge.json test ./plugins/png-viewer
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

The next section is the author, who creates a workspace and publishes a plugin from it.

## Author

You are writing a plugin. You bootstrap a workspace once, create a plugin inside it, change and test that plugin, set a version, and publish it.

### Bootstrap a workspace

You want a directory you can build and test immediately, before you have written a plugin of your own.

Bootstrap creates the workspace: host config, a `plugins/` directory, and one sample plugin whose test already passes. Create, in the next section, adds a further plugin to that workspace. You bootstrap once per workspace. You create once per plugin.

`init` has no required arguments. With no flags it uses the current directory, names the workspace after that directory, and adds a sample GUI plugin written in Lua.

```text
ascend-forge init [--name <name>] [--dir <directory>]
```

```bash
ascend-forge init
ascend-forge init --name png-tools --dir ./png-tools
```

```bash
cd png-tools
ascend-forge test
ascend-forge run --plugin ./plugins/hello
```

In a terminal, `ascend-forge init` opens a short wizard. Each answer already has the default above. `--name` and `--dir` skip the wizard.

### Create a plugin

You want to add one plugin of a known shape to the workspace you just bootstrapped.

`--kind` picks the shape. `--output` picks what publish will produce for that plugin.

| Flag | Values | What you get |
|---|---|---|
| `--kind` | `gui`, `cli`, `server` | A cross-platform window, a command-line tool, or a background service |
| `--lang` | `lua`, `c` | A safe Lua plugin, or a native C plugin |
| `--output` | `library`, `app` | A shared library the host loads, or a standalone program |

The default, used when you pass only `--name`, is `--kind gui --lang lua --output library`. In a terminal, `plugin new` with no flags opens a wizard with those defaults filled in.

`plugin new` writes the plugin, registers it, and leaves a passing test. `plugin validate` checks version, description, tags, group, dependencies, and entry points.

```text
ascend-forge plugin new --name <name> [--kind gui|cli|server] [--lang lua|c] [--output library|app]
ascend-forge plugin validate --plugin <plugin>
```

```bash
ascend-forge plugin new --name png-viewer
ascend-forge plugin new --name png-viewer --kind gui --lang lua --output library
ascend-forge plugin new --name thumb --kind cli --lang c --output app
ascend-forge plugin new --name watcher --kind server --lang c --output library
ascend-forge plugin validate --plugin ./plugins/png-viewer
```

### Add media

You have images, fonts, or other files the plugin needs at run time.

A plugin reads media in three places: a file on disk, one packed archive, or bytes embedded in the binary. Publish then produces either one self-contained binary, or, when the data is large, one packed archive in the spirit of a Doom WAD. `--key` encrypts embedded media with a symmetric key.

```text
ascend-forge plugin embed --plugin <plugin> --file <path> [--key <key-file>]
ascend-forge publish --plugin <plugin> [--pack binary|archive] [--key <key-file>]
```

```bash
ascend-forge plugin embed --plugin ./plugins/png-viewer --file ./assets/icon.png
ascend-forge plugin embed --plugin ./plugins/png-viewer --file ./assets/icon.png --key ./secret.key
ascend-forge publish --plugin ./plugins/png-viewer --pack binary --key ./secret.key
ascend-forge publish --plugin ./plugins/png-viewer --pack archive
```

`--pack binary` is the default. The running plugin still reads unpacked files from disk when the caller passes a path.

### Change, test, and debug

You changed the plugin and want to know if that change holds.

`test` follows `go test`. With no arguments it discovers and runs every test in the workspace. `-run` selects tests by name. `-list` prints matching test names and does not run them. `-v` prints each test as it runs. `./...` means every plugin.

```text
ascend-forge test [plugin|./...] [-run <name>] [-list <name>] [-v]
ascend-forge test new --plugin <plugin> --name <test-name>
```

```bash
ascend-forge test
ascend-forge test ./plugins/png-viewer
ascend-forge test ./plugins/png-viewer -run RejectsEmpty
ascend-forge test ./plugins/png-viewer -run RejectsEmpty -v
ascend-forge test ./plugins/png-viewer -list .
ascend-forge test ./...
ascend-forge test new --plugin ./plugins/png-viewer --name RejectsEmptyInput
```

`test` does not ask questions. Filtering is `-run`.

`run` tries the plugin. `inspect` prints its name, version, entry points, tags, group, dependencies, and source. `debug` runs it with symbols and stops on the first broken invariant. In a terminal, `run` with no `--plugin` asks which plugin to start, unless the workspace has only one. Pass `--plugin` and it does not ask.

```text
ascend-forge run --plugin <plugin> [--entry <entry>] [--args <plugin-arguments>...]
ascend-forge inspect --plugin <plugin>
ascend-forge debug --plugin <plugin> [--args <plugin-arguments>...]
```

```bash
ascend-forge run --plugin ./plugins/png-viewer --args sample.png --out thumb.png
ascend-forge run --plugin ./plugins/png-viewer --entry thumb --args sample.png
ascend-forge inspect --plugin ./plugins/png-viewer
ascend-forge debug --plugin ./plugins/png-viewer --args sample.png
```

### Set the version and publish

The plugin works. You want other people to fetch a specific version from your git remote.

`plugin version` writes the semver. `publish` tags that version and pushes the tag. It refuses a dirty tree, a version that is not semver, and a tag that already exists. `--pack` and `--key` are the media choices from the previous section.

```text
ascend-forge plugin version --plugin <plugin> --set <version>
ascend-forge publish --plugin <plugin> [--pack binary|archive] [--key <key-file>]
```

```bash
ascend-forge plugin version --plugin ./plugins/png-viewer --set 0.1.0
ascend-forge plugin validate --plugin ./plugins/png-viewer
ascend-forge test ./plugins/png-viewer
ascend-forge publish --plugin ./plugins/png-viewer --pack binary
```

The plugin is now a git tag other people can name. The next section is that person.

## Caller

You want a plugin someone else published, or a plugin that shipped with the host. You query, get, run, upgrade, and remove. While you were the author, `run` on `./plugins/png-viewer` was this same command.

### Query the catalog

You want to see what is available before you fetch or run anything.

No path lists plugins that shipped with the host. A directory lists that directory. A git URL lists that repo. No filter lists everything at that source. Filters are `name:`, `tag:`, `group:`, and `version:`. Several filters together mean all of them must match.

```text
ascend-forge query [directory|git-url] [filter]
```

```bash
ascend-forge query
ascend-forge query ./plugins
ascend-forge query ./plugins 'tag:image group:viewers'
ascend-forge query --json ./plugins 'name:png*'
ascend-forge query https://github.com/acme/forge-plugins.git
ascend-forge query git@github.com:acme/forge-plugins.git@v0.1.0 'group:viewers'
ascend-forge query github.com/acme/forge-plugins/png-viewer@v0.1.0
```

### Get a version

You found the plugin. You want that version registered locally.

```text
ascend-forge get <plugin>@v<version>
```

```bash
ascend-forge get github.com/acme/forge-plugins/png-viewer@v0.1.0
```

### Run a plugin

You want to invoke one plugin, or several, and pass the arguments that plugin defines.

`--args` stops host options. Arguments after it belong to the plugin, including arguments that start with `-`. `--entry` picks an entry point such as `thumb`. `--file` is how you name several plugins and give each its own arguments. In a terminal, `run` with no plugin asks which one to start. `--plugin` and `--file` skip that question.

```text
ascend-forge run --plugin <plugin>[@v<version>] [--entry <entry>] [--args <plugin-arguments>...]
ascend-forge run --file <request.json>
```

```bash
ascend-forge run --plugin ./plugins/png-viewer --args sample.png --out thumb.png
ascend-forge run --plugin github.com/acme/forge-plugins/png-viewer@v0.1.0 --entry thumb --args sample.png
ascend-forge run --file ./request.json
ascend-forge run --json --file ./request.json
```

```json
{
  "plugins": [
    {"plugin": "./plugins/decode", "entry": "main", "args": ["sample.png"]},
    {"plugin": "./plugins/view", "entry": "main", "args": []}
  ]
}
```

`--kind` from create tells the host whether this run is a window, a command-line tool, or a background service. The command stays `run`.

### Upgrade

A newer tag exists. You want the registered plugin to become that version.

```text
ascend-forge get <plugin>@v<version>
```

```bash
ascend-forge get github.com/acme/forge-plugins/png-viewer@v0.2.0
ascend-forge run --plugin github.com/acme/forge-plugins/png-viewer@v0.2.0 --args sample.png
```

### Remove a plugin

You want the plugin gone from this workspace. Remove deletes its source and its registration. It does not stop a run that is already active. That lifetime is the host's, in the next section.

```text
ascend-forge plugin remove --plugin <plugin>
```

```bash
ascend-forge plugin remove --plugin ./plugins/png-viewer
ascend-forge plugin remove --json --plugin ./plugins/png-viewer
```

## Host

You want to know what the host does with a plugin during the commands above. You do not type a separate lifetime command for a normal run.

Create and get register the plugin, so the catalog and its entry points can see it. `run` activates it. When `run` returns, the host deactivates it and destroys that run's scene state. The plugin stays registered. `plugin remove` is the caller's delete, and it is a different step.

```bash
ascend-forge plugin new --name png-viewer --kind gui --lang lua --output library
ascend-forge get github.com/acme/forge-plugins/png-viewer@v0.1.0
ascend-forge run --plugin ./plugins/png-viewer --args sample.png
ascend-forge plugin remove --plugin ./plugins/png-viewer
```

## CI

A job has no person and no wizard. Use the same commands, pass every choice as a flag, and add `--json`. A non-zero exit fails the job.

```bash
ascend-forge version
ascend-forge init --name png-tools --dir ./png-tools
ascend-forge plugin new --name png-viewer --kind gui --lang lua --output library
ascend-forge plugin validate --json --plugin ./plugins/png-viewer
ascend-forge test --json ./plugins/png-viewer -run RejectsEmpty -v
ascend-forge run --json --file ./request.json
ascend-forge query --json ./plugins 'tag:image'
```

```yaml
- run: ascend-forge plugin validate --json --plugin ./plugins/png-viewer
- run: ascend-forge test --json ./plugins/png-viewer
- run: ascend-forge run --json --file ./request.json
```

## Other ways to invoke the host

A shared library, a network caller, a TUI, a GUI, and a REST API carry the same plugin names, arguments, and results.

- Shared library: the program calls these steps in process.
- Network: the caller sends the same body as `--file`.
- TUI: catalog, run, and inspect in the terminal.
- GUI: the launcher, the scene switcher, and New Plugin call these steps. New Plugin is the wizard behind `plugin new`.
- REST: `POST /run` takes the request body. `GET /query` takes the same path and filter.
