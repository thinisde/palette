#!/usr/bin/env bash
set -euo pipefail

DEFAULT_AGENT_ENV_FILE="/etc/palette/deploy-agent.env"
AGENT_ENV_FILE="${DEPLOY_AGENT_ENV_FILE:-${DEFAULT_AGENT_ENV_FILE}}"

load_env_file() {
    local file="$1"
    if [[ -f "${file}" ]]; then
        set -a
        # shellcheck disable=SC1090
        source "${file}"
        set +a
    fi
}

load_env_file "${AGENT_ENV_FILE}"

: "${GITHUB_OWNER:?GITHUB_OWNER is required}"
: "${GITHUB_REPO:?GITHUB_REPO is required}"

INSTALL_ROOT="${INSTALL_ROOT:-/opt/palette}"
RELEASES_DIR="${RELEASES_DIR:-${INSTALL_ROOT}/releases}"
CURRENT_LINK="${CURRENT_LINK:-${INSTALL_ROOT}/current}"
STATE_DIR="${STATE_DIR:-/var/lib/palette-deploy-agent}"
SERVICE_NAME="${SERVICE_NAME:-palette-app.service}"
BINARY_NAME="${BINARY_NAME:-palette}"
APP_ENV_FILE="${APP_ENV_FILE:-/etc/palette/palette.env}"
SYSTEM_LIB_DIR="${SYSTEM_LIB_DIR:-/lib}"

mkdir -p "${INSTALL_ROOT}" "${RELEASES_DIR}" "${STATE_DIR}" "${SYSTEM_LIB_DIR}"

CURRENT_TAG_FILE="${STATE_DIR}/current_tag"

map_arch() {
    local raw_arch
    raw_arch="$(uname -m)"
    case "${raw_arch}" in
    x86_64) echo "amd64" ;;
    aarch64 | arm64) echo "arm64" ;;
    *)
        echo "${raw_arch}"
        ;;
    esac
}

http_header_args() {
    local -a headers
    headers=("-H" "Accept: application/vnd.github+json")
    if [[ -n "${GITHUB_TOKEN:-}" ]]; then
        headers+=("-H" "Authorization: Bearer ${GITHUB_TOKEN}")
    fi
    printf '%s\n' "${headers[@]}"
}

fetch_latest_release_json() {
    local api_url
    api_url="https://api.github.com/repos/${GITHUB_OWNER}/${GITHUB_REPO}/releases/latest"

    mapfile -t header_args < <(http_header_args)
    curl -fsSL "${header_args[@]}" "${api_url}"
}

download_file() {
    local url="$1"
    local out="$2"
    mapfile -t header_args < <(http_header_args)
    curl -fsSL "${header_args[@]}" -o "${out}" "${url}"
}

print_service_debug() {
    local unit="$1"
    echo "---- systemctl status ${unit} ----" >&2
    systemctl --no-pager --full status "${unit}" >&2 || true
    echo "---- journalctl ${unit} (last 80 lines) ----" >&2
    journalctl --no-pager -n 80 -u "${unit}" >&2 || true
}

latest_release_json="$(fetch_latest_release_json)"
latest_tag="$(jq -r '.tag_name // empty' <<<"${latest_release_json}")"
if [[ -z "${latest_tag}" ]]; then
    echo "No tag_name found in latest release." >&2
    exit 1
fi

if [[ -f "${CURRENT_TAG_FILE}" ]]; then
    current_tag="$(cat "${CURRENT_TAG_FILE}")"
    if [[ "${current_tag}" == "${latest_tag}" ]]; then
        echo "No new release. Current tag: ${current_tag}"
        exit 0
    fi
fi

arch="$(map_arch)"
version="${latest_tag#v}"
asset_name="${ASSET_NAME:-palette-${version}-linux-${arch}.tar.gz}"
checksum_name="${CHECKSUM_NAME:-${asset_name%.tar.gz}.sha256}"

asset_url="$(jq -r --arg name "${asset_name}" '.assets[] | select(.name == $name) | .browser_download_url' <<<"${latest_release_json}" | head -n1)"
if [[ -z "${asset_url}" ]]; then
    echo "Release asset not found: ${asset_name}" >&2
    exit 1
fi

checksum_url="$(jq -r --arg name "${checksum_name}" '.assets[] | select(.name == $name) | .browser_download_url' <<<"${latest_release_json}" | head -n1)"
if [[ -z "${checksum_url}" ]]; then
    echo "Checksum asset not found: ${checksum_name}" >&2
    exit 1
fi

tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

download_file "${asset_url}" "${tmp_dir}/${asset_name}"
download_file "${checksum_url}" "${tmp_dir}/${checksum_name}"

expected_sha="$(awk 'NR==1{print $1}' "${tmp_dir}/${checksum_name}")"
actual_sha="$(sha256sum "${tmp_dir}/${asset_name}" | awk '{print $1}')"
if [[ -z "${expected_sha}" || "${expected_sha}" != "${actual_sha}" ]]; then
    echo "Checksum verification failed for ${asset_name}" >&2
    exit 1
fi

release_dir="${RELEASES_DIR}/${latest_tag}"
rm -rf "${release_dir}"
mkdir -p "${release_dir}"
tar -xzf "${tmp_dir}/${asset_name}" -C "${release_dir}"

extracted_binary="$(find "${release_dir}" -type f -name "${BINARY_NAME}" | head -n1 || true)"
if [[ -z "${extracted_binary}" ]]; then
    echo "Could not find binary ${BINARY_NAME} in extracted release." >&2
    exit 1
fi

install -m 0755 "${extracted_binary}" "${release_dir}/${BINARY_NAME}"

# Install bundled libtopgg shared libraries into the system library path.
while IFS= read -r -d '' bundled_lib; do
    cp -a "${bundled_lib}" "${SYSTEM_LIB_DIR}/"
done < <(find "${release_dir}" -mindepth 2 -name 'libtopgg.so*' \( -type f -o -type l \) -print0)

# Keep an app env symlink inside each release for easier service introspection.
if [[ -f "${APP_ENV_FILE}" ]]; then
    ln -sfn "${APP_ENV_FILE}" "${release_dir}/.env"
fi

previous_target="$(readlink -f "${CURRENT_LINK}" || true)"
ln -sfn "${release_dir}" "${CURRENT_LINK}"

systemctl daemon-reload
if ! systemctl restart "${SERVICE_NAME}" || ! systemctl --quiet is-active "${SERVICE_NAME}"; then
    echo "Service restart failed for ${SERVICE_NAME}, attempting rollback." >&2
    print_service_debug "${SERVICE_NAME}"
    if [[ -n "${previous_target}" && -d "${previous_target}" ]]; then
        ln -sfn "${previous_target}" "${CURRENT_LINK}"
        systemctl daemon-reload
        systemctl restart "${SERVICE_NAME}" || true
        if ! systemctl --quiet is-active "${SERVICE_NAME}"; then
            echo "Rollback restart failed for ${SERVICE_NAME}." >&2
            print_service_debug "${SERVICE_NAME}"
        fi
    fi
    exit 1
fi

echo "${latest_tag}" > "${CURRENT_TAG_FILE}"
echo "Deployed ${latest_tag} successfully."
