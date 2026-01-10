#!/usr/bin/env bash
# Script to flash ESP32 sniffer, read its MAC, optionally derive a PSK from the MAC, and register via HTTP POST
# Examples:
#   sudo ./flash.sh -p /dev/ttyUSB0 -f esp32-freeRTOS2.ino.merged.bin
#   sudo ./flash.sh -p /dev/ttyUSB0 -f esp32-freeRTOS2.ino.merged.bin \
#     -u https://api.example.com/devices/register -t YOUR_TOKEN \
#     --project-name MyProject --model-name esp32-sniffer --uid DEVICE_UID \
#     --protocol MQTT --name "My ESP32" --template-id 42 \
#     --psk-from-mac --psk-secret "MY_SERVER_SIDE_SECRET" --psk-format hex --psk-length 32

set -euo pipefail

filename="esp32-freeRTOS2.ino.merged.bin"
port="/dev/ttyUSB0"
register_url="https://devices.dev.inloc.cloud/api/devices"
token=""

# Payload fields
projectName="freeRTOS2"
templateId=""
modelName="sniffer-gw"
uid=""
name=""
protocol="MQTT"
psk=""

# PSK generation options
psk_from_mac=false        # if true, derive PSK from MAC (deterministic)
psk_secret="${PSK_SECRET:-}" # optional secret/salt; can also set via env PSK_SECRET
psk_format="hex"          # hex | base64
psk_length=32             # number of characters in final PSK (trimmed)

usage() {
  cat <<EOF
Usage: sudo $0 [options]

Flash options:
  -p, --port           Serial port for ESP32 (default: ${port})
  -f, --filename       Firmware image in current directory (default: ${filename})

Registration options:
  -u, --register-url   API URL to register device (HTTP POST)
  -t, --token          Token to include in registration POST body

Payload fields (added to JSON body):
  --project-name       (required when registering)
  --template-id        (optional, number)
  --model-name         (required when registering)
  --uid                (required when registering)
  --name               (optional)
  --protocol           (required when registering) one of: MQTT, LwM2M, mqtt, lwm2m
  --psk                (optional) pre-shared key; overrides generated PSK

PSK generation (from MAC):
  --psk-from-mac       Enable deriving PSK deterministically from MAC
  --psk-secret VALUE   Secret/salt for HMAC-SHA256 (recommended). Or set env PSK_SECRET.
  --psk-format FMT     'hex' (default) or 'base64'
  --psk-length N       Trim PSK to N characters (default: 32)

Notes:
  - Requires: esptool.py, curl, jq, openssl
  - If --psk is not given and --psk-from-mac is set, PSK = HMAC-SHA256(secret, normalized_mac)
    formatted as hex/base64 and trimmed to --psk-length.
  - If --psk-from-mac is set without --psk-secret, a warning is shown and an unsalted SHA256 is used.
EOF
}

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || { echo "Error: '$1' not found in PATH"; exit 1; }
}

# Parse args
while [ "$#" -gt 0 ]; do
  case "$1" in
    -h|--help) usage; exit 0;;
    -p|--port) port="${2:-}"; echo "port set: ${port}"; shift 2;;
    -f|--filename) filename="${2:-}"; echo "filename: ${filename}"; shift 2;;
    -u|--register-url) register_url="${2:-}"; echo "register URL: ${register_url}"; shift 2;;
    -t|--token) token="${2:-}"; echo "token provided"; shift 2;;

    --project-name) projectName="${2:-}"; shift 2;;
    --template-id) templateId="${2:-}"; shift 2;;
    --model-name) modelName="${2:-}"; shift 2;;
    --uid) uid="${2:-}"; shift 2;;
    --name) name="${2:-}"; shift 2;;
    --protocol) protocol="${2:-}"; shift 2;;
    --psk) psk="${2:-}"; shift 2;;

    --psk-from-mac) psk_from_mac=true; shift 1;;
    --psk-secret) psk_secret="${2:-}"; shift 2;;
    --psk-format) psk_format="${2:-}"; shift 2;;
    --psk-length) psk_length="${2:-}"; shift 2;;

    *) echo "Unknown parameter: $1"; usage; exit 1;;
  esac
done

# Requirements
require_cmd esptool.py
require_cmd curl
require_cmd jq
require_cmd openssl

[ -f "$filename" ] || { echo "Error: File '$filename' not found"; exit 1; }
[ -c "$port" ] || echo "Warning: Port '$port' not found as a character device (continuing anyway)"

# Normalize and validate protocol if provided
if [ -n "$protocol" ]; then
  lower_proto="$(printf '%s' "$protocol" | tr '[:upper:]' '[:lower:]')"
  case "$lower_proto" in
    mqtt) protocol="MQTT" ;;
    lwm2m) protocol="LwM2M" ;;
    *) echo "Error: --protocol must be MQTT or LwM2M"; exit 1 ;;
  esac
