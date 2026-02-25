# Deploy Agent (Polling + systemd)

This folder contains a polling-based deployment agent for Linux servers.

It does the following:

1. Polls GitHub Releases for the latest published release.
2. Finds the correct binary asset for server architecture.
3. Downloads binary archive and checksum.
4. Verifies checksum.
5. Deploys release into versioned folders.
6. Updates `/opt/palette/current` symlink.
7. Installs bundled `libtopgg.so*` from the release `lib/` folder into `/lib` (configurable).
8. Restarts app via systemd service.
9. Rolls back symlink if restart fails.

## Files

- `deploy-agent.sh`: one-shot polling/deploy script (called by systemd service).
- `install.sh`: installer that copies scripts, units, and default env files.
- `deploy-agent.env.example`: deploy agent config.
- `palette.env.example`: app runtime env (loaded by `palette-app.service`).
- `systemd/palette-app.service`: runs deployed binary.
- `systemd/palette-deploy-agent.service`: one-shot poll/deploy job.
- `systemd/palette-deploy-agent.timer`: periodic trigger for poll job.

## Prerequisites

Install runtime tools on server:

```bash
sudo apt-get update
sudo apt-get install -y curl jq tar coreutils
```

## Install

From repo root:

```bash
sudo bash deploy-agent/install.sh
```

This installs:

- Script to `/opt/palette/deploy-agent/deploy-agent.sh`
- Unit files to `/etc/systemd/system/`
- Config templates to `/etc/palette/`

## Configure

1. Edit deploy-agent config:

```bash
sudo nano /etc/palette/deploy-agent.env
```

Set at minimum:

- `GITHUB_OWNER`
- `GITHUB_REPO`

Set optional:

- `GITHUB_TOKEN` (recommended for private repos/rate limits)
- path overrides and service name
- `SYSTEM_LIB_DIR` (defaults to `/lib`, target directory for bundled `libtopgg.so*`)

2. Edit app env file:

```bash
sudo nano /etc/palette/palette.env
```

Set runtime vars such as:

- `BOT_ENV`
- `DISCORD_TOKEN_PRODUCTION`
- `DISCORD_TOKEN_DEVELOPMENT`
- `BOT_WORKER_THREADS`

## Run and Verify

Trigger immediate deploy check:

```bash
sudo systemctl start palette-deploy-agent.service
```

Check status:

```bash
sudo systemctl status palette-deploy-agent.service
sudo systemctl status palette-app.service
sudo systemctl status palette-deploy-agent.timer
```

View logs:

```bash
sudo journalctl -u palette-deploy-agent.service -f
sudo journalctl -u palette-app.service -f
```

## Deploy Layout

- Releases: `/opt/palette/releases/<tag>`
- Current symlink: `/opt/palette/current`
- State: `/var/lib/palette-deploy-agent/current_tag`
- Bundled library install target: `/lib` (override with `SYSTEM_LIB_DIR`)

Release archive contents (example):

- `palette`
- `lib/libtopgg.so*`
- `LICENSE`

## Notes

- App service reads env from `/etc/palette/palette.env`.
- Deploy agent reads env from `/etc/palette/deploy-agent.env`.
- Deploy agent copies bundled `libtopgg.so*` from the extracted release `lib/` folder into `SYSTEM_LIB_DIR` before restarting the service.
- Timer default interval is every 24 hours.
