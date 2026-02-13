#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

# Load .env if it exists
if [[ -f .env ]]; then
    set -a
    source .env
    set +a
fi

echo "[run.sh] DISCORD_TOKEN is: ${DISCORD_TOKEN:+SET} ${DISCORD_TOKEN:-NOT SET}"
echo "[run.sh] Running: ./build/palette"
exec ./build/palette
