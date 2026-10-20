# Coglet — XIAO ESP32-S3 Sense

Coglet is documented, tested and released from its own project,
[github.com/ricklon/coglet](https://github.com/ricklon/coglet). That repo holds
the servo controller (`firmware/coglet_controller`), the hardware files, the web
flasher, and the docs: wiring, calibration, operation, and how agent-hub drives
Coglet over MCP.

This folder is the thin board layer xiaozhi's build needs: pins (`config.h`),
the board class (`coglet.cc`), and the `sdkconfig` defaults. The MCP tool
registration shared with the C3 profile is `../common/coglet_mcp_tools.cc`.

Build from the Coglet repo (`just build s3`). Building here works too when the
Coglet repo is checked out next to this one as `../coglet`, or when
`COGLET_CONTROLLER_DIR` points at its `firmware/coglet_controller`:

```bash
./switch-board.sh coglet build
```
