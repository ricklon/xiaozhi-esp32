#include "coglet_mcp_tools.h"
#include "coglet_controller.h"
#include "coglet_voice.h"
#include "application.h"
#include "assets/lang_config.h"
#include "board.h"
#include "mcp_server.h"
#include "settings.h"
#include "ssid_manager.h"
#include "wifi_manager.h"

#include <esp_app_desc.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <string>

// Every Coglet controller command is an MCP tool: MCP is the robot's one
// interface (see ricklon/coglet docs/decisions.md). Tool names and arguments
// match the C3 firmware in ricklon/coglet firmware/app/main/mcp.cc. Arguments
// that take fractions are strings here, since xiaozhi's properties are
// integer, string or boolean only.
namespace {

using Args = std::vector<std::string>;

ReturnValue Run(const std::string& verb, const PropertyList& p, const Args& names) {
    std::string line = verb;
    for (const auto& n : names) {
        const auto& prop = p[n];
        if (prop.type() == kPropertyTypeInteger) {
            line += " " + std::to_string(prop.value<int>());
        } else if (prop.type() == kPropertyTypeString) {
            auto v = prop.value<std::string>();
            if (v.find(' ') != std::string::npos) throw std::invalid_argument("Argument must be one word: " + n);
            if (!v.empty()) line += " " + v;
        }
    }
    return CogletController::Instance().Command(line);
}

void Add(const char* name, const std::string& description, const PropertyList& props,
         const std::string& verb, const Args& names = {}) {
    McpServer::GetInstance().AddTool(name, description, props,
        [verb, names](const PropertyList& p) -> ReturnValue { return Run(verb, p, names); });
}

std::string AnimationList(bool eyes) {
    std::string out;
    for (const auto& a : CogletController::Animations()) {
        if (a.eyes != eyes) continue;
        if (!out.empty()) out += ", ";
        out += a.name;
    }
    return out;
}

// Runs work a second from now, after this call's MCP reply has gone out:
// rebooting or closing the session first would swallow the reply.
void AfterReply(std::function<void()> work) {
    auto* job = new std::function<void()>(std::move(work));
    xTaskCreate([](void* arg) {
        auto* job = static_cast<std::function<void()>*>(arg);
        vTaskDelay(pdMS_TO_TICKS(1000));
        (*job)();
        delete job;
        vTaskDelete(nullptr);
    }, "coglet_after", 4096, job, 2, nullptr);
}

// A decimal argument; xiaozhi's properties have no number type.
float Decimal(const std::string& text, const char* name) {
    char* end = nullptr;
    float value = strtof(text.c_str(), &end);
    if (text.empty() || end == nullptr || *end != '\0') {
        throw std::invalid_argument(std::string("Argument must be a decimal string: ") + name);
    }
    return value;
}

std::string MicStatus() {
    auto codec = Board::GetInstance().GetAudioCodec();
    char text[96];
    snprintf(text, sizeof(text), "Mic: %s, gain %.1f", codec->input_enabled() ? "on" : "muted", codec->input_gain());
    return text;
}

const char* SessionState(const Application& app) {
    if (app.IsListeningPaused()) return "quiet";
    switch (app.GetDeviceState()) {
        case kDeviceStateIdle: return "idle";
        case kDeviceStateConnecting: return "connecting";
        case kDeviceStateListening: return "listening";
        case kDeviceStateSpeaking: return "speaking";
        default: return "starting";
    }
}

// The OTA check-in URL for a full URL or a bare host[:port], as !server does
// for the default path.
std::string ServerUrl(const std::string& arg) {
    if (arg.rfind("http://", 0) == 0 || arg.rfind("https://", 0) == 0) return arg;
    if (arg.find('/') != std::string::npos) throw std::invalid_argument("Give a full URL or a bare host");
    return "http://" + arg + (arg.find(':') == std::string::npos ? ":8003" : "") + "/xiaozhi/ota/";
}

Property I(const char* n, int lo, int hi) { return Property(n, kPropertyTypeInteger, lo, hi); }
Property I0(const char* n, int lo, int hi) { return Property(n, kPropertyTypeInteger, 0, lo, hi); }
Property S(const char* n) { return Property(n, kPropertyTypeString); }

}  // namespace