fi

echo "Step 1/5: Reading ESP32 MAC..."
MAC_OUTPUT="$(sudo esptool.py --port "${port}" read_mac 2>&1 || true)"
MAC="$(printf '%s\n' "$MAC_OUTPUT" | grep -oE '([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}' | head -n1 || true)"
# Normalize MAC: lowercase, remove colons (aabbccddeeff)
MAC_NORM="$(printf '%s' "$MAC" | tr '[:upper:]' '[:lower:]' | tr -d ':')"
if [ -z "${MAC}" ]; then
  echo "Error: Could not parse MAC from esptool output:"
  echo "$MAC_OUTPUT"
  exit 1
fi
echo "ESP32 MAC: ${MAC}"

# Generate PSK from MAC if requested and not explicitly provided
generate_psk_from_mac() {
  local mac_norm="$1"     # e.g., AA:BB:CC:DD:EE:FF
  local secret="$2"      # may be empty
  local fmt="$3"         # hex|base64
  local length="$4"      # number of characters

  local out=""
  if [ -n "$secret" ]; then
    # HMAC-SHA256(MAC_NORM)
    if [ "$fmt" = "hex" ]; then
      out="$(printf '%s' "$mac_norm" | openssl dgst -sha256 -hmac "$secret" | awk '{print $2}')"
    else
      out="$(printf '%s' "$mac_norm" | openssl dgst -sha256 -hmac "$secret" -binary | base64 | tr -d '\n=')"
    fi
  else
    echo "Warning: --psk-from-mac used without --psk-secret. Using unsalted SHA256." >&2
    if [ "$fmt" = "hex" ]; then
      out="$(printf '%s' "$mac_norm" | openssl dgst -sha256 | awk '{print $2}')"
    else
      out="$(printf '%s' "$mac_norm" | openssl dgst -sha256 -binary | base64 | tr -d '\n=')"
    fi
  fi

  # Trim to requested length
  out="${out:0:$length}"
  printf '%s' "$out"
}

if $psk_from_mac && [ -z "$psk" ]; then
  echo "Deriving PSK from MAC (${psk_format}, length ${psk_length})..."
  psk="$(generate_psk_from_mac "$MAC_NORM" "$psk_secret" "$psk_format" "$psk_length")"
  echo "PSK generated."
fi

echo "Step 2/5: Erasing flash..."
sudo esptool.py --port "${port}" erase_flash

echo "Step 3/5: Writing firmware '${filename}' to 0x0 at 921600 baud..."
sudo esptool.py --port "${port}" --baud 921600 write_flash 0x0 "${filename}"

# Registration (optional)
if [ -n "${register_url}" ]; then
  echo "Step 4/5: Validating registration fields..."
  uid="$MAC_NORM"
  for req in projectName modelName uid protocol; do
    if [ -z "${!req}" ]; then
      echo "Error: --${req//_/-} is required when using --register-url"
      echo "Missing: $req"
      exit 1
    fi
  done

  # Build JSON payload
  payload="$(
    jq -n \
      --arg projectName "$projectName" \
      --arg modelName "$modelName" \
      --arg uid "$uid" \
      --arg protocol "$protocol" \
      --arg name "${name:-}" \
      --arg psk "${psk:-}" \
      --arg templateId "${templateId:-}" \
      '
      def maybe_num(k; v):
        if (v|length) > 0 and (v|test("^[0-9]+$")) then { (k): (v|tonumber) } else {} end;

      { uid: $uid }
      + { projectName: $projectName, modelName: $modelName, protocol: $protocol }
      + (if ($name|length) > 0 then { name: $name } else {} end)
      + (if ($psk|length) > 0 then { psk: $psk } else {} end)
      + maybe_num("templateId"; $templateId)
      '
  )"

  echo "Step 5/5: Registering device..."
  tmp_body="$(mktemp)"
  headers=( -H 'Content-Type: application/json' )
  [ -n "$token" ] && headers+=( -H "token:${token}" )

  http_code="$(
    curl -sS -X POST "${register_url}" \
      "${headers[@]}" \
      --data "${payload}" \
      -o "${tmp_body}" -w '%{http_code}' || echo "000"
  )"

  echo "url: ${register_url}"
  echo "headers: ${headers[@]}"
  echo "Response code: ${http_code}"
  echo "Response body:"
  cat "${tmp_body}" || true
  rm -f "${tmp_body}"

  if [[ "${http_code}" -lt 200 || "${http_code}" -ge 300 ]]; then
    echo "Registration failed (HTTP ${http_code})."
    exit 1
  else
    echo "Registration succeeded."
  fi
else
  echo "Registration skipped (provide --register-url to enable)."
fi

echo "Done."