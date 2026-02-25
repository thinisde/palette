# Palette

`Palette` is a Discord bot (D++) for color analysis and palette generation. It supports multiple color models, produces PNG palette images, and includes release/deployment automation for server operations.

## Features

- Multi-format color input parsing: `hex`, `rgb`, `hsl`, `cmyk`.
- Palette image generation (`800x600`) with numbered swatches when needed.
- Interactive shade/tint controls using Discord buttons (`+1` / `-1`) that update the original message.
- Color theory commands: complementary, split complementary, monochrome/scheme generation.
- Accessibility tooling: WCAG contrast checks against black/white backgrounds with generated proof image.
- Color utility tools: nearest web-safe comparison and color mixing.
- Environment-aware slash command registration (guild in development, global in production).
- Thread-pooled command dispatch for non-blocking bot interactions.

## Why It Is Special

- It has a custom in-process PNG renderer (`src/services/palette_image.cpp`) instead of relying on external imaging libraries at runtime. This reduces runtime dependency surface and keeps image output deterministic.
- It separates reusable color/domain logic from command handlers (`src/services/*` vs `src/commands/*`), so adding new commands is mostly orchestration instead of rewriting parsing/conversion code.
- It uses async Discord event handling plus a configurable worker pool (`BOT_WORKER_THREADS`) to keep command processing responsive under concurrent usage.
- It supports environment-gated command registration, which solves the common Discord global-command propagation delay problem in dev workflows.
- It includes release-to-runtime operational flow (GitHub Actions auto version/tag/package/release + optional checksum-verified deploy-agent with rollback).

## Commands

| Command               | What it does                                                         | Example                                  |
| --------------------- | -------------------------------------------------------------------- | ---------------------------------------- |
| `/color`              | Identifies a color and shows metadata from TheColorAPI.              | `/color hex:#24B1E0`                     |
| `/scheme`             | Builds a scheme from a seed (`mode`, `count`).                       | `/scheme hex:#24B1E0 mode:triad count:5` |
| `/complementary`      | Finds opposite color on wheel + palette image.                       | `/complementary rgb:0,71,171`            |
| `/splitcomplementary` | Returns base + two colors around complement.                         | `/splitcomplementary hsl:215,100%,34%`   |
| `/shades`             | Generates darkening steps to black (2-8). Supports multiple colors.  | `/shades amount:5 hex:#FF0000;#00AAFF`   |
| `/tints`              | Generates lightening steps to white (2-8). Supports multiple colors. | `/tints amount:5 rgb:255,0,0;0,128,255`  |
| `/websafe`            | Compares original color to nearest web-safe color.                   | `/websafe hex:#2D6CDF`                   |
| `/contrast`           | WCAG contrast test against `black` or `white`.                       | `/contrast background:black hex:#80C342` |
| `/mix`                | Mixes up to 3 colors by averaging RGB channels.                      | `/mix hex:#FF0000;#0000FF`               |

Notes:

- Multi-color lists use `;` as separator.
- `shades`/`tints` allow up to 10 source colors per command.
- `mix` allows up to 3 source colors.

## Architecture

- `src/main.cpp`: bootstrapping, env loading, thread-pool sizing.
- `src/commands/*`: slash command handlers.
- `src/buttons/*`: interactive button handlers for shade/tint controls.
- `src/services/color_utils.cpp`: parsing, conversion, contrast, web-safe, mixing.
- `src/services/palette_image.cpp`: palette/text image rendering and PNG encoding.
- `src/services/palette_controls.cpp`: stateful shade/tint session tokens and control updates.
- `src/services/color_api.cpp`: TheColorAPI client wrapper.

## Configuration

The bot reads `.env` (via `load_dotenv_file()`), then resolves environment-specific token selection.

Required/important variables:

- `BOT_ENV=development|production`
- `DISCORD_TOKEN_DEVELOPMENT=...`
- `DISCORD_TOKEN_PRODUCTION=...`
- `DISCORD_TOKEN=...` (fallback if env-specific token is missing)
- `DISCORD_DEV_GUILD_ID=...` (recommended in development)
- `DISCORD_GUILD_ID=...` (alternative guild id key)
- `BOT_WORKER_THREADS=4`

## Build and Run (Local)

### 1) Install dependencies

You need:

- C++ toolchain with C++20 support
- `cmake` (>= 3.22)
- `ninja-build`
- `nlohmann-json3-dev`
- D++ (`libdpp`)

### 2) Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 3) Run

```bash
./build/palette
```

## Docker

```bash
docker compose up --build -d
```

`docker-compose.yml` currently pins `platform: linux/amd64`. If your host is ARM, either use emulation or update the image/dependency strategy for ARM-compatible D++ binaries.

## CI/CD and Releases

Workflow file: `.github/workflows/release.yml`

- Trigger: push to `main` where commit message starts with `[MERGE]` (or manual dispatch).
- Auto behavior: resolve next semver tag, create/push tag, build, package `palette-<version>-linux-<arch>.tar.gz`, generate `.sha256`, create GitHub Release.

## Deploy Agent (Optional)

Folder: `deploy-agent/`

It provides a polling deployment model on Linux/systemd:

- polls latest GitHub release on interval (timer-based)
- downloads release archive + checksum
- verifies checksum
- deploys version into `/opt/palette/releases/<tag>`
- flips `/opt/palette/current` symlink
- restarts `palette-app.service`
- rolls back on failed restart

See `deploy-agent/README.md` for server setup.