void RegisterCogletMcpTools() {
    const std::string roles =
        " Roles: base, neck_tilt, neck_roll, eye_pan, eye_tilt, lids, jaw, ear_left, ear_right.";
    const std::string window = " Positions are commands, not measurements: no servo reports where it is.";

    // Motion. Needs the robot engaged; positions stay inside calibrated windows.
    Add("self.coglet.gaze", "Point the eyes: horizontal -100 (left) to 100 (right), vertical -100 (down) to 100 (up). "
        "The top lids track the gaze. Needs the robot engaged and both eye servos fitted." + window,
        PropertyList({I0("horizontal", -100, 100), I0("vertical", -100, 100)}), "gaze", {"horizontal", "vertical"});
    Add("self.coglet.head", "Point the head: pan -100 (left) to 100 (right) on the base, tilt -100 (down) to 100 (up), "
        "roll -100 (left) to 100 (right). 0 is centre. Unfitted servos are skipped. Needs the robot engaged." + window,
        PropertyList({I0("pan", -100, 100), I0("tilt", -100, 100), I0("roll", -100, 100)}), "head", {"pan", "tilt", "roll"});
    Add("self.coglet.jaw", "Open the jaw: 0 closed to 100 open. Needs the robot engaged." + window,
        PropertyList({I("open", 0, 100)}), "jaw", {"open"});
    Add("self.coglet.ears", "Set each ear: 0 folded back to 100 forward. Needs the robot engaged." + window,
        PropertyList({I("left", 0, 100), I("right", 0, 100)}), "ears", {"left", "right"});
    Add("self.coglet.lids", "Set the top lids: 0 closed to 100 open, until the next gaze hands them back to gaze "
        "tracking. Needs the robot engaged." + window, PropertyList({I("open", 0, 100)}), "lids", {"open"});
    Add("self.coglet.blink", "One blink, timed on the robot. Needs the robot engaged, the lid servo fitted and a gaze "
        "already set.", PropertyList(), "blink");
    Add("self.coglet.animate", "Start an animation, timed on the robot. Eyes and lids: " + AnimationList(true) +
        ". Rest of the head: " + AnimationList(false) + ". Poll self.coglet.state for completion or faults. "
        "Needs the robot engaged.", PropertyList({S("name")}), "animate", {"name"});
    Add("self.coglet.stop", "Stop any animation or servo test and hold the last commanded pose. Outputs stay on; use "
        "release to turn them off.", PropertyList(), "stop");
    Add("self.coglet.release", "Turn every servo output off and latch it off until engage or calibrate. Doesn't cut "
        "servo power: the rail switch does that.", PropertyList(), "release");
    Add("self.coglet.state", "Robot state: released, calibration mode, animation, servo test, per-role calibration and last "
        "commanded angles, and hardware errors." + window, PropertyList(), "state");

    // Modes.
    Add("self.coglet.engage", "Turn servo outputs on for normal motion. Needs every assigned servo calibrated and "
        "confirmed. Commands no pose by itself. Only from released.", PropertyList(), "engage");
    Add("self.coglet.calibrate", "Turn servo outputs on for calibration and servo tests. Puts the voice in quiet mode "
        "(mic and speaker off; MCP stays connected) until it's turned off. Only from released.",
        PropertyList(), "calibrate");

    // Calibration, one servo at a time.
    Add("self.coglet.fit", "Assign a fitted servo to its standard PCA9685 channel (base 0 through ear_right 8), "
        "unconfirmed, with no range yet. Only while released." + roles, PropertyList({S("role")}), "fit", {"role"});
    Add("self.coglet.configure", "Set one role's calibration by hand: channel (-1 unassigns), low and high endpoint "
        "angles 0-180 (decimal strings), pulse range in microseconds, trim, confirmed 0 or 1. Only while released." + roles,
        PropertyList({S("role"), I("channel", -1, 15), S("low"), S("high"), I("min_us", 300, 3000),
                      I("max_us", 300, 3000), I("trim_us", -2700, 2700), I("confirmed", 0, 1)}),
        "configure", {"role", "channel", "low", "high", "min_us", "max_us", "trim_us", "confirmed"});
    Add("self.coglet.lid_coupling", "Set how the top lids track the gaze: trim and coefficient, 0 to 1 each (decimal "
        "strings). Only while released.", PropertyList({S("trim"), S("coefficient")}), "lid_coupling",
        {"trim", "coefficient"});
    Add("self.coglet.identify", "Wiggle a raw PCA9685 channel for about 1.4 s to see which part is on it. Calibration mode "
        "only.", PropertyList({I("channel", 0, 15)}), "identify", {"channel"});
    Add("self.coglet.explore", "Start hunting a role's endpoints: centre it at 1500 us, then nudge. Calibration mode only." +
        roles, PropertyList({S("role")}), "explore", {"role"});
    Add("self.coglet.nudge", "Move the role being explored by up to 5 degrees (a decimal string), inside the hard "
        "1000-2000 us bound. Calibration mode only.", PropertyList({S("degrees")}), "nudge", {"degrees"});
    Add("self.coglet.mark", "Record the explored position as the role's low or high endpoint (end: low or high). "
        "Clears its confirmation.", PropertyList({S("end")}), "mark", {"end"});
    Add("self.coglet.confirm", "Confirm a role's endpoints after someone has watched the servo reach both." + roles,
        PropertyList({S("role")}), "confirm", {"role"});
    Add("self.coglet.save", "Save the calibration on the robot. Released or calibration mode.", PropertyList(), "save");
    Add("self.coglet.export", "The full calibration and state as JSON, for a backup.", PropertyList(), "export");
    Add("self.coglet.servo", "Move one role to an angle (a decimal string, 0-180) inside its calibrated window. "
        "Calibration mode only." + roles, PropertyList({S("role"), S("degrees")}), "servo", {"role", "degrees"});
    Add("self.coglet.jog", "Move one role by up to 5 degrees (a decimal string) from its last commanded angle, inside "
        "its window. Calibration mode only.", PropertyList({S("role"), S("degrees")}), "jog", {"role", "degrees"});
    Add("self.coglet.test", "Servo test: sweep a role centre, low, high, centre (about 5 s), or with no role every "
        "calibrated role in turn. Calibration mode only. Stop or release ends it." + roles,
        PropertyList({Property("role", kPropertyTypeString, std::string(""))}), "test", {"role"});
    McpServer::GetInstance().AddTool("self.coglet.scan",
        "List the addresses answering on the servo I2C bus (the PCA9685 is 0x40). With sda and scl GPIOs (-1 for "
        "none), rebuilds the bus on those pins first, which releases the robot until a reboot.",
        PropertyList({Property("sda", kPropertyTypeInteger, -1, -1, 48), Property("scl", kPropertyTypeInteger, -1, -1, 48)}),
        [](const PropertyList& p) -> ReturnValue {
            int sda = p["sda"].value<int>(), scl = p["scl"].value<int>();
            if ((sda < 0) != (scl < 0)) throw std::invalid_argument("Give both sda and scl, or neither");
            return CogletController::Instance().Command(
                sda < 0 ? "scan" : "scan " + std::to_string(sda) + " " + std::to_string(scl));
        });

    // Voice, audio and the device. Names and arguments match the C3; optional
    // arguments are strings here, empty meaning "leave as it is".
    auto& mcp = McpServer::GetInstance();
    mcp.AddTool("self.voice.quiet",
        "Quiet mode on or off. On: the mic and speaker stop and the robot neither listens nor speaks; this MCP "
        "connection stays up. Calibration mode turns it on.",
        PropertyList({Property("on", kPropertyTypeBoolean)}),
        [](const PropertyList& p) -> ReturnValue {
            bool on = p["on"].value<bool>();
            Application::GetInstance().SetListeningPaused(on);
            return std::string(on ? "Quiet mode on: not listening, not speaking." : "Quiet mode off.");
        });
    mcp.AddTool("self.voice.wake_word",
        "The wake phrase. With neither argument, says what the robot listens for. phrase: new English words "
        "(\"default\" restores the build's); threshold: 1-99, lower is more sensitive. A change reboots the robot, "
        "which comes back released. Only the typed-phrase build can change it.",
        PropertyList({Property("phrase", kPropertyTypeString, std::string("")),
                      Property("threshold", kPropertyTypeInteger, 0, 0, 99)}),
        [](const PropertyList& p) -> ReturnValue {
            auto phrase = p["phrase"].value<std::string>();
            int threshold = p["threshold"].value<int>();
            if (!phrase.empty() && threshold > 0) {
                throw std::invalid_argument("Set phrase or threshold, one at a time");
            }
            bool changed = false;
            std::string out = CogletWakeWord(threshold > 0 ? "threshold " + std::to_string(threshold) : phrase,
                                             &changed);
            if (changed) AfterReply([] { esp_restart(); });
            return out;
        });
    mcp.AddTool("self.voice.restart", "Close and reopen the voice session. This MCP connection drops for a few seconds.",
        PropertyList(), [](const PropertyList&) -> ReturnValue {
            AfterReply([] { Application::GetInstance().CloseSession(); });
            return std::string("Restarting the voice session.");
        });
    mcp.AddTool("self.audio_speaker.test_tone", "Play a short tone through the speaker.",
        PropertyList(), [](const PropertyList&) -> ReturnValue {
            Application::GetInstance().PlaySound(Lang::Sounds::OGG_SUCCESS);
            return std::string("Playing a test tone.");
        });
    mcp.AddTool("self.audio_mic.status", "Mic mute state and gain.",
        PropertyList(), [](const PropertyList&) -> ReturnValue { return MicStatus(); });
    mcp.AddTool("self.audio_mic.set",
        "Mute or unmute the mic (muted: \"true\" or \"false\"), and/or set its gain (a decimal string). Leave "
        "either empty to keep it.",
        PropertyList({Property("muted", kPropertyTypeString, std::string("")),
                      Property("gain", kPropertyTypeString, std::string(""))}),
        [](const PropertyList& p) -> ReturnValue {
            auto codec = Board::GetInstance().GetAudioCodec();
            auto muted = p["muted"].value<std::string>();
            auto gain = p["gain"].value<std::string>();
            if (!muted.empty() && muted != "true" && muted != "false") {
                throw std::invalid_argument("muted must be \"true\" or \"false\"");
            }
            float g = gain.empty() ? 0 : Decimal(gain, "gain");
            if (!gain.empty() && (g < 0 || g > 64)) throw std::invalid_argument("Argument out of range: gain");
            if (!muted.empty()) codec->EnableInput(muted == "false");
            if (!gain.empty()) codec->SetInputGain(g);
            return MicStatus();
        });
    mcp.AddTool("self.device.status", "Firmware, Wi-Fi, hub URL, voice session state and memory, as text.",
        PropertyList(), [](const PropertyList&) -> ReturnValue {
            auto& wifi = WifiManager::GetInstance();
            Settings settings("wifi", false);
            std::string url = settings.GetString("ota_url");
            if (url.empty()) url = std::string(CONFIG_OTA_URL) + " (default)";
            size_t token = url.find("enrollment_token=");
            if (token != std::string::npos) {
                size_t end = url.find('&', token);
                url.replace(token + 17, end == std::string::npos ? std::string::npos : end - token - 17, "<redacted>");
            }
            auto& app = Application::GetInstance();
            char text[384];
            snprintf(text, sizeof(text),
                     "Firmware %s on coglet\nWi-Fi %s (%s, RSSI %d)\nHub %s\nSession %s\nCamera %s\nFree heap %lu bytes",
                     esp_app_get_description()->version, wifi.GetSsid().c_str(), wifi.GetIpAddress().c_str(),
                     wifi.GetRssi(), url.c_str(), SessionState(app),
                     Board::GetInstance().GetCamera() ? "available" : "not available",
                     (unsigned long)esp_get_free_heap_size());
            return std::string(text);
        });
    mcp.AddTool("self.wifi.list", "Saved Wi-Fi networks.",
        PropertyList(), [](const PropertyList&) -> ReturnValue {
            const auto& list = SsidManager::GetInstance().GetSsidList();
            std::string out = list.empty() ? "No saved networks" : "";
            for (size_t i = 0; i < list.size(); i++) {
                out += (i ? "\n" : "") + std::to_string(i + 1) + ". " + list[i].ssid;
            }
            return out;
        });
    mcp.AddTool("self.wifi.add",
        "Save a Wi-Fi network (2.4 GHz). It's used the next time the robot connects, after a reboot or a dropped "
        "link.",
        PropertyList({Property("ssid", kPropertyTypeString), Property("password", kPropertyTypeString)}),
        [](const PropertyList& p) -> ReturnValue {
            auto ssid = p["ssid"].value<std::string>();
            if (ssid.empty() || ssid.size() > 32) throw std::invalid_argument("ssid must be 1 to 32 characters");
            SsidManager::GetInstance().AddSsid(ssid, p["password"].value<std::string>());
            return "Saved " + ssid;
        });
    mcp.AddTool("self.wifi.remove", "Forget a saved Wi-Fi network by its number in self.wifi.list.",
        PropertyList({I("index", 1, 10)}), [](const PropertyList& p) -> ReturnValue {
            auto& ssids = SsidManager::GetInstance();
            int index = p["index"].value<int>();
            if (index > (int)ssids.GetSsidList().size()) throw std::invalid_argument("No saved network with that number");
            std::string ssid = ssids.GetSsidList()[index - 1].ssid;
            ssids.RemoveSsid(index - 1);
            return "Removed " + ssid;
        });
    mcp.AddTool("self.hub.set_server",
        "Set the agent-hub check-in URL (or a bare host) and reboot. The robot reconnects to that hub.",
        PropertyList({Property("url", kPropertyTypeString)}), [](const PropertyList& p) -> ReturnValue {
            std::string url = ServerUrl(p["url"].value<std::string>());
            Settings("wifi", true).SetString("ota_url", url);
            AfterReply([] { esp_restart(); });
            return std::string("Hub set; rebooting.");
        });
    // Registered before xiaozhi's own self.reboot, which the hub can't see.
    mcp.AddTool("self.reboot", "Restart the robot. It comes back released, with the servo outputs off.",
        PropertyList(), [](const PropertyList&) -> ReturnValue {
            AfterReply([] { esp_restart(); });
            return std::string("Rebooting.");
        });
}
