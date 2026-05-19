#!/usr/bin/env bash
# Serve the web flasher locally. Run from the web-flasher/ directory.
# Usage: ./serve.sh [port]   (default port: 8080)
set -euo pipefail

PORT=${1:-8080}
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

LOCAL_IP=$(ip route get 1.1.1.1 2>/dev/null | grep -oP 'src \K\S+' | head -1 || echo "127.0.0.1")

echo "╔══════════════════════════════════════════════╗"
echo "║         XiaoZhi Web Flasher                  ║"
echo "╠══════════════════════════════════════════════╣"
printf "║  Local:   http://localhost:%-18s║\n" "$PORT"
printf "║  Network: http://%-27s║\n" "$LOCAL_IP:$PORT"
echo "╚══════════════════════════════════════════════╝"
echo ""
echo "Share the Network URL with attendees on the same WiFi."
echo "Press Ctrl-C to stop."
echo ""

cd "$SCRIPT_DIR"
python3 -m http.server "$PORT"
