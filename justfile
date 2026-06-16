# justfile — thin wrapper over switch-board.sh and idf.py.
# Real build logic (sdkconfig merge, per-board build dirs, target detection)
# lives in switch-board.sh; these recipes just provide a friendlier front door.
#
# Usage:
#   just                       # list recipes
#   just list                  # list boards + build state
#   just build xiao-esp32-c6
#   just test xiao-esp32-c6                  # file checks + compile-only build
#   just flash xiao-esp32-c6                 # auto-detect port
#   just flash xiao-esp32-c6 /dev/ttyACM1    # explicit port
#   just monitor                             # serial monitor on /dev/ttyACM0
#   just status xiao-esp32-c6
#   just clean xiao-esp32-c6

# Default board when a recipe omits one. Override: just board=xiao-esp32-c3 build
board := "xiao-esp32-c6"

# Default serial port for flash/monitor.
port := env_var_or_default("PORT", "/dev/ttyACM0")

# Default web flasher port.
web_port := env_var_or_default("WEB_PORT", "8081")

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

# Smoke-test a board: check required files, then compile-only build.
test board=board:
    ./test-board.sh {{board}}

# Build if needed, then flash to the given port (auto-detected if omitted).
flash board=board port=port:
    ./switch-board.sh {{board}} flash {{port}}

# Show sdkconfig key settings and build state for a board.
status board=board:
    ./switch-board.sh {{board}} status

# Delete a board's build dir.
clean board=board:
    ./switch-board.sh {{board}} clean

# Copy a built board's firmware into the web flasher firmware slot.
web-firmware board="xiao-esp32-c6-eyes" flasher_board="c6-eyes":
    mkdir -p web-flasher/firmware/{{flasher_board}}
    cp build-{{board}}/bootloader/bootloader.bin web-flasher/firmware/{{flasher_board}}/bootloader.bin
    cp build-{{board}}/partition_table/partition-table.bin web-flasher/firmware/{{flasher_board}}/partition-table.bin
    cp build-{{board}}/ota_data_initial.bin web-flasher/firmware/{{flasher_board}}/ota_data_initial.bin
    cp build-{{board}}/xiaozhi.bin web-flasher/firmware/{{flasher_board}}/xiaozhi.bin
    ls -lh web-flasher/firmware/{{flasher_board}}

# Populate firmware/c6-eyes from the C6 eyes build, then serve the web flasher.
run board="xiao-esp32-c6-eyes" flasher_board="c6-eyes" port=web_port:
    just web-firmware {{board}} {{flasher_board}}
    cd web-flasher && UV_CACHE_DIR=.uv-cache uv run python serve.py {{port}}

# Tail the serial console (idf.py monitor needs a TTY, so use a plain reader).
# seconds=0 reads until interrupted (Ctrl-C); pass a number to stop after N sec.
monitor port=port seconds="0":
    python3 scripts/monitor.py {{port}} {{seconds}}
