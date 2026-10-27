#include "coglet_voice.h"
#include "application.h"
#include "coglet_controller.h"
#include "settings.h"

#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cctype>
#include <cstdlib>
#include <stdexcept>

#define TAG "Coglet"

namespace {

#if CONFIG_USE_AFE_WAKE_WORD || CONFIG_USE_ESP_WAKE_WORD || CONFIG_USE_CUSTOM_WAKE_WORD
constexpr bool kWakeWord = true;
#else
constexpr bool kWakeWord = false;
#endif

constexpr int kLoopMs = 500;
constexpr int kReopenLoops = 20;           // a closed session is retried every 10 s
constexpr int64_t kFollowUpUs = 12000000;  // after a wake word: listen this long past the last voice

int64_t Now() {
    return esp_timer_get_time();
}

// The session carries MCP, so it stays open in every mode; only the mic changes.
//
// With a wake word (the S3): the session opens without listening, and the wake
// word starts listening. Listening ends 12 s after the last voice, which leaves
// room for follow-up questions, and the robot waits for the wake word again
// with the session still open.
//
// Without one (the fork's C3 profile): always listening, reopened about a
// second after every drop to idle.
//
// Calibration needs silence, so entering calibration mode turns quiet on.
// Leaving it doesn't turn quiet off: the LLM can call self.coglet.release over
// MCP, and an auto-resume let the robot talk its own way out of a calibration
// session. Quiet ends only when someone turns it off (self.voice.quiet or
// !quiet off). Quiet keeps the session open too.
void VoiceLoop(void*) {
    auto& app = Application::GetInstance();
    app.SetKeepSessionWhenPaused(true);
    DeviceState last_state = kDeviceStateUnknown;
    bool quieted = false, was_paused = false;
    int64_t heard_at = 0;  // entering Listening, or the last voice heard there
    int loops = 0;
    ESP_LOGI(TAG, "Voice: %s", kWakeWord ? "wake word, session always open" : "always listening");
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
        bool idle = state == kDeviceStateIdle;
        // Reopen a dropped session at once, and retry every 10 s after that.
        bool reopen = loops % kReopenLoops == 0 || last_state != kDeviceStateIdle;
        if (idle && reopen && (kWakeWord || app.IsListeningPaused())) {
            app.OpenSessionWithoutListening();
        }
        if (app.IsListeningPaused()) {
            was_paused = true;
            last_state = state;
            continue;
        }
        bool resumed = was_paused;
        was_paused = false;
        if (resumed) ESP_LOGI(TAG, "Quiet off");

        if (kWakeWord) {
            if (state == kDeviceStateListening) {
                if (last_state != kDeviceStateListening || app.IsVoiceDetected()) heard_at = Now();
                if (Now() - heard_at > kFollowUpUs) {
                    ESP_LOGI(TAG, "Nothing heard for %lld s: waiting for the wake word",
                             kFollowUpUs / 1000000);
                    app.StopListening();
                }
            }
            if (resumed && idle) app.OpenSessionWithoutListening();
        } else if (idle && (resumed || last_state != kDeviceStateIdle)) {
            // Always listening: reopen about a second after every drop to idle.
            vTaskDelay(pdMS_TO_TICKS(1000));
            if (app.GetDeviceState() == kDeviceStateIdle && !app.IsListeningPaused()) {
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

std::string CogletWakeWord(const std::string& args, bool* changed) {
    *changed = false;
#if CONFIG_USE_CUSTOM_WAKE_WORD
    Settings settings("wake_word", true);
    std::string phrase = settings.GetString("phrase");
    int threshold = settings.GetInt("threshold", 0);
    if (args.empty() || args == "status") {
        return std::string("Wake phrase: \"") + (phrase.empty() ? CONFIG_CUSTOM_WAKE_WORD : phrase) + "\"" +
               (phrase.empty() ? " (default)" : " (set on this robot)") + ", threshold " +
               std::to_string(threshold > 0 ? threshold : CONFIG_CUSTOM_WAKE_WORD_THRESHOLD) +
               (threshold > 0 ? "" : " (default)");
    }
    if (args == "default") {
        settings.EraseKey("phrase");
        settings.EraseKey("threshold");
        *changed = true;
        return std::string("Wake phrase back to \"") + CONFIG_CUSTOM_WAKE_WORD + "\"; rebooting.";
    }
    if (args.rfind("threshold", 0) == 0) {
        char* end = nullptr;
        long value = strtol(args.c_str() + 9, &end, 10);
        if (args.size() <= 10 || args[9] != ' ' || *end != '\0' || value < 1 || value > 99) {
            throw std::invalid_argument("threshold is 1 to 99; lower is more sensitive");
        }
        settings.SetInt("threshold", (int)value);
        *changed = true;
        return "Wake threshold " + std::to_string(value) + "; rebooting.";
    }
    // Lower-case English words, single spaces: what the phrase model expects.
    std::string clean;
    for (char c : args) {
        if (isalpha((unsigned char)c) || c == '\'') {
            clean += (char)tolower((unsigned char)c);
        } else if (c == ' ') {
            if (!clean.empty() && clean.back() != ' ') clean += ' ';
        } else {
            throw std::invalid_argument("A wake phrase is English words: letters, spaces and apostrophes");
        }
    }
    while (!clean.empty() && clean.back() == ' ') clean.pop_back();
    if (clean.size() < 3 || clean.size() > 40) throw std::invalid_argument("A wake phrase is 3 to 40 characters");
    settings.SetString("phrase", clean);
    *changed = true;
    return "Wake phrase \"" + clean + "\"; rebooting.";
#elif CONFIG_USE_AFE_WAKE_WORD || CONFIG_USE_ESP_WAKE_WORD
    if (args.empty() || args == "status") {
        return "Wake word: Computer (a trained model). For your own phrase, flash the Coglet S3 typed-phrase build.";
    }
    throw std::invalid_argument("This build's wake word is a trained model (Computer); flash the typed-phrase build "
                                "to choose your own");
#else
    if (args.empty() || args == "status") return "No wake word: this robot is always listening.";
    throw std::invalid_argument("No wake word on this build: it's always listening");
#endif
}
