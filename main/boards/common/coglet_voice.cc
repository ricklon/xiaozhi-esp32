#include "coglet_voice.h"
#include "application.h"
#include "coglet_controller.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "Coglet"

namespace {

constexpr int kLoopMs = 500;
constexpr int kReopenLoops = 20;  // a dropped session is retried every 10 s while quiet

// Calibration needs silence, so entering calibration mode turns quiet on. Leaving
// it doesn't turn quiet off: the LLM can call self.coglet.release over MCP, and an
// auto-resume let the robot talk its own way out of a calibration session. Quiet
// ends only when someone turns it off (self.voice.quiet or !quiet off).
//
// Quiet keeps the session open, because MCP rides in it: a robot in calibration
// mode has to stay reachable for release, state and the calibration tools.
void VoiceLoop(void*) {
    auto& app = Application::GetInstance();
    app.SetKeepSessionWhenPaused(true);
    DeviceState last_state = kDeviceStateUnknown;
    bool quieted = false, was_paused = false;
    int loops = 0;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(kLoopMs));
        loops++;
        if (CogletController::Instance().CalibrationMode() && !quieted) {
            quieted = true;
            app.SetListeningPaused(true);
            ESP_LOGI(TAG, "Calibration mode: voice quiet until quiet is turned off");
        } else if (quieted && !app.IsListeningPaused()) {
            quieted = false;
        }
        DeviceState state = app.GetDeviceState();
        if (app.IsListeningPaused()) {
            if (state == kDeviceStateIdle && loops % kReopenLoops == 0) {
                app.ReopenPausedSession();
            }
            was_paused = true;
            last_state = state;
            continue;
        }
        if (was_paused) {
            // Idle may not have changed while quiet, so the rule below wouldn't fire.
            was_paused = false;
            ESP_LOGI(TAG, "Quiet off: listening again");
            if (state == kDeviceStateIdle) {
                app.ToggleChatState();
                last_state = state;
                continue;
            }
        }
        // Always listening: reopen about a second after every drop to idle.
        if (state == kDeviceStateIdle && last_state != kDeviceStateIdle) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            if (app.GetDeviceState() == kDeviceStateIdle) {
                app.ToggleChatState();
            }
        }
        last_state = state;
    }
}

}  // namespace

void StartCogletVoiceLoop() {
    xTaskCreate(VoiceLoop, "auto_connect", 4096, nullptr, 3, nullptr);
}
