# Board Expansion Roadmap

## Current Status
- **Working Boards**: 4 (xiao-esp32-c6, xiao-esp32-c3, xiao-esp32-s3-sense, waveshare)
- **Total Boards Available**: 104
- **Boards to Test**: 100

---

## Phase 1: Test Similar Boards (Same Chip Family)

Based on your working boards, prioritize testing boards with the same chip type.

### ESP32-S3 Boards (35 available)
**Priority**: HIGH - You have xiao-esp32-s3-sense working

Recommended boards to test next:
- [ ] lilygo-t-display-s3 - Popular dev board with display
- [ ] m5stack-core-s3 - Well-documented hardware
- [ ] esp-box-3 - Official Espressif dev kit
- [ ] atoms3r-echo-base - Similar to your XIAO but different form factor
- [ ] du-chatx - Compact form factor

### ESP32-C3 Boards (9 available)
**Priority**: MEDIUM - You have xiao-esp32-c3 working

Recommended boards to test next:
- [ ] lichuang-c3-dev - Popular low-cost board
- [ ] magiclick-c3-v2 - Similar to your working board
- [ ] xmini-c3 - Compact form factor

### ESP32-C6 Boards (1 available)
**Priority**: LOW - Only xiao-esp32-c6 exists

---

## Phase 2: Test Different Chip Families

### ESP32-P4 Boards (4 available)
**Priority**: LOW - Newer chip, less community support

### Basic ESP32 Boards (5 available)
**Priority**: LOW - Older chip, limited AI capabilities

---

## Testing Workflow

### Quick Test (Compilation Only)
```bash
just test <board-name>
```

This recipe (wrapping `test-board.sh`) will:
1. Check if all required files exist
2. Attempt to build the firmware
3. Report success/failure
4. Provide manual testing checklist

### Full Test (Hardware Required)
1. Flash the board: `just flash <board-name>`
2. Connect serial monitor
3. Follow the checklist in test-board.sh output
4. Update BOARDS_STATUS.md with results

---

## Automation Ideas

### Future Enhancements
- [ ] Auto-detect board hardware from config files
- [ ] Batch test compilation for all boards
- [ ] CI/CD pipeline for automated building
- [ ] Hardware-in-the-loop testing with test jig

---

## Resources

- **Board Status**: `cat BOARDS_STATUS.md`
- **Testing**: `just test <board-name>`
- **Board Template**: `cat BOARD_ADDITION_TEMPLATE.md`
- **Build/flash/monitor**: `just <recipe> <board-name>` (see `just --list`)

---

## Next Actions

1. **Today**: Test one ESP32-S3 board similar to xiao-esp32-s3-sense
   - Run: `just test lilygo-t-display-s3`
   - If builds, flash and test hardware

2. **This Week**: Test 3-5 boards from Phase 1 list
   - Update BOARDS_STATUS.md after each test
   - Document any issues in the board's Notes column

3. **Ongoing**: Expand to 10+ working boards
   - Focus on popular boards (M5Stack, LilyGO, Waveshare)
   - Document common patterns for each chip family
