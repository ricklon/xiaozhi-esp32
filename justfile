# justfile — thin wrapper over switch-board.sh and idf.py.
# Real build logic (sdkconfig merge, per-board build dirs, target detection)
# lives in switch-board.sh; these recipes just provide a friendlier front door.
#
# Usage:
#   just                       # list recipes
#   just list                  # list boards + build state
#   just build xiao-esp32-c6
#   just flash xiao-esp32-c6                 # auto-detect port
#   just flash xiao-esp32-c6 /dev/ttyACM1    # explicit port
#   just monitor                             # serial monitor on /dev/ttyACM0
#   just status xiao-esp32-c6
#   just clean xiao-esp32-c6

# Default board when a recipe omits one. Override: just board=xiao-esp32-c3 build
board := "xiao-esp32-c6"

# Default serial port for flash/monitor.
port := env_var_or_default("PORT", "/dev/ttyACM0")

# Show available recipes (default).
default:
    @just --list

# List all boards and their build state.
list:
    ./switch-board.sh list

# Configure a board's build dir (first-time setup).
setup board=board:
    ./switch-board.sh {{board}} setup

# Build a board (runs setup automatically if needed).
build board=board:
    ./switch-board.sh {{board}} build

# Build if needed, then flash to the given port (auto-detected if omitted).
flash board=board port=port:
    ./switch-board.sh {{board}} flash {{port}}

# Show sdkconfig key settings and build state for a board.
status board=board:
    ./switch-board.sh {{board}} status

# Delete a board's build dir.
clean board=board:
    ./switch-board.sh {{board}} clean

# Tail the serial console (idf.py monitor needs a TTY, so use a plain reader).
# seconds=0 reads until interrupted (Ctrl-C); pass a number to stop after N sec.
monitor port=port seconds="0":
    python3 scripts/monitor.py {{port}} {{seconds}}
