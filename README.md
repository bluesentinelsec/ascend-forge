# ascend-forge

Developer guide for this **cppboot** environment. This README orients you to
the build, test, and source-onboarding workflows — not product requirements.

Default library surface is the **version** component driven by the root **`VERSION`** file (single source of truth) with CLI `--version` / `-V`, unit tests, and a small benchmark. Bump `VERSION` only — CMake regenerates the version API. Add real features as new components under `src/<component>/`.


## Prerequisites

- CMake **3.20+** (3.28+ if this project was generated with C++20 modules)
- A **C17** compiler (GCC, Clang, or MSVC)
- GNU Make (or another make that understands this Makefile)
- **Ninja** is required when this project uses C++20 modules (`--with-modules`)
- Optional tools: `clang-format`, `doxygen`, an LSP client with clangd
- Network access on first configure (CMake **FetchContent** downloads pinned deps)


## Continuous integration

GitHub Actions workflows (default; disable with `cppboot --no-github-actions`):

| Workflow | File | Purpose |
|----------|------|---------|
| **CI** | `.github/workflows/ci.yml` | Ubuntu/macOS/Windows × Debug+Release, tests, benches, artifacts |
| **Sanitizers** | `.github/workflows/sanitizers.yml` | Linux ASan+UBSan build + `ctest` (failures fail the job) |
| **Release** | `.github/workflows/release.yml` | Tag `v*` or manual dispatch → notes + zip assets |
| **Android** | `.github/workflows/android.yml` | Prefab AAR build + verification + emulator device tests |
| **iOS** | `.github/workflows/ios.yml` | XCFramework build + verification + Simulator package tests |
| **Web** | `.github/workflows/web.yml` | wasm library + canvas demo build + browser tests in headless Chrome |

**CI** (each OS):

1. Configure and build **Debug** (Ninja)
2. Run unit tests (CTest) on Debug
3. Configure and build **Release**
4. Run unit tests on Release
5. Run Google Benchmark binaries under `build/release` (short min time)
6. Package and upload zip artifacts via `actions/upload-artifact` for **Debug**
   and **Release** on each OS

Artifact archive name shape:

```text
<app>-<os>-<cpu-arch>-<debug|release>-<version>.zip
```

Examples: `myproj-linux-x86_64-debug-0.1.0.zip`,
`myproj-macos-arm64-release-0.1.0.zip`,
`myproj-windows-x86_64-release-0.1.0.zip`.

Each zip contains the contents of `build/<config>/bin/` (app, tests, benches).
Version comes from the root **`VERSION`** file (also what `./<app> --version`
prints after configure).

**Sanitizers** (Ubuntu + Clang): configure with
`-DASCEND_FORGE_ENABLE_SANITIZERS=ON`, build, run tests with
`ASAN_OPTIONS` / `UBSAN_OPTIONS` set so findings abort (non-zero exit).

Locally on Linux: `make sanitizer` (same flags and env).

**Android** (`--with-android-ci`): builds the Debug and Release Prefab AARs and
the consumer test APK with Gradle, verifies the release AAR contents (ABIs,
headers, version), then runs the native device tests on an Android emulator
(`scripts/run_android_tests.sh`). The Release workflow builds the release AAR
the same way and attaches `<app>-android-release-<version>.aar` to the
GitHub Release alongside the zips.

**iOS** (`--with-ios-ci`): builds the Debug and Release XCFrameworks
(`scripts/build_ios_xcframework.sh`) and the consumer test apps, verifies the
packaged slices/headers/version, then runs the package tests in an iOS
Simulator (`scripts/run_ios_tests.sh`). The Release workflow builds the
release XCFramework the same way and attaches
`<app>-ios-xcframework-release-<version>.zip` to the GitHub Release.

**Web** (`--with-web-ci`): builds the wasm32 static library, the HTML5 canvas
demo, and the browser test page with Emscripten (Debug and Release), runs the
tests in headless Chrome via `emrun`, and packages the installed library with
the demo bundled. The Release workflow attaches
`<app>-web-wasm32-release-<version>.zip` to the GitHub Release.

### Creating a release

The root **`VERSION`** file is the only place to bump the package version.

