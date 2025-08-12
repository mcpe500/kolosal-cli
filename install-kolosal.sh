#!/usr/bin/env bash
set -euo pipefail

VERSION="1.0.0"
RELEASE_TAG="v0.1-pre"

URL_WIN="https://github.com/KolosalAI/kolosal-cli/releases/download/${RELEASE_TAG}/kolosal-1.0.0-win64.exe"
URL_DEB="https://github.com/KolosalAI/kolosal-cli/releases/download/${RELEASE_TAG}/kolosal_1.0.0_amd64.deb"
URL_DMG_ARM64="https://github.com/KolosalAI/kolosal-cli/releases/download/${RELEASE_TAG}/kolosal_1.0.0_arm64.dmg"

SCRIPT_NAME="$(basename "$0")"
TMP_DIR="$(mktemp -d -t kolosal-install-XXXXXXXX)"
DOWNLOADED_FILE=""
MOUNT_POINT=""

cleanup() {
  local ec=$?
  if [[ -n "${MOUNT_POINT}" && -d "${MOUNT_POINT}" ]]; then
    hdiutil detach "${MOUNT_POINT}" >/dev/null 2>&1 || true
  fi
  [[ -n "${DOWNLOADED_FILE}" && -f "${DOWNLOADED_FILE}" ]] && rm -f "${DOWNLOADED_FILE}" || true
  [[ -d "${TMP_DIR}" ]] && rm -rf "${TMP_DIR}" || true
  trap - EXIT INT TERM
  exit $ec
}
trap cleanup EXIT INT TERM

need_cmd() { command -v "$1" >/dev/null 2>&1 || { echo "Error: required command '$1' not found." >&2; exit 1; }; }

ensure_sudo() {
  if [[ $EUID -ne 0 ]]; then
    if command -v sudo >/dev/null 2>&1; then
      sudo -v || { echo "Cannot obtain sudo privileges" >&2; exit 1; }
    else
      echo "This operation requires root privileges, and 'sudo' is not available." >&2
      exit 1
    fi
  fi
}

detect_os() {
  local uname_s
  uname_s="$(uname -s 2>/dev/null || echo unknown)"
  case "$uname_s" in
    Darwin) echo mac; return ;;
    Linux)
      if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        case "${ID,,}" in
          ubuntu|debian) echo deb; return ;;
        esac
        case " ${ID_LIKE:-} " in
          *debian*) echo deb; return ;;
        esac
      fi
      echo linux-unsupported; return ;;
    MINGW*|MSYS*|CYGWIN*) echo windows; return ;;
    *)
      if [[ "${OS:-}" == "Windows_NT" ]]; then
        echo windows; return
      fi
      echo unknown; return ;;
  esac
}

download() {
  local url=$1
  local out=$2
  if command -v curl >/dev/null 2>&1; then
    curl -L --fail --progress-bar "$url" -o "$out"
  elif command -v wget >/dev/null 2>&1; then
    wget -O "$out" "$url"
  else
    echo "Error: need either curl or wget to download files." >&2
    exit 1
  fi
}

