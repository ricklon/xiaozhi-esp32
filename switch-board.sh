#!/bin/bash
# switch-board.sh — per-board build management for xiaozhi-esp32
#
# Each board gets its own build directory (build-<board>/), preserving
# compiled artifacts and sdkconfig so switching boards never throws away work.
# Board-specific sdkconfig.defaults are merged automatically — no manual
# sdkconfig patching required.
#
# Usage:
#   ./switch-board.sh <board>                 setup if needed, then build
#   ./switch-board.sh <board> setup           configure build dir from scratch
#   ./switch-board.sh <board> build           build (setup if needed)
#   ./switch-board.sh <board> flash [port]    build if needed, then flash
#   ./switch-board.sh <board> clean           remove board's build dir
#   ./switch-board.sh <board> status          show config and build state
#   ./switch-board.sh list                    show all boards and build status
#
# Environment:
#   PORT=<dev>   Serial port override (default: auto-detect /dev/ttyACM0)
#
# Examples:
#   ./switch-board.sh xiao-esp32-c3 flash
#   ./switch-board.sh xiao-esp32-c6 flash /dev/ttyACM1
#   ./switch-board.sh lilygo-t-display-s3 build
#   ./switch-board.sh list

set -e

BOARDS_DIR="main/boards"
IDF_EXPORT="${IDF_PATH:-$HOME/esp/esp-idf}/export.sh"

# Source ESP-IDF if idf.py not on PATH
if ! command -v idf.py &>/dev/null; then
    # shellcheck disable=SC1090
    . "$IDF_EXPORT" > /dev/null 2>&1
fi

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

die() { echo "Error: $*" >&2; exit 1; }

auto_port() {
    local p
    for p in /dev/ttyACM0 /dev/ttyACM1 /dev/ttyUSB0 /dev/ttyUSB1; do
        [ -e "$p" ] && echo "$p" && return
    done
    echo "/dev/ttyACM0"
}

board_dir() { echo "$BOARDS_DIR/$1"; }

build_dir() {
    # Replace / with - for nested board paths (e.g. waveshare/foo -> waveshare-foo)
    echo "build-$(echo "$1" | tr '/' '-')"
}

board_target() {
    local cfg="$(board_dir "$1")/config.json"
    [ -f "$cfg" ] || die "No config.json for board '$1'"
    python3 -c "import json; print(json.load(open('$cfg'))['target'])"
}

# Build a merged sdkconfig.defaults for this board and write it to the build dir.
# Merge order (later entries win):
#   1. sdkconfig.defaults           (project-wide base)
#   2. sdkconfig.defaults.<target>  (target-level tweaks, if present)
#   3. main/boards/<board>/sdkconfig.defaults  (board authority)
merge_defaults() {
    local board="$1" target="$2" bdir="$3"
    local merged="$bdir/.sdkconfig_defaults_merged"
    mkdir -p "$bdir"
    {
        [ -f "sdkconfig.defaults" ]                        && cat "sdkconfig.defaults"
        [ -f "sdkconfig.defaults.$target" ]                && cat "sdkconfig.defaults.$target"
        [ -f "$(board_dir "$board")/sdkconfig.defaults" ]  && cat "$(board_dir "$board")/sdkconfig.defaults"
    } > "$merged"
    echo "$merged"
}

# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

cmd_list() {
    echo "Available boards:"
    # Find all config.json files, handle nested dirs
    while IFS= read -r cfg; do
        local b bdir target built
        b="${cfg#$BOARDS_DIR/}"
        b="${b%/config.json}"
        bdir=$(build_dir "$b")
        target=$(python3 -c "import json; print(json.load(open('$cfg')).get('target','?'))" 2>/dev/null)
        if [ -f "$bdir/xiaozhi.bin" ]; then
            built="[built]"
        elif [ -d "$bdir" ]; then
            built="[configured]"
        else
            built=""
        fi
        printf "  %-42s %-12s %s\n" "$b" "($target)" "$built"
    done < <(find "$BOARDS_DIR" -name "config.json" | sort)
}

