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
#   AGENT_HUB_PUBLIC_HOST=<host>
#                Agent Hub Funnel host for df-k10
#   AGENT_HUB_SERVER_ENROLLMENT_TOKEN=<token>
#                Agent Hub enrollment token embedded in df-k10 firmware
#   AGENT_HUB_ENV_FILE=<path>
#                dotenv fallback (default: ../agent-hub/.env when present)
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

# Persistent cache for a board's merged sdkconfig.defaults. Lives OUTSIDE the
# build dir (so set-target never sees a pre-created non-CMake build dir and
# refuses to fullclean it) and outside /tmp (so the path CMake bakes into
# build.ninja still exists on later incremental builds). Doubles as the
# change-detection baseline. Gitignored via .switch-board/.
defaults_cache() {
    echo ".switch-board/$(echo "$1" | tr '/' '-').defaults"
}

board_target() {
    local cfg="$(board_dir "$1")/config.json"
    [ -f "$cfg" ] || die "No config.json for board '$1'"
    python3 -c "import json; print(json.load(open('$cfg'))['target'])"
}

# Build a merged sdkconfig.defaults for this board and write it to $outfile.
# Merge order (later entries win):
#   1. sdkconfig.defaults           (project-wide base)
#   2. sdkconfig.defaults.<target>  (target-level tweaks, if present)
#   3. main/boards/<board>/sdkconfig.defaults  (board authority)
#
# The setup path writes this to the persistent defaults_cache and passes that
# to -DSDKCONFIG_DEFAULTS, so CMake bakes a path that still exists on later
# incremental builds. (A /tmp mktemp path gets cleaned out from under us and
# breaks ninja's re-run of CMake.)
merge_defaults() {
    local board="$1" target="$2" outfile="$3"
    mkdir -p "$(dirname "$outfile")"
    {
        [ -f "sdkconfig.defaults" ]                        && cat "sdkconfig.defaults"
        [ -f "sdkconfig.defaults.$target" ]                && cat "sdkconfig.defaults.$target"
        [ -f "$(board_dir "$board")/sdkconfig.defaults" ]  && cat "$(board_dir "$board")/sdkconfig.defaults"
    } > "$outfile"

    # The K10 enrolls through Agent Hub's public HTTPS Funnel. Keep the secret
    # out of tracked defaults while allowing the same value used by Agent Hub's
    # .env to be injected into the firmware. The generated cache is gitignored.
    if [ "$board" = "df-k10" ]; then
        local agent_hub_env agent_hub_host agent_hub_token dotenv_key dotenv_value
        agent_hub_env="${AGENT_HUB_ENV_FILE:-../agent-hub/.env}"
        agent_hub_host="${AGENT_HUB_PUBLIC_HOST:-}"
        agent_hub_token="${AGENT_HUB_SERVER_ENROLLMENT_TOKEN:-}"

        # Parse only the two required dotenv assignments; do not source the
        # file, because dotenv files should never be executed as shell code.
        if [ -z "$agent_hub_token" ] && [ -f "$agent_hub_env" ]; then
            while IFS='=' read -r dotenv_key dotenv_value; do
                dotenv_value="${dotenv_value%$'\r'}"
                dotenv_value="${dotenv_value%\"}"
                dotenv_value="${dotenv_value#\"}"
                dotenv_value="${dotenv_value%\'}"
                dotenv_value="${dotenv_value#\'}"
                case "$dotenv_key" in
                    AGENT_HUB_PUBLIC_HOST) agent_hub_host="$dotenv_value" ;;
                    AGENT_HUB_SERVER_ENROLLMENT_TOKEN) agent_hub_token="$dotenv_value" ;;
                esac
            done < "$agent_hub_env"
        fi

        agent_hub_host="${agent_hub_host:-agent-hub.panthera-hamlet.ts.net}"

        [ -n "$agent_hub_token" ] || return 0

        [[ "$agent_hub_host" =~ ^[A-Za-z0-9.-]+$ ]] ||
            die "AGENT_HUB_PUBLIC_HOST must be a hostname without a scheme, path, or port"
        [[ "$agent_hub_token" =~ ^[A-Za-z0-9._~-]+$ ]] ||
            die "AGENT_HUB_SERVER_ENROLLMENT_TOKEN contains characters that require URL encoding"

        printf '\nCONFIG_OTA_URL="https://%s/xiaozhi/ota/?enrollment_token=%s"\n' \
            "$agent_hub_host" "$agent_hub_token" >> "$outfile"
    fi

    # A missing optional defaults file makes the final test in the group
    # return 1.  Under `set -e` that used to abort setup for boards that only
    # inherit project/target defaults, even though the merge succeeded.
    return 0
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
            "$bdir/sdkconfig" 2>/dev/null | grep -v "is not set" |
            sed -E 's/(enrollment_token=)[^"&]*/\1<redacted>/g; s/^/  /'
    fi
}

cmd_setup() {
    local board="$1"
    local bdir target current_target cache abs_cache

    bdir=$(build_dir "$board")
    cache=$(defaults_cache "$board")
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

    # Write the merged defaults to the persistent cache (outside the build dir)
    # and hand CMake an absolute path to it. set-target then creates the build
    # dir itself, and ninja's later CMake re-runs can always find the file.
    merge_defaults "$board" "$target" "$cache"
    abs_cache="$(cd "$(dirname "$cache")" && pwd)/$(basename "$cache")"
    idf.py -B "$bdir" -DSDKCONFIG="$bdir/sdkconfig" -DSDKCONFIG_DEFAULTS="$abs_cache" set-target "$target"
    echo "Done — $bdir is ready."
}

cmd_build() {
    local board="$1"
    local bdir target cache
    bdir=$(build_dir "$board")
    cache=$(defaults_cache "$board")
    target=$(board_target "$board")

    if [ ! -f "$bdir/CMakeCache.txt" ]; then
        cmd_setup "$board"
    else
        local probe
        probe=$(mktemp)
        merge_defaults "$board" "$target" "$probe"
        if [ ! -f "$cache" ] || ! cmp -s "$probe" "$cache"; then
            echo "Board defaults changed for $board, re-running setup..."
            cmd_setup "$board"
        fi
        rm -f "$probe"
    fi

    echo "Building $board..."
    idf.py -B "$bdir" -DSDKCONFIG="$bdir/sdkconfig" build
}

cmd_flash() {
    local board="$1" port="$2"
    local bdir
    bdir=$(build_dir "$board")

    # Always run the incremental build. Besides being cheap when nothing has
    # changed, this detects a new Agent Hub token/defaults before flashing and
    # prevents an older binary from being reused silently.
    cmd_build "$board"

    echo "Flashing $board to $port..."
    idf.py -B "$bdir" -DSDKCONFIG="$bdir/sdkconfig" -p "$port" flash
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
