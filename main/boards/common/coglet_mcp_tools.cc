#include "coglet_mcp_tools.h"
#include "coglet_controller.h"
#include "application.h"
#include "mcp_server.h"

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

    // Voice. Quiet keeps this session, and MCP with it, open.
    McpServer::GetInstance().AddTool("self.voice.quiet",
        "Quiet mode on or off. On: the mic and speaker stop and the robot neither listens nor speaks; this MCP "
        "connection stays up. Calibration mode turns it on.",
        PropertyList({Property("on", kPropertyTypeBoolean)}),
        [](const PropertyList& p) -> ReturnValue {
            bool on = p["on"].value<bool>();
            Application::GetInstance().SetListeningPaused(on);
            return std::string(on ? "Quiet mode on: not listening, not speaking." : "Quiet mode off: listening resumed.");
        });
}