cmd_status() {
    local board="$1" port="$2"
    local bdir target
    bdir=$(build_dir "$board")
    target=$(board_target "$board")

    echo "Board:     $board"
    echo "Target:    $target"
    echo "Build dir: $bdir"
    echo "Port:      $port"

    if [ -f "$bdir/xiaozhi.bin" ]; then
        local size
        size=$(du -h "$bdir/xiaozhi.bin" | cut -f1)
        echo "Status:    built ($size)"
    elif [ -d "$bdir" ]; then
        echo "Status:    configured (not built)"
    else
        echo "Status:    not configured"
    fi

    if [ -f "$bdir/sdkconfig" ]; then
        echo ""
        echo "Key settings:"
        grep -E "^CONFIG_(IDF_TARGET|BOARD_TYPE_[A-Z0-9_]+=y|ESPTOOLPY_FLASHSIZE=\"|LANGUAGE_[A-Z_]+=y|OTA_URL=)" \
            "$bdir/sdkconfig" 2>/dev/null | grep -v "is not set" | sed 's/^/  /'
    fi
}

cmd_setup() {
    local board="$1"
    local bdir target merged current_target

    bdir=$(build_dir "$board")
    target=$(board_target "$board")

    echo "Setting up $board (target: $target, build dir: $bdir)"

    # If build dir exists with a different target, wipe it
    if [ -f "$bdir/CMakeCache.txt" ]; then
        current_target=$(grep "^IDF_TARGET:STRING=" "$bdir/CMakeCache.txt" 2>/dev/null | cut -d= -f2 || true)
        if [ -n "$current_target" ] && [ "$current_target" != "$target" ]; then
            echo "Target changed ($current_target -> $target), cleaning $bdir..."
            rm -rf "$bdir"
        fi
    fi

    merged=$(merge_defaults "$board" "$target" "$bdir")
    idf.py -B "$bdir" -DSDKCONFIG_DEFAULTS="$merged" set-target "$target"
    echo "Done — $bdir is ready."
}

cmd_build() {
    local board="$1"
    local bdir
    bdir=$(build_dir "$board")

    if [ ! -f "$bdir/CMakeCache.txt" ]; then
        cmd_setup "$board"
    fi

    echo "Building $board..."
    idf.py -B "$bdir" build
}

cmd_flash() {
    local board="$1" port="$2"
    local bdir
    bdir=$(build_dir "$board")

    if [ ! -f "$bdir/xiaozhi.bin" ]; then
        cmd_build "$board"
    fi

    echo "Flashing $board to $port..."
    idf.py -B "$bdir" -p "$port" flash
}

cmd_clean() {
    local board="$1"
    local bdir
    bdir=$(build_dir "$board")
    echo "Removing $bdir"
    rm -rf "$bdir"
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

board="$1"
cmd="${2:-build}"
port="${PORT:-$(auto_port)}"

# Allow port as third arg: ./switch-board.sh <board> flash /dev/ttyACM1
[ -n "$3" ] && port="$3"

if [ -z "$board" ] || [ "$board" = "-h" ] || [ "$board" = "--help" ] || [ "$board" = "help" ]; then
    head -20 "$0" | grep "^#" | sed 's/^# \{0,1\}//'
    exit 0
fi

if [ "$board" = "list" ]; then
    cmd_list
    exit 0
fi

[ -d "$(board_dir "$board")" ] || die "Board '$board' not found. Run: ./switch-board.sh list"

case "$cmd" in
    setup)   cmd_setup  "$board" ;;
    build)   cmd_build  "$board" ;;
    flash)   cmd_flash  "$board" "$port" ;;
    clean)   cmd_clean  "$board" ;;
    status)  cmd_status "$board" "$port" ;;
    *)       die "Unknown command '$cmd'. Use: setup build flash clean status list" ;;
esac
