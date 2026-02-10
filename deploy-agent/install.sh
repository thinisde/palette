#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

INSTALL_ROOT="${INSTALL_ROOT:-/opt/palette}"
AGENT_INSTALL_DIR="${AGENT_INSTALL_DIR:-${INSTALL_ROOT}/deploy-agent}"
SYSTEMD_DIR="${SYSTEMD_DIR:-/etc/systemd/system}"
CONFIG_DIR="${CONFIG_DIR:-/etc/palette}"

mkdir -p "${AGENT_INSTALL_DIR}" "${CONFIG_DIR}"

install -m 0755 "${SCRIPT_DIR}/deploy-agent.sh" "${AGENT_INSTALL_DIR}/deploy-agent.sh"

install -m 0644 "${SCRIPT_DIR}/systemd/palette-app.service" \
    "${SYSTEMD_DIR}/palette-app.service"
install -m 0644 "${SCRIPT_DIR}/systemd/palette-deploy-agent.service" \
    "${SYSTEMD_DIR}/palette-deploy-agent.service"
install -m 0644 "${SCRIPT_DIR}/systemd/palette-deploy-agent.timer" \
    "${SYSTEMD_DIR}/palette-deploy-agent.timer"

if [[ ! -f "${CONFIG_DIR}/deploy-agent.env" ]]; then
    install -m 0644 "${SCRIPT_DIR}/deploy-agent.env.example" \
        "${CONFIG_DIR}/deploy-agent.env"
fi

if [[ ! -f "${CONFIG_DIR}/palette.env" ]]; then
    install -m 0644 "${SCRIPT_DIR}/palette.env.example" \
        "${CONFIG_DIR}/palette.env"
fi

systemctl daemon-reload
systemctl enable palette-app.service
systemctl enable --now palette-deploy-agent.timer

echo "Deploy agent installed."
echo "Edit ${CONFIG_DIR}/deploy-agent.env and ${CONFIG_DIR}/palette.env as needed."
echo "Then run: systemctl start palette-deploy-agent.service"