1. Edit **`VERSION`** (e.g. `1.0.0`), commit, and merge to the default branch.
2. Either:
   - **Tag:** `git tag -a v1.0.0 -m v1.0.0 && git push origin v1.0.0`
     (tag **must** match `VERSION`), or
   - **Actions → Release → Run workflow** — leave the version input empty to
     use `VERSION`, or pass the same semver (mismatch fails the job).
3. The Release workflow builds **Release** on Linux/macOS/Windows, verifies the
   binary `--version` matches `VERSION`/tag, generates notes, and uploads zips
   named `<app>-<os>-<arch>-release-<version>.zip`.

Action pins use current stable majors (`actions/checkout@v7`,
`actions/upload-artifact@v7`, `actions/download-artifact@v7`,
`lukka/get-cmake@latest`, `softprops/action-gh-release@v3`).


## Android package (Prefab AAR)

The `android/` Gradle project (`cppboot --with-android-ci`) packages the C
library as an Android [Prefab](https://google.github.io/prefab/) AAR and hosts
the on-device test application.

Build (requires JDK 17; Gradle downloads the NDK 27.2.12479018 and its CMake
3.22.1 automatically):

```bash
./android/gradlew -p android :ascend_forge:assembleRelease
```

Output: `android/ascend_forge/build/outputs/aar/ascend_forge-release.aar` with
`libascend_forge.so` for `armeabi-v7a`, `arm64-v8a`, `x86_64`.
Android consumers add the AAR with `buildFeatures { prefab true }` and link
`ascend_forge::ascend_forge` via `find_package(ascend_forge REQUIRED CONFIG)`
(see `tests/android/CMakeLists.txt` for a working example).

Device tests build `android/test-app` against the AAR and run
`tests/android/android_test.c` on an emulator or attached device:

```bash
./android/gradlew -p android :test-app:assembleRelease
bash scripts/run_android_tests.sh   # needs adb + a running emulator/device
```

Notes:

- The Android application namespace defaults to **`com.example.ascendforge`** —
  rename it in `android/ascend_forge/build.gradle`,
  `android/test-app/build.gradle`, and the test sources before publishing.
- On Android the core library is always built **static** (even when the
  project was generated with `--shared`) and folded into one
  `libascend_forge.so`.
- Pinned toolchain: Gradle 8.10.2, Android Gradle Plugin
  8.7.3, NDK 27.2.12479018, compileSdk 35,
  minSdk 21.


## iOS package (XCFramework)

The iOS scripts (`cppboot --with-ios-ci`) package the C library as a static
XCFramework with device (arm64) and simulator (arm64/x86_64) slices, the
public headers, and the generated `version.h`.

Build on macOS with Xcode command line tools installed:

```bash
bash scripts/build_ios_xcframework.sh Release
```

Output: `build/ios/release/ascend-forge.xcframework` and the versioned archive
`build/ios/release/ascend-forge-ios-xcframework-release-<version>.zip`. The
script verifies the package (`scripts/verify_ios_xcframework.sh`) after
building. Deployment target defaults to iOS 13.0; override
with `ASCEND_FORGE_IOS_DEPLOYMENT_TARGET`.

Consume the XCFramework from an application's CMake build by linking the
`.xcframework` directory directly:

```cmake
target_link_libraries(app_native PRIVATE "/path/to/ascend-forge.xcframework")
```

Package tests live in `tests/ios/` and run inside an iOS Simulator:

```bash
bash scripts/build_ios_test_apps.sh build/ios/release/ascend-forge.xcframework Release
bash scripts/run_ios_tests.sh   # boots a temporary Simulator, polls os_log
```

Notes:

- The test app bundle identifier defaults to `com.example.ascendforge.test` —
  change it in `tests/ios/CMakeLists.txt` before shipping anything derived
  from it.
- On iOS the core library is always built **static** (even when the project
  was generated with `--shared`).
- Add project-specific package checks to `tests/ios/test_main.mm`. The
  desktop CTest binaries do not run on iOS.


## Web package (Emscripten / WebAssembly)

The web scaffold (`cppboot --with-web-ci`) targets **browser game
development**: `src/web/` holds an HTML5 canvas demo built around
`emscripten_set_main_loop` (the browser drives frame pacing via
requestAnimationFrame), rendering through an `EM_JS` canvas bridge, with a
fullscreen-canvas shell page (`src/web/shell.html`).

Build with the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
activated (`emcmake` on PATH):

```bash
emcmake cmake -S . -B build/web-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/web-release --parallel
emrun build/web-release/bin/ascend-forge.html   # serves + opens the demo
```

Grow it into a game:

- Put per-frame simulation in `frame` and rendering in the draw call —
  swap the canvas-2D `EM_JS` bridge for WebGL2 (`-sUSE_WEBGL2=1` is already
  linked) or SDL2 (`-sUSE_SDL=2`, see the commented block in
  `src/web/CMakeLists.txt`).
- Bundle assets into the virtual filesystem with `--preload-file
  assets@/assets` (commented in `src/web/CMakeLists.txt`), then read them
  with normal file I/O.
- Never block the main thread; the browser owns the loop.

Browser tests live in `tests/web/` (self-hosted C compiled to wasm) and run in
headless Chrome:

```bash
emrun --browser=google-chrome \
  --browser_args="--headless=new --no-sandbox --disable-gpu" \
  --kill_exit --timeout 120 build/web-release/bin/ascend_forge_web_test.html
```

The release package `ascend-forge-web-wasm32-release-<version>.zip` contains
the installed wasm32 static library + headers (consumable from any Emscripten
CMake build via `find_package`), the playable demo under `demo/`, and the
`EMSCRIPTEN_VERSION` it was built with. On the web the core library is always
built **static** (even when the project was generated with `--shared`).


## GitHub Codespaces (VS Code in the browser)

This repo includes a **Dev Container** (`.devcontainer/`) for
[GitHub Codespaces](https://github.com/features/codespaces) and local
VS Code Dev Containers.

### Open in Codespaces

1. Push the repo to GitHub (or use an existing remote).
2. On GitHub: **Code → Codespaces → Create codespace on &lt;branch&gt;**.
3. Wait for the container create hooks:
   - `onCreateCommand`: install Ninja, clang-format, Doxygen, ctags
   - `postCreateCommand`: `cmake --preset debug` + Debug build (FetchContent)
4. In the browser VS Code: **F5** (*Debug ascend-forge*), Testing view, or terminal
   `make test` / `./build/debug/bin/ascend-forge --version`.

### Local Dev Container

In desktop VS Code with the Dev Containers extension:
**Dev Containers: Reopen in Container**.

Disable with `cppboot --no-codespaces` when regenerating a project.


## Open in VS Code

1. Open this folder in VS Code (`code .`).
2. Install the **recommended extensions** when prompted
   (clangd, CMake Tools, CodeLLDB, **C++ TestMate**).
3. Install **Ninja** (`brew install ninja` / `apt install ninja-build`). VS Code
   presets and `make` both prefer Ninja so they share the same `build/debug` tree.
4. **Build once:** `Ctrl/Cmd+Shift+B` (*Build Debug*). This configures CMake,
   builds, and places `compile_commands.json` at the repo root for **clangd**.
5. If red squiggles remain: Command Palette → **clangd: Restart language server**
   (or reload the window). IntelliSense needs that compilation database.
6. **Debug the app:** F5 → *Debug ascend-forge* (rebuilds Debug first).
7. **Unit tests (per-test ▶):** open the **Testing** view (beaker icon).
   After a Debug build, CTest runs the self-hosted binaries under
   `build/debug/bin/`.
8. **Bulk tests:** Task *Test*, CMake Tools CTest, or `make test`.

Presets live in `CMakePresets.json` (`debug` → `build/debug`, `release` → `build/release`).

**Generator mismatch:** If you see *generator : Ninja Does not match ... Unix Makefiles*,
the build tree was configured with a different generator. Fix with:

```bash
make reconfigure-debug
# or: rm -rf build/debug && cmake --preset debug
```

Then F5 again. Prefer always using Ninja (install it; both `make` and VS Code will use it).

**IntelliSense note:** Disable Microsoft C/C++ IntelliSense (already set) and use
**clangd** only. Do not enable both. Project headers resolve
from `compile_commands.json` after the first successful configure+build.

**Tests note:** TestMate needs built test executables — Build Debug first.
Individual test debug uses CodeLLDB (`lldb`) on macOS/Linux; on Windows you may
set `testMate.cpp.debug.configTemplate.type` to `cppvsdbg`.

Windows: use the *Debug ascend-forge (Windows)* launch config (MSVC debugger).

## Dependencies

Default builds do not fetch CLI11, nlohmann JSON, spdlog, or GoogleTest. The
entrypoint is C and parses `-V` / `--version` itself. JSON is the `Json_`
primitive. Logging is `SDL_Log`. Tests are self-hosted.

## Layout

```text
ascend-forge/
  VERSION              # single source of truth for package version
  src/main.c           # THE program entrypoint (always here)
  src/<component>/     # library implementation; each dir has CMakeLists.txt
  include/             # public headers (classic layout only; version.h is generated)
  tests/<component>/   # self-hosted checks
  benchmarks/<component>/
  cmake/               # shared CMake modules + version.*.in templates
  CMakeLists.txt
  Makefile             # macOS / Linux day-to-day targets
  build.bat            # Windows day-to-day targets (mirrors Makefile)
```

This project uses **classic headers**. Public headers live under `include/ascend_forge/` (directory tree == namespace).

**Modules toolchain note:** CMake C++20 modules need **Ninja** (the Makefile
selects it automatically when `ninja` is on `PATH`), **CMake 3.28+**, and a
compiler with module dependency scanning (**Clang 16+**, **GCC 14+**, or
**MSVC 17.4+**). Stock **AppleClang** often cannot scan modules for CMake yet;
use a recent LLVM Clang/GCC/MSVC when building a modules project.

A project-local `.vimrc` was generated (default; disable with `cppboot --no-vim`). Enable `:set exrc` (and ideally `secure`) in your global Vim config to load it.

**VS Code** config is included (default; disable with `cppboot --no-vscode`): open the folder, install recommended extensions, select the **debug** CMake preset, then Build / F5 / Test. See [Open in VS Code](#open-in-vs-code).

**ctags:** `.ctags` is configured for Universal Ctags (default; `cppboot --no-ctags` to skip). Run `make tags` to build the `tags` index.

## Versioning

Package version lives in the root **`VERSION`** file (one line, e.g. `0.1.0`).

| Consumer | How it gets the version |
|----------|-------------------------|
| CMake `project(... VERSION ...)` | Reads `VERSION` at configure time |
| `Version_String()` / CLI `--version` | Generated from `cmake/version.h.in` and `cmake/version.c.in` |
| GitHub Release workflow | Requires tag / dispatch input to match `VERSION` |
| Doxygen `PROJECT_NUMBER` | Injected from `VERSION` by `make doc` / `build.bat doc` |

**To ship a new version:** edit `VERSION` only, commit, then tag `vX.Y.Z` (or
run the Release workflow). Do not hand-edit generated version sources under the
build tree.

## Using this library from another CMake project

The library target is **`ascend-forge::lib`** (also **`ascend-forge::ascend_forge`**).

When this repo is **not** the top-level CMake project (via `add_subdirectory` or
`FetchContent`), app/tests/benchmarks and optional app deps default **off** so
you only build the library.

### `add_subdirectory`

```cmake
add_subdirectory(path/to/ascend-forge)
target_link_libraries(my_app PRIVATE ascend-forge::lib)
```

### `FetchContent` (e.g. from GitHub)

```cmake
include(FetchContent)
FetchContent_Declare(
  ascend-forge
  GIT_REPOSITORY https://github.com/<org>/ascend-forge.git
  GIT_TAG        v0.1.0   # or main / a commit SHA
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(ascend-forge)
target_link_libraries(my_app PRIVATE ascend-forge::lib)
```

Optional cache variables (prefix `ASCEND_FORGE_`):

| Option | Default (top-level / embedded) | Meaning |
|--------|----------------------------------|---------|
| `ASCEND_FORGE_BUILD_APP` | ON / OFF | Demo executable |
| `ASCEND_FORGE_BUILD_TESTS` | ON / OFF | Self-hosted checks |
| `ASCEND_FORGE_BUILD_BENCHMARKS` | OFF | No C++ benchmark is fetched |

### `find_package` (after install)

```bash
cmake --install build/release --prefix /path/to/prefix
```

```cmake
find_package(ascend-forge REQUIRED CONFIG)
target_link_libraries(my_app PRIVATE ascend-forge::lib)
```

## Build

Out-of-source builds only. Artifacts land under `build/`.

| Command (Unix) | Command (Windows) | Meaning |
|----------------|-------------------|---------|
| `make` / `make debug` | `build.bat` / `build.bat debug` | Configure and build **Debug** |
| `make release` | `build.bat release` | Configure and build **Release** |
| `make test` | `build.bat test` | Unit tests (Debug) |
| `make bench` | `build.bat bench` | Microbenchmarks (Release) |
| `make fmt` | `build.bat fmt` | clang-format |
| `make doc` | `build.bat doc` | Doxygen HTML |
| `make clean` | `build.bat clean` | Remove build trees |

Debug tree: `build/debug`
Release tree: `build/release`

The Debug configure step links/copies `compile_commands.json` at the repo root for LSP.

### Windows

Prefer **`build.bat`** (same targets as the Makefile). Open a **Developer
Command Prompt for VS** or ensure `cmake` (and ideally `ninja`) are on `PATH`:

```bat
build.bat
build.bat test
build.bat release
build.bat help
```

WSL / MSYS2 can keep using `make` if you prefer.

## Run the app

The program entrypoint is always **`src/main.c`**.
`make` links **`./ascend-forge`** at the project root to `build/debug/bin/ascend-forge`.

After `make debug`:

```bash
./ascend-forge
./ascend-forge --version
./ascend-forge -V
```

## Tests

```bash
make test
```

Runs CTest against the Debug build (includes version API tests).

## Benchmarks

```bash
make bench
```

Builds **Release**. The host does not fetch Google Benchmark, so this finds no benchmark binary and exits successfully.

## Format

```bash
make fmt
```

Runs **clang-format** with the checked-in `.clang-format` (**Microsoft** style).

**Formatting** uses Microsoft clang-format. Naming and design follow AGENTS.md.

## Documentation

```bash
make doc
```

Runs **Doxygen** using the checked-in `Doxyfile`. HTML output: `docs/html/`.

## Clean

```bash
make clean
```

Removes `build/`, generated docs, and the root `compile_commands.json` link.

## LSP / clangd

- CMake exports a compilation database (`CMAKE_EXPORT_COMPILE_COMMANDS=ON`).
- After `make debug`, `compile_commands.json` is available at the **project root**.
- Open the project root in your editor so clangd picks it up.
- See `.clangd` for the minimal clangd config.

## Compiler warnings

This project compiles with **warnings as errors** (`-Wall -Wextra -Wpedantic -Werror`
on GCC/Clang; `/W4 /WX` on MSVC). Fix warnings instead of silencing them.

## Onboarding new source files

Sources are organized by **logical component**. Each component directory has its
own `CMakeLists.txt` and **lists files explicitly** (no `file(GLOB)`).

### Example: add a `parser` component

1. **Public header** (classic layout):

   `include/ascend_forge/parser.h`

2. **Implementation**:

   ```text
   src/parser/
     CMakeLists.txt
     parser.c
   ```

3. **`src/parser/CMakeLists.txt`**:

   ```cmake
   target_sources(${PROJECT_NAME}_lib
     PRIVATE
       parser.c
   )
   ```

4. **Register the component** in `src/CMakeLists.txt`:

   ```cmake
   add_subdirectory(version)
   add_subdirectory(parser)
   ```

5. **Tests** (same pattern):

   ```text
   tests/parser/
     CMakeLists.txt
     parser_test.c
   ```

   And `add_subdirectory(parser)` in `tests/CMakeLists.txt`.

### Rules of thumb

- The only program entrypoint is **`src/main.c`**. Keep it thin; put logic in library components.
- Library code goes under `src/<component>/`, never into `src/main.c` beyond startup wiring.
- List every `.c` file in the component `CMakeLists.txt`.
- Mirror component names under `tests/` and `benchmarks/`.
- Do not rely on directory globs for sources — explicit lists keep reviews and CI unambiguous.

## Agent / contributor conventions

See **[AGENTS.md](AGENTS.md)** for coding standards, documentation rules, and
how automated agents (and humans) should work in this repository.

## License

See `LICENSE` (apache-2.0).
