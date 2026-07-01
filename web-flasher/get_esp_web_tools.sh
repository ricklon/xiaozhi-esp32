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
queue=("$ENTRY_LOCAL")
seen=""

while [ "${#queue[@]}" -gt 0 ]; do
  current="${queue[0]}"
  queue=("${queue[@]:1}")

  if [[ "$seen" == *"|$current|"* ]]; then
    continue
  fi
  seen="$seen|$current|"

  while IFS= read -r chunk; do
    name="${chunk#./}"
    name="${name%%\?*}"
    if [ -z "$name" ]; then
      continue
    fi

    if [ ! -f "$VENDOR/$name" ]; then
      echo "  downloading $name..."
      curl -fsSL "$BASE/$name?module" -o "$VENDOR/$name"
    fi

    queue+=("$name")
  done < <(python3 -c 'import re, sys
from pathlib import Path
text = Path(sys.argv[1]).read_text(encoding="utf-8")
for match in re.findall(r"""['\''"](\./[^'\''"?]+)(?:\?module)?['\''"]""", text):
    print(match)' "$VENDOR/$current")
done

# Remove stale install-button.js if it exists (was downloaded by an old version of this script)
rm -f "$VENDOR/install-button.js"

echo ""
echo "Done. Files in vendor/:"
ls -lh "$VENDOR"/*.js