install_mac() {
  need_cmd hdiutil
  local arch
  arch="$(uname -m)"
  if [[ "$arch" != "arm64" ]]; then
    echo "Warning: Provided DMG is arm64. You're on $arch. Installation may fail." >&2
  fi
  DOWNLOADED_FILE="${TMP_DIR}/kolosal-${VERSION}.dmg"
  echo "Downloading Kolosal (macOS) ..."
  download "$URL_DMG_ARM64" "$DOWNLOADED_FILE"
  echo "Mounting DMG..."
  # Prefer explicit mountpoint for reliability
  MOUNT_POINT="${TMP_DIR}/mnt"
  mkdir -p "$MOUNT_POINT"
  if ! hdiutil attach -nobrowse -noautoopen -readonly -mountpoint "$MOUNT_POINT" "$DOWNLOADED_FILE" >/dev/null 2>&1; then
    echo "Primary attach method failed, attempting fallback parsing method..." >&2
    local attach_output
    attach_output=$(hdiutil attach -nobrowse -noautoopen "$DOWNLOADED_FILE" 2>&1 || true)
    echo "--- hdiutil attach output (debug) ---" >&2
    echo "$attach_output" >&2
    echo "-------------------------------------" >&2
    # Try to extract any /Volumes path
    MOUNT_POINT=$(echo "$attach_output" | awk '/\/Volumes\// {for(i=1;i<=NF;i++){if($i ~ /\/Volumes\//){print $i}}}' | tail -1)
    if [[ -z "$MOUNT_POINT" || ! -d "$MOUNT_POINT" ]]; then
      # Last resort: query hdiutil info
      MOUNT_POINT=$(hdiutil info | awk -v dmg="$DOWNLOADED_FILE" 'tolower($0) ~ tolower(dmg){capture=1} capture && /\/Volumes\// {for(i=1;i<=NF;i++){if($i ~ /\/Volumes\//){print $i; exit}}}')
    fi
    if [[ -z "$MOUNT_POINT" || ! -d "$MOUNT_POINT" ]]; then
      echo "Failed to determine mount point" >&2
      exit 1
    fi
  fi
  echo "Mounted at $MOUNT_POINT"
  local app_path
  app_path=$(find "$MOUNT_POINT" -maxdepth 1 -name '*.app' -type d | head -1 || true)
  if [[ -z "$app_path" ]]; then
    echo "Could not locate .app bundle inside DMG." >&2
    exit 1
  fi
  local app_name
  app_name="$(basename "$app_path")"
  echo "Found application: $app_name"
  local target_app="/Applications/${app_name}"
  if [[ -d "$target_app" ]]; then
    echo "Removing existing $target_app"
    ensure_sudo
    sudo rm -rf "$target_app"
  fi
  echo "Copying to /Applications (may require sudo)..."
  if [[ -w /Applications ]]; then
    cp -R "$app_path" /Applications/
  else
    ensure_sudo
    sudo cp -R "$app_path" /Applications/
  fi
  echo "Detaching DMG..."
  hdiutil detach "$MOUNT_POINT" >/dev/null 2>&1 || true
  MOUNT_POINT=""
  local bin_target="/usr/local/bin/kolosal"
  local app_exec="/Applications/${app_name}/Contents/MacOS/kolosal"
  ensure_sudo
  if [[ -x "$app_exec" ]]; then
    echo "Creating symlink $bin_target -> $app_exec"
    sudo ln -sf "$app_exec" "$bin_target"
  else
    echo "Creating wrapper script $bin_target that opens the app (binary not found at expected path)."
    sudo tee "$bin_target" >/dev/null <<EOF
#!/usr/bin/env bash
open -a "${app_name%.app}" --args "$@"
EOF
    sudo chmod +x "$bin_target"
  fi
  echo "Launching application once for initial setup..."
  open -a "${app_name%.app}" || true
  echo "Kolosal installed. Test with: kolosal --help"
}

install_deb() {
  need_cmd dpkg
  DOWNLOADED_FILE="${TMP_DIR}/kolosal_${VERSION}_amd64.deb"
  echo "Downloading Kolosal (.deb)..."
  download "$URL_DEB" "$DOWNLOADED_FILE"
  ensure_sudo
  echo "Installing .deb package..."
  if command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update -y || true
    sudo apt-get install -y "./$(basename "$DOWNLOADED_FILE")" 2>/dev/null || {
      sudo dpkg -i "$DOWNLOADED_FILE" || true
      sudo apt-get -f install -y
    }
  else
    sudo dpkg -i "$DOWNLOADED_FILE" || {
      echo "dpkg reported issues. Resolve dependencies manually." >&2
    }
  fi
  echo "Kolosal installed. Test with: kolosal --help"
}

install_windows_note() {
  echo "Detected Windows environment (MSYS/Cygwin/Git Bash)." >&2
  echo "Automated silent install not guaranteed. Downloading installer..." >&2
  DOWNLOADED_FILE="${TMP_DIR}/kolosal-${VERSION}-win64.exe"
  download "$URL_WIN" "$DOWNLOADED_FILE"
  echo "Attempting to run installer (may prompt GUI)..."
  if command -v cmd.exe >/dev/null 2>&1; then
    cmd.exe /c start "KolosalInstaller" "$(printf '%s' "$DOWNLOADED_FILE" | sed 's|/|\\|g')" || true
    echo "If no silent mode, follow GUI and ensure kolosal is added to PATH."
  else
    echo "cmd.exe not available; run the installer manually: $DOWNLOADED_FILE" >&2
  fi
}

main() {
  if [[ ${1:-} == "--print-os" ]]; then
    detect_os; exit 0
  fi
  local os
  os=$(detect_os)
  case "$os" in
    mac) install_mac ;;
    deb) install_deb ;;
    windows) install_windows_note ;;
    linux-unsupported)
      echo "Unsupported Linux distribution. Only Debian/Ubuntu (.deb) installation automated." >&2
      exit 2 ;;
    unknown)
      echo "Could not determine OS. Exiting." >&2
      exit 2 ;;
  esac
}

main "$@"
# End of file
