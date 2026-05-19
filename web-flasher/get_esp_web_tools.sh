#!/usr/bin/env bash
# Downloads esp-web-tools and all its dynamic chunk imports for fully offline use.
# Run once before the event if the venue may not have reliable internet.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
VENDOR="$SCRIPT_DIR/vendor"
mkdir -p "$VENDOR"

BASE="https://unpkg.com/esp-web-tools@10/dist/web"
ENTRY_REMOTE="install-button.js"
ENTRY_LOCAL="esp-web-tools.js"   # matches the <script src> in index.html

echo "Downloading esp-web-tools entry point..."
curl -fsSL "$BASE/$ENTRY_REMOTE?module" -o "$VENDOR/$ENTRY_LOCAL"

echo "Scanning for dynamic chunk imports..."
CHUNKS=$(grep -oP "(?<=['\"])\./[^'\"?]+(?=[?'\"])" "$VENDOR/$ENTRY_LOCAL" || true)

for chunk in $CHUNKS; do
  name="${chunk#./}"
  name="${name%%\?*}"
  if [ ! -f "$VENDOR/$name" ]; then
    echo "  downloading $name..."
    curl -fsSL "$BASE/$name?module" -o "$VENDOR/$name"
    SUB=$(grep -oP "(?<=['\"])\./[^'\"?]+(?=[?'\"])" "$VENDOR/$name" || true)
    for sub in $SUB; do
      sname="${sub#./}"
      sname="${sname%%\?*}"
      if [ ! -f "$VENDOR/$sname" ]; then
        echo "    downloading sub-chunk $sname..."
        curl -fsSL "$BASE/$sname?module" -o "$VENDOR/$sname" || true
      fi
    done
  fi
done

# Remove stale install-button.js if it exists (was downloaded by an old version of this script)
rm -f "$VENDOR/install-button.js"

echo ""
echo "Done. Files in vendor/:"
ls -lh "$VENDOR"/*.js
