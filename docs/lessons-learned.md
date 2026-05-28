# Lessons Learned

Running log of non-obvious gotchas hit while building/flashing this project, with
the fix, so we don't re-debug them. Newest first.

---

## ESP-IDF WiFi-lib submodule drift → `undefined reference to esp_wifi_*` at link

**Date:** 2026-05-28 · **Boards affected:** all (hit on `xiao-esp32-c6`)

### Symptom
Build compiles every `.cc` cleanly, then **fails at the link step** with errors like:

```
undefined reference to `esp_wifi_internal_update_modem_sleep_default_params'
undefined reference to `pm_beacon_offset_funcs_empty_init'
undefined reference to `esp_wifi_ap_get_sae_ext_config_internal'
undefined reference to `esp_wifi_ap_get_gtk_rekeying_config_internal'
undefined reference to `esp_wifi_ap_set_group_mgmt_cipher_internal'
collect2: error: ld returned 1 exit status
```

### Root cause
**Toolchain drift, not project code.** ESP-IDF ships its WiFi/PHY/coex stacks as
precompiled blobs in git submodules. If those submodules in `~/esp/esp-idf` are
checked out at the wrong commit (a leading `+` in `git submodule status`), the
blobs are missing newer internal symbols that the IDF **source** at the pinned
version (v5.5.2) expects → unresolved symbols at link.

### Fix (persists — it's in the ESP-IDF install, outside this repo)
```bash
cd ~/esp/esp-idf
git submodule update --init --recursive
# or just the WiFi-related submodules:
git submodule update --init components/esp_wifi/lib components/esp_phy/lib components/esp_coex/lib
```
Verify: `git submodule status components/esp_wifi/lib` — the leading `+` should be gone.
Confirm the symbol is now in the blob:
```bash
nm ~/esp/esp-idf/components/esp_wifi/lib/esp32c6/*.a | grep esp_wifi_ap_get_sae_ext_config_internal
```

### Version note
This project pins **ESP-IDF v5.5.2** (CI `build.yml` / `release-firmware.yml`, and
local toolchain). Min is 5.4. Keep the IDF submodules matched to the v5.5.2 checkout.

---

## Don't pipe builds through `tail`/`head` — it masks the real exit code

**Date:** 2026-05-28

### Symptom
`just build <board> 2>&1 | tail -60` reported **exit code 0** even though the build
had actually failed at link. The reported status was `tail`'s exit code, not the
build's, so a real failure looked like success.

### Fix
Redirect to a file and check the build's own status, or use `pipefail`:
```bash
just build xiao-esp32-c6 > /tmp/build.log 2>&1; echo "EXIT=$?"
# or
set -o pipefail
```
Then grep the log for `Project build complete` / `FAILED` / `error:`.
